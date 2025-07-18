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


#include "storage/tablet/ob_i_tablet_mds_interface.h"
#include "storage/tablet/ob_mds_scan_param_helper.h"
#include "storage/tablet/ob_mds_schema_helper.h"
#include "storage/tx_storage/ob_ls_service.h"

namespace oceanbase
{
namespace storage
{
int ObITabletMdsInterface::get_src_tablet_handle_and_base_ptr_(
    ObTabletHandle &tablet_handle,
    ObITabletMdsInterface *&base_ptr) const
{
  int ret = OB_SUCCESS;
  const share::ObLSID &ls_id = get_tablet_meta_().transfer_info_.ls_id_;
  const common::ObTabletID &tablet_id = get_tablet_meta_().tablet_id_;
  ObLSService *ls_service = MTL(ObLSService*);
  ObLSHandle ls_handle;
  ObLS *ls = nullptr;
  ObTablet *tablet = nullptr;

  if (OB_FAIL(ls_service->get_ls(ls_id, ls_handle, ObLSGetMod::TABLET_MOD))) {
    MDS_LOG(WARN, "fail to get ls", K(ret), K(ls_id));
  } else if (OB_ISNULL(ls = ls_handle.get_ls())) {
    ret = OB_ERR_UNEXPECTED;
    MDS_LOG(WARN, "ls is null", K(ret), KP(ls), K(ls_id));
  } else if (OB_FAIL(ls->get_tablet(tablet_id, tablet_handle, 0, ObMDSGetTabletMode::READ_WITHOUT_CHECK))) {
    MDS_LOG(WARN, "fail to get tablet", K(ret), K(ls_id), K(tablet_id));
  } else if (OB_ISNULL(tablet = tablet_handle.get_obj())) {
    ret = OB_ERR_UNEXPECTED;
    MDS_LOG(WARN, "tablet is null", K(ret), K(ls_id), K(tablet_id), K(tablet_handle));
  } else {
    base_ptr = static_cast<ObITabletMdsInterface*>(tablet);
  }

  return ret;
}

int ObITabletMdsInterface::get_tablet_status(
    const share::SCN &snapshot,
    ObTabletCreateDeleteMdsUserData &data,
    const int64_t timeout) const
{
  #define PRINT_WRAPPER KR(ret), K(snapshot), K(data), K(timeout), K(*this)
  MDS_TG(10_ms);
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!check_is_inited_())) {
    ret = OB_NOT_INIT;
    MDS_LOG_GET(WARN, "not inited");
  } else if (OB_UNLIKELY(!snapshot.is_max())) {
    ret = OB_NOT_SUPPORTED;
    MDS_LOG_GET(WARN, "only support read latest data currently");
  } else if (CLICK_FAIL((get_snapshot<mds::DummyKey, ObTabletCreateDeleteMdsUserData>(
      mds::DummyKey(),
      ReadTabletStatusOp(data),
      snapshot,
      timeout)))) {
    if (OB_EMPTY_RESULT != ret) {
      MDS_LOG(WARN, "fail to get snapshot", K(ret));
    }
  } else if (GCTX.is_shared_storage_mode() && OB_FAIL(get_tablet_status_for_transfer(
      mds::TwoPhaseCommitState::ON_COMMIT, data))) {
    MDS_LOG(WARN, "fail to get tablet status for transfer", K(ret), K(data));
  }
  return ret;
  #undef PRINT_WRAPPER
}

int ObITabletMdsInterface::get_latest_tablet_status(
    ObTabletCreateDeleteMdsUserData &data,
    mds::MdsWriter &writer,
    mds::TwoPhaseCommitState &trans_stat,
    share::SCN &trans_version,
    const int64_t read_seq) const
{
  #define PRINT_WRAPPER KR(ret), K(data)
  MDS_TG(10_ms);
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!check_is_inited_())) {
    ret = OB_NOT_INIT;
    MDS_LOG_GET(WARN, "not inited");
  } else if (CLICK_FAIL((get_latest<ObTabletCreateDeleteMdsUserData>(
      ReadTabletStatusOp(data),
      writer,
      trans_stat,
      trans_version,
      read_seq)))) {
    if (OB_EMPTY_RESULT != ret) {
      MDS_LOG(WARN, "fail to get latest", K(ret));
    }
  } else if (GCTX.is_shared_storage_mode() && OB_FAIL(get_tablet_status_for_transfer(trans_stat, data))) {
    MDS_LOG(WARN, "fail to get tablet status for transfer", K(ret), K(data));
  }
  return ret;
  #undef PRINT_WRAPPER
}

int ObITabletMdsInterface::get_latest_ddl_data(
    ObTabletBindingMdsUserData &data,
    mds::MdsWriter &writer,
    mds::TwoPhaseCommitState &trans_stat,
    share::SCN &trans_version,
    const int64_t read_seq) const
{
  #define PRINT_WRAPPER KR(ret), K(data)
  MDS_TG(10_ms);
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!check_is_inited_())) {
    ret = OB_NOT_INIT;
    MDS_LOG_GET(WARN, "not inited");
  } else {
    ObITabletMdsInterface *src = nullptr;
    ObTabletHandle src_tablet_handle;
    if (get_tablet_meta_().has_transfer_table()) {
      if (CLICK_FAIL(get_src_tablet_handle_and_base_ptr_(src_tablet_handle, src))) {
        MDS_LOG(WARN, "fail to get src tablet handle", K(ret), K(get_tablet_meta_()));
      }
    }

    if (OB_FAIL(ret)) {
    } else if (CLICK_FAIL((cross_ls_get_latest<ObTabletBindingMdsUserData>(
        src,
        ReadBindingInfoOp(data),
        writer,
        trans_stat,
        trans_version,
        read_seq)))) {
      if (OB_EMPTY_RESULT != ret) {
        MDS_LOG_GET(WARN, "fail to cross ls get latest", K(lbt()));
      }
    } else if (!data.is_valid()) {
      ret = OB_ERR_UNEXPECTED;
      MDS_LOG_GET(WARN, "invalid user data", K(lbt()));
    }
  }
  return ret;
  #undef PRINT_WRAPPER
}

int ObITabletMdsInterface::get_ddl_data(
    const share::SCN &snapshot,
    ObTabletBindingMdsUserData &data,
    const int64_t timeout) const
{
  #define PRINT_WRAPPER KR(ret), K(data), K(snapshot), K(timeout)
  MDS_TG(10_ms);
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!check_is_inited_())) {
    ret = OB_NOT_INIT;
    MDS_LOG_GET(WARN, "not inited");
  } else if (OB_UNLIKELY(!snapshot.is_max())) {
    ret = OB_NOT_SUPPORTED;
    MDS_LOG_GET(WARN, "only support read latest data currently");
  } else {
    ObITabletMdsInterface *src = nullptr;
    ObTabletHandle src_tablet_handle;
    if (get_tablet_meta_().has_transfer_table()) {
      if (CLICK_FAIL(get_src_tablet_handle_and_base_ptr_(src_tablet_handle, src))) {
        MDS_LOG(WARN, "fail to get src tablet handle", K(ret), K(get_tablet_meta_()));
      }
    }

    if (OB_FAIL(ret)) {
    } else if (CLICK_FAIL((cross_ls_get_snapshot<mds::DummyKey, ObTabletBindingMdsUserData>(src, mds::DummyKey(),
        ReadBindingInfoOp(data), snapshot, timeout)))) {
      if (OB_EMPTY_RESULT != ret) {
        MDS_LOG_GET(WARN, "fail to cross ls get snapshot", K(lbt()));
      } else {
        data.set_default_value(); // use default value
        ret = OB_SUCCESS;
      }
    }
  }
  return ret;
  #undef PRINT_WRAPPER
}

int ObITabletMdsInterface::get_autoinc_seq(
    ObIAllocator &allocator,
    const share::SCN &snapshot,
    share::ObTabletAutoincSeq &data,
    const int64_t timeout) const
{
  #define PRINT_WRAPPER KR(ret), K(data), K(snapshot), K(timeout)
  MDS_TG(10_ms);
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!check_is_inited_())) {
    ret = OB_NOT_INIT;
    MDS_LOG_GET(WARN, "not inited");
  } else if (OB_UNLIKELY(!snapshot.is_max())) {
    ret = OB_NOT_SUPPORTED;
    MDS_LOG_GET(WARN, "only support read latest data currently");
  } else {
    ObITabletMdsInterface *src = nullptr;
    ObTabletHandle src_tablet_handle;
    if (get_tablet_meta_().has_transfer_table()) {
      if (CLICK_FAIL(get_src_tablet_handle_and_base_ptr_(src_tablet_handle, src))) {
        MDS_LOG(WARN, "fail to get src tablet handle", K(ret), K(get_tablet_meta_()));
      }
    }

    if (OB_FAIL(ret)) {
    } else if (CLICK_FAIL((cross_ls_get_snapshot<mds::DummyKey, share::ObTabletAutoincSeq>(src, mds::DummyKey(),
        ReadAutoIncSeqOp(allocator, data), snapshot, timeout)))) {
      if (OB_EMPTY_RESULT != ret) {
        MDS_LOG_GET(WARN, "fail to cross ls get snapshot", K(lbt()));
      } else {
        data.reset(); // use default value
        ret = OB_SUCCESS;
      }
    }
  }

  return ret;
  #undef PRINT_WRAPPER
}

int ObITabletMdsInterface::get_split_data(
    ObTabletSplitMdsUserData &data,
    const int64_t timeout) const
{
  #define PRINT_WRAPPER KR(ret), K(data), K(timeout)
  MDS_TG(10_ms);
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!check_is_inited_())) {
    ret = OB_NOT_INIT;
    MDS_LOG_GET(WARN, "not inited");
  } else {
    // TODO(lihongqin.lhq): use get_latest_committed and block during 2pc
    const share::SCN snapshot = share::SCN::max_scn();
    ObITabletMdsInterface *src = nullptr;
    ObTabletHandle src_tablet_handle;
    if (get_tablet_meta_().has_transfer_table()) {
      if (CLICK_FAIL(get_src_tablet_handle_and_base_ptr_(src_tablet_handle, src))) {
        MDS_LOG(WARN, "fail to get src tablet handle", K(ret), K(get_tablet_meta_()));
      }
    }

    if (OB_FAIL(ret)) {
    } else if (CLICK_FAIL((cross_ls_get_snapshot<mds::DummyKey, ObTabletSplitMdsUserData>(src, mds::DummyKey(),
        ReadSplitDataOp(data), snapshot, timeout)))) {
      if (OB_EMPTY_RESULT != ret) {
        MDS_LOG_GET(WARN, "fail to cross ls get snapshot", K(lbt()));
      } else {
        data.reset(); // use default value
        ret = OB_SUCCESS;
      }
    }
  }

  return ret;
  #undef PRINT_WRAPPER
}

int ObITabletMdsInterface::split_partkey_compare(const blocksstable::ObDatumRowkey &rowkey,
                                                 const ObITableReadInfo &rowkey_read_info,
                                                 const ObIArray<uint64_t> &partkey_projector,
                                                 int &cmp_ret,
                                                 const int64_t timeout) const
{
  #define PRINT_WRAPPER KR(ret), K(rowkey), K(rowkey_read_info)
  MDS_TG(10_ms);
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!check_is_inited_())) {
    ret = OB_NOT_INIT;
    MDS_LOG_GET(WARN, "not inited");
  } else {
    // TODO(lihongqin.lhq): use get_latest_committed and block during 2pc
    const share::SCN snapshot = share::SCN::max_scn();
    ObITabletMdsInterface *src = nullptr;
    ObTabletHandle src_tablet_handle;
    if (get_tablet_meta_().has_transfer_table()) {
      if (CLICK_FAIL(get_src_tablet_handle_and_base_ptr_(src_tablet_handle, src))) {
        MDS_LOG(WARN, "fail to get src tablet handle", K(ret), K(get_tablet_meta_()));
      }
    }

    if (OB_FAIL(ret)) {
    } else if (CLICK_FAIL((cross_ls_get_snapshot<mds::DummyKey, ObTabletSplitMdsUserData>(src, mds::DummyKey(),
        ReadSplitDataPartkeyCompareOp(rowkey, rowkey_read_info, partkey_projector, cmp_ret), snapshot, timeout)))) {
      if (OB_EMPTY_RESULT != ret) {
        MDS_LOG_GET(WARN, "fail to cross ls get snapshot", K(ret), K(lbt()));
      }
    }
  }

  return ret;
  #undef PRINT_WRAPPER
}

int ObITabletMdsInterface::read_raw_data(
    common::ObIAllocator &allocator,
    const uint8_t mds_unit_id,
    const common::ObString &udf_key,
    const share::SCN &snapshot,
    const int64_t timeout_us,
    mds::MdsDumpKV &kv) const
{
  int ret = OB_SUCCESS;
  const share::ObLSID &ls_id = get_tablet_meta_().ls_id_;
  const common::ObTabletID &tablet_id = get_tablet_meta_().tablet_id_;
  const int64_t abs_timeout = timeout_us + ObClockGenerator::getClock();
  ObMdsReadInfoCollector placeholder_collector;
  SMART_VARS_3((ObTableScanParam, scan_param), (ObStoreCtx, store_ctx), (ObMdsRowIterator, iter)) {
    if (OB_FAIL(ObMdsScanParamHelper::build_scan_param(
        allocator,
        ls_id,
        tablet_id,
        ObMdsSchemaHelper::MDS_TABLE_ID,
        mds_unit_id,
        udf_key,
        true/*is_get*/,
        abs_timeout,
        ObVersionRange(0/*base_version*/, snapshot.get_val_for_tx()/*snapshot_version*/),
        placeholder_collector,
        scan_param))) {
      MDS_LOG(WARN, "fail to build scan param", K(ret));
    } else if (OB_FAIL(mds_table_scan(scan_param, store_ctx, iter))) {
      MDS_LOG(WARN, "fail to do mds table scan", K(ret), K(snapshot), K(scan_param));
    } else {
      int tmp_ret = OB_SUCCESS;
      if (OB_FAIL(iter.get_next_mds_kv(allocator, kv))) {
        if (OB_ITER_END != ret) {
          MDS_LOG(WARN, "fail to get next row", K(ret));
        }
      } else if (OB_UNLIKELY(OB_ITER_END != (tmp_ret = iter.get_next_row()))) {
        ret = OB_ERR_UNEXPECTED;
        MDS_LOG(WARN, "iter should reach the end", K(ret), K(tmp_ret), K(iter));
      }
    }

    if (OB_FAIL(ret)) {
      kv.reset();
    }
  }

  return ret;
}

int ObITabletMdsInterface::mds_table_scan(
    ObTableScanParam &scan_param,
    ObStoreCtx &store_ctx,
    ObMdsRowIterator &iter) const
{
  int ret = OB_SUCCESS;
  ObTabletHandle tablet_handle;

  if (OB_FAIL(get_tablet_handle_from_this(tablet_handle))) {
    MDS_LOG(WARN, "fail to build tablet handle", K(ret));
  } else if (OB_FAIL(iter.init(scan_param, tablet_handle, store_ctx))) {
    MDS_LOG(WARN, "fail to init mds row iter", K(ret), KPC(tablet_handle.get_obj()), K(scan_param));
  }

  return ret;
}

int ObITabletMdsInterface::get_tablet_handle_from_this(
  ObTabletHandle &tablet_handle) const
{
  int ret = OB_SUCCESS;
  const ObTablet *tablet = static_cast<const ObTablet*>(this);
  const share::ObLSID &ls_id = get_tablet_meta_().ls_id_;
  const common::ObTabletID &tablet_id = get_tablet_meta_().tablet_id_;
  ObTenantMetaMemMgr *t3m = MTL(ObTenantMetaMemMgr*);
  if (OB_FAIL(t3m->build_tablet_handle_for_mds_scan(const_cast<ObTablet*>(tablet), tablet_handle))) {
    MDS_LOG(WARN, "fail to build tablet handle", K(ret), K(ls_id), K(tablet_id));
  }
  return ret;
}

int ObITabletMdsInterface::get_tablet_status_for_transfer(
    const mds::TwoPhaseCommitState &trans_stat,
    ObTabletCreateDeleteMdsUserData &tablet_status) const
{
  int ret = OB_SUCCESS;
  bool is_transfer_out_deleted = get_tablet_meta_().transfer_info_.is_transfer_out_deleted();

  // when is_transfer_out_deleted is true, tablet_status can be not only TRANSFER_OUT
  // that is because when restart, is_transfer_out_deleted in tablet meta is true,
  // but the tablet status in mds may not be replayed to TRANSFER_OUT,
  // therefore, when tablet status is not TRANSFER_OUT, we need to skip the update
  if (!is_transfer_out_deleted || ObTabletStatus::TRANSFER_OUT != tablet_status.tablet_status_) {
    // do nothing
  } else if (trans_stat != mds::TwoPhaseCommitState::ON_COMMIT) {
    // do nothing
  } else {
    tablet_status.tablet_status_ = ObTabletStatus::TRANSFER_OUT_DELETED;
    tablet_status.delete_commit_scn_ = tablet_status.start_transfer_commit_scn_;
    tablet_status.delete_commit_version_ = tablet_status.start_transfer_commit_version_;
  }
  return ret;
}

} // namespace storage
} // namespace oceanbase
