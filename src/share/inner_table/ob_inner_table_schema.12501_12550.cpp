/**
 * Copyright (c) 2021 OceanBase
 * OceanBase CE is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#define USING_LOG_PREFIX SHARE_SCHEMA
#include "ob_inner_table_schema.h"

#include "share/schema/ob_schema_macro_define.h"
#include "share/schema/ob_schema_service_sql_impl.h"
#include "share/schema/ob_table_schema.h"
#include "share/scn.h"

namespace oceanbase
{
using namespace share::schema;
using namespace common;
namespace share
{

int ObInnerTableSchema::all_virtual_kv_client_info_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_KV_CLIENT_INFO_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(0);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_KV_CLIENT_INFO_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("client_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObUInt64Type, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(uint64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("client_ip", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      MAX_IP_ADDR_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("client_port", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_ip", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      1, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      MAX_IP_ADDR_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_port", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      2, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("user_name", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_USER_NAME_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("first_login_ts", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("last_login_ts", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("client_info", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      2048, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }
  if (OB_SUCC(ret)) {
    table_schema.get_part_option().set_part_num(1);
    table_schema.set_part_level(PARTITION_LEVEL_ONE);
    table_schema.get_part_option().set_part_func_type(PARTITION_FUNC_TYPE_LIST_COLUMNS);
    if (OB_FAIL(table_schema.get_part_option().set_part_expr("svr_ip, svr_port"))) {
      LOG_WARN("set_part_expr failed", K(ret));
    } else if (OB_FAIL(table_schema.mock_list_partition_array())) {
      LOG_WARN("mock list partition array failed", K(ret));
    }
  }
  table_schema.set_index_using_type(USING_HASH);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_wr_sql_plan_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_WR_SQL_PLAN_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(9);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_WR_SQL_PLAN_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("cluster_id", //column_name
      ++column_id, //column_id
      2, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("snap_id", //column_name
      ++column_id, //column_id
      3, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_ip", //column_name
      ++column_id, //column_id
      4, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      MAX_IP_ADDR_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_port", //column_name
      ++column_id, //column_id
      5, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("sql_id", //column_name
      ++column_id, //column_id
      6, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_SQL_ID_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("plan_hash", //column_name
      ++column_id, //column_id
      7, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObUInt64Type, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(uint64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("plan_id", //column_name
      ++column_id, //column_id
      8, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ObObj id_default;
    id_default.set_uint64(0);
    ADD_COLUMN_SCHEMA_T("id", //column_name
      ++column_id, //column_id
      9, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObUInt64Type, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(uint64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      id_default,
      id_default); //default_value
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("db_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_create", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("operator", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      255, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("options", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      255, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("object_node", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      40, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("object_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("object_owner", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      128, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("object_name", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      128, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("object_alias", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      261, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("object_type", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      20, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("optimizer", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      4000, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("parent_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("depth", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("position", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("is_last_child", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("cost", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      20, //column_precision
      0, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("real_cost", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      20, //column_precision
      0, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("cardinality", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      20, //column_precision
      0, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("real_cardinality", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      20, //column_precision
      0, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("bytes", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      20, //column_precision
      0, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("rowset", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("other_tag", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      4000, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("partition_start", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      4000, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("other", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      4000, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("cpu_cost", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      20, //column_precision
      0, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("io_cost", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      20, //column_precision
      0, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("access_predicates", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      4000, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("filter_predicates", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      4000, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("startup_predicates", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      4000, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("projection", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      4000, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("special_predicates", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      4000, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("qblock_name", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      128, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("remarks", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      4000, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("other_xml", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      4000, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }
  table_schema.set_index_using_type(USING_BTREE);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_wr_res_mgr_sysstat_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_WR_RES_MGR_SYSSTAT_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(7);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_WR_RES_MGR_SYSSTAT_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("group_id", //column_name
      ++column_id, //column_id
      2, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("cluster_id", //column_name
      ++column_id, //column_id
      3, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("snap_id", //column_name
      ++column_id, //column_id
      4, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_ip", //column_name
      ++column_id, //column_id
      5, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      MAX_IP_ADDR_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_port", //column_name
      ++column_id, //column_id
      6, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("stat_id", //column_name
      ++column_id, //column_id
      7, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("value", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false); //is_autoincrement
  }
  table_schema.set_index_using_type(USING_BTREE);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_kv_redis_table_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_KV_REDIS_TABLE_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(2);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_KV_REDIS_TABLE_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("command_name", //column_name
      ++column_id, //column_id
      2, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      1024, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_create", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_modified", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("table_name", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_TABLE_NAME_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }
  table_schema.set_index_using_type(USING_BTREE);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_function_io_stat_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_FUNCTION_IO_STAT_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(5);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_FUNCTION_IO_STAT_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_ip", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      1, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      MAX_IP_ADDR_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_port", //column_name
      ++column_id, //column_id
      2, //rowkey_id
      0, //index_id
      2, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      3, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("function_name", //column_name
      ++column_id, //column_id
      4, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      32, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("mode", //column_name
      ++column_id, //column_id
      5, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      32, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("size", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("real_iops", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("real_mbps", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("schedule_us", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("io_delay_us", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("total_us", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }
  if (OB_SUCC(ret)) {
    table_schema.get_part_option().set_part_num(1);
    table_schema.set_part_level(PARTITION_LEVEL_ONE);
    table_schema.get_part_option().set_part_func_type(PARTITION_FUNC_TYPE_LIST_COLUMNS);
    if (OB_FAIL(table_schema.get_part_option().set_part_expr("svr_ip, svr_port"))) {
      LOG_WARN("set_part_expr failed", K(ret));
    } else if (OB_FAIL(table_schema.mock_list_partition_array())) {
      LOG_WARN("mock list partition array failed", K(ret));
    }
  }
  table_schema.set_index_using_type(USING_HASH);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_temp_file_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_TEMP_FILE_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(0);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_TEMP_FILE_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_ip", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      1, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      MAX_IP_ADDR_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_port", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      2, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("file_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ObObj trace_id_default;
    trace_id_default.set_varchar(ObString::make_string(""));
    ADD_COLUMN_SCHEMA_T("trace_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_TRACE_ID_BUFFER_SIZE, //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false, //is_autoincrement
      trace_id_default,
      trace_id_default); //default_value
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("dir_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("data_bytes", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("start_offset", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("is_deleting", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTinyIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      1, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("cached_data_page_num", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("write_back_data_page_num", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("flushed_data_page_num", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("ref_cnt", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("total_writes", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("unaligned_writes", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("total_reads", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("unaligned_reads", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("total_read_bytes", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("last_access_time", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("last_modify_time", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("birth_time", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("file_ptr", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      20, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ObObj file_label_default;
    file_label_default.set_varchar(ObString::make_string(""));
    ADD_COLUMN_SCHEMA_T("file_label", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      16, //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false, //is_autoincrement
      file_label_default,
      file_label_default); //default_value
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("meta_tree_epoch", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("meta_tree_levels", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("meta_bytes", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("cached_meta_page_num", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("write_back_meta_page_num", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("page_flush_cnt", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("type", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("compressible_fd", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("persisted_tail_page_writes", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("lack_page_cnt", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("total_truncated_page_read_cnt", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("truncated_page_hits", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("total_kv_cache_page_read_cnt", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("kv_cache_page_read_hits", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("total_uncached_page_read_cnt", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("uncached_page_hits", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("aggregate_read_io_cnt", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("total_wbp_page_read_cnt", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("wbp_page_hits", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }
  if (OB_SUCC(ret)) {
    table_schema.get_part_option().set_part_num(1);
    table_schema.set_part_level(PARTITION_LEVEL_ONE);
    table_schema.get_part_option().set_part_func_type(PARTITION_FUNC_TYPE_LIST_COLUMNS);
    if (OB_FAIL(table_schema.get_part_option().set_part_expr("svr_ip, svr_port"))) {
      LOG_WARN("set_part_expr failed", K(ret));
    } else if (OB_FAIL(table_schema.mock_list_partition_array())) {
      LOG_WARN("mock list partition array failed", K(ret));
    }
  }
  table_schema.set_index_using_type(USING_HASH);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_ncomp_dll_v2_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_NCOMP_DLL_V2_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(6);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_NCOMP_DLL_V2_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("database_id", //column_name
      ++column_id, //column_id
      2, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("key_id", //column_name
      ++column_id, //column_id
      3, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("compile_db_id", //column_name
      ++column_id, //column_id
      4, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("arch_type", //column_name
      ++column_id, //column_id
      5, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      128, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("build_version", //column_name
      ++column_id, //column_id
      6, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_SERVER_VERSION_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_create", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_modified", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("merge_version", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("dll", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObLongTextType, //column_type
      CS_TYPE_BINARY, //column_collation_type
      0, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("stack_size", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObLongTextType, //column_type
      CS_TYPE_BINARY, //column_collation_type
      0, //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("extra_info", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObLongTextType, //column_type
      CS_TYPE_BINARY, //column_collation_type
      0, //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false); //is_autoincrement
  }
  table_schema.set_index_using_type(USING_BTREE);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_wr_sql_plan_aux_key2snapshot_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_WR_SQL_PLAN_AUX_KEY2SNAPSHOT_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(9);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_WR_SQL_PLAN_AUX_KEY2SNAPSHOT_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_ip", //column_name
      ++column_id, //column_id
      2, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      MAX_IP_ADDR_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_port", //column_name
      ++column_id, //column_id
      3, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("sql_id", //column_name
      ++column_id, //column_id
      4, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_SQL_ID_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("plan_hash", //column_name
      ++column_id, //column_id
      5, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObUInt64Type, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(uint64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ObObj id_default;
    id_default.set_uint64(0);
    ADD_COLUMN_SCHEMA_T("id", //column_name
      ++column_id, //column_id
      6, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObUInt64Type, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(uint64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      id_default,
      id_default); //default_value
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("plan_id", //column_name
      ++column_id, //column_id
      7, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("snap_id", //column_name
      ++column_id, //column_id
      8, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("cluster_id", //column_name
      ++column_id, //column_id
      9, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }
  table_schema.set_index_using_type(USING_BTREE);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_cs_replica_tablet_stats_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_CS_REPLICA_TABLET_STATS_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(0);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_CS_REPLICA_TABLET_STATS_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_ip", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      1, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      MAX_IP_ADDR_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_port", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      2, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("ls_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tablet_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("macro_block_cnt", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("is_cs", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTinyIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      1, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("is_cs_replica", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTinyIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      1, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("available", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTinyIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      1, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }
  if (OB_SUCC(ret)) {
    table_schema.get_part_option().set_part_num(1);
    table_schema.set_part_level(PARTITION_LEVEL_ONE);
    table_schema.get_part_option().set_part_func_type(PARTITION_FUNC_TYPE_LIST_COLUMNS);
    if (OB_FAIL(table_schema.get_part_option().set_part_expr("svr_ip, svr_port"))) {
      LOG_WARN("set_part_expr failed", K(ret));
    } else if (OB_FAIL(table_schema.mock_list_partition_array())) {
      LOG_WARN("mock list partition array failed", K(ret));
    }
  }
  table_schema.set_index_using_type(USING_HASH);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_ddl_diagnose_info_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_DDL_DIAGNOSE_INFO_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(0);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_DDL_DIAGNOSE_INFO_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("ddl_task_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("object_table_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("opname", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_DDL_ID_STR_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("create_time", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("finish_time", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("diagnose_info", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_DIAGNOSE_INFO_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }
  table_schema.set_index_using_type(USING_HASH);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_ddl_diagnose_info_all_virtual_ddl_diagnose_info_i1_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_DDL_DIAGNOSE_INFO_ALL_VIRTUAL_DDL_DIAGNOSE_INFO_I1_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(0);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(USER_INDEX);
  table_schema.set_index_type(INDEX_TYPE_NORMAL_LOCAL);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_DDL_DIAGNOSE_INFO_ALL_VIRTUAL_DDL_DIAGNOSE_INFO_I1_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));
  table_schema.set_index_using_type(USING_HASH);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("ddl_task_id", //column_name
      column_id + 2, //column_id
      1, //rowkey_id
      1, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false,//is_nullable
      false); //is_autoincrement
  }
  table_schema.set_index_status(INDEX_STATUS_AVAILABLE);
  table_schema.set_index_type(INDEX_TYPE_NORMAL_LOCAL);
  table_schema.set_data_table_id(OB_ALL_VIRTUAL_DDL_DIAGNOSE_INFO_TID);

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_WITH_COLUMN_FLAGS("tenant_id", //column_name
      column_id + 1, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false,//is_nullable
      false,//is_autoincrement
      false,//is_hidden
      true);//is_storing_column
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_WITH_COLUMN_FLAGS("object_table_id", //column_name
      column_id + 3, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false,//is_nullable
      false,//is_autoincrement
      false,//is_hidden
      true);//is_storing_column
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_WITH_COLUMN_FLAGS("opname", //column_name
      column_id + 4, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_DDL_ID_STR_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false,//is_nullable
      false,//is_autoincrement
      false,//is_hidden
      true);//is_storing_column
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS_WITH_COLUMN_FLAGS("create_time", //column_name
      column_id + 5, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false, //is_on_update_for_timestamp
      false, //is_hidden
      true);//is_storing_column
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS_WITH_COLUMN_FLAGS("finish_time", //column_name
      column_id + 6, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false, //is_on_update_for_timestamp
      false, //is_hidden
      true);//is_storing_column
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_WITH_COLUMN_FLAGS("diagnose_info", //column_name
      column_id + 7, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_DIAGNOSE_INFO_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false,//is_nullable
      false,//is_autoincrement
      false,//is_hidden
      true);//is_storing_column
  }

  table_schema.set_max_used_column_id(column_id + 7);
  return ret;
}

int ObInnerTableSchema::all_virtual_plugin_info_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_PLUGIN_INFO_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(0);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_PLUGIN_INFO_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_ip", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      1, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      MAX_IP_ADDR_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_port", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      2, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ObObj name_default;
    name_default.set_null();
    ADD_COLUMN_SCHEMA_T("name", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      64, //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false, //is_autoincrement
      name_default,
      name_default); //default_value
  }

  if (OB_SUCC(ret)) {
    ObObj status_default;
    status_default.set_null();
    ADD_COLUMN_SCHEMA_T("status", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      64, //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false, //is_autoincrement
      status_default,
      status_default); //default_value
  }

  if (OB_SUCC(ret)) {
    ObObj type_default;
    type_default.set_null();
    ADD_COLUMN_SCHEMA_T("type", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      80, //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false, //is_autoincrement
      type_default,
      type_default); //default_value
  }

  if (OB_SUCC(ret)) {
    ObObj library_default;
    library_default.set_null();
    ADD_COLUMN_SCHEMA_T("library", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      128, //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false, //is_autoincrement
      library_default,
      library_default); //default_value
  }

  if (OB_SUCC(ret)) {
    ObObj library_version_default;
    library_version_default.set_null();
    ADD_COLUMN_SCHEMA_T("library_version", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      80, //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false, //is_autoincrement
      library_version_default,
      library_version_default); //default_value
  }

  if (OB_SUCC(ret)) {
    ObObj library_revision_default;
    library_revision_default.set_null();
    ADD_COLUMN_SCHEMA_T("library_revision", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      80, //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false, //is_autoincrement
      library_revision_default,
      library_revision_default); //default_value
  }

  if (OB_SUCC(ret)) {
    ObObj interface_version_default;
    interface_version_default.set_null();
    ADD_COLUMN_SCHEMA_T("interface_version", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      80, //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false, //is_autoincrement
      interface_version_default,
      interface_version_default); //default_value
  }

  if (OB_SUCC(ret)) {
    ObObj author_default;
    author_default.set_null();
    ADD_COLUMN_SCHEMA_T("author", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      64, //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false, //is_autoincrement
      author_default,
      author_default); //default_value
  }

  if (OB_SUCC(ret)) {
    ObObj license_default;
    license_default.set_null();
    ADD_COLUMN_SCHEMA_T("license", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      64, //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false, //is_autoincrement
      license_default,
      license_default); //default_value
  }

  if (OB_SUCC(ret)) {
    ObObj description_default;
    description_default.set_null();
    ADD_COLUMN_SCHEMA_T("description", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      65535, //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false, //is_autoincrement
      description_default,
      description_default); //default_value
  }
  if (OB_SUCC(ret)) {
    table_schema.get_part_option().set_part_num(1);
    table_schema.set_part_level(PARTITION_LEVEL_ONE);
    table_schema.get_part_option().set_part_func_type(PARTITION_FUNC_TYPE_LIST_COLUMNS);
    if (OB_FAIL(table_schema.get_part_option().set_part_expr("svr_ip, svr_port"))) {
      LOG_WARN("set_part_expr failed", K(ret));
    } else if (OB_FAIL(table_schema.mock_list_partition_array())) {
      LOG_WARN("mock list partition array failed", K(ret));
    }
  }
  table_schema.set_index_using_type(USING_HASH);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_catalog_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_CATALOG_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(2);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_CATALOG_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("catalog_id", //column_name
      ++column_id, //column_id
      2, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_create", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_modified", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ObObj catalog_name_default;
    catalog_name_default.set_varchar(ObString::make_string(""));
    ADD_COLUMN_SCHEMA_T("catalog_name", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_CATALOG_NAME_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      catalog_name_default,
      catalog_name_default); //default_value
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("catalog_properties", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_BINARY, //column_collation_type
      OB_MAX_VARCHAR_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false); //is_autoincrement
  }
  table_schema.set_index_using_type(USING_BTREE);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_catalog_history_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_CATALOG_HISTORY_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(3);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_CATALOG_HISTORY_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("catalog_id", //column_name
      ++column_id, //column_id
      2, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("schema_version", //column_name
      ++column_id, //column_id
      3, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_create", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_modified", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("is_deleted", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ObObj catalog_name_default;
    catalog_name_default.set_varchar(ObString::make_string(""));
    ADD_COLUMN_SCHEMA_T("catalog_name", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_CATALOG_NAME_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false, //is_autoincrement
      catalog_name_default,
      catalog_name_default); //default_value
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("catalog_properties", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_BINARY, //column_collation_type
      OB_MAX_VARCHAR_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false); //is_autoincrement
  }
  table_schema.set_index_using_type(USING_BTREE);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_catalog_privilege_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_CATALOG_PRIVILEGE_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(3);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_CATALOG_PRIVILEGE_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("user_id", //column_name
      ++column_id, //column_id
      2, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("catalog_name", //column_name
      ++column_id, //column_id
      3, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_BINARY, //column_collation_type
      OB_MAX_CATALOG_NAME_BINARY_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_create", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_modified", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ObObj priv_set_default;
    priv_set_default.set_int(0);
    ADD_COLUMN_SCHEMA_T("priv_set", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      priv_set_default,
      priv_set_default); //default_value
  }
  table_schema.set_index_using_type(USING_BTREE);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_catalog_privilege_history_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_CATALOG_PRIVILEGE_HISTORY_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(4);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_CATALOG_PRIVILEGE_HISTORY_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("user_id", //column_name
      ++column_id, //column_id
      2, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("catalog_name", //column_name
      ++column_id, //column_id
      3, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_BINARY, //column_collation_type
      OB_MAX_CATALOG_NAME_BINARY_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("schema_version", //column_name
      ++column_id, //column_id
      4, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_create", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_modified", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("is_deleted", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ObObj priv_set_default;
    priv_set_default.set_int(0);
    ADD_COLUMN_SCHEMA_T("priv_set", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false, //is_autoincrement
      priv_set_default,
      priv_set_default); //default_value
  }
  table_schema.set_index_using_type(USING_BTREE);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_sswriter_group_stat_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_SSWRITER_GROUP_STAT_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(0);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_SSWRITER_GROUP_STAT_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_ip", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      1, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      MAX_IP_ADDR_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_port", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      2, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("ls_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("type", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      32, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("group_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("grant_svr_ip", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      MAX_IP_ADDR_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("grant_svr_port", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("current_ts", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("remaining_lease_us", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("lease_expire_ts", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("create_ts", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("epoch", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("last_request_lease_ts", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }
  if (OB_SUCC(ret)) {
    table_schema.get_part_option().set_part_num(1);
    table_schema.set_part_level(PARTITION_LEVEL_ONE);
    table_schema.get_part_option().set_part_func_type(PARTITION_FUNC_TYPE_LIST_COLUMNS);
    if (OB_FAIL(table_schema.get_part_option().set_part_expr("svr_ip, svr_port"))) {
      LOG_WARN("set_part_expr failed", K(ret));
    } else if (OB_FAIL(table_schema.mock_list_partition_array())) {
      LOG_WARN("mock list partition array failed", K(ret));
    }
  }
  table_schema.set_index_using_type(USING_HASH);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_sswriter_lease_mgr_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_SSWRITER_LEASE_MGR_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(0);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_SSWRITER_LEASE_MGR_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_ip", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      1, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      MAX_IP_ADDR_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_port", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      2, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("ls_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("type", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      32, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("group_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("region", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      MAX_REGION_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("state", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      32, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("target_svr_ip", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      MAX_IP_ADDR_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("target_svr_port", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("current_ts", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("last_update_state_ts", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("lease_expire_ts", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("epoch", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }
  if (OB_SUCC(ret)) {
    table_schema.get_part_option().set_part_num(1);
    table_schema.set_part_level(PARTITION_LEVEL_ONE);
    table_schema.get_part_option().set_part_func_type(PARTITION_FUNC_TYPE_LIST_COLUMNS);
    if (OB_FAIL(table_schema.get_part_option().set_part_expr("svr_ip, svr_port"))) {
      LOG_WARN("set_part_expr failed", K(ret));
    } else if (OB_FAIL(table_schema.mock_list_partition_array())) {
      LOG_WARN("mock list partition array failed", K(ret));
    }
  }
  table_schema.set_index_using_type(USING_HASH);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_pl_recompile_objinfo_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_PL_RECOMPILE_OBJINFO_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(2);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_PL_RECOMPILE_OBJINFO_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("recompile_obj_id", //column_name
      ++column_id, //column_id
      2, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_create", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_modified", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("ref_obj_name", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_CORE_TALBE_NAME_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("schema_version", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("fail_count", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }
  table_schema.set_index_using_type(USING_BTREE);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_vector_index_task_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_VECTOR_INDEX_TASK_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(4);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_VECTOR_INDEX_TASK_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("table_id", //column_name
      ++column_id, //column_id
      2, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tablet_id", //column_name
      ++column_id, //column_id
      3, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("task_id", //column_name
      ++column_id, //column_id
      4, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_create", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_modified", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("trigger_type", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("task_type", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("status", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("target_scn", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("ret_code", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("trace_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_ERROR_MSG_LEN, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }
  table_schema.set_index_using_type(USING_BTREE);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_vector_index_task_history_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_VECTOR_INDEX_TASK_HISTORY_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(4);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_VECTOR_INDEX_TASK_HISTORY_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("table_id", //column_name
      ++column_id, //column_id
      2, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tablet_id", //column_name
      ++column_id, //column_id
      3, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("task_id", //column_name
      ++column_id, //column_id
      4, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_create", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_modified", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("trigger_type", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("task_type", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("status", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("target_scn", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("ret_code", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("trace_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_ERROR_MSG_LEN, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }
  table_schema.set_index_using_type(USING_BTREE);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::tenant_virtual_show_create_catalog_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_TENANT_VIRTUAL_SHOW_CREATE_CATALOG_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(1);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_TENANT_VIRTUAL_SHOW_CREATE_CATALOG_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("catalog_id", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("catalog_name", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_CATALOG_NAME_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("create_catalog", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObLongTextType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      0, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }
  table_schema.set_index_using_type(USING_HASH);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::tenant_virtual_show_catalog_databases_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_TENANT_VIRTUAL_SHOW_CATALOG_DATABASES_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(2);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_TENANT_VIRTUAL_SHOW_CATALOG_DATABASES_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("catalog_id", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("database_name", //column_name
      ++column_id, //column_id
      2, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_TABLE_NAME_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }
  table_schema.set_index_using_type(USING_HASH);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_storage_cache_task_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_STORAGE_CACHE_TASK_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(4);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_STORAGE_CACHE_TASK_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_ip", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      1, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      MAX_IP_ADDR_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_port", //column_name
      ++column_id, //column_id
      2, //rowkey_id
      0, //index_id
      2, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      3, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tablet_id", //column_name
      ++column_id, //column_id
      4, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("status", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      64, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("speed", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      29, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("start_time", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("complete_time", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("result", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("comment", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      4096, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }
  if (OB_SUCC(ret)) {
    table_schema.get_part_option().set_part_num(1);
    table_schema.set_part_level(PARTITION_LEVEL_ONE);
    table_schema.get_part_option().set_part_func_type(PARTITION_FUNC_TYPE_LIST_COLUMNS);
    if (OB_FAIL(table_schema.get_part_option().set_part_expr("svr_ip, svr_port"))) {
      LOG_WARN("set_part_expr failed", K(ret));
    } else if (OB_FAIL(table_schema.mock_list_partition_array())) {
      LOG_WARN("mock list partition array failed", K(ret));
    }
  }
  table_schema.set_index_using_type(USING_HASH);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_tablet_local_cache_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_TABLET_LOCAL_CACHE_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(4);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_TABLET_LOCAL_CACHE_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_ip", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      1, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      MAX_IP_ADDR_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_port", //column_name
      ++column_id, //column_id
      2, //rowkey_id
      0, //index_id
      2, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      3, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tablet_id", //column_name
      ++column_id, //column_id
      4, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("storage_cache_policy", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      64, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("cached_data_size", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("cache_hit_count", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("cache_miss_count", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("cache_hit_size", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("cache_miss_size", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("info", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      4096, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }
  if (OB_SUCC(ret)) {
    table_schema.get_part_option().set_part_num(1);
    table_schema.set_part_level(PARTITION_LEVEL_ONE);
    table_schema.get_part_option().set_part_func_type(PARTITION_FUNC_TYPE_LIST_COLUMNS);
    if (OB_FAIL(table_schema.get_part_option().set_part_expr("svr_ip, svr_port"))) {
      LOG_WARN("set_part_expr failed", K(ret));
    } else if (OB_FAIL(table_schema.mock_list_partition_array())) {
      LOG_WARN("mock list partition array failed", K(ret));
    }
  }
  table_schema.set_index_using_type(USING_HASH);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_mview_running_job_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_MVIEW_RUNNING_JOB_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(0);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_MVIEW_RUNNING_JOB_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_ip", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      1, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      MAX_IP_ADDR_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_port", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      2, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("table_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("job_type", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObUInt64Type, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(uint64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("session_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObUInt64Type, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(uint64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("read_snapshot", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("parallel", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("job_start_time", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("target_data_sync_scn", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObUInt64Type, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(uint64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }
  if (OB_SUCC(ret)) {
    table_schema.get_part_option().set_part_num(1);
    table_schema.set_part_level(PARTITION_LEVEL_ONE);
    table_schema.get_part_option().set_part_func_type(PARTITION_FUNC_TYPE_LIST_COLUMNS);
    if (OB_FAIL(table_schema.get_part_option().set_part_expr("svr_ip, svr_port"))) {
      LOG_WARN("set_part_expr failed", K(ret));
    } else if (OB_FAIL(table_schema.mock_list_partition_array())) {
      LOG_WARN("mock list partition array failed", K(ret));
    }
  }
  table_schema.set_index_using_type(USING_HASH);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_dynamic_partition_table_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_DYNAMIC_PARTITION_TABLE_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(0);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_DYNAMIC_PARTITION_TABLE_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_schema_version", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("database_name", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_DATABASE_NAME_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("table_name", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_TABLE_NAME_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("table_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("max_high_bound_val", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_PARTITION_EXPR_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("enable", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      1024, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("time_unit", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      1024, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("precreate_time", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      1024, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("expire_time", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      1024, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("time_zone", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      1024, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("bigint_precision", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      1024, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }
  table_schema.set_index_using_type(USING_HASH);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_ls_migration_task_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_LS_MIGRATION_TASK_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(0);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_LS_MIGRATION_TASK_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_ip", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      1, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      MAX_IP_ADDR_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_port", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      2, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("ls_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("type", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_SYS_TASK_TYPE_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("status", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_DEFAULT_STATUS_LENTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("task_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_TRACE_ID_BUFFER_SIZE, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("priority", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("config_version", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      128, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("src", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      MAX_IP_PORT_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("dst", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      MAX_IP_PORT_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("data_src", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      MAX_IP_PORT_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("paxos_replica_number", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("prioritize_same_zone_src", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      MAX_BOOL_STR_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }
  if (OB_SUCC(ret)) {
    table_schema.get_part_option().set_part_num(1);
    table_schema.set_part_level(PARTITION_LEVEL_ONE);
    table_schema.get_part_option().set_part_func_type(PARTITION_FUNC_TYPE_LIST_COLUMNS);
    if (OB_FAIL(table_schema.get_part_option().set_part_expr("svr_ip, svr_port"))) {
      LOG_WARN("set_part_expr failed", K(ret));
    } else if (OB_FAIL(table_schema.mock_list_partition_array())) {
      LOG_WARN("mock list partition array failed", K(ret));
    }
  }
  table_schema.set_index_using_type(USING_HASH);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_ss_notify_tasks_stat_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_SS_NOTIFY_TASKS_STAT_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(4);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_SS_NOTIFY_TASKS_STAT_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("ls_id", //column_name
      ++column_id, //column_id
      2, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tablet_id", //column_name
      ++column_id, //column_id
      3, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("id", //column_name
      ++column_id, //column_id
      4, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_ip", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      1, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      MAX_IP_ADDR_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_port", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      2, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("state", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObLongTextType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      0, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("type", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObLongTextType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      0, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("reorganization_scn", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("acquire_sslog_scn", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("acquire_aim_ls_scn", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("sslog_kv_commit_version", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("notify_path", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObLongTextType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      0, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("generate_ts", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("enqueue_ts", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("ready_ts", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("consume_ts", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("retire_reason", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObLongTextType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      0, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("err_ret", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("retry_cnt", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }
  if (OB_SUCC(ret)) {
    table_schema.get_part_option().set_part_num(1);
    table_schema.set_part_level(PARTITION_LEVEL_ONE);
    table_schema.get_part_option().set_part_func_type(PARTITION_FUNC_TYPE_LIST_COLUMNS);
    if (OB_FAIL(table_schema.get_part_option().set_part_expr("svr_ip, svr_port"))) {
      LOG_WARN("set_part_expr failed", K(ret));
    } else if (OB_FAIL(table_schema.mock_list_partition_array())) {
      LOG_WARN("mock list partition array failed", K(ret));
    }
  }
  table_schema.set_index_using_type(USING_HASH);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_ss_notify_tablets_stat_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_SS_NOTIFY_TABLETS_STAT_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(3);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_SS_NOTIFY_TABLETS_STAT_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("ls_id", //column_name
      ++column_id, //column_id
      2, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tablet_id", //column_name
      ++column_id, //column_id
      3, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("reorganization_scn", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_ip", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      1, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      MAX_IP_ADDR_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("svr_port", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      2, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("apply_ss_change_version", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("notify_ss_change_version", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("applying_ss_change_version", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("oldest_ss_change_version", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }
  if (OB_SUCC(ret)) {
    table_schema.get_part_option().set_part_num(1);
    table_schema.set_part_level(PARTITION_LEVEL_ONE);
    table_schema.get_part_option().set_part_func_type(PARTITION_FUNC_TYPE_LIST_COLUMNS);
    if (OB_FAIL(table_schema.get_part_option().set_part_expr("svr_ip, svr_port"))) {
      LOG_WARN("set_part_expr failed", K(ret));
    } else if (OB_FAIL(table_schema.mock_list_partition_array())) {
      LOG_WARN("mock list partition array failed", K(ret));
    }
  }
  table_schema.set_index_using_type(USING_HASH);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_tenant_location_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_TENANT_LOCATION_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(2);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_TENANT_LOCATION_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("location_id", //column_name
      ++column_id, //column_id
      2, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_create", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_modified", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ObObj location_name_default;
    location_name_default.set_varchar(ObString::make_string(""));
    ADD_COLUMN_SCHEMA_T("location_name", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_LOCATION_NAME_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      location_name_default,
      location_name_default); //default_value
  }

  if (OB_SUCC(ret)) {
    ObObj location_url_default;
    location_url_default.set_varchar(ObString::make_string(""));
    ADD_COLUMN_SCHEMA_T("location_url", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_LOCATION_URL_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      location_url_default,
      location_url_default); //default_value
  }

  if (OB_SUCC(ret)) {
    ObObj location_access_info_default;
    location_access_info_default.set_varchar(ObString::make_string(""));
    ADD_COLUMN_SCHEMA_T("location_access_info", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_LOCATION_ACCESS_INFO_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      location_access_info_default,
      location_access_info_default); //default_value
  }
  table_schema.set_index_using_type(USING_BTREE);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_tenant_location_history_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_TENANT_LOCATION_HISTORY_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(3);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_TENANT_LOCATION_HISTORY_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("location_id", //column_name
      ++column_id, //column_id
      2, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("schema_version", //column_name
      ++column_id, //column_id
      3, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_create", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_modified", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("is_deleted", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ObObj location_name_default;
    location_name_default.set_varchar(ObString::make_string(""));
    ADD_COLUMN_SCHEMA_T("location_name", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_LOCATION_NAME_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false, //is_autoincrement
      location_name_default,
      location_name_default); //default_value
  }

  if (OB_SUCC(ret)) {
    ObObj location_url_default;
    location_url_default.set_varchar(ObString::make_string(""));
    ADD_COLUMN_SCHEMA_T("location_url", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_LOCATION_URL_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false, //is_autoincrement
      location_url_default,
      location_url_default); //default_value
  }

  if (OB_SUCC(ret)) {
    ObObj location_access_info_default;
    location_access_info_default.set_varchar(ObString::make_string(""));
    ADD_COLUMN_SCHEMA_T("location_access_info", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_LOCATION_ACCESS_INFO_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false, //is_autoincrement
      location_access_info_default,
      location_access_info_default); //default_value
  }
  table_schema.set_index_using_type(USING_BTREE);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_objauth_mysql_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_OBJAUTH_MYSQL_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(4);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_OBJAUTH_MYSQL_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("user_id", //column_name
      ++column_id, //column_id
      2, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("obj_name", //column_name
      ++column_id, //column_id
      3, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_CORE_TALBE_NAME_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("obj_type", //column_name
      ++column_id, //column_id
      4, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_create", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_modified", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ObObj all_priv_default;
    all_priv_default.set_int(0);
    ADD_COLUMN_SCHEMA_T("all_priv", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      all_priv_default,
      all_priv_default); //default_value
  }

  if (OB_SUCC(ret)) {
    ObObj grantor_default;
    grantor_default.set_varchar(ObString::make_string(""));
    ADD_COLUMN_SCHEMA_T("grantor", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_USER_NAME_LENGTH_STORE, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      grantor_default,
      grantor_default); //default_value
  }

  if (OB_SUCC(ret)) {
    ObObj grantor_host_default;
    grantor_host_default.set_varchar(ObString::make_string(""));
    ADD_COLUMN_SCHEMA_T("grantor_host", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_HOST_NAME_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      grantor_host_default,
      grantor_host_default); //default_value
  }
  table_schema.set_index_using_type(USING_BTREE);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_objauth_mysql_history_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_OBJAUTH_MYSQL_HISTORY_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(5);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_OBJAUTH_MYSQL_HISTORY_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("user_id", //column_name
      ++column_id, //column_id
      2, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("obj_name", //column_name
      ++column_id, //column_id
      3, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_CORE_TALBE_NAME_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("obj_type", //column_name
      ++column_id, //column_id
      4, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("schema_version", //column_name
      ++column_id, //column_id
      5, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_create", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_modified", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("is_deleted", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ObObj all_priv_default;
    all_priv_default.set_int(0);
    ADD_COLUMN_SCHEMA_T("all_priv", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false, //is_autoincrement
      all_priv_default,
      all_priv_default); //default_value
  }

  if (OB_SUCC(ret)) {
    ObObj grantor_default;
    grantor_default.set_varchar(ObString::make_string(""));
    ADD_COLUMN_SCHEMA_T("grantor", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_USER_NAME_LENGTH_STORE, //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false, //is_autoincrement
      grantor_default,
      grantor_default); //default_value
  }

  if (OB_SUCC(ret)) {
    ObObj grantor_host_default;
    grantor_host_default.set_varchar(ObString::make_string(""));
    ADD_COLUMN_SCHEMA_T("grantor_host", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_HOST_NAME_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false, //is_autoincrement
      grantor_host_default,
      grantor_host_default); //default_value
  }
  table_schema.set_index_using_type(USING_BTREE);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::tenant_virtual_show_create_location_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_TENANT_VIRTUAL_SHOW_CREATE_LOCATION_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(1);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_TENANT_VIRTUAL_SHOW_CREATE_LOCATION_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("location_id", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("location_name", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_LOCATION_NAME_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("create_location", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      LOCATION_DEFINE_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }
  table_schema.set_index_using_type(USING_HASH);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::tenant_virtual_list_file_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_TENANT_VIRTUAL_LIST_FILE_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(3);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_TENANT_VIRTUAL_LIST_FILE_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("location_id", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("location_sub_path", //column_name
      ++column_id, //column_id
      2, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_LOCATION_NAME_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("pattern", //column_name
      ++column_id, //column_id
      3, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_LOCATION_NAME_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("file_name", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      16384, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("file_size", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }
  table_schema.set_index_using_type(USING_HASH);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_external_resource_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_EXTERNAL_RESOURCE_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(2);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_EXTERNAL_RESOURCE_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("resource_id", //column_name
      ++column_id, //column_id
      2, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_create", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_modified", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("database_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("name", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_TABLE_NAME_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("type", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("content", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObLongTextType, //column_type
      CS_TYPE_BINARY, //column_collation_type
      0, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("comment", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      MAX_TABLE_COMMENT_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }
  table_schema.set_index_using_type(USING_BTREE);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}

int ObInnerTableSchema::all_virtual_external_resource_history_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_EXTERNAL_RESOURCE_HISTORY_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(3);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_EXTERNAL_RESOURCE_HISTORY_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCharset::get_default_collation(ObCharset::get_default_charset()));

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("tenant_id", //column_name
      ++column_id, //column_id
      1, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("resource_id", //column_name
      ++column_id, //column_id
      2, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("schema_version", //column_name
      ++column_id, //column_id
      3, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_create", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA_TS("gmt_modified", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObTimestampType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(ObPreciseDateTime), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false, //is_autoincrement
      false); //is_on_update_for_timestamp
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("is_deleted", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("database_id", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("name", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      OB_MAX_TABLE_NAME_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("type", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObIntType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      sizeof(int64_t), //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("content", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObLongTextType, //column_type
      CS_TYPE_BINARY, //column_collation_type
      0, //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("comment", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      MAX_TABLE_COMMENT_LENGTH, //column_length
      -1, //column_precision
      -1, //column_scale
      true, //is_nullable
      false); //is_autoincrement
  }
  table_schema.set_index_using_type(USING_BTREE);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);
  table_schema.set_micro_index_clustered(false);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}


} // end namespace share
} // end namespace oceanbase
