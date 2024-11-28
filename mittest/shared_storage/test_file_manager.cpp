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
#define USING_LOG_PREFIX STORAGETEST

#include <gtest/gtest.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/types.h>
#include <gmock/gmock.h>
#define protected public
#define private public
#include "mittest/mtlenv/mock_tenant_module_env.h"
#include "share/allocator/ob_tenant_mutil_allocator_mgr.h"
#include "storage/shared_storage/ob_disk_space_manager.h"
#include "storage/shared_storage/ob_dir_manager.h"
#include "storage/shared_storage/ob_file_manager.h"
#include "storage/shared_storage/ob_ss_format_util.h"
#include "storage/blocksstable/ob_macro_block_id.h"
#include "mittest/shared_storage/clean_residual_data.h"
#include "storage/shared_storage/ob_file_op.h"
#include "storage/tmp_file/ob_tmp_file_manager.h"

#undef private
#undef protected

namespace oceanbase
{
namespace storage
{
using namespace oceanbase::blocksstable;
using namespace oceanbase::storage;

class TestFileDelOp : public share::ObScanDirOp
{
public:
  TestFileDelOp(const char *dir = nullptr) {}
  virtual int func(const dirent *entry) override
  {
    int ret = OB_SUCCESS;
    if (OB_ISNULL(entry)) {
      ret = OB_INVALID_ARGUMENT;
      LOG_WARN("invalid entry", KR(ret));
    } else {
      char full_path[common::MAX_PATH_SIZE] = { 0 };
      int p_ret = snprintf(full_path, sizeof(full_path), "%s/%s", dir_, entry->d_name);
      if (p_ret <= 0 || p_ret >= sizeof(full_path)) {
        ret = OB_BUF_NOT_ENOUGH;
        LOG_WARN("file name too long", KR(ret), K_(dir), K(entry->d_name));
      } else if (OB_FAIL(ObIODeviceLocalFileOp::unlink(full_path))) {
        if (OB_NO_SUCH_FILE_OR_DIRECTORY == ret) {
          ret = OB_SUCCESS;
        } else {
          LOG_WARN("fail to del file", KR(ret), K(full_path));
        }
      }
    }
    return ret;
  }
};

class TestDirDelOp : public share::ObScanDirOp
{
public:
  TestDirDelOp(const char *dir = nullptr) {}
  virtual int func(const dirent *entry) override
  {
    int ret = OB_SUCCESS;
    if (OB_ISNULL(entry)) {
      ret = OB_INVALID_ARGUMENT;
      LOG_WARN("invalid entry", KR(ret));
    } else {
      char full_path[common::MAX_PATH_SIZE] = { 0 };
      int p_ret = snprintf(full_path, sizeof(full_path), "%s/%s", dir_, entry->d_name);
      if (p_ret <= 0 || p_ret >= sizeof(full_path)) {
        ret = OB_BUF_NOT_ENOUGH;
        LOG_WARN("file name too long", KR(ret), K_(dir), K(entry->d_name));
      } else if (OB_FAIL(ObIODeviceLocalFileOp::rmdir(full_path))) {
        if (OB_NO_SUCH_FILE_OR_DIRECTORY == ret) {
          ret = OB_SUCCESS;
        } else {
          LOG_WARN("fail to del dir", KR(ret), K(full_path));
        }
      }
    }
    return ret;
  }
};

class TestFileManager : public ::testing::Test
{
public:
  TestFileManager() = default;
  virtual ~TestFileManager() = default;
  static void SetUpTestCase();
  static void TearDownTestCase();
};

void TestFileManager::SetUpTestCase()
{
  GCTX.startup_mode_ = observer::ObServerMode::SHARED_STORAGE_MODE;
  EXPECT_EQ(OB_SUCCESS, MockTenantModuleEnv::get_instance().init());
  MTL(tmp_file::ObTenantTmpFileManager *)->stop();
  MTL(tmp_file::ObTenantTmpFileManager *)->wait();
  MTL(tmp_file::ObTenantTmpFileManager *)->destroy();
}

void TestFileManager::TearDownTestCase()
{
  int ret = OB_SUCCESS;
  if (OB_FAIL(ResidualDataCleanerHelper::clean_in_mock_env())) {
      LOG_WARN("failed to clean residual data", KR(ret));
  }
  MockTenantModuleEnv::get_instance().destroy();
}

TEST_F(TestFileManager, test_path_convert)
{
  int ret = OB_SUCCESS;
  ObTenantFileManager* tenant_file_mgr = MTL(ObTenantFileManager*);
  ASSERT_NE(nullptr, tenant_file_mgr);
  MacroBlockId file_id;
  char file_path[common::MAX_PATH_SIZE] = {0};
  char expected_path[common::MAX_PATH_SIZE] = {0};
  bool is_local_cache = true;
  int64_t ls_epoch_id = 4;
  file_id.set_id_mode(static_cast<uint64_t>(ObMacroBlockIdMode::ID_MODE_SHARE));
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::PRIVATE_DATA_MACRO));
  file_id.set_second_id(3);
  file_id.set_third_id(2);
  file_id.set_macro_transfer_seq(0);
  file_id.set_tenant_seq(5);
  char *object_storage_root_dir = nullptr;
  ASSERT_EQ(OB_SUCCESS, OB_DIR_MGR.get_object_storage_root_dir(object_storage_root_dir));

  // 1.PRIVATE_DATA_MACRO
  // tenant_id_epoch_id/tablet_data/tablet_id/transfer_seq/data/macro_server_id_seq_id
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/1_0/%s/3/0/%s/2_5",
                                        OB_DIR_MGR.get_local_cache_root_dir(), TABLET_DATA_DIR_STR, DATA_MACRO_DIR_STR));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  is_local_cache = false;
  // cluster_id/server_id/tenant_id_epoch_id/tablet_data/tablet_id/transfer_seq/data/macro_server_id_seq_id
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/%s_1/%s_2/1_0/%s/3/0/%s/2_5",
                                        object_storage_root_dir, CLUSTER_DIR_STR, SERVER_DIR_STR,
                                        TABLET_DATA_DIR_STR, DATA_MACRO_DIR_STR));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));


  // 2.SHARED_MAJOR_DATA_MACRO
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::SHARED_MAJOR_DATA_MACRO));
  // tenant_id_epoch_id/shared_major_macro_cache/tablet_id_cg_id_macro_seq_id_data
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/1_0/%s/3_0_2_%s",
                                        OB_DIR_MGR.get_local_cache_root_dir(), MAJOR_DATA_DIR_STR, DATA_MACRO_DIR_STR));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));
  // cluster_id/tenant_id/tablet/tablet_id/major/cg_id/data/macro_seq_id
  is_local_cache = false;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/%s_1/%s_1/%s/3/%s/%s/%s_0/%s/2",
                                        object_storage_root_dir, CLUSTER_DIR_STR, TENANT_DIR_STR,
                                        TABLET_DIR_STR, MAJOR_DIR_STR, SHARED_TABLET_SSTABLE_DIR_STR,
                                        COLUMN_GROUP_STR, DATA_MACRO_DIR_STR));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  // 3.TMP_FILE
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::TMP_FILE));
  // tenant_id_epoch_id/tmp_data/tmp_file_id/segment_id
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/1_0/%s/3/2",
                                        OB_DIR_MGR.get_local_cache_root_dir(), TMP_DATA_DIR_STR));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  file_path[0] = '\0';
  expected_path[0] = '\0';
  // tenant_id_epoch_id/tmp_data/tmp_file_id/segment_id.deleted
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id, 0/*ls_epoch_id*/, false/*is_atomic_tmp*/, OB_INVALID_ID/*seq*/, true/*is_deleted_file*/));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/1_0/%s/3/2.deleted",
                                        OB_DIR_MGR.get_local_cache_root_dir(), TMP_DATA_DIR_STR));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  is_local_cache = false;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  // cluster_id/server_id/tenant_id_epoch_id/tmp_data/tmp_file_id/segment_id
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/%s_1/%s_1/1_0/%s/3/2",
                                        object_storage_root_dir, CLUSTER_DIR_STR, SERVER_DIR_STR, TMP_DATA_DIR_STR));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  // 4.SERVER_META
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::SERVER_META));
  // super_block
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/%s",
                                        OB_DIR_MGR.get_local_cache_root_dir(), get_storage_objet_type_str(ObStorageObjectType::SERVER_META)));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  is_local_cache = false;
  file_path[0] = '\0';
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));

  // 5.TENANT_SUPER_BLOCK
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::TENANT_SUPER_BLOCK));
  // tenant_id_epoch_id/tenant_super_block
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/3_2/%s",
                                        OB_DIR_MGR.get_local_cache_root_dir(), get_storage_objet_type_str(ObStorageObjectType::TENANT_SUPER_BLOCK)));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  is_local_cache = false;
  file_path[0] = '\0';
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));

  // 6.TENANT_UNIT_META
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::TENANT_UNIT_META));
  // tenant_id_epoch_id/unit_meta
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/3_2/%s",
                                        OB_DIR_MGR.get_local_cache_root_dir(), get_storage_objet_type_str(ObStorageObjectType::TENANT_UNIT_META)));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  is_local_cache = false;
  file_path[0] = '\0';
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));

  // 7.LS_META
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::LS_META));
  // tenant_id_epoch_id/ls/ls_id_epoch_id/ls_meta
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id, ls_epoch_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/1_0/%s/3_4/%s",
                                        OB_DIR_MGR.get_local_cache_root_dir(), LS_DIR_STR,
                                        get_storage_objet_type_str(ObStorageObjectType::LS_META)));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  is_local_cache = false;
  file_path[0] = '\0';
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));

  // 8.LS_DUP_TABLE_META
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::LS_DUP_TABLE_META));
  // tenant_id_epoch_id/ls/ls_id_epoch_id/ls_dup_table_meta
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id, ls_epoch_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/1_0/%s/3_4/%s",
                                        OB_DIR_MGR.get_local_cache_root_dir(), LS_DIR_STR,
                                        get_storage_objet_type_str(ObStorageObjectType::LS_DUP_TABLE_META)));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  is_local_cache = false;
  file_path[0] = '\0';
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id, ls_epoch_id));

  // 9.LS_ACTIVE_TABLET_ARRAY
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::LS_ACTIVE_TABLET_ARRAY));
  // tenant_id_epoch_id/ls/ls_id_epoch_id/tablet_id_array
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id, ls_epoch_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/1_0/%s/3_4/%s",
                                        OB_DIR_MGR.get_local_cache_root_dir(), LS_DIR_STR,
                                        get_storage_objet_type_str(ObStorageObjectType::LS_ACTIVE_TABLET_ARRAY)));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  is_local_cache = false;
  file_path[0] = '\0';
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));

  // 10.LS_PENDING_FREE_TABLET_ARRAY
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::LS_PENDING_FREE_TABLET_ARRAY));
  // tenant_id_epoch_id/ls/ls_id_epoch_id/pending_free_tablet_array
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id, ls_epoch_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/1_0/%s/3_4/%s",
                                        OB_DIR_MGR.get_local_cache_root_dir(), LS_DIR_STR,
                                        get_storage_objet_type_str(ObStorageObjectType::LS_PENDING_FREE_TABLET_ARRAY)));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  is_local_cache = false;
  file_path[0] = '\0';
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));

  // 11.LS_TRANSFER_TABLET_ID_ARRAY
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::LS_TRANSFER_TABLET_ID_ARRAY));
  // tenant_id_epoch_id/ls/ls_id_epoch_id/transfer_tablet_id_array
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id, ls_epoch_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/1_0/%s/3_4/%s",
                                        OB_DIR_MGR.get_local_cache_root_dir(), LS_DIR_STR,
                                        get_storage_objet_type_str(ObStorageObjectType::LS_TRANSFER_TABLET_ID_ARRAY)));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  is_local_cache = false;
  file_path[0] = '\0';
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));

  // 12.PRIVATE_TABLET_META
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::PRIVATE_TABLET_META));
  // tenant_id_epoch_id/ls/ls_id_epoch_id/tablet_meta/tablet_id/tablet_meta_version_transfer_seq
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id, ls_epoch_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/1_0/%s/3_4/%s/2/5_0",
                                        OB_DIR_MGR.get_local_cache_root_dir(), LS_DIR_STR, TABLET_META_DIR_STR));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  is_local_cache = false;
  file_path[0] = '\0';
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));

  // 13.PRIVATE_TABLET_CURRENT_VERSION
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::PRIVATE_TABLET_CURRENT_VERSION));
  // tenant_id_epoch_id/ls/ls_id_epoch_id/tablet_meta/tablet_id/current_version
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id, ls_epoch_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/1_0/%s/3_4/%s/2/%s",
                                        OB_DIR_MGR.get_local_cache_root_dir(), LS_DIR_STR, TABLET_META_DIR_STR,
                                        get_storage_objet_type_str(ObStorageObjectType::PRIVATE_TABLET_CURRENT_VERSION)));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  is_local_cache = false;
  file_path[0] = '\0';
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));

  // 14.SHARED_MAJOR_TABLET_META
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::SHARED_MAJOR_TABLET_META));
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  // cluster_id/tenant_id/tablet/tablet_id/major/meta/tablet_meta_version
  is_local_cache = false;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/%s_1/%s_1/%s/3/%s/%s/2",
                                        object_storage_root_dir, CLUSTER_DIR_STR, TENANT_DIR_STR,
                                        TABLET_DIR_STR, MAJOR_DIR_STR, SHARED_TABLET_META_DIR_STR));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));


  // 15.COMPACTION_SERVER
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::COMPACTION_SERVER));
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  // cluster_id/tenant_id/compaction/scheduler/ls_id_compaction_tasks
  is_local_cache = false;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/%s_1/%s_1/%s/%s/3_%s",
                                        object_storage_root_dir, CLUSTER_DIR_STR,
                                        TENANT_DIR_STR, COMPACTION_DIR_STR, SCHEDULER_DIR_STR,
                                        get_storage_objet_type_str(ObStorageObjectType::COMPACTION_SERVER)));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));


  // 16.LS_SVR_COMPACTION_STATUS
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::LS_SVR_COMPACTION_STATUS));
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  // cluster_id/tenant_id/compaction/compactor/ls_id_server_id_ls_svr_compaction_status
  is_local_cache = false;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/%s_1/%s_1/%s/%s/3_2_%s",
                                        object_storage_root_dir, CLUSTER_DIR_STR,
                                        TENANT_DIR_STR, COMPACTION_DIR_STR, COMPACTOR_DIR_STR,
                                        get_storage_objet_type_str(ObStorageObjectType::LS_SVR_COMPACTION_STATUS)));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  // 17.COMPACTION_REPORT
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::COMPACTION_REPORT));
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  // cluster_id/tenant_id/compaction/compactor/server_id_compaction_report
  is_local_cache = false;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/%s_1/%s_1/%s/%s/3_%s",
                                        object_storage_root_dir, CLUSTER_DIR_STR,
                                        TENANT_DIR_STR, COMPACTION_DIR_STR, COMPACTOR_DIR_STR,
                                        get_storage_objet_type_str(ObStorageObjectType::COMPACTION_REPORT)));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  // 18.PRIVATE_META_MACRO
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::PRIVATE_META_MACRO));
  // tenant_id_epoch_id/tablet_data/tablet_id/transfer_seq/meta/macro_server_id_seq_id
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/1_0/%s/3/0/%s/2_5",
                                        OB_DIR_MGR.get_local_cache_root_dir(), TABLET_DATA_DIR_STR, META_MACRO_DIR_STR));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  is_local_cache = false;
  // cluster_id/server_id/tenant_id_epoch_id/tablet_data/tablet_id/transfer_seq/meta/macro_server_id_seq_id
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/%s_1/%s_2/1_0/%s/3/0/%s/2_5",
                                        object_storage_root_dir, CLUSTER_DIR_STR, SERVER_DIR_STR,
                                        TABLET_DATA_DIR_STR, META_MACRO_DIR_STR));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  // 19.SHARED_MAJOR_META_MACRO
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::SHARED_MAJOR_META_MACRO));
  // tenant_id_epoch_id/shared_major_macro_cache/tablet_id_cg_id_macro_seq_id_meta
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/1_0/%s/3_0_2_%s",
                                        OB_DIR_MGR.get_local_cache_root_dir(), MAJOR_DATA_DIR_STR, META_MACRO_DIR_STR));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));
  // cluster_id/tenant_id/tablet/tablet_id/major/cg_id/meta/macro_seq_id
  is_local_cache = false;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/%s_1/%s_1/%s/3/%s/%s/%s_0/%s/2",
                                        object_storage_root_dir, CLUSTER_DIR_STR, TENANT_DIR_STR, TABLET_DIR_STR,
                                        MAJOR_DIR_STR, SHARED_TABLET_SSTABLE_DIR_STR, COLUMN_GROUP_STR, META_MACRO_DIR_STR));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  // 20.SHARED_MAJOR_GC_INFO
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::SHARED_MAJOR_GC_INFO));
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  // cluster_id/tenant_id/tablet/tablet_id/major/meta/gc_info
  is_local_cache = false;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/%s_1/%s_1/%s/3/%s/%s/%s",
                                        object_storage_root_dir, CLUSTER_DIR_STR, TENANT_DIR_STR,
                                        TABLET_DIR_STR, MAJOR_DIR_STR, SHARED_TABLET_META_DIR_STR,
                                        get_storage_objet_type_str(ObStorageObjectType::SHARED_MAJOR_GC_INFO)));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  // 21.SHARED_MAJOR_META_LIST
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::SHARED_MAJOR_META_LIST));
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  // cluster_id/tenant_id/tablet/tablet_id/major/meta/meta_list
  is_local_cache = false;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/%s_1/%s_1/%s/3/%s/%s/%s",
                                        object_storage_root_dir, CLUSTER_DIR_STR, TENANT_DIR_STR,
                                        TABLET_DIR_STR, MAJOR_DIR_STR, SHARED_TABLET_META_DIR_STR,
                                        get_storage_objet_type_str(ObStorageObjectType::SHARED_MAJOR_META_LIST)));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  // 22.LS_COMPACTION_STATUS
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::LS_COMPACTION_STATUS));
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  // cluster_id/tenant_id/compaction/scheduler/ls_id_ls_compaction_status
  is_local_cache = false;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/%s_1/%s_1/%s/%s/3_%s",
                                        object_storage_root_dir, CLUSTER_DIR_STR,
                                        TENANT_DIR_STR, COMPACTION_DIR_STR, SCHEDULER_DIR_STR,
                                        get_storage_objet_type_str(ObStorageObjectType::LS_COMPACTION_STATUS)));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  // 23.TABLET_COMPACTION_STATUS
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::TABLET_COMPACTION_STATUS));
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  // cluster_id/tenant_id/tablet/tablet_id/major/scn_id_tablet_compaction_status
  is_local_cache = false;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/%s_1/%s_1/%s/3/%s/2_%s",
                                        object_storage_root_dir, CLUSTER_DIR_STR,
                                        TENANT_DIR_STR, TABLET_DIR_STR, MAJOR_DIR_STR,
                                        get_storage_objet_type_str(ObStorageObjectType::TABLET_COMPACTION_STATUS)));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  // 24.TENANT_DISK_SPACE_META
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::TENANT_DISK_SPACE_META));
  // tenant_id_epoch_id/tenant_disk_space_meta
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/3_2/%s",
                                        OB_DIR_MGR.get_local_cache_root_dir(), get_storage_objet_type_str(ObStorageObjectType::TENANT_DISK_SPACE_META)));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  is_local_cache = false;
  file_path[0] = '\0';
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));

  // 25.SHARED_TABLET_ID
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::SHARED_TABLET_ID));
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  // cluster_id/tenant_id/tablet_ids/tablet_id
  is_local_cache = false;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/%s_1/%s_1/%s/3",
                                        object_storage_root_dir, CLUSTER_DIR_STR,
                                        TENANT_DIR_STR, TABLET_IDS_DIR_STR));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  // 26.LS_COMPACTION_LIST
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::LS_COMPACTION_LIST));
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  // cluster_id/tenant_id/compaction/scheduler/ls_id_ls_compaction_status
  is_local_cache = false;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/%s_1/%s_1/%s/%s/3_%s",
                                        object_storage_root_dir, CLUSTER_DIR_STR,
                                        TENANT_DIR_STR, COMPACTION_DIR_STR, SCHEDULER_DIR_STR,
                                        get_storage_objet_type_str(ObStorageObjectType::LS_COMPACTION_LIST)));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  // 27. IS_DELETED
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::IS_SHARED_TABLET_DELETED));
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  // cluster_id/tenant_id/tablet_ids/tablet_id/is_deleted
  is_local_cache = false;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/%s_1/%s_1/%s/3/%s",
                                        object_storage_root_dir, CLUSTER_DIR_STR,
                                        TENANT_DIR_STR, TABLET_DIR_STR,
                                        get_storage_objet_type_str(ObStorageObjectType::IS_SHARED_TABLET_DELETED)));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  // 28. IS_SHARED_TENANT_DELETED
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::IS_SHARED_TENANT_DELETED));
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  // cluster_id/tenant_id/is_shared_tenant_deleted
  is_local_cache = false;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/%s_1/%s_3/%s",
                                        object_storage_root_dir, CLUSTER_DIR_STR,
                                        TENANT_DIR_STR, get_storage_objet_type_str(ObStorageObjectType::IS_SHARED_TENANT_DELETED)));

  // 29. MAJOR_PREWARM_DATA
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::MAJOR_PREWARM_DATA));
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  // cluster_id/tenant_id/tablet/tablet_id/major/scn_id_compaction_scn_prewarm_data
  is_local_cache = false;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/%s_1/%s_1/%s/3/%s/2_%s",
                                        object_storage_root_dir, CLUSTER_DIR_STR,
                                        TENANT_DIR_STR, TABLET_DIR_STR, MAJOR_DIR_STR,
                                        get_storage_objet_type_str(ObStorageObjectType::MAJOR_PREWARM_DATA)));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  // 30. MAJOR_PREWARM_DATA_INDEX
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::MAJOR_PREWARM_DATA_INDEX));
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  // cluster_id/tenant_id/tablet/tablet_id/major/scn_id_compaction_scn_prewarm_data
  is_local_cache = false;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/%s_1/%s_1/%s/3/%s/2_%s",
                                        object_storage_root_dir, CLUSTER_DIR_STR,
                                        TENANT_DIR_STR, TABLET_DIR_STR, MAJOR_DIR_STR,
                                        get_storage_objet_type_str(ObStorageObjectType::MAJOR_PREWARM_DATA_INDEX)));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  // 31. MAJOR_PREWARM_META
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::MAJOR_PREWARM_META));
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  // cluster_id/tenant_id/tablet/tablet_id/major/scn_id_compaction_scn_prewarm_data
  is_local_cache = false;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/%s_1/%s_1/%s/3/%s/2_%s",
                                        object_storage_root_dir, CLUSTER_DIR_STR,
                                        TENANT_DIR_STR, TABLET_DIR_STR, MAJOR_DIR_STR,
                                        get_storage_objet_type_str(ObStorageObjectType::MAJOR_PREWARM_META)));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  // 32. MAJOR_PREWARM_META_INDEX
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::MAJOR_PREWARM_META_INDEX));
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  // cluster_id/tenant_id/tablet/tablet_id/major/scn_id_compaction_scn_prewarm_data
  is_local_cache = false;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/%s_1/%s_1/%s/3/%s/2_%s",
                                        object_storage_root_dir, CLUSTER_DIR_STR,
                                        TENANT_DIR_STR, TABLET_DIR_STR, MAJOR_DIR_STR,
                                        get_storage_objet_type_str(ObStorageObjectType::MAJOR_PREWARM_META_INDEX)));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  // 36. CHECKSUM_ERROR_DUMP_MACRO
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::CHECKSUM_ERROR_DUMP_MACRO));
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  // cluster_id/tenant_id/tablet/tablet_id/major/sstable/cg_id/checksum_error_macro/svr_id_compaction_scn_block_id
  is_local_cache = false;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_fourth_id(5); // different from trans_seq + macro_seq
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/%s_1/%s_1/%s/3/%s/%s/%s_0/%s/%ld_2_5",
                                        object_storage_root_dir, CLUSTER_DIR_STR,
                                        TENANT_DIR_STR, TABLET_DIR_STR, MAJOR_DIR_STR, SHARED_TABLET_SSTABLE_DIR_STR, COLUMN_GROUP_STR,
                                        CKM_ERROR_DIR_STR, GCTX.get_server_id()));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));

  // 37. UNSEALED_REMOTE_SEG_FILE
  is_local_cache = true;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::UNSEALED_REMOTE_SEG_FILE));
  ASSERT_EQ(OB_NOT_SUPPORTED, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id));
  is_local_cache = false;
  file_path[0] = '\0';
  expected_path[0] = '\0';
  // cluster_id/server_id/tenant_id_epoch_id/tmp_data/tmp_file_id/segment_id_valid_length
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(is_local_cache, file_path, common::MAX_PATH_SIZE, file_id,
            0/*ls_epoch_id*/, false/*is_atomic_tmp*/, OB_INVALID_ID/*seq*/));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_path, common::MAX_PATH_SIZE, "%s/%s_1/%s_1/1_0/%s/3/2_5",
                                        object_storage_root_dir, CLUSTER_DIR_STR, SERVER_DIR_STR, TMP_DATA_DIR_STR));
  ASSERT_EQ(0, STRCMP(file_path, expected_path));
}

TEST_F(TestFileManager, test_get_file_parent_dir)
{
  int ret = OB_SUCCESS;
  ObTenantFileManager* tenant_file_mgr = MTL(ObTenantFileManager*);
  ASSERT_NE(nullptr, tenant_file_mgr);
  MacroBlockId file_id;
  char dir_path[common::MAX_PATH_SIZE] = {0};
  char expected_dir_path[common::MAX_PATH_SIZE] = {0};
  int64_t ls_epoch_id = 4;
  file_id.set_id_mode(static_cast<uint64_t>(ObMacroBlockIdMode::ID_MODE_SHARE));
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::PRIVATE_DATA_MACRO));
  file_id.set_second_id(3);
  file_id.set_third_id(2);
  file_id.set_macro_transfer_seq(0);
  file_id.set_tenant_seq(5);

  // 1.PRIVATE_DATA_MACRO
  // tenant_id_epoch_id/tablet_data/tablet_id/transfer_seq/data/
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_dir_path, common::MAX_PATH_SIZE, "%s/1_0/%s/3/0/%s",
                                        OB_DIR_MGR.get_local_cache_root_dir(), TABLET_DATA_DIR_STR, DATA_MACRO_DIR_STR));
  ASSERT_EQ(0, STRCMP(dir_path, expected_dir_path));

  // 2.SHARED_MAJOR_DATA_MACRO
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::SHARED_MAJOR_DATA_MACRO));
  ASSERT_EQ(OB_ERR_UNEXPECTED, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id));


  // 3.TMP_FILE
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::TMP_FILE));
  // tenant_id_epoch_id/tmp_data/tmp_file_id/segment_id
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_dir_path, common::MAX_PATH_SIZE, "%s/1_0/%s/3",
                                        OB_DIR_MGR.get_local_cache_root_dir(), TMP_DATA_DIR_STR));
  ASSERT_EQ(0, STRCMP(dir_path, expected_dir_path));

  // 4.SERVER_META
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::SERVER_META));
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_dir_path, common::MAX_PATH_SIZE, "%s",
                                        OB_DIR_MGR.get_local_cache_root_dir()));
  ASSERT_EQ(0, STRCMP(dir_path, expected_dir_path));

  // 5.TENANT_SUPER_BLOCK
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::TENANT_SUPER_BLOCK));
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_dir_path, common::MAX_PATH_SIZE, "%s/3_2",
                                        OB_DIR_MGR.get_local_cache_root_dir()));
  ASSERT_EQ(0, STRCMP(dir_path, expected_dir_path));

  // 6.TENANT_UNIT_META
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::TENANT_UNIT_META));
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_dir_path, common::MAX_PATH_SIZE, "%s/3_2",
                                        OB_DIR_MGR.get_local_cache_root_dir()));
  ASSERT_EQ(0, STRCMP(dir_path, expected_dir_path));

  // 7.LS_META
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::LS_META));
  // tenant_id_epoch_id/ls/ls_id_epoch_id/ls_meta
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id, ls_epoch_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_dir_path, common::MAX_PATH_SIZE, "%s/1_0/%s/3_4",
                                        OB_DIR_MGR.get_local_cache_root_dir(), LS_DIR_STR));
  ASSERT_EQ(0, STRCMP(dir_path, expected_dir_path));

  // 8.LS_DUP_TABLE_META
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::LS_DUP_TABLE_META));
  // tenant_id_epoch_id/ls/ls_id_epoch_id/ls_dup_table_meta
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id, ls_epoch_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_dir_path, common::MAX_PATH_SIZE, "%s/1_0/%s/3_4",
                                        OB_DIR_MGR.get_local_cache_root_dir(), LS_DIR_STR));
  ASSERT_EQ(0, STRCMP(dir_path, expected_dir_path));

  // 9.LS_ACTIVE_TABLET_ARRAY
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::LS_ACTIVE_TABLET_ARRAY));
  // tenant_id_epoch_id/ls/ls_id_epoch_id/tablet_id_array
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id, ls_epoch_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_dir_path, common::MAX_PATH_SIZE, "%s/1_0/%s/3_4",
                                        OB_DIR_MGR.get_local_cache_root_dir(), LS_DIR_STR));
  ASSERT_EQ(0, STRCMP(dir_path, expected_dir_path));

  // 10.LS_PENDING_FREE_TABLET_ARRAY
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::LS_PENDING_FREE_TABLET_ARRAY));
  // tenant_id_epoch_id/ls/ls_id_epoch_id/pending_free_tablet_array
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id, ls_epoch_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_dir_path, common::MAX_PATH_SIZE, "%s/1_0/%s/3_4",
                                        OB_DIR_MGR.get_local_cache_root_dir(), LS_DIR_STR));
  ASSERT_EQ(0, STRCMP(dir_path, expected_dir_path));

  // 11.LS_TRANSFER_TABLET_ID_ARRAY
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::LS_TRANSFER_TABLET_ID_ARRAY));
  // tenant_id_epoch_id/ls/ls_id_epoch_id/transfer_tablet_id_array
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id, ls_epoch_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_dir_path, common::MAX_PATH_SIZE, "%s/1_0/%s/3_4",
                                        OB_DIR_MGR.get_local_cache_root_dir(), LS_DIR_STR));
  ASSERT_EQ(0, STRCMP(dir_path, expected_dir_path));

  // 12.PRIVATE_TABLET_META
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::PRIVATE_TABLET_META));
  // tenant_id_epoch_id/ls/ls_id_epoch_id/tablet_meta/tablet_id/tablet_meta_version_transfer_seq
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id, ls_epoch_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_dir_path, common::MAX_PATH_SIZE, "%s/1_0/%s/3_4/%s/2",
                                        OB_DIR_MGR.get_local_cache_root_dir(), LS_DIR_STR, TABLET_META_DIR_STR));
  ASSERT_EQ(0, STRCMP(dir_path, expected_dir_path));

  // 13.PRIVATE_TABLET_CURRENT_VERSION
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::PRIVATE_TABLET_CURRENT_VERSION));
  // tenant_id_epoch_id/ls/ls_id_epoch_id/tablet_meta/tablet_id/current_version
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id, ls_epoch_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_dir_path, common::MAX_PATH_SIZE, "%s/1_0/%s/3_4/%s/2",
                                        OB_DIR_MGR.get_local_cache_root_dir(), LS_DIR_STR, TABLET_META_DIR_STR));
  ASSERT_EQ(0, STRCMP(dir_path, expected_dir_path));

  // 14.SHARED_MAJOR_TABLET_META
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::SHARED_MAJOR_TABLET_META));
  ASSERT_EQ(OB_ERR_UNEXPECTED, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id));

  // 15.COMPACTION_SERVER
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::COMPACTION_SERVER));
  ASSERT_EQ(OB_ERR_UNEXPECTED, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id));

  // 16.LS_SVR_COMPACTION_STATUS
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::LS_SVR_COMPACTION_STATUS));
  ASSERT_EQ(OB_ERR_UNEXPECTED, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id));

  // 17.COMPACTION_REPORT
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::COMPACTION_REPORT));
  ASSERT_EQ(OB_ERR_UNEXPECTED, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id));

  // 18.PRIVATE_META_MACRO
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::PRIVATE_META_MACRO));
  // tenant_id_epoch_id/tablet_data/tablet_id/transfer_seq/meta/
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_dir_path, common::MAX_PATH_SIZE, "%s/1_0/%s/3/0/%s",
                                        OB_DIR_MGR.get_local_cache_root_dir(), TABLET_DATA_DIR_STR, META_MACRO_DIR_STR));
  ASSERT_EQ(0, STRCMP(dir_path, expected_dir_path));

  // 19.SHARED_MAJOR_META_MACRO
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::SHARED_MAJOR_META_MACRO));
  ASSERT_EQ(OB_ERR_UNEXPECTED, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id));

  // 20.SHARED_MAJOR_GC_INFO
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::SHARED_MAJOR_GC_INFO));
  ASSERT_EQ(OB_ERR_UNEXPECTED, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id));

  // 21.SHARED_MAJOR_META_LIST
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::SHARED_MAJOR_META_LIST));
  ASSERT_EQ(OB_ERR_UNEXPECTED, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id));

  // 22.LS_COMPACTION_STATUS
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::LS_COMPACTION_STATUS));
  ASSERT_EQ(OB_ERR_UNEXPECTED, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id));

  // 23.TABLET_COMPACTION_STATUS
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::TABLET_COMPACTION_STATUS));
  ASSERT_EQ(OB_ERR_UNEXPECTED, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id));

  // 24.TENANT_DISK_SPACE_META
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::TENANT_DISK_SPACE_META));
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id));
  ASSERT_EQ(OB_SUCCESS, databuff_printf(expected_dir_path, common::MAX_PATH_SIZE, "%s/3_2",
                                        OB_DIR_MGR.get_local_cache_root_dir()));
  ASSERT_EQ(0, STRCMP(dir_path, expected_dir_path));

  // 25.SHARED_MAJOR_TABLET_META
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::SHARED_TABLET_ID));
  ASSERT_EQ(OB_ERR_UNEXPECTED, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id));

  // 26.LS_COMPACTION_LIST
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::LS_COMPACTION_LIST));
  ASSERT_EQ(OB_ERR_UNEXPECTED, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id));

  // 27. IS_DELETED
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::IS_SHARED_TABLET_DELETED));
  ASSERT_EQ(OB_ERR_UNEXPECTED, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id));

  // 28. IS_SHARED_TENANT_DELETED
  dir_path[0] = '\0';
  expected_dir_path[0] = '\0';
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::IS_SHARED_TENANT_DELETED));
  ASSERT_EQ(OB_ERR_UNEXPECTED, tenant_file_mgr->get_file_parent_dir(dir_path, common::MAX_PATH_SIZE, file_id));
}

TEST_F(TestFileManager, test_private_macro_file_operator)
{
  int ret = OB_SUCCESS;
  ObTenantFileManager* tenant_file_mgr = MTL(ObTenantFileManager*);
  ASSERT_NE(nullptr, tenant_file_mgr);
  MacroBlockId file_id;
  file_id.set_id_mode(static_cast<uint64_t>(ObMacroBlockIdMode::ID_MODE_SHARE));
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::PRIVATE_DATA_MACRO));
  file_id.set_second_id(3);  //tablet_id
  file_id.set_third_id(2);   //seq_id
  file_id.set_macro_transfer_seq(0); // transfer_seq
  file_id.set_tenant_seq(5);  //tenant_seq

  // step 1: create dir
  int64_t tablet_id = 3;
  ASSERT_EQ(OB_SUCCESS, OB_DIR_MGR.create_tablet_data_tablet_id_transfer_seq_dir(MTL_ID(), MTL_EPOCH_ID(), tablet_id, 0/*transfer_seq*/));

  // step 2: test write private_macro_file
  ObStorageObjectHandle write_object_handle;
  ASSERT_EQ(OB_SUCCESS, write_object_handle.set_macro_block_id(file_id));
  const int64_t write_io_size = 4096; // 4KB
  char write_buf[write_io_size] = { 0 };
  memset(write_buf, 'a', write_io_size);
  ObStorageObjectWriteInfo write_info;
  write_info.io_desc_.set_wait_event(1);
  write_info.buffer_ = write_buf;
  write_info.offset_ = 0;
  write_info.size_ = write_io_size;
  write_info.io_timeout_ms_ = DEFAULT_IO_WAIT_TIME_MS;
  write_info.mtl_tenant_id_ = MTL_ID();
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->write_file(write_info, write_object_handle));

  // step 3: test read private_macro_file
  ObStorageObjectHandle read_object_handle;
  ObStorageObjectReadInfo read_info;
  read_info.macro_block_id_ = file_id;
  read_info.io_desc_.set_wait_event(1);
  char read_buf[write_io_size] = { 0 };
  read_info.buf_ = read_buf;
  read_info.offset_ = 0;
  read_info.size_ = write_io_size;
  read_info.io_timeout_ms_ = DEFAULT_IO_WAIT_TIME_MS;
  read_info.mtl_tenant_id_ = MTL_ID();
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->pread_file(read_info, read_object_handle));
  ASSERT_NE(nullptr, read_object_handle.get_buffer());
  ASSERT_EQ(read_info.size_, read_object_handle.get_data_size());
  ASSERT_EQ(0, memcmp(write_buf, read_object_handle.get_buffer(), write_io_size));

  // test 4 : test fsync private_macro_file
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->fsync_file(file_id));

  // step 5: test is exist file
  bool is_exist = false;
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->is_exist_file(file_id, 0 /*ls_epoch_id*/, is_exist));
  ASSERT_TRUE(is_exist);

  // step 6: test get file length
  int64_t file_length = 0;
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->get_file_length(file_id, 0 /*ls_epoch_id*/, file_length));
  ASSERT_EQ(write_io_size, file_length);

  // step 7: test list tablet_dir
  ObArray<int64_t> tablet_dirs;
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->list_tablet_data_dir(tablet_dirs));
  ASSERT_EQ(1, tablet_dirs.count());
  ASSERT_EQ(tablet_id, tablet_dirs.at(0));

  // test 8: test calc_private_macro_disk_space
  int64_t total_disk_size = 0;
  int64_t expected_disk_size = write_io_size;
  char dir_path[ObBaseFileManager::OB_MAX_FILE_PATH_LENGTH] = {0};
  ObIODFileStat statbuf;
  ASSERT_EQ(OB_SUCCESS, OB_DIR_MGR.get_tablet_data_tablet_id_dir(dir_path, sizeof(dir_path), MTL_ID(), MTL_EPOCH_ID(), tablet_id));
  ASSERT_EQ(OB_SUCCESS, ObIODeviceLocalFileOp::stat(dir_path, statbuf));
  expected_disk_size += statbuf.size_;
  dir_path[0] ='\0';
  ASSERT_EQ(OB_SUCCESS, OB_DIR_MGR.get_tablet_data_tablet_id_transfer_seq_dir(dir_path, sizeof(dir_path),
            MTL_ID(), MTL_EPOCH_ID(), tablet_id, 0/*transfer_seq*/));
  ASSERT_EQ(OB_SUCCESS, ObIODeviceLocalFileOp::stat(dir_path, statbuf));
  expected_disk_size += statbuf.size_;
  dir_path[0] ='\0';
  ASSERT_EQ(OB_SUCCESS, OB_DIR_MGR.get_local_tablet_id_macro_dir(dir_path, sizeof(dir_path),
            MTL_ID(), MTL_EPOCH_ID(), tablet_id, 0/*transfer_seq*/, ObMacroType::DATA_MACRO));
  ASSERT_EQ(OB_SUCCESS, ObIODeviceLocalFileOp::stat(dir_path, statbuf));
  expected_disk_size += statbuf.size_;
  dir_path[0] ='\0';
  ASSERT_EQ(OB_SUCCESS, OB_DIR_MGR.get_local_tablet_id_macro_dir(dir_path, sizeof(dir_path),
            MTL_ID(), MTL_EPOCH_ID(), tablet_id, 0/*transfer_seq*/, ObMacroType::META_MACRO));
  ASSERT_EQ(OB_SUCCESS, ObIODeviceLocalFileOp::stat(dir_path, statbuf));
  expected_disk_size += statbuf.size_;
  ob_usleep(2000*1000);
  int64_t start_calc_size_time_s = ObTimeUtility::current_time_s();
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->calc_private_macro_disk_space(start_calc_size_time_s, total_disk_size));
  ASSERT_EQ(expected_disk_size, total_disk_size);

  // step 9: test delete file
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->delete_file(file_id));

  // step 10: test delete dir
  ASSERT_EQ(OB_SUCCESS, OB_DIR_MGR.delete_tablet_data_tablet_id_transfer_seq_dir(MTL_ID(), MTL_EPOCH_ID(), tablet_id, 0/*transfer_seq*/));
  ASSERT_EQ(OB_SUCCESS, OB_DIR_MGR.delete_tablet_data_tablet_id_dir(MTL_ID(), MTL_EPOCH_ID(), tablet_id));

}

TEST_F(TestFileManager, test_tmp_file_operator)
{
  int ret = OB_SUCCESS;
  ObTenantFileManager* tenant_file_mgr = MTL(ObTenantFileManager*);
  ASSERT_NE(nullptr, tenant_file_mgr);
  MacroBlockId file_id;
  file_id.set_id_mode(static_cast<uint64_t>(ObMacroBlockIdMode::ID_MODE_SHARE));
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::TMP_FILE));
  file_id.set_second_id(3);  //tmp_file_id
  file_id.set_third_id(2);   //segment_id

  // step 1: create dir
  int64_t tmp_file_id = 3;
  // ASSERT_EQ(OB_SUCCESS, OB_DIR_MGR.create_tmp_file_dir(tmp_file_id));

  // step 2: test write tmp_file
  ObStorageObjectHandle write_object_handle;
  ASSERT_EQ(OB_SUCCESS, write_object_handle.set_macro_block_id(file_id));
  const int64_t write_io_size = 8 * 1024; // 8KB
  char write_buf[write_io_size] = { 0 };
  memset(write_buf, 'a', write_io_size);
  ObStorageObjectWriteInfo write_info;
  write_info.io_desc_.set_wait_event(1);
  write_info.io_desc_.set_unsealed();
  write_info.buffer_ = write_buf;
  write_info.offset_ = 0;
  write_info.size_ = write_io_size;
  write_info.io_timeout_ms_ = DEFAULT_IO_WAIT_TIME_MS;
  write_info.mtl_tenant_id_ = MTL_ID();
  write_info.tmp_file_valid_length_ = write_io_size;
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->append_file(write_info, write_object_handle));

  // step 3: test read tmp_file
  ObStorageObjectHandle read_object_handle;
  ObStorageObjectReadInfo read_info;
  read_info.macro_block_id_ = file_id;
  read_info.io_desc_.set_wait_event(1);
  char read_buf[write_io_size] = { 0 };
  read_info.buf_ = read_buf;
  read_info.offset_ = 0;
  read_info.size_ = write_io_size;
  read_info.io_timeout_ms_ = DEFAULT_IO_WAIT_TIME_MS;
  read_info.mtl_tenant_id_ = MTL_ID();
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->pread_file(read_info, read_object_handle));
  ASSERT_NE(nullptr, read_object_handle.get_buffer());
  ASSERT_EQ(read_info.size_, read_object_handle.get_data_size());
  ASSERT_EQ(0, memcmp(write_buf, read_object_handle.get_buffer(), write_io_size));

  // test 4: test fsync tmp_file
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->fsync_file(file_id));

  // test 5: test append tmp_file
  ObStorageObjectWriteInfo append_info;
  append_info.io_desc_.set_wait_event(1);
  append_info.io_desc_.set_unsealed();
  append_info.buffer_ = write_buf;
  append_info.offset_ = write_io_size;
  append_info.size_ = write_io_size;
  append_info.io_timeout_ms_ = DEFAULT_IO_WAIT_TIME_MS;
  append_info.mtl_tenant_id_ = MTL_ID();
  append_info.tmp_file_valid_length_ = write_io_size + write_io_size;
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->append_file(append_info, write_object_handle));

  // step 6: test read tmp_file
  read_object_handle.reset();
  read_info.macro_block_id_ = file_id;
  read_info.io_desc_.set_wait_event(1);
  read_buf[0] = '\0';
  read_info.buf_ = read_buf;
  read_info.offset_ = write_io_size;
  read_info.size_ = write_io_size;
  read_info.io_timeout_ms_ = DEFAULT_IO_WAIT_TIME_MS;
  read_info.mtl_tenant_id_ = MTL_ID();
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->pread_file(read_info, read_object_handle));
  ASSERT_NE(nullptr, read_object_handle.get_buffer());
  ASSERT_EQ(read_info.size_, read_object_handle.get_data_size());
  ASSERT_EQ(0, memcmp(write_buf, read_object_handle.get_buffer(), write_io_size));

  // step 7: test is exist file
  bool is_exist = false;
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->is_exist_file(file_id, 0, is_exist));
  ASSERT_TRUE(is_exist);

  // step 8: test get file length
  int64_t file_length = 0;
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->get_file_length(file_id, 0, file_length));
  ASSERT_EQ(2 * write_io_size, file_length);

  // test 9: test calc_private_macro_disk_space
  int64_t total_disk_size = 0;
  int64_t tmp_file_size = 0;
  int64_t total_tmp_file_read_cache_alloc_size = 0;
  char dir_path[ObBaseFileManager::OB_MAX_FILE_PATH_LENGTH] = {0};
  int64_t expected_disk_size = 2 * write_io_size;
  ObIODFileStat statbuf;
  ASSERT_EQ(OB_SUCCESS, OB_DIR_MGR.get_local_tmp_file_dir(dir_path, sizeof(dir_path), MTL_ID(), MTL_EPOCH_ID(), tmp_file_id));
  ASSERT_EQ(OB_SUCCESS, ObIODeviceLocalFileOp::stat(dir_path, statbuf));
  expected_disk_size += statbuf.size_;
  ob_usleep(2000*1000);
  int64_t start_calc_size_time_s = ObTimeUtility::current_time_s();
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->calc_private_macro_disk_space(start_calc_size_time_s, total_disk_size));
  ASSERT_EQ(0, total_disk_size);
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->calc_tmp_data_disk_space(start_calc_size_time_s, total_tmp_file_read_cache_alloc_size, tmp_file_size));
  ASSERT_EQ(expected_disk_size, tmp_file_size);
  ASSERT_EQ(0, total_tmp_file_read_cache_alloc_size);

  // step 10: test delete file
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->delete_tmp_file(file_id));

}

TEST_F(TestFileManager, test_meta_file_operator)
{
  int ret = OB_SUCCESS;
  ObTenantFileManager* tenant_file_mgr = MTL(ObTenantFileManager*);
  ASSERT_NE(nullptr, tenant_file_mgr);
  MacroBlockId file_id;
  const int64_t ls_id = 7;
  const int64_t ls_epoch_id = 0;
  const int64_t tablet_id = 8;
  const int64_t meta_version_id = 9;
  file_id.set_id_mode(static_cast<uint64_t>(ObMacroBlockIdMode::ID_MODE_SHARE));
  file_id.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::PRIVATE_TABLET_META));
  file_id.set_second_id(ls_id);
  file_id.set_third_id(tablet_id);
  file_id.set_meta_transfer_seq(0);
  file_id.set_meta_version_id(meta_version_id);

  // step 1: create dir
  ASSERT_EQ(OB_SUCCESS, OB_DIR_MGR.create_ls_id_dir(MTL_ID(), MTL_EPOCH_ID(), ls_id, ls_epoch_id));
  ASSERT_EQ(OB_SUCCESS, OB_DIR_MGR.create_tablet_meta_tablet_id_dir(MTL_ID(), MTL_EPOCH_ID(), ls_id, ls_epoch_id, tablet_id));

  // step 2: test write private_tablet_meta
  ObStorageObjectHandle write_object_handle;
  ASSERT_EQ(OB_SUCCESS, write_object_handle.set_macro_block_id(file_id));
  const int64_t write_io_size = 4096; // 4KB
  char write_buf[write_io_size] = { 0 };
  memset(write_buf, 'a', write_io_size);
  ObStorageObjectWriteInfo write_info;
  write_info.io_desc_.set_wait_event(1);
  write_info.buffer_ = write_buf;
  write_info.offset_ = 0;
  write_info.size_ = write_io_size;
  write_info.io_timeout_ms_ = DEFAULT_IO_WAIT_TIME_MS;
  write_info.mtl_tenant_id_ = MTL_ID();
  write_info.ls_epoch_id_ = ls_epoch_id;
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->write_file(write_info, write_object_handle));

  // step 3: test read private_tablet_meta
  ObStorageObjectHandle read_object_handle;
  ObStorageObjectReadInfo read_info;
  read_info.macro_block_id_ = file_id;
  read_info.io_desc_.set_wait_event(1);
  char read_buf[write_io_size] = { 0 };
  read_info.buf_ = read_buf;
  read_info.offset_ = 0;
  read_info.size_ = write_io_size;
  read_info.io_timeout_ms_ = DEFAULT_IO_WAIT_TIME_MS;
  read_info.mtl_tenant_id_ = MTL_ID();
  read_info.ls_epoch_id_ = ls_epoch_id;
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->pread_file(read_info, read_object_handle));
  ASSERT_NE(nullptr, read_object_handle.get_buffer());
  ASSERT_EQ(read_info.size_, read_object_handle.get_data_size());
  ASSERT_EQ(0, memcmp(write_buf, read_object_handle.get_buffer(), write_io_size));

  // test 4 : test fsync private_tablet_meta
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->fsync_file(file_id));

  // step 5: test is exist file
  bool is_exist = false;
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->is_exist_file(file_id, ls_epoch_id, is_exist));
  ASSERT_TRUE(is_exist);

  // step 6: test get file length
  int64_t file_length = 0;
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->get_file_length(file_id, ls_epoch_id, file_length));
  ASSERT_EQ(write_io_size, file_length);

  // step 7: test list tablet_dir
  ObArray<int64_t> tablet_dirs;
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->list_tablet_meta_dir(ls_id, ls_epoch_id, tablet_dirs));
  ASSERT_EQ(1, tablet_dirs.count());
  ASSERT_EQ(tablet_id, tablet_dirs.at(0));

  // test 8: test calc_meta_file_disk_space
  int64_t total_disk_size = 0;
  int64_t expected_disk_size = write_io_size;
  char dir_path[ObBaseFileManager::OB_MAX_FILE_PATH_LENGTH] = {0};
  ObIODFileStat statbuf;
  ASSERT_EQ(OB_SUCCESS, OB_DIR_MGR.get_tablet_meta_tablet_id_dir(dir_path, sizeof(dir_path),
            MTL_ID(), MTL_EPOCH_ID(), ls_id, ls_epoch_id, tablet_id));
  ASSERT_EQ(OB_SUCCESS, ObIODeviceLocalFileOp::stat(dir_path, statbuf));
  expected_disk_size += statbuf.size_;
  dir_path[0] = '\0';
  ASSERT_EQ(OB_SUCCESS, OB_DIR_MGR.get_tablet_meta_dir(dir_path, sizeof(dir_path),
            MTL_ID(), MTL_EPOCH_ID(), ls_id, ls_epoch_id));
  ASSERT_EQ(OB_SUCCESS, ObIODeviceLocalFileOp::stat(dir_path, statbuf));
  expected_disk_size += statbuf.size_;
  dir_path[0] = '\0';
  ASSERT_EQ(OB_SUCCESS, OB_DIR_MGR.get_ls_id_dir(dir_path, sizeof(dir_path),
            MTL_ID(), MTL_EPOCH_ID(), ls_id, ls_epoch_id));
  ASSERT_EQ(OB_SUCCESS, ObIODeviceLocalFileOp::stat(dir_path, statbuf));
  expected_disk_size += statbuf.size_;
  ob_usleep(2000*1000);
  int64_t start_calc_size_time_s = ObTimeUtility::current_time_s();
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->calc_meta_file_disk_space(start_calc_size_time_s, total_disk_size));
  // private_tablet_meta + tablet_id_dir
  ASSERT_EQ(expected_disk_size, total_disk_size);

  // step 9: test delete file
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->delete_file(file_id));
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->is_exist_file(file_id, ls_epoch_id, is_exist));
  ASSERT_FALSE(is_exist);

  // step 10: test delete dir
  ASSERT_EQ(OB_SUCCESS, OB_DIR_MGR.delete_tablet_meta_tablet_id_dir(MTL_ID(), MTL_EPOCH_ID(), ls_id, ls_epoch_id, tablet_id));
  ASSERT_EQ(OB_SUCCESS, OB_DIR_MGR.delete_ls_id_dir(MTL_ID(), MTL_EPOCH_ID(), ls_id, ls_epoch_id));
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->calc_meta_file_disk_space(start_calc_size_time_s, total_disk_size));
  ASSERT_EQ(0, total_disk_size);
}

/* Test whether the data written by pwrite_cache_block is 4K aligned when the valid data of buf is smaller than
 * size. */
TEST_F(TestFileManager, test_micro_cache_pread_and_pwrite)
{
  int ret = OB_SUCCESS;
  ObTenantFileManager *tenant_file_mgr = MTL(ObTenantFileManager *);
  ASSERT_NE(nullptr, tenant_file_mgr);

  const int64_t alignment = 4096;
  const int64_t WRITE_IO_SIZE = 1 << 14;      // 16K
  const int64_t valid_data_size = 15 * 1024;  // 15K
  ObMemAttr attr(MTL_ID(), "test");
  char *write_buf = static_cast<char *>(ob_malloc_align(alignment, WRITE_IO_SIZE, attr));
  char *read_buf = static_cast<char *>(ob_malloc_align(alignment, WRITE_IO_SIZE, attr));

  MEMSET(write_buf, '\0', WRITE_IO_SIZE);
  MEMSET(write_buf, 'a', valid_data_size);  // 15K
  int64_t written_size = 0;
  int64_t read_size = 0;
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->pwrite_cache_block(0, WRITE_IO_SIZE, write_buf, written_size));
  ASSERT_EQ(written_size, WRITE_IO_SIZE);
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->fsync_cache_file());
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->pread_cache_block(0, WRITE_IO_SIZE, read_buf, read_size));
  ASSERT_EQ(read_size, WRITE_IO_SIZE);
  ASSERT_EQ(0, memcmp(write_buf, read_buf, read_size));

  ob_free_align(write_buf);
  ob_free_align(read_buf);
}

TEST_F(TestFileManager, test_tenant_disk_space_meta)
{
  int ret = OB_SUCCESS;
  ObTenantFileManager* tenant_file_mgr = MTL(ObTenantFileManager*);
  ASSERT_NE(nullptr, tenant_file_mgr);
  ObTenantDiskSpaceManager* tenant_disk_space_mgr = MTL(ObTenantDiskSpaceManager*);
  ASSERT_NE(nullptr, tenant_disk_space_mgr);

  // step 1: write tmp_file
  MacroBlockId tmp_file;
  int64_t tmp_file_id = 4;
  int64_t segment_id = 5;
  tmp_file.set_id_mode(static_cast<uint64_t>(ObMacroBlockIdMode::ID_MODE_SHARE));
  tmp_file.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::TMP_FILE));
  tmp_file.set_second_id(tmp_file_id);
  tmp_file.set_third_id(segment_id);
  ObStorageObjectHandle write_object_handle;
  ASSERT_EQ(OB_SUCCESS, write_object_handle.set_macro_block_id(tmp_file));
  const int64_t write_io_size = 4096 * 2; // 8KB
  char write_buf[write_io_size] = { 0 };
  memset(write_buf, 'a', write_io_size);
  ObStorageObjectWriteInfo write_info;
  write_info.io_desc_.set_wait_event(1);
  write_info.io_desc_.set_unsealed();
  write_info.buffer_ = write_buf;
  write_info.offset_ = 0;
  write_info.size_ = write_io_size;
  write_info.io_timeout_ms_ = DEFAULT_IO_WAIT_TIME_MS;
  write_info.mtl_tenant_id_ = MTL_ID();
  ASSERT_EQ(OB_SUCCESS, OB_DIR_MGR.create_tmp_file_dir(MTL_ID(), MTL_EPOCH_ID(), tmp_file_id));
  int64_t expected_disk_size = write_io_size;
  char dir_path[ObBaseFileManager::OB_MAX_FILE_PATH_LENGTH] = {0};
  ObIODFileStat statbuf;
  ASSERT_EQ(OB_SUCCESS, OB_DIR_MGR.get_local_tmp_file_dir(dir_path, sizeof(dir_path),
            MTL_ID(), MTL_EPOCH_ID(), tmp_file_id));
  ASSERT_EQ(OB_SUCCESS, ObIODeviceLocalFileOp::stat(dir_path, statbuf));
  expected_disk_size += statbuf.size_;
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->append_file(write_info, write_object_handle));
  // step 2: write tenant disk space meta
  ObTenantDiskSpaceMeta disk_space_meta(MTL_ID());
  disk_space_meta.body_.tmp_file_write_cache_alloc_size_ = tenant_disk_space_mgr->get_tmp_file_write_cache_alloc_size();
  ASSERT_EQ(OB_SUCCESS, disk_space_meta.header_.construct_header(disk_space_meta.body_));
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->write_tenant_disk_space_meta(disk_space_meta));
  // step 3: read tenant disk space meta
  disk_space_meta.reset();
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->read_tenant_disk_space_meta(disk_space_meta));
  ASSERT_EQ(expected_disk_size, disk_space_meta.body_.tmp_file_write_cache_alloc_size_);
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->delete_tmp_file(tmp_file));
}

TEST_F(TestFileManager, test_ss_format)
{
  int ret = OB_SUCCESS;
  bool is_exist = false;
  ObBackupDest storage_dest;
  ASSERT_EQ(OB_SUCCESS, OB_SERVER_FILE_MGR.get_storage_dest(storage_dest));
  // step 1: is exist ss_format
  ASSERT_EQ(OB_SUCCESS, ObSSFormatUtil::is_exist_ss_format(storage_dest, is_exist));
  ASSERT_EQ(false, is_exist);
  // step 2: write ss_format
  ObSSFormat ss_format;
  const uint64_t cur_cluster_version = CLUSTER_CURRENT_VERSION;
  const int64_t cur_us = ObTimeUtility::fast_current_time();
  ss_format.body_.cluster_version_ = cur_cluster_version;
  ss_format.body_.create_timestamp_ = cur_us;
  ASSERT_EQ(OB_SUCCESS, ss_format.header_.construct_header(ss_format.body_));
  ASSERT_EQ(OB_SUCCESS, ObSSFormatUtil::write_ss_format(storage_dest, ss_format));
  // step 3: is exist ss_format
  ASSERT_EQ(OB_SUCCESS, ObSSFormatUtil::is_exist_ss_format(storage_dest, is_exist));
  ASSERT_EQ(true, is_exist);
  // step 4: read ss_format
  ss_format.reset();
  ASSERT_EQ(OB_SUCCESS, ObSSFormatUtil::read_ss_format(storage_dest, ss_format));
  ASSERT_EQ(cur_cluster_version, ss_format.body_.cluster_version_);
  ASSERT_EQ(cur_us, ss_format.body_.create_timestamp_);
}

TEST_F(TestFileManager, test_list_and_delete_dir_operator)
{
  int ret = OB_SUCCESS;
  ObTenantFileManager* tenant_file_mgr = MTL(ObTenantFileManager*);
  ASSERT_NE(nullptr, tenant_file_mgr);

  // test1: list tmp file
  MacroBlockId tmp_file;
  int64_t tmp_file_id = 3;
  int64_t segment_file_id = 2;
  tmp_file.set_id_mode(static_cast<uint64_t>(ObMacroBlockIdMode::ID_MODE_SHARE));
  tmp_file.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::TMP_FILE));
  tmp_file.set_second_id(tmp_file_id);
  tmp_file.set_third_id(segment_file_id);
  ObStorageObjectHandle write_object_handle;
  ASSERT_EQ(OB_SUCCESS, write_object_handle.set_macro_block_id(tmp_file));
  const int64_t write_io_size = 8 * 1024; // 8KB
  char write_buf[write_io_size] = { 0 };
  memset(write_buf, 'a', write_io_size);
  ObStorageObjectWriteInfo write_info;
  write_info.io_desc_.set_wait_event(1);
  write_info.io_desc_.set_unsealed();
  write_info.buffer_ = write_buf;
  write_info.offset_ = 0;
  write_info.size_ = write_io_size;
  write_info.io_timeout_ms_ = DEFAULT_IO_WAIT_TIME_MS;
  write_info.mtl_tenant_id_ = MTL_ID();
  write_info.tmp_file_valid_length_ = write_io_size;
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->append_file(write_info, write_object_handle));
  ObArray<MacroBlockId> tmp_files;
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->list_tmp_file(tmp_files));
  ASSERT_EQ(1, tmp_files.count());
  ASSERT_EQ(tmp_file_id, tmp_files.at(0).second_id());

  // test2: list private tablet data macro
  MacroBlockId tablet_data_macro;
  int64_t tablet_id = 4;
  int64_t server_id = 5;
  int64_t seq_id = 6;
  tablet_data_macro.set_id_mode(static_cast<uint64_t>(ObMacroBlockIdMode::ID_MODE_SHARE));
  tablet_data_macro.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::PRIVATE_DATA_MACRO));
  tablet_data_macro.set_second_id(tablet_id);
  tablet_data_macro.set_third_id(server_id);
  tablet_data_macro.set_macro_transfer_seq(0); // transfer_seq
  tablet_data_macro.set_tenant_seq(seq_id);  //tenant_seq
  write_object_handle.reset();
  ASSERT_EQ(OB_SUCCESS, write_object_handle.set_macro_block_id(tablet_data_macro));
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->write_file(write_info, write_object_handle));
  ObArray<MacroBlockId> tablet_data_macros;
  ObArray<int64_t> tablet_ids;
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->list_tablet_data_dir(tablet_ids));
  ASSERT_EQ(1, tablet_ids.count());
  ASSERT_EQ(tablet_id, tablet_ids.at(0));
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->list_private_macro_file(tablet_ids.at(0), 0/*transfer_seq*/, tablet_data_macros));
  ASSERT_EQ(1, tablet_data_macros.count());
  ASSERT_EQ(tablet_data_macro.second_id(), tablet_data_macros.at(0).second_id());
  ASSERT_EQ(tablet_data_macro.third_id(), tablet_data_macros.at(0).third_id());
  ASSERT_EQ(tablet_data_macro.tenant_seq(), tablet_data_macros.at(0).tenant_seq());
  ASSERT_EQ(tablet_data_macro.macro_transfer_seq(), tablet_data_macros.at(0).macro_transfer_seq());

  // test3: list private tablet meta macro
  MacroBlockId tablet_meta_macro;
  tablet_id = 4;
  server_id = 5;
  seq_id = 7;
  tablet_meta_macro.set_id_mode(static_cast<uint64_t>(ObMacroBlockIdMode::ID_MODE_SHARE));
  tablet_meta_macro.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::PRIVATE_META_MACRO));
  tablet_meta_macro.set_second_id(tablet_id);
  tablet_meta_macro.set_third_id(server_id);
  tablet_meta_macro.set_macro_transfer_seq(0); // transfer_seq
  tablet_meta_macro.set_tenant_seq(seq_id);  //tenant_seq
  write_object_handle.reset();
  ASSERT_EQ(OB_SUCCESS, write_object_handle.set_macro_block_id(tablet_meta_macro));
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->write_file(write_info, write_object_handle));
  ObArray<MacroBlockId> tablet_meta_macros;
  tablet_ids.reset();
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->list_tablet_data_dir(tablet_ids));
  ASSERT_EQ(1, tablet_ids.count());
  ASSERT_EQ(tablet_id, tablet_ids.at(0));
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->list_private_macro_file(tablet_ids.at(0), 0/*transfer_seq*/, tablet_meta_macros));
  ASSERT_EQ(2, tablet_meta_macros.count());
  ASSERT_EQ(tablet_meta_macro.second_id(), tablet_meta_macros.at(1).second_id());
  ASSERT_EQ(tablet_meta_macro.third_id(), tablet_meta_macros.at(1).third_id());
  ASSERT_EQ(tablet_meta_macro.tenant_seq(), tablet_meta_macros.at(1).tenant_seq());
  ASSERT_EQ(tablet_meta_macro.macro_transfer_seq(), tablet_meta_macros.at(1).macro_transfer_seq());
  ASSERT_EQ(tablet_data_macro.second_id(), tablet_meta_macros.at(0).second_id());
  ASSERT_EQ(tablet_data_macro.third_id(), tablet_meta_macros.at(0).third_id());
  ASSERT_EQ(tablet_data_macro.tenant_seq(), tablet_meta_macros.at(0).tenant_seq());
  ASSERT_EQ(tablet_data_macro.macro_transfer_seq(), tablet_meta_macros.at(0).macro_transfer_seq());

  // test4: list private tablet meta version
  MacroBlockId ls_meta;
  int64_t ls_id = 8;
  int64_t ls_epoch_id = 0;
  int64_t meta_version_id = 10;
  ls_meta.set_id_mode(static_cast<uint64_t>(ObMacroBlockIdMode::ID_MODE_SHARE));
  ls_meta.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::LS_META));
  ls_meta.set_second_id(ls_id);
  write_object_handle.reset();
  ASSERT_EQ(OB_SUCCESS, write_object_handle.set_macro_block_id(ls_meta));
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->write_file(write_info, write_object_handle));
  MacroBlockId tablet_meta_version;
  tablet_meta_version.set_id_mode(static_cast<uint64_t>(ObMacroBlockIdMode::ID_MODE_SHARE));
  tablet_meta_version.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::PRIVATE_TABLET_META));
  tablet_meta_version.set_second_id(ls_id);
  tablet_meta_version.set_third_id(tablet_id);
  tablet_data_macro.set_meta_transfer_seq(0); // transfer_seq
  tablet_data_macro.set_meta_version_id(meta_version_id);  //tenant_seq
  write_object_handle.reset();
  ASSERT_EQ(OB_SUCCESS, write_object_handle.set_macro_block_id(tablet_meta_version));
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->write_file(write_info, write_object_handle));

  // test5: list shared tablet ids
  MacroBlockId shared_tablet_ids;
  int shared_tablet_id = 11;
  shared_tablet_ids.set_id_mode(static_cast<uint64_t>(ObMacroBlockIdMode::ID_MODE_SHARE));
  shared_tablet_ids.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::SHARED_TABLET_ID));
  shared_tablet_ids.set_second_id(shared_tablet_id);
  write_object_handle.reset();
  ASSERT_EQ(OB_SUCCESS, write_object_handle.set_macro_block_id(shared_tablet_ids));
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->write_file(write_info, write_object_handle));
  tablet_ids.reset();
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->list_shared_tablet_ids(tablet_ids));
  ASSERT_EQ(1, tablet_ids.count());
  ASSERT_EQ(shared_tablet_id, tablet_ids.at(0));

  // test6: list shared tenant dir
  ObArray<uint64_t> tenant_ids;
  uint64_t tenant_id = 1;
  ASSERT_EQ(OB_SUCCESS, OB_SERVER_FILE_MGR.list_shared_tenant_ids(MTL_ID(), tenant_ids));
  ASSERT_EQ(1, tenant_ids.count());
  ASSERT_EQ(tenant_id, tenant_ids.at(0));
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->delete_file(shared_tablet_ids));
  bool is_exist = true;
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->is_exist_file(shared_tablet_ids, 0, is_exist));
  ASSERT_EQ(false, is_exist);
  tenant_ids.reset();
  // ASSERT_EQ(OB_SUCCESS, OB_SERVER_FILE_MGR.list_shared_tenant_ids(MTL_ID(), tenant_ids));
  // ASSERT_EQ(0, tenant_ids.count());

  // test7: list shared tablet meta macro
  MacroBlockId shared_tablet_meta_macro;
  int64_t tablet_meta_id = 13;
  seq_id = 14;
  shared_tablet_meta_macro.set_id_mode(static_cast<uint64_t>(ObMacroBlockIdMode::ID_MODE_SHARE));
  shared_tablet_meta_macro.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::SHARED_MAJOR_META_MACRO));
  shared_tablet_meta_macro.set_second_id(tablet_meta_id);
  shared_tablet_meta_macro.set_third_id(seq_id);
  write_object_handle.reset();
  ASSERT_EQ(OB_SUCCESS, write_object_handle.set_macro_block_id(shared_tablet_meta_macro));
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->write_file(write_info, write_object_handle));

  // test8: delete shared tablet data dir
  is_exist = false;
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->is_exist_file(shared_tablet_meta_macro, 0, is_exist));
  ASSERT_EQ(true, is_exist);
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->delete_shared_tablet_data_dir(tablet_meta_id));
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->is_exist_file(shared_tablet_meta_macro, 0, is_exist));
  ASSERT_EQ(false, is_exist);

  // test9: list shared tablet meta version
  MacroBlockId shared_tablet_meta_version;
  int64_t tablet_meta_version_id = 15;
  meta_version_id = 16;
  shared_tablet_meta_version.set_id_mode(static_cast<uint64_t>(ObMacroBlockIdMode::ID_MODE_SHARE));
  shared_tablet_meta_version.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::SHARED_MAJOR_TABLET_META));
  shared_tablet_meta_version.set_second_id(tablet_meta_version_id);
  shared_tablet_meta_version.set_third_id(meta_version_id);
  write_object_handle.reset();
  ASSERT_EQ(OB_SUCCESS, write_object_handle.set_macro_block_id(shared_tablet_meta_version));
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->write_file(write_info, write_object_handle));

  // test10: delete shared tablet meta versions
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->delete_file(shared_tablet_meta_version));

  // test11: delete tmp file
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->delete_tmp_file(tmp_file));

  // test12: delete private macro files
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->delete_files(tablet_meta_macros));

  // test13: delete ls dir file
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->delete_ls_dir(ls_id, ls_epoch_id));

}

TEST_F(TestFileManager, test_list_local_files_rec)
{
  int ret = OB_SUCCESS;
  ObTenantFileManager* tenant_file_mgr = MTL(ObTenantFileManager*);
  ASSERT_NE(nullptr, tenant_file_mgr);

  // test1: list tmp file
  MacroBlockId tmp_file;
  int64_t tmp_file_id = 3;
  int64_t segment_file_id = 2;
  tmp_file.set_id_mode(static_cast<uint64_t>(ObMacroBlockIdMode::ID_MODE_SHARE));
  tmp_file.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::TMP_FILE));
  tmp_file.set_second_id(tmp_file_id);
  tmp_file.set_third_id(segment_file_id);
  ObStorageObjectHandle write_object_handle;
  ASSERT_EQ(OB_SUCCESS, write_object_handle.set_macro_block_id(tmp_file));
  const int64_t write_io_size = 8 * 1024; // 8KB
  char write_buf[write_io_size] = { 0 };
  memset(write_buf, 'a', write_io_size);
  ObStorageObjectWriteInfo write_info;
  write_info.io_desc_.set_resource_group_id(0);
  write_info.io_desc_.set_wait_event(1);
  write_info.io_desc_.set_unsealed();
  write_info.buffer_ = write_buf;
  write_info.offset_ = 0;
  write_info.size_ = write_io_size;
  write_info.io_timeout_ms_ = DEFAULT_IO_WAIT_TIME_MS;
  write_info.mtl_tenant_id_ = MTL_ID();
  write_info.tmp_file_valid_length_ = write_io_size;
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->append_file(write_info, write_object_handle));
  TestDirDelOp dir_del_op;
  TestFileDelOp file_del_op;
  char file_path[common::MAX_PATH_SIZE] = {0};
  bool is_exist = true;
  ASSERT_EQ(OB_SUCCESS, OB_DIR_MGR.get_local_tmp_data_dir(file_path, sizeof(file_path), MTL_ID(), MTL_EPOCH_ID()));
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->list_local_files_rec(file_path, file_del_op, dir_del_op));
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->is_exist_file(tmp_file, 0, is_exist));
  ASSERT_EQ(false, is_exist);
  ObArray<MacroBlockId> tmp_files;
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->list_tmp_file(tmp_files));
  ASSERT_EQ(0, tmp_files.count());

  // test2: list private tablet data macro
  MacroBlockId tablet_data_macro;
  int64_t tablet_id = 4;
  int64_t server_id = 5;
  int64_t seq_id = 6;
  tablet_data_macro.set_id_mode(static_cast<uint64_t>(ObMacroBlockIdMode::ID_MODE_SHARE));
  tablet_data_macro.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::PRIVATE_DATA_MACRO));
  tablet_data_macro.set_second_id(tablet_id);
  tablet_data_macro.set_third_id(server_id);
  tablet_data_macro.set_macro_transfer_seq(0); // transfer_seq
  tablet_data_macro.set_tenant_seq(seq_id);  //tenant_seq
  write_object_handle.reset();
  ASSERT_EQ(OB_SUCCESS, write_object_handle.set_macro_block_id(tablet_data_macro));
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->write_file(write_info, write_object_handle));
  MacroBlockId tablet_meta_macro;
  tablet_id = 4;
  server_id = 5;
  seq_id = 7;
  tablet_meta_macro.set_id_mode(static_cast<uint64_t>(ObMacroBlockIdMode::ID_MODE_SHARE));
  tablet_meta_macro.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::PRIVATE_META_MACRO));
  tablet_meta_macro.set_second_id(tablet_id);
  tablet_meta_macro.set_third_id(server_id);
  tablet_meta_macro.set_macro_transfer_seq(0); // transfer_seq
  tablet_meta_macro.set_tenant_seq(seq_id);  //tenant_seq
  write_object_handle.reset();
  ASSERT_EQ(OB_SUCCESS, write_object_handle.set_macro_block_id(tablet_meta_macro));
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->write_file(write_info, write_object_handle));

  file_path[0] = '\0';
  ASSERT_EQ(OB_SUCCESS, OB_DIR_MGR.get_local_tablet_data_dir(file_path, sizeof(file_path), MTL_ID(), MTL_EPOCH_ID()));
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->list_local_files_rec(file_path, file_del_op, dir_del_op));
  is_exist = true;
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->is_exist_file(tablet_data_macro, 0, is_exist));
  ASSERT_EQ(false, is_exist);
  is_exist = true;
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->is_exist_file(tablet_meta_macro, 0, is_exist));
  ASSERT_EQ(false, is_exist);

  ObArray<int64_t> tablet_ids;
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->list_tablet_data_dir(tablet_ids));
  ASSERT_EQ(0, tablet_ids.count());
}

TEST_F(TestFileManager, test_delete_tmp_seq_files)
{
  int ret = OB_SUCCESS;
  ObTenantFileManager* tenant_file_mgr = MTL(ObTenantFileManager*);
  ASSERT_NE(nullptr, tenant_file_mgr);
  int64_t seq_id = 100;
  char disk_space_meta_path[ObBaseFileManager::OB_MAX_FILE_PATH_LENGTH] = {0};
  ASSERT_EQ(OB_SUCCESS, databuff_printf(disk_space_meta_path, ObBaseFileManager::OB_MAX_FILE_PATH_LENGTH, "%s/%lu_%ld/%s%s%ld",
                        OB_DIR_MGR.get_local_cache_root_dir(), MTL_ID(), MTL_EPOCH_ID(),
                        get_storage_objet_type_str(ObStorageObjectType::TENANT_DISK_SPACE_META), DEFAULT_TMP_STR, seq_id));
  ObIOFd fd;
  const int64_t write_io_size = 4096; // 4KB
  char write_buf[write_io_size] = { 0 };
  memset(write_buf, 'a', write_io_size);
  int64_t written_size = 0;
  ASSERT_EQ(OB_SUCCESS, ObIODeviceLocalFileOp::open(disk_space_meta_path, O_RDWR | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR, fd));
  ASSERT_EQ(OB_SUCCESS, ObIODeviceLocalFileOp::write(fd, write_buf, write_io_size, written_size));
  ObIODFileStat statbuf;
  ASSERT_EQ(OB_SUCCESS, ObIODeviceLocalFileOp::stat(disk_space_meta_path, statbuf));
  volatile bool is_stop = false;
  ObDirCalcSizeOp del_tmp_seq_file_op(is_stop);
  const int64_t del_security_time_s = 5;  // 5s
  ASSERT_EQ(OB_SUCCESS, del_tmp_seq_file_op.set_tmp_seq_file_del_secy_time_s(del_security_time_s));
  ob_usleep(2 * 1000 * 1000L); // sleep 2s
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->calibrate_disk_space_task_.delete_tmp_seq_files(del_tmp_seq_file_op));
  ASSERT_EQ(OB_SUCCESS, ObIODeviceLocalFileOp::stat(disk_space_meta_path, statbuf));
  ob_usleep(5 * 1000 * 1000L); // sleep 5s
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->calibrate_disk_space_task_.delete_tmp_seq_files(del_tmp_seq_file_op));
  ASSERT_EQ(OB_NO_SUCH_FILE_OR_DIRECTORY, ObIODeviceLocalFileOp::stat(disk_space_meta_path, statbuf));
}

TEST_F(TestFileManager, test_gc_local_major_data)
{
  int ret = OB_SUCCESS;
  ObTenantFileManager* tenant_file_mgr = MTL(ObTenantFileManager*);
  ASSERT_NE(nullptr, tenant_file_mgr);
  MacroBlockId shared_data_macro;
  int64_t tablet_id = 100;
  int64_t seq_id = 101;
  shared_data_macro.set_id_mode(static_cast<uint64_t>(ObMacroBlockIdMode::ID_MODE_SHARE));
  shared_data_macro.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::SHARED_MAJOR_DATA_MACRO));
  shared_data_macro.set_second_id(tablet_id);
  shared_data_macro.set_third_id(seq_id);

  char shared_data_macro_path[ObBaseFileManager::OB_MAX_FILE_PATH_LENGTH] = {0};
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(true/*is_local_cache*/, shared_data_macro_path, ObBaseFileManager::OB_MAX_FILE_PATH_LENGTH, shared_data_macro));
  ObIOFd fd;
  const int64_t write_io_size = 4096; // 4KB
  char write_buf[write_io_size] = { 0 };
  memset(write_buf, 'a', write_io_size);
  int64_t written_size = 0;
  ASSERT_EQ(OB_SUCCESS, ObIODeviceLocalFileOp::open(shared_data_macro_path, O_RDWR | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR, fd));
  ASSERT_EQ(OB_SUCCESS, ObIODeviceLocalFileOp::write(fd, write_buf, write_io_size, written_size));
  ObIODFileStat statbuf;
  ASSERT_EQ(OB_SUCCESS, ObIODeviceLocalFileOp::stat(shared_data_macro_path, statbuf));
  ob_usleep(2 * 1000 * 1000L); // sleep 2s
  int64_t del_time_s = ObTimeUtility::current_time_s();
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->delete_local_major_data_dir(del_time_s));
  ASSERT_EQ(OB_NO_SUCH_FILE_OR_DIRECTORY, ObIODeviceLocalFileOp::stat(shared_data_macro_path, statbuf));
}

TEST_F(TestFileManager, test_deleted_file_operator)
{
  int ret = OB_SUCCESS;
  ObTenantFileManager* tenant_file_mgr = MTL(ObTenantFileManager*);
  ASSERT_NE(nullptr, tenant_file_mgr);
  ObTenantDiskSpaceManager* tenant_disk_space_mgr = MTL(ObTenantDiskSpaceManager*);
  ASSERT_NE(nullptr, tenant_disk_space_mgr);
  MacroBlockId tmp_file;
  int64_t tmp_file_id = 10;
  int64_t segment_file_id = 11;
  tmp_file.set_id_mode(static_cast<uint64_t>(ObMacroBlockIdMode::ID_MODE_SHARE));
  tmp_file.set_storage_object_type(static_cast<uint64_t>(ObStorageObjectType::TMP_FILE));
  tmp_file.set_second_id(tmp_file_id);
  tmp_file.set_third_id(segment_file_id);
  ObStorageObjectHandle write_object_handle;
  ASSERT_EQ(OB_SUCCESS, write_object_handle.set_macro_block_id(tmp_file));
  const int64_t write_io_size = 8 * 1024; // 8KB
  char write_buf[write_io_size] = { 0 };
  memset(write_buf, 'a', write_io_size);
  ObStorageObjectWriteInfo write_info;
  write_info.io_desc_.set_resource_group_id(0);
  write_info.io_desc_.set_wait_event(1);
  write_info.io_desc_.set_unsealed();
  write_info.buffer_ = write_buf;
  write_info.offset_ = 0;
  write_info.size_ = write_io_size;
  write_info.io_timeout_ms_ = DEFAULT_IO_WAIT_TIME_MS;
  write_info.mtl_tenant_id_ = MTL_ID();

  // step 1: write tmp_file
  ASSERT_EQ(OB_SUCCESS, OB_DIR_MGR.create_tmp_file_dir(MTL_ID(), MTL_EPOCH_ID(), tmp_file_id));
  int64_t expected_disk_size = write_io_size;
  char dir_path[ObBaseFileManager::OB_MAX_FILE_PATH_LENGTH] = {0};
  ObIODFileStat statbuf;
  ASSERT_EQ(OB_SUCCESS, OB_DIR_MGR.get_local_tmp_file_dir(dir_path, sizeof(dir_path),
            MTL_ID(), MTL_EPOCH_ID(), tmp_file_id));
  ASSERT_EQ(OB_SUCCESS, ObIODeviceLocalFileOp::stat(dir_path, statbuf));
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->append_file(write_info, write_object_handle));
  int64_t tmp_file_write_cache_alloc_size = tenant_disk_space_mgr->get_tmp_file_write_cache_alloc_size();
  bool is_exist = false;
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->is_exist_local_file(tmp_file, 0/*ls_epoch_id*/, is_exist));
  ASSERT_TRUE(is_exist);

  // step 2: delete tmp_file when pause_gc
  tenant_file_mgr->set_tmp_file_cache_pause_gc();
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->delete_local_file(tmp_file));
  // old path has been rename, old path is not exist
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->is_exist_local_file(tmp_file, 0/*ls_epoch_id*/, is_exist));
  ASSERT_FALSE(is_exist);
  char deleted_file_path[ObBaseFileManager::OB_MAX_FILE_PATH_LENGTH] = {0};
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->macro_block_id_to_path(true/*is_local_cache*/, deleted_file_path, ObBaseFileManager::OB_MAX_FILE_PATH_LENGTH, tmp_file, 0/*ls_epoch_id*/, false/*is_atomic_tmp*/, OB_INVALID_ID/*seq*/, true/*is_deleted_file*/));
  // old path has been rename, new path is exist
  ASSERT_EQ(OB_SUCCESS, ObIODeviceLocalFileOp::exist(deleted_file_path, is_exist));
  ASSERT_TRUE(is_exist);
  // delete tmp_file when pause_gc, rename file path, so tmp_file_write_cache_alloc_size does not changed
  ASSERT_EQ(tmp_file_write_cache_alloc_size, tenant_disk_space_mgr->get_tmp_file_write_cache_alloc_size());

  // step 3: delete .deleted file
  tenant_file_mgr->set_tmp_file_cache_allow_gc();
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->calibrate_disk_space_task_.rm_logical_deleted_file());
  // after calibrate, rm_logical_deleted_file, so renamed file path is not exist
  ASSERT_EQ(OB_SUCCESS, ObIODeviceLocalFileOp::exist(deleted_file_path, is_exist));
  ASSERT_FALSE(is_exist);
  // delete .deleted file, so tmp_file_write_cache_alloc_size reduce
  ASSERT_EQ(tmp_file_write_cache_alloc_size - write_io_size - statbuf.size_, tenant_disk_space_mgr->get_tmp_file_write_cache_alloc_size());

  // step 4: test rename not exist file
  ASSERT_EQ(OB_SUCCESS, tenant_file_mgr->logical_delete_local_file(tmp_file, 0/*ls_epoch_id*/));
}

TEST_F(TestFileManager, test_delete_tenant)
{
  // delete local tenant dir file
  ASSERT_EQ(OB_SUCCESS, OB_SERVER_FILE_MGR.delete_local_tenant_dir(MTL_ID(), MTL_EPOCH_ID()));
  // delete remote tenant dir file
  ASSERT_EQ(OB_SUCCESS, OB_SERVER_FILE_MGR.delete_remote_tenant_dir(MTL_ID(), MTL_EPOCH_ID()));
}


} // namespace storage
} // namespace oceanbase

int main(int argc, char **argv)
{
  int ret = 0;
  system("rm -f ./test_file_manager.log*");
  OB_LOGGER.set_file_name("test_file_manager.log", true);
  OB_LOGGER.set_log_level("INFO");
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}