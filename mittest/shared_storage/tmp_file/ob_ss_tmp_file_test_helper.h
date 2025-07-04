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

#ifndef OB_SS_TMP_FILE_TEST_HELPER_
#define OB_SS_TMP_FILE_TEST_HELPER_
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mittest/mtlenv/storage/tmp_file/ob_tmp_file_test_helper.h"
#define USING_LOG_PREFIX STORAGE
#define protected public
#define private public
#include "share/ob_thread_pool.h"
#include "share/rc/ob_tenant_base.h"
#include "storage/tmp_file/ob_tmp_file_io_info.h"
#include "storage/tmp_file/ob_tmp_file_io_handle.h"
#include "storage/tmp_file/ob_tmp_file_manager.h"
#include "storage/tmp_file/ob_compress_tmp_file_manager.h"
#include "storage/tmp_file/ob_ss_tmp_file_flush_list_iterator.h"

namespace oceanbase
{
using namespace common;
using namespace tmp_file;
using namespace share;
/* ------------------------------ Test Helper ------------------------------ */
int set_ss_tmp_file_flushing(ObSharedStorageTmpFile &file)
{
  int ret = OB_SUCCESS;
  ObSSTmpFileFlushListIterator iter;
  bool find = false;
  if (OB_FAIL(iter.init(file.flush_prio_mgr_))) {
    LOG_WARN("fail to init flush list iterator", KR(ret));
  } else {
    FlushCtxState cur_stage = FlushCtxState::FSM_F1;
    const FlushCtxState end_stage = FlushCtxState::FSM_F3;
    ObArray<ObSharedStorageTmpFile*> unmatched_files;
    while(OB_SUCC(ret) && cur_stage <= end_stage && !find) {
      ObSSTmpFileHandle file_handle;
      if (OB_FAIL(iter.next(cur_stage, file_handle))) {
        if (OB_ITER_END != ret) {
          LOG_WARN("fail to get next file in flush list", KR(ret), K(cur_stage));
        } else if (OB_FAIL(ObTmpFileGlobal::advance_flush_ctx_state(cur_stage, cur_stage))) {
          LOG_WARN("fail to advance flush ctx state", KR(ret), K(cur_stage));
        }
      } else if (OB_ISNULL(file_handle.get())) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("fail to get temporary file", KR(ret), KP(file_handle.get()));
      } else if (file_handle.get()->fd_ == file.fd_) {
        find = true;
      } else if (OB_FAIL(unmatched_files.push_back(file_handle.get()))) {
        LOG_WARN("fail to push back file handle", KR(ret), K(file_handle));
      }
    } // end while
    for (int i = 0; OB_SUCC(ret) && i < unmatched_files.count(); ++i) {
      if (OB_FAIL(unmatched_files.at(i)->reinsert_data_flush_node())) {
        LOG_ERROR("fail to reinsert data flush node", KR(ret), KPC(unmatched_files.at(i)));
      }
    }
  }

  if (OB_SUCC(ret) && OB_UNLIKELY(!find)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("fail to find file in flush list", KR(ret), K(file));
  }
  return ret;
}

// Verify that the free_page_list accurately reflects all available free pages,
// ensuring that max_page_num - used_page_num equals free_page_size.
void check_wbp_free_list(ObSSTmpWriteBufferPool &wbp)
{
  int ret = OB_SUCCESS;
  int64_t max_page_num = wbp.get_max_page_num();
  ObArray<uint32_t> data_page_id;
  ObArray<uint32_t> meta_page_id;

  int64_t free_page_num = max_page_num - wbp.used_page_num_;
  LOG_DEBUG("checking free page num", K(free_page_num), K(wbp.used_page_num_));
  wbp.print_statistics();
  EXPECT_GT(free_page_num, 0);

  // alloc data
  for (int32_t i = 0; OB_SUCC(ret) && i < free_page_num; ++i) {
    char *buf = nullptr;
    uint32_t new_page_id = ObTmpFileGlobal::INVALID_PAGE_ID;
    if (OB_FAIL(wbp.alloc_page(0/*fd*/, ObTmpFilePageUniqKey(i), new_page_id, buf))) {
      LOG_DEBUG("wbp fail to alloc data page", K(wbp.shrink_ctx_));
    } else {
      ASSERT_EQ(OB_SUCCESS, data_page_id.push_back(new_page_id));
    }
  }
  ret = OB_SUCCESS;

  // alloc meta
  for (int32_t i = 0; OB_SUCC(ret) && i < free_page_num; ++i) {
    char *buf = nullptr;
    uint32_t new_page_id = ObTmpFileGlobal::INVALID_PAGE_ID;
    if (OB_FAIL(wbp.alloc_page(0/*fd*/, ObTmpFilePageUniqKey(0, i), new_page_id, buf))) {
      LOG_DEBUG("wbp fail to alloc meta page", K(wbp.shrink_ctx_));
    } else {
      ASSERT_EQ(OB_SUCCESS, meta_page_id.push_back(new_page_id));
    }
  }
  LOG_DEBUG("allocate all usable free pages",
           K(free_page_num), K(data_page_id.size()), K(meta_page_id.size()));
  wbp.print_statistics();
  EXPECT_EQ(free_page_num, data_page_id.size() + meta_page_id.size());

  // free all pages
  ret = OB_SUCCESS;
  uint32_t next_page_id = ObTmpFileGlobal::INVALID_PAGE_ID;
  for (int32_t i = 0; OB_SUCC(ret) && i < data_page_id.size(); ++i) {
    ret = wbp.free_page(0/*fd*/, data_page_id.at(i), ObTmpFilePageUniqKey(i), next_page_id);
    ASSERT_EQ(OB_SUCCESS, ret);
  }
  for (int32_t i = 0; OB_SUCC(ret) && i < meta_page_id.size(); ++i) {
    ret = wbp.free_page(0/*fd*/, meta_page_id.at(i), ObTmpFilePageUniqKey(0, i), next_page_id);
    ASSERT_EQ(OB_SUCCESS, ret);
  }
}

/* -------------------------- TestTmpFileStress --------------------------- */
enum TmpFileOp {
  WRITE,
  READ,
  TRUNCATE,
  SEAL,
  OP_MAX
};

class TestTmpFileStress : public share::ObThreadPool
{
public:
  TestTmpFileStress(ObTenantBase *tenant_ctx);
  virtual ~TestTmpFileStress();
  int init(const int fd, const TmpFileOp op, const int64_t thread_cnt, const int64_t timeout_ms,
           char *buf, const int64_t offset, const int64_t size);
  void reset();
  virtual void run1();
  TO_STRING_KV(K(thread_cnt_), K(fd_), K(timeout_ms_), K(op_), KP(buf_), K(offset_), K(size_));
private:
  void write_data_(const int64_t write_size);
  void truncate_data_();
  void read_data_(const int64_t read_offset, const int64_t read_size);
private:
  int64_t thread_cnt_;
  int fd_;
  int64_t timeout_ms_;
  TmpFileOp op_;
  char *buf_;
  int64_t offset_;
  int64_t size_;
  ObTenantBase *tenant_ctx_;
};

TestTmpFileStress::TestTmpFileStress(ObTenantBase *tenant_ctx)
  : thread_cnt_(0), fd_(0),
    timeout_ms_(0), op_(OP_MAX),
    buf_(nullptr), offset_(0),
    size_(0),
    tenant_ctx_(tenant_ctx)
{
}

TestTmpFileStress::~TestTmpFileStress()
{
}

int TestTmpFileStress::init(const int fd, const TmpFileOp op,
                            const int64_t thread_cnt,
                            const int64_t timeout_ms,
                            char *buf, int64_t offset,
                            const int64_t size)
{
  int ret = OB_SUCCESS;
  if (thread_cnt < 0 || OB_ISNULL(buf) || offset < 0 || size <= 0 || timeout_ms <= 0) {
    ret = OB_INVALID_ARGUMENT;
    STORAGE_LOG(WARN, "invalid argument", K(ret), K(thread_cnt), KP(buf), K(offset), K(size), K(timeout_ms));
  } else if (TmpFileOp::OP_MAX == op) {
    ret = OB_INVALID_ARGUMENT;
    STORAGE_LOG(WARN, "invalid argument", K(ret), K(op));
  } else if ((op == TmpFileOp::WRITE || op == TmpFileOp::TRUNCATE) && 1 != thread_cnt) {
    ret = OB_INVALID_ARGUMENT;
    STORAGE_LOG(WARN, "invalid argument", K(ret), K(op), K(thread_cnt));
  } else {
    buf_ = buf;
    thread_cnt_ = thread_cnt;
    timeout_ms_ = timeout_ms;
    fd_ = fd;
    op_ = op;
    offset_ = offset;
    size_ = size;
    set_thread_count(static_cast<int32_t>(thread_cnt));
  }
  return ret;
}

void TestTmpFileStress::reset()
{
  thread_cnt_ = 0;
  fd_ = 0;
  op_ = OP_MAX;
  buf_ = nullptr;
  offset_ = 0;
  size_ = 0;
}

void TestTmpFileStress::write_data_(const int64_t write_size)
{
  STORAGE_LOG(INFO, "TestTmpFileStress write thread", K(fd_), K(thread_idx_), KP(buf_), K(size_));
  int ret = OB_SUCCESS;
  ObArray<int64_t> size_array;
  ObTmpFileIOInfo io_info;
  ASSERT_EQ(OB_SUCCESS, ret);
  io_info.fd_ = fd_;
  io_info.io_desc_.set_wait_event(2);
  io_info.io_timeout_ms_ = timeout_ms_;
  int64_t already_write = 0;
  std::vector<int64_t> turn_write_size = generate_random_sequence(1, write_size / 3, write_size, 3);
  for (int i = 0; i < turn_write_size.size(); ++i) {
    int64_t this_turn_write_size = turn_write_size[i];
    STORAGE_LOG(INFO, "random write size", K(fd_), K(thread_idx_), KP(buf_), K(size_), K(this_turn_write_size));
    // write data
    io_info.buf_ = buf_ + already_write;
    if (this_turn_write_size % ObTmpFileGlobal::PAGE_SIZE == 0 && i == 0) {
      io_info.size_ = this_turn_write_size - 2 * 1024;
      ret = MTL(ObTenantTmpFileManager *)->write(MTL_ID(), io_info);
      if (OB_FAIL(ret)) {
        STORAGE_LOG(ERROR, "TestTmpFileStress write thread failed", KR(ret), K(fd_), K(thread_idx_),
                    K(this_turn_write_size), K(already_write), K(write_size), K(io_info));
        ob_abort();
      }

      io_info.size_ = 2 * 1024;
      io_info.buf_ = buf_ + already_write + this_turn_write_size - 2 * 1024;
      ret = MTL(ObTenantTmpFileManager *)->write(MTL_ID(), io_info);
      if (OB_FAIL(ret)) {
        STORAGE_LOG(ERROR, "TestTmpFileStress write thread failed", KR(ret), K(fd_), K(thread_idx_),
                    K(this_turn_write_size), K(already_write), K(write_size), K(io_info));
        ob_abort();
      }
    } else {
      io_info.size_ = this_turn_write_size;
      ret = MTL(ObTenantTmpFileManager *)->write(MTL_ID(), io_info);
      if (OB_FAIL(ret)) {
        STORAGE_LOG(ERROR, "TestTmpFileStress write thread failed", KR(ret), K(fd_), K(thread_idx_),
                    K(this_turn_write_size), K(already_write), K(write_size), K(io_info));
        ob_abort();
      }
    }
    already_write += this_turn_write_size;
  }

  ASSERT_EQ(OB_SUCCESS, ret);
  STORAGE_LOG(INFO, "TestTmpFileStress write thread finished", K(fd_), K(thread_idx_), KP(buf_), K(size_));
}

void TestTmpFileStress::read_data_(const int64_t read_offset, const int64_t read_size)
{
  STORAGE_LOG(INFO, "TestTmpFileStress read thread start", K(fd_), K(thread_idx_), KP(buf_), K(read_offset), K(read_size));
  int ret = OB_SUCCESS;
  char *read_buf = new char[read_size];
  ObTmpFileIOInfo io_info;
  ObTmpFileIOHandle handle;
  io_info.fd_ = fd_;
  io_info.size_ = read_size;
  io_info.io_desc_.set_wait_event(2);
  io_info.io_timeout_ms_ = timeout_ms_;
  io_info.buf_ = read_buf;
  ret = MTL(ObTenantTmpFileManager *)->pread(MTL_ID(), io_info, read_offset, handle);
  int cmp = memcmp(handle.get_buffer(), buf_ + read_offset, io_info.size_);
  if (cmp != 0 || OB_FAIL(ret)) {
    printf("TestTmpFileStress read thread failed, fd_:%d, thread_idx_:%ld\n", fd_, thread_idx_);
    STORAGE_LOG(ERROR, "TestTmpFileStress read thread failed", KR(ret), K(fd_), K(cmp), K(thread_idx_), KP(buf_), K(read_offset), K(read_size));
    ob_abort();
  }
  ASSERT_EQ(OB_SUCCESS, ret);
  ASSERT_EQ(0, cmp);
  handle.reset();
  delete[] read_buf;
  STORAGE_LOG(INFO, "TestTmpFileStress read thread finished", K(fd_), K(thread_idx_), KP(buf_), K(read_offset), K(read_size));
}

void TestTmpFileStress::truncate_data_()
{
  int64_t truncate_offset = offset_ + MIN(size_, MAX(size_ / 10, 8 * 1024));
  STORAGE_LOG(INFO, "TestTmpFileStress truncate thread start", K(fd_), K(thread_idx_), KP(buf_),
              K(truncate_offset), K(offset_), K(size_));
  int ret = MTL(ObTenantTmpFileManager *)->truncate(MTL_ID(), fd_, truncate_offset);
  if (OB_FAIL(ret)) {
    STORAGE_LOG(ERROR, "TestTmpFileStress truncate thread failed", KR(ret), K(fd_), K(thread_idx_),
                K(truncate_offset));
    ob_abort();
  }
  ObTmpFileIOInfo io_info;
  io_info.fd_ = fd_;
  io_info.io_desc_.set_wait_event(2);
  io_info.io_timeout_ms_ = timeout_ms_;
  const int64_t invalid_size = truncate_offset - offset_;
  const int64_t valid_size = size_ - invalid_size;

  char *zero_buf = new char[invalid_size];
  MEMSET(zero_buf, 0, invalid_size);
  char *read_buf = new char[size_];
  io_info.size_ = size_;
  io_info.buf_ = read_buf;
  ObTmpFileIOHandle handle;
  ret = MTL(ObTenantTmpFileManager *)->pread(MTL_ID(), io_info, offset_, handle);
  int cmp = memcmp(handle.get_buffer()+invalid_size, buf_ + truncate_offset, valid_size);
  if (cmp != 0 || OB_FAIL(ret)) {
    STORAGE_LOG(ERROR, "TestTmpFileStress truncate thread failed. "
                "fail to compare valid part.", KR(ret), K(cmp), K(fd_), K(thread_idx_), KP(buf_),
                K(truncate_offset), K(valid_size), K(invalid_size), K(offset_), K(size_));
    ob_abort();
  }
  cmp = memcmp(handle.get_buffer(), zero_buf, invalid_size);
  if (cmp != 0) {
    STORAGE_LOG(ERROR, "TestTmpFileStress truncate thread failed. "
                "fail to compare zero part.", KR(ret), K(cmp), K(fd_), K(thread_idx_), KP(buf_),
                K(truncate_offset), K(valid_size), K(invalid_size), K(offset_), K(size_));
    ob_abort();
  }
  ASSERT_EQ(0, cmp);
  handle.reset();
  delete[] read_buf;
  delete[] zero_buf;

  truncate_offset = offset_ + size_;
  ret = MTL(ObTenantTmpFileManager *)->truncate(MTL_ID(), fd_, truncate_offset);
  if (OB_FAIL(ret)) {
    STORAGE_LOG(ERROR, "TestTmpFileStress truncate thread failed", KR(ret), K(fd_), K(thread_idx_),
                K(truncate_offset));
    ob_abort();
  }

  zero_buf = new char[size_];
  MEMSET(zero_buf, 0, size_);
  read_buf = new char[size_];
  io_info.buf_ = read_buf;
  ret = MTL(ObTenantTmpFileManager *)->pread(MTL_ID(), io_info, offset_, handle);
  cmp = memcmp(handle.get_buffer(), zero_buf, size_);
  ASSERT_EQ(OB_SUCCESS, ret);
  ASSERT_EQ(0, cmp);
  handle.reset();
  delete[] read_buf;
  delete[] zero_buf;
  STORAGE_LOG(INFO, "TestTmpFileStress truncate thread finished", K(fd_), K(thread_idx_), KP(buf_), K(offset_), K(size_));
}

void TestTmpFileStress::run1()
{
  ObTenantEnv::set_tenant(tenant_ctx_);
  common::ObCurTraceId::TraceId trace_id;
  ObCurTraceId::TraceId *cur_trace_id = ObCurTraceId::get_trace_id();
  if (nullptr != cur_trace_id && cur_trace_id->is_valid()) {
    trace_id = *cur_trace_id;
    LOG_INFO("init TestTmpFileStress with an old trace_id", KPC(cur_trace_id), KPC(this));
  } else {
    trace_id.init(GCONF.self_addr_);
    LOG_INFO("init TestTmpFileStress with a new trace_id", K(trace_id), KPC(this));
  }
  ObTraceIDGuard trace_guard(trace_id);

  if (op_ == TmpFileOp::WRITE) {
    write_data_(size_);
  } else if (op_ == TmpFileOp::READ) {
    int64_t read_offset = offset_ + (size_ / thread_cnt_) * thread_idx_;
    int64_t read_size = 0;
    if (thread_idx_ == thread_cnt_ - 1) {
      read_size = size_ / thread_cnt_ + size_ % thread_cnt_;
    } else {
      read_size = size_ / thread_cnt_;
    }
    read_data_(read_offset, read_size);
  } else {
    truncate_data_();
  }
}

/* -------------------------- TestMultiTmpFileStress --------------------------- */
class TestMultiTmpFileStress : public share::ObThreadPool
{
public:
  TestMultiTmpFileStress(ObTenantBase *tenant_ctx);
  virtual ~TestMultiTmpFileStress();
  int init(const int64_t file_cnt, const int64_t dir_id, const int64_t thread_cnt, const int64_t timeout_ms,
           const int64_t batch_size, const int64_t batch_num);
  virtual void run1();
private:
  int64_t file_cnt_;
  int64_t dir_id_;
  int64_t read_thread_cnt_perf_file_;
  int64_t timeout_ms_;
  int64_t batch_size_;
  int64_t batch_num_;
  ObTenantBase *tenant_ctx_;
};

TestMultiTmpFileStress::TestMultiTmpFileStress(ObTenantBase *tenant_ctx)
  : file_cnt_(0),
    dir_id_(-1),
    read_thread_cnt_perf_file_(0),
    timeout_ms_(0),
    batch_size_(0),
    batch_num_(0),
    tenant_ctx_(tenant_ctx)
{
}

TestMultiTmpFileStress::~TestMultiTmpFileStress()
{
}

int TestMultiTmpFileStress::init(const int64_t file_cnt,
                                 const int64_t dir_id,
                                 const int64_t thread_cnt,
                                 const int64_t timeout_ms,
                                 const int64_t batch_size,
                                 const int64_t batch_num)
{
  int ret = OB_SUCCESS;
  if (file_cnt < 0 || thread_cnt < 0) {
    ret = OB_INVALID_ARGUMENT;
    STORAGE_LOG(WARN, "invalid argument", K(ret), K(file_cnt), K(thread_cnt));
  } else {
    file_cnt_ = file_cnt;
    dir_id_ = dir_id;
    read_thread_cnt_perf_file_ = thread_cnt;
    timeout_ms_ = timeout_ms;
    batch_size_ = batch_size;
    batch_num_ = batch_num;
    set_thread_count(static_cast<int32_t>(file_cnt));
  }
  return ret;
}

void TestMultiTmpFileStress::run1()
{
  STORAGE_LOG(INFO, "TestMultiTmpFileStress thread run start");
  int ret = OB_SUCCESS;
  int64_t fd = 0;
  ObTenantEnv::set_tenant(tenant_ctx_);

  ret = MTL(ObTenantTmpFileManager *)->open(fd, dir_id_, "");
  std::cout << "normal case, fd: " << fd << std::endl;
  ASSERT_EQ(OB_SUCCESS, ret);
  STORAGE_LOG(INFO, "open file success", K(fd));
  tmp_file::ObSSTmpFileHandle file_handle;
  ret = MTL(ObTenantTmpFileManager *)->get_tmp_file(fd, file_handle);
  ASSERT_EQ(OB_SUCCESS, ret);
  file_handle.get()->page_idx_cache_.max_bucket_array_capacity_ = ObSSTmpFileWBPIndexCache::INIT_BUCKET_ARRAY_CAPACITY * 2;
  file_handle.reset();

  int64_t file_size = batch_size_ * batch_num_;
  char * data_buffer = new char[file_size];
  for (int64_t i = 0; i < file_size;) {
    int64_t random_length = generate_random_int(1024, 8 * 1024);
    int64_t random_int = generate_random_int(0, 256);
    for (int64_t j = 0; j < random_length && i + j < file_size; ++j) {
      data_buffer[i + j] = random_int;
    }
    i += random_length;
  }

  TestTmpFileStress test_truncate(tenant_ctx_);
  for (int64_t i = 0; i < batch_num_; ++i) {
    if (i > 0) {
      // truncate read data in previous round
      test_truncate.init(fd, TmpFileOp::TRUNCATE, 1, timeout_ms_, data_buffer, (i-1) * batch_size_, batch_size_);
      ASSERT_EQ(OB_SUCCESS, ret);
      STORAGE_LOG(INFO, "test_truncate run start", K(i), K(batch_size_));
      test_truncate.start();
    }
    TestTmpFileStress test_write(tenant_ctx_);
    ret = test_write.init(fd, TmpFileOp::WRITE, 1, timeout_ms_, data_buffer + i * batch_size_, 0, batch_size_);
    ASSERT_EQ(OB_SUCCESS, ret);
    STORAGE_LOG(INFO, "test_write run start");
    test_write.start();
    test_write.wait();
    STORAGE_LOG(INFO, "test_write run end");

    TestTmpFileStress test_read(tenant_ctx_);
    ret = test_read.init(fd, TmpFileOp::READ, read_thread_cnt_perf_file_, timeout_ms_, data_buffer, i * batch_size_, batch_size_);
    ASSERT_EQ(OB_SUCCESS, ret);

    STORAGE_LOG(INFO, "test_read run start", K(i), K(batch_size_));
    test_read.start();
    test_read.wait();
    STORAGE_LOG(INFO, "test_read run end");

    if (i > 0) {
      // wait to truncate read data in last round
      test_truncate.wait();
      test_truncate.reset();
      STORAGE_LOG(INFO, "test_truncate run end", K(i));
    }

    STORAGE_LOG(INFO, "TestMultiTmpFileStress thread run a batch end", K(i));
  }

  test_truncate.init(fd, TmpFileOp::TRUNCATE, 1, timeout_ms_, data_buffer, file_size - batch_size_, batch_size_);
  ASSERT_EQ(OB_SUCCESS, ret);
  STORAGE_LOG(INFO, "test_truncate run start");
  test_truncate.start();
  test_truncate.wait();
  STORAGE_LOG(INFO, "test_truncate run end");

  ret = MTL(ObTenantTmpFileManager *)->remove(fd);
  ASSERT_EQ(OB_SUCCESS, ret);

  delete[] data_buffer;
  STORAGE_LOG(INFO, "TestMultiTmpFileStress thread run end");
}

} // namespace oceanbase

#endif
