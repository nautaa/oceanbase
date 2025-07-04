/**
 * Copyright (c) 2023 OceanBase
 * OceanBase CE is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#define USING_LOG_PREFIX SQL_ENG
#include "ob_parquet_table_row_iter.h"
#include "sql/engine/basic/ob_arrow_basic.h"
#include "sql/engine/expr/ob_expr_get_path.h"
#include "share/external_table/ob_external_table_utils.h"
#include "sql/engine/expr/ob_datum_cast.h"
#include <parquet/api/reader.h>

namespace oceanbase
{
using namespace share::schema;
using namespace common;
using namespace share;
namespace sql {

bool mem_zero_detect(void *buf, size_t n)
{
  size_t size = n;
  if (size == 0)
      return true;
  uint8_t * ptr = (uint8_t *)buf;
  if (*ptr == 0 && memcmp(ptr, ptr + 1, size - 1) == 0)
    return true;
  return false;
}

ObParquetTableRowIterator::~ObParquetTableRowIterator()
{
  for (int i = 0; i < column_readers_.count(); i++) {
    column_readers_.at(i) = NULL;
  }
  file_prebuffer_.destroy();
  column_range_slices_.destroy();
}
int ObParquetTableRowIterator::init(const storage::ObTableScanParam *scan_param)
{
  int ret = OB_SUCCESS;
  ObEvalCtx &eval_ctx = scan_param->op_->get_eval_ctx();
  mem_attr_ = ObMemAttr(MTL_ID(), "ParquetRowIter");
  allocator_.set_attr(mem_attr_);
  str_res_mem_.set_attr(mem_attr_);
  arrow_alloc_.init(MTL_ID());
  make_external_table_access_options(eval_ctx.exec_ctx_.get_my_session()->get_stmt_type());
  if (options_.enable_prebuffer_) {
    OZ(file_prebuffer_.init(options_.cache_options_, scan_param->timeout_));
  }
  OZ (ObExternalTableRowIterator::init(scan_param));
  OZ (ObExternalTablePushdownFilter::init(scan_param));

  if (OB_SUCC(ret)) {
    ObArray<ObExpr*> file_column_exprs;
    ObArray<ObExpr*> mapping_column_exprs;
    ObArray<uint64_t> mapping_column_ids;
    ObArray<ObExpr*> file_meta_column_exprs;
    bool mapping_generated = !scan_param->ext_mapping_column_exprs_->empty()
                              && !scan_param->ext_mapping_column_ids_->empty();
    for (int i = 0; OB_SUCC(ret) && i < scan_param->ext_file_column_exprs_->count(); i++) {
      ObExpr* ext_file_column_expr = scan_param->ext_file_column_exprs_->at(i);
      if (OB_ISNULL(ext_file_column_expr)) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("unexpected ptr", K(ret));
      } else if (ext_file_column_expr->type_ == T_PSEUDO_EXTERNAL_FILE_URL
                 || ext_file_column_expr->type_ == T_PSEUDO_PARTITION_LIST_COL) {
        OZ (file_meta_column_exprs.push_back(ext_file_column_expr));
      } else if (ext_file_column_expr->type_ == T_PSEUDO_EXTERNAL_FILE_COL) {
        OZ (file_column_exprs.push_back(ext_file_column_expr));
        OZ (mapping_column_exprs.push_back(mapping_generated
            ? scan_param->ext_mapping_column_exprs_->at(i) : nullptr));
        OZ (mapping_column_ids.push_back(mapping_generated
            ? scan_param->ext_mapping_column_ids_->at(i) : OB_INVALID_ID));
      } else {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("unexpected expr", KPC(ext_file_column_expr));
      }
    }
    OZ (file_column_exprs_.assign(file_column_exprs));
    OZ (mapping_column_exprs_.assign(mapping_column_exprs));
    OZ (mapping_column_ids_.assign(mapping_column_ids));
    OZ (file_meta_column_exprs_.assign(file_meta_column_exprs));
    OZ (state_.init(file_column_exprs.count(), allocator_));
    if (file_column_exprs_.count() > 0) {
      OZ (column_indexs_.allocate_array(allocator_, file_column_exprs_.count()));
      OZ (column_readers_.allocate_array(allocator_, file_column_exprs_.count()));
      OZ (load_funcs_.allocate_array(allocator_, file_column_exprs_.count()));
    }
    LOG_DEBUG("check exprs", K(file_column_exprs), K(file_meta_column_exprs), KPC(scan_param->ext_file_column_exprs_));
  }

  if (OB_SUCC(ret) && OB_ISNULL(bit_vector_cache_)) {
    void *mem = nullptr;
    if (OB_ISNULL(mem = allocator_.alloc(ObBitVector::memory_size(eval_ctx.max_batch_size_)))) {
      ret = OB_ALLOCATE_MEMORY_FAILED;
      LOG_WARN("failed to alloc memory for skip", K(ret), K(eval_ctx.max_batch_size_));
    } else {
      bit_vector_cache_ = to_bit_vector(mem);
      bit_vector_cache_->reset(eval_ctx.max_batch_size_);
    }
  }

  if (OB_SUCC(ret)) {
    OZ (def_levels_buf_.allocate_array(allocator_, eval_ctx.max_batch_size_));
    OZ (rep_levels_buf_.allocate_array(allocator_, eval_ctx.max_batch_size_));
  }

  if (OB_SUCC(ret)) {
    OZ (file_url_ptrs_.allocate_array(allocator_, eval_ctx.max_batch_size_));
    OZ (file_url_lens_.allocate_array(allocator_, eval_ctx.max_batch_size_));
  }

  return ret;
}

int ObParquetTableRowIterator::next_file()
{
#define BEGIN_CATCH_EXCEPTIONS try {
#define END_CATCH_EXCEPTIONS                                                                       \
  } catch (const ObErrorCodeException &ob_error) {                                                 \
    if (OB_SUCC(ret)) {                                                                            \
      ret = ob_error.get_error_code();                                                             \
      LOG_WARN("fail to read file", K(ret));                                                       \
    }                                                                                              \
  } catch (const ::parquet::ParquetStatusException &e) {                                           \
    if (OB_SUCC(ret)) {                                                                            \
      status = e.status();                                                                         \
      ret = OB_INVALID_EXTERNAL_FILE;                                                              \
      LOG_WARN("unexpected error", K(ret), "Info", e.what());                                      \
    }                                                                                              \
  } catch (const ::parquet::ParquetException &e) {                                                 \
    if (OB_SUCC(ret)) {                                                                            \
      ret = OB_INVALID_EXTERNAL_FILE;                                                              \
      LOG_USER_ERROR(OB_INVALID_EXTERNAL_FILE, e.what());                                          \
      LOG_WARN("unexpected error", K(ret), "Info", e.what());                                      \
    }                                                                                              \
  } catch (...) {                                                                                  \
    if (OB_SUCC(ret)) {                                                                            \
      ret = OB_ERR_UNEXPECTED;                                                                     \
      LOG_WARN("unexpected error", K(ret));                                                        \
    }                                                                                              \
  }                                                                                                \

  int ret = OB_SUCCESS;
  ObEvalCtx &eval_ctx = scan_param_->op_->get_eval_ctx();
  ObString location = scan_param_->external_file_location_;
  int64_t task_idx = 0;
  arrow::Status status = arrow::Status::OK();

  do {
    ret = OB_SUCCESS;
    for (int i = 0; OB_SUCC(ret) && i < column_indexs_.count(); i++) {
      column_readers_.at(i).reset();
    }
    file_meta_.reset();
    file_reader_.reset();
    status = arrow::Status::OK();
    if ((task_idx = state_.file_idx_++) >= scan_param_->key_ranges_.count()) {
      ret = OB_ITER_END;
    } else {
      state_.cur_file_url_ = scan_param_->key_ranges_.at(task_idx).get_start_key().get_obj_ptr()[ObExternalTableUtils::FILE_URL].get_string();
      url_.reuse();
      const char *split_char = "/";
      OZ (url_.append_fmt("%.*s%s%.*s", location.length(), location.ptr(),
                                        (location.empty() || location[location.length() - 1] == '/') ? "" : split_char,
                                        state_.cur_file_url_.length(), state_.cur_file_url_.ptr()));

      if (OB_SUCC(ret)) {
        BEGIN_CATCH_EXCEPTIONS
          std::shared_ptr<ObArrowFile> cur_file = std::make_shared<ObArrowFile>(
            data_access_driver_, url_.ptr(), &arrow_alloc_);
          ObExternalFileUrlInfo file_info(scan_param_->external_file_location_,
                                          scan_param_->external_file_access_info_, url_.string());
          ObExternalFileCacheOptions cache_options(options_.enable_page_cache_,
                                                   options_.enable_disk_cache_);
          if (options_.enable_prebuffer_) {
            cur_file->set_file_prebuffer(&file_prebuffer_);
          }
          cur_file->set_timeout_timestamp(scan_param_->timeout_);
          read_props_.enable_buffered_stream();
          OZ(cur_file.get()->open(file_info, cache_options));
          OX(file_reader_ = parquet::ParquetFileReader::Open(cur_file, read_props_));
          OX(file_meta_ = file_reader_->metadata());
          if (OB_SUCC(ret)) {
            ObString expr_file_url;
            if (data_access_driver_.get_storage_type() == OB_STORAGE_FILE) {
              ObSqlString full_name;
              if (ip_port_.empty()) {
                OZ(gen_ip_port(allocator_));
              }
              OZ(full_name.append_fmt("%.*s%%%.*s", ip_port_.length(), ip_port_.ptr(),
                                      state_.cur_file_url_.length(), state_.cur_file_url_.ptr()));
              OZ(ob_write_string(allocator_, full_name.string(), expr_file_url));
            } else {
              expr_file_url = state_.cur_file_url_;
            }
            for (int i = 0; OB_SUCC(ret) && i < eval_ctx.max_batch_size_; i++) {
              file_url_ptrs_.at(i) = expr_file_url.ptr();
              file_url_lens_.at(i) = expr_file_url.length();
            }
          }
        END_CATCH_EXCEPTIONS
      }

      LOG_DEBUG("current external file", K(url_), K(ret));
    }
  } while (OB_OBJECT_NOT_EXIST == ret || OB_HDFS_PATH_NOT_FOUND == ret ||
           (OB_INVALID_EXTERNAL_FILE == ret && status.IsInvalid())); // skip not exist or empty file

  if (OB_SUCC(ret)) {
    int64_t part_id = scan_param_->key_ranges_.at(task_idx).get_start_key().get_obj_ptr()[ObExternalTableUtils::PARTITION_ID].get_int();
    if (part_id != 0 && state_.part_id_ != part_id) {
      state_.part_id_ = part_id;
      OZ (calc_file_partition_list_value(part_id, allocator_, state_.part_list_val_));
    }

    state_.cur_file_id_ = scan_param_->key_ranges_.at(task_idx).get_start_key().get_obj_ptr()[ObExternalTableUtils::FILE_ID].get_int();
    OZ (ObExternalTableUtils::resolve_line_number_range(scan_param_->key_ranges_.at(task_idx),
                                                        ObExternalTableUtils::ROW_GROUP_NUMBER,
                                                        state_.cur_row_group_idx_,
                                                        state_.end_row_group_idx_));
    OX (state_.end_row_group_idx_ = std::min((int64_t)(file_meta_->num_row_groups()), state_.end_row_group_idx_));

    BEGIN_CATCH_EXCEPTIONS
      for (int i = 0; OB_SUCC(ret) && i < file_column_exprs_.count(); i++) {
        ObDataAccessPathExtraInfo *data_access_info =
            static_cast<ObDataAccessPathExtraInfo *>(file_column_exprs_.at(i)->extra_info_);
        int column_index =
            file_meta_->schema()->ColumnIndex(std::string(data_access_info->data_access_path_.ptr(),
                                                          data_access_info->data_access_path_.length()));
        const parquet::ColumnDescriptor *col_desc = NULL;
        if (column_index < 0) {
          ret = OB_INVALID_EXTERNAL_FILE_COLUMN_PATH;
          LOG_USER_ERROR(OB_INVALID_EXTERNAL_FILE_COLUMN_PATH,
                         data_access_info->data_access_path_.length(),
                         data_access_info->data_access_path_.ptr());
        } else {
          col_desc = file_meta_->schema()->Column(column_index);
          load_funcs_.at(i) = DataLoader::select_load_function(file_column_exprs_.at(i)->datum_meta_, col_desc);
          if (OB_ISNULL(load_funcs_.at(i))
              || col_desc->max_repetition_level() != 0) {
            ret = OB_EXTERNAL_FILE_COLUMN_TYPE_MISMATCH;
            std::string p_type = col_desc->logical_type()->ToString();
            int64_t pos = 0;
            ObArrayWrap<char> buf;
            ObDatumMeta &meta = file_column_exprs_.at(i)->datum_meta_;
            const char *ob_type = ob_obj_type_str(file_column_exprs_.at(i)->datum_meta_.type_);
            if (OB_SUCCESS == buf.allocate_array(allocator_, 100)) {
              ObArray<ObString> extended_type_info;
              if (ob_is_collection_sql_type(meta.type_)) {
                int tmp_ret = OB_SUCCESS;
                const ObSqlCollectionInfo *coll_info = NULL;
                uint16_t subschema_id = file_column_exprs_.at(i)->obj_meta_.get_subschema_id();
                ObSubSchemaValue value;
                if (OB_SUCCESS != (tmp_ret = eval_ctx.exec_ctx_.get_sqludt_meta_by_subschema_id(subschema_id, value))) {
                  LOG_WARN("failed to get subschema ctx", K(tmp_ret));
                } else if (FALSE_IT(coll_info = reinterpret_cast<const ObSqlCollectionInfo *>(value.value_))) {
                } else if (OB_SUCCESS != (tmp_ret = extended_type_info.push_back(coll_info->get_def_string()))) {
                  LOG_WARN("failed to push back to array", K(tmp_ret), KPC(coll_info));
                }
              }
              ob_sql_type_str(buf.get_data(), buf.count(), pos, meta.type_,
                              OB_MAX_VARCHAR_LENGTH, meta.precision_, meta.scale_, meta.cs_type_, extended_type_info);
              if (pos < buf.count()) {
                buf.at(pos++) = '\0';
                ob_type = buf.get_data();
              }
            }
            LOG_WARN("not supported type", K(ret), K(file_column_exprs_.at(i)->datum_meta_),
                     K(ObString(p_type.length(), p_type.data())),
                     K(col_desc->physical_type()),
                     "rep_level", col_desc->max_repetition_level());
            LOG_USER_ERROR(OB_EXTERNAL_FILE_COLUMN_TYPE_MISMATCH, p_type.c_str(), ob_type);
          } else {
            column_indexs_.at(i) = column_index;
            LOG_DEBUG("mapped ob type", K(column_index), "column type",
                      file_meta_->schema()->Column(column_index)->physical_type(), "path",
                      data_access_info->data_access_path_);
          }
        }
      }
    END_CATCH_EXCEPTIONS
  }
  if (OB_SUCC(ret)
      && OB_FAIL(prepare_filter_col_meta(column_indexs_, mapping_column_ids_, mapping_column_exprs_))) {
    LOG_WARN("fail to prepare filter col meta", K(ret), K(column_indexs_.count()), K(mapping_column_exprs_.count()));
  }
#undef BEGIN_CATCH_EXCEPTIONS
#undef END_CATCH_EXCEPTIONS
  return ret;
}

int ObParquetTableRowIterator::next_row_group()
{
  int ret = OB_SUCCESS;
  bool find_row_group = false;
  //init all meta
  while (OB_SUCC(ret) && !find_row_group) {
    while (OB_SUCC(ret) && state_.cur_row_group_idx_ > state_.end_row_group_idx_) {
      if (OB_FAIL(next_file())) {
        if (OB_ITER_END != ret) {
          LOG_WARN("fail to next row group", K(ret));
        }
      }
    }
    if (OB_SUCC(ret)) {
      int64_t cur_row_group = (state_.cur_row_group_idx_++) - 1;
      bool can_skip = false;
      std::shared_ptr<parquet::RowGroupReader> rg_reader = file_reader_->RowGroup(cur_row_group);
      ObEvalCtx::TempAllocGuard alloc_guard(scan_param_->op_->get_eval_ctx());
      ParquetMinMaxFilterParamBuilder param_builder(this, rg_reader, file_meta_, alloc_guard.get_allocator());
      if (OB_FAIL(ObExternalTablePushdownFilter::apply_skipping_index_filter(
          PushdownLevel::ROW_GROUP, param_builder, can_skip))) {
        LOG_WARN("failed to apply skip index", K(ret));
      } else if (can_skip) {
        LOG_DEBUG("print skip rg", K(state_.cur_row_group_idx_), K(state_.end_row_group_idx_));
        continue;
      } else if (options_.enable_prebuffer_ && OB_FAIL(pre_buffer(rg_reader))) {
        LOG_WARN("failed to pre buffer", K(ret));
      } else {
        find_row_group = true;
        try {
          memset(pointer_cast<char *> (&state_.cur_row_group_read_row_counts_.at(0)), 0, sizeof(int64_t) * state_.cur_row_group_read_row_counts_.count());
          state_.cur_row_group_row_count_ = file_meta_->RowGroup(cur_row_group)->num_rows();
          for (int i = 0; OB_SUCC(ret) && i < column_indexs_.count(); i++) {
            std::unique_ptr<parquet::PageReader> page_reader = rg_reader->GetColumnPageReader(column_indexs_.at(i));
            //page_reader->set_data_page_filter(read_pages);
            column_readers_.at(i) = parquet::ColumnReader::Make(
                                    rg_reader->metadata()->schema()->Column(column_indexs_.at(i)),
                                    std::move(page_reader));
          }
        } catch (const ObErrorCodeException &ob_error) {
          if (OB_SUCC(ret)) {
            ret = ob_error.get_error_code();
            LOG_WARN("fail to read file", K(ret));
          }
        } catch(const std::exception& e) {
          ret = OB_ERR_UNEXPECTED;
          LOG_WARN("unexpected index", K(ret), "Info", e.what(), K(cur_row_group), K(column_indexs_));
        } catch(...) {
          ret = OB_ERR_UNEXPECTED;
          LOG_WARN("unexpected index", K(ret), K(cur_row_group), K(column_indexs_));
        }
      }
    }
  }
  return ret;
}

ObExternalTableAccessOptions&
ObParquetTableRowIterator::make_external_table_access_options(stmt::StmtType stmt_type)
{
  if (stmt::T_INSERT == stmt_type) {
    options_ = ObExternalTableAccessOptions::disable_cache_defaults();
  } else {
    options_ = ObExternalTableAccessOptions::lazy_defaults();
  }
  return options_;
}

int ObParquetTableRowIterator::DataLoader::load_data_for_col(LOAD_FUNC &func)
{
  return (this->*func)();
}

ObParquetTableRowIterator::DataLoader::LOAD_FUNC ObParquetTableRowIterator::DataLoader::select_load_function(
    const ObDatumMeta &datum_type, const parquet::ColumnDescriptor *col_desc)
{
  LOAD_FUNC func = NULL;
  const parquet::LogicalType* log_type = col_desc->logical_type().get();
  parquet::Type::type phy_type = col_desc->physical_type();
  bool no_log_type = log_type->is_none();
  if (no_log_type && parquet::Type::BOOLEAN == phy_type) {
    if (ob_is_number_or_decimal_int_tc(datum_type.type_)) {
      func = &DataLoader::load_decimal_any_col;
    } else if (ob_is_integer_type(datum_type.type_)) {
      func = &DataLoader::load_bool_to_int64_vec;
    }
  } else if ((no_log_type || log_type->is_int()) && ob_is_integer_type(datum_type.type_)) {
    //convert parquet int storing as int32/int64 to
    // ObTinyIntType/ObSmallIntType/ObMediumIntType/ObInt32Type/ObIntType using int64_t memory layout
    // ObUTinyIntType/ObUSmallIntType/ObUMediumIntType/ObUInt32Type/ObUInt64Type using uint64_t memory layout
    if (parquet::Type::INT64 == phy_type) {
      func = &DataLoader::load_int64_to_int64_vec;
    } else if (parquet::Type::INT32 == phy_type) {
      if (log_type->is_int() && !static_cast<const parquet::IntLogicalType*>(log_type)->is_signed()) {
        func = &DataLoader::load_uint32_to_int64_vec;
      } else {
        func = &DataLoader::load_int32_to_int64_vec;
      }
    }
    //sign and width
    ObObj temp_obj;
    temp_obj.set_int(datum_type.type_, 0);
    if ((no_log_type || static_cast<const parquet::IntLogicalType*>(log_type)->is_signed()) != temp_obj.is_signed_integer()) {
      func = NULL;
    }
    if (no_log_type ? (temp_obj.get_tight_data_len() != (parquet::Type::INT32 == phy_type ? 4 : 8))
                    : static_cast<const parquet::IntLogicalType*>(log_type)->bit_width() > temp_obj.get_tight_data_len() * 8) {
      func = NULL;
    }
  } else if ((no_log_type || log_type->is_string() || log_type->is_enum())
             && (ob_is_string_type(datum_type.type_) || ObRawType == datum_type.type_)) {
    //convert parquet enum/string to string vector
    if (parquet::Type::BYTE_ARRAY == phy_type) {
      func = &DataLoader::load_string_col;
    } else if (parquet::Type::FIXED_LEN_BYTE_ARRAY == phy_type) {
      func = &DataLoader::load_fixed_string_col;
    }
  } else if ((no_log_type || log_type->is_int() || log_type->is_decimal())
             && ob_is_number_or_decimal_int_tc(datum_type.type_)) {
    // no_log_type || log_type->is_int() for oracle int, phy_type should be int32 or int64
    //convert parquet int storing as int32/int64 to number/decimal vector
    if (log_type->is_decimal() && (col_desc->type_precision() != ((datum_type.precision_ == -1) ? 38 : datum_type.precision_)
                                   || col_desc->type_scale() != datum_type.scale_)) {
      func = NULL;
    } else if (!log_type->is_decimal() && parquet::Type::INT32 != phy_type
               && parquet::Type::INT64 != phy_type) {
      func = NULL;
    } else {
      //there is 4 kinds of physical format in parquet(int32/int64/fixedbytearray/bytearray)
      // and 2 class of types for OB vector(decimalint/number)
      if (parquet::Type::INT32 == phy_type && ob_is_decimal_int_tc(datum_type.type_)
          && DECIMAL_INT_32 == get_decimalint_type(datum_type.precision_)) {
        func = &DataLoader::load_int32_to_int32_vec;
      } else if (parquet::Type::INT64 == phy_type && ob_is_decimal_int_tc(datum_type.type_)
                 && DECIMAL_INT_64 == get_decimalint_type(datum_type.precision_)) {
        func = &DataLoader::load_int64_to_int64_vec;
      } else if (parquet::Type::INT32 == phy_type
                 || parquet::Type::INT64 == phy_type
                 || parquet::Type::BYTE_ARRAY == phy_type
                 || parquet::Type::FIXED_LEN_BYTE_ARRAY == phy_type) {
        func = &DataLoader::load_decimal_any_col;
      }
    }
  } else if ((no_log_type || log_type->is_date())
             && (ob_is_datetime_or_mysql_datetime(datum_type.type_)
                 || ob_is_date_or_mysql_date(datum_type.type_))) {
    if (parquet::Type::INT32 == phy_type && ob_is_date_tc(datum_type.type_)) {
      func = &DataLoader::load_int32_to_int32_vec;
    } else if (parquet::Type::INT32 == phy_type && ob_is_mysql_date_tc(datum_type.type_)) {
      func = &DataLoader::load_date_to_mysql_date;
    } else if (parquet::Type::INT32 == phy_type) {
      if(ob_is_datetime(datum_type.type_)) {
        func = &DataLoader::load_date_col_to_datetime;
      } else if (ob_is_mysql_datetime(datum_type.type_)) {
        func = &DataLoader::load_date_col_to_mysql_datetime;
      }
    }
  } else if ((no_log_type || log_type->is_int()) && parquet::Type::INT32 == phy_type
             && ob_is_year_tc(datum_type.type_)) {
    func = &DataLoader::load_year_col;
  } else if (log_type->is_time() && ob_is_time_tc(datum_type.type_)) {
    switch (static_cast<const parquet::TimeLogicalType*>(log_type)->time_unit()) {
      case parquet::LogicalType::TimeUnit::unit::MILLIS: {
        if (parquet::Type::INT32 == phy_type) {
          func = &DataLoader::load_time_millis_col;
        }
        break;
      }
      case parquet::LogicalType::TimeUnit::unit::MICROS: {
        if (parquet::Type::INT64 == phy_type) {
          func = &DataLoader::load_int64_to_int64_vec;
        }
        break;
      }
      case parquet::LogicalType::TimeUnit::unit::NANOS: {
        if (parquet::Type::INT64 == phy_type) {
          func = &DataLoader::load_time_nanos_col;
        }
        break;
      }
      default: {
        func = NULL;
      }
    }
  } else if (log_type->is_timestamp() && parquet::Type::INT64 == phy_type
             && (ob_is_otimestamp_type(datum_type.type_) || ob_is_datetime_or_mysql_datetime_tc(datum_type.type_))) {
    switch (static_cast<const parquet::TimestampLogicalType*>(log_type)->time_unit()) {
      case parquet::LogicalType::TimeUnit::unit::MILLIS: {
        if (ob_is_datetime_or_mysql_datetime_tc(datum_type.type_)
            || ObTimestampLTZType == datum_type.type_
            || ObTimestampNanoType == datum_type.type_) {
          func = &DataLoader::load_timestamp_millis_col;
        }
        break;
      }
      case parquet::LogicalType::TimeUnit::unit::MICROS: {
        if ((ObTimestampType == datum_type.type_ && is_parquet_store_utc(log_type))
            || (ObDateTimeType == datum_type.type_ && !is_parquet_store_utc(log_type))) {
          //mysql timestamp storing utc timestamp as int64 values
          func = &DataLoader::load_int64_to_int64_vec;
        } else if (ob_is_datetime_or_mysql_datetime_tc(datum_type.type_)
                   || ObTimestampLTZType == datum_type.type_
                   || ObTimestampNanoType == datum_type.type_) {
          func = &DataLoader::load_timestamp_micros_col;
        }
        break;
      }
      case parquet::LogicalType::TimeUnit::unit::NANOS: {
        if (ob_is_datetime_or_mysql_datetime_tc(datum_type.type_)
            || ObTimestampLTZType == datum_type.type_
            || ObTimestampNanoType == datum_type.type_) {
          func = &DataLoader::load_timestamp_nanos_col;
        }
        break;
      }
      default: {
        func = NULL;
      }
    }
  } else if ((no_log_type || log_type->is_timestamp()) && parquet::Type::INT96 == phy_type
             && (ob_is_otimestamp_type(datum_type.type_) || ObTimestampType == datum_type.type_)) {
    func = &DataLoader::load_timestamp_hive;
  } else if (no_log_type && parquet::Type::FLOAT == phy_type && ob_is_float_tc(datum_type.type_)) {
    func = &DataLoader::load_float;
  } else if (no_log_type && parquet::Type::DOUBLE == phy_type && ob_is_double_tc(datum_type.type_)) {
    func = &DataLoader::load_double;
  } else if (log_type->is_interval()
             || log_type->is_map()
             || log_type->is_list()
             || log_type->is_JSON()) {
    func = NULL;
  }
  return func;
}


#define IS_PARQUET_COL_NOT_NULL (0 == max_def_level)
#define IS_PARQUET_COL_VALUE_IS_NULL(V) (V < max_def_level)

int ObParquetTableRowIterator::DataLoader::load_int32_to_int32_vec()
{
  int ret = OB_SUCCESS;
  int64_t values_cnt = 0;
  ObEvalCtx::TempAllocGuard tmp_alloc_g(eval_ctx_);
  ObFixedLengthBase *dec_vec = static_cast<ObFixedLengthBase *>(file_col_expr_->get_vector(eval_ctx_));
  int16_t max_def_level = reader_->descr()->max_definition_level();
  ObArrayWrap<int32_t> values;

  OZ (values.allocate_array(tmp_alloc_g.get_allocator(), batch_size_));
  if (OB_SUCC(ret)) {
    row_count_ = static_cast<parquet::Int32Reader*>(reader_)->ReadBatch(
          batch_size_, def_levels_buf_.get_data(), rep_levels_buf_.get_data(),
          values.get_data(), &values_cnt);
    int j = 0;
    if (IS_PARQUET_COL_NOT_NULL && values_cnt == row_count_) {
      MEMCPY(pointer_cast<int32_t*>(dec_vec->get_data()) + row_offset_, values.get_data(), sizeof(int32_t) * row_count_);
    } else {
      for (int i = 0; OB_SUCC(ret) && i < row_count_; i++) {
        if (IS_PARQUET_COL_VALUE_IS_NULL(def_levels_buf_.at(i))) {
          dec_vec->set_null(i + row_offset_);
        } else {
          dec_vec->set_int32(i + row_offset_, values.at(j++));
        }
      }
    }
  }
  return ret;
}

int ObParquetTableRowIterator::DataLoader::load_date_to_mysql_date()
{
  int ret = OB_SUCCESS;
  int64_t values_cnt = 0;
  ObEvalCtx::TempAllocGuard tmp_alloc_g(eval_ctx_);
  ObFixedLengthBase *dec_vec = static_cast<ObFixedLengthBase *>(file_col_expr_->get_vector(eval_ctx_));
  int16_t max_def_level = reader_->descr()->max_definition_level();
  ObArrayWrap<int32_t> values;

  OZ (values.allocate_array(tmp_alloc_g.get_allocator(), batch_size_));
  if (OB_SUCC(ret)) {
    row_count_ = static_cast<parquet::Int32Reader*>(reader_)->ReadBatch(
          batch_size_, def_levels_buf_.get_data(), rep_levels_buf_.get_data(),
          values.get_data(), &values_cnt);
    int j = 0;
    if (IS_PARQUET_COL_NOT_NULL && values_cnt == row_count_) {
      MEMCPY(pointer_cast<int32_t*>(dec_vec->get_data()) + row_offset_, values.get_data(), sizeof(int32_t) * row_count_);
    } else {
      ObMySQLDate md_value = 0;
      for (int i = 0; OB_SUCC(ret) && i < row_count_; i++) {
        if (IS_PARQUET_COL_VALUE_IS_NULL(def_levels_buf_.at(i))) {
          dec_vec->set_null(i + row_offset_);
        } else if (OB_FAIL(ObTimeConverter::date_to_mdate(values.at(j++), md_value))) {
          LOG_WARN("date_to_mdate fail", K(ret));
        } else {
          dec_vec->set_mysql_date(i + row_offset_, md_value);
        }
      }
    }
  }
  return ret;
}

// convert int value to decimal int or number
int ObParquetTableRowIterator::DataLoader::to_numeric(const int64_t idx, const int64_t int_value)
{
  int ret = OB_SUCCESS;
  ObEvalCtx::TempAllocGuard tmp_alloc_g(eval_ctx_);
  if (ObDecimalIntType == file_col_expr_->datum_meta_.type_) {
    ObFixedLengthBase *vec = static_cast<ObFixedLengthBase *>(file_col_expr_->get_vector(eval_ctx_));
    ObDecimalInt *decint = NULL;
    int32_t int_bytes = 0;
    if (OB_FAIL(wide::from_integer(int_value, tmp_alloc_g.get_allocator(), decint,
                                   int_bytes, file_col_expr_->datum_meta_.precision_))) {
      LOG_WARN("fail to from integer", K(ret));
    } else {
      vec->set_decimal_int(idx, decint, int_bytes);
    }
  } else if (ObNumberType == file_col_expr_->datum_meta_.type_) {
    ObDiscreteBase *vec = static_cast<ObDiscreteBase *>(file_col_expr_->get_vector(eval_ctx_));
    number::ObNumber res_nmb;
    if (OB_FAIL(res_nmb.from(int_value, tmp_alloc_g.get_allocator()))) {
      LOG_WARN("fail to from number", K(ret));
    } else {
      vec->set_number(idx, res_nmb);
    }
  } else {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("not supported type", K(file_col_expr_->datum_meta_));
  }
  return ret;
}

// convert string value to decimal int or number
int ObParquetTableRowIterator::DataLoader::to_numeric(
    const int64_t idx,
    const char *str,
    const int32_t length)
{
  int ret = OB_SUCCESS;
  ObEvalCtx::TempAllocGuard tmp_alloc_g(eval_ctx_);
  ObDecimalInt *decint = NULL;
  int32_t val_len = 0;
  int16_t in_precision = 0;
  int16_t in_scale = 0;
  int16_t out_precision = file_col_expr_->datum_meta_.precision_;
  int16_t out_scale = file_col_expr_->datum_meta_.scale_;
  if (OB_FAIL(wide::from_string(str, length, tmp_alloc_g.get_allocator(), in_scale, in_precision, val_len, decint))) {
    LOG_WARN("fail to from number", K(ret), KPHEX(str, length));
  } else {
    if (ObDecimalIntType == file_col_expr_->datum_meta_.type_) {
      ObFixedLengthBase *vec = static_cast<ObFixedLengthBase *>(file_col_expr_->get_vector(eval_ctx_));
      if (in_precision > out_precision) {
        ret = OB_DECIMAL_PRECISION_OVERFLOW;
      } else {
        ObDecimalIntBuilder res_val;
        if (ObDatumCast::need_scale_decimalint(in_scale, in_precision, out_scale, out_precision)) {
          if (OB_FAIL(ObDatumCast::common_scale_decimalint(decint, val_len, in_scale, out_scale,
                                                           out_precision, 0, res_val))) {
            LOG_WARN("scale decimal int failed", K(ret));
          } else {
            vec->set_decimal_int(idx, res_val.get_decimal_int(), res_val.get_int_bytes());
          }
        } else {
          vec->set_decimal_int(idx, decint, val_len);
        }
      }
    } else if (ObNumberType == file_col_expr_->datum_meta_.type_) {
      ObDiscreteBase *vec = static_cast<ObDiscreteBase *>(file_col_expr_->get_vector(eval_ctx_));
      number::ObNumber res_nmb;
      if (OB_FAIL(wide::to_number(decint, val_len, file_col_expr_->datum_meta_.scale_,
                                  tmp_alloc_g.get_allocator(), res_nmb))) {
        LOG_WARN("fail to from", K(ret));
      } else {
        vec->set_number(idx, res_nmb);
      }
    } else {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("not supported type", K(file_col_expr_->datum_meta_));
    }
  }

  return ret;
}

int ObParquetTableRowIterator::DataLoader::to_numeric_hive(
    const int64_t idx,
    const char *str,
    const int32_t length,
    char *buf,
    const int64_t data_len)
{
  int ret = OB_SUCCESS;
  ObDecimalInt *decint = NULL;
  int32_t val_len = 0;
  if (OB_UNLIKELY(length > data_len)) {
    ret = OB_DECIMAL_PRECISION_OVERFLOW;
    LOG_WARN("overflow", K(length), K(data_len));
  } else {
    //to little endian
    MEMSET(buf, (*str >> 8), data_len); // fill 1 when the input value is negetive, otherwise fill 0
    if (data_len <= 4) {
      //for precision <= 9
      MEMCPY(buf + 4 - length, str, length);
      uint32_t *res = pointer_cast<uint32_t*>(buf);
      uint32_t temp_v = *res;
      *res = ntohl(temp_v);
    } else {
      int64_t pos = 0;
      int64_t temp_len = length;
      while (temp_len >= 8) {
        uint64_t temp_v = *(pointer_cast<const uint64_t*>(str + temp_len - 8));
        *(pointer_cast<uint64_t*>(buf + pos)) = ntohll(temp_v);
        pos+=8;
        temp_len-=8;
      }
      if (temp_len > 0) {
        MEMCPY(buf + pos + 8 - temp_len, str, temp_len);
        uint64_t temp_v = *(pointer_cast<uint64_t*>(buf + pos));
        *(pointer_cast<uint64_t*>(buf + pos)) = ntohll(temp_v);
      }
    }
    decint = pointer_cast<ObDecimalInt *>(buf);
    val_len = static_cast<int32_t>(data_len);
    if (ObDecimalIntType == file_col_expr_->datum_meta_.type_) {
      ObFixedLengthBase *vec = static_cast<ObFixedLengthBase *>(file_col_expr_->get_vector(eval_ctx_));
      vec->set_decimal_int(idx, decint, val_len);
    } else if (ObNumberType == file_col_expr_->datum_meta_.type_
               || ObUNumberType == file_col_expr_->datum_meta_.type_) {
      ObEvalCtx::TempAllocGuard tmp_alloc_g(eval_ctx_);
      ObDiscreteBase *vec = static_cast<ObDiscreteBase *>(file_col_expr_->get_vector(eval_ctx_));
      number::ObNumber res_nmb;
      if (OB_FAIL(wide::to_number(decint, val_len, file_col_expr_->datum_meta_.scale_,
                                  tmp_alloc_g.get_allocator(), res_nmb))) {
        LOG_WARN("fail to from", K(ret));
      } else {
        vec->set_number(idx, res_nmb);
      }
    } else {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("not supported type", K(file_col_expr_->datum_meta_));
    }
  }
  return ret;
}

int ObParquetTableRowIterator::to_numeric_hive(
    const char *str,
    const int32_t length,
    char *buf,
    const int64_t data_len,
    const ObDatumMeta &meta,
    ObIAllocator &alloc,
    ObStorageDatum &datum)
{
  int ret = OB_SUCCESS;
  ObDecimalInt *decint = NULL;
  int32_t val_len = 0;
  if (OB_UNLIKELY(length > data_len)) {
    ret = OB_DECIMAL_PRECISION_OVERFLOW;
    LOG_WARN("overflow", K(length), K(data_len));
  } else {
    //to little endian
    MEMSET(buf, (*str >> 8), data_len); // fill 1 when the input value is negetive, otherwise fill 0
    if (data_len <= 4) {
      //for precision <= 9
      MEMCPY(buf + 4 - length, str, length);
      uint32_t *res = pointer_cast<uint32_t*>(buf);
      uint32_t temp_v = *res;
      *res = ntohl(temp_v);
    } else {
      int64_t pos = 0;
      int64_t temp_len = length;
      while (temp_len >= 8) {
        uint64_t temp_v = *(pointer_cast<const uint64_t*>(str + temp_len - 8));
        *(pointer_cast<uint64_t*>(buf + pos)) = ntohll(temp_v);
        pos+=8;
        temp_len-=8;
      }
      if (temp_len > 0) {
        MEMCPY(buf + pos + 8 - temp_len, str, temp_len);
        uint64_t temp_v = *(pointer_cast<uint64_t*>(buf + pos));
        *(pointer_cast<uint64_t*>(buf + pos)) = ntohll(temp_v);
      }
    }
    decint = pointer_cast<ObDecimalInt *>(buf);
    val_len = static_cast<int32_t>(data_len);
    if (ObDecimalIntType == meta.type_) {
      datum.set_decimal_int(decint, val_len);
    } else if (ObNumberType == meta.type_
               || ObUNumberType == meta.type_) {
      number::ObNumber res_nmb;
      if (OB_FAIL(wide::to_number(decint, val_len, meta.scale_,
                                  alloc, res_nmb))) {
        LOG_WARN("fail to from", K(ret));
      } else {
        datum.set_number(res_nmb);
      }
    } else {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("not supported type", K(meta));
    }
  }
  return ret;
}


//convert int32/int64/string value(from parquet file) to decimal int or number(ob types)
int ObParquetTableRowIterator::DataLoader::load_decimal_any_col()
{
  int ret = OB_SUCCESS;
  int64_t values_cnt = 0;
  ObEvalCtx::TempAllocGuard tmp_alloc_g(eval_ctx_);
  int16_t max_def_level = reader_->descr()->max_definition_level();
  //int16_t def_precision = static_cast<int16_t>(reader_->descr()->type_precision());
  //int16_t def_scale = static_cast<int16_t>(reader_->descr()->type_precision());

  if (reader_->descr()->physical_type() == parquet::Type::type::INT32) {
    ObArrayWrap<int32_t> values;
    OZ (values.allocate_array(tmp_alloc_g.get_allocator(), batch_size_));
    if (OB_SUCC(ret)) {
      row_count_ = static_cast<parquet::Int32Reader*>(reader_)->ReadBatch(
            batch_size_, def_levels_buf_.get_data(), rep_levels_buf_.get_data(),
            values.get_data(), &values_cnt);
    }
    int j = 0;
    for (int i = 0; OB_SUCC(ret) && i < row_count_; i++) {
      if (IS_PARQUET_COL_VALUE_IS_NULL(def_levels_buf_.at(i))) {
        file_col_expr_->get_vector(eval_ctx_)->set_null(i + row_offset_);
      } else {
        OZ (to_numeric(i + row_offset_, values.at(j++)));
      }
    }
  } else if (reader_->descr()->physical_type() == parquet::Type::type::INT64) {
    ObArrayWrap<int64_t> values;
    OZ (values.allocate_array(tmp_alloc_g.get_allocator(), batch_size_));
    if (OB_SUCC(ret)) {
      row_count_ = static_cast<parquet::Int64Reader*>(reader_)->ReadBatch(
            batch_size_, def_levels_buf_.get_data(), rep_levels_buf_.get_data(),
            values.get_data(), &values_cnt);
    }
    int j = 0;
    for (int i = 0; OB_SUCC(ret) && i < row_count_; i++) {
      if (IS_PARQUET_COL_VALUE_IS_NULL(def_levels_buf_.at(i))) {
        file_col_expr_->get_vector(eval_ctx_)->set_null(i + row_offset_);
      } else {
        OZ (to_numeric(i + row_offset_, values.at(j++)));
      }
    }
  } else if (reader_->descr()->physical_type() == parquet::Type::type::BOOLEAN) {
    ObArrayWrap<bool> values;
    OZ (values.allocate_array(tmp_alloc_g.get_allocator(), batch_size_));
    if (OB_SUCC(ret)) {
      row_count_ = static_cast<parquet::BoolReader*>(reader_)->ReadBatch(
            batch_size_, def_levels_buf_.get_data(), rep_levels_buf_.get_data(),
            values.get_data(), &values_cnt);
    }
    int j = 0;
    for (int i = 0; OB_SUCC(ret) && i < row_count_; i++) {
      if (IS_PARQUET_COL_VALUE_IS_NULL(def_levels_buf_.at(i))) {
        file_col_expr_->get_vector(eval_ctx_)->set_null(i + row_offset_);
      } else {
        OZ (to_numeric(i + row_offset_, values.at(j++)));
      }
    }
  } else if (reader_->descr()->physical_type() == parquet::Type::Type::FIXED_LEN_BYTE_ARRAY) {
    ObArrayWrap<parquet::FixedLenByteArray> values;
    int32_t fixed_length = reader_->descr()->type_length();
    int32_t int_bytes = wide::ObDecimalIntConstValue::get_int_bytes_by_precision(
                                                    (file_col_expr_->datum_meta_.precision_ == -1)
                                                    ? 38 : file_col_expr_->datum_meta_.precision_);
    ObArrayWrap<char> buffer;
    OZ (buffer.allocate_array(tmp_alloc_g.get_allocator(), int_bytes));
    OZ (values.allocate_array(tmp_alloc_g.get_allocator(), batch_size_));
    if (OB_SUCC(ret)) {
      row_count_ = static_cast<parquet::FixedLenByteArrayReader*>(reader_)->ReadBatch(
            batch_size_, def_levels_buf_.get_data(), rep_levels_buf_.get_data(),
            values.get_data(), &values_cnt);
    }
    int j = 0;
    for (int i = 0; OB_SUCC(ret) && i < row_count_; i++) {
      if (IS_PARQUET_COL_VALUE_IS_NULL(def_levels_buf_.at(i))) {
        file_col_expr_->get_vector(eval_ctx_)->set_null(i + row_offset_);
      } else {
        parquet::FixedLenByteArray &cur_v = values.at(j++);
        OZ (to_numeric_hive(i + row_offset_, pointer_cast<const char*>(cur_v.ptr), fixed_length, buffer.get_data(), buffer.count()));
        //OZ (to_numeric(i, pointer_cast<const char*>(cur_v.ptr), fixed_length));
      }
    }
  } else if (reader_->descr()->physical_type() == parquet::Type::Type::BYTE_ARRAY) {
    ObArrayWrap<parquet::ByteArray> values;
    int32_t int_bytes = wide::ObDecimalIntConstValue::get_int_bytes_by_precision(
                                                    (file_col_expr_->datum_meta_.precision_ == -1)
                                                    ? 38 : file_col_expr_->datum_meta_.precision_);
    ObArrayWrap<char> buffer;
    OZ (buffer.allocate_array(tmp_alloc_g.get_allocator(), int_bytes));
    OZ (values.allocate_array(tmp_alloc_g.get_allocator(), batch_size_));
    if (OB_SUCC(ret)) {
      row_count_ = static_cast<parquet::ByteArrayReader*>(reader_)->ReadBatch(
            batch_size_, def_levels_buf_.get_data(), rep_levels_buf_.get_data(),
            values.get_data(), &values_cnt);
    }
    int j = 0;
    for (int i = 0; OB_SUCC(ret) && i < row_count_; i++) {
      if (IS_PARQUET_COL_VALUE_IS_NULL(def_levels_buf_.at(i))) {
        file_col_expr_->get_vector(eval_ctx_)->set_null(i + row_offset_);
      } else {
        parquet::ByteArray &cur_v = values.at(j++);
        OZ (to_numeric_hive(i + row_offset_, pointer_cast<const char*>(cur_v.ptr), cur_v.len, buffer.get_data(), buffer.count()));
        //OZ (to_numeric(i, pointer_cast<const char*>(cur_v.ptr), cur_v.len));
      }
    }
  }

  return ret;
}

int ObParquetTableRowIterator::DataLoader::load_fixed_string_col()
{
  int ret = OB_SUCCESS;
  int64_t values_cnt = 0;
  ObEvalCtx::TempAllocGuard tmp_alloc_g(eval_ctx_);
  int16_t max_def_level = reader_->descr()->max_definition_level();
  StrDiscVec *text_vec = static_cast<StrDiscVec *>(file_col_expr_->get_vector(eval_ctx_));
  ObArrayWrap<parquet::FixedLenByteArray> values;

  CK (VEC_DISCRETE == text_vec->get_format());
  OZ (values.allocate_array(tmp_alloc_g.get_allocator(), batch_size_));
  if (OB_SUCC(ret)) {
    int32_t fixed_length = reader_->descr()->type_length();
    row_count_ = static_cast<parquet::FixedLenByteArrayReader*>(reader_)->ReadBatch(
          batch_size_, def_levels_buf_.get_data(), rep_levels_buf_.get_data(),
          values.get_data(), &values_cnt);
    if (OB_UNLIKELY(values_cnt > row_count_)) {
      ret = OB_NOT_SUPPORTED;
      LOG_WARN("repeated data not support");
    } else {
      bool is_byte_length = is_oracle_byte_length(
            lib::is_oracle_mode(), file_col_expr_->datum_meta_.length_semantics_);
      int j = 0;
      for (int i = 0; i < row_count_; i++) {
        if (IS_PARQUET_COL_VALUE_IS_NULL(def_levels_buf_.at(i))) {
          text_vec->set_null(i + row_offset_);
        } else {
          void *res_ptr = NULL;
          parquet::FixedLenByteArray &cur_v = values.at(j++);
          if (OB_UNLIKELY(fixed_length > file_col_expr_->max_length_
                          && (is_byte_length || ObCharset::strlen_char(CS_TYPE_UTF8MB4_BIN,
                                                                       pointer_cast<const char *>(cur_v.ptr),
                                                                       fixed_length) > file_col_expr_->max_length_))) {
            ret = OB_ERR_DATA_TOO_LONG;
            LOG_WARN("data too long", K(file_col_expr_->max_length_), K(fixed_length), K(is_byte_length), K(ret));
          } else {
            if (row_count_ == batch_size_) {
              res_ptr = (void*)(cur_v.ptr);
            } else if (fixed_length > 0) {
              //when row_count_ less than batch_size_, it may reach page end and reload next page
              //string values need deep copy
              if (OB_ISNULL(res_ptr = str_res_mem_.alloc(fixed_length))) {
                ret = OB_ALLOCATE_MEMORY_FAILED;
                LOG_WARN("fail to allocate memory", K(fixed_length));
              } else {
                MEMCPY(res_ptr, cur_v.ptr, fixed_length);
              }
            }
            text_vec->set_string(i + row_offset_, pointer_cast<const char *>(res_ptr), fixed_length);
          }
        }
      }
    }
  }
  return ret;
}

int ObParquetTableRowIterator::DataLoader::load_string_col()
{
  int ret = OB_SUCCESS;
  int64_t values_cnt = 0;
  ObEvalCtx::TempAllocGuard tmp_alloc_g(eval_ctx_);
  int16_t max_def_level = reader_->descr()->max_definition_level();
  StrDiscVec *text_vec = static_cast<StrDiscVec *>(file_col_expr_->get_vector(eval_ctx_));
  ObArrayWrap<parquet::ByteArray> values;

  CK (VEC_DISCRETE == text_vec->get_format());
  OZ (values.allocate_array(tmp_alloc_g.get_allocator(), batch_size_));
  if (OB_SUCC(ret)) {
    row_count_ = static_cast<parquet::ByteArrayReader*>(reader_)->ReadBatch(
          batch_size_, def_levels_buf_.get_data(), rep_levels_buf_.get_data(),
          values.get_data(), &values_cnt);
    if (OB_UNLIKELY(values_cnt > row_count_)) {
      ret = OB_NOT_SUPPORTED;
      LOG_WARN("repeated data not support");
    } else {
      bool is_oracle_mode = lib::is_oracle_mode();
      bool is_byte_length = is_oracle_byte_length(
            is_oracle_mode, file_col_expr_->datum_meta_.length_semantics_);
      int j = 0;
      for (int i = 0; i < row_count_; i++) {
        if (IS_PARQUET_COL_VALUE_IS_NULL(def_levels_buf_.at(i))) {
          text_vec->set_null(i + row_offset_);
        } else {
          parquet::ByteArray &cur_v = values.at(j++);
          if (is_oracle_mode && 0 == cur_v.len) {
            text_vec->set_null(i + row_offset_);
          } else {
            void *res_ptr = NULL;
            if (OB_UNLIKELY(cur_v.len > file_col_expr_->max_length_
                            && (is_byte_length || ObCharset::strlen_char(CS_TYPE_UTF8MB4_BIN,
                                                                        pointer_cast<const char *>(cur_v.ptr),
                                                                        cur_v.len) > file_col_expr_->max_length_))) {
              ret = OB_ERR_DATA_TOO_LONG;
              LOG_WARN("data too long", K(file_col_expr_->max_length_), K(cur_v.len), K(is_byte_length), K(ret));
            } else {
              if (row_count_ == batch_size_) {
                res_ptr = (void *)(cur_v.ptr);
              } else if (cur_v.len > 0) {
                //when row_count_ less than batch_size_, it may reach page end and reload next page
                //string values need deep copy
                if (OB_ISNULL(res_ptr = str_res_mem_.alloc(cur_v.len))) {
                  ret = OB_ALLOCATE_MEMORY_FAILED;
                  LOG_WARN("fail to allocate memory", K(cur_v.len));
                } else {
                  MEMCPY(res_ptr, cur_v.ptr, cur_v.len);
                }
              }
              text_vec->set_string(i + row_offset_, pointer_cast<const char *>(res_ptr), cur_v.len);
            }
          }
        }
      }
    }
  }
  return ret;
}

int ObParquetTableRowIterator::DataLoader::load_int32_to_int64_vec()
{
  int ret = OB_SUCCESS;
  int64_t values_cnt = 0;
  ObEvalCtx::TempAllocGuard tmp_alloc_g(eval_ctx_);
  int16_t max_def_level = reader_->descr()->max_definition_level();
  ObFixedLengthBase *int64_vec = static_cast<ObFixedLengthBase *>(file_col_expr_->get_vector(eval_ctx_));
  ObArrayWrap<int32_t> values;

  CK (VEC_FIXED == int64_vec->get_format());
  OZ (values.allocate_array(tmp_alloc_g.get_allocator(), batch_size_));
  if (OB_SUCC(ret)) {
    row_count_ = static_cast<parquet::Int32Reader*>(reader_)->ReadBatch(
          batch_size_, def_levels_buf_.get_data(), rep_levels_buf_.get_data(),
          values.get_data(), &values_cnt);
    if (OB_UNLIKELY(values_cnt > row_count_)) {
      ret = OB_NOT_SUPPORTED;
      LOG_WARN("repeated data not support");
    } else {
      int j = 0;
      for (int i = 0; i < row_count_; i++) {
        if (IS_PARQUET_COL_VALUE_IS_NULL(def_levels_buf_.at(i))) {
          int64_vec->set_null(i + row_offset_);
        } else {
          int64_vec->set_int(i + row_offset_, values.at(j++));
        }
      }
    }
  }
  return ret;
}

int ObParquetTableRowIterator::DataLoader::load_bool_to_int64_vec()
{
  int ret = OB_SUCCESS;
  int64_t values_cnt = 0;
  ObEvalCtx::TempAllocGuard tmp_alloc_g(eval_ctx_);
  int16_t max_def_level = reader_->descr()->max_definition_level();
  ObFixedLengthBase *int64_vec = static_cast<ObFixedLengthBase *>(file_col_expr_->get_vector(eval_ctx_));
  ObArrayWrap<bool> values;

  CK (VEC_FIXED == int64_vec->get_format());
  OZ (values.allocate_array(tmp_alloc_g.get_allocator(), batch_size_));
  if (OB_SUCC(ret)) {
    row_count_ = static_cast<parquet::BoolReader*>(reader_)->ReadBatch(
          batch_size_, def_levels_buf_.get_data(), rep_levels_buf_.get_data(),
          values.get_data(), &values_cnt);
    if (OB_UNLIKELY(values_cnt > row_count_)) {
      ret = OB_NOT_SUPPORTED;
      LOG_WARN("repeated data not support");
    } else {
      int j = 0;
      for (int i = 0; i < row_count_; i++) {
        if (IS_PARQUET_COL_VALUE_IS_NULL(def_levels_buf_.at(i))) {
          int64_vec->set_null(i + row_offset_);
        } else {
          int64_vec->set_int(i + row_offset_, values.at(j++));
        }
      }
    }
  }
  return ret;
}

int ObParquetTableRowIterator::DataLoader::load_uint32_to_int64_vec()
{
  int ret = OB_SUCCESS;
  int64_t values_cnt = 0;
  ObEvalCtx::TempAllocGuard tmp_alloc_g(eval_ctx_);
  int16_t max_def_level = reader_->descr()->max_definition_level();
  ObFixedLengthBase *int32_vec = static_cast<ObFixedLengthBase *>(file_col_expr_->get_vector(eval_ctx_));
  ObArrayWrap<int32_t> values;

  CK (VEC_FIXED == int32_vec->get_format());
  OZ (values.allocate_array(tmp_alloc_g.get_allocator(), batch_size_));
  if (OB_SUCC(ret)) {
    row_count_ = static_cast<parquet::Int32Reader*>(reader_)->ReadBatch(
          batch_size_, def_levels_buf_.get_data(), rep_levels_buf_.get_data(),
          values.get_data(), &values_cnt);
    if (OB_UNLIKELY(values_cnt > row_count_)) {
      ret = OB_NOT_SUPPORTED;
      LOG_WARN("repeated data not support");
    } else {
      int j = 0;
      for (int i = 0; i < row_count_; i++) {
        if (IS_PARQUET_COL_VALUE_IS_NULL(def_levels_buf_.at(i))) {
          int32_vec->set_null(i);
        } else {
          uint32_t uint_value = static_cast<uint32_t>(values.at(j));
          int32_vec->set_int(i, static_cast<int64_t>(uint_value));
          j++;
        }
      }
    }
  }
  return ret;
}

int ObParquetTableRowIterator::DataLoader::load_int64_to_int64_vec()
{
  int ret = OB_SUCCESS;
  int64_t values_cnt = 0;
  ObEvalCtx::TempAllocGuard tmp_alloc_g(eval_ctx_);
  int16_t max_def_level = reader_->descr()->max_definition_level();
  ObFixedLengthBase *int64_vec = static_cast<ObFixedLengthBase *>(file_col_expr_->get_vector(eval_ctx_));
  ObArrayWrap<int64_t> values;

  CK (VEC_FIXED == int64_vec->get_format());
  OZ (values.allocate_array(tmp_alloc_g.get_allocator(), batch_size_));
  if (OB_SUCC(ret)) {
    row_count_ = static_cast<parquet::Int64Reader*>(reader_)->ReadBatch(
          batch_size_, def_levels_buf_.get_data(), rep_levels_buf_.get_data(),
          values.get_data(), &values_cnt);
    if (OB_UNLIKELY(values_cnt > row_count_)) {
      ret = OB_NOT_SUPPORTED;
      LOG_WARN("repeated data not support");
    } else if (IS_PARQUET_COL_NOT_NULL && values_cnt == row_count_) {
      MEMCPY(pointer_cast<int64_t*>(int64_vec->get_data()) + row_offset_, values.get_data(), sizeof(int64_t) * row_count_);
    } else {
      int j = 0;
      for (int i = 0; i < row_count_; i++) {
        if (IS_PARQUET_COL_VALUE_IS_NULL(def_levels_buf_.at(i))) {
          int64_vec->set_null(i + row_offset_);
        } else {
          int64_vec->set_int(i + row_offset_, values.at(j++));
        }
      }
    }
  }
  return ret;
}

int ObParquetTableRowIterator::DataLoader::load_date_col_to_datetime()
{
  int ret = OB_SUCCESS;
  int64_t values_cnt = 0;
  ObEvalCtx::TempAllocGuard tmp_alloc_g(eval_ctx_);
  ObFixedLengthBase *dec_vec = static_cast<ObFixedLengthBase *>(file_col_expr_->get_vector(eval_ctx_));
  int16_t max_def_level = reader_->descr()->max_definition_level();
  ObArrayWrap<int32_t> values;

  OZ (values.allocate_array(tmp_alloc_g.get_allocator(), batch_size_));
  if (OB_SUCC(ret)) {
    row_count_ = static_cast<parquet::Int32Reader*>(reader_)->ReadBatch(
          batch_size_, def_levels_buf_.get_data(), rep_levels_buf_.get_data(),
          values.get_data(), &values_cnt);
    int j = 0;
    for (int i = 0; OB_SUCC(ret) && i < row_count_; i++) {
      if (IS_PARQUET_COL_VALUE_IS_NULL(def_levels_buf_.at(i))) {
        dec_vec->set_null(i + row_offset_);
      } else {
        dec_vec->set_datetime(i + row_offset_, values.at(j++) * USECS_PER_DAY);
      }
    }
  }
  return ret;
}

int ObParquetTableRowIterator::DataLoader::load_date_col_to_mysql_datetime()
{
  int ret = OB_SUCCESS;
  int64_t values_cnt = 0;
  ObEvalCtx::TempAllocGuard tmp_alloc_g(eval_ctx_);
  ObFixedLengthBase *dec_vec = static_cast<ObFixedLengthBase *>(file_col_expr_->get_vector(eval_ctx_));
  int16_t max_def_level = reader_->descr()->max_definition_level();
  ObArrayWrap<int32_t> values;
  OZ (values.allocate_array(tmp_alloc_g.get_allocator(), batch_size_));
  if (OB_SUCC(ret)) {
    row_count_ = static_cast<parquet::Int32Reader*>(reader_)->ReadBatch(
          batch_size_, def_levels_buf_.get_data(), rep_levels_buf_.get_data(),
          values.get_data(), &values_cnt);
    int j = 0;
    ObMySQLDateTime mdt_value = 0;
    for (int i = 0; OB_SUCC(ret) && i < row_count_; i++) {
      if (IS_PARQUET_COL_VALUE_IS_NULL(def_levels_buf_.at(i))) {
        dec_vec->set_null(i + row_offset_);
      } else if (OB_FAIL(ObTimeConverter::date_to_mdatetime(values.at(j++), mdt_value))) {
        LOG_WARN("date_to_mdatetime fail", K(ret));
      } else {
        dec_vec->set_mysql_datetime(i + row_offset_, mdt_value);
      }
    }
  }
  return ret;
}

int ObParquetTableRowIterator::DataLoader::load_year_col()
{
  int ret = OB_SUCCESS;
  int64_t values_cnt = 0;
  ObEvalCtx::TempAllocGuard tmp_alloc_g(eval_ctx_);
  ObFixedLengthBase *dec_vec = static_cast<ObFixedLengthBase *>(file_col_expr_->get_vector(eval_ctx_));
  int16_t max_def_level = reader_->descr()->max_definition_level();
  ObArrayWrap<int32_t> values;
  OZ (values.allocate_array(tmp_alloc_g.get_allocator(), batch_size_));
  if (OB_SUCC(ret)) {
    row_count_ = static_cast<parquet::Int32Reader*>(reader_)->ReadBatch(
          batch_size_, def_levels_buf_.get_data(), rep_levels_buf_.get_data(),
          values.get_data(), &values_cnt);
    int j = 0;
    for (int i = 0; OB_SUCC(ret) && i < row_count_; i++) {
      if (IS_PARQUET_COL_VALUE_IS_NULL(def_levels_buf_.at(i))) {
        file_col_expr_->get_vector(eval_ctx_)->set_null(i);
      } else {
        dec_vec->set_year(i, values.at(j++));
      }
    }
  }
  return ret;
}

int ObParquetTableRowIterator::DataLoader::load_time_millis_col()
{
  int ret = OB_SUCCESS;
  int64_t values_cnt = 0;
  ObEvalCtx::TempAllocGuard tmp_alloc_g(eval_ctx_);
  ObFixedLengthBase *dec_vec = static_cast<ObFixedLengthBase *>(file_col_expr_->get_vector(eval_ctx_));
  int16_t max_def_level = reader_->descr()->max_definition_level();
  ObArrayWrap<int32_t> values;
  OZ (values.allocate_array(tmp_alloc_g.get_allocator(), batch_size_));
  if (OB_SUCC(ret)) {
    row_count_ = static_cast<parquet::Int32Reader*>(reader_)->ReadBatch(
          batch_size_, def_levels_buf_.get_data(), rep_levels_buf_.get_data(),
          values.get_data(), &values_cnt);
    int j = 0;
    for (int i = 0; OB_SUCC(ret) && i < row_count_; i++) {
      if (IS_PARQUET_COL_VALUE_IS_NULL(def_levels_buf_.at(i))) {
        dec_vec->set_null(i + row_offset_);
      } else {
        dec_vec->set_time(i + row_offset_, values.at(j++) * USECS_PER_MSEC);
      }
    }
  }
  return ret;
}

int ObParquetTableRowIterator::DataLoader::load_time_nanos_col()
{
  int ret = OB_SUCCESS;
  int64_t values_cnt = 0;
  ObEvalCtx::TempAllocGuard tmp_alloc_g(eval_ctx_);
  ObFixedLengthBase *dec_vec = static_cast<ObFixedLengthBase *>(file_col_expr_->get_vector(eval_ctx_));
  int16_t max_def_level = reader_->descr()->max_definition_level();
  ObArrayWrap<int64_t> values;
  OZ (values.allocate_array(tmp_alloc_g.get_allocator(), batch_size_));
  if (OB_SUCC(ret)) {
    row_count_ = static_cast<parquet::Int64Reader*>(reader_)->ReadBatch(
          batch_size_, def_levels_buf_.get_data(), rep_levels_buf_.get_data(),
          values.get_data(), &values_cnt);
    int j = 0;
    for (int i = 0; OB_SUCC(ret) && i < row_count_; i++) {
      if (IS_PARQUET_COL_VALUE_IS_NULL(def_levels_buf_.at(i))) {
        dec_vec->set_null(i + row_offset_);
      } else {
        dec_vec->set_time(i + row_offset_, values.at(j++) / NSECS_PER_USEC);
      }
    }
  }
  return ret;
}

bool ObParquetTableRowIterator::is_parquet_store_utc(const parquet::LogicalType *logtype)
{
  return logtype->is_timestamp() ? static_cast<const parquet::TimestampLogicalType*>(logtype)->is_adjusted_to_utc() : true;
}

bool ObParquetTableRowIterator::DataLoader::is_ob_type_store_utc(const ObDatumMeta &meta)
{
  return (lib::is_mysql_mode() && ObTimestampType == meta.type_)
         || (lib::is_oracle_mode() && ObTimestampLTZType == meta.type_);
}

int64_t ObParquetTableRowIterator::DataLoader::calc_tz_adjust_us(const parquet::LogicalType *logtype,
                                                                 const ObDatumMeta &meta,
                                                                 ObSQLSessionInfo *session)
{
  int64_t res = 0;
  bool is_utc_src = is_parquet_store_utc(logtype);
  bool is_utc_dst = is_ob_type_store_utc(meta);
  if (is_utc_src != is_utc_dst) {
    int32_t tmp_offset = 0;
    if (OB_NOT_NULL(session)
        && OB_NOT_NULL(session->get_timezone_info())
        && OB_SUCCESS == session->get_timezone_info()->get_timezone_offset(0, tmp_offset)) {
      res = SEC_TO_USEC(tmp_offset) * (is_utc_src ? 1 : -1);
    }
  }
  return res;
}

int ObParquetTableRowIterator::DataLoader::load_timestamp_millis_col()
{
  int ret = OB_SUCCESS;
  int64_t values_cnt = 0;
  ObEvalCtx::TempAllocGuard tmp_alloc_g(eval_ctx_);
  ObFixedLengthBase *dec_vec = static_cast<ObFixedLengthBase *>(file_col_expr_->get_vector(eval_ctx_));
  int16_t max_def_level = reader_->descr()->max_definition_level();
  ObArrayWrap<int64_t> values;
  int64_t adjust_us = calc_tz_adjust_us(reader_->descr()->logical_type().get(),
                                        file_col_expr_->datum_meta_,
                                        eval_ctx_.exec_ctx_.get_my_session());

  OZ (values.allocate_array(tmp_alloc_g.get_allocator(), batch_size_));
  if (OB_SUCC(ret)) {
    row_count_ = static_cast<parquet::Int64Reader*>(reader_)->ReadBatch(
          batch_size_, def_levels_buf_.get_data(), rep_levels_buf_.get_data(),
          values.get_data(), &values_cnt);
    int j = 0;
    for (int i = 0; OB_SUCC(ret) && i < row_count_; i++) {
      if (IS_PARQUET_COL_VALUE_IS_NULL(def_levels_buf_.at(i))) {
        dec_vec->set_null(i + row_offset_);
      } else {
        int64_t adjusted_value = values.at(j++) * USECS_PER_MSEC + adjust_us;
        if (ob_is_datetime_tc(file_col_expr_->datum_meta_.type_)) {
          dec_vec->set_timestamp(i + row_offset_, adjusted_value);
        } else if (ob_is_mysql_datetime_tc(file_col_expr_->datum_meta_.type_)) {
          ObMySQLDateTime mdatetime;
          ObTimeConverter::datetime_to_mdatetime(adjusted_value, mdatetime);
          dec_vec->set_mysql_datetime(i + row_offset_, mdatetime);
        } else {
          ObOTimestampData data;
          data.time_us_ = adjusted_value;
          dec_vec->set_otimestamp_tiny(i + row_offset_, ObOTimestampTinyData().from_timestamp_data(data));
        }
      }
    }
  }
  return ret;
}

int ObParquetTableRowIterator::DataLoader::load_timestamp_micros_col()
{
  int ret = OB_SUCCESS;
  int64_t values_cnt = 0;
  ObEvalCtx::TempAllocGuard tmp_alloc_g(eval_ctx_);
  ObFixedLengthBase *dec_vec = static_cast<ObFixedLengthBase *>(file_col_expr_->get_vector(eval_ctx_));
  int16_t max_def_level = reader_->descr()->max_definition_level();
  ObArrayWrap<int64_t> values;
  int64_t adjust_us = calc_tz_adjust_us(reader_->descr()->logical_type().get(),
                                        file_col_expr_->datum_meta_,
                                        eval_ctx_.exec_ctx_.get_my_session());
  OZ (values.allocate_array(tmp_alloc_g.get_allocator(), batch_size_));
  if (OB_SUCC(ret)) {
    row_count_ = static_cast<parquet::Int64Reader*>(reader_)->ReadBatch(
          batch_size_, def_levels_buf_.get_data(), rep_levels_buf_.get_data(),
          values.get_data(), &values_cnt);
    int j = 0;
    for (int i = 0; OB_SUCC(ret) && i < row_count_; i++) {
      if (IS_PARQUET_COL_VALUE_IS_NULL(def_levels_buf_.at(i))) {
        dec_vec->set_null(i + row_offset_);
      } else {
        int64_t adjusted_value = (values.at(j++) + adjust_us);
        if (ob_is_datetime_tc(file_col_expr_->datum_meta_.type_)) {
          dec_vec->set_timestamp(i + row_offset_, adjusted_value);
        } else if (ob_is_mysql_datetime_tc(file_col_expr_->datum_meta_.type_)) {
          ObMySQLDateTime mdatetime;
          ObTimeConverter::datetime_to_mdatetime(adjusted_value, mdatetime);
          dec_vec->set_mysql_datetime(i + row_offset_, mdatetime);
        } else {
          ObOTimestampData data;
          data.time_us_ = adjusted_value;
          dec_vec->set_otimestamp_tiny(i + row_offset_, ObOTimestampTinyData().from_timestamp_data(data));
        }
      }
    }
  }
  return ret;
}

int ObParquetTableRowIterator::DataLoader::load_timestamp_nanos_col()
{
  int ret = OB_SUCCESS;
  int64_t values_cnt = 0;
  ObEvalCtx::TempAllocGuard tmp_alloc_g(eval_ctx_);
  ObFixedLengthBase *dec_vec = static_cast<ObFixedLengthBase *>(file_col_expr_->get_vector(eval_ctx_));
  int16_t max_def_level = reader_->descr()->max_definition_level();
  ObArrayWrap<int64_t> values;
  int64_t adjust_us = calc_tz_adjust_us(reader_->descr()->logical_type().get(),
                                        file_col_expr_->datum_meta_,
                                        eval_ctx_.exec_ctx_.get_my_session());
  OZ (values.allocate_array(tmp_alloc_g.get_allocator(), batch_size_));
  if (OB_SUCC(ret)) {
    row_count_ = static_cast<parquet::Int64Reader*>(reader_)->ReadBatch(
          batch_size_, def_levels_buf_.get_data(), rep_levels_buf_.get_data(),
          values.get_data(), &values_cnt);
    int j = 0;
    for (int i = 0; OB_SUCC(ret) && i < row_count_; i++) {
      if (IS_PARQUET_COL_VALUE_IS_NULL(def_levels_buf_.at(i))) {
        dec_vec->set_null(i + row_offset_);
      } else {
        if (ob_is_datetime_tc(file_col_expr_->datum_meta_.type_)) {
          dec_vec->set_timestamp(i + row_offset_, values.at(j++) / NSECS_PER_USEC + adjust_us);
        } else if (ob_is_mysql_datetime_tc(file_col_expr_->datum_meta_.type_)) {
          ObMySQLDateTime mdatetime;
          ObTimeConverter::datetime_to_mdatetime(values.at(j++) / NSECS_PER_USEC + adjust_us, mdatetime);
          dec_vec->set_mysql_datetime(i + row_offset_, mdatetime);
        } else {
          ObOTimestampData data;
          int64_t cur_value = values.at(j++);
          data.time_us_ = cur_value / NSECS_PER_USEC + adjust_us;
          data.time_ctx_.set_tail_nsec(cur_value % NSECS_PER_USEC);
          dec_vec->set_otimestamp_tiny(i + row_offset_, ObOTimestampTinyData().from_timestamp_data(data));
        }
      }
    }
  }
  return ret;
}

int ObParquetTableRowIterator::DataLoader::load_timestamp_hive()
{
  int ret = OB_SUCCESS;
  int64_t values_cnt = 0;
  ObEvalCtx::TempAllocGuard tmp_alloc_g(eval_ctx_);
  ObFixedLengthBase *dec_vec = static_cast<ObFixedLengthBase *>(file_col_expr_->get_vector(eval_ctx_));
  int16_t max_def_level = reader_->descr()->max_definition_level();
  ObArrayWrap<parquet::Int96> values;
  int64_t adjust_us = calc_tz_adjust_us(reader_->descr()->logical_type().get(),
                                        file_col_expr_->datum_meta_,
                                        eval_ctx_.exec_ctx_.get_my_session());
  OZ (values.allocate_array(tmp_alloc_g.get_allocator(), batch_size_));
  if (OB_SUCC(ret)) {
    row_count_ = static_cast<parquet::Int96Reader*>(reader_)->ReadBatch(
          batch_size_, def_levels_buf_.get_data(), rep_levels_buf_.get_data(),
          values.get_data(), &values_cnt);
    int j = 0;
    for (int i = 0; OB_SUCC(ret) && i < row_count_; i++) {
      if (IS_PARQUET_COL_VALUE_IS_NULL(def_levels_buf_.at(i))) {
        dec_vec->set_null(i + row_offset_);
      } else {
        parquet::Int96 &value = values.at(j++);
        uint64_t nsec_time_value = ((uint64_t)value.value[1] << 32) + (uint64_t)value.value[0];
        uint32_t julian_date_value = value.value[2];
        int64_t utc_timestamp =((int64_t)julian_date_value - 2440588LL) * 86400000000LL + (int64_t)(nsec_time_value / NSECS_PER_USEC);
        if (ob_is_datetime_tc(file_col_expr_->datum_meta_.type_)) {
          dec_vec->set_timestamp(i + row_offset_, utc_timestamp + adjust_us);
        } else if (ob_is_mysql_datetime_tc(file_col_expr_->datum_meta_.type_)) {
          ObMySQLDateTime mdatetime;
          ObTimeConverter::datetime_to_mdatetime(utc_timestamp + adjust_us, mdatetime);
          dec_vec->set_mysql_datetime(i + row_offset_, mdatetime);
        } else {
          ObOTimestampData data;
          data.time_us_ = utc_timestamp + adjust_us;
          data.time_ctx_.set_tail_nsec((int32_t)(nsec_time_value % NSECS_PER_USEC));
          dec_vec->set_otimestamp_tiny(i + row_offset_, ObOTimestampTinyData().from_timestamp_data(data));
        }
      }
    }
  }
  return ret;
}

int ObParquetTableRowIterator::DataLoader::load_float()
{
  int ret = OB_SUCCESS;
  int64_t values_cnt = 0;
  ObEvalCtx::TempAllocGuard tmp_alloc_g(eval_ctx_);
  int16_t max_def_level = reader_->descr()->max_definition_level();
  ObFixedLengthBase *float_vec = static_cast<ObFixedLengthBase *>(file_col_expr_->get_vector(eval_ctx_));
  ObArrayWrap<float> values;

  CK (VEC_FIXED == float_vec->get_format());
  OZ (values.allocate_array(tmp_alloc_g.get_allocator(), batch_size_));
  if (OB_SUCC(ret)) {
    row_count_ = static_cast<parquet::FloatReader*>(reader_)->ReadBatch(
          batch_size_, def_levels_buf_.get_data(), rep_levels_buf_.get_data(),
          values.get_data(), &values_cnt);
    if (OB_UNLIKELY(values_cnt > row_count_)) {
      ret = OB_NOT_SUPPORTED;
      LOG_WARN("repeated data not support");
    } else if (IS_PARQUET_COL_NOT_NULL && values_cnt == row_count_) {
      MEMCPY(pointer_cast<float*>(float_vec->get_data()) + row_offset_, values.get_data(), sizeof(float) * row_count_);
    } else {
      int j = 0;
      for (int i = 0; i < row_count_; i++) {
        if (IS_PARQUET_COL_VALUE_IS_NULL(def_levels_buf_.at(i))) {
          float_vec->set_null(i + row_offset_);
        } else {
          float_vec->set_float(i + row_offset_, values.at(j++));
        }
      }
    }
  }
  return ret;
}

int ObParquetTableRowIterator::DataLoader::load_double()
{
  int ret = OB_SUCCESS;
  int64_t values_cnt = 0;
  ObEvalCtx::TempAllocGuard tmp_alloc_g(eval_ctx_);
  int16_t max_def_level = reader_->descr()->max_definition_level();
  ObFixedLengthBase *double_vec = static_cast<ObFixedLengthBase *>(file_col_expr_->get_vector(eval_ctx_));
  ObArrayWrap<double> values;

  CK (VEC_FIXED == double_vec->get_format());
  OZ (values.allocate_array(tmp_alloc_g.get_allocator(), batch_size_));
  if (OB_SUCC(ret)) {
    row_count_ = static_cast<parquet::DoubleReader*>(reader_)->ReadBatch(
          batch_size_, def_levels_buf_.get_data(), rep_levels_buf_.get_data(),
          values.get_data(), &values_cnt);
    if (OB_UNLIKELY(values_cnt > row_count_)) {
      ret = OB_NOT_SUPPORTED;
      LOG_WARN("repeated data not support");
    } else if (IS_PARQUET_COL_NOT_NULL && values_cnt == row_count_) {
      MEMCPY(pointer_cast<double*>(double_vec->get_data()) + row_offset_, values.get_data(), sizeof(double) * row_count_);
    } else {
      int j = 0;
      for (int i = 0; i < row_count_; i++) {
        if (IS_PARQUET_COL_VALUE_IS_NULL(def_levels_buf_.at(i))) {
          double_vec->set_null(i + row_offset_);
        } else {
          double_vec->set_double(i + row_offset_, values.at(j++));
        }
      }
    }
  }
  return ret;
}

#undef IS_PARQUET_COL_NOT_NULL
#undef IS_PARQUET_COL_VALUE_IS_NULL

int ObParquetTableRowIterator::get_next_rows(int64_t &count, int64_t capacity)
{
  int ret = OB_SUCCESS;
  ObEvalCtx &eval_ctx = scan_param_->op_->get_eval_ctx();
  const ExprFixedArray &column_conv_exprs = *(scan_param_->ext_column_convert_exprs_);
  int64_t read_count = 0;
  ObMallocHookAttrGuard guard(mem_attr_);

  if (OB_SUCC(ret) && state_.cur_row_group_read_row_counts_[0] >= state_.cur_row_group_row_count_) {
    if (OB_FAIL(next_row_group())) {
      if (OB_ITER_END != ret) {
        LOG_WARN("fail to next row group", K(ret));
      }
    }
  }
  if (!file_column_exprs_.count()) {
    read_count = std::min(capacity, state_.cur_row_group_row_count_ - state_.cur_row_group_read_row_counts_[0]);
  } else {
    str_res_mem_.reuse();
    try {
      //load vec data from parquet file to file column expr
      for (int i = 0; OB_SUCC(ret) && i < file_column_exprs_.count(); ++i) {
        int64_t load_row_count = 0;
        OZ (file_column_exprs_.at(i)->init_vector_for_write(
                eval_ctx, file_column_exprs_.at(i)->get_default_res_format(), eval_ctx.max_batch_size_));
        while (OB_SUCC(ret) && load_row_count < capacity && column_readers_.at(i).get()->HasNext()) {
          int64_t temp_row_count = 0;
          DataLoader loader(eval_ctx, file_column_exprs_.at(i), column_readers_.at(i).get(),
                            def_levels_buf_, rep_levels_buf_, str_res_mem_,
                            capacity - load_row_count, load_row_count, temp_row_count);
          MEMSET(def_levels_buf_.get_data(), 0, sizeof(def_levels_buf_.at(0)) * eval_ctx.max_batch_size_);
          MEMSET(rep_levels_buf_.get_data(), 0, sizeof(rep_levels_buf_.at(0)) * eval_ctx.max_batch_size_);
          OZ (loader.load_data_for_col(load_funcs_.at(i)));
          load_row_count += temp_row_count;
        }
        if (OB_SUCC(ret)) {
          if (0 == read_count) {
            read_count = load_row_count;
          } else {
            if (read_count != load_row_count) {
              ret = OB_ERR_UNEXPECTED;
            }
          }
        }
        file_column_exprs_.at(i)->set_evaluated_projected(eval_ctx);
      }
    } catch (const ObErrorCodeException &ob_error) {
      if (OB_SUCC(ret)) {
        ret = ob_error.get_error_code();
        LOG_WARN("fail to read file", K(ret));
      }
    } catch(const std::exception& e) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("unexpected index", K(ret), "Info", e.what());
    } catch(...) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("unexpected index", K(ret));
    }
  }
  if (OB_SUCC(ret) && read_count > 0) {
    //fill expr results from metadata
    for (int i = 0; OB_SUCC(ret) && i < file_meta_column_exprs_.count(); i++) {
      ObExpr *meta_expr = file_meta_column_exprs_.at(i);
      if (meta_expr->type_ == T_PSEUDO_EXTERNAL_FILE_URL) {
        StrDiscVec *text_vec = static_cast<StrDiscVec *>(meta_expr->get_vector(eval_ctx));
        OZ (meta_expr->init_vector_for_write(eval_ctx, VEC_DISCRETE, read_count));
        if (OB_SUCC(ret)) {
          text_vec->set_ptrs(file_url_ptrs_.get_data());
          text_vec->set_lens(file_url_lens_.get_data());
        }
      } else if (meta_expr->type_ == T_PSEUDO_PARTITION_LIST_COL) {
        OZ (meta_expr->init_vector_for_write(eval_ctx, VEC_UNIFORM, read_count));
        OZ (fill_file_partition_expr(meta_expr, state_.part_list_val_, read_count));
      } else {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("unexpected expr", KPC(meta_expr));
      }
      meta_expr->set_evaluated_projected(eval_ctx);
    }

    for (int i = 0; OB_SUCC(ret) && i < column_exprs_.count(); i++) {
      //column_conv_exprs is 1-1 mapped to column_exprs
      //calc gen column exprs
      if (!column_conv_exprs.at(i)->get_eval_info(eval_ctx).evaluated_) {
        OZ (column_conv_exprs.at(i)->init_vector_default(eval_ctx, read_count));
        OZ (column_conv_exprs.at(i)->eval_vector(eval_ctx, *bit_vector_cache_, read_count, true));
        column_conv_exprs.at(i)->set_evaluated_projected(eval_ctx);
      }
      //assign gen column exprs value to column exprs(output exprs)
      if (OB_SUCC(ret)) {
        ObExpr *to = column_exprs_.at(i);
        ObExpr *from = column_conv_exprs.at(i);
        VectorHeader &to_vec_header = to->get_vector_header(eval_ctx);
        VectorHeader &from_vec_header = from->get_vector_header(eval_ctx);
        if (from_vec_header.format_ == VEC_UNIFORM_CONST) {
          ObDatum *from_datum =
            static_cast<ObUniformBase *>(from->get_vector(eval_ctx))->get_datums();
          OZ(to->init_vector(eval_ctx, VEC_UNIFORM, read_count));
          ObUniformBase *to_vec = static_cast<ObUniformBase *>(to->get_vector(eval_ctx));
          ObDatum *to_datums = to_vec->get_datums();
          for (int64_t j = 0; j < read_count && OB_SUCC(ret); j++) {
            to_datums[j] = *from_datum;
          }
        } else if (from_vec_header.format_ == VEC_UNIFORM) {
          ObUniformBase *uni_vec = static_cast<ObUniformBase *>(from->get_vector(eval_ctx));
          ObDatum *src = uni_vec->get_datums();
          ObDatum *dst = to->locate_batch_datums(eval_ctx);
          if (src != dst) {
            MEMCPY(dst, src, read_count * sizeof(ObDatum));
          }
          OZ(to->init_vector(eval_ctx, VEC_UNIFORM, read_count));
        } else if (OB_FAIL(to_vec_header.assign(from_vec_header))) {
          LOG_WARN("assign vector header failed", K(ret));
        }
        column_exprs_.at(i)->set_evaluated_projected(eval_ctx);
      }
    }
    OZ (calc_exprs_for_rowid(read_count, state_));
  }
  if (OB_SUCC(ret)) {
    for (int64_t i = 0; i < state_.cur_row_group_read_row_counts_.count(); ++i) {
      state_.cur_row_group_read_row_counts_[i] += read_count;
    }
    count = read_count;
  } else if (OB_UNLIKELY(OB_ITER_END != ret)) {
    LOG_WARN("fail to get next rows from parquet file", K(ret), K(state_));
  }
  return ret;
}

int ObParquetTableRowIterator::get_next_row()
{
  int ret = OB_NOT_SUPPORTED;
  return ret;
}

void ObParquetTableRowIterator::reset() {
  // reset state_ to initial values for rescan
  state_.reuse();
  file_prebuffer_.destroy();
}

int ObParquetTableRowIterator::read_min_max_datum(const std::string &min_val, const std::string &max_val,
                                                  const parquet::Type::type physical_type, const parquet::LogicalType* log_type,
                                                  const ObDatumMeta &datum_type, const int64_t max_length,
                                                  const parquet::SortOrder::type sort_order, const bool has_lob_header,
                                                  ObEvalCtx &eval_ctx, blocksstable::ObStorageDatum &min_datum,
                                                  blocksstable::ObStorageDatum &max_datum, ObIAllocator &tmp_alloc)
{
  int ret = OB_SUCCESS;
  bool no_log_type = log_type->is_none();
  switch (physical_type) {
    case parquet::Type::BOOLEAN: {
      if (parquet::SortOrder::type::SIGNED != sort_order) {
      } else if ((no_log_type || log_type->is_int()) && ob_is_int_tc(datum_type.type_)) {
        min_datum.set_bool(reinterpret_cast<const bool*>(min_val.data())[0]);
        max_datum.set_bool(reinterpret_cast<const bool*>(max_val.data())[0]);
      }
      break;
    }
    case parquet::Type::INT32: {
      int32_t tmp_min32 = reinterpret_cast<const int32_t*>(min_val.data())[0];
      int32_t tmp_max32 = reinterpret_cast<const int32_t*>(max_val.data())[0];
      if (parquet::SortOrder::type::SIGNED != sort_order) {
      } else if (no_log_type || log_type->is_int()) {
        if (ob_is_int_tc(datum_type.type_)
            || (ob_is_decimal_int_tc(datum_type.type_)
                && DECIMAL_INT_32 == get_decimalint_type(datum_type.precision_))) {
          min_datum.set_int32(tmp_min32);
          max_datum.set_int32(tmp_max32);
        } else if (ob_is_year_tc(datum_type.type_)) {
          min_datum.set_year(tmp_min32);
          max_datum.set_year(tmp_max32);
        }
      } else if (no_log_type || log_type->is_date()) {
        if (ObDateType == datum_type.type_) {
          min_datum.set_date(tmp_min32);
          max_datum.set_date(tmp_max32);
        } else if (ObMySQLDateType == datum_type.type_) {
          ObMySQLDate min_md_value = 0;
          ObMySQLDate max_md_value = 0;
          if (OB_FAIL(ObTimeConverter::date_to_mdate(tmp_min32, min_md_value))) {
            LOG_WARN("date_to_mdate fail", K(ret));
          } else if (OB_FAIL(ObTimeConverter::date_to_mdate(tmp_max32, max_md_value))) {
            LOG_WARN("date_to_mdate fail", K(ret));
          } else {
            min_datum.set_mysql_date(min_md_value);
            max_datum.set_mysql_date(max_md_value);
          }
        }
      } else if (log_type->is_time() && ob_is_time_tc(datum_type.type_)
                && parquet::LogicalType::TimeUnit::unit::MILLIS
                   == static_cast<const parquet::TimeLogicalType*>(log_type)->time_unit()) {
        min_datum.set_time(tmp_min32 * USECS_PER_MSEC);
        max_datum.set_time(tmp_max32 * USECS_PER_MSEC);
      }
      break;
    }
    case parquet::Type::INT64: {
      int64_t tmp_min64 = reinterpret_cast<const int64_t*>(min_val.data())[0];
      int64_t tmp_max64 = reinterpret_cast<const int64_t*>(max_val.data())[0];
      if (parquet::SortOrder::type::SIGNED != sort_order) {
      } else if ((no_log_type || log_type->is_int())
          && (ob_is_int_tc(datum_type.type_)
              || (ob_is_decimal_int_tc(datum_type.type_)
                  && DECIMAL_INT_64 == get_decimalint_type(datum_type.precision_)))) {
        min_datum.set_int(tmp_min64);
        max_datum.set_int(tmp_max64);
      } else if (log_type->is_time() && ob_is_time_tc(datum_type.type_)) {
        if (parquet::LogicalType::TimeUnit::unit::MICROS
            == static_cast<const parquet::TimeLogicalType*>(log_type)->time_unit()) {
          min_datum.set_time(tmp_min64);
          max_datum.set_time(tmp_max64);
        } else if (parquet::LogicalType::TimeUnit::unit::NANOS
                   == static_cast<const parquet::TimeLogicalType*>(log_type)->time_unit()) {
          min_datum.set_time(tmp_min64 / NSECS_PER_USEC);
          max_datum.set_time(tmp_max64 / NSECS_PER_USEC);
        }
      } else if (log_type->is_timestamp() && (ob_is_otimestamp_type(datum_type.type_)
                                              || ob_is_datetime_or_mysql_datetime_tc(datum_type.type_))) {
          int64_t adjust_us = DataLoader::calc_tz_adjust_us(log_type, datum_type, eval_ctx.exec_ctx_.get_my_session());
          if (parquet::LogicalType::TimeUnit::unit::MILLIS == static_cast<const parquet::TimestampLogicalType*>(log_type)->time_unit()) {
            int64_t adjusted_min_value = tmp_min64 * USECS_PER_MSEC + adjust_us;
            int64_t adjusted_max_value = tmp_max64 * USECS_PER_MSEC + adjust_us;
            if (ob_is_datetime_or_mysql_datetime_tc(datum_type.type_)
                || ObTimestampLTZType == datum_type.type_
                || ObTimestampNanoType == datum_type.type_) {
              if (OB_FAIL(convert_timestamp_datum(datum_type, adjusted_min_value,
                                                  adjusted_max_value, min_datum,
                                                  max_datum))) {
                LOG_WARN("failed to convert timestamp", K(ret));
              }
            }
          } else if (parquet::LogicalType::TimeUnit::unit::MICROS == static_cast<const parquet::TimestampLogicalType*>(log_type)->time_unit()) {
            if ((ObTimestampType == datum_type.type_ && is_parquet_store_utc(log_type))
                || (ObDateTimeType == datum_type.type_ && !is_parquet_store_utc(log_type))) {
              min_datum.set_timestamp(tmp_min64);
              max_datum.set_timestamp(tmp_max64);
            } else if (ob_is_datetime_or_mysql_datetime_tc(datum_type.type_)
                      || ObTimestampLTZType == datum_type.type_
                      || ObTimestampNanoType == datum_type.type_) {
              int64_t adjusted_min_value = tmp_min64 + adjust_us;
              int64_t adjusted_max_value = tmp_max64 + adjust_us;
              if (OB_FAIL(convert_timestamp_datum(datum_type, adjusted_min_value,
                                                  adjusted_max_value, min_datum,
                                                  max_datum))) {
                LOG_WARN("failed to convert timestamp", K(ret));
              }
            }
          } else if (parquet::LogicalType::TimeUnit::unit::NANOS == static_cast<const parquet::TimestampLogicalType*>(log_type)->time_unit()) {
            if (ob_is_datetime_or_mysql_datetime_tc(datum_type.type_)
                || ObTimestampLTZType == datum_type.type_
                || ObTimestampNanoType == datum_type.type_) {
              int64_t adjusted_min_value = tmp_min64 / NSECS_PER_USEC + adjust_us;
              int64_t adjusted_max_value = tmp_max64 / NSECS_PER_USEC + adjust_us;
              if (OB_FAIL(convert_timestamp_datum(datum_type, adjusted_min_value,
                                                  adjusted_max_value, min_datum,
                                                  max_datum))) {
                LOG_WARN("failed to convert timestamp", K(ret));
              }
            }
          }
      }
      break;
    }
    case parquet::Type::DOUBLE: {
      if (parquet::SortOrder::type::SIGNED != sort_order) {
      } else if (no_log_type && ob_is_double_tc(datum_type.type_)) {
        min_datum.set_double(reinterpret_cast<const double*>(min_val.data())[0]);
        max_datum.set_double(reinterpret_cast<const double*>(max_val.data())[0]);
      }
      break;
    }
    case parquet::Type::FLOAT: {
      if (parquet::SortOrder::type::SIGNED != sort_order) {
      } else if (no_log_type && ob_is_float_tc(datum_type.type_)) {
        min_datum.set_float(reinterpret_cast<const float*>(min_val.data())[0]);
        max_datum.set_float(reinterpret_cast<const float*>(max_val.data())[0]);
      }
      break;
    }
    case parquet::Type::INT96: {
      break;
    }
    case parquet::Type::BYTE_ARRAY: {
      if (ObCharset::is_bin_sort(datum_type.cs_type_)) {
        ObString min_ob_str(min_val.size(), min_val.c_str());
        ObString max_ob_str(max_val.size(), max_val.c_str());
        if ((ob_is_varbinary_or_binary(datum_type.type_, datum_type.cs_type_)
              || ob_is_varchar_or_char(datum_type.type_, datum_type.cs_type_))
            && min_val.size() <= max_length
            && max_val.size() <= max_length) {
          ObString tmp_min;
          ObString tmp_max;
          if (OB_FAIL(ob_write_string(tmp_alloc, min_ob_str, tmp_min))) {
            LOG_WARN("failed to copy string", K(ret));
          } else if (OB_FAIL(ob_write_string(tmp_alloc, max_ob_str, tmp_max))) {
            LOG_WARN("failed to copy string", K(ret));
          } else {
            min_datum.set_string(tmp_min);
            max_datum.set_string(tmp_max);
          }
        } else if (ob_is_text_tc(datum_type.type_)) {
          if (OB_FAIL(ObTextStringHelper::string_to_templob_result(datum_type.type_, has_lob_header, tmp_alloc, min_ob_str,
              min_datum))) {
            LOG_WARN("fail to string to templob result", K(ret));
          } else if (OB_FAIL(ObTextStringHelper::string_to_templob_result(datum_type.type_, has_lob_header, tmp_alloc,
              max_ob_str, max_datum))) {
            LOG_WARN("fail to string to templob result", K(ret));
          }
        }
      }
      break;
    }
    case parquet::Type::FIXED_LEN_BYTE_ARRAY: {
      if (parquet::SortOrder::type::SIGNED != sort_order) {
      } else if (log_type->is_decimal() && ob_is_number_or_decimal_int_tc(datum_type.type_)) {
        if (ob_is_decimal_int_tc(datum_type.type_)
            && get_decimalint_type(datum_type.precision_) > DECIMAL_INT_256) {
          // ObStorageDatum only support 40 byte memory
          // do nothing
        } else {
          ObEvalCtx::TempAllocGuard tmp_alloc_g(eval_ctx);
          int32_t int_bytes = wide::ObDecimalIntConstValue::get_int_bytes_by_precision(
                                                    (datum_type.precision_ == -1)
                                                    ? 38 : datum_type.precision_);
          ObArrayWrap<char> buffer;
          OZ (buffer.allocate_array(tmp_alloc_g.get_allocator(), int_bytes));
          if (OB_FAIL(to_numeric_hive(pointer_cast<const char*>(min_val.data()), min_val.size(),
                                                                buffer.get_data(), buffer.count(),
                                                                datum_type, tmp_alloc_g.get_allocator(), min_datum))) {
            LOG_WARN("failed to convert numeric", K(ret));
          } else if (OB_FAIL(to_numeric_hive(pointer_cast<const char*>(max_val.data()), max_val.size(),
                                                                       buffer.get_data(), buffer.count(),
                                                                       datum_type, tmp_alloc_g.get_allocator(), max_datum))) {
            LOG_WARN("failed to convert numeric", K(ret));
          }
        }
      }
      break;
    }
    case parquet::Type::UNDEFINED: {
      break;
    }
    default:
      break;
  }
  return ret;
}

int ObParquetTableRowIterator::convert_timestamp_datum(const ObDatumMeta &datum_type, int64_t adjusted_min_value,
                                                       int64_t adjusted_max_value, ObStorageDatum &min_datum,
                                                       ObStorageDatum &max_datum)
{
  int ret = OB_SUCCESS;
  if (ob_is_datetime_tc(datum_type.type_)) {
    min_datum.set_timestamp(adjusted_min_value);
    max_datum.set_timestamp(adjusted_max_value);
  } else if (ob_is_mysql_datetime_tc(datum_type.type_)) {
    ObMySQLDateTime min_mdatetime;
    ObMySQLDateTime max_mdatetime;
    if (OB_FAIL(ObTimeConverter::datetime_to_mdatetime(adjusted_min_value, min_mdatetime))) {
      LOG_WARN("failed to convert mysql datetime", K(ret));
    } else if (OB_FAIL(ObTimeConverter::datetime_to_mdatetime(adjusted_max_value, max_mdatetime))) {
      LOG_WARN("failed to convert mysql datetime", K(ret));
    } else {
      min_datum.set_mysql_datetime(min_mdatetime);
      max_datum.set_mysql_datetime(max_mdatetime);
    }
  } else {
    ObOTimestampData min_data;
    ObOTimestampData max_data;
    min_data.time_us_ = adjusted_min_value;
    max_data.time_us_ = adjusted_max_value;
    min_datum.set_otimestamp_tiny(min_data);
    max_datum.set_otimestamp_tiny(max_data);
  }
  return ret;
}

int ObParquetTableRowIterator::ParquetMinMaxFilterParamBuilder::build(const int64_t ext_table_id,
                                                                      const ObExpr *expr,
                                                                      blocksstable::ObMinMaxFilterParam &param)
{
  int ret = OB_SUCCESS;
  std::shared_ptr<parquet::Statistics> stat = rg_reader_->metadata()->ColumnChunk(ext_table_id)->statistics();
  param.set_uncertain();
  if (nullptr != stat) {
    std::string min_val = stat->EncodeMin();
    std::string max_val = stat->EncodeMax();
    parquet::Type::type parquet_type = file_meta_->schema()->Column(ext_table_id)->physical_type();
    parquet::SortOrder::type sort_order = stat->descr()->sort_order();
    const parquet::LogicalType* log_type = file_meta_->schema()->Column(ext_table_id)->logical_type().get();
    if (stat->HasMinMax()) {
      if (OB_FAIL(ObParquetTableRowIterator::read_min_max_datum(min_val, max_val,
                                            parquet_type, log_type,
                                            expr->datum_meta_, expr->max_length_,
                                            sort_order, expr->obj_meta_.has_lob_header(),
                                            row_iter_->scan_param_->op_->get_eval_ctx(),
                                            param.min_datum_, param.max_datum_, tmp_alloc_))) {
        LOG_WARN("failed to read min/max value", K(ret));
      }
    }
    if (!param.min_datum_.is_null() && !param.max_datum_.is_null()) {
      param.null_count_.set_int(1);
    }
    LOG_TRACE("print min/max info", K(parquet_type), K(sort_order), K(log_type->type()),
                                    K(expr->datum_meta_.type_),
                                    K(param.min_datum_), K(param.max_datum_));
  }
  return ret;
}

int ObParquetTableRowIterator::pre_buffer(std::shared_ptr<parquet::RowGroupReader> rg_reader)
{
  int ret = OB_SUCCESS;
  ObFilePreBuffer::ColumnRangeSlicesList column_range_slice_list;
  if (OB_UNLIKELY(column_range_slices_.empty())) {
    if (OB_FAIL(column_range_slices_.prepare_allocate(column_indexs_.count()))) {
      LOG_WARN("fail to prepare allocate array", K(ret));
    } else {
      // init slice for reuse.
      for (int64_t i = 0; OB_SUCC(ret) && i < column_range_slices_.count(); ++i) {
        void *buf = nullptr;
        if (OB_ISNULL(buf = allocator_.alloc(sizeof(ObFilePreBuffer::ColumnRangeSlices)))) {
          ret = OB_ALLOCATE_MEMORY_FAILED;
          LOG_WARN("fail to allocate memory", K(ret));
        } else {
          column_range_slices_.at(i) = new(buf)ObFilePreBuffer::ColumnRangeSlices;
        }
      }
    }
  }
  for (int i = 0; OB_SUCC(ret) && i < column_indexs_.count(); i++) {
    std::unique_ptr<parquet::ColumnChunkMetaData> metadata = rg_reader->metadata()->ColumnChunk(column_indexs_.at(i));
    int64_t col_start = metadata->data_page_offset();
    if (metadata->has_dictionary_page()
        && metadata->dictionary_page_offset() > 0
        && col_start > metadata->dictionary_page_offset()) {
      col_start = metadata->dictionary_page_offset();
    }
    int64_t read_size = rg_reader->metadata()->ColumnChunk(column_indexs_.at(i))->total_compressed_size();
    column_range_slices_.at(i)->range_list_.reuse();
    OZ (column_range_slices_.at(i)->range_list_.push_back(ObFilePreBuffer::ReadRange(col_start, read_size)));
    OZ (column_range_slice_list.push_back(column_range_slices_.at(i)));
  }
  if (OB_SUCC(ret) && !column_range_slice_list.empty()) {
    try {
      ret = file_prebuffer_.pre_buffer(column_range_slice_list);
    } catch (const ObErrorCodeException &ob_error) {
      if (OB_SUCC(ret)) {
        ret = ob_error.get_error_code();
        LOG_WARN("fail to read file", K(ret));
      }
    } catch(const std::exception& e) {
      if (OB_SUCC(ret)) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("unexpected error", K(ret), "Info", e.what());
      }
    } catch(...) {
      if (OB_SUCC(ret)) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("unexpected error", K(ret));
      }
    }
  }

  return ret;
}

DEF_TO_STRING(ObParquetIteratorState)
{
  int64_t pos = 0;
  J_OBJ_START();
  J_NAME("ob_external_iterator_state");
  J_COLON();
  pos += ObExternalIteratorState::to_string(buf + pos, buf_len - pos);
  J_COMMA();
  J_KV(K_(row_group_idx),
       K_(cur_row_group_idx),
       K_(end_row_group_idx),
       K_(cur_row_group_row_count));
  J_OBJ_END();
  return pos;
}


}
}
