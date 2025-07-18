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

#ifndef SRC_LIBRARY_SRC_LIB_RESTORE_OB_BACKUP_IO_ADAPTER_H_
#define SRC_LIBRARY_SRC_LIB_RESTORE_OB_BACKUP_IO_ADAPTER_H_

#include "common/storage/ob_io_device.h"
#include "common/storage/ob_device_common.h"
#include "lib/container/ob_array.h"
#include "share/backup/ob_backup_struct.h"

namespace oceanbase
{
namespace common
{
int switch_cos_to_s3(ObIAllocator &allocator, const common::ObString &src_uri, common::ObString &dest_uri);

class ObBackupIoAdapter
{
public:
  explicit ObBackupIoAdapter() {}
  virtual ~ObBackupIoAdapter() {}

  static int open_with_access_type(
      ObIODevice *&device_handle, ObIOFd &fd,
      const common::ObObjectStorageInfo *storage_info,
      const common::ObString &uri,
      ObStorageAccessType access_type,
      const common::ObStorageIdMod &storage_id_mod);
  static int get_and_init_device(
      ObIODevice *&dev_handle,
      const common::ObObjectStorageInfo *storage_info,
      const common::ObString &storage_type_prefix,
      const common::ObStorageIdMod &storage_id_mod);
  static int close_device_and_fd(ObIODevice*& device_handle, ObIOFd &fd);
  static int set_access_type(ObIODOpts *opts, bool is_appender, int max_opt_num);
  static int set_open_mode(ObIODOpts *opts, bool lock_mode, bool new_file, int max_opt_num);
  static int set_append_strategy(ObIODOpts *opts, bool is_data_file, int64_t epoch, int max_opt_num);

  static int is_exist(
      const common::ObString &uri, const common::ObObjectStorageInfo *storage_info, bool &exist);
  //TODO (@shifangdan.sfd): refine repeated logics between normal interfaces and adaptive ones
  static int adaptively_is_exist(
      const common::ObString &uri, const common::ObObjectStorageInfo *storage_info, bool &exist);
  static int is_tagging(
      const common::ObString &uri, const common::ObObjectStorageInfo *storage_info, bool &is_tagging);
  static int get_file_length(const common::ObString &uri,
      const common::ObObjectStorageInfo *storage_info, int64_t &file_length);
  static int get_file_size(ObIODevice *device_handle, const ObIOFd &fd, int64_t &file_length);
  static int adaptively_get_file_length(const common::ObString &uri,
      const common::ObObjectStorageInfo *storage_info, int64_t &file_length);
  static int del_file(
      const common::ObString &uri, const common::ObObjectStorageInfo *storage_info);
  static int adaptively_del_file(
      const common::ObString &uri, const common::ObObjectStorageInfo *storage_info);
  static int get_file_modify_time(const common::ObString &uri,
      const common::ObObjectStorageInfo *storage_info, int64_t &modify_time_s);
  /**
   * Deletes a list of specified objects (files_to_delete).
   * If some objects are deleted successfully and others fail, the function
   * returns OB_SUCCESS. It uses the failed_files_idx to return the indices
   * of the objects that failed to delete.
   *
   * It's important to ensure that all the objects provided for deletion are located
   * on the same destination. If the destination is object storage, all objects must be
   * within the same bucket.
   *
   * Due to the absence of a batch tagging interface, if delete mode 'tagging' is set
   * when initiating the utility, it will switch to a looped tagging operation.
   *
   * As NFS does not offer a batch deleting interface, and GCS's batch delete interface
   * is not compatible with the S3 protocol, GCS and NFS will revert to looped delete operations.
   *
   * If it switches to looped operations, upon the failure of any deletion request,
   * the function attempts to record that object along with all remaining unprocessed objects
   * as failed_files. After successfully recording failures, it returns OB_SUCCESS.
   *
   * @param files_to_delete: The objects intended for deletion.
   * @param failed_files_idx: The index list where indices of failed deletions will be returned.
   */
  static int batch_del_files(
      const common::ObObjectStorageInfo *storage_info,
      const ObIArray<ObString> &files_to_delete,
      ObIArray<int64_t> &failed_files_idx);

  static int mkdir(
      const common::ObString &uri, const common::ObObjectStorageInfo *storage_info);
  static int mk_parent_dir(
      const common::ObString &uri, const common::ObObjectStorageInfo *storage_info);
  static int is_empty_directory(
      const common::ObString &uri,
      const common::ObObjectStorageInfo *storage_info,
      bool &is_empty_directory);
  static int is_directory(const common::ObString &uri,
                          const common::ObObjectStorageInfo *storage_info,
                          bool &is_directory);
  static int list_files(
      const common::ObString &dir_path,
      const common::ObObjectStorageInfo *storage_info,
      common::ObBaseDirEntryOperator &op);
  static int adaptively_list_files(
      const common::ObString &dir_path,
      const common::ObObjectStorageInfo *storage_info,
      common::ObBaseDirEntryOperator &op);
  static int list_directories(
      const common::ObString &uri,
      const common::ObObjectStorageInfo *storage_info,
      common::ObBaseDirEntryOperator &op);
  // This function handles the deletion of directories specified by the 'uri' parameter. Its behavior varies depending
  // on the 'recursive' flag and the underlying storage mechanism (e.g., NFS or object storage).
  //
  // For NFS storage, when 'recursive' is false (the default),
  // the function attempts to delete the specified empty directory,
  // which matches the behavior of the UNIX command 'rm -d uri'.
  // If 'recursive' is true,
  // it resembles 'rm -rf', deleting the directory and all its contents indiscriminately.
  //
  // For object storage, if 'recursive' is false, the function returns success immediately
  // since there is no "directory" to check for content presence.
  // When 'recursive' is true,
  // the function interprets the 'uri' as a prefix and deletes all objs under it, treating it as a "directory".
  //
  // Note: Using the 'recursive' option, especially with object storage, initiates numerous list and delete operations,
  //       potentially leading to long execution times.
  //       Users should be aware of the performance implications and proceed with caution
  //       when choosing to recursively delete "directories" and their contents in object storage environments.
  static int del_dir(const common::ObString &uri,
      const common::ObObjectStorageInfo *storage_info, const bool recursive = false);

  static int write_single_file(
      const common::ObString &uri,
      const common::ObObjectStorageInfo *storage_info,
      const char *buf, const int64_t size,
      const common::ObStorageIdMod &storage_id_mod);
  static int pwrite(
      const common::ObString &uri,
      const common::ObObjectStorageInfo *storage_info,
      const char *buf, const int64_t offset, const int64_t size,
      const common::ObStorageAccessType access_type,
      int64_t &write_size,
      const bool is_can_seal,
      const common::ObStorageIdMod &storage_id_mod);

  static int seal_file(
      const common::ObString &uri,
      const common::ObObjectStorageInfo *storage_info,
      const common::ObStorageIdMod &storage_id_mod);
  static int pwrite(
      common::ObIODevice &device_handle, common::ObIOFd &fd,
      const char *buf, const int64_t offset, const int64_t size,
      int64_t &write_size,
      const bool is_can_seal);

  static int read_single_file(
      const common::ObString &uri,
      const common::ObObjectStorageInfo *storage_info,
      char *buf, const int64_t buf_size, int64_t &read_size,
      const common::ObStorageIdMod &storage_id_mod);
  static int adaptively_read_single_file(
      const common::ObString &uri,
      const common::ObObjectStorageInfo *storage_info,
      char *buf, const int64_t buf_size, int64_t &read_size,
      const common::ObStorageIdMod &storage_id_mod);
  static int read_single_text_file(
      const common::ObString &uri,
      const common::ObObjectStorageInfo *storage_info,
      char *buf, const int64_t buf_size,
      const common::ObStorageIdMod &storage_id_mod);
  static int adaptively_read_single_text_file(
      const common::ObString &uri,
      const common::ObObjectStorageInfo *storage_info,
      char *buf, const int64_t buf_size,
      const common::ObStorageIdMod &storage_id_mod);
  static int read_part_file(
      const common::ObString &uri,
      const common::ObObjectStorageInfo *storage_info,
      char *buf, const int64_t buf_size, const int64_t offset,
      int64_t &read_size,
      const common::ObStorageIdMod &storage_id_mod);
  static int adaptively_read_part_file(
      const common::ObString &uri,
      const common::ObObjectStorageInfo *storage_info,
      char *buf, const int64_t buf_size, const int64_t offset,
      int64_t &read_size,
      const common::ObStorageIdMod &storage_id_mod);
  static int pread(
      const common::ObString &uri,
      const common::ObObjectStorageInfo *storage_info,
      char *buf, const int64_t buf_size, const int64_t offset,
      int64_t &read_size,
      const common::ObStorageIdMod &storage_id_mod);
  // design for concurrent read one object.
  // @device_handle: ObObjectDevice obtained by open_with_access_type.
  // @fd: ObIOFd obtained by open_with_access_type.
  // @buf: buf for store the data to read.
  // @offset: offset of the data to read.
  // @size: size of the data to read.
  // @io_handle: user exploit this io_handle to wait this async read io finish
  static int async_pread(
      common::ObIODevice &device_handle,
      common::ObIOFd &fd,
      char *buf, const int64_t offset, const int64_t size,
      common::ObIOHandle &io_handle,
      const uint64_t sys_module_id=OB_INVALID_ID);

  static int async_upload_data(
      common::ObIODevice &device_handle,
      common::ObIOFd &fd,
      const char *buf,
      const int64_t offset,
      const int64_t size,
      common::ObIOHandle &io_handle,
      const uint64_t sys_module_id=OB_INVALID_ID);
  static int complete(common::ObIODevice &device_handle, common::ObIOFd &fd);
  static int abort(common::ObIODevice &device_handle, common::ObIOFd &fd);
  static int del_unmerged_parts(
      const common::ObString &uri, const common::ObObjectStorageInfo *storage_info);

  static int delete_tmp_files(
      const common::ObString &uri,
      const common::ObObjectStorageInfo *storage_info);

  static uint64_t get_tenant_id();

  static int basic_init_read_info(
      common::ObIODevice &device_handle,
      common::ObIOFd &fd,
      char *buf,
      const int64_t offset,
      const int64_t size,
      const uint64_t sys_module_id,
      common::ObIOInfo &io_info);

  static int async_pread_with_io_info(
      const common::ObIOInfo &io_info,
      common::ObIOHandle &io_handle);

private:
  static int async_io_manager_read(
      char *buf, const int64_t offset, const int64_t size,
      const common::ObIOFd &fd,
      common::ObIOHandle &io_handle,
      const uint64_t sys_module_id=OB_INVALID_ID);
  static int io_manager_read(
      char *buf, const int64_t offset, const int64_t size,
      const common::ObIOFd &fd,
      int64_t &read_size);
  static int io_manager_write(
      const char *buf, const int64_t offset, const int64_t size,
      const common::ObIOFd &fd,
      int64_t &write_size);
  
  // When utilizing the buffered multipart writer, data is cached at the lower level,
  // and during the completion phase, there might still be a portion of the data pending upload.
  // A separate invocation of 'async_io_manager_upload' is required to upload this remaining data.
  // In this particular call,
  // the variables 'buf' and 'offset' do not carry practical significance.
  // Therefore, the parameter 'is_complete_mode'
  // is used to indicate if the function is operating in this specific scenario.
  static int async_io_manager_upload(
      const char *buf,
      const int64_t offset,
      const int64_t size,
      const ObIOFd &fd,
      ObIOHandle &io_handle,
      const bool is_complete_mode,
      const uint64_t sys_module_id=OB_INVALID_ID);
  
  DISALLOW_COPY_AND_ASSIGN(ObBackupIoAdapter);
};

#define DECLARE_PARAMS_(N, ...) CONCAT(DECLARE_PARAMS_, N)(__VA_ARGS__)
#define DECLARE_PARAMS(...) DECLARE_PARAMS_(ARGS_NUM(__VA_ARGS__), __VA_ARGS__)

#define DECLARE_PARAM(type, name) type name
#define DECLARE_PARAMS_0()
#define DECLARE_PARAMS_2(type, name)           DECLARE_PARAM(type, name)
#define DECLARE_PARAMS_4(type, name, args...)  DECLARE_PARAM(type, name),  DECLARE_PARAMS_2(args)
#define DECLARE_PARAMS_6(type, name, args...)  DECLARE_PARAM(type, name),  DECLARE_PARAMS_4(args)
#define DECLARE_PARAMS_8(type, name, args...)  DECLARE_PARAM(type, name),  DECLARE_PARAMS_6(args)
#define DECLARE_PARAMS_10(type, name, args...) DECLARE_PARAM(type, name), DECLARE_PARAMS_8(args)
#define DECLARE_PARAMS_12(type, name, args...) DECLARE_PARAM(type, name), DECLARE_PARAMS_10(args)
#define DECLARE_PARAMS_14(type, name, args...) DECLARE_PARAM(type, name), DECLARE_PARAMS_12(args)
#define DECLARE_PARAMS_16(type, name, args...) DECLARE_PARAM(type, name), DECLARE_PARAMS_14(args)
#define DECLARE_PARAMS_18(type, name, args...) DECLARE_PARAM(type, name), DECLARE_PARAMS_16(args)

#define SHIFT_PARAMS_(N, ...) CONCAT(SHIFT_PARAMS_, N)(__VA_ARGS__)
#define SHIFT_PARAMS(...) SHIFT_PARAMS_(ARGS_NUM(__VA_ARGS__), __VA_ARGS__)

#define SHIFT_PARAM(type, name) name
#define SHIFT_PARAMS_0()
#define SHIFT_PARAMS_2(type, name)           name
#define SHIFT_PARAMS_4(type, name, args...)  SHIFT_PARAM(type, name), SHIFT_PARAMS_2(args)
#define SHIFT_PARAMS_6(type, name, args...)  SHIFT_PARAM(type, name), SHIFT_PARAMS_4(args)
#define SHIFT_PARAMS_8(type, name, args...)  SHIFT_PARAM(type, name), SHIFT_PARAMS_6(args)
#define SHIFT_PARAMS_10(type, name, args...) SHIFT_PARAM(type, name), SHIFT_PARAMS_8(args)
#define SHIFT_PARAMS_12(type, name, args...) SHIFT_PARAM(type, name), SHIFT_PARAMS_10(args)
#define SHIFT_PARAMS_14(type, name, args...) SHIFT_PARAM(type, name), SHIFT_PARAMS_12(args)
#define SHIFT_PARAMS_16(type, name, args...) SHIFT_PARAM(type, name), SHIFT_PARAMS_14(args)
#define SHIFT_PARAMS_18(type, name, args...) SHIFT_PARAM(type, name), SHIFT_PARAMS_16(args)

#define EXTERNAL_IO_ADAPTER_FUNCTION(fun_name, uri_type, uri, ...) \
  static int fun_name(uri_type uri, DECLARE_PARAMS(__VA_ARGS__))                      \
  {                                                                                   \
    int ret = OB_SUCCESS;                                                             \
    ObArenaAllocator allocator;                                                       \
    ObString new_uri;                                                                 \
    if (OB_FAIL(switch_cos_to_s3(allocator, uri, new_uri))) {         \
      OB_LOG(WARN, "fail to switch cos to s3", K(ret), K(uri));       \
    } else if (OB_FAIL(ObBackupIoAdapter::fun_name(new_uri, SHIFT_PARAMS(__VA_ARGS__)))) { \
      OB_LOG(WARN, "fail to get and init device", K(ret)); \
    }                                                                                 \
    return ret;                                                                       \
  }                                                                                   \


// After removing the cos c sdk, since some modules (e.g., external file) still have the need to access the driver using cos://,
// this class is specifically provided to convert cos:// to s3://.
class ObExternalIoAdapter: public ObBackupIoAdapter
{
public:
  explicit ObExternalIoAdapter() {}
  virtual ~ObExternalIoAdapter() {}


  static int open_with_access_type(ObIODevice *&device_handle, ObIOFd &fd,
                                   const common::ObObjectStorageInfo *storage_info,
                                   const common::ObString &uri,
                                   ObStorageAccessType access_type,
                                   const common::ObStorageIdMod &storage_id_mod)
  {
    int ret = OB_SUCCESS;
    ObArenaAllocator allocator;
    ObString new_uri;
    if (OB_FAIL(switch_cos_to_s3(allocator, uri, new_uri))) {
      OB_LOG(WARN, "fail to switch cos to s3", K(ret), K(uri));
    } else if (OB_FAIL(ObBackupIoAdapter::open_with_access_type(device_handle, fd, storage_info, new_uri, access_type, storage_id_mod))) {
      OB_LOG(WARN, "fail to open with access type", K(ret), KPC(storage_info), K(new_uri), K(access_type), K(storage_id_mod));
    }
    return ret;
  }
  static int get_and_init_device(ObIODevice *&device_handle,
                                 const common::ObObjectStorageInfo *storage_info,
                                 const common::ObString &storage_type_prefix,
                                 const common::ObStorageIdMod &storage_id_mod)
  {
    int ret = OB_SUCCESS;
    ObArenaAllocator allocator;
    ObString new_uri;
    if (OB_FAIL(switch_cos_to_s3(allocator, storage_type_prefix, new_uri))) {
      OB_LOG(WARN, "fail to switch cos to s3", K(ret), K(storage_type_prefix));
    } else if (OB_FAIL(ObBackupIoAdapter::get_and_init_device(device_handle, storage_info, new_uri, storage_id_mod))) {
      OB_LOG(WARN, "fail to get and init device", K(ret), KPC(storage_info), K(new_uri), K(storage_id_mod));
    }
    return ret;
  }
  EXTERNAL_IO_ADAPTER_FUNCTION(is_exist, const common::ObString &, uri, const common::ObObjectStorageInfo *, storage_info, bool &, exist)
  EXTERNAL_IO_ADAPTER_FUNCTION(adaptively_is_exist, const common::ObString &, uri,
                                                    const common::ObObjectStorageInfo *, storage_info,
                                                    bool &, exist)
  EXTERNAL_IO_ADAPTER_FUNCTION(is_tagging, const common::ObString &, uri,
                                           const common::ObObjectStorageInfo *, storage_info,
                                           bool &, is_tagging)
  EXTERNAL_IO_ADAPTER_FUNCTION(get_file_length, const common::ObString &, uri,
                                                const common::ObObjectStorageInfo *, storage_info,
                                                int64_t &, file_length)
  EXTERNAL_IO_ADAPTER_FUNCTION(adaptively_get_file_length, const common::ObString &, uri,
                                                           const common::ObObjectStorageInfo *, storage_info,
                                                           int64_t &, file_length)
  EXTERNAL_IO_ADAPTER_FUNCTION(del_file, const common::ObString &, uri,
                                         const common::ObObjectStorageInfo *, storage_info)
  EXTERNAL_IO_ADAPTER_FUNCTION(adaptively_del_file, const common::ObString &, uri,
                                                    const common::ObObjectStorageInfo *, storage_info)
  static int batch_del_files(const common::ObObjectStorageInfo *storage_info, const ObIArray<ObString> &files_to_delete, ObIArray<int64_t> &failed_files_idx)
  {
    return OB_NOT_SUPPORTED;
  }

  EXTERNAL_IO_ADAPTER_FUNCTION(mkdir, const common::ObString &, uri,
                                      const common::ObObjectStorageInfo *, storage_info)
  EXTERNAL_IO_ADAPTER_FUNCTION(mk_parent_dir, const common::ObString &, uri,
                                              const common::ObObjectStorageInfo *, storage_info)
  EXTERNAL_IO_ADAPTER_FUNCTION(is_empty_directory, const common::ObString &, uri,
                                                   const common::ObObjectStorageInfo *, storage_info,
                                                   bool &, is_empty_directory)
  EXTERNAL_IO_ADAPTER_FUNCTION(is_directory, const common::ObString &, uri,
                                             const common::ObObjectStorageInfo *, storage_info,
                                             bool &, is_directory)
  EXTERNAL_IO_ADAPTER_FUNCTION(list_files, const common::ObString &, dir_path,
                                            const common::ObObjectStorageInfo *, storage_info,
                                            common::ObBaseDirEntryOperator &, op)
  EXTERNAL_IO_ADAPTER_FUNCTION(adaptively_list_files, const common::ObString &, dir_path,
                                                      const common::ObObjectStorageInfo *, storage_info,
                                                      common::ObBaseDirEntryOperator &, op)
  EXTERNAL_IO_ADAPTER_FUNCTION(list_directories, const common::ObString &, uri,
                                                 const common::ObObjectStorageInfo *, storage_info,
                                                 common::ObBaseDirEntryOperator &, op)
  static int del_dir(const common::ObString & uri, const common::ObObjectStorageInfo * storage_info, const bool recursive = false)
  {
    int ret = OB_SUCCESS;
    ObArenaAllocator allocator;
    ObString new_uri;
    if (OB_FAIL(switch_cos_to_s3(allocator, uri, new_uri))) {
      OB_LOG(WARN, "fail to switch cos to s3", K(ret));
    } else if (OB_FAIL(ObBackupIoAdapter::del_dir(uri, storage_info, recursive))) {
      OB_LOG(WARN, "fail to get and init device", K(ret), KPC(storage_info), K(recursive));
    }
    return ret;
  }

  EXTERNAL_IO_ADAPTER_FUNCTION(write_single_file, const common::ObString &, uri,
                                                  const common::ObObjectStorageInfo *, storage_info,
                                                  const char *, buf,
                                                  const int64_t , size,
                                                  const common::ObStorageIdMod &, storage_id_mod)
  EXTERNAL_IO_ADAPTER_FUNCTION(pwrite, const common::ObString &, uri,
                                       const common::ObObjectStorageInfo *, storage_info,
                                       const char *, buf,
                                       const int64_t, offset,
                                       const int64_t, size,
                                       const common::ObStorageAccessType, access_type,
                                       int64_t &, write_size,
                                       const bool, is_can_seal,
                                       const common::ObStorageIdMod &, storage_id_mod)

  EXTERNAL_IO_ADAPTER_FUNCTION(seal_file, const common::ObString &, uri,
                                          const share::ObBackupStorageInfo *, storage_info,
                                          const common::ObStorageIdMod &, storage_id_mod)
  static int pwrite(common::ObIODevice &device_handle,
                    common::ObIOFd &fd,
                    const char *buf,
                    const int64_t offset,
                    const int64_t size,
                    int64_t &write_size,
                    const bool is_can_seal)
  {
    return ObBackupIoAdapter::pwrite(device_handle, fd, buf, offset, size, write_size, is_can_seal);
  }

  EXTERNAL_IO_ADAPTER_FUNCTION(read_single_file, const common::ObString &,uri,
                                                 const common::ObObjectStorageInfo *, storage_info,
                                                 char *, buf,
                                                 const int64_t, buf_size,
                                                 int64_t &, read_size,
                                                 const common::ObStorageIdMod &, storage_id_mod)
  EXTERNAL_IO_ADAPTER_FUNCTION(adaptively_read_single_file, const common::ObString &, uri,
                                                            const common::ObObjectStorageInfo *, storage_info,
                                                            char *, buf,
                                                            const int64_t, buf_size,
                                                            int64_t &, read_size,
                                                            const common::ObStorageIdMod &, storage_id_mod)
  EXTERNAL_IO_ADAPTER_FUNCTION(read_single_text_file, const common::ObString &, uri,
                                                      const common::ObObjectStorageInfo *, storage_info,
                                                      char *, buf,
                                                      const int64_t, buf_size,
                                                      const common::ObStorageIdMod &, storage_id_mod)
  EXTERNAL_IO_ADAPTER_FUNCTION(adaptively_read_single_text_file, const common::ObString &,uri,
                                                                 const common::ObObjectStorageInfo *, storage_info,
                                                                 char *, buf,
                                                                 const int64_t, buf_size,
                                                                 const common::ObStorageIdMod &, storage_id_mod)
  EXTERNAL_IO_ADAPTER_FUNCTION(read_part_file, const common::ObString &,uri,
                                               const common::ObObjectStorageInfo *, storage_info,
                                               char *, buf,
                                               const int64_t, buf_size,
                                               const int64_t, offset,
                                               int64_t &, read_size,
                                               const common::ObStorageIdMod &, storage_id_mod)
  EXTERNAL_IO_ADAPTER_FUNCTION(adaptively_read_part_file, const common::ObString &,uri,
                                                          const common::ObObjectStorageInfo *, storage_info,
                                                          char *, buf,
                                                          const int64_t, buf_size,
                                                          const int64_t, offset,
                                                          int64_t &, read_size,
                                                          const common::ObStorageIdMod &, storage_id_mod)
  EXTERNAL_IO_ADAPTER_FUNCTION(pread, const common::ObString &, uri,
                                      const common::ObObjectStorageInfo *, storage_info,
                                      char *, buf,
                                      const int64_t, buf_size,
                                      const int64_t, offset,
                                      int64_t &, read_size,
                                      const common::ObStorageIdMod &, storage_id_mod)
  EXTERNAL_IO_ADAPTER_FUNCTION(del_unmerged_parts, const common::ObString &, uri, const common::ObObjectStorageInfo *, storage_info)
  EXTERNAL_IO_ADAPTER_FUNCTION(delete_tmp_files, const common::ObString &, uri, const common::ObObjectStorageInfo *, storage_info)
private:
  DISALLOW_COPY_AND_ASSIGN(ObExternalIoAdapter);
};

class ObCntFileListOp : public ObBaseDirEntryOperator
{
public:
  ObCntFileListOp() : file_count_(0) {}
  ~ObCntFileListOp() {}
  int func(const dirent *entry) 
  {
    UNUSED(entry);
    file_count_++;
    return OB_SUCCESS;
  }
  int64_t get_file_count() {return file_count_;}
private:
  int64_t file_count_;
};

class ObFileListArrayOp : public ObBaseDirEntryOperator
{
public: 
  ObFileListArrayOp(common::ObIArray <common::ObString>& name_array, common::ObIAllocator& array_allocator)
    : name_array_(name_array), allocator_(array_allocator) {}
  ~ObFileListArrayOp() {}
  int func(const dirent *entry) ;

private:
  common::ObIArray <common::ObString>& name_array_;
  common::ObIAllocator& allocator_;
};

class ObDirPrefixEntryNameFilter : public ObBaseDirEntryOperator
{
public:
  ObDirPrefixEntryNameFilter(common::ObIArray<ObIODirentEntry> &d_entrys)
      : is_inited_(false),
        d_entrys_(d_entrys)
  {
    filter_str_[0] = '\0';
  }
  virtual ~ObDirPrefixEntryNameFilter() = default;
  int init(const char *filter_str, const int32_t filter_str_len);
  virtual int func(const dirent *entry) override;
private:
  bool is_inited_;
  char filter_str_[common::MAX_PATH_SIZE];
  common::ObIArray<ObIODirentEntry> &d_entrys_;
private:
  DISALLOW_COPY_AND_ASSIGN(ObDirPrefixEntryNameFilter);
};

class ObDirPrefixLSIDFilter final : public ObBaseDirEntryOperator
{
  public:
  ObDirPrefixLSIDFilter(common::ObIArray<share::ObLSID> &d_entrys)
      : is_inited_(false),
        d_entrys_(d_entrys)
  {
    filter_str_[0] = '\0';
    format_buffer_[0] = '\0';
  }
  virtual ~ObDirPrefixLSIDFilter() = default;
  int init(const char *filter_str, const int32_t filter_str_len);
  virtual int func(const dirent *entry) override;
private:
  bool is_inited_;
  char filter_str_[common::MAX_PATH_SIZE];
  char format_buffer_[share::OB_BACKUP_LS_DIR_NAME_LENGTH];
  common::ObIArray<share::ObLSID> &d_entrys_;
  DISALLOW_COPY_AND_ASSIGN(ObDirPrefixLSIDFilter);
};

}
}

#endif
