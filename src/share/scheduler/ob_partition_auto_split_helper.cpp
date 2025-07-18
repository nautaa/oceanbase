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

#define USING_LOG_PREFIX STORAGE

#include "share/scheduler/ob_partition_auto_split_helper.h"
#include "share/schema/ob_schema_printer.h"
#include "observer/omt/ob_tenant_timezone_mgr.h"
#include "rootserver/ob_root_service.h"
#include "storage/tx_storage/ob_ls_service.h"
#include "sql/resolver/ob_resolver_utils.h"
#include "rootserver/ddl_task/ob_ddl_scheduler.h"


namespace oceanbase
{
namespace share
{

uint64_t ObSplitTask::hash() const
{
  uint64_t hash_val = murmurhash(&tenant_id_, sizeof(tenant_id_), 0);
  hash_val = murmurhash(&tablet_id_, sizeof(tablet_id_), hash_val);
  return hash_val;
}

bool ObSplitTask::operator==(const ObSplitTask &other) const
{
  return tenant_id_ == other.tenant_id_ && tablet_id_ == other.tablet_id_;
}

bool ObSplitTask::operator!=(const ObSplitTask &other) const
{
  return !(*this == other);
}

int ObSplitTask::assign(const ObSplitTask &other)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!other.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(other));
  } else {
    tenant_id_ = other.tenant_id_;
    tablet_id_ = other.tablet_id_;
  }
  return ret;
}
int ObAutoSplitTask::assign(const ObAutoSplitTask &other)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!other.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(other));
  } else {
    auto_split_tablet_size_ = other.auto_split_tablet_size_;
    ls_id_ = other.ls_id_;
    retry_times_ = other.retry_times_;
    tablet_id_ = other.tablet_id_;
    tenant_id_ = other.tenant_id_;
    used_disk_space_ = other.used_disk_space_;
  }
  return ret;
}

int ObTabletSplitTask::assign(const ObTabletSplitTask &other)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!other.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(other));
  } else {
    tenant_id_ = other.tenant_id_;
    tablet_id_ = other.tablet_id_;
    next_schedule_time_ = other.next_schedule_time_;
    task_status_ = other.task_status_;
  }
  return ret;
}

ObAutoSplitTaskCache::ObAutoSplitTaskCache()
  : ObSplitTaskCache(), max_heap_(max_comp_, &cache_malloc_), min_heap_(min_comp_, &cache_malloc_)
  {}

int ObAutoSplitTaskCache::init(const int64_t bucket_num, const uint64_t tenant_id, const uint64_t host_tenant_id)
{
  int ret = OB_SUCCESS;
  ObLockGuard<ObSpinLock> guard(lock_);
  if (OB_UNLIKELY(inited_)) {
    ret = OB_INIT_TWICE;
    LOG_WARN("init ObAutoSplitTaskCache twice", K(ret), K(inited_));
  } else if (OB_UNLIKELY(bucket_num <= 0 || OB_INVALID_TENANT_ID == tenant_id || OB_INVALID_TENANT_ID == host_tenant_id)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(bucket_num), K(tenant_id), K(host_tenant_id));
  } else if (OB_FAIL(tasks_set_.create(bucket_num, ObMemAttr(host_tenant_id, "task_cache")))) {
    LOG_WARN("fail to create hashset", KR(ret));
  } else {
    inited_ = true;
    tenant_id_ = tenant_id;
    host_tenant_id_ = host_tenant_id;
    cache_malloc_.set_attr(ObMemAttr(host_tenant_id_, "task_cache"));
  }
  return ret;
}

int ObAutoSplitTaskCache::mtl_init(ObAutoSplitTaskCache *&task_cache)
{
  int ret = OB_SUCCESS;
  if (OB_ISNULL(task_cache)) {
    ret = OB_NULL_CHECK_ERROR;
    LOG_WARN("unexpeted null ptr of task_cache", K(ret), KP(task_cache));
  } else if (OB_FAIL(task_cache->init(ObAutoSplitTaskCache::CACHE_MAX_CAPACITY, MTL_ID(), MTL_ID()))) {
    LOG_WARN("failed to init ObAutoSplitTaskCache", K(ret), "tenant_id: ", MTL_ID());
  }
  return ret;
}

void ObAutoSplitTaskCache::destroy()
{
  ObLockGuard<ObSpinLock> guard(lock_);
  const ObIArray<ObAutoSplitTaskCache::ObAutoSplitTaskWrapper *> &min_heap_array = min_heap_.get_heap_data();
  const ObIArray<ObAutoSplitTaskCache::ObAutoSplitTaskWrapper *> &max_heap_array = max_heap_.get_heap_data();
  for (int64_t i = 0; i < min_heap_array.count(); ++i) {
    ObAutoSplitTaskWrapper *ptr_to_tsak_wrapper = min_heap_array.at(i);
    if (OB_NOT_NULL(ptr_to_tsak_wrapper)) {
      (*ptr_to_tsak_wrapper).~ObAutoSplitTaskWrapper();
      cache_malloc_.free(ptr_to_tsak_wrapper);
      ptr_to_tsak_wrapper = nullptr;
    }
  }
  for (int64_t i = 0; i < max_heap_array.count(); ++i) {
    ObAutoSplitTaskWrapper *ptr_in_max = max_heap_array.at(i);
    bool find = false;
    for (int64_t j = 0; j < min_heap_array.count(); ++j) {
      ObAutoSplitTaskWrapper *ptr_in_min = min_heap_array.at(j);
      if (ptr_in_min == ptr_in_max) {
        find = true;
        break;
      }
    }
    if (OB_UNLIKELY(!find) && OB_NOT_NULL(ptr_in_max)) {
      (*ptr_in_max).~ObAutoSplitTaskWrapper();
      cache_malloc_.free(ptr_in_max);
      ptr_in_max = nullptr;
    }
  }
  inited_ = false;
  total_tasks_ = 0;
  tenant_id_ = OB_INVALID_TENANT_ID;
  max_heap_.reset();
  min_heap_.reset();
  (void) tasks_set_.destroy();
}

int ObAutoSplitTaskCache::remove_tasks(const int64_t num_tasks_to_rem)
{
  int ret = OB_SUCCESS;
  int64_t num_tasks_rem = 0;
  if (OB_UNLIKELY(!inited_)) {
    ret = OB_NOT_INIT;
    LOG_WARN("not init", K(ret), K(inited_));
  } else if (OB_UNLIKELY(num_tasks_to_rem <= 0)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(num_tasks_to_rem));
  } else {
    int64_t num_tasks_can_rem = min(get_tasks_num(), num_tasks_to_rem);
    for (; OB_SUCC(ret) && num_tasks_can_rem > 0; --num_tasks_can_rem) {
      if (OB_FAIL(atomic_remove_task())) {
        LOG_WARN("atomic remove task failed", K(ret));
      }
    }
  }
  return ret;
}

int ObAutoSplitTaskCache::atomic_remove_task()
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!inited_)) {
    ret = OB_NOT_INIT;
    LOG_WARN("not init", K(ret), K(inited_));
  } else if (OB_UNLIKELY(min_heap_.count() <= 0 || max_heap_.count() <= 0)) {
    ret = OB_ENTRY_NOT_EXIST;
    LOG_WARN("no element to remove", K(ret), K(min_heap_.count()), K(max_heap_.count()));
  } else {
    ObAutoSplitTaskWrapper *ptr_task_wrapper = min_heap_.top();
    if (OB_ISNULL(ptr_task_wrapper)) {
      ret = OB_NULL_CHECK_ERROR;
      LOG_ERROR("ptr_task_wrapper is nullptr", K(ret));
      int tmp_ret = OB_SUCCESS;
      if (OB_TMP_FAIL(min_heap_.pop())) {
        LOG_WARN("pop from min heap failed", K(ret));
      }
    } else if (OB_FAIL(tasks_set_.erase_refactored(ObSplitTask(ptr_task_wrapper->task_.tenant_id_, ptr_task_wrapper->task_.tablet_id_))) && OB_HASH_NOT_EXIST != ret) {
      LOG_WARN("remove key from task_set_ failed", K(ret));
    } else {
      if (OB_HASH_NOT_EXIST == ret) {
        LOG_WARN("task key not existed in tasks_sets", K(ret), K(ObSplitTask(ptr_task_wrapper->task_.tenant_id_, ptr_task_wrapper->task_.tablet_id_)));
        //overwrite ret
        ret = OB_SUCCESS;
      }
      int64_t pos_at_max_heap = ptr_task_wrapper->pos_at_max_heap_;
      if (pos_at_max_heap >= 0 && pos_at_max_heap < max_heap_.count() && max_heap_.at(pos_at_max_heap) == ptr_task_wrapper) {
        if (OB_FAIL(max_heap_.remove(ptr_task_wrapper))) {
          LOG_ERROR("remove from max_heap failed", K(ret));
        }
      } else {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("the content of min heap and max heap don't match", K(ret), KPC(max_heap_.at(pos_at_max_heap)), KPC(ptr_task_wrapper));
        //overwrite ret
        ret = OB_SUCCESS;
      }
      if (OB_SUCC(ret)) {
        if (OB_FAIL(min_heap_.pop())) {
          LOG_ERROR("pop from min heap failed", K(ret));
        } else {
          (*ptr_task_wrapper).~ObAutoSplitTaskWrapper();
          cache_malloc_.free(ptr_task_wrapper);
          ptr_task_wrapper = nullptr;
          (void) ATOMIC_FAA(&total_tasks_, -1);
        }
      }
    }
  }
  return ret;
}

int ObAutoSplitTaskCache::atomic_push_task(const ObAutoSplitTask &task)
{
  int ret = OB_SUCCESS;
  void *buf = nullptr;
  if (OB_UNLIKELY(!inited_)) {
    ret = OB_NOT_INIT;
    LOG_WARN("not init", K(ret), K(inited_));
  } else if (OB_UNLIKELY(!task.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(task));
  } else if (OB_ISNULL(buf = cache_malloc_.alloc(sizeof(ObAutoSplitTaskWrapper)))) {
    ret = OB_ALLOCATE_MEMORY_FAILED;
    LOG_WARN("allocate memory for task failed", K(ret));
  } else {
    ObAutoSplitTaskWrapper *ptr_t_wra_to_min = new (buf) ObAutoSplitTaskWrapper;
    ObAutoSplitTaskWrapper *ptr_t_wra_to_max = nullptr;
    ptr_t_wra_to_min->priority_ = static_cast<double>(task.used_disk_space_) / task.auto_split_tablet_size_;
    int tmp_ret = OB_SUCCESS;
    (void) ATOMIC_FAA(&total_tasks_, 1);
    if (OB_FAIL(ptr_t_wra_to_min->task_.assign(task))) {
      LOG_WARN("task assign failed", K(ret), K(task));
    } else if (OB_FAIL(min_heap_.push(ptr_t_wra_to_min))) {
      LOG_WARN("push task into min_heap_ failed", K(ret), K(task));
    } else if (OB_FALSE_IT(ptr_t_wra_to_max = ptr_t_wra_to_min)) {
    } else if (OB_FALSE_IT(ptr_t_wra_to_min = nullptr)) {
    } else if (OB_FAIL(max_heap_.push(ptr_t_wra_to_max))) {
      LOG_WARN("push task into max_heap_ failed", K(ret), K(task));
    } else if (OB_FALSE_IT(ptr_t_wra_to_max = nullptr)) {
    } else if (OB_FAIL(tasks_set_.set_refactored(ObSplitTask (task.tenant_id_, task.tablet_id_)))) {
      LOG_WARN("push into task_set_ failed", K(ret));
    }
    if (OB_NOT_NULL(ptr_t_wra_to_min)) {
      (void) ATOMIC_FAA(&total_tasks_, -1);
      (*ptr_t_wra_to_min).~ObAutoSplitTaskWrapper();
      cache_malloc_.free(ptr_t_wra_to_min);
      ptr_t_wra_to_min = nullptr;
    }
  }
  return ret;
}

int ObAutoSplitTaskCache::atomic_pop_task(ObAutoSplitTask &task)
{
  int ret = OB_SUCCESS;
  task.reset();
  if (OB_UNLIKELY(!inited_)) {
    ret = OB_NOT_INIT;
    LOG_WARN("not init", K(ret), K(inited_));
  } else if (OB_UNLIKELY(min_heap_.count() <= 0 || max_heap_.count() <= 0)) {
    ret = OB_ENTRY_NOT_EXIST;
    LOG_WARN("no element to pop", K(ret), K(min_heap_.count()), K(max_heap_.count()));
  } else {
    ObAutoSplitTaskWrapper *ptr_task_wrapper = max_heap_.top();
    if (OB_ISNULL(ptr_task_wrapper)) {
      ret = OB_NULL_CHECK_ERROR;
      LOG_WARN("ptr_task_wrapper is nullptr", K(ret));
      int tmp_ret = OB_SUCCESS;
      if (OB_TMP_FAIL(max_heap_.pop())) {
        LOG_WARN("pop from min heap failed", K(ret));
      }
    } else if (OB_FAIL(task.assign(ptr_task_wrapper->task_))) {
      LOG_WARN("assign task failed", K(ret), K(task));
    } else if (OB_FAIL(tasks_set_.erase_refactored(ObSplitTask(task.tenant_id_, task.tablet_id_))) && OB_HASH_NOT_EXIST != ret) {
      LOG_WARN("remove key from task_set_ failed", K(ret));
    } else {
      if (OB_HASH_NOT_EXIST == ret) {
        LOG_WARN("task key not existed in tasks_sets", K(ret));
        //overwrite ret
        ret = OB_SUCCESS;
      }
      if (OB_FAIL(max_heap_.pop())) {
        //overwrite ret
        LOG_ERROR("max_heap_ pop failed", K(ret));
      } else if (OB_FAIL(min_heap_.remove(ptr_task_wrapper))) {
        //overwrite ret
        LOG_WARN("remove from min_heap_ failed", K(ret));
      } else {
        (*ptr_task_wrapper).~ObAutoSplitTaskWrapper();
        cache_malloc_.free(ptr_task_wrapper);
        ptr_task_wrapper = nullptr;
        (void) ATOMIC_FAA(&total_tasks_, -1);
      }
    }
  }
  return ret;
}

int ObAutoSplitTaskCache::pop_tasks(const int64_t num_tasks_to_pop, ObIArray<ObSplitTask*> &task_array, ObIAllocator &allocator)
{
  int ret = OB_SUCCESS;
  task_array.reuse();
  ObLockGuard<ObSpinLock> guard(lock_);
  if (OB_UNLIKELY(!inited_)) {
    ret = OB_NOT_INIT;
    LOG_WARN("not init", K(ret), K(inited_));
  } else if (OB_UNLIKELY(num_tasks_to_pop <= 0)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(num_tasks_to_pop));
  } else if (OB_UNLIKELY(min_heap_.count() <= 0 || max_heap_.count() <= 0)) {
    ret = OB_ENTRY_NOT_EXIST;
    LOG_DEBUG("no element to pop", K(ret), K(min_heap_.count()), K(max_heap_.count()));
  } else {
    int ret = OB_SUCCESS;
    int64_t num_tasks_can_pop = min(get_tasks_num(), num_tasks_to_pop);
    ObAutoSplitTask *task = nullptr;
    void *buf = nullptr;
    for (; OB_SUCC(ret) && num_tasks_can_pop > 0; --num_tasks_can_pop) {
      if (OB_ISNULL(buf = allocator.alloc(sizeof(ObAutoSplitTask)))) {
        ret = OB_ALLOCATE_MEMORY_FAILED;
        LOG_WARN("allocate memory failed", K(ret), K(task));
      } else if (FALSE_IT(task = new (buf) ObAutoSplitTask())) {
      } else if (OB_ISNULL(task)) {
        ret = OB_NULL_CHECK_ERROR;
        LOG_WARN("unexpected null ptr of task", K(ret), K(task));
      } else if (OB_FALSE_IT(task->reset())) {
      } else if (OB_FAIL(atomic_pop_task(*task))) {
        LOG_WARN("pop task failed", K(ret));
      } else if (OB_FAIL(task_array.push_back(task))) {
        LOG_WARN("push back into task array failed", K(ret));
      }
    }
    if (OB_FAIL(ret) && OB_NOT_NULL(task)) {
      task->~ObAutoSplitTask();
      allocator.free(task);
      task = nullptr;
    }
  }
  return ret;
}

int ObAutoSplitTaskCache::push_tasks(const ObIArray<const ObSplitTask*> &task_array)
{
  ObLockGuard<ObSpinLock> guard(lock_);
  int ret = OB_SUCCESS;
  int tmp_ret = OB_SUCCESS;

  if (OB_UNLIKELY(!inited_)) {
    ret = OB_NOT_INIT;
    LOG_WARN("not init", K(ret), K(inited_));
  } else if (OB_UNLIKELY(task_array.count() == 0)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(task_array));
  } else {
    for (int64_t i = 0; OB_SUCC(ret) && i < task_array.count(); ++i) {
      const ObSplitTask *split_task = task_array.at(i);
      const ObAutoSplitTask *auto_split_task = nullptr;
      double task_priority = 0;
      tmp_ret = OB_SUCCESS;
      if (OB_ISNULL(split_task)) {
        tmp_ret = OB_NULL_CHECK_ERROR;
        LOG_WARN("fail to check null ptr", K(tmp_ret), K(split_task));
      } else if (OB_ISNULL(auto_split_task = static_cast<const ObAutoSplitTask *>(split_task))) {
        tmp_ret = OB_ERR_UNEXPECTED;
        LOG_WARN("task type doesn't match the cache type", K(tmp_ret), K(split_task));
      } else if (OB_UNLIKELY(!auto_split_task->is_valid())) {
        tmp_ret = OB_INVALID_ARGUMENT;
        LOG_WARN("trying to push an invalid task into cache", K(tmp_ret), K(split_task));
      } else if (OB_FALSE_IT(task_priority = static_cast<double>(auto_split_task->used_disk_space_) / auto_split_task->auto_split_tablet_size_)) {
      } else if (get_tasks_num() >= CACHE_MAX_CAPACITY && task_priority <= min_heap_.top()->priority_) {
        // do nothing
      } else if (OB_TMP_FAIL(tasks_set_.exist_refactored(ObSplitTask(auto_split_task->tenant_id_, auto_split_task->tablet_id_)))) {
        if (OB_HASH_NOT_EXIST == tmp_ret) {
          tmp_ret = OB_SUCCESS;
          if (OB_FAIL(atomic_push_task(*auto_split_task))) {
            LOG_WARN("atomic push task failed", K(ret), K(*auto_split_task));
          }
        } else if (OB_HASH_EXIST == tmp_ret) {
          tmp_ret = OB_SUCCESS;
        } else {
          LOG_WARN("check task key existed failed", K(tmp_ret));
        }
      } else {
        //ObHashSet::exist_refactored always returns error
        LOG_WARN("never expect to reach here", K(tmp_ret));
      }
    }
    tmp_ret = OB_SUCCESS;
    int64_t diff =  get_tasks_num() - CACHE_MAX_CAPACITY;
    if (diff > 0 && OB_FAIL(remove_tasks(diff))) {
      LOG_WARN("remove task from cache failed", K(ret), K(diff));
    }
  }
  return ret;
}

int ObAutoSpTaskSchedEntry::assign(const ObAutoSpTaskSchedEntry &other)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!other.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(other));
  } else {
    tenant_id_ = other.tenant_id_;
    table_id_ = other.table_id_;
    next_valid_schedule_time_ = other.next_valid_schedule_time_;
    if (OB_FAIL(task_.assign(other.task_))) {
      LOG_WARN("failed to assign", K(ret), K(task_), K(other.task_));
    }
  }
  return ret;
}

int ObRsAutoSplitScheduler::pop_tasks(const int64_t num_tasks_can_pop, const bool throttle_by_table, ObArray<ObAutoSplitTask> &task_array)
{
  int ret = OB_SUCCESS;
  task_array.reuse();
  ObArray<ObArray<ObAutoSplitTask>> tenant_task_arrays;
  ObSEArray<ObSEArray<ObSplitTask*,10>, 5> tmp_tenant_task_arrays;
  common::ObArenaAllocator allocator(common::ObMemAttr(OB_SERVER_TENANT_ID, "sche_split"));
  if (throttle_by_table) {
    int64_t num_tasks_pop_from_poll_mgr = MAX_SPLIT_TASK_DIRECT_CACHE_SIZE - task_direct_cache_.count();
    if (polling_mgr_.is_empty() && task_direct_cache_.empty()) {
      //do nothing
    } else {
      if (OB_FAIL(polling_mgr_.pop_tasks(num_tasks_pop_from_poll_mgr, false, tmp_tenant_task_arrays, allocator))) {
       LOG_WARN("fail to pop tasks from tree", K(ret));
      }
      ObArray<ObAutoSplitTask> tmp_task_array;
      for (int64_t i = 0; OB_SUCC(ret) && i < tmp_tenant_task_arrays.count(); ++i) {
        ObSEArray<ObSplitTask*, 10> &tmp_tenant_task = tmp_tenant_task_arrays.at(i);
        tmp_task_array.reuse();
        for (int64_t j = 0; OB_SUCC(ret) && j < tmp_tenant_task.count(); ++j) {
          ObAutoSplitTask *ptr_to_auto_task = static_cast<ObAutoSplitTask*>(tmp_tenant_task.at(j));
          if (OB_ISNULL(ptr_to_auto_task)) {
            ret = OB_NULL_CHECK_ERROR;
            LOG_WARN("failed to do null check", K(ret), K(ptr_to_auto_task));
          } else if (OB_FAIL(tmp_task_array.push_back(*ptr_to_auto_task))) {
            LOG_WARN("failed to push back into tmp_task_array", K(ret));
          }
        }
        if (OB_FAIL(ret)) {
        } else if (OB_FAIL(tenant_task_arrays.push_back(tmp_task_array))) {
          LOG_WARN("failed to push bakc into", K(ret));
        }
      }
      if (OB_FAIL(ret)) {
      } else if (OB_FAIL(push_to_direct_cache(tenant_task_arrays))) {
        LOG_WARN("failed to push to direct cache", K(ret));
      } else if (OB_FAIL(pop_from_direct_cache(num_tasks_can_pop, task_array))) {
       LOG_WARN("failed ot pop from direct cache", K(ret));
      }
    }
  } else {
    if (OB_FAIL(pop_from_direct_cache(num_tasks_can_pop, task_array))) {
      LOG_WARN("failed ot pop from direct cache", K(ret));
    } else if (task_array.size() < num_tasks_can_pop) {
      if (OB_FAIL(polling_mgr_.pop_tasks(num_tasks_can_pop - task_array.size(), false, tmp_tenant_task_arrays, allocator))) {
        LOG_WARN("fail to pop tasks from tree", K(ret));
      } else {
        for (int64_t i = 0; OB_SUCC(ret) && i < tmp_tenant_task_arrays.count(); i++) {
          for (int64_t j = 0; OB_SUCC(ret) && j < tmp_tenant_task_arrays.at(i).count(); j++) {
            if (OB_FAIL(task_array.push_back(*static_cast<ObAutoSplitTask*>(tmp_tenant_task_arrays.at(i).at(j))))) {
              LOG_WARN("failed to append tasks from polling mgr", K(ret));
            }
          }
        }
      }
    }
  }
  return ret;
}


#ifdef OB_BUILD_SHARED_STORAGE
int ObTabletSplitTaskCache::init(const int64_t bucket_num, const uint64_t tenant_id, const uint64_t host_tenant_id)
{
  int ret = OB_SUCCESS;
  ObLockGuard<ObSpinLock> guard(lock_);
  if (OB_UNLIKELY(inited_)) {
    ret = OB_INIT_TWICE;
    LOG_WARN("init ObAutoSplitTaskCache twice", K(ret), K(inited_));
  } else if (OB_UNLIKELY(bucket_num <= 0 || OB_INVALID_TENANT_ID == tenant_id || OB_INVALID_TENANT_ID == host_tenant_id)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(bucket_num), K(tenant_id), K(host_tenant_id));
  } else if (OB_FAIL(tasks_set_.create(bucket_num, ObMemAttr(host_tenant_id, "task_cache")))) {
    LOG_WARN("fail to create hashset", KR(ret));
  } else {
    inited_ = true;
    tenant_id_ = tenant_id;
    host_tenant_id_ = host_tenant_id;
    cache_malloc_.set_attr(ObMemAttr(host_tenant_id_, "task_cache"));
  }
  return ret;
}

int ObTabletSplitTaskCache::pop_tasks(const int64_t num_tasks_to_pop, ObIArray<ObSplitTask*> &task_array, ObIAllocator &allocator)
{
  ObLockGuard<ObSpinLock> guard(lock_);
  int ret = OB_SUCCESS;
  task_array.reuse();
  if (OB_UNLIKELY(!inited_)) {
    ret = OB_NOT_INIT;
    LOG_WARN("not init", K(ret), K(inited_));
  } else if (OB_UNLIKELY(num_tasks_to_pop < 0)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(num_tasks_to_pop));
  } else {
    int64_t cur_time = ObTimeUtility::current_time();
    int64_t num_tasks_consumed = 0;
    ObTabletSplitTask *split_task_in_heap = nullptr;
    bool finished = false;
    while(OB_SUCC(ret) && num_tasks_consumed < num_tasks_to_pop && !schedule_time_min_heap_.empty() && !finished) {
      void *buf = nullptr;
      ObTabletSplitTask *new_tablet_split_task = nullptr;
      if (OB_ISNULL(split_task_in_heap = schedule_time_min_heap_.top())) {
        ret = OB_NULL_CHECK_ERROR;
        LOG_WARN("failed to null check", K(ret), KP(split_task_in_heap));
        int tmp_ret = OB_SUCCESS;
        if (OB_TMP_FAIL(schedule_time_min_heap_.pop())) {
          LOG_WARN("failed to pop from min heap", K(tmp_ret));
        }
      } else if (split_task_in_heap->next_schedule_time_ > cur_time) {
        finished = true;
      } else if (OB_ISNULL(buf = allocator.alloc(sizeof(ObTabletSplitTask)))) {
        ret = OB_ALLOCATE_MEMORY_FAILED;
        LOG_WARN("allocate memory failed", K(ret), KP(buf));
      } else if (FALSE_IT(new_tablet_split_task = new (buf) ObTabletSplitTask())) {
      } else if (OB_FAIL(new_tablet_split_task->assign(*split_task_in_heap))) {
        LOG_WARN("failed to assign to new_tablet_split_task", K(ret), K(split_task_in_heap));
      } else if (OB_FAIL(task_array.push_back(new_tablet_split_task))) {
        LOG_WARN("failed to push back into task_array", K(ret), KPC(new_tablet_split_task));
      } else if (OB_FALSE_IT(new_tablet_split_task = nullptr)) {
      } else if (OB_FALSE_IT(++num_tasks_consumed)) {
      } else if (OB_FAIL(tasks_set_.erase_refactored(*split_task_in_heap))) {
        LOG_WARN("failed to erase from task set", K(ret));
        if (OB_HASH_NOT_EXIST == ret) {
          //overwrite ret
          ret = OB_SUCCESS;
        }
      }
      if (OB_FAIL(ret)) {
        for (int64_t i = 0; i < task_array.count(); ++i) {
          ObTabletSplitTask *tmp_split_task = static_cast<ObTabletSplitTask *>(task_array.at(i));
          tmp_split_task->~ObTabletSplitTask();
          allocator.free(tmp_split_task);
          if (tmp_split_task == new_tablet_split_task) {
            new_tablet_split_task = nullptr;
          }
          task_array.at(i) = nullptr;
          tmp_split_task = nullptr;
        }
        if (OB_NOT_NULL(new_tablet_split_task)) {
          new_tablet_split_task->~ObTabletSplitTask();
          allocator.free(new_tablet_split_task);
          new_tablet_split_task = nullptr;
        }
      } else if (!finished) {
        if (OB_FAIL(schedule_time_min_heap_.pop())) {
          LOG_WARN("failed to pop from min heap", K(ret));
        } else if (OB_NOT_NULL(split_task_in_heap)) {
          split_task_in_heap->~ObTabletSplitTask();
          cache_malloc_.free(split_task_in_heap);
          split_task_in_heap = nullptr;
        }
      }
    }
    ATOMIC_STORE(&total_tasks_, tasks_set_.size());
  }
  return ret;
}

int ObTabletSplitTaskCache::push_tasks(const ObIArray<const ObSplitTask*> &task_array)
{
  ObLockGuard<ObSpinLock> guard(lock_);
  int ret = OB_SUCCESS;
  ObTabletSplitTask *task_in_set = nullptr;
  if (OB_UNLIKELY(!inited_)) {
    ret = OB_NOT_INIT;
    LOG_WARN("not init", K(ret), K(inited_));
  }
  for (int64_t i = 0; OB_SUCC(ret) && i < task_array.count(); ++i) {
    const ObSplitTask *split_task = task_array.at(i);
    const ObTabletSplitTask *tablet_split_task = static_cast<const ObTabletSplitTask*>(split_task);
    void *buf = nullptr;
    ObTabletSplitTask *new_tablet_split_task = nullptr;
    if (OB_ISNULL(tablet_split_task)) {
      ret = OB_NULL_CHECK_ERROR;
      LOG_WARN("unexpected null ptr of tablet_split_task", K(ret), K(tablet_split_task), K(task_array));
    } else if (OB_FALSE_IT(task_in_set = tasks_set_.get(*tablet_split_task))) {
    } else if (OB_ISNULL(task_in_set)) { //task not exist
      if (OB_ISNULL(buf = cache_malloc_.alloc(sizeof(ObTabletSplitTask)))) {
        ret = OB_ALLOCATE_MEMORY_FAILED;
        LOG_WARN("allocate memory failed", K(ret), K(buf));
      } else if (OB_FALSE_IT(new_tablet_split_task = new (buf) ObTabletSplitTask())) {
      } else if (OB_FAIL(new_tablet_split_task->assign(*tablet_split_task))) {
        LOG_WARN("failed to assign tablet_split_task", K(ret), KP(tablet_split_task));
      } else if (OB_FAIL(schedule_time_min_heap_.push(new_tablet_split_task))) {
        LOG_WARN("fail to push into schedule time min heap", K(ret));
      } else if (OB_FALSE_IT(new_tablet_split_task = nullptr)) {
      } else if (OB_FAIL(tasks_set_.set_refactored(*tablet_split_task))) {
        LOG_WARN("push into task_set_ failed", K(ret), K(*tablet_split_task));
      }
    } else if (TabletSplitTaskTatus::WAITING_SPLIT_DATA_COMPLEMENT == task_in_set->task_status_) {
      task_in_set->task_status_ = tablet_split_task->task_status_; //update task status
    }
    if (OB_FAIL(ret) && OB_NOT_NULL(new_tablet_split_task)) {
      new_tablet_split_task->~ObTabletSplitTask();
      cache_malloc_.free(new_tablet_split_task);
      new_tablet_split_task = nullptr;
    }
  }
  ATOMIC_STORE(&total_tasks_, tasks_set_.size());
  return ret;
};

int ObTabletSplitTaskCache::mtl_init(ObTabletSplitTaskCache *&task_cache)
{
  int ret = OB_SUCCESS;
  if (OB_ISNULL(task_cache)) {
    ret = OB_NULL_CHECK_ERROR;
    LOG_WARN("unexpeted null ptr of task_cache", K(ret), KP(task_cache));
  } else if (OB_FAIL(task_cache->init(ObTabletSplitTaskCache::BUCKET_NUM, MTL_ID(), MTL_ID()))) {
    LOG_WARN("failed to init ObAutoSplitTaskCache", K(ret), "tenant_id: ", MTL_ID());
  }
  return ret;
}
#endif

ObRsAutoSplitScheduler &ObRsAutoSplitScheduler::get_instance()
{
  static ObRsAutoSplitScheduler instance;
  return instance;
}

int ObRsAutoSplitScheduler::push_tasks(const ObArray<ObAutoSplitTask> &task_array)
{
  int ret = OB_SUCCESS;
  ObSEArray<const ObSplitTask*, 10> input_task_array;
  if (OB_UNLIKELY(!inited_)) {
    ret = OB_NOT_INIT;
    LOG_WARN("not init", K(ret), K(inited_));
  } else if (OB_UNLIKELY(task_array.count() == 0)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(task_array));
  } else {
    for (int64_t i = 0; OB_SUCC(ret) && i < task_array.count(); ++i) {
      const ObAutoSplitTask &auto_split_task = task_array.at(i);
      if (OB_FAIL(input_task_array.push_back(&auto_split_task))) {
        LOG_WARN("failed to push back into input_task_array", K(ret), K(i), K(auto_split_task));
      }
    }
    if (OB_FAIL(ret)) {
    } else if (OB_FAIL(polling_mgr_.push_tasks(input_task_array))) {
      LOG_WARN("fail to push tasks into polling_mgr_", K(ret));
    }
  }
  return ret;
}

bool ObRsAutoSplitScheduler::can_retry(const ObAutoSplitTask &task, const int ret)
{
  return task.retry_times_ < ObRsAutoSplitScheduler::MAX_TIMES_TASK_RETRY
      && ((!share::ObIDDLTask::in_ddl_retry_black_list(ret) && share::ObIDDLTask::in_ddl_retry_white_list(ret)) || OB_ERR_PARALLEL_DDL_CONFLICT == ret);
}

int ObRsAutoSplitScheduler::init()
{
  int ret = OB_SUCCESS;
  if (inited_) {
    ret = OB_INIT_TWICE;
    LOG_WARN("init twice", K(ret));
  } else if (OB_FAIL(polling_mgr_.init())) {
    LOG_WARN("failed to init polling manager", K(ret));
  } else {
    inited_ = true;
  }
  return ret;
}

int ObRsAutoSplitScheduler::check_ls_migrating(
      const uint64_t tenant_id,
      const ObTabletID &tablet_id,
      bool &is_migrating)
{
  int ret = OB_SUCCESS;
  is_migrating = false;
  ObLSID ls_id;
  ObAddr leader_addr;
  const int64_t rpc_timeout = ObDDLUtil::get_default_ddl_rpc_timeout();
  if (OB_UNLIKELY(OB_INVALID_TENANT_ID == tenant_id || !tablet_id.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid args", K(ret), K(tenant_id), K(tablet_id));
  } else if (OB_FAIL(ObDDLUtil::get_tablet_leader_addr(GCTX.location_service_,
          tenant_id, tablet_id, rpc_timeout, ls_id, leader_addr))) {
    LOG_WARN("failed to get orig leader addr", K(ret), K(tenant_id), K(tablet_id));
  } else {
    obrpc::ObFetchLSMemberAndLearnerListArg arg;
    arg.tenant_id_ = tenant_id;
    arg.ls_id_ = ls_id;
    storage::ObStorageHASrcInfo src_info;
    src_info.cluster_id_ = GCONF.cluster_id;
    src_info.src_addr_ = leader_addr;
    storage::ObStorageRpc *storage_rpc = nullptr;
    ObLSService *ls_service = nullptr;
    obrpc::ObFetchLSMemberAndLearnerListInfo member_info;
    MTL_SWITCH (OB_SYS_TENANT_ID) {
      if (OB_ISNULL(ls_service = MTL(ObLSService *))) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("ls service should not be NULL", K(ret), K(tenant_id), K(ls_id));
      } else if (OB_ISNULL(storage_rpc = ls_service->get_storage_rpc())) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("storage rpc should not be NULL", K(ret), K(tenant_id), K(ls_id));
      } else if (OB_FAIL(storage_rpc->fetch_ls_member_and_learner_list(tenant_id, ls_id, src_info, member_info))) {
        LOG_WARN("failed to check ls is valid member", K(ret), K(tenant_id), K(ls_id));
      } else {
        for (int64_t i = 0; OB_SUCC(ret) && !is_migrating && i < member_info.member_list_.get_member_number(); i++) {
          common::ObMember member;
          if (OB_FAIL(member_info.member_list_.get_member_by_index(i, member))) {
            LOG_WARN("get member failed", K(ret), K(i), K(member_info));
          } else if (member.is_migrating()) {
            is_migrating = true;
          }
        }
        for (int64_t i = 0; OB_SUCC(ret) && !is_migrating && i < member_info.learner_list_.get_member_number(); i++) {
          common::ObMember member;
          if (OB_FAIL(member_info.learner_list_.get_member_by_index(i, member))) {
            LOG_WARN("get member failed", K(ret), K(i), K(member_info));
          } else if (member.is_migrating()) {
            is_migrating = true;
          }
        }
      }
    }
  }
  return ret;
}

int ObRsAutoSplitScheduler::gc_deleted_tenant_caches()
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!inited_)) {
    ret = OB_NOT_INIT;
    LOG_WARN("not init", K(ret), K(inited_));
  } else if (OB_FAIL(polling_mgr_.gc_deleted_tenant_caches())) {
    LOG_WARN("failed to gc deleted tenant caches", K(ret));
  }
  return ret;
}

ObServerAutoSplitScheduler &ObServerAutoSplitScheduler::get_instance()
{
  static ObServerAutoSplitScheduler instance;
  return instance;
}

int ObServerAutoSplitScheduler::init()
{
  int ret = OB_SUCCESS;
  if (inited_) {
    ret = OB_INIT_TWICE;
    LOG_WARN("init twice", K(ret));
  } else if (OB_FAIL(polling_manager_.init())) {
    LOG_WARN("failed to init polling manager", K(ret));
  } else {
    inited_ = true;
  }
  return ret;
}

int ObRsAutoSplitScheduler::pop_from_direct_cache(const int64_t num_tasks_can_pop, ObIArray<ObAutoSplitTask> &task_array)
{
  int ret = OB_SUCCESS;
  int64_t budget = num_tasks_can_pop;
  ObSEArray<ObAutoSpTaskSchedEntry, MAX_SPLIT_TASK_DIRECT_CACHE_SIZE> unavailable_tasks;
  if (num_tasks_can_pop < 0) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(num_tasks_can_pop));
  } else {
    int64_t cur_time = ObTimeUtility::current_time();
    for (int64_t i = 0; OB_SUCC(ret) && i < task_direct_cache_.count(); ++i) {
      ObAutoSpTaskSchedEntry &task_record = task_direct_cache_.at(i);
      if (task_record.next_valid_schedule_time_ > cur_time || budget == 0) {
        if (OB_FAIL(unavailable_tasks.push_back(task_record))) {
          LOG_WARN("failed to push back", K(ret), K(task_record));
        }
      } else if (OB_FAIL(task_array.push_back(task_record.task_))) {
        LOG_WARN("failed to push back", K(ret));
      } else {
        --budget;
      }
    }
  }
  if (OB_FAIL(ret)) {
  } else {
    task_direct_cache_.reuse();
    if (OB_FAIL(append(task_direct_cache_, unavailable_tasks))) {
      LOG_WARN("failed to append", K(ret), K(task_direct_cache_), K(unavailable_tasks));
    }
  }
  return ret;
}

int ObRsAutoSplitScheduler::push_to_direct_cache(ObArray<ObArray<ObAutoSplitTask>> &tenant_task_arrays)
{
  int ret = OB_SUCCESS;
  ObSEArray<ObTabletID, MAX_SPLIT_TASKS_ONE_ROUND> tablet_ids;
  ObSEArray<uint64_t, MAX_SPLIT_TASKS_ONE_ROUND> table_ids;
  uint64_t tenant_id = OB_INVALID_ID;
  ObAutoSpTaskSchedEntry split_task_sched_entry;
  const int64_t cur_time = ObTimeUtility::current_time();
  const int64_t redundancy_time = 1L * 1000L * 1000L;
  for (int64_t i = 0; OB_SUCC(ret) && i < tenant_task_arrays.count(); ++i) {
    ObArray<ObAutoSplitTask> &split_task_array = tenant_task_arrays.at(i);
    tablet_ids.reuse();
    table_ids.reuse();
    for (int64_t j = 0; OB_SUCC(ret) && j < split_task_array.count(); ++j) {
      ObAutoSplitTask &auto_split_task = split_task_array.at(j);
      tenant_id = auto_split_task.tenant_id_;
      if (OB_FAIL(tablet_ids.push_back(auto_split_task.tablet_id_))) {
        LOG_WARN("failed to push back", K(ret), K(tablet_ids));
      }
    }
    if (OB_FAIL(ret)) {
    } else if (OB_FAIL(ObAutoSplitArgBuilder::acquire_table_id_of_tablets(tenant_id, tablet_ids, table_ids))) {
      LOG_WARN("failed to acquire table ids", K(ret));
    } else if (OB_UNLIKELY(tablet_ids.count() != split_task_array.count() || table_ids.count() != split_task_array.count())) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("unexpected error", K(ret), K(tablet_ids), K(split_task_array), K(table_ids));
    } else {
      for (int64_t m = 0; OB_SUCC(ret) && m < split_task_array.count(); ++m) {
        ObAutoSplitTask &auto_split_task = split_task_array.at(m);
        split_task_sched_entry.reset();
        split_task_sched_entry.tenant_id_ = auto_split_task.tenant_id_;
        split_task_sched_entry.table_id_ = table_ids.at(m);
        split_task_sched_entry.next_valid_schedule_time_ = cur_time;
        if (OB_FAIL(split_task_sched_entry.task_.assign(auto_split_task))) {
          LOG_WARN("failed to assgin auto split task", K(ret), K(auto_split_task));
        }
        bool duplicated = false;
        for (int64_t n = 0; !duplicated && OB_SUCC(ret) && n < task_direct_cache_.count(); ++n) {
          ObAutoSpTaskSchedEntry &entry_in_cache = task_direct_cache_.at(n);
          if (split_task_sched_entry == entry_in_cache) {
            split_task_sched_entry.next_valid_schedule_time_ = entry_in_cache.next_valid_schedule_time_ + rootserver::ObDDLScheduler::DDL_TASK_SCAN_PERIOD + redundancy_time;
            if (split_task_sched_entry.task_.tablet_id_ == entry_in_cache.task_.tablet_id_) {
              duplicated = true;
            }
          }
        }
        if (OB_FAIL(ret)) {
        } else if (!duplicated && OB_FAIL(task_direct_cache_.push_back(split_task_sched_entry))) {
          LOG_WARN("faied to push back", K(ret), K(split_task_sched_entry));
        }
      }
    }
  }
  return ret;
}

int ObServerAutoSplitScheduler::check_sstable_limit(const storage::ObTablet &tablet, bool &exceed_limit)
{
  int ret = OB_SUCCESS;
  ObTableStoreIterator iter;
  ObITable *unused_table = nullptr;
  int64_t count = 0;
  exceed_limit = false;
  if (OB_UNLIKELY(!tablet.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(tablet));
  } else if (OB_FAIL(tablet.get_all_sstables(iter))) {
    LOG_WARN("get all sstables failed", K(ret), K(tablet));
  }
  while (OB_SUCC(ret)) {
    if (OB_FAIL(iter.get_next(unused_table))) {
      if (OB_UNLIKELY(OB_ITER_END != ret)) {
        LOG_WARN("try to iterate sstables of the tablet failed", K(ret), K(tablet));
      } else {
        //overwrite ret
        ret = OB_SUCCESS;
        break;
      }
    } else if (OB_UNLIKELY((++count) > ObServerAutoSplitScheduler::SOURCE_TABLET_SSTABLE_LIMIT)) {
      exceed_limit = true;
      break;
    }
  }
  return ret;
}

int ObServerAutoSplitScheduler::check_and_fetch_tablet_split_info(const storage::ObTabletHandle &tablet_handle,
                                                                  storage::ObLS &ls,
                                                                  bool &can_split,
                                                                  ObAutoSplitTask &task)
{
  int ret = OB_SUCCESS;
  int64_t used_disk_space = OB_INVALID_SIZE;
  int64_t auto_split_tablet_size = OB_INVALID_SIZE;
  int64_t real_auto_split_size = OB_INVALID_SIZE;
  ObTablet *tablet = nullptr;
  ObTabletPointer *tablet_ptr = nullptr;
  ObRole role = INVALID_ROLE;
  const share::ObLSID ls_id = ls.get_ls_id();
  bool num_sstables_exceed_limit = false;
  can_split = false;
  task.reset();
  ObTabletSplitMdsUserData split_data;
  mds::MdsWriter writer;// will be removed later
  mds::TwoPhaseCommitState trans_stat;// will be removed later
  share::SCN trans_version;// will be removed later

  if (OB_UNLIKELY(!tablet_handle.is_valid() || !ls_id.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(tablet_handle), K(ls_id));
  } else if (OB_ISNULL(tablet = tablet_handle.get_obj())) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("pointer to tablet is nullptr", K(ret), KP(tablet));
  } else if (OB_FAIL(tablet->ObITabletMdsCustomizedInterface::get_latest_split_data(
      split_data, writer, trans_stat, trans_version))) {
    if (OB_EMPTY_RESULT == ret) {
      ret = OB_SUCCESS;
      auto_split_tablet_size = OB_INVALID_SIZE;
    } else {
      LOG_WARN("fail to get split data", K(ret), KP(tablet));
    }
  } else if (OB_FAIL(split_data.get_auto_part_size(auto_split_tablet_size))) {
    LOG_WARN("fail to get auto part size", K(ret), K(split_data));
  }


  if (OB_FAIL(ret)) {
  } else if (OB_ISNULL(tablet_ptr = tablet->get_pointer_handle().get_tablet_pointer())) {
    ret = OB_ERR_NULL_VALUE;
    LOG_WARN("unexpected null tablet pointer", K(ret), KPC(tablet));
  } else if (mds::TwoPhaseCommitState::ON_COMMIT == trans_stat) {
    tablet_ptr->set_auto_part_size(auto_split_tablet_size);
  } else {
    auto_split_tablet_size = tablet_ptr->get_auto_part_size();
  }

  if (OB_FAIL(ret)) {
  } else if (OB_INVALID_SIZE == auto_split_tablet_size) {
    can_split = false;
  } else if (OB_FAIL(check_sstable_limit(*tablet, num_sstables_exceed_limit))) {
    LOG_WARN("fail to check sstable limit", K(ret), KPC(tablet));
  } else if (OB_FAIL(ls.get_ls_role(role))) {
    LOG_WARN("get role failed", K(ret), K(MTL_ID()), K(ls_id));
  } else if (OB_FAIL(check_tablet_creation_limit(ObAutoSplitArgBuilder::get_max_split_partition_num(), 0.8/*safe_ratio*/, auto_split_tablet_size, real_auto_split_size))) {
    LOG_WARN("check_create_new_tablets fail", K(ret));
    if (OB_TOO_MANY_PARTITIONS_ERROR == ret) {
      can_split = false;
      ret = OB_SUCCESS;
    }
  } else {
    can_split = tablet->get_major_table_count() > 0 && tablet->get_data_tablet_id() == tablet->get_tablet_id()
        && common::ObRole::LEADER == role && !num_sstables_exceed_limit && MTL_ID() != OB_SYS_TENANT_ID;
    // TODO gaishun.gs resident_info
    const int64_t used_disk_space = tablet->get_tablet_meta().space_usage_.all_sstable_data_required_size_;
    can_split &= (used_disk_space > real_auto_split_size);
    if (OB_SUCC(ret) && can_split) {
      ObTabletCreateDeleteMdsUserData user_data;
      common::ObArenaAllocator allocator;
      const compaction::ObMediumCompactionInfoList *medium_info_list = nullptr;
      if (OB_FAIL(tablet->ObITabletMdsInterface::get_tablet_status(share::SCN::max_scn(),
          user_data, ObTabletCommon::DEFAULT_GET_TABLET_DURATION_US))) {
        LOG_WARN("failed to get tablet status", K(ret), KPC(tablet));
        can_split = false;
      } else if (OB_FAIL(tablet->read_medium_info_list(allocator, medium_info_list))) {
        LOG_WARN("failed to get medium info list", K(ret), KPC(tablet));
        can_split = false;
      } else if ((can_split = user_data.get_tablet_status() == ObTabletStatus::Status::NORMAL && (medium_info_list->size() == 0))) {
        task.tenant_id_ = MTL_ID();
        task.ls_id_ = ls_id;
        task.tablet_id_ = tablet->get_tablet_id();
        task.auto_split_tablet_size_ = auto_split_tablet_size;
        task.used_disk_space_ = used_disk_space;
        task.retry_times_ = 0;
      }
    }
  }
  return ret;
}

int ObServerAutoSplitScheduler::push_task(const storage::ObTabletHandle &tablet_handle, oceanbase::storage::ObLS &ls)
{
  int ret = OB_SUCCESS;
  ObSEArray<const ObSplitTask* ,1> task_array;
  common::ObArenaAllocator allocator(common::ObMemAttr(OB_SERVER_TENANT_ID, "sche_split"));
  ObAutoSplitTask task;
  bool can_split = false;
  if (OB_UNLIKELY(!inited_)) {
    ret = OB_NOT_INIT;
    LOG_WARN("not init", K(ret), K(inited_));
  } else if (OB_UNLIKELY(!tablet_handle.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(tablet_handle));
  } else if (OB_FAIL(check_and_fetch_tablet_split_info(tablet_handle, ls, can_split, task))) {
    if (OB_UNLIKELY(OB_NOT_SUPPORTED != ret)) {
      LOG_WARN("failed to check and fetch tablet split info", K(ret), K(task));
    }
  } else if (can_split && OB_FAIL(task_array.push_back(&task))) {
    LOG_WARN("task_array push back failed" , K(ret), K(task_array));
  } else if (can_split && OB_FAIL(polling_manager_.push_tasks(task_array))) {
    LOG_WARN("polling manager push task failed" , K(ret));
  } else if (ObTimeUtility::current_time() > ATOMIC_LOAD(&next_valid_time_) && !polling_manager_.is_empty()) {
    ObSEArray<ObSEArray<ObSplitTask*, 10>, 10> tenant_task_arrays;
    if (OB_FAIL(pop_tasks(tenant_task_arrays, allocator))) {
      if (OB_ENTRY_NOT_EXIST == ret) {
        LOG_DEBUG("tree pop task fail", K(ret) ,K(task_array));
        //overwrite ret
        ret = OB_SUCCESS;
      } else {
        LOG_WARN("tree pop task fail", K(ret), K(task_array));
      }
    } else if (tenant_task_arrays.count() == 0) {
      //do nothing
    } else if (OB_FAIL(batch_send_split_request(tenant_task_arrays))) {
      LOG_WARN("fail to send split request", K(ret), K(tenant_task_arrays));
    }
    for (int64_t i = 0; i < tenant_task_arrays.count(); ++i) {
      ObSEArray<ObSplitTask*, 10> &tmp_task_array = tenant_task_arrays.at(i);
      for (int64_t j = 0; j < tmp_task_array.count(); ++j) {
        ObSplitTask *&split_task = tmp_task_array.at(j);
        if (OB_NOT_NULL(split_task)) {
          split_task->~ObSplitTask();
          allocator.free(split_task);
          split_task = nullptr;
        }
      }
    }
  }
 return ret;
}

int ObServerAutoSplitScheduler::pop_tasks(ObIArray<ObSEArray<ObSplitTask*, 10>> &task_array, ObIAllocator &allocator)
{
  int ret = OB_SUCCESS;
  int tmp_ret = OB_SUCCESS;
  task_array.reuse();
  if (OB_UNLIKELY(!inited_)) {
    ret = OB_NOT_INIT;
    LOG_WARN("not init", K(ret), K(inited_));
  } else if (polling_manager_.is_empty()) {
    //do nothing
  } else if (OB_FAIL(polling_manager_.pop_tasks(ObServerAutoSplitScheduler::MAX_SPLIT_RPC_IN_BATCH, true/*need_to_prior_pop*/, task_array, allocator))) {
    LOG_WARN("fail to pop tasks from tree", K(ret));
  }
  return ret;
}


int ObServerAutoSplitScheduler::batch_send_split_request(const ObIArray<ObSEArray<ObSplitTask*, 10>> &tenant_task_arrays)
{
  int ret = OB_SUCCESS;
  obrpc::ObCommonRpcProxy *rpc_proxy = GCTX.rs_rpc_proxy_;
  obrpc::ObAutoSplitTabletBatchArg args;
  if (OB_UNLIKELY(!inited_)) {
    ret = OB_NOT_INIT;
    LOG_WARN("not init", K(ret), K(inited_));
  }
  for (int64_t i = 0; OB_SUCC(ret) && i < tenant_task_arrays.count(); ++i) {
    const ObSEArray<ObSplitTask*, 10> &task_array = tenant_task_arrays.at(i);
    for (int64_t j = 0; OB_SUCC(ret) && j < task_array.count(); ++j) {
      const ObAutoSplitTask *auto_split_task = static_cast<ObAutoSplitTask*>(task_array.at(j));
      if (OB_UNLIKELY(!auto_split_task->is_valid())) {
        //ignore ret
        int tmp_ret = OB_INVALID_ARGUMENT;
        LOG_WARN("invalid split task", K(tmp_ret), K(auto_split_task));
      } else {
        obrpc::ObAutoSplitTabletArg single_arg;
        single_arg.auto_split_tablet_size_ = auto_split_task->auto_split_tablet_size_;
        single_arg.ls_id_ = auto_split_task->ls_id_;
        single_arg.tablet_id_ = auto_split_task->tablet_id_;
        single_arg.tenant_id_ = auto_split_task->tenant_id_;
        single_arg.used_disk_space_ = auto_split_task->used_disk_space_;
        if (OB_FAIL(args.args_.push_back(single_arg))) {
          LOG_WARN("push task failed", K(ret), K(auto_split_task), K(j));
        }
      }
    }
    obrpc::ObAutoSplitTabletBatchRes results;
    if (OB_SUCC(ret)) {
      if (OB_FAIL(rpc_proxy->timeout(GCONF._ob_ddl_timeout).send_auto_split_tablet_task_request(args, results))) {
        LOG_WARN("failed to send_auto_split_tablet_task_request", KR(ret), K(args), K(results));
      } else if (OB_UNLIKELY(results.rets_.count() != args.args_.count())) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("send_auto_split_tablet_task_request rpc failed, the number of results doesn't match the number of arguments",
            K(ret), K(results.rets_), K(args.args_));
      } else {
        int64_t next_valid_time = max(ATOMIC_LOAD(&next_valid_time_), results.suggested_next_valid_time_);
        ATOMIC_STORE(&next_valid_time_, next_valid_time);
      }
    }
  }
  return ret;
}

int ObServerAutoSplitScheduler::check_tablet_creation_limit(const int64_t inc_tablet_cnt, const double safe_ratio, const int64_t split_size, int64_t &real_split_size)
{
  int ret = OB_SUCCESS;
  real_split_size = OB_INVALID_SIZE;
  const uint64_t tenant_id = MTL_ID();
  ObUnitInfoGetter::ObTenantConfig unit;
  ObTenantMetaMemMgr *t3m = MTL(ObTenantMetaMemMgr*);
  int64_t tablet_cnt_per_gb = ObServerAutoSplitScheduler::TABLET_CNT_PER_GB; // default value
  if (OB_UNLIKELY(inc_tablet_cnt < 0 || safe_ratio > 1 || safe_ratio <= 0 || split_size <= 0)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(inc_tablet_cnt), K(safe_ratio), K(split_size));
  } else {
    omt::ObTenantConfigGuard tenant_config(TENANT_CONF(tenant_id));
    if (OB_UNLIKELY(!tenant_config.is_valid())) {
      ret = OB_ERR_UNEXPECTED;
      LOG_ERROR("get invalid tenant config", K(ret));
    } else {
      tablet_cnt_per_gb = tenant_config->_max_tablet_cnt_per_gb;
    }
  }

  if (FAILEDx(GCTX.omt_->get_tenant_unit(tenant_id, unit))) {
    if (OB_TENANT_NOT_IN_SERVER != ret) {
      LOG_WARN("failed to get tenant unit", K(ret), K(tenant_id));
    } else {
      // during restart, tenant unit not ready, skip check
      ret = OB_SUCCESS;
    }
  } else {
    const double memory_limit = unit.config_.memory_size();
    const int64_t max_tablet_cnt = static_cast<int64_t>(memory_limit / (1 << 30) * tablet_cnt_per_gb * safe_ratio);
    const int64_t cur_tablet_cnt = t3m->get_total_tablet_cnt();
    double cur_ratio = 0.0;
    if (OB_UNLIKELY(cur_tablet_cnt + inc_tablet_cnt > max_tablet_cnt)) {
      ret = OB_TOO_MANY_PARTITIONS_ERROR;
      LOG_WARN("too many partitions of tenant", K(ret), K(tenant_id), K(memory_limit), K(tablet_cnt_per_gb),
          K(max_tablet_cnt), K(cur_tablet_cnt), K(inc_tablet_cnt));
    } else if (OB_UNLIKELY(max_tablet_cnt <= 0)) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("unexpected value of max_tablet_cnt", K(ret), K(max_tablet_cnt));
    } else if (OB_FALSE_IT(cur_ratio = static_cast<double>(cur_tablet_cnt + inc_tablet_cnt) / max_tablet_cnt)) {
    } else if (OB_FAIL(cal_real_auto_split_size(0.5/*base_ratio*/, cur_ratio, split_size, real_split_size))) {
      LOG_WARN("failed to cal tablet limit penalty", K(ret));
    }
  }
  return ret;
}

// since we don't want to do the auto split when the number of tablets is closed to the limit
// we implicitly increase the auto split size, when the number of tablets approaches to the limit
int ObServerAutoSplitScheduler::cal_real_auto_split_size(const double base_ratio, const double cur_ratio, const int64_t split_size, int64_t &real_split_size)
{
  int ret = OB_SUCCESS;
  int64_t tablet_limit_penalty = 1;
  real_split_size = 0;
  if (OB_UNLIKELY(base_ratio < 0 || base_ratio > 1.0 || cur_ratio < 0 || cur_ratio > 1.0 || split_size <= 0)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(base_ratio), K(cur_ratio), K(split_size));
  } else if (cur_ratio > base_ratio) {
    // the tablet_limit_penalty is designed to fit large table(10pb)
    // if we consider the base_ratio to be 0.5
    // than cur_ratio | tablet_limit_penalty
    //      0.55      |  2
    //      0.65      |  32
    //      0.75      |  256
    //      0.85      |  2048
    //      0.95      |  16384
    //      1.00      |  65536
    int64_t factor = static_cast<int64_t>(base_ratio >= cur_ratio ? 0 : (cur_ratio - base_ratio) / 0.03);
    if (OB_UNLIKELY(factor >= 32 || factor < 0)) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("unexpected value of factor", K(ret), K(factor));
    } else if (OB_FALSE_IT(tablet_limit_penalty = 1<<factor)) {
    } else if (tablet_limit_penalty > 0 && tablet_limit_penalty > INT64_MAX / split_size) {
      ret = OB_NUMERIC_OVERFLOW;
      LOG_WARN("multiplication overflow detected", K(ret), K(tablet_limit_penalty), K(split_size));
    } else {
      real_split_size = tablet_limit_penalty * split_size;
    }
  } else {
    real_split_size = split_size;
  }
  return ret;
}
#ifdef OB_BUILD_SHARED_STORAGE
int ObLSTabletSplitScheduler::init()
{
  int ret = OB_SUCCESS;
  if (inited_) {
    ret = OB_INIT_TWICE;
    LOG_WARN("init twice", K(ret));
  } else if (OB_FAIL(polling_manager_.init())) {
    LOG_WARN("failed to init polling manager", K(ret));
  } else {
    inited_ = true;
  }
  return ret;
}

int ObLSTabletSplitScheduler::try_schedule_available_tasks()
{
  int ret = OB_SUCCESS;
  int tmp_ret =  OB_SUCCESS;
  ObSEArray<ObSEArray<ObSplitTask*, 10>, 5> tenant_task_arrays;
  ObArenaAllocator allocator;
  ObSEArray<ObTabletSplitTask, 5> split_data_tasks;
  ObSEArray<ObTabletSplitTask, 5> pre_warm_tasks;
  if (OB_UNLIKELY(!inited_)) {
    ret = OB_NOT_INIT;
    LOG_WARN("not init", K(ret), K(inited_));
  } else if (polling_manager_.is_empty()) {
    // do nothing
  } else if (OB_FAIL(polling_manager_.pop_tasks(INT64_MAX/*to allow all avaliable tasks pop*/, false/*need_to_prior_pop*/, tenant_task_arrays, allocator))) {
    if (OB_ENTRY_NOT_EXIST == ret) {
      LOG_DEBUG("no tasks exist", K(ret));
      //over write ret code
      ret = OB_SUCCESS;
    } else {
      LOG_WARN("failed to pop tasks from polling manager", K(ret));
    }
  } else {
    int64_t cur_time = common::ObTimeUtility::current_time();
    for (int64_t i = 0; OB_SUCC(ret) && i < tenant_task_arrays.count(); ++i) {
      ObSEArray<ObSplitTask*, 10> &task_array = tenant_task_arrays.at(i);
      for (int64_t j = 0; OB_SUCC(ret) && j < task_array.count(); ++j) {
        tmp_ret = OB_SUCCESS;
        ObSplitTask *split_task = task_array.at(j);
        ObTabletSplitTask *tablet_split_task = static_cast<ObTabletSplitTask*>(split_task);
        if (OB_ISNULL(tablet_split_task)) {
          tmp_ret = OB_NULL_CHECK_ERROR;
          LOG_WARN("unexpected null ptr of tablet_split_task", K(tmp_ret), K(tablet_split_task));
        } else {
          switch (tablet_split_task->task_status_) {
            case TabletSplitTaskTatus::WAITING_PRE_WARM:
            case TabletSplitTaskTatus::WAITING_SPLIT_DATA_COMPLEMENT: {
              if (OB_TMP_FAIL(split_data_tasks.push_back(*tablet_split_task))) {
                LOG_WARN("failed to push tasks", K(tmp_ret));
              }
              break;
            }
            default : {
              tmp_ret = OB_ERR_UNEXPECTED;
              LOG_WARN("unexpected TabletSplitTaskTatus ", K(tmp_ret), KPC(tablet_split_task));
            }
          }
        }
      }
    }
    if (OB_SUCC(ret) && split_data_tasks.count() != 0) {
      if (OB_FAIL(schedule_data_split_dag(split_data_tasks))) {
        LOG_WARN("failed ot schedule_tablet_split_dag", K(tmp_ret));
      }
    }
  }
  //release resource
  for (int64_t i = 0; i < tenant_task_arrays.count(); ++i) {
    ObSEArray<ObSplitTask*, 10> &task_array = tenant_task_arrays.at(i);
    for (int64_t j = 0; j < task_array.count(); ++j) {
      ObSplitTask *&split_task = task_array.at(j);
      if (OB_NOT_NULL(split_task)) {
        split_task->~ObSplitTask();
        allocator.free(split_task);
        split_task = nullptr;
      }
    }
  }
  return ret;
}

int ObLSTabletSplitScheduler::push_task(const ObIArray<ObTabletSplitTask> &task_array)
{
  int ret = OB_SUCCESS;
  int tmp_ret = OB_SUCCESS;
  ObSEArray<const ObSplitTask*, 5> tmp_task_ptr_array;
  int64_t cur_time = ObTimeUtil::current_time();
  if (OB_UNLIKELY(!inited_)) {
    ret = OB_NOT_INIT;
    LOG_WARN("not init", K(ret), K(inited_));
  }
  for (int64_t i = 0; OB_SUCC(ret) && i < task_array.count(); ++i) {
    tmp_ret = OB_SUCCESS;
    const ObTabletSplitTask &tablet_split_task = task_array.at(i);
    if (OB_TMP_FAIL(tmp_task_ptr_array.push_back(&tablet_split_task))) {
      LOG_WARN("failed to push back into tmp_task_ptr_array", K(tmp_ret), K(i), K(tablet_split_task));
    }
  }
  if (OB_FAIL(ret)) {
  } else if (OB_FAIL(polling_manager_.push_tasks(tmp_task_ptr_array))) {
    LOG_WARN("failed to push back into polling manager", K(ret), K(task_array));
  }
  return ret;
}

int ObLSTabletSplitScheduler::schedule_data_split_dag(ObIArray<ObTabletSplitTask> &split_data_tasks)
{
  int ret = OB_SUCCESS;
  obrpc::ObSrvRpcProxy *rpc_proxy = GCTX.srv_rpc_proxy_;
  common::ObArenaAllocator allocator(common::ObMemAttr(OB_SERVER_TENANT_ID, "ScheSlit"));
  rootserver::ObDDLBuildSingleReplicaRequestProxy proxy(*rpc_proxy,
        &obrpc::ObSrvRpcProxy::build_ddl_single_replica_request);
  ObSEArray<obrpc::ObDDLBuildSingleReplicaRequestArg, 5> args;
  ObSEArray<ObAddr, 5> addrs;
  ObSEArray<TabletSplitTaskTatus, 5> split_task_status;
  ObSEArray<ObAddr, 5> split_replica_addrs;
  ObLSID ls_id;
  ObTabletID src_tablet_id;
  const int64_t rpc_timeout = ObDDLUtil::get_default_ddl_rpc_timeout();
  ObLSService *ls_service = nullptr;
  ObLSHandle ls_handle;
  ObTabletSplitInfoMdsUserData split_info_data;
  for (int64_t i = 0; OB_SUCC(ret) && i < split_data_tasks.count(); ++i) {
    ObTabletSplitTask &task = split_data_tasks.at(i);
    split_replica_addrs.reuse();
    ls_handle.reset();
    split_info_data.reset();
    ls_id.reset();
    MTL_SWITCH (task.tenant_id_) {
      ObTabletHandle tablet_handle;
      if (OB_ISNULL(ls_service = MTL(ObLSService *))) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("ls service should not be NULL", K(ret), K(task.tenant_id_), K(ls_id));
      } else if (OB_FAIL(get_split_addr(task, ls_id, split_replica_addrs))) {
        LOG_WARN("failed to get split addr", K(task));
      } else if (OB_FAIL(ls_service->get_ls(ls_id, ls_handle, ObLSGetMod::DDL_MOD))) {
        LOG_WARN("get ls failed", K(ret), K(ls_id));
      } else if (OB_FAIL(ObDDLUtil::ddl_get_tablet(ls_handle, task.tablet_id_, tablet_handle))) {
          LOG_WARN("get tablet failed", K(ret));
      } else if (OB_FAIL(tablet_handle.get_obj()->get_split_info_data(share::SCN::max_scn(),
          split_info_data, ObTabletCommon::DEFAULT_GET_TABLET_DURATION_10_S))) {
        LOG_WARN("failed to get split data", K(ret));
      } else if (OB_FAIL(construct_args_from_mds_data(ls_id, task, split_replica_addrs,
          split_info_data, args, addrs, split_task_status, allocator))) {
        LOG_WARN("failed to construct arg from mds data", K(ret));
      }
    }
  }

  if (OB_FAIL(ret)) {
  } else if (OB_UNLIKELY(args.count() != addrs.count() || args.count() != split_task_status.count())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(args), K(addrs), K(split_task_status));
  }
  for (int64_t i = 0; OB_SUCC(ret) && i < args.count(); ++i) {
    const ObAddr &addr = addrs.at(i);
    const obrpc::ObDDLBuildSingleReplicaRequestArg &arg = args.at(i);
    if (OB_FAIL(proxy.call(addr, rpc_timeout, arg.tenant_id_, arg))) {
      LOG_WARN("failed to send rpc", K(ret), K(addr), K(rpc_timeout), K(arg));
    } else {
      LOG_INFO("send build single replica request", K(addr), K(arg));
    }
  }
  int tmp_ret = OB_SUCCESS;
  common::ObArray<int> ret_array;
  if (OB_SUCCESS != (tmp_ret = proxy.wait_all(ret_array))) {
    LOG_WARN("rpc_proxy wait failed", K(ret), K(tmp_ret));
    ret = (OB_SUCCESS == ret) ? tmp_ret : ret;
  } else if (OB_SUCC(ret)) {
    const ObIArray<const obrpc::ObDDLBuildSingleReplicaRequestResult *> &result_array = proxy.get_results();
    if (OB_FAIL(process_rpc_results(args, addrs, result_array, ret_array, split_task_status))) {
      LOG_WARN("failed to process result", K(ret));
    }
  }
  return ret;
}

int ObLSTabletSplitScheduler::get_split_addr(const ObTabletSplitTask &split_task, ObLSID &ls_id, ObIArray<ObAddr> &split_replica_addrs)
{
  int ret = OB_SUCCESS;
  ObAddr unused_addr;
  if (OB_UNLIKELY(!split_task.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(split_task));
  } else if (OB_FAIL(ObDDLUtil::get_tablet_leader_addr(GCTX.location_service_, split_task.tenant_id_, split_task.tablet_id_, ObDDLUtil::get_default_ddl_rpc_timeout(), ls_id, unused_addr))) {
      LOG_WARN("get ls id failed", K(ret), K(split_task.tenant_id_), K(split_task.tablet_id_));
  } else if (split_task.task_status_ == TabletSplitTaskTatus::WAITING_SPLIT_DATA_COMPLEMENT) {
   ObSSWriterService *ss_writer_service = MTL(ObSSWriterService*);
   ObSSWriterKey key(ObSSWriterType::COMPACTION, ls_id, split_task.tablet_id_);
   ObSSWriterAddr sswriter_addr;
   //FIXME ly435438 figure out the appropriate timeout here
   const int64_t timeout_us = 100L * 1000L; //0.1s
   if (OB_FAIL(ss_writer_service->get_sswriter_addr(key, sswriter_addr, timeout_us))) {
    LOG_WARN("failed to get ss writer addr", K(ret), K(key), K(timeout_us));
   } else if (OB_FAIL(split_replica_addrs.push_back(sswriter_addr.addr_))) {
    LOG_WARN("failed to push back into split_replica_addrs", K(ret), K(sswriter_addr.addr_));
   }
  } else if (split_task.task_status_ == TabletSplitTaskTatus::WAITING_PRE_WARM) {
     if (OB_FAIL(ObDDLUtil::get_split_replicas_addrs(split_task.tenant_id_, ls_id, split_replica_addrs))) {
      LOG_WARN("get split replica addrs failed", K(ret), K(split_task.tenant_id_), K(ls_id));
    }
  } else {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("unexpected error", K(ret));
  }
  return ret;
}

int ObLSTabletSplitScheduler::process_rpc_results(const ObIArray<obrpc::ObDDLBuildSingleReplicaRequestArg> &args,
                                                  const ObIArray<ObAddr> &addrs,
                                                  const ObIArray<const obrpc::ObDDLBuildSingleReplicaRequestResult *> &result_array,
                                                  const ObIArray<int> &ret_array,
                                                  const ObIArray<TabletSplitTaskTatus> &split_task_status)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(args.count() != addrs.count() || args.count() != result_array.count()
      || args.count() != ret_array.count())) {
      ret = OB_INVALID_ARGUMENT;
      LOG_WARN("invalid argument", K(ret), K(args), K(addrs), K(result_array), K(ret_array));
  }
  ObSEArray<ObTabletSplitTask, 5> reschedule_tasks;
  int tmp_ret = OB_SUCCESS;
  int64_t cur_time = ObTimeUtility::current_time();
  ObTabletSplitTask reschedule_task;
  for (int64_t i = 0; OB_SUCC(ret) && i < result_array.count(); ++i) {
    int ret_code = ret_array.at(i);
    const obrpc::ObDDLBuildSingleReplicaRequestArg &arg = args.at(i);
    const obrpc::ObDDLBuildSingleReplicaRequestResult *res = result_array.at(i);
    if (OB_ISNULL(res)) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("unexpected null ptr of res", K(ret), K(res));
    } else {
      if (OB_SUCCESS != ret_code) {
        LOG_WARN("failed to schedule tablet split dag", K(ret_code), K(args.at(i)), K(result_array.at(i)));
      } else {
        LOG_INFO("success to schedule tablet split dag", K(ret_code), K(args.at(i)), K(result_array.at(i)));
      }
      reschedule_task.tenant_id_ = arg.tenant_id_;
      reschedule_task.tablet_id_ = arg.source_tablet_id_;
      reschedule_task.next_schedule_time_ = cur_time + SPLIT_TASK_CHECK_TIME_INTERVAL;
      if (TabletSplitTaskTatus::WAITING_SPLIT_DATA_COMPLEMENT == split_task_status.at(i)) {
        if (OB_SUCCESS == ret_code && res->is_data_split_finished_) {
          reschedule_task.task_status_ = TabletSplitTaskTatus::WAITING_PRE_WARM;
        } else {
          reschedule_task.task_status_ = TabletSplitTaskTatus::WAITING_SPLIT_DATA_COMPLEMENT;
        }
        if ((share::ObIDDLTask::in_ddl_retry_white_list(ret_code) || OB_SUCCESS == ret_code)
            && OB_FAIL(reschedule_tasks.push_back(reschedule_task))) {
          LOG_WARN("fail to push back into reschedule_tasks", K(ret), K(reschedule_task));
        }
      } else if (TabletSplitTaskTatus::WAITING_PRE_WARM == split_task_status.at(i)) {
        if (((OB_SUCCESS != ret_code && share::ObIDDLTask::in_ddl_retry_white_list(ret_code)) || !res->is_data_split_finished_)) {
          reschedule_task.task_status_ = TabletSplitTaskTatus::WAITING_PRE_WARM;
          if (OB_FAIL(reschedule_tasks.push_back(reschedule_task))) {
            LOG_WARN("fail to push back into reschedule_tasks", K(ret), K(reschedule_task));
          }
        }
      } else {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("unexpected split task status", K(ret), K(split_task_status.at(i)), K(arg));
      }
    }
  }
  if (OB_FAIL(ret)) {
  } else if (!reschedule_tasks.empty() && OB_FAIL(push_task(reschedule_tasks))) {
    LOG_WARN("failed to push reschedule_tasks", K(ret), K(reschedule_tasks));
  }
  return ret;
}

int ObLSTabletSplitScheduler::construct_args_from_mds_data(const ObLSID &ls_id,
                                                           const ObTabletSplitTask &split_task,
                                                           const ObIArray<ObAddr> &split_replica_addrs,
                                                           ObTabletSplitInfoMdsUserData &split_info_data,
                                                           ObIArray<obrpc::ObDDLBuildSingleReplicaRequestArg> &args,
                                                           ObIArray<ObAddr> &addrs,
                                                           ObIArray<TabletSplitTaskTatus> &split_task_status,
                                                           ObIAllocator &allocator)
{
  int ret = OB_SUCCESS;
  obrpc::ObDDLBuildSingleReplicaRequestArg arg;
  if (OB_UNLIKELY(!split_info_data.is_valid() || args.count() != addrs.count() || args.count() != split_task_status.count())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(split_info_data), K(args), K(addrs), K(split_task_status));
  } else if (OB_FAIL(arg.lob_col_idxs_.assign(split_info_data.lob_col_idxs_))) {
    LOG_WARN("failed to assign to lob col idxs", K(ret));
  } else {
    arg.tenant_id_ = split_task.tenant_id_;
    arg.dest_tenant_id_ =  split_task.tenant_id_;
    arg.source_tablet_id_ =  split_task.tablet_id_;
    arg.source_table_id_ = split_info_data.table_id_;
    arg.dest_schema_id_ = split_info_data.lob_col_idxs_.count() > 0 ? split_info_data.lob_table_id_ : split_info_data.table_id_; //dest_datable_id
    arg.schema_version_ = split_info_data.schema_version_;
    arg.dest_schema_version_ = split_info_data.dest_schema_version_;
    arg.snapshot_version_ = 1L;
    arg.ddl_type_ = split_info_data.task_type_;
    arg.task_id_ = split_info_data.task_id_;
    arg.execution_id_ = 1L;
    arg.tablet_task_id_ = split_info_data.tablet_task_id_;
    arg.data_format_version_ = split_info_data.data_format_version_;
    arg.consumer_group_id_ = split_info_data.consumer_group_id_;
    arg.can_reuse_macro_block_ = split_info_data.can_reuse_macro_block_;
    arg.split_sstable_type_ = share::ObSplitSSTableType::SPLIT_BOTH;
    arg.parallelism_ = split_info_data.parallelism_;
    arg.is_no_logging_ = split_info_data.is_no_logging_;
    arg.ls_id_ = ls_id;
    arg.dest_ls_id_ = ls_id;
    int64_t dest_tablets_count = split_info_data.dest_tablets_id_.count();
    int64_t replica_addrs_count = split_replica_addrs.count();
    for (int64_t i = 0; OB_SUCC(ret) && i < dest_tablets_count; ++i) {
      arg.dest_tablet_id_ = split_info_data.dest_tablets_id_.at(i);
      for (int64_t j = 0; OB_SUCC(ret) && j < replica_addrs_count; ++j) {
        int64_t abs_idx = args.count();
        if (OB_FAIL(args.push_back(arg))) {
          LOG_WARN("failed to push back into args", K(ret), K(arg));
        } else if (OB_FAIL(addrs.push_back(split_replica_addrs.at(j)))) {
          LOG_WARN("failed to push bakc into addrs", K(ret));
        } else if (OB_FAIL(split_task_status.push_back(split_task.task_status_))) {
          LOG_WARN("failed to push back into tenant_ids", K(ret), K(split_task.task_status_));
        } else if (OB_FAIL(args.at(abs_idx).parallel_datum_rowkey_list_.prepare_allocate(split_info_data.parallel_datum_rowkey_list_.count()))) {
          LOG_WARN("prepare allocate failed", K(ret), K(split_info_data));
        }
        for (int64_t k = 0; OB_SUCC(ret) && k < split_info_data.parallel_datum_rowkey_list_.count(); ++k) {
          if (OB_FAIL(split_info_data.parallel_datum_rowkey_list_.at(k).deep_copy(
                args.at(abs_idx).parallel_datum_rowkey_list_.at(k), allocator))) {
            LOG_WARN("failed to deep copy row_key", K(ret));
          }
        }
      }
    }
  }
  return ret;
}
ObLSTabletSplitScheduler &ObLSTabletSplitScheduler::get_instance()
{
  static ObLSTabletSplitScheduler instance;
  return instance;
}

int ObLSTabletSplitScheduler::is_split_src_tablet(ObTabletHandle &tablet_handle, bool &is_split_tablet)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!tablet_handle.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("tablet handl is invalid", K(ret));
  } else {
    ObTabletCreateDeleteMdsUserData user_data;
    const compaction::ObMediumCompactionInfoList *medium_info_list = nullptr;
    if (OB_FAIL(tablet_handle.get_obj()->ObITabletMdsInterface::get_tablet_status(share::SCN::max_scn(),
        user_data, ObTabletCommon::DEFAULT_GET_TABLET_DURATION_US))) {
      LOG_WARN("failed to get tablet status", K(ret), K(*tablet_handle.get_obj()));
    } else {
      is_split_tablet = ObTabletStatus::Status::SPLIT_SRC == user_data.get_tablet_status();
    }
  }
  return ret;
}
#endif

int ObSplitTaskPollingMgr::init()
{
  int ret = OB_SUCCESS;
  ObLockGuard<ObSpinLock> guard(lock_);
  if (OB_UNLIKELY(inited_)) {
    ret = OB_INIT_TWICE;
    LOG_WARN("try to init an inited ObSplitTaskPollingMgr", K(ret));
  } else if (OB_FAIL(map_tenant_to_cache_.create(ObSplitTaskPollingMgr::INITIAL_TENANT_COUNT, ObMemAttr(OB_SERVER_TENANT_ID, "spl_task_map")))) {
    LOG_WARN("fail to create map_tenant_to_cache_", K(ret));
  } else {
    inited_ = true;
    polling_mgr_malloc_.set_attr(ObMemAttr(OB_SERVER_TENANT_ID, "spl_task_mal"));
  }
  return ret;
}

void ObSplitTaskPollingMgr::reset()
{
  ObLockGuard<ObSpinLock> guard(lock_);
  for (hash::ObHashMap<uint64_t, ObSplitTaskCache*>::iterator iter = map_tenant_to_cache_.begin(); iter != map_tenant_to_cache_.end(); iter++) {
    uint64_t tenant_id = iter->first;
    ObSplitTaskCache *&tenant_cache = iter->second;
    if (OB_NOT_NULL(tenant_cache)) {
      tenant_cache->~ObSplitTaskCache();
      polling_mgr_malloc_.free(tenant_cache);
      tenant_cache = nullptr;
    }
  }
  (void) map_tenant_to_cache_.destroy();
  polling_mgr_malloc_.reset();
  inited_ = false;
  total_tasks_ = 0;
}

int ObSplitTaskPollingMgr::get_task_cache(const int tenant_id, ObSplitTaskCache *&tenant_cache)
{
  int ret = OB_SUCCESS;
  tenant_cache = nullptr;
  int64_t tenant_cache_idx = OB_INVALID_INDEX;
  if (OB_UNLIKELY(!inited_)) {
    ret = OB_NOT_INIT;
    LOG_WARN("not init", K(ret), K(inited_));
  } else if (OB_INVALID_TENANT_ID == tenant_id) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(tenant_id));
  } else if (OB_FAIL(map_tenant_to_cache_.get_refactored(tenant_id, tenant_cache))) {
    LOG_WARN("get tenant cache index failed", K(ret), K(tenant_id));
  }
  if (OB_SUCC(ret) && OB_ISNULL(tenant_cache)) {
    ret = OB_NULL_CHECK_ERROR;
    LOG_WARN("unexpected of null ptr of tenant_cache", K(ret), KP(tenant_cache));
    int tmp_ret = OB_SUCCESS;
    if (OB_TMP_FAIL(map_tenant_to_cache_.erase_refactored(tenant_id))) {
      LOG_WARN("failed to remove tenant cache from map_tenant_to_cache_", K(ret));
    }
  }
  return ret;
}

int ObSplitTaskPollingMgr::push_tasks_(const ObIArray<const ObSplitTask*> &task_array, uint64_t tenant_id, ObSplitTaskCache *task_cache)
{
  int ret = OB_SUCCESS;
  int64_t cache_total_task_old = OB_INVALID_SIZE;
  if (OB_ISNULL(task_cache)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("unexpted null ptr of tenant_cache", K(ret), KP(task_cache), K(tenant_id), K(is_root_server_), K(MTL_ID()));
    int tmp_ret = OB_SUCCESS;
    if (OB_TMP_FAIL(map_tenant_to_cache_.erase_refactored(tenant_id))) {
      LOG_WARN("erase from tenantid_idx failed", K(tmp_ret));
    }
  } else if (OB_FALSE_IT(cache_total_task_old = task_cache->get_tasks_num())) {
  } else if (OB_FAIL(task_cache->push_tasks(task_array))) {
    LOG_WARN("push tasks into tenant cache failed", K(ret));
  }
  if (OB_NOT_NULL(task_cache) && cache_total_task_old != OB_INVALID_SIZE) {
    const int64_t cache_total_task_new = task_cache->get_tasks_num();
    if (OB_LIKELY(cache_total_task_old >= 0 && cache_total_task_new >=0 && cache_total_task_old < cache_total_task_new)) {
      (void) ATOMIC_FAA(&total_tasks_, cache_total_task_new - cache_total_task_old);
    }
  }
  return ret;
}

int ObSplitTaskPollingMgr::gc_deleted_tenant_caches()
{
  int ret = OB_SUCCESS;
  ObLockGuard<ObSpinLock> guard(lock_);
  if (OB_UNLIKELY(!inited_)) {
    ret = OB_NOT_INIT;
    LOG_WARN("not init", K(ret), K(inited_));
  }
  if (OB_SUCC(ret) && is_root_server_ && REACH_TIME_INTERVAL(60L * 60L * 1000L * 1000L)/*1h*/) {
    ObSchemaGetterGuard schema_guard;
    ObSEArray<uint64_t, 10> tenant_ids;
    common::hash::ObHashSet<uint64_t> existed_tenants_set;

    if (OB_ISNULL(GCTX.schema_service_)) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("schema service should not be null", K(ret), K(GCTX.schema_service_));
    } else if (OB_FAIL(GCTX.schema_service_->get_tenant_schema_guard(OB_SYS_TENANT_ID, schema_guard))) {
      LOG_WARN("get_schema_guard failed", K(ret));
    } else if (OB_FAIL(schema_guard.get_tenant_ids(tenant_ids))) {
      LOG_WARN("failed to get all tenant ids", K(ret));
    } else if (OB_FAIL(existed_tenants_set.create(5, ObMemAttr(OB_SERVER_TENANT_ID, "as_ten_set")))) {
      LOG_WARN("failed to create hash set", K(ret));
    } else {
      for (int64_t i = 0; OB_SUCC(ret) && i < tenant_ids.count(); ++i) {
        if (OB_FAIL(existed_tenants_set.set_refactored(tenant_ids.at(i)))) {
          LOG_WARN("failed to push into task set", K(ret), K(i), K(tenant_ids));
        }
      }
      if (OB_FAIL(ret)) {
      } else {
        GcTenantCacheOperator tc_op(existed_tenants_set);
        if (OB_FAIL(map_tenant_to_cache_.foreach_refactored(tc_op))) {
          LOG_WARN("failed to do for each refactored", K(ret));
        } else {
          ObSEArray<oceanbase::common::hash::HashMapPair<uint64_t, ObSplitTaskCache*>, 1> &needed_gc_tenant_caches = tc_op.needed_gc_tenant_caches_;
          for (int64_t i = 0; OB_SUCC(ret) && i < needed_gc_tenant_caches.count(); ++i) {
            oceanbase::common::hash::HashMapPair<uint64_t, ObSplitTaskCache*> &pair = needed_gc_tenant_caches.at(i);
            uint64_t tenant_id = pair.first;
            ObSplitTaskCache *&task_cache = pair.second;
            if (OB_FAIL(map_tenant_to_cache_.erase_refactored(tenant_id))) {
              LOG_WARN("failed to erase tenant cache from map_tenant_to_cache_", K(ret), K(tenant_id));
            } else if (OB_ISNULL(task_cache)) {
              ret = OB_ERR_UNEXPECTED;
              LOG_WARN("tenant cache ptr should not be null", K(ret), K(task_cache));
            } else {
              task_cache->~ObSplitTaskCache();
              polling_mgr_malloc_.free(task_cache);
              task_cache = nullptr;
            }
          }
        }
      }
    }
  }
  return ret;
}

//it's ok to return task_cache as null, since observer thread relies tenant context to get task cache
int ObSplitTaskPollingMgr::get_or_create_task_cache(uint64_t tenant_id, ObSplitTaskCache *&task_cache)
{
  int ret = OB_SUCCESS;
  ObAutoSplitTaskCache *auto_split_task_cache = nullptr;
  if (OB_INVALID_TENANT_ID == tenant_id) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(tenant_id));
  } else if (OB_FALSE_IT(task_cache = nullptr)) {
  } else if (OB_FAIL(map_tenant_to_cache_.get_refactored(tenant_id, task_cache))) {
    if (OB_HASH_NOT_EXIST == ret) {
      //overwrite ret
      ret = OB_SUCCESS;
      if (is_root_server_ && OB_FAIL(create_auto_split_task_cache(tenant_id, OB_SERVER_TENANT_ID, auto_split_task_cache))) {
        LOG_WARN("failed to create task_cache ", K(ret), K(tenant_id));
      } else if (is_root_server_ && OB_ISNULL(task_cache = static_cast<ObSplitTaskCache *>(auto_split_task_cache))) {
        LOG_WARN("unexpected nullptr of task_cache", K(ret), K(task_cache));
        // notice: only RS thread relies on pooling mgr to create and hold task cache
        // observer thread relies tenant context to get task cache, so it's ok to let the tenant_cache to be null here for observer.
      } else if (OB_FAIL(register_task_cache(tenant_id, task_cache))) {
        LOG_WARN("failed to register task_cache ", K(ret), K(tenant_id), KPC(task_cache));
      }
    } else {
      LOG_WARN("failed to get task_cache from map_tenant_to_cache_", K(ret), K(tenant_id));
    }
    if (OB_FAIL(ret) && OB_NOT_NULL(auto_split_task_cache)) {
      auto_split_task_cache->~ObAutoSplitTaskCache();
      polling_mgr_malloc_.free(auto_split_task_cache);
      auto_split_task_cache = nullptr;
      task_cache = nullptr;
    }
  }
  return ret;
}


int ObSplitTaskPollingMgr::GcTenantCacheOperator::operator() (oceanbase::common::hash::HashMapPair<uint64_t, ObSplitTaskCache*> &entry)
{
  int ret = OB_SUCCESS;
  uint64_t tenant_id = entry.first;
  ObSplitTaskCache *task_cache = entry.second;
  if (OB_ISNULL(task_cache)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("tenant cache ptr should not be null", K(ret), K(task_cache));
  } else if (OB_UNLIKELY(OB_HASH_NOT_EXIST == existed_tenants_set_.exist_refactored(tenant_id)) &&
      OB_FAIL(needed_gc_tenant_caches_.push_back(entry))) {
    LOG_WARN("failed to push back into needed_gc_tenant_caches_", K(ret));
  }
  return ret;
}

int ObSplitTaskPollingMgr::pop_tasks_from_tenant_cache(const int64_t num_tasks_to_pop,
                                                       ObIArray<ObSplitTask*> &task_array,
                                                       ObSplitTaskCache *tenant_cache,
                                                       ObIAllocator &allocator)
{
  int ret = OB_SUCCESS;
  task_array.reuse();
  int64_t tenant_cache_idx = OB_INVALID_INDEX;
  if (OB_UNLIKELY(!inited_)) {
    ret = OB_NOT_INIT;
    LOG_WARN("not init", K(ret), K(inited_));
  } else if (num_tasks_to_pop <= 0 || OB_ISNULL(tenant_cache)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(num_tasks_to_pop), KP(tenant_cache));
  } else {
    int64_t cache_total_task_old = OB_INVALID_SIZE;
    if (OB_FALSE_IT(cache_total_task_old = tenant_cache->get_tasks_num())) {
    } else if (OB_FAIL(tenant_cache->pop_tasks(num_tasks_to_pop, task_array, allocator))) {
      if (OB_ENTRY_NOT_EXIST == ret) {
        LOG_DEBUG("trying to pop from empty tenant cache", K(ret));
        //overwrite ret
        ret = OB_SUCCESS;
      } else {
        LOG_WARN("pop tasks from tenant's cache failed", K(ret));
      }
    }
    if (OB_NOT_NULL(tenant_cache) && cache_total_task_old != OB_INVALID_SIZE) {
      const int64_t cache_total_task_new = tenant_cache->get_tasks_num();
      if (OB_LIKELY(cache_total_task_old >= 0 && cache_total_task_new >= 0 && cache_total_task_old > cache_total_task_new)) {
        (void) ATOMIC_FAA(&total_tasks_, cache_total_task_new - cache_total_task_old);
      }
    }
  }
  return ret;
}

int ObSplitTaskPollingMgr::pop_tasks(const int64_t num_tasks_can_pop, const bool need_to_prior_pop, ObIArray<ObSEArray<ObSplitTask*, 10>> &task_array, ObIAllocator &allocator) {
  int ret = OB_SUCCESS;
  task_array.reuse();
  ObSEArray<ObSplitTask*, 10> tmp_array;
  ObLockGuard<ObSpinLock> guard(lock_);
  int64_t total_tasks_pop_budge = num_tasks_can_pop;
  int tmp_ret = OB_SUCCESS;
  if (OB_UNLIKELY(!inited_)) {
    ret = OB_NOT_INIT;
    LOG_WARN("not init", K(ret), K(inited_));
  } else if (num_tasks_can_pop <= 0) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(num_tasks_can_pop));
  } else if (OB_UNLIKELY(get_total_tenants() == 0)) {
    ret = OB_ENTRY_NOT_EXIST;
    LOG_DEBUG("no task exist", K(ret));
  } else if ((need_to_prior_pop) &&
      OB_TMP_FAIL(pop_tasks_from_tenant_cache(1/*num_tasks_to_pop*/, tmp_array, MTL(ObAutoSplitTaskCache*), allocator))) {
  } else if (tmp_array.count() == 0) {
  } else if (tmp_array.count() > 0 && OB_TMP_FAIL(task_array.push_back(tmp_array))) {
    LOG_WARN("push task into task array failed", K(tmp_ret));
  } else {
    --total_tasks_pop_budge;
  }
  if (OB_SUCC(ret) && OB_LIKELY(total_tasks_pop_budge > 0)) {
    ObArray<uint64_t> tenants_id;
    for (hash::ObHashMap<uint64_t, ObSplitTaskCache*>::iterator iter = map_tenant_to_cache_.begin(); OB_SUCC(ret) && iter != map_tenant_to_cache_.end(); iter++) {
      uint64_t tenant_id = iter->first;
      if (OB_FAIL(tenants_id.push_back(tenant_id))) {
        LOG_WARN("failed to push task into tenants_id", K(ret));
      }
    }
    int64_t tasks_budget_per_tenant = max(total_tasks_pop_budge / get_total_tenants(), 1);
    int64_t tasks_pop_this_round = (total_tasks_pop_budge / get_total_tenants() == 0) ? 0 : total_tasks_pop_budge % get_total_tenants();
    total_tasks_pop_budge -= tasks_pop_this_round;
    if (tasks_budget_per_tenant == 1) {
      std::random_shuffle(tenants_id.begin(), tenants_id.end());
    }
    for (int64_t i = 0; OB_SUCC(ret) && (tasks_pop_this_round > 0 || total_tasks_pop_budge > 0) && i < tenants_id.size(); ++i) {
      int tmp_ret = OB_SUCCESS;
      int64_t tenant_id = tenants_id.at(i);
      ObSplitTaskCache * tenant_cache = nullptr;
      tmp_array.reuse();
      if (total_tasks_pop_budge > 0) {
        total_tasks_pop_budge -= tasks_budget_per_tenant;
        tasks_pop_this_round += tasks_budget_per_tenant;
      }
      if (!is_root_server_) {
        MAKE_TENANT_SWITCH_SCOPE_GUARD(guard);
        if (OB_TMP_FAIL(guard.switch_to(tenant_id, false /*need_check_allow*/))) {
          LOG_WARN("failed to switch to tenant", K(tmp_ret), K(tenant_id));
          tmp_ret = OB_SUCCESS;
          if (OB_TMP_FAIL(map_tenant_to_cache_.erase_refactored(tenant_id))) {
            LOG_WARN("failed to remove cache from map_tenant_to_cache_", K(tmp_ret), K(tenant_id));
          }
        } else {
          switch (cache_type_) {
            case ObSplitCacheType::AUTO_SPLIT_CACHE_TYPE: {
              tenant_cache = MTL(ObAutoSplitTaskCache*);
              break;
            }
#ifdef OB_BUILD_SHARED_STORAGE
            case ObSplitCacheType::TABLET_SPLIT_CACHE_TYPE: {
              tenant_cache = MTL(ObTabletSplitTaskCache*);
              break;
            }
#endif
            default : {
              ret = OB_ERR_UNEXPECTED;
              LOG_WARN("can't get the task cache type", K(ret), K(cache_type_));
            }
          }
          if (OB_FAIL(ret) || OB_TMP_FAIL(tmp_ret)) {
          } else if (OB_TMP_FAIL(pop_tasks_from_tenant_cache(tasks_pop_this_round, tmp_array, tenant_cache, allocator))) {
            LOG_WARN("failed to pop tasks from tenant cache", K(tmp_ret));
          }
        }
      } else if (OB_TMP_FAIL(get_task_cache(tenant_id, tenant_cache))) {
        LOG_WARN("get tenant cache failed", K(tmp_ret), K(tenant_id));
      } else if (OB_TMP_FAIL(pop_tasks_from_tenant_cache(tasks_pop_this_round, tmp_array, tenant_cache, allocator))) {
        LOG_WARN("failed to pop tasks from tenant cache", K(tmp_ret));
      }
      if (OB_FAIL(ret) || OB_TMP_FAIL(tmp_ret)) {
      } else {
        tasks_pop_this_round -= tmp_array.count();
        if (OB_FAIL(tmp_array.count() > 0 && OB_FAIL(task_array.push_back(tmp_array)))) {
          LOG_WARN("failed to push tasks into task_array", K(ret));
        }
      }
    }
  }
  return ret;
}

int ObSplitTaskPollingMgr::push_tasks(const ObIArray<const ObSplitTask*> &task_array)
{
  int ret = OB_SUCCESS;
  ObLockGuard<ObSpinLock> guard(lock_);
  uint64_t tenant_id = OB_INVALID_TENANT_ID;

  if (OB_UNLIKELY(!inited_)) {
    ret = OB_NOT_INIT;
    LOG_WARN("not init", K(ret), K(inited_));
  } else if (task_array.count() == 0) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(task_array));
  } else {
    // ensure all valid task share the same tenant id and ignore invalid task
    for (int64_t i = 0; OB_SUCC(ret) && i < task_array.count(); ++i) {
      const ObSplitTask *task = task_array.at(i);
      if (OB_ISNULL(task)) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("unexpected error of nullptr", K(ret));
      } else if (OB_UNLIKELY(!task->is_valid())) {
        //ignore ret
        int tmp_ret = OB_INVALID_ARGUMENT;
        LOG_WARN("invalid argument", K(tmp_ret));
      } else if (OB_INVALID_TENANT_ID == tenant_id && FALSE_IT(tenant_id = task->get_tenant_id())) {
      } else if (OB_UNLIKELY(task->get_tenant_id() != tenant_id)) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("tenant ids don't match", K(ret), K(task->get_tenant_id()), K(tenant_id));
      }
    }
    ObSplitTaskCache *task_cache = nullptr;
    if (OB_FAIL(ret)) {
    } else if (OB_FAIL(get_or_create_task_cache(tenant_id, task_cache))) {
      LOG_WARN("failed to get or create task cache", K(ret), K(tenant_id));
    }
    if (OB_FAIL(ret)) {
    } else {
      if (is_root_server_) {
        if (OB_FAIL(push_tasks_(task_array, tenant_id, task_cache))) {
          LOG_WARN("failed to push tasks", K(ret), K(task_array), K(tenant_id));
        }
      } else {
        MTL_SWITCH(tenant_id) {
          switch (cache_type_) {
            case ObSplitCacheType::AUTO_SPLIT_CACHE_TYPE: {
              task_cache = MTL(ObAutoSplitTaskCache*);
              break;
            }
#ifdef OB_BUILD_SHARED_STORAGE
            case ObSplitCacheType::TABLET_SPLIT_CACHE_TYPE: {
              task_cache = MTL(ObTabletSplitTaskCache*);
              break;
            }
#endif
            default : {
              ret = OB_ERR_UNEXPECTED;
              LOG_WARN("can't get the task cache type", K(ret), K(cache_type_));
            }
          }
          if (OB_FAIL(ret)) {
          } else if (OB_FAIL(push_tasks_(task_array, tenant_id, task_cache))) {
            LOG_WARN("failed to push tasks", K(ret), K(task_array), K(tenant_id));
          }
        }
      }
    }
  }
  return ret;
}

int ObSplitTaskPollingMgr::create_auto_split_task_cache(const uint64_t tenant_id, const uint64_t host_tenant_id, ObAutoSplitTaskCache *&tenant_cache)
{
  int ret = OB_SUCCESS;
  void *buf = nullptr;
  tenant_cache = nullptr;
  if (OB_UNLIKELY(!inited_)) {
    ret = OB_NOT_INIT;
    LOG_WARN("not init", K(ret), K(inited_));
  } else if (OB_UNLIKELY(OB_INVALID_TENANT_ID == tenant_id || OB_INVALID_TENANT_ID == host_tenant_id)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(tenant_id), K(host_tenant_id));
  } else if (OB_ISNULL(buf = polling_mgr_malloc_.alloc(sizeof(ObAutoSplitTaskCache)))) {
    ret = OB_ALLOCATE_MEMORY_FAILED;
    LOG_WARN("allocate memory failed", K(ret), KP(buf));
  } else if (FALSE_IT(tenant_cache = new (buf) ObAutoSplitTaskCache())) {
  } else if (OB_FAIL(tenant_cache->init(ObAutoSplitTaskCache::CACHE_MAX_CAPACITY, tenant_id, host_tenant_id))) {
    LOG_WARN("failed to init ", K(ret));
  }
  if (OB_FAIL(ret) && OB_NOT_NULL(tenant_cache)) {
    tenant_cache->destroy();
    polling_mgr_malloc_.free(tenant_cache);
    tenant_cache = nullptr;
  }
  return ret;
}

int ObSplitTaskPollingMgr::register_task_cache(const uint64_t tenant_id, ObSplitTaskCache * const tenant_cache)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!inited_)) {
    ret = OB_NOT_INIT;
    LOG_WARN("not init", K(ret), K(inited_));
  } else if (OB_UNLIKELY(OB_INVALID_TENANT_ID == tenant_id)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", K(ret), K(tenant_id));
  } else if (OB_FAIL(map_tenant_to_cache_.set_refactored(tenant_id, tenant_cache))) {
    LOG_WARN("push into map_tenant_to_cache_ failed", K(ret));
  }
  return ret;
}

int ObAutoSplitArgBuilder::build_arg(const uint64_t tenant_id,
                                     const share::ObLSID ls_id,
                                     const ObTabletID tablet_id,
                                     const int64_t auto_split_tablet_size,
                                     const int64_t used_disk_space,
                                     obrpc::ObAlterTableArg &arg)
{
  int ret = OB_SUCCESS;
  const share::schema::ObTableSchema *table_schema = nullptr;
  const share::schema::ObSimpleDatabaseSchema *db_schema = nullptr;
  ObSplitSampler sampler;
  ObArray<common::ObNewRange> ranges;
  common::ObArenaAllocator range_allocator;
  share::schema::ObSchemaGetterGuard guard;
  int64_t ranges_num = 0;
  arg.reset();

  if (tenant_id == OB_INVALID_ID || !ls_id.is_valid() || !tablet_id.is_valid() ||
      auto_split_tablet_size <= 0 || used_disk_space <= 0 ||
      used_disk_space < auto_split_tablet_size ) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(tenant_id), K(ls_id), K(tablet_id),
                                 K(auto_split_tablet_size), K(used_disk_space));
  } else if (FALSE_IT(ranges_num = (used_disk_space / auto_split_tablet_size +
                                    (used_disk_space % auto_split_tablet_size == 0 ? 0 : 1)))) {
  } else if (FALSE_IT(ranges_num = MAX_SPLIT_PARTITION_NUM > ranges_num ?
                                   ranges_num : MAX_SPLIT_PARTITION_NUM)) {
  } else if (OB_FAIL(acquire_schema_info_of_tablet_(tenant_id, tablet_id, table_schema, db_schema, guard, arg))) {
    LOG_WARN("fail to acquire schema info of tablet", KR(ret), K(tenant_id), K(tablet_id));
  } else if (OB_ISNULL(table_schema) || OB_ISNULL(db_schema)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("unexpected null", KR(ret), K(tablet_id), KPC(table_schema), KPC(db_schema));
  } else if (OB_UNLIKELY(!table_schema->is_auto_partitioned_table())) {
    ret = OB_OP_NOT_ALLOW;
    LOG_WARN("attempt to auto split tablet of a non-auto-partitioned table", KR(ret), KPC(table_schema));
  } else if (OB_FAIL(table_schema->check_validity_for_auto_partition())) {
    LOG_WARN("table is invalid for auto partition", KR(ret), K(tenant_id), K(tablet_id), KPC(table_schema));
  } else if (OB_FAIL(sampler.query_ranges(tenant_id,
                                          db_schema->get_database_name_str(),
                                          *table_schema,
                                          tablet_id,
                                          ranges_num,
                                          used_disk_space,
                                          range_allocator,
                                          ranges))) {
    LOG_WARN("fail to acquire ranges for split partition", KR(ret));
  } else if (OB_UNLIKELY(ranges.empty())) { // fail to sample
    ret = OB_EAGAIN;
    LOG_WARN("partition is empty or all data have same partition key", KR(ret));
  } else {
    if (OB_FAIL(build_arg_(tenant_id, db_schema->get_database_name_str(),
                            *table_schema, tablet_id, ranges, arg))) {
      LOG_WARN("fail to build split arg", KR(ret), K(tenant_id), KPC(db_schema),
                                          KPC(table_schema), K(tablet_id), K(ranges));
    }
  }

  return ret;
}

int ObAutoSplitArgBuilder::acquire_schema_info_of_tablet_(const uint64_t tenant_id,
                                                          const ObTabletID tablet_id,
                                                          const share::schema::ObTableSchema *&table_schema,
                                                          const share::schema::ObSimpleDatabaseSchema *&db_schema,
                                                          share::schema::ObSchemaGetterGuard &guard,
                                                          obrpc::ObAlterTableArg &arg)
{
  int ret = OB_SUCCESS;
  share::schema::ObMultiVersionSchemaService *schema_service = GCTX.schema_service_;
  uint64_t table_id = OB_INVALID_ID;
  uint64_t db_id = OB_INVALID_ID;
  ObSEArray<ObTabletID, 1> tablet_ids;
  ObSEArray<uint64_t, 1> table_ids;
  if (OB_ISNULL(schema_service)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("unexpected null", KR(ret), K(tenant_id), K(tablet_id));
  } else if (OB_FAIL(tablet_ids.push_back(tablet_id))) {
    LOG_WARN("failed to push back", K(ret), K(tablet_id));
  } else if (OB_FAIL(acquire_table_id_of_tablets(tenant_id, tablet_ids, table_ids))) {
    LOG_WARN("fail to acquire tablet info", KR(ret), K(tablet_id));
  } else if (table_ids.count() != 1) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("unexpected count of the table_ids", K(ret), K(table_ids));
  } else if (OB_FALSE_IT(table_id = table_ids.at(0))) {
  } else if (OB_FAIL(schema_service->get_tenant_schema_guard(tenant_id, guard))){
    LOG_WARN("fail to get tenant schema guard", KR(ret), K(tenant_id));
  } else if (OB_FAIL(guard.get_table_schema(tenant_id, table_id, table_schema))){
    LOG_WARN("fail to get table schema", KR(ret), K(tenant_id), K(table_id), K(tablet_id));
  } else if (OB_ISNULL(table_schema)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("unexpected null", KR(ret), K(tenant_id), K(table_id), K(tablet_id));
  } else if (OB_FAIL(arg.based_schema_object_infos_.push_back(ObBasedSchemaObjectInfo(table_schema->get_table_id(),
      schema::TABLE_SCHEMA, table_schema->get_schema_version(), table_schema->get_tenant_id())))) {
    LOG_WARN("fail to push back into based_schema_object_infos_", K(ret));
  } else if (FALSE_IT(db_id = table_schema->get_database_id())){
  } else if (OB_FAIL(guard.get_database_schema(tenant_id, db_id, db_schema))) {
    LOG_WARN("fail to get database schema", KR(ret), K(tenant_id), K(table_id), K(tablet_id));
  } else if (OB_ISNULL(db_schema)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("unexpected null", KR(ret), K(tenant_id), K(db_id), K(table_id), K(tablet_id));
  }
  return ret;
}

int ObAutoSplitArgBuilder::acquire_table_id_of_tablets(const uint64_t tenant_id,
                                                       const ObIArray<ObTabletID> &tablet_ids,
                                                       ObIArray<uint64_t> &table_ids)
{
  int ret = OB_SUCCESS;
  ObSqlString sql;
  ObMySQLProxy *mysql_proxy = GCTX.sql_proxy_;
  table_ids.reuse();
  if (OB_UNLIKELY(tablet_ids.count() > ObDDLUtil::MAX_BATCH_COUNT)) {
    ret = OB_NOT_SUPPORTED;
    LOG_WARN("batch size should not be grater than ObDDLUtil::MAX_BATCH_COUNT(128)", K(ret), K(tablet_ids));
  }
  for (int64_t i = 0; OB_SUCC(ret) && i < tablet_ids.count(); ++i) {
    if (OB_UNLIKELY(!tablet_ids.at(i).is_valid())) {
      ret = OB_INVALID_ARGUMENT;
      LOG_WARN("invalid argument", K(ret), K(tablet_ids));
    }
  }
  SMART_VAR(ObMySQLProxy::MySQLResult, res) {
    sqlclient::ObMySQLResult* sql_result = nullptr;
    if (OB_ISNULL(mysql_proxy)) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("unexpected null", KR(ret));
    } else if (OB_FAIL(sql.assign_fmt("SELECT table_id, tablet_id FROM oceanbase.%s "
                                      "WHERE tablet_id in (",
                                      share::OB_ALL_TABLET_TO_LS_TNAME))) {
      LOG_WARN("failed to assign sql", KR(ret));
    } else {
      for (int64_t idx = 0; OB_SUCC(ret) && (idx < tablet_ids.count()); ++idx) {
        if (OB_FAIL(sql.append_fmt(
          "%lu%s",
          tablet_ids.at(idx).id(),
          ((idx == tablet_ids.count() - 1) ? ")" : ",")))) {
          LOG_WARN("assign sql string failed", K(ret), K(tenant_id), K(tablet_ids.at(idx).id()));
        }
      }
      if (OB_FAIL(ret)) {
      } else if (OB_FAIL(mysql_proxy->read(res, tenant_id, sql.ptr()))) {
        LOG_WARN("execute sql failed", KR(ret), K(sql));
      } else if (OB_ISNULL(sql_result = res.get_result())) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("result is null", KR(ret), K(sql));
      } else if (OB_FAIL(table_ids.prepare_allocate(tablet_ids.count()))) {
        LOG_WARN("failed to prepare_allocate", K(ret), K(tablet_ids.count()));
      } else if (table_ids.count() != tablet_ids.count()) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("unexpected diff length of table_ids and tablet_ids", K(ret), K(table_ids), K(tablet_ids));
      } else {
        while (OB_SUCC(ret)) {
          uint64_t table_id = OB_INVALID_ID;
          uint64_t tablet_id = OB_INVALID_ID;
          if (OB_FAIL(sql_result->next())) {
            if (OB_ITER_END && table_ids.count() == tablet_ids.count()) {
              //overwrite ret
              ret = OB_SUCCESS;
              break;
            } else {
              LOG_WARN("failed to get next result", K(ret), K(table_ids), K(tablet_ids));
            }
          } else {
            EXTRACT_INT_FIELD_MYSQL(*sql_result, "table_id", table_id, uint64_t);
            EXTRACT_INT_FIELD_MYSQL(*sql_result, "tablet_id", tablet_id, uint64_t);
            for (int64_t i = 0; i < tablet_ids.count(); ++i) {
              if (tablet_ids.at(i).id() == tablet_id) {
                table_ids.at(i) = table_id;
              }
            }
          }
        }
      }
    }
  }
  return ret;
}

int ObAutoSplitArgBuilder::build_arg_(const uint64_t tenant_id,
                                      const ObString &db_name,
                                      const share::schema::ObTableSchema &table_schema,
                                      const ObTabletID split_source_tablet_id,
                                      const ObArray<ObNewRange> &ranges,
                                      obrpc::ObAlterTableArg &arg)
{
  int ret = OB_SUCCESS;
  ObTZMapWrap tz_map_wrap;
  share::schema::AlterTableSchema& alter_table_schema = arg.alter_table_schema_;
  if (tenant_id == OB_INVALID_ID) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("invalid tenant_id", KR(ret), K(tenant_id));
  } else if (OB_FAIL(build_alter_table_schema_(tenant_id, db_name, table_schema,
                                               split_source_tablet_id,
                                               ranges,
                                               arg.tz_info_wrap_.get_time_zone_info(),
                                               alter_table_schema))) {
    LOG_WARN("fail to build alter_table_schema", KR(ret), K(tenant_id), K(db_name),
                                                 K(table_schema), K(split_source_tablet_id),
                                                 K(ranges));
  } else if (OB_FAIL(OTTZ_MGR.get_tenant_tz(tenant_id, tz_map_wrap))) {
    LOG_WARN("get tenant timezone map failed", KR(ret), K(tenant_id));
  } else {
    arg.alter_part_type_ = obrpc::ObAlterTableArg::AlterPartitionType::AUTO_SPLIT_PARTITION;
    arg.exec_tenant_id_ = tenant_id;
    arg.is_alter_partitions_ = true;
    arg.is_inner_ = true;
    arg.is_add_to_scheduler_ = false;
    arg.tz_info_wrap_.set_tz_info_offset(0);
    arg.nls_formats_[ObNLSFormatEnum::NLS_DATE] = ObTimeConverter::COMPAT_OLD_NLS_DATE_FORMAT;
    arg.nls_formats_[ObNLSFormatEnum::NLS_TIMESTAMP] = ObTimeConverter::COMPAT_OLD_NLS_TIMESTAMP_FORMAT;
    arg.nls_formats_[ObNLSFormatEnum::NLS_TIMESTAMP_TZ] = ObTimeConverter::COMPAT_OLD_NLS_TIMESTAMP_TZ_FORMAT;
    arg.set_tz_info_map(tz_map_wrap.get_tz_map());
    if (table_schema.is_user_table() && OB_FAIL(build_ddl_stmt_str_(table_schema, alter_table_schema, split_source_tablet_id, arg.tz_info_wrap_.get_time_zone_info(), arg.allocator_, arg.ddl_stmt_str_))) {
      LOG_WARN("failed to build ddl stmt str", K(ret), K(tenant_id), K(table_schema.get_table_id()), K(split_source_tablet_id));
    }
  }
  return ret;
}

int ObAutoSplitArgBuilder::print_identifier(
    ObIAllocator &allocator,
    const bool is_oracle_mode,
    const ObString &name,
    ObString &ident)
{
  int ret = OB_SUCCESS;
  int64_t buf_len = OB_MAX_TEXT_LENGTH;
  int64_t pos = 0;
  char *buf = nullptr;
  const char *quote = is_oracle_mode ? "\"" : "`";
  if (OB_ISNULL(buf = static_cast<char *>(allocator.alloc(buf_len)))) {
    ret = OB_ALLOCATE_MEMORY_FAILED;
    LOG_WARN("failed to alloc", KR(ret));
  } else if (OB_FAIL(databuff_printf(buf, buf_len, pos, ObString(quote)))) {
    LOG_WARN("failed to print quote", K(ret));
  } else if (OB_FAIL(sql::ObSQLUtils::print_identifier(buf, buf_len, pos, CS_TYPE_UTF8MB4_GENERAL_CI, name, is_oracle_mode))) {
    LOG_WARN("print partition name failed", K(ret), K(name), K(is_oracle_mode));
  } else if (OB_FAIL(databuff_printf(buf, buf_len, pos, ObString(quote)))) {
    LOG_WARN("failed to print quote", K(ret));
  } else {
    ident.assign_ptr(buf, pos);
  }
  return ret;
}

int ObAutoSplitArgBuilder::convert_rowkey_to_sql_literal(
    const ObRowkey &rowkey,
    const bool is_oracle_mode,
    const ObTimeZoneInfo *tz_info,
    ObIAllocator &allocator,
    ObString &rowkey_str)
{
  int ret = OB_SUCCESS;
  char *buf = NULL;
  const int64_t buf_len = OB_MAX_B_HIGH_BOUND_VAL_LENGTH;
  int64_t pos = 0;
  if (OB_ISNULL(buf = static_cast<char *>(allocator.alloc(buf_len)))) {
    ret = OB_ALLOCATE_MEMORY_FAILED;
    LOG_WARN("failed to alloc", K(ret), K(buf_len));
  } else if (OB_FAIL(ObPartitionUtils::convert_rowkey_to_sql_literal(is_oracle_mode, rowkey, buf, buf_len, pos, false/*print_collation*/, tz_info))) {
    LOG_WARN("failed to convert rowkey to sql text", K(tz_info), K(ret));
  } else {
    rowkey_str.assign_ptr(buf, pos);
  }
  return ret;
}

int ObAutoSplitArgBuilder::build_ddl_stmt_str_(
    const share::schema::ObTableSchema &orig_table_schema,
    const share::schema::AlterTableSchema &alter_table_schema,
    const ObTabletID &src_tablet_id,
    const ObTimeZoneInfo *tz_info,
    ObIAllocator &allocator,
    ObString &ddl_stmt_str)
{
  int ret = OB_SUCCESS;
  const bool from_non_partitioned_table = orig_table_schema.get_part_level() == PARTITION_LEVEL_ZERO;
  const int64_t part_num = alter_table_schema.get_partition_num();
  share::schema::ObPartition **part_array = alter_table_schema.get_part_array();
  bool is_oracle_mode = false;
  ObSqlString sql_string;
  ObArenaAllocator tmp_allocator;
  ObString table_name; // by allocator
  share::schema::ObSchemaGetterGuard mock_schema_guard;
  share::schema::ObSchemaPrinter schema_printer(mock_schema_guard);

  if (OB_FAIL(sql_string.append_fmt("/*ob_auto_split*/ "))) {
    LOG_WARN("failed to append fmt", K(ret));
  } else if (OB_FAIL(orig_table_schema.check_if_oracle_compat_mode(is_oracle_mode))) {
    LOG_WARN("failed to check if oracle mode", K(ret), K(orig_table_schema.get_table_id()));
  } else if (OB_FAIL(print_identifier(allocator, is_oracle_mode, orig_table_schema.get_table_name_str(), table_name))) {
    LOG_WARN("failed to generate new name with escape character", K(ret), K(orig_table_schema.get_table_name()));
  } else if (from_non_partitioned_table) {
    ObArray<uint64_t> presetting_partition_keys;
    if (OB_FAIL(sql_string.append_fmt("ALTER TABLE %.*s", table_name.length(), table_name.ptr()))) {
      LOG_WARN("failed to append fmt", K(ret));
    } else if (OB_FAIL(orig_table_schema.get_presetting_partition_keys(presetting_partition_keys))) {
      LOG_WARN("failed to get presetting partition key columns", KR(ret), K(orig_table_schema));
    } else if (presetting_partition_keys.empty()) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("invalid schema for auto partitioning", KR(ret), K(orig_table_schema));
    } else {
      const bool is_multi_partkey = presetting_partition_keys.count() > 1;
      const ObString &orig_part_func_expr = orig_table_schema.get_part_option().get_part_func_expr_str();
      ObString part_func_expr(orig_part_func_expr.length(), orig_part_func_expr.ptr());
      ObPartitionFuncType part_func_type = orig_table_schema.get_part_option().get_part_func_type();
      if (part_func_expr.empty()) {
        int64_t buf_len = OB_MAX_TEXT_LENGTH;
        int64_t pos = 0;
        char *buf = static_cast<char *>(tmp_allocator.alloc(buf_len));
        if (OB_ISNULL(buf)) {
          ret = OB_ALLOCATE_MEMORY_FAILED;
          LOG_WARN("fail to alloc", KR(ret));
        } else if (OB_FAIL(schema_printer.print_column_list(orig_table_schema, presetting_partition_keys, buf, buf_len, pos))) {
          LOG_WARN("failed to print part func expr", K(ret), K(orig_table_schema.get_table_id()), K(presetting_partition_keys));
        } else {
          part_func_expr.assign_ptr(buf, pos);
        }
        for (int64_t i = 0; OB_SUCC(ret) && i < presetting_partition_keys.count(); i++) {
          const ObColumnSchemaV2 *column_schema = orig_table_schema.get_column_schema(presetting_partition_keys.at(i));
          if (OB_ISNULL(column_schema)) {
            ret = OB_ERR_UNEXPECTED;
            LOG_WARN("column_schema is null", KR(ret));
          } else if (ObResolverUtils::is_partition_range_column_type(column_schema->get_meta_type().get_type())) {
            part_func_type = PARTITION_FUNC_TYPE_RANGE_COLUMNS;
          }
        }
      }
      if (OB_FAIL(ret)) {
      } else if (sql_string.append_fmt(" %s(%.*s) (",
            is_oracle_mode ? "MODIFY PARTITION BY RANGE" : ((is_multi_partkey || part_func_type == PARTITION_FUNC_TYPE_RANGE_COLUMNS) ? "PARTITION BY RANGE COLUMNS" : "PARTITION BY RANGE"),
            part_func_expr.length(), part_func_expr.ptr())) {
        LOG_WARN("failed to append fmt", K(ret));
      }
    }
  } else {
    int64_t part_idx = OB_INVALID_INDEX_INT64;
    int64_t subpart_idx = OB_INVALID_INDEX_INT64;
    ObBasePartition *src_partition = nullptr;
    ObString part_name;
    if (OB_FAIL(orig_table_schema.get_part_idx_by_tablet(src_tablet_id, part_idx, subpart_idx))) {
      LOG_WARN("failed to get part idx", K(ret), K(src_tablet_id));
    } else if (OB_FAIL(orig_table_schema.get_part_by_idx(part_idx, subpart_idx, src_partition))) {
      LOG_WARN("failed to get part by idx", K(ret), K(part_idx), K(subpart_idx));
    } else if (OB_ISNULL(src_partition)) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("invalid partition", K(ret), K(part_idx), K(subpart_idx));
    } else if (OB_FAIL(print_identifier(tmp_allocator, is_oracle_mode, src_partition->get_part_name(), part_name))) {
      LOG_WARN("print partition name failed", K(ret), KPC(src_partition));
    } else if (OB_FAIL(sql_string.append_fmt(is_oracle_mode ? "ALTER TABLE %.*s SPLIT PARTITION %.*s INTO ("
                                                            : "ALTER TABLE %.*s REORGANIZE PARTITION %.*s INTO (",
        table_name.length(), table_name.ptr(),
        part_name.length(), part_name.ptr()))) {
      LOG_WARN("failed to append fmt", K(ret));
    }
  }

  if (OB_FAIL(ret)) {
  } else if (OB_ISNULL(part_array) || OB_UNLIKELY(part_num < 2)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("part_array is null or invalid split part num", K(ret), K(part_array), K(part_num));
  }
  for (int64_t i = 0; OB_SUCC(ret) && i < part_num; ++i) {
    tmp_allocator.reuse();
    const bool is_last_part = i+1 == part_num;
    ObString part_name;
    ObString high_bound_val_str;
    if (OB_ISNULL(part_array[i])) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("part is null", K(ret), K(part_array[i]));
    } else if (OB_FAIL(print_identifier(tmp_allocator, is_oracle_mode, part_array[i]->get_part_name(), part_name))) {
      LOG_WARN("print partition name failed", K(ret), KPC(part_array[i]));
    } else if (OB_FAIL(sql_string.append_fmt("PARTITION %.*s", part_name.length(), part_name.ptr()))) {
      LOG_WARN("failed to append fmt", K(ret), K(part_name));
    } else if (OB_FAIL(convert_rowkey_to_sql_literal(part_array[i]->get_high_bound_val(), is_oracle_mode, tz_info, tmp_allocator, high_bound_val_str))) {
      LOG_WARN("failed to convert high bound val", K(ret), KPC(part_array[i]), K(is_oracle_mode));
    } else if (!is_oracle_mode || (is_oracle_mode && (!is_last_part || from_non_partitioned_table))) {
      if (OB_FAIL(sql_string.append_fmt(" VALUES LESS THAN (%.*s)%s",
              high_bound_val_str.length(), high_bound_val_str.ptr(),
              is_last_part ? "" : ", "))) {
        LOG_WARN("failed to append fmt", K(ret));
      }
    }
  }
  if (OB_FAIL(ret)) {
  } else if (OB_FAIL(sql_string.append_fmt(")"))) {
    LOG_WARN("failed to append fmt", K(ret));
  } else if (OB_FAIL(ob_write_string(allocator, sql_string.string(), ddl_stmt_str, true/*c_style*/))) {
    LOG_WARN("failed to write string", K(ret));
  }
  return ret;
}

int ObAutoSplitArgBuilder::build_alter_table_schema_(const uint64_t tenant_id,
                                                     const ObString &db_name,
                                                     const share::schema::ObTableSchema &table_schema,
                                                     const ObTabletID split_source_tablet_id,
                                                     const ObArray<ObNewRange> &ranges,
                                                     const ObTimeZoneInfo *tz_info,
                                                     share::schema::AlterTableSchema &alter_table_schema)
{
  int ret = OB_SUCCESS;
  const ObString& table_name = table_schema.get_table_name_str();
  const uint64_t table_id = table_schema.get_table_id();
  const int64_t part_num = ranges.size();
  const ObString& part_func_expr = table_schema.get_part_option().get_part_func_expr_str();
  const ObPartitionFuncType part_func_type = table_schema.get_part_option().get_part_func_type();
  const ObPartitionLevel target_part_level = table_schema.get_target_part_level_for_auto_partitioned_table();

  if (OB_FAIL(alter_table_schema.set_origin_database_name(db_name))) {
    LOG_WARN("fail to set origin database name", KR(ret), K(db_name));
  } else if (OB_FAIL(alter_table_schema.set_origin_table_name(table_name))) {
    LOG_WARN("fail to set origin table name", KR(ret), K(table_name));
  } else if (OB_UNLIKELY(target_part_level == ObPartitionLevel::PARTITION_LEVEL_MAX)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("invalid target part level", KR(ret), K(table_schema));
  } else if (FALSE_IT(alter_table_schema.set_table_type(table_schema.get_table_type()))) {
  } else if (FALSE_IT(alter_table_schema.set_index_type(table_schema.get_index_type()))) {
  } else if (FALSE_IT(alter_table_schema.set_tenant_id(tenant_id))) {
  } else if (FALSE_IT(alter_table_schema.set_part_level(target_part_level))) {
  } else if (FALSE_IT(alter_table_schema.get_part_option().set_part_func_type(part_func_type))) {
  } else if (FALSE_IT(alter_table_schema.get_part_option().set_part_expr(part_func_expr))) {
  } else if (FALSE_IT(alter_table_schema.set_part_num(part_num))) {
  } else {
    share::schema::ObPartition new_part;

    for (int64_t i = 0; OB_SUCC(ret) && i < part_num; i++) {
      const ObRowkey& high_bound_val = ranges[i].get_end_key();

      if (OB_FAIL(build_partition_(tenant_id, table_id,
                                   split_source_tablet_id, high_bound_val, tz_info,
                                   new_part))) {
        LOG_WARN("fail to build partition", KR(ret), K(tenant_id), K(table_id), K(split_source_tablet_id),
                                            K(high_bound_val), K(table_schema));
      } else if (OB_FAIL(alter_table_schema.add_partition(new_part))) {
        LOG_WARN("fail to add partition", KR(ret), K(new_part));
      } else {
        new_part.reset();
      }
    }


    if (OB_FAIL(ret)) {
    } else if (OB_FAIL(rootserver::ObDDLService::fill_part_name(table_schema, alter_table_schema))) {
      LOG_WARN("failed to fill part name", K(ret));
    } else {
      const int64_t part_num = alter_table_schema.get_partition_num();
      share::schema::ObPartition **part_array = alter_table_schema.get_part_array();
      if (OB_ISNULL(part_array)) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("invalid part array", K(ret));
      }
      for (int64_t i = 0; OB_SUCC(ret) && i < part_num; i++) {
        if (OB_ISNULL(part_array[i])) {
          ret = OB_ERR_UNEXPECTED;
          LOG_WARN("part is null", K(ret), K(part_array[i]));
        } else if (OB_UNLIKELY(part_array[i]->get_part_name().empty())) {
          ret = OB_ERR_UNEXPECTED;
          LOG_WARN("part name is empty after fill", K(ret), KPC(part_array[i]));
        } else {
          part_array[i]->set_is_empty_partition_name(false); // so that rs won't generated part name again
        }
      }
    }
  }
  return ret;
}

int ObAutoSplitArgBuilder::build_partition_(const uint64_t tenant_id, const uint64_t table_id,
                                            const ObTabletID split_source_tablet_id,
                                            const ObRowkey &high_bound_val,
                                            const ObTimeZoneInfo *tz_info,
                                            share::schema::ObPartition &new_part)
{
  int ret = OB_SUCCESS;
  bool need_cast = false;
  ObRowkey cast_high_bound_val;
  common::ObArenaAllocator cast_allocator;
  if (OB_FAIL(check_and_cast_high_bound(high_bound_val, tz_info, cast_high_bound_val, need_cast, cast_allocator))) {
    LOG_WARN("failed to check cast high bound", K(ret));
  } else if (need_cast && OB_FAIL(new_part.set_high_bound_val(cast_high_bound_val))) {
    LOG_WARN("failed to set high_bound_val", KR(ret));
  } else if (!need_cast && OB_FAIL(new_part.set_high_bound_val(high_bound_val))) {
    LOG_WARN("failed to set high_bound_val", KR(ret));
  } else {
    new_part.set_is_empty_partition_name(true);
    new_part.set_tenant_id(tenant_id);
    new_part.set_table_id(table_id);
    new_part.set_split_source_tablet_id(split_source_tablet_id);
    new_part.set_partition_type(PartitionType::PARTITION_TYPE_NORMAL);
  }

  if (OB_NOT_NULL(cast_high_bound_val.get_obj_ptr())) {
    cast_high_bound_val.destroy(cast_allocator);
  }
  return ret;
}

int ObAutoSplitArgBuilder::check_and_cast_high_bound(const ObRowkey &origin_high_bound_val,
                                                     const ObTimeZoneInfo *tz_info,
                                                     ObRowkey &cast_hight_bound_val,
                                                     bool &need_cast,
                                                     ObIAllocator &allocator)
{
  int ret = OB_SUCCESS;
  const ObObj *ori_obj_ptr = origin_high_bound_val.get_obj_ptr();
  const int64_t obj_count = origin_high_bound_val.get_obj_cnt();
  need_cast = false;
  cast_hight_bound_val.reset();
  if (OB_ISNULL(ori_obj_ptr)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("object ptr should not be null", K(ret), K(origin_high_bound_val));
  } else {
    for (int64_t i = 0; OB_SUCC(ret) && !need_cast && i < obj_count; ++i) {
      if (OB_FAIL(check_need_to_cast(ori_obj_ptr[i], need_cast))) {
        LOG_WARN("fail to check need to cast", K(ret), K(ori_obj_ptr[i]));
      }
    }
    if (OB_SUCC(ret) && need_cast) {
      ObObj *cast_obj_ptr = nullptr;
      if (OB_FAIL(origin_high_bound_val.deep_copy(cast_hight_bound_val, allocator))) {
        LOG_WARN("failed to copy rowkey", K(origin_high_bound_val));
      } else if (OB_ISNULL(cast_obj_ptr = cast_hight_bound_val.get_obj_ptr())) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("object ptr should not be null", K(ret), K(cast_obj_ptr));
      } else {
        for (int64_t i = 0; OB_SUCC(ret) && i < obj_count; ++i) {
          const ObObj *o_obj_ptr = &ori_obj_ptr[i];
          ObObj *c_obj_ptr = &cast_obj_ptr[i];
          bool need_to_cast = false;
          if (OB_ISNULL(c_obj_ptr) || OB_ISNULL(o_obj_ptr)) {
            ret = OB_ERR_UNEXPECTED;
            LOG_WARN("ptr should not be null", K(ret), K(c_obj_ptr), K(o_obj_ptr));
          } else if (OB_FAIL(check_need_to_cast(*o_obj_ptr, need_to_cast))) {
            LOG_WARN("fail to check need to cast", K(ret), K(*o_obj_ptr));
          } else if (need_to_cast) {
            const ObObjType expected_obj_type = ori_obj_ptr[i].is_timestamp_ltz() ? ObTimestampTZType : (ob_is_int_tc(c_obj_ptr->get_type()) ? ObIntType : ObUInt64Type);
            int64_t cm_mode = CM_NONE;
            ObDataTypeCastParams dtc_params;
            dtc_params.tz_info_ = tz_info;
            ObCastCtx cast_ctx(&allocator, &dtc_params, cm_mode, c_obj_ptr->get_meta().get_collation_type());
            if (OB_FAIL(ObObjCaster::to_type(expected_obj_type, cast_ctx, *o_obj_ptr, *c_obj_ptr))) {
              STORAGE_LOG(WARN, "fail to cast obj",
                  K(ret), K(*o_obj_ptr), K(*c_obj_ptr), K(o_obj_ptr->get_type()),
                  K(ob_obj_type_str(o_obj_ptr->get_type())),
                  K(o_obj_ptr->get_meta().get_type()), K(ob_obj_type_str(o_obj_ptr->get_meta().get_type())));
            }
          }
        }
      }
    }
  }
  return ret;
}

int ObAutoSplitArgBuilder::check_need_to_cast(const ObObj &obj, bool &need_to_cast)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!obj.is_valid_type())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid type", K(ret), K(obj));
  } else {
    need_to_cast = (ob_is_integer_type(obj.get_type()) && (ObIntType != obj.get_type() || ObUInt64Type != obj.get_type()))
              || obj.is_timestamp_ltz();
  }
  return ret;
}

// sample rowkey ranges of data_table/global_index from given tablet
int ObSplitSampler::query_ranges(const uint64_t tenant_id,
                                 const ObString &db_name,
                                 const share::schema::ObTableSchema &table_schema,
                                 const ObTabletID tablet_id,
                                 const int64_t range_num, const int64_t used_disk_space,
                                 common::ObArenaAllocator &range_allocator,
                                 ObArray<ObNewRange> &ranges)
{
  int ret = OB_SUCCESS;
  const uint64_t table_id = table_schema.get_table_id();
  PartitionMeta part_meta;
  ObArray<ObString> column_names;
  ObArray<ObNewRange> unused_column_ranges;
  common::ObRowkey low_bound_val;
  common::ObRowkey high_bound_val;
  bool is_oracle_mode = false;

  if (OB_UNLIKELY(OB_INVALID_ID == tenant_id || OB_INVALID_ID == table_id ||
                  !tablet_id.is_valid())) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("invalid id", KR(ret), K(tenant_id), K(table_id), K(tablet_id));
  } else if (OB_UNLIKELY(db_name.empty())){
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("invalid db name", KR(ret));
  } else if (OB_UNLIKELY(range_num < 2)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("it's no need to split", KR(ret), K(range_num));
  } else if (OB_UNLIKELY(used_disk_space == 0)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("used_disk_space can't be 0", KR(ret));
  } else if (OB_UNLIKELY(!table_schema.is_user_table() && !table_schema.is_global_index_table())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("only support to sample data_table or global index", KR(ret), K(table_schema));
  } else if (OB_FAIL(acquire_partition_key_name_(table_schema, column_names))){
    LOG_WARN("fail to acquire partition key name", KR(ret), K(table_schema));
  } else if (OB_UNLIKELY(column_names.empty())){
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("invalid array count", KR(ret), K(column_names));
  } else if (table_schema.is_partitioned_table()) {
    if (OB_FAIL(acquire_partition_meta_(table_schema, tablet_id, part_meta))) {
      LOG_WARN("fail to acquire partition meta", KR(ret), K(tenant_id), K(table_id), K(tablet_id));
    } else if (nullptr == part_meta.part_) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("meta's partition is NULL", KR(ret), K(tenant_id), K(table_id), K(tablet_id), K(part_meta));
    }
  }
  if (OB_FAIL(ret)) {
  } else if (OB_FAIL(table_schema.check_if_oracle_compat_mode(is_oracle_mode))) {
    LOG_WARN("failed to check if oracle compat mode", K(ret), K(tenant_id), K(table_schema.get_table_id()));
  } else if (OB_FAIL(fill_query_range_bounder(part_meta, unused_column_ranges, column_names.count(), low_bound_val, high_bound_val, range_allocator))) {
    LOG_WARN("fail to fill query range bounder", K(ret));
  } else if (OB_FAIL(query_ranges_(tenant_id, db_name, table_schema.get_table_name_str(), part_meta,
                            column_names, unused_column_ranges,
                            range_num, used_disk_space,
                            table_schema.is_global_index_table(),
                            is_oracle_mode,
                            low_bound_val, high_bound_val,
                            range_allocator, ranges))) {
    LOG_WARN("fail to acquire ranges for split partition", KR(ret), K(tenant_id), K(db_name),
                                                           K(table_schema), K(part_meta),
                                                           K(range_num), K(used_disk_space),
                                                           K(ranges));
  }
  LOG_DEBUG("query range result", K(ret), K(ranges));
  return ret;
}

// this function is only called by pre-splitting partition.
// it will sample ranges of given column from data_table.
// column_names records a set of columns which we want to sample(the empty column_names sample all).
// column_ranges should be empty or be same size with column_names.
// each range of column_ranges records the scope of column in column_names,
// we will filter the sampling result which are not in the scope.
int ObSplitSampler::query_ranges(const uint64_t tenant_id,
                                 const ObString &db_name,
                                 const share::schema::ObTableSchema &data_table_schema,
                                 const ObIArray<ObString> &column_names,
                                 const ObIArray<ObNewRange> &column_ranges,
                                 const int64_t range_num, const int64_t used_disk_space,
                                 common::ObArenaAllocator &range_allocator,
                                 ObArray<ObNewRange> &ranges)
{
  int ret = OB_SUCCESS;
  PartitionMeta unused_part_meta;
  const int64_t unused_presetting_column_cnt = 0;
  common::ObRowkey low_bound_val;
  common::ObRowkey high_bound_val;
  bool is_oracle_mode = false;

  if (OB_UNLIKELY(OB_INVALID_ID == tenant_id)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("invalid tenant id", KR(ret), K(tenant_id));
  } else if (OB_UNLIKELY(db_name.empty())){
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("invalid db name", KR(ret));
  } else if (OB_UNLIKELY(range_num < 2)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("it's no need to split", KR(ret), K(range_num));
  } else if (OB_UNLIKELY(used_disk_space == 0)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("used_disk_space can't be 0", KR(ret));
  } else if (OB_UNLIKELY(column_names.empty())){
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("invalid array count", KR(ret), K(column_names));
  } else if (OB_UNLIKELY(!column_ranges.empty() && column_names.count() != column_ranges.count())) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("invalid array count", KR(ret), K(column_names), K(column_ranges));
  } else if (OB_FAIL(data_table_schema.check_if_oracle_compat_mode(is_oracle_mode))) {
    LOG_WARN("failed to check if oracle compat mode", K(ret), K(tenant_id), K(data_table_schema.get_table_id()));
  } else if (OB_FAIL(fill_query_range_bounder(unused_part_meta, column_ranges, unused_presetting_column_cnt, low_bound_val, high_bound_val, range_allocator))) {
    LOG_WARN("fail to fill query range bounder", K(ret), K(column_ranges));
  } else if (OB_FAIL(query_ranges_(tenant_id, db_name, data_table_schema.get_table_name_str(),
                                   unused_part_meta,
                                   column_names, column_ranges,
                                   range_num, used_disk_space,
                                   false /*query_index*/,
                                   is_oracle_mode,
                                   low_bound_val, high_bound_val,
                                   range_allocator, ranges))) {
    LOG_WARN("fail to acquire ranges for split partition", KR(ret), K(tenant_id), K(db_name),
                                                           K(data_table_schema), K(range_num),
                                                           K(used_disk_space),
                                                           K(ranges));
  }
  LOG_DEBUG("query range result", K(ret), K(column_ranges), K(ranges));
  return ret;
}

/*
  如果是重建自动分区全局索引非分区表，part_column_cnt为潜在的分区键
*/
int ObSplitSampler::fill_query_range_bounder(
    const PartitionMeta& part_meta,
    const ObIArray<ObNewRange> &column_ranges,
    const int64_t presetting_part_column_cnt,
    common::ObRowkey &l_bound_val,
    common::ObRowkey &h_bound_val,
    common::ObArenaAllocator &allocator)
{
  int ret = OB_SUCCESS;
  l_bound_val.set_min_row();
  h_bound_val.set_max_row();
  if (nullptr != part_meta.part_) {
    l_bound_val = part_meta.part_->get_low_bound_val();
    h_bound_val = part_meta.part_->get_high_bound_val();
  } else if (column_ranges.count() > 0 || presetting_part_column_cnt > 0) {
    // row key complement
    ObObj l_obj_buf[OB_MAX_ROWKEY_COLUMN_NUMBER];
    ObObj h_obj_buf[OB_MAX_ROWKEY_COLUMN_NUMBER];
    const int64_t column_range_cnt = column_ranges.count();
    const int64_t column_obj_cnt = 1;
    int64_t copy_key_length = 0;
    if (column_range_cnt > 0) {
      for (int64_t i = 0; OB_SUCC(ret) && i < column_range_cnt; ++i) {
        const ObNewRange &tmp_range = column_ranges.at(i);
        const ObRowkey &l_key = tmp_range.start_key_;
        const ObRowkey &h_key = tmp_range.end_key_;
        if (l_key.get_obj_cnt() != column_obj_cnt || h_key.get_obj_cnt() != column_obj_cnt) {
          ret = OB_ERR_UNEXPECTED;
          LOG_WARN("fail to fill query range bounder", K(ret), K(l_key), K(h_key));
        } else if (OB_ISNULL(l_key.get_obj_ptr()) || OB_ISNULL(h_key.get_obj_ptr())) { // shallow copy
          ret = OB_NULL_CHECK_ERROR;
          LOG_WARN("check null ptr failed", K(ret), KP(l_key.get_obj_ptr()), KP(h_key.get_obj_ptr()));
        } else {
          l_obj_buf[i] = l_key.get_obj_ptr()[0];
          h_obj_buf[i] = h_key.get_obj_ptr()[0];
        }
      }
      copy_key_length = column_range_cnt;
    } else {
      for (int64_t i = 0; i < presetting_part_column_cnt; ++i) {
        ObObj tmp_l_buf;
        ObObj tmp_h_buf;
        tmp_l_buf.set_min_value();
        tmp_h_buf.set_max_value();
        l_obj_buf[i] = tmp_l_buf;
        h_obj_buf[i] = tmp_h_buf;
      }
      copy_key_length = presetting_part_column_cnt;
    }
    if (OB_SUCC(ret)) {
      ObRowkey l_rowkey(l_obj_buf, copy_key_length);
      ObRowkey h_rowkey(h_obj_buf, copy_key_length);
      if (OB_FAIL(l_rowkey.deep_copy(l_bound_val, allocator))) {
        LOG_WARN("fail to set low bound val", K(ret), K(l_rowkey));
      } else if (OB_FAIL(h_rowkey.deep_copy(h_bound_val, allocator))) {
        LOG_WARN("fail to set high bound val", K(ret), K(h_rowkey));
      }
    }
  } else {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("fail to fill query range bounder",
      K(ret), K(part_meta), K(column_ranges), K(presetting_part_column_cnt));
  }
  return ret;
}

int ObSplitSampler::acquire_partition_meta_(const share::schema::ObTableSchema &table_schema,
                                            const ObTabletID &tablet_id, PartitionMeta &meta)
{
  int ret = OB_SUCCESS;
  ObPartitionSchemaIter iter(table_schema, ObCheckPartitionMode::CHECK_PARTITION_MODE_NORMAL);
  PartitionMeta tmp_meta;
  bool find = false;
  while (!find && OB_SUCC(iter.next_partition_info(tmp_meta))) {
    if (tmp_meta.tablet_id_ == tablet_id) {
      find = true;
      meta = tmp_meta;
    }
  }

  if (OB_FAIL(ret)) {
    if (ret == OB_ITER_END) {
      ret = OB_UNKNOWN_PARTITION;
      LOG_WARN("fail to find partition info in table schema", KR(ret), K(tablet_id));
    } else {
      LOG_WARN("fail to get next partition info", KR(ret), K(tablet_id));
    }
  }
  return ret;
}

int ObSplitSampler::query_ranges_(const uint64_t tenant_id, const ObString &db_name,
                                  const ObString &table_name,
                                  const PartitionMeta &part_meta,
                                  const ObIArray<ObString> &column_names,
                                  const ObIArray<ObNewRange> &column_ranges,
                                  const int64_t range_num, const int64_t used_disk_space,
                                  const bool query_index,
                                  const bool is_oracle_mode,
                                  common::ObRowkey &low_bound_val,
                                  common::ObRowkey &high_bound_val,
                                  common::ObArenaAllocator& range_allocator,
                                  ObArray<common::ObNewRange> &ranges)
{
  int ret = OB_SUCCESS;
  const ObString* part_name = nullptr;
  if (nullptr != part_meta.part_) {
    part_name = &part_meta.part_->get_part_name();
  }
  ObOracleSqlProxy oracle_sql_proxy(*GCTX.sql_proxy_);
  ObSqlString sql;
  ObSingleConnectionProxy single_conn_proxy;
  static const int64_t MAX_SAMPLE_SCALE = 128L * 1024 * 1024; // at most sample 128MB
  double sample_pct = MAX_SAMPLE_SCALE >= used_disk_space ?
                      100 :
                      static_cast<double>(MAX_SAMPLE_SCALE) / used_disk_space * 100;
  ranges.reset();

  if (is_oracle_mode) {
    if (OB_FAIL(single_conn_proxy.connect(tenant_id, 0/*group_id*/, &oracle_sql_proxy))) {
      LOG_WARN("failed to get mysql connect", KR(ret), K(tenant_id));
    }
  } else if (OB_FAIL(single_conn_proxy.connect(tenant_id, 0/*group_id*/, GCTX.sql_proxy_))) {
    LOG_WARN("failed to get mysql connect", KR(ret), K(tenant_id));
  }
  if (OB_FAIL(ret)) {
  } else if (query_index) {
    ObSqlString set_sql;
    int64_t affected_rows = 0;
    if (OB_FAIL(set_sql.assign_fmt("SET session %s = true", share::OB_SV_ENABLE_INDEX_DIRECT_SELECT))) {
      LOG_WARN("failed to assign sql", KR(ret));
    } else if (OB_FAIL(single_conn_proxy.write(tenant_id, set_sql.ptr(), affected_rows))) {
      LOG_WARN("single_conn_proxy write failed", KR(ret), K(set_sql));
    }
  }

  // the sql will sample spliting table and acquire "range_num" rowkeys as split points
  if (OB_FAIL(ret)) {
  } else if (OB_FAIL(build_sample_sql_(db_name, table_name, part_name,
                                       column_names, column_ranges,
                                       range_num, sample_pct, is_oracle_mode, sql))) {
    LOG_WARN("fail to build sample sql", KR(ret), K(db_name), K(table_name), K(part_name),
                                         K(column_names), K(column_ranges),
                                         K(range_num), K(sample_pct));
  } else {
    SMART_VAR(ObMySQLProxy::MySQLResult, res) {
      sqlclient::ObMySQLResult *sql_result = nullptr;
      if (OB_FAIL(single_conn_proxy.read(res, tenant_id, sql.ptr()))) {
        LOG_WARN("execute sql failed", KR(ret), K(sql));
      } else if (OB_ISNULL(sql_result = res.get_result())) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("result is null", KR(ret), K(sql));
      } else {
        const ObNewRow *row = nullptr;
        bool is_first_row = true;
        int column_count = 0;
        ObObj objs[OB_MAX_ROWKEY_COLUMN_NUMBER];
        ObNewRange range;

        while (OB_SUCC(ret)) {
          if (OB_FAIL(sql_result->next())) {
            if (OB_ITER_END != ret) {
              LOG_WARN("get next result failed", KR(ret), K(sql));
            }
          } else if (OB_ISNULL(row = sql_result->get_row())) {
            ret = OB_ERR_UNEXPECTED;
            LOG_WARN("NULL row", KR(ret), K(sql));
          } else if (is_first_row) {
            // to acquire "range_num" split partitions,
            // only need "range_num - 1" split points,
            // the first point from sampling sql is unused.
            is_first_row = false;
            column_count = row->get_count();
          } else if (OB_UNLIKELY(column_count <= 0 || column_count > OB_MAX_ROWKEY_COLUMN_NUMBER)) {
            ret = OB_ERR_UNEXPECTED;
            LOG_WARN("invalid column count", KR(ret), K(column_count), K(sql));
          } else {
            // build start_row_key
            if (ranges.size() == 0) {
              // copy low_bound_val to range.start_key_
              if (OB_FAIL(low_bound_val.deep_copy(range.start_key_, range_allocator))) {
                LOG_WARN("fail to copy low_bound_val to range.start_key_", KR(ret));
              }
            } else {
              ObRowkey& old_end = ranges[ranges.size()-1].end_key_;
              range.start_key_.assign(old_end.get_obj_ptr(), column_count);
            }

            // build end_row_key
            if (OB_FAIL(ret)) {
            } else {
              for (int64_t i = 0; i < column_count; i++) {
                objs[i] = row->get_cell(i);
              }

              ObRowkey end(objs, column_count);
              // copy end to range.end_key_
              if (OB_FAIL(end.deep_copy(range.end_key_, range_allocator))) {
                LOG_WARN("fail to copy ObRowkey", KR(ret));
              } else {
                range.border_flag_.set_inclusive_end();
              }
            }

            if (OB_FAIL(ret)) {
            } else if (OB_FAIL(ranges.push_back(range))) {
              LOG_WARN("range push back failed", KR(ret));
            }
          }
        } // end while

        if (OB_LIKELY(OB_ITER_END == ret)) {
          ret = OB_SUCCESS;
          if (!ranges.empty()) {
            ObRowkey& old_end = ranges[ranges.size()-1].end_key_;
            range.start_key_.assign(old_end.get_obj_ptr(), column_count);

            if (OB_FAIL(high_bound_val.deep_copy(range.end_key_, range_allocator))) {
              LOG_WARN("fail to copy high_bound_val to range.end_key_", KR(ret));
            } else if (OB_FAIL(ranges.push_back(range))) {
              LOG_WARN("range push back failed", KR(ret));
            }
          }
        }
      }
    } // end SMART_VAR(ObMySQLProxy::MySQLResult, res)
  }

  return ret;
}

int ObSplitSampler::build_sample_sql_(const ObString &db_name, const ObString &table_name, const ObString *part_name,
                                      const ObIArray<ObString> &column_names,
                                      const ObIArray<ObNewRange> &column_ranges,
                                      const int range_num, const double sample_pct,
                                      const bool is_oracle_mode,
                                      ObSqlString &sql)
{
  int ret = OB_SUCCESS;
  ObArenaAllocator tmp_allocator;
  ObString table_name_quoted;
  ObString db_name_quoted;
  ObSqlString col_alias_str;
  ObSqlString col_name_alias_str;

  if (OB_FAIL(gen_column_alias_(column_names, is_oracle_mode, col_alias_str, col_name_alias_str))) {
    LOG_WARN("fail to gen column alias", KR(ret), K(column_names));
  } else if (OB_FAIL(ObAutoSplitArgBuilder::print_identifier(tmp_allocator, is_oracle_mode, db_name, db_name_quoted))) {
    LOG_WARN("failed to generate new name with escape character", K(ret), K(db_name));
  } else if (OB_FAIL(ObAutoSplitArgBuilder::print_identifier(tmp_allocator, is_oracle_mode, table_name, table_name_quoted))) {
    LOG_WARN("failed to generate new name with escape character", K(ret), K(table_name));
  } else if (OB_FAIL(sql.assign_fmt(
      "SELECT %.*s FROM "
      "(SELECT %.*s, bucket, ROW_NUMBER() OVER (PARTITION BY bucket ORDER BY %.*s) rn FROM "
      "(SELECT %.*s, NTILE(%d) OVER (ORDER BY %.*s) bucket FROM "
      "(SELECT /*+ index(%.*s primary) */ %.*s FROM %.*s.%.*s ",
      static_cast<int>(col_alias_str.length()), col_alias_str.ptr(),

      static_cast<int>(col_alias_str.length()), col_alias_str.ptr(),
      static_cast<int>(col_alias_str.length()), col_alias_str.ptr(),

      static_cast<int>(col_alias_str.length()), col_alias_str.ptr(),
      range_num,
      static_cast<int>(col_alias_str.length()), col_alias_str.ptr(),

      table_name.length(), table_name.ptr(),
      static_cast<int>(col_name_alias_str.length()), col_name_alias_str.ptr(),
      db_name_quoted.length(), db_name_quoted.ptr(),
      table_name_quoted.length(), table_name_quoted.ptr()))) {
    LOG_WARN("string assign failed", KR(ret), K(col_alias_str), K(col_name_alias_str), K(db_name_quoted), K(table_name_quoted));
  }

  if (OB_FAIL(ret)){
  } else if (part_name != nullptr) {
    ObString part_name_quoted;
    if (OB_FAIL(ObAutoSplitArgBuilder::print_identifier(tmp_allocator, is_oracle_mode, *part_name, part_name_quoted))) {
      LOG_WARN("failed to print identifier", K(ret), KPC(part_name), K(is_oracle_mode));
    } else if (OB_FAIL(sql.append_fmt("PARTITION (%.*s) ", part_name_quoted.length(), part_name_quoted.ptr()))) {
      LOG_WARN("string assign failed", KR(ret), K(part_name_quoted));
    }
  }

  if (OB_FAIL(ret)){
  } else if (sample_pct < 100 && OB_FAIL(sql.append_fmt("SAMPLE BLOCK(%g) ", sample_pct))) {
    LOG_WARN("string assign failed", KR(ret), K(sample_pct));
  } else if (OB_FAIL(add_sample_condition_sqls_(column_names, column_ranges, sql))) {
    LOG_WARN("fail to add sample conditions", KR(ret), K(column_names), K(column_ranges));
  } else if (OB_FAIL(sql.append_fmt(") a) b) c WHERE rn = 1 GROUP BY %.*s ORDER BY %.*s",
                      static_cast<int>(col_alias_str.length()), col_alias_str.ptr(),
                      static_cast<int>(col_alias_str.length()), col_alias_str.ptr()))) {
    LOG_WARN("string assign failed", KR(ret), K(col_alias_str));
  }
  return ret;
}

int ObSplitSampler::add_sample_condition_sqls_(const ObIArray<ObString> &columns,
                                               const ObIArray<ObNewRange> &column_ranges,
                                               ObSqlString &sql)
{
  int ret = OB_SUCCESS;
  if (column_ranges.empty()) {
  } else if (OB_UNLIKELY(columns.count() != column_ranges.count())) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("invalid count", KR(ret), K(columns), K(column_ranges));
  } else if (OB_FAIL(sql.append_fmt("WHERE 1=1 "))) {
    LOG_WARN("string assign failed", KR(ret));
  } else {
    for (int64_t i = 0; OB_SUCC(ret) && i < columns.count(); i++) {
      const ObString& column = columns.at(i);
      const ObNewRange& range = column_ranges.at(i);
      if (range.start_key_.is_min_row() && range.end_key_.is_max_row()) {
      } else if (OB_FAIL(sql.append_fmt("AND "))) {
        LOG_WARN("string assign failed", KR(ret));
      } else if (OB_FAIL(add_sample_condition_sql_(column, range, sql))) {
        LOG_WARN("fail to add sample condition", KR(ret), K(column), K(range));
      }
    }
  }
  return ret;
}

int ObSplitSampler::add_sample_condition_sql_(const ObString &column,
                                              const ObNewRange &range,
                                              ObSqlString &sql)
{
  int ret = OB_SUCCESS;
  const int64_t buf_len = common::OB_MAX_ROW_KEY_LENGTH;
  char *range_buf = nullptr;
  int64_t pos = 0;
  ObArenaAllocator allocator;

  if (range.start_key_.get_obj_cnt() != 1 || range.end_key_.get_obj_cnt() != 1 ||
      range.start_key_.is_max_row() || range.end_key_.is_min_row()) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid range", KR(ret), K(column), K(range.start_key_), K(range.end_key_));
  } else if (OB_ISNULL(range_buf = (char *)allocator.alloc(buf_len))) {
    ret = OB_ALLOCATE_MEMORY_FAILED;
    LOG_WARN("fail to alloc buf", KR(ret));
  } else {
    if (!range.start_key_.is_min_row()) {
      ObString min_val;
      char *min_val_buf = range_buf;
      pos = range.start_key_.to_plain_string(min_val_buf, buf_len);
      min_val.assign(min_val_buf, pos);

      if (OB_UNLIKELY(min_val.empty())) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("fail to get min val", KR(ret), K(column), K(range));
      } else if (OB_FAIL(sql.append_fmt("%.*s %s %.*s ",
                                        column.length(), column.ptr(),
                                        range.border_flag_.inclusive_start() ? ">=" : ">",
                                        min_val.length(), min_val.ptr()
                                        ))) {
        LOG_WARN("string assign failed", KR(ret), K(column), K(range));
      }
    }

    if (OB_FAIL(ret)) {
    } else if (!range.end_key_.is_max_row()) {
      if (pos > 0 && OB_FAIL(sql.append_fmt("AND "))) {
        LOG_WARN("string assign failed", KR(ret), K(column), K(range));
      } else {
        ObString max_val;
        char *max_val_buf = range_buf + pos;
        pos = range.end_key_.to_plain_string(max_val_buf, buf_len - pos);
        max_val.assign(max_val_buf, pos);

        if (OB_UNLIKELY(max_val.empty())) {
          ret = OB_ERR_UNEXPECTED;
          LOG_WARN("fail to get min val", KR(ret), K(column), K(range));
        } else if (OB_FAIL(sql.append_fmt("%.*s %s %.*s ",
                                          column.length(), column.ptr(),
                                          range.border_flag_.inclusive_end() ? "<=" : "<",
                                          max_val.length(), max_val.ptr()
                                          ))) {
          LOG_WARN("string assign failed", KR(ret), K(column), K(range));
        }
      }
    }
  }
  return ret;
}

int ObSplitSampler::acquire_partition_key_name_(const share::schema::ObTableSchema &table_schema,
                                                ObIArray<ObString> &column_names)
{
  int ret = OB_SUCCESS;
  ObArray<uint64_t> column_ids;
  if (table_schema.is_partitioned_table() &&
      OB_FAIL(table_schema.get_partition_key_info().get_column_ids(column_ids))) {
    LOG_WARN("get column ids failed", KR(ret), K(table_schema));
  } else if (!table_schema.is_partitioned_table() &&
             OB_FAIL(table_schema.get_presetting_partition_keys(column_ids))) {
    LOG_WARN("get column ids failed", KR(ret), K(table_schema));
  } else {
    for (int64_t i = 0; OB_SUCC(ret) && i < column_ids.count(); i++) {
      const ObColumnSchemaV2 *col = table_schema.get_column_schema(column_ids.at(i));

      if (OB_ISNULL(col)) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("get column schema failed", KR(ret), "col", column_names.at(i));
      } else if (OB_FAIL(column_names.push_back(ObString(col->get_column_name_str().length(),
                                                         col->get_column_name_str().ptr())))) {
        LOG_WARN("append string failed", KR(ret), KPC(col));
      }
    }
  }
  return ret;
}

int ObSplitSampler::gen_column_alias_(const ObIArray<ObString> &columns,
                                      const bool is_oracle_mode,
                                      ObSqlString &col_alias_str,
                                      ObSqlString &col_name_alias_str)
{
  int ret = OB_SUCCESS;
  col_alias_str.reset();
  col_name_alias_str.reset();

  ObArenaAllocator tmp_allocator;
  for (int64_t i = 0; OB_SUCC(ret) && i < columns.count(); i++) {
    tmp_allocator.reuse();
    if (i > 0) {
      if (OB_FAIL(col_alias_str.append(", "))) {
        LOG_WARN("string append failed", KR(ret));
      } else if (OB_FAIL(col_name_alias_str.append(", "))) {
        LOG_WARN("string append failed", KR(ret));
      }
    }
    if (OB_SUCC(ret)) {
      const ObString &column_name = columns.at(i);
      ObSqlString alias;
      ObString column_name_quoted;
      if (OB_FAIL(alias.append_fmt("col%ld", i))) {
        LOG_WARN("append string failed", KR(ret));
      } else if (OB_FAIL(col_alias_str.append(alias.string()))) {
        LOG_WARN("append string failed", KR(ret));
      } else if (OB_FAIL(ObAutoSplitArgBuilder::print_identifier(tmp_allocator, is_oracle_mode, column_name, column_name_quoted))) {
        LOG_WARN("failed to generate new name with escape character", K(ret), K(column_name));
      } else if (OB_FAIL(col_name_alias_str.append_fmt(
                                              "%.*s AS %.*s",
                                              column_name_quoted.length(), column_name_quoted.ptr(),
                                              alias.string().length(),
                                              alias.string().ptr()))) {
        LOG_WARN("append string failed", KR(ret));
      }
    }
  }

  return ret;
}

}
}
