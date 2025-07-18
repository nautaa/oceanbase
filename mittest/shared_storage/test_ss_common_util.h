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
#ifndef USING_LOG_PREFIX
#define USING_LOG_PREFIX STORAGETEST
#endif

#define private public
#define protected public
#include "lib/ob_errno.h"
#include "storage/blocksstable/ob_macro_block_id.h"
#include "storage/blocksstable/ob_block_sstable_struct.h"
#include "storage/blocksstable/ob_storage_object_rw_info.h"
#include "storage/shared_storage/ob_ss_reader_writer.h"
#include "storage/shared_storage/ob_ss_micro_cache.h"
#include "lib/random/ob_random.h"
#include "lib/compress/ob_compress_util.h"

namespace oceanbase
{
namespace storage
{
using namespace oceanbase::common;
using namespace oceanbase::blocksstable;

static const int64_t DEFAULT_BLOCK_SIZE = 2 * 1024 * 1024; // 2MB

class TestSSCommonUtil
{
public:
  struct MicroBlockInfo
  {
    MicroBlockInfo() : macro_id_(), offset_(0), size_(0)
    {}
    MicroBlockInfo(MacroBlockId macro_id, int64_t offset, int64_t size)
        : macro_id_(macro_id), offset_(offset), size_(size)
    {}
    MicroBlockInfo(const ObSSMicroBlockId &micro_id)
        : macro_id_(micro_id.macro_id_), offset_(micro_id.offset_), size_(micro_id.size_)
    {}
    bool is_valid() const
    {
      return macro_id_.is_valid() && offset_ > 0 && size_ > 0;
    }
    MacroBlockId macro_id_;
    int64_t offset_;
    int64_t size_;
    TO_STRING_KV(K_(macro_id), K_(offset), K_(size));
  };

public:
  static MacroBlockId gen_macro_block_id(const int64_t second_id);
  static MacroBlockId gen_macro_block_id(const int64_t second_id, const int64_t fourth_id);
  static ObMicroBlockId gen_micro_block_id(const MacroBlockId &macro_id, const int64_t offset, const int64_t size);
  static ObSSMicroBlockCacheKey gen_phy_micro_key(const MacroBlockId &macro_id, const int32_t offset, const int32_t size);
  static int gen_random_data(char *buf, const int64_t size);
  static ObCompressorType get_random_compress_type();
  static int wait_for_persist_task();
  static int alloc_micro_block_meta(ObSSMicroBlockMeta *&micro_meta);
  static int prepare_micro_blocks(const int64_t macro_block_cnt, const int64_t block_size,
      ObArray<MicroBlockInfo> &micro_block_arr, const int64_t start_macro_id = 1, const bool random_micro_size = true,
      const int32_t min_micro_size = 16 * 1024, const int32_t max_micro_size = 16 * 1024);
  static int get_micro_block(const MicroBlockInfo &micro_info, char *read_buf);
  static int init_io_info(ObIOInfo &io_info, const ObSSMicroBlockCacheKey &micro_key, const int32_t size, char *read_buf);
  static int clear_micro_cache(ObSSMicroMetaManager *micro_meta_mgr, ObSSPhysicalBlockManager *phy_blk_mgr,
                               ObSSMicroRangeManager *micro_range_mgr, int64_t &micro_data_blk_cnt,
                               int64_t &range_micro_sum);
  static void stop_all_bg_task(ObSSMicroCache *micro_cache);
  static void resume_all_bg_task(ObSSMicroCache *micro_cache);
};

MacroBlockId TestSSCommonUtil::gen_macro_block_id(const int64_t second_id)
{
  MacroBlockId macro_id(0, second_id, 0);
  macro_id.set_id_mode((uint64_t)ObMacroBlockIdMode::ID_MODE_SHARE);
  macro_id.set_storage_object_type((uint64_t)ObStorageObjectType::SHARED_MAJOR_DATA_MACRO);
  return macro_id;
}

MacroBlockId TestSSCommonUtil::gen_macro_block_id(const int64_t second_id, const int64_t fourth_id)
{
  MacroBlockId macro_id(0, second_id, 0);
  macro_id.set_id_mode((uint64_t)ObMacroBlockIdMode::ID_MODE_SHARE);
  macro_id.set_storage_object_type((uint64_t)ObStorageObjectType::SHARED_MAJOR_DATA_MACRO);
  macro_id.set_reorganization_scn(fourth_id);
  return macro_id;
}

ObMicroBlockId TestSSCommonUtil::gen_micro_block_id(
    const MacroBlockId &macro_id,
    const int64_t offset,
    const int64_t size)
{
  ObMicroBlockId micro_id(macro_id, offset, size);
  return micro_id;
}

ObSSMicroBlockCacheKey TestSSCommonUtil::gen_phy_micro_key(
    const MacroBlockId &macro_id,
    const int32_t offset,
    const int32_t size)
{
  ObSSMicroBlockCacheKey micro_key;
  micro_key.mode_ = ObSSMicroBlockCacheKeyMode::PHYSICAL_KEY_MODE;
  micro_key.micro_id_.macro_id_ = macro_id;
  micro_key.micro_id_.offset_ = offset;
  micro_key.micro_id_.size_ = size;
  return micro_key;
}

int TestSSCommonUtil::gen_random_data(char *buf, const int64_t size)
{
  int ret = OB_SUCCESS;
  ObRandom rand;
  if (OB_ISNULL(buf) || OB_UNLIKELY(size <= 0)) {
    ret = OB_INVALID_ARGUMENT;
  } else {
    const int64_t end = size - 1;
    for (int64_t i = 0; i < end; ++i) {
      buf[i] = static_cast<char>(ObRandom::rand(0, 128));
    }
    buf[end] = '\0';
  }
  return ret;
}

ObCompressorType TestSSCommonUtil::get_random_compress_type()
{
  const int64_t random_val = ObRandom::rand(1, 2);
  const ObCompressorType compress_type = (random_val % 2 == 0) ? ObCompressorType::SNAPPY_COMPRESSOR : ObCompressorType::NONE_COMPRESSOR;
  return compress_type;
}

int TestSSCommonUtil::wait_for_persist_task()
{
  int ret = OB_SUCCESS;
  const int64_t interval_us = 10 * 1000 * 1000L;
  const int64_t start_us = ObTimeUtility::current_time();
  while (ATOMIC_LOAD(&SSMicroCacheStat.mem_blk_stat().mem_blk_fg_used_cnt_) > 1 ||
         ATOMIC_LOAD(&SSMicroCacheStat.mem_blk_stat().mem_blk_bg_used_cnt_) > 1) {
    ob_usleep(1000);
    const int64_t cur_us = ObTimeUtility::current_time();
    if (cur_us - start_us > interval_us) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("waiting time is too long", KR(ret), "cache_stat", SSMicroCacheStat);
      break;
    }
  }
  return ret;
}

int TestSSCommonUtil::alloc_micro_block_meta(ObSSMicroBlockMeta *&micro_meta)
{
  int ret = OB_SUCCESS;
  void *ptr = SSMicroMetaAlloc.alloc();
  if (OB_ISNULL(ptr)) {
    ret = OB_ALLOCATE_MEMORY_FAILED;
    LOG_WARN("fail to alloc micro_meta mem", KR(ret));
  } else {
    micro_meta = new(ptr) ObSSMicroBlockMeta();
    SSMicroCacheStat.mem_stat().micro_alloc_cnt_ += 1;
  }
  return ret;
}

int TestSSCommonUtil::get_micro_block(const MicroBlockInfo &micro_info, char *read_buf)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!micro_info.is_valid()|| OB_ISNULL(read_buf))) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(micro_info), KP(read_buf));
  } else {
    ObStorageObjectHandle object_handle;
    ObIOInfo io_info;
    io_info.tenant_id_ = MTL_ID();
    io_info.offset_ = micro_info.offset_;
    io_info.size_ = micro_info.size_;
    io_info.flag_.set_wait_event(1);
    io_info.timeout_us_ = 5 * 1000 * 1000;
    io_info.buf_ = read_buf;
    io_info.user_data_buf_ = read_buf;
    io_info.effective_tablet_id_ = micro_info.macro_id_.second_id();
    ObSSMicroBlockCacheKey micro_key = gen_phy_micro_key(micro_info.macro_id_, micro_info.offset_, micro_info.size_);
    ObSSMicroBlockId phy_micro_id(micro_info.macro_id_, micro_info.offset_, micro_info.size_);
    ObSSMicroCache *micro_cache = MTL(ObSSMicroCache *);
    bool hit_cache = false;
    if (OB_FAIL(micro_cache->get_micro_block_cache(micro_key, phy_micro_id,
        ObSSMicroCacheGetType::FORCE_GET_DATA, io_info, object_handle,
        ObSSMicroCacheAccessType::COMMON_IO_TYPE, hit_cache))) {
      LOG_WARN("fail to get micro block cache", KR(ret), K(micro_info));
    } else if (OB_FAIL(object_handle.wait())) {
      LOG_WARN("fail to wait until get micro block data", KR(ret), K(micro_info));
    } else if (OB_UNLIKELY(io_info.size_ != object_handle.get_data_size())) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("get data size wrong", KR(ret), K(io_info.size_), K(object_handle.get_data_size()), K(micro_info));
    } else {
      char c = micro_info.macro_id_.hash() % 128;
      for (int64_t i = 0; OB_SUCC(ret) && i < micro_info.size_; i++) {
        if (c != read_buf[i]) {
          ret = OB_ERR_UNEXPECTED;
          LOG_WARN("get wrong data", KR(ret), K(i), K(c), K(read_buf[i]));
        }
      }
    }
  }
  return ret;
}

int TestSSCommonUtil::prepare_micro_blocks(
    const int64_t macro_block_cnt,
    const int64_t block_size,
    ObArray<MicroBlockInfo> &micro_block_arr,
    const int64_t start_macro_id,
    const bool random_micro_size,
    const int32_t min_micro_size,
    const int32_t max_micro_size)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(macro_block_cnt <= 0 || block_size <= 0)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(macro_block_cnt), K(block_size));
  } else {
    int64_t alignment = 4096;
    ObMemAttr attr(MTL_ID(), "test");
    char *buf = nullptr;
    if (OB_ISNULL(buf = static_cast<char *>(ob_malloc_align(alignment, block_size, attr)))) {
      ret = OB_ALLOCATE_MEMORY_FAILED;
      LOG_WARN("fail to allocate memory", KR(ret));
    } else {
      const int64_t random_min_micro_size = 200;
      const int64_t random_max_micro_size = 16 * 1024;
      const int64_t start_offset = ObSSMemBlock::get_reserved_size();
      for (int64_t i = start_macro_id; OB_SUCC(ret) && i < (macro_block_cnt + start_macro_id); ++i) {
        MacroBlockId macro_id = gen_macro_block_id(i);
        char c = macro_id.hash() % 128;
        MEMSET(buf, c, block_size);
        ObStorageObjectHandle write_object_handle;
        ObSSObjectStorageWriter object_storage_writer;
        ObStorageObjectWriteInfo write_info;
        write_info.io_desc_.set_wait_event(1);
        write_info.buffer_ = buf;
        write_info.offset_ = 0;
        write_info.size_ = block_size;
        write_info.io_timeout_ms_ = DEFAULT_IO_WAIT_TIME_MS;
        write_info.mtl_tenant_id_ = MTL_ID();
        if (OB_FAIL(write_object_handle.set_macro_block_id(macro_id))) {
          LOG_WARN("fail to set macro block id", KR(ret), K(macro_id));
        } else if (OB_FAIL(object_storage_writer.aio_write(write_info, write_object_handle))) {
          LOG_WARN("fail to aio write", KR(ret), K(write_info), K(write_object_handle));
        } else if (OB_FAIL(write_object_handle.wait())) {
          LOG_WARN("fail to wait", KR(ret));
        }

        if (OB_SUCC(ret)) {
          // split macro block into micro blocks.
          int64_t remain_size = block_size - start_offset;
          int32_t offset = start_offset;
          do {
            const int32_t micro_size = random_micro_size ? ObRandom::rand(min_micro_size, max_micro_size)
                                                         : (min_micro_size + max_micro_size) / 2;
            ObSSMicroBlockCacheKey tmp_key = gen_phy_micro_key(macro_id, offset, micro_size);
            ObSSMicroBlockIndex micro_index(tmp_key, micro_size);
            const int32_t delta_size = micro_size + micro_index.get_serialize_size();
            if (remain_size >= delta_size) {
              MicroBlockInfo micro_info(macro_id, offset, micro_size);
              if (OB_FAIL(micro_block_arr.push_back(micro_info))) {
                LOG_WARN("fail to push back", KR(ret), K(micro_info));
              } else {
                offset += micro_size;
                remain_size -= delta_size;
              }
            } else {
              remain_size = 0;
            }
          } while (OB_SUCC(ret) && remain_size > 0);
        }
      }
    }
    if (buf != nullptr) {
      ob_free_align(buf);
    }
  }
  return ret;
}

int TestSSCommonUtil::init_io_info(
    ObIOInfo &io_info,
    const ObSSMicroBlockCacheKey &micro_key,
    const int32_t size,
    char *read_buf)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!micro_key.is_valid() || micro_key.is_logical_key()) || OB_ISNULL(read_buf)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(micro_key), K(read_buf));
  } else {
    io_info.tenant_id_ = MTL_ID();
    io_info.offset_ = micro_key.micro_id_.offset_;
    io_info.size_ = size;
    io_info.flag_.set_wait_event(1);
    io_info.timeout_us_ = 5 * 1000 * 1000;
    io_info.buf_ = read_buf;
    io_info.user_data_buf_ = read_buf;
    io_info.effective_tablet_id_ = micro_key.get_macro_tablet_id().id();
  }
  return ret;
}

int TestSSCommonUtil::clear_micro_cache(
    ObSSMicroMetaManager *micro_meta_mgr,
    ObSSPhysicalBlockManager *phy_blk_mgr,
    ObSSMicroRangeManager *micro_range_mgr,
    int64_t &micro_data_blk_cnt,
    int64_t &range_micro_sum)
{
  int ret = OB_SUCCESS;
  if (OB_ISNULL(micro_meta_mgr) || OB_ISNULL(phy_blk_mgr) || OB_ISNULL(micro_range_mgr)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(micro_meta_mgr), K(phy_blk_mgr), K(micro_range_mgr));
  } else {
    // clear micro_meta map
    micro_meta_mgr->micro_meta_map_.clear();
    const int64_t cur_micro_cnt = micro_meta_mgr->micro_meta_map_.count();
    if (cur_micro_cnt != 0) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("unexpected micro_meta map", KR(ret), K(cur_micro_cnt));
    }

    // clear phy_block
    if (OB_SUCC(ret)) {
      const int64_t total_blk_cnt = phy_blk_mgr->blk_cnt_info_.total_blk_cnt_;
      for (int64_t i = SS_SUPER_BLK_COUNT; OB_SUCC(ret) && i < total_blk_cnt; ++i) {
        ObSSPhysicalBlock *phy_blk = phy_blk_mgr->inner_get_phy_block_by_idx(i);
        if (OB_NOT_NULL(phy_blk)) {
          if (phy_blk->get_block_type() == ObSSPhyBlockType::SS_MICRO_DATA_BLK) {
            phy_blk->valid_len_ = 0;
            phy_blk->is_sealed_ = 0;
            phy_blk->is_free_ = 1;
            phy_blk_mgr->free_bitmap_->set(i, true);
            ++micro_data_blk_cnt;
          }
        } else {
          ret = OB_ERR_UNEXPECTED;
          LOG_WARN("phy_block should not be null", KR(ret), K(i), K(total_blk_cnt));
        }
      }

      if (OB_FAIL(ret)) {
      } else {
        phy_blk_mgr->blk_cnt_info_.reuse();
        phy_blk_mgr->reusable_blks_.reuse();
        phy_blk_mgr->super_blk_.reuse();
      }
    }
    // clear micro_range
    if (OB_SUCC(ret)) {
      ObSSMicroInitRangeInfo **init_range_arr = micro_range_mgr->get_init_range_arr();
      const int64_t init_range_cnt = micro_range_mgr->get_init_range_cnt();
      range_micro_sum = 0;
      for (int64_t i = 0; i < init_range_cnt; ++i) {
        range_micro_sum += init_range_arr[i]->get_total_micro_cnt();
        init_range_arr[i]->try_free_sub_ranges();
      }
    }
  }
  return ret;
}

void TestSSCommonUtil::stop_all_bg_task(ObSSMicroCache *micro_cache)
{
  if (nullptr != micro_cache) {
    ObSSMicroCacheTaskRunner &task_runner = micro_cache->task_runner_;
    task_runner.persist_data_task_.is_inited_ = false;
    task_runner.persist_meta_task_.is_inited_ = false;
    task_runner.release_cache_task_.is_inited_ = false;
    task_runner.blk_ckpt_task_.is_inited_ = false;
  }
}

void TestSSCommonUtil::resume_all_bg_task(ObSSMicroCache *micro_cache)
{
  if (nullptr != micro_cache) {
    ObSSMicroCacheTaskRunner &task_runner = micro_cache->task_runner_;
    task_runner.persist_data_task_.is_inited_ = true;
    task_runner.persist_meta_task_.is_inited_ = true;
    task_runner.release_cache_task_.is_inited_ = true;
    task_runner.blk_ckpt_task_.is_inited_ = true;
  }
}

}  // namespace storage
}  // namespace oceanbase