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

#define USING_LOG_PREFIX RS_LB

#include "ob_disaster_recovery_worker.h"
#include "observer/ob_inner_sql_connection.h"
#include "storage/tablelock/ob_lock_inner_connection_util.h"
#include "rootserver/ob_root_service.h"
#include "rootserver/tenant_snapshot/ob_tenant_snapshot_util.h"

namespace oceanbase
{

using namespace common;
using namespace share;
using namespace share::schema;
using namespace obrpc;
namespace rootserver
{

ObLSReplicaTaskDisplayInfo::ObLSReplicaTaskDisplayInfo()
  : tenant_id_(OB_INVALID_TENANT_ID),
    ls_id_(),
    task_type_(ObDRTaskType::MAX_TYPE),
    task_priority_(ObDRTaskPriority::MAX_PRI),
    target_server_(),
    target_replica_type_(REPLICA_TYPE_FULL),
    target_replica_paxos_replica_number_(OB_INVALID_COUNT),
    source_server_(),
    source_replica_type_(REPLICA_TYPE_FULL),
    source_replica_paxos_replica_number_(OB_INVALID_COUNT),
    execute_server_(),
    comment_("LSReplicaTask"),
    config_version_()
{
}

ObLSReplicaTaskDisplayInfo::~ObLSReplicaTaskDisplayInfo()
{
}

void ObLSReplicaTaskDisplayInfo::reset()
{
  tenant_id_ = OB_INVALID_TENANT_ID;
  ls_id_.reset();
  task_type_ = ObDRTaskType::MAX_TYPE;
  task_priority_ = ObDRTaskPriority::MAX_PRI;
  target_server_.reset();
  target_replica_type_ = REPLICA_TYPE_FULL;
  target_replica_paxos_replica_number_ = OB_INVALID_COUNT;
  source_server_.reset();
  source_replica_type_ = REPLICA_TYPE_FULL;
  source_replica_paxos_replica_number_ = OB_INVALID_COUNT;
  execute_server_.reset();
  comment_.reset();
  config_version_.reset();
}

int ObLSReplicaTaskDisplayInfo::init(
    const uint64_t &tenant_id,
    const share::ObLSID &ls_id,
    const obrpc::ObDRTaskType &task_type,
    const ObDRTaskPriority &task_priority,
    const common::ObAddr &target_server,
    const common::ObReplicaType &target_replica_type,
    const int64_t &target_replica_paxos_replica_number,
    const common::ObAddr &source_server,
    const common::ObReplicaType &source_replica_type,
    const int64_t &source_replica_paxos_replica_number,
    const common::ObAddr &execute_server,
    const ObString &comment,
    const palf::LogConfigVersion &config_version)
{
  int ret = OB_SUCCESS;
  if (!ls_id.is_valid_with_tenant(tenant_id)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument to init a ObLSReplicaTaskDisplayInfo",
             KR(ret), K(tenant_id), K(ls_id));
  } else if (OB_FAIL(comment_.assign(comment))) {
    LOG_WARN("fail to assign comment", KR(ret), K(comment));
  } else {
    tenant_id_ = tenant_id;
    ls_id_ = ls_id;
    task_type_ = task_type;
    task_priority_ = task_priority;
    target_server_ = target_server;
    target_replica_type_ = target_replica_type;
    target_replica_paxos_replica_number_ = target_replica_paxos_replica_number;
    source_server_ = source_server;
    source_replica_type_ = source_replica_type;
    source_replica_paxos_replica_number_ = source_replica_paxos_replica_number;
    execute_server_ = execute_server;
    config_version_ = config_version;
  }
  return ret;
}

bool ObLSReplicaTaskDisplayInfo::is_valid() const
{
  // do not check source_server and source_replica_type because it maybe useless
  return ls_id_.is_valid_with_tenant(tenant_id_)
         && task_type_ != ObDRTaskType::MAX_TYPE
         && task_priority_ != ObDRTaskPriority::MAX_PRI
         && target_server_.is_valid()
         && target_replica_type_ != REPLICA_TYPE_INVALID
         && target_replica_paxos_replica_number_ != OB_INVALID_COUNT
         && source_replica_paxos_replica_number_ != OB_INVALID_COUNT
         && execute_server_.is_valid();
}

int ObLSReplicaTaskDisplayInfo::assign(const ObLSReplicaTaskDisplayInfo &other)
{
  int ret = OB_SUCCESS;
  if (this != &other) {
    reset();
    if (OB_FAIL(comment_.assign(other.comment_))) {
      LOG_WARN("fail to assign comment", KR(ret), K(other));
    } else {
      tenant_id_ = other.tenant_id_;
      ls_id_ = other.ls_id_;
      task_type_ = other.task_type_;
      task_priority_ = other.task_priority_;
      target_server_ = other.target_server_;
      target_replica_type_ = other.target_replica_type_;
      target_replica_paxos_replica_number_ = other.target_replica_paxos_replica_number_;
      source_server_ = other.source_server_;
      source_replica_type_ = other.source_replica_type_;
      source_replica_paxos_replica_number_ = other.source_replica_paxos_replica_number_;
      execute_server_ = other.execute_server_;
      config_version_ = other.config_version_;
    }
  }
  return ret;
}

int64_t ObLSReplicaTaskDisplayInfo::to_string(char *buf, const int64_t buf_len) const
{
  int64_t pos = 0;
  J_OBJ_START();
  J_KV(
      K_(tenant_id),
      K_(ls_id),
      K_(task_type),
      K_(task_priority),
      K_(target_server),
      K_(target_replica_type),
      K_(target_replica_paxos_replica_number),
      K_(source_server),
      K_(source_replica_type),
      K_(source_replica_paxos_replica_number),
      K_(execute_server),
      K_(comment),
      K_(config_version));
  J_OBJ_END();
  return pos;
}

int ObDRWorker::LocalityAlignment::locate_zone_locality(
    const common::ObZone &zone,
    ReplicaDescArray *&replica_desc_array)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(zone.is_empty())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret));
  } else {
    ReplicaDescArray *my_desc_array = nullptr;
    int tmp_ret = locality_map_.get_refactored(zone, my_desc_array);
    if (OB_SUCCESS == tmp_ret) {
      if (OB_ISNULL(my_desc_array)) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("my_desc_array ptr is null", KR(ret));
      } else {
        replica_desc_array = my_desc_array;
      }
    } else if (OB_HASH_NOT_EXIST == tmp_ret) {
      void *raw_ptr = nullptr;
      my_desc_array = nullptr;
      if (OB_UNLIKELY(nullptr == (raw_ptr = allocator_.alloc(sizeof(ReplicaDescArray))))) {
        ret = OB_ALLOCATE_MEMORY_FAILED;
        LOG_WARN("fail to allocate memory", KR(ret));
      } else if (OB_UNLIKELY(nullptr == (my_desc_array = new (raw_ptr) ReplicaDescArray()))) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("fail to construct replica desc array", KR(ret));
      } else if (OB_FAIL(locality_map_.set_refactored(zone, my_desc_array))) {
        LOG_WARN("fail to set refactored", KR(ret));
      } else {
        replica_desc_array = my_desc_array;
      }
      if (OB_FAIL(ret) && OB_NOT_NULL(my_desc_array)) {
        my_desc_array->~ReplicaDescArray();
      }
    } else {
      ret = tmp_ret;
      LOG_WARN("fail to get from map", KR(ret), K(zone));
    }
  }
  return ret;
}

ObDRWorker::LocalityAlignment::LocalityAlignment(DRLSInfo &dr_ls_info, const uint64_t tenant_id)
  : task_idx_(0),
    add_replica_task_(),
    dr_ls_info_(dr_ls_info),
    task_array_(),
    curr_paxos_replica_number_(0),
    locality_paxos_replica_number_(0),
    locality_map_(),
    replica_stat_map_(),
    unit_provider_(),
    allocator_("ObDRWorker", OB_MALLOC_NORMAL_BLOCK_SIZE, tenant_id)
{
}

ObDRWorker::LocalityAlignment::~LocalityAlignment()
{
  for (int64_t i = 0; i < task_array_.count(); ++i) {
    LATask *task = task_array_.at(i);
    if (nullptr != task) {
      task->~LATask();
    }
  }
  for (LocalityMap::iterator iter = locality_map_.begin(); iter != locality_map_.end(); ++iter) {
    ReplicaDescArray *val = iter->second;
    if (nullptr != val) {
      val->~ReplicaDescArray();
    }
  }
  allocator_.reset(); // free the memory above
}

int ObDRWorker::LocalityAlignment::generate_paxos_replica_number()
{
  int ret = OB_SUCCESS;
  const ObIArray<ObZoneReplicaAttrSet> &locality_array = dr_ls_info_.get_locality();
  curr_paxos_replica_number_ = dr_ls_info_.get_paxos_replica_number();
  locality_paxos_replica_number_ = 0;
  for (int64_t i = 0; OB_SUCC(ret) && i < locality_array.count(); ++i) {
    const ObZoneReplicaAttrSet &locality = locality_array.at(i);
    locality_paxos_replica_number_ += locality.get_paxos_replica_num();
  }
  return ret;
}

int ObDRWorker::LocalityAlignment::build_locality_stat_map()
{
  int ret = OB_SUCCESS;
  uint64_t tenant_id = OB_INVALID_ID;
  share::ObLSID ls_id;
  if (OB_FAIL(dr_ls_info_.get_ls_id(tenant_id, ls_id))) {
    LOG_WARN("fail to get ls id", KR(ret));
  } else {
    LOG_INFO("build ls locality stat map", K(tenant_id), K(ls_id));
    for (int64_t i = 0; OB_SUCC(ret) && i < dr_ls_info_.get_locality().count(); ++i) {
      ReplicaDescArray *zone_replica_desc = nullptr;
      const ObZoneReplicaAttrSet &zone_locality = dr_ls_info_.get_locality().at(i);
      const common::ObZone &zone = zone_locality.zone_;
      // full locality
      const ObIArray<ReplicaAttr> &full_locality =
        zone_locality.replica_attr_set_.get_full_replica_attr_array();
      // logonly locality
      const ObIArray<ReplicaAttr> &logonly_locality =
        zone_locality.replica_attr_set_.get_logonly_replica_attr_array();
      // encryption logonly locality
      const ObIArray<ReplicaAttr> &encryption_locality =
        zone_locality.replica_attr_set_.get_encryption_logonly_replica_attr_array();
      // readonly locality
      const ObIArray<ReplicaAttr> &readonly_locality =
        zone_locality.replica_attr_set_.get_readonly_replica_attr_array();
      // columnstore locality
      const ObIArray<ReplicaAttr> &columnstore_locality =
        zone_locality.replica_attr_set_.get_columnstore_replica_attr_array();

      if (OB_FAIL(locate_zone_locality(zone, zone_replica_desc))) {
        LOG_WARN("fail to locate zone locality", KR(ret), K(zone));
      } else if (OB_ISNULL(zone_replica_desc)) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("fail to locate zone locality", KR(ret), K(zone));
      } else {
        // full replica
        for (int64_t j = 0; OB_SUCC(ret) && j < full_locality.count(); ++j) {
          const ReplicaAttr &replica_attr = full_locality.at(j);
          if (1 != replica_attr.num_) {
            ret = OB_ERR_UNEXPECTED;
            LOG_WARN("replica num unexpected", KR(ret), K(zone), K(full_locality));
          } else if (OB_FAIL(zone_replica_desc->push_back(ReplicaDesc(REPLICA_TYPE_FULL,
                                                                      replica_attr.memstore_percent_,
                                                                      replica_attr.num_)))) {
            LOG_WARN("fail to push back", KR(ret));
          }
        }
        // logonly replica
        for (int64_t j = 0; OB_SUCC(ret) && j < logonly_locality.count(); ++j) {
          const ReplicaAttr &replica_attr = logonly_locality.at(j);
          if (1 != replica_attr.num_) {
            ret = OB_ERR_UNEXPECTED;
            LOG_WARN("replica num unexpected", KR(ret), K(zone), K(logonly_locality));
          } else if (OB_FAIL(zone_replica_desc->push_back(ReplicaDesc(REPLICA_TYPE_LOGONLY,
                                                                      replica_attr.memstore_percent_,
                                                                      replica_attr.num_)))) {
            LOG_WARN("fail to push back", KR(ret));
          }
        }
        // encryption logonly replica
        for (int64_t j = 0; OB_SUCC(ret) && j < encryption_locality.count(); ++j) {
          const ReplicaAttr &replica_attr = encryption_locality.at(j);
          if (1 != replica_attr.num_) {
            ret = OB_ERR_UNEXPECTED;
            LOG_WARN("replica num unexpected", KR(ret), K(zone), K(encryption_locality));
          } else if (OB_FAIL(zone_replica_desc->push_back(ReplicaDesc(REPLICA_TYPE_ENCRYPTION_LOGONLY,
                                                                      replica_attr.memstore_percent_,
                                                                      replica_attr.num_)))) {
            LOG_WARN("fail to push back", KR(ret));
          }
        }
        // readonly or colmnstore replica, all_server
        if (dr_ls_info_.is_duplicate_ls()) {
          // duplicate ls, should has R-replica or C-replica all_server
          //   if locality is C, then set columnstore_all_server.
          //   if locality is F/R, then set readonly_all_server,
          if (columnstore_locality.count() > 0) {
            // dup_ls must not be sys_ls
            zone_replica_desc->set_columnstore_all_server();
          } else {
            zone_replica_desc->set_readonly_all_server();
          }
        } else {
          // readonly replica, normal
          for (int64_t j = 0; OB_SUCC(ret) && j < readonly_locality.count(); ++j) {
            const ReplicaAttr &replica_attr = readonly_locality.at(j);
            if (0 >= replica_attr.num_) {
              ret = OB_ERR_UNEXPECTED;
              LOG_WARN("replica num unexpected", KR(ret), K(zone), K(readonly_locality));
            } else if (OB_FAIL(zone_replica_desc->push_back(ReplicaDesc(REPLICA_TYPE_READONLY,
                                                                        replica_attr.memstore_percent_,
                                                                        replica_attr.num_)))) {
              LOG_WARN("fail to push back", KR(ret));
            }
          }
          // columnstore replica, normal
          if (ls_id.is_sys_ls()) {
            // for sys ls, ignore C-replica in locality
          } else {
            for (int64_t j = 0; OB_SUCC(ret) && j < columnstore_locality.count(); ++j) {
              const ReplicaAttr &replica_attr = columnstore_locality.at(j);
              if (0 >= replica_attr.num_) {
                ret = OB_ERR_UNEXPECTED;
                LOG_WARN("replica num unexpected", KR(ret), K(zone), K(columnstore_locality));
              } else if (OB_FAIL(zone_replica_desc->push_back(ReplicaDesc(REPLICA_TYPE_COLUMNSTORE,
                                                                          replica_attr.memstore_percent_,
                                                                          replica_attr.num_)))) {
                LOG_WARN("fail to push back", KR(ret));
              }
            }
          }
        }
      }
      LOG_INFO("ls zone locality map info", K(zone), KPC(zone_replica_desc));
    }
  }
  return ret;
}

int ObDRWorker::LocalityAlignment::build_replica_stat_map()
{
  int ret = OB_SUCCESS;
  int64_t replica_cnt = 0;
  if (OB_FAIL(dr_ls_info_.get_replica_cnt(replica_cnt))) {
    LOG_WARN("fail to get replica cnt", KR(ret));
  } else {
    for (int64_t i = 0; OB_SUCC(ret) && i < replica_cnt; ++i) {
      share::ObLSReplica *replica = nullptr;
      DRServerStatInfo *server_stat_info = nullptr;
      DRUnitStatInfo *unit_stat_info = nullptr;
      DRUnitStatInfo *unit_in_group_stat_info = nullptr;
      if (OB_FAIL(dr_ls_info_.get_replica_stat(
              i,
              replica,
              server_stat_info,
              unit_stat_info,
              unit_in_group_stat_info))) {
        LOG_WARN("fail to get replica stat", KR(ret));
      } else if (OB_ISNULL(replica)
                 || OB_ISNULL(server_stat_info)
                 || OB_ISNULL(unit_stat_info)
                 || OB_ISNULL(unit_in_group_stat_info)) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("replica related ptrs are null", KR(ret),
                 KP(replica),
                 KP(server_stat_info),
                 KP(unit_stat_info),
                 KP(unit_in_group_stat_info));
      } else if (OB_FAIL(replica_stat_map_.push_back(
              ReplicaStatDesc(replica,
                              server_stat_info,
                              unit_stat_info,
                              unit_in_group_stat_info)))) {
        LOG_WARN("fail to push back", KR(ret));
      }
    }
    LOG_INFO("ls replica stat", "replica_stat_map", replica_stat_map_);
  }
  return ret;
}

int ObDRWorker::LocalityAlignment::try_remove_match(
    ReplicaStatDesc &replica_stat_desc,
    const int64_t index)
{
  // replica_stat_desc contains informations of one replica in __all_ls_meta_table.
  // try_remove_match() aimed to figure out whether this replica's location and type is expected.
  // If is expected, we remove it from both replica_desc and locality_desc.
  // If is not expected, reserve this replica in replica_desc and locality_desc,
  // let do_generate_locality_task() generate certain tasks by referencing replica_desc and locality_desc later.
  //
  // We regard a replica is expected if these rules below all satisfied:
  //   (rule 1) this replica's unit is in pool
  //   (rule 2) this replica's unit is in active status
  //   (rule 3) the server of the replica is the same as the server of the unit it belongs
  //   (rule 4) the type of this replica is the same as locality described
  //   (rule 5) the memstore_percent is the same as locality described
  //   (rule 6) the remained replica number described in locality is not 0
  //
  // Rule 2 can ensure not removing replica on active unit when another unit is in deleting status
  // Rule 3 can ensure not removing migrate dest replica when source replica and dest replica both exists in member_list(learner_list)
  //
  // Under the rules described above, consider this case:
  //   This replica is migrate source replica or the unit it belongs is deleting AND dest replica not exist yet.
  //   This replica can remained in replica_desc and locality_desc, because one of those rules not satisfied.
  //   BUT we should treat this replica is expected and remove it from replica_desc and locality_desc anyway.
  //   Because we want migrate_unit() and shrink_resource_pool() to handle this situation, DO NOT let locality_alignment generate tasks
  int ret = OB_SUCCESS;
  share::ObLSReplica *replica = nullptr;
  DRServerStatInfo *server_stat_info = nullptr;
  DRUnitStatInfo *unit_stat_info = nullptr;
  DRUnitStatInfo *unit_in_group_stat_info = nullptr;
  if (OB_UNLIKELY(!replica_stat_desc.is_valid()
                  || index >= replica_stat_map_.count())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(replica_stat_desc), K(index),
             "replica_stat_map_count", replica_stat_map_.count());
  } else if (OB_ISNULL(replica = replica_stat_desc.replica_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("replica ptr is null", KR(ret), K(replica_stat_desc));
  } else if (OB_ISNULL(server_stat_info = replica_stat_desc.server_stat_info_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("server stat info ptr is null", KR(ret));
  } else if (OB_ISNULL(unit_stat_info = replica_stat_desc.unit_stat_info_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("unit stat info ptr is null", KR(ret));
  } else if (OB_ISNULL(unit_in_group_stat_info = replica_stat_desc.unit_in_group_stat_info_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("unit in group stat info ptr is null", KR(ret));
  } else {
    const common::ObZone &zone = replica->get_zone();
    ReplicaDescArray *zone_replica_desc = nullptr;
    int tmp_ret = locality_map_.get_refactored(zone, zone_replica_desc);
    if (OB_HASH_NOT_EXIST == tmp_ret) {
      // zone not exist in locality, not match
    } else if (OB_SUCCESS == tmp_ret) {
      if (OB_ISNULL(zone_replica_desc)) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("zone replica desc ptr is null", KR(ret), K(zone));
      } else {
        bool has_correct_dest_replica = false;
        if (replica->get_server() != unit_stat_info->get_unit().server_
            || replica->get_server() != unit_in_group_stat_info->get_unit().server_
            || !unit_stat_info->get_unit().is_active_status()) {
          // this replica is migrating or unit is deleting, check whether has a correct dest replica
          LOG_TRACE("try to check whether has dest replica", KPC(replica), KPC(unit_stat_info), KPC(unit_in_group_stat_info));
          const int64_t map_count = replica_stat_map_.count();
          for (int64_t i = map_count - 1; OB_SUCC(ret) && i >= 0; --i) {
            // A replica is a correct dest replica if following conditions all satisfied
            //   (1) replica is in member_list(learner_lsit)
            //   (2) replica type is expected
            //   (3) replica is on the server the same as source replica's unit (a unit migrate task triggered expected dest replica)
            //       or replica is on the server the same as its own unit (a shrink resource task triggerd expected dest replica)
            ReplicaStatDesc &replica_stat_desc_to_compare = replica_stat_map_.at(i);
            if (OB_UNLIKELY(!replica_stat_desc_to_compare.is_valid())) {
              ret = OB_ERR_UNEXPECTED;
              LOG_WARN("replica stat desc unexpected", KR(ret), K(replica_stat_desc_to_compare));
            } else if (OB_ISNULL(replica_stat_desc_to_compare.unit_stat_info_)
                || OB_ISNULL(replica_stat_desc_to_compare.unit_in_group_stat_info_)) {
              ret = OB_ERR_UNEXPECTED;
              LOG_WARN("invalid argument", KR(ret), K(replica_stat_desc_to_compare));
            } else if (replica->get_zone() != replica_stat_desc_to_compare.replica_->get_zone()) {
              // not the same zone, just skip
            } else if (replica->is_in_service()
                && replica_stat_desc_to_compare.replica_->get_server() != replica->get_server()
                && replica_stat_desc_to_compare.replica_->get_replica_type() == replica->get_replica_type()) {
              if (replica_stat_desc_to_compare.replica_->get_server() == unit_stat_info->get_unit().server_
                  && replica_stat_desc_to_compare.replica_->get_server() == unit_in_group_stat_info->get_unit().server_) {
                // migrate dest unit has matching replica
                has_correct_dest_replica = true;
              } else if (replica_stat_desc_to_compare.replica_->get_server() == replica_stat_desc_to_compare.unit_stat_info_->get_unit().server_
                  && replica_stat_desc_to_compare.replica_->get_server() == replica_stat_desc_to_compare.unit_in_group_stat_info_->get_unit().server_) {
                // shrink dest unit has matching replica
                has_correct_dest_replica = true;
              }
            }
            if (has_correct_dest_replica) {
              break;
            } else {
              LOG_TRACE("dest replica not match",
                        "replica_type_to_compare", replica_stat_desc_to_compare.replica_->get_replica_type(),
                        "replica_type", replica->get_replica_type(),
                        "server_to_compare", replica_stat_desc_to_compare.replica_->get_server(),
                        "server", replica->get_server(),
                        "server_with_unit", unit_stat_info->get_unit().server_,
                        "server_with_unit_to_compare", replica_stat_desc_to_compare.unit_stat_info_->get_unit().server_,
                        "unit_status_is_active", replica_stat_desc_to_compare.unit_stat_info_->get_unit().is_active_status());
            }
          } // end for
        }

        for (int64_t i = zone_replica_desc->count() - 1; OB_SUCC(ret) && i >= 0; --i) {
          bool found = false;
          ReplicaDesc &replica_desc = zone_replica_desc->at(i);
          if (unit_stat_info->is_in_pool()
              && replica->get_replica_type() == replica_desc.replica_type_
              && replica->get_memstore_percent() == replica_desc.memstore_percent_
              && replica_desc.replica_num_ > 0
              && !has_correct_dest_replica) {
            found = true;
            if (OB_FAIL(replica_stat_map_.remove(index))) {
              LOG_WARN("fail to remove from stat map", KR(ret));
            } else if (FALSE_IT(--replica_desc.replica_num_)) {
              // shall never be here
            } else if (replica_desc.replica_num_ > 0) {
              // bypass
            } else if (OB_FAIL(zone_replica_desc->remove(i))) {
              LOG_WARN("fail to remove element", KR(ret));
            } else {
              break;
            }
          }
        }
      }
    } else {
      ret = tmp_ret;
      LOG_WARN("fail to get from locality map", KR(ret), K(zone));
    }
  }
  return ret;
}

int ObDRWorker::LocalityAlignment::prepare_generate_locality_task()
{
  int ret = OB_SUCCESS;
  const int64_t map_count = replica_stat_map_.count();
  for (int64_t i = map_count - 1; OB_SUCC(ret) && i >= 0; --i) {
    ReplicaStatDesc &replica_stat_desc = replica_stat_map_.at(i);
    if (OB_UNLIKELY(!replica_stat_desc.is_valid())) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("replica stat desc unexpected", KR(ret));
    } else if (OB_FAIL(try_remove_match(replica_stat_desc, i))) {
      LOG_WARN("fail to try remove match", KR(ret));
    }
  }
  return ret;
}

int ObDRWorker::LocalityAlignment::do_generate_locality_task_from_full_replica(
    ReplicaStatDesc &replica_stat_desc,
    share::ObLSReplica &replica,
    const int64_t index)
{
  int ret = OB_SUCCESS;
  const common::ObZone &zone = replica.get_zone();
  if (REPLICA_TYPE_FULL != replica.get_replica_type()) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("replica type unexpected", KR(ret), K(replica));
  } else {
    ReplicaDescArray *zone_replica_desc = nullptr;
    int tmp_ret = locality_map_.get_refactored(zone, zone_replica_desc);
    if (OB_HASH_NOT_EXIST == tmp_ret) {
      if (OB_FAIL(generate_remove_replica_task(replica_stat_desc))) {
        LOG_WARN("fail to generate remove replica task", KR(ret));
      } else if (OB_FAIL(replica_stat_map_.remove(index))) {
        LOG_WARN("fail to remove", KR(ret));
      }
    } else if (OB_SUCCESS == tmp_ret && nullptr != zone_replica_desc) {
      bool found = false;
      lib::ob_sort(zone_replica_desc->begin(), zone_replica_desc->end());
      for (int64_t i = zone_replica_desc->count() - 1; !found && OB_SUCC(ret) && i >= 0; --i) {
        ReplicaDesc &replica_desc = zone_replica_desc->at(i);
        if (REPLICA_TYPE_FULL == replica_desc.replica_type_) {
          ret = OB_ERR_UNEXPECTED;
          LOG_WARN("replica type unexpected", KR(ret), K(dr_ls_info_));
        } else if (REPLICA_TYPE_LOGONLY == replica_desc.replica_type_
            || REPLICA_TYPE_READONLY == replica_desc.replica_type_) {
          if (OB_FAIL(generate_type_transform_task(
                  replica_stat_desc,
                  replica_desc.replica_type_,
                  replica_desc.memstore_percent_))) {
            LOG_WARN("fail to generate type transform task", KR(ret), K(replica_stat_desc));
          } else if (OB_FAIL(zone_replica_desc->remove(i))) {
            LOG_WARN("fail to remove", KR(ret), K(i), K(replica), KPC(zone_replica_desc));
          } else if (OB_FAIL(replica_stat_map_.remove(index))) {
            LOG_WARN("fail to remove", KR(ret), K(index), K(replica), K(replica_stat_map_));
          } else {
            found = true;
          }
        } else if (ObReplicaTypeCheck::is_columnstore_replica(replica_desc.replica_type_)) {
          // if zone_locality is C-replica, remove this F-replica
          // because transform from F to C is not supported
          if (OB_FAIL(generate_remove_replica_task(replica_stat_desc))) {
            LOG_WARN("fail to generate remove replica task", KR(ret));
          } else if (OB_FAIL(replica_stat_map_.remove(index))) {
            LOG_WARN("fail to remove", KR(ret));
          } else {
            found = true;
          }
        }
      }
      // process not found
      if (OB_FAIL(ret)) {
        // failed
      } else if (found) {
        // found, bypass
      } else if (zone_replica_desc->is_columnstore_all_server()) {
        // remove this F, because transform from F to C is not supported
        if (OB_FAIL(generate_remove_replica_task(replica_stat_desc))) {
          LOG_WARN("fail to generate remove replica task", KR(ret));
        } else if (OB_FAIL(replica_stat_map_.remove(index))) {
          LOG_WARN("fail to remove", KR(ret));
        }
      } else if (zone_replica_desc->is_readonly_all_server()) {
        if (OB_FAIL(generate_type_transform_task(
                replica_stat_desc,
                REPLICA_TYPE_READONLY,
                zone_replica_desc->get_readonly_memstore_percent()))) {
          LOG_WARN("fail to generate type transform task", KR(ret), K(replica_stat_desc));
        } else if (OB_FAIL(replica_stat_map_.remove(index))) {
          LOG_WARN("fail to remove", KR(ret), K(index), K(replica), K(replica_stat_map_));
        }
      } else {
        if (OB_FAIL(generate_remove_replica_task(replica_stat_desc))) {
          LOG_WARN("fail to generate remove replica task", KR(ret));
        } else if (OB_FAIL(replica_stat_map_.remove(index))) {
          LOG_WARN("fail to remove", KR(ret));
        }
      }
    } else {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("fail to get refactored", KR(ret), K(zone));
    }
  }
  return ret;
}

int ObDRWorker::LocalityAlignment::do_generate_locality_task_from_logonly_replica(
    ReplicaStatDesc &replica_stat_desc,
    share::ObLSReplica &replica,
    const int64_t index)
{
  int ret = OB_SUCCESS;
  const common::ObZone &zone = replica.get_zone();
  if (REPLICA_TYPE_LOGONLY != replica.get_replica_type()) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("replica type unexpected", KR(ret), K(replica));
  } else {
    ReplicaDescArray *zone_replica_desc = nullptr;
    int tmp_ret = locality_map_.get_refactored(zone, zone_replica_desc);
    if (OB_HASH_NOT_EXIST == tmp_ret) {
      if (OB_FAIL(generate_remove_replica_task(replica_stat_desc))) {
        LOG_WARN("fail to generate remove replica task", KR(ret));
      } else if (OB_FAIL(replica_stat_map_.remove(index))) {
        LOG_WARN("fail to remove", KR(ret));
      }
    } else if (OB_SUCCESS == tmp_ret && nullptr != zone_replica_desc) {
      bool found = false;
      lib::ob_sort(zone_replica_desc->begin(), zone_replica_desc->end());
      // defensive check
      for (int64_t i = zone_replica_desc->count() - 1; !found && OB_SUCC(ret) && i >= 0; --i) {
        ReplicaDesc &replica_desc = zone_replica_desc->at(i);
        if (REPLICA_TYPE_LOGONLY == replica_desc.replica_type_) {
          ret = OB_ERR_UNEXPECTED;
          LOG_WARN("replica type unexpected", KR(ret), K(dr_ls_info_));
        }
      }
      // normal routine
      if (OB_SUCC(ret)) {
        if (OB_FAIL(generate_remove_replica_task(replica_stat_desc))) {
          LOG_WARN("fail to generate remove replica task", KR(ret));
        } else if (OB_FAIL(replica_stat_map_.remove(index))) {
          LOG_WARN("fail to remove", KR(ret));
        }
      }
    } else {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("fail to get refactored", KR(ret), K(zone));
    }
  }
  return ret;
}

int ObDRWorker::LocalityAlignment::do_generate_locality_task_from_encryption_logonly_replica(
    ReplicaStatDesc &replica_stat_desc,
    share::ObLSReplica &replica,
    const int64_t index)
{
  int ret = OB_SUCCESS;
  const common::ObZone &zone = replica.get_zone();
  if (REPLICA_TYPE_ENCRYPTION_LOGONLY != replica.get_replica_type()) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("replica type unexpected", KR(ret), K(replica));
  } else {
    ReplicaDescArray *zone_replica_desc = nullptr;
    int tmp_ret = locality_map_.get_refactored(zone, zone_replica_desc);
    if (OB_HASH_NOT_EXIST == tmp_ret) {
      if (OB_FAIL(generate_remove_replica_task(replica_stat_desc))) {
        LOG_WARN("fail to generate remove replica task", KR(ret));
      } else if (OB_FAIL(replica_stat_map_.remove(index))) {
        LOG_WARN("fail to remove", KR(ret));
      }
    } else if (OB_SUCCESS == tmp_ret && nullptr != zone_replica_desc) {
      bool found = false;
      lib::ob_sort(zone_replica_desc->begin(), zone_replica_desc->end());
      // defensive check
      for (int64_t i = zone_replica_desc->count() - 1; !found && OB_SUCC(ret) && i >= 0; --i) {
        ReplicaDesc &replica_desc = zone_replica_desc->at(i);
        if (REPLICA_TYPE_ENCRYPTION_LOGONLY == replica_desc.replica_type_) {
          ret = OB_ERR_UNEXPECTED;
          LOG_WARN("replica type unexpected", KR(ret), K(dr_ls_info_));
        }
      }
      // normal routine
      if (OB_SUCC(ret)) {
        if (OB_FAIL(generate_remove_replica_task(replica_stat_desc))) {
          LOG_WARN("fail to generate remove replica task", KR(ret));
        } else if (OB_FAIL(replica_stat_map_.remove(index))) {
          LOG_WARN("fail to remove", KR(ret));
        }
      }
    } else {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("fail to get refactored", KR(ret), K(zone));
    }
  }
  return ret;
}

int ObDRWorker::LocalityAlignment::try_generate_task_for_readonly_replica_(
    ReplicaDescArray &zone_replica_desc_in_locality,
    ReplicaStatDesc &replica_stat_desc,
    const int64_t index,
    bool &task_generated)
{
  int ret = OB_SUCCESS;
  task_generated = false;
  if (OB_UNLIKELY(0 > index)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(index));
  } else {
    for (int64_t i = zone_replica_desc_in_locality.count() - 1; !task_generated && OB_SUCC(ret) && i >= 0; --i) {
      ReplicaDesc &replica_desc = zone_replica_desc_in_locality.at(i);
      if (REPLICA_TYPE_READONLY == replica_desc.replica_type_) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("replica type unexpected", KR(ret), K(dr_ls_info_));
      } else if (ObReplicaTypeCheck::is_columnstore_replica(replica_desc.replica_type_)) {
        // if locality is C-replica, remove this R-replica because transform from R to C is not supported
        if (OB_FAIL(generate_remove_replica_task(replica_stat_desc))) {
          LOG_WARN("fail to generate remove replica task", KR(ret), K(replica_stat_desc));
        } else if (OB_FAIL(replica_stat_map_.remove(index))) {
          LOG_WARN("fail to remove", KR(ret), K(replica_stat_map_), K(index));
        } else {
          task_generated = true;
        }
      } else if (REPLICA_TYPE_FULL == replica_desc.replica_type_) {
        if (OB_ISNULL(replica_stat_desc.unit_stat_info_)) {
          ret = OB_INVALID_ARGUMENT;
          LOG_WARN("invalid argument", KR(ret), K(replica_stat_desc));
        } else {
          const share::ObUnit &unit = replica_stat_desc.unit_stat_info_->get_unit();
          bool server_is_active = false;
          if (!unit.is_active_status()) {
            FLOG_INFO("unit status is not normal, can not generate type transform task", K(unit));
          } else if (OB_FAIL(SVR_TRACER.check_server_active(unit.server_, server_is_active))) {
            LOG_WARN("fail to check server is active", KR(ret), K(unit));
          } else if (!server_is_active) {
            FLOG_INFO("server status is not active, can not generate type transform task", K(unit));
          } else if (OB_FAIL(generate_type_transform_task(
                  replica_stat_desc,
                  replica_desc.replica_type_,
                  replica_desc.memstore_percent_))) {
            LOG_WARN("fail to generate type transform task", KR(ret), K(replica_stat_desc));
          } else if (OB_FAIL(zone_replica_desc_in_locality.remove(i))) {
            LOG_WARN("fail to remove", KR(ret), K(i), K(replica_stat_desc), K(zone_replica_desc_in_locality));
          } else if (OB_FAIL(replica_stat_map_.remove(index))) {
            LOG_WARN("fail to remove", KR(ret), K(index), K(replica_stat_desc), K(replica_stat_map_));
          } else {
            task_generated = true;
          }
        }
      }
    }
  }
  return ret;
}

int ObDRWorker::LocalityAlignment::try_generate_remove_redundant_replica_task_for_dup_ls_(
    ReplicaStatDesc &replica_stat_desc,
    share::ObLSReplica &replica,
    const int64_t index)
{
  int ret = OB_SUCCESS;
  ObUnitTableOperator unit_operator;
  common::ObArray<share::ObUnit> unit_info_array;
  if (OB_ISNULL(GCTX.sql_proxy_) || OB_UNLIKELY(0 > index)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(index));
  } else if (OB_FAIL(unit_operator.init(*GCTX.sql_proxy_))) {
    LOG_WARN("unit operator init failed", KR(ret));
  } else if (OB_FAIL(unit_operator.get_units_by_tenant(gen_user_tenant_id(replica.get_tenant_id()), unit_info_array))) {
    LOG_WARN("fail to get unit info array", KR(ret), K(replica));
  } else {
    bool replica_need_delete = true;
    for (int64_t j = 0; OB_SUCC(ret) && j < unit_info_array.count(); ++j) {
      if (unit_info_array.at(j).server_ == replica.get_server()) {
        replica_need_delete = false;
        break;
      }
    }
    if (OB_FAIL(ret)) {
    } else if (replica_need_delete) {
      // this R-replica need to be deleted
      if (OB_FAIL(generate_remove_replica_task(replica_stat_desc))) {
        LOG_WARN("fail to generate remove replica task", KR(ret), K(replica_stat_desc));
      } else if (OB_FAIL(replica_stat_map_.remove(index))) {
        LOG_WARN("fail to remove", KR(ret), K(index), K(replica), K(replica_stat_map_));
      }
    } else {
      // delete this R-replica from memory to avoid removing this replca
      if (OB_FAIL(replica_stat_map_.remove(index))) {
        LOG_WARN("fail to remove", KR(ret), K(replica_stat_map_), K(index));
      }
    }
  }
  return ret;
}

int ObDRWorker::LocalityAlignment::do_generate_locality_task_from_readonly_replica(
    ReplicaStatDesc &replica_stat_desc,
    share::ObLSReplica &replica,
    const int64_t index)
{
  int ret = OB_SUCCESS;
  const common::ObZone &zone = replica.get_zone();
  if (REPLICA_TYPE_READONLY != replica.get_replica_type()) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("replica type unexpected", KR(ret), K(replica));
  } else {
    ReplicaDescArray *zone_replica_desc = nullptr;
    int tmp_ret = locality_map_.get_refactored(zone, zone_replica_desc);
    if (OB_HASH_NOT_EXIST == tmp_ret) {
      // zone has been shrinked, generate remove replica task
      if (OB_FAIL(generate_remove_replica_task(replica_stat_desc))) {
        LOG_WARN("fail to generate remove replica task", KR(ret));
      } else if (OB_FAIL(replica_stat_map_.remove(index))) {
        LOG_WARN("fail to remove", KR(ret));
      }
    } else if (OB_SUCCESS == tmp_ret && OB_NOT_NULL(zone_replica_desc)) {
      bool task_generated = false;
      lib::ob_sort(zone_replica_desc->begin(), zone_replica_desc->end());
      // try to generate type_transform task if needed
      if (OB_FAIL(try_generate_task_for_readonly_replica_(
                      *zone_replica_desc, replica_stat_desc, index, task_generated))) {
        LOG_WARN("fail to try generate type transform task", KR(ret),
                 KPC(zone_replica_desc), K(replica_stat_desc), K(index), K(task_generated));
      } else if (task_generated) {
        // a type transform task or remove task generated, bypass
      } else if (zone_replica_desc->is_columnstore_all_server()) {
        // for duplicate log stream, if locality is C-replica, remove this R-replica
        // because transform from R to C is not supported
        if (OB_FAIL(generate_remove_replica_task(replica_stat_desc))) {
          LOG_WARN("fail to generate remove replica task", KR(ret), K(replica_stat_desc));
        } else if (OB_FAIL(replica_stat_map_.remove(index))) {
          LOG_WARN("fail to remove", KR(ret), K(replica_stat_map_), K(index));
        }
      } else if (zone_replica_desc->is_readonly_all_server()) {
        // for duplicate log stream, if locality is R-replica, try to remove redudant R-replicas
        if (OB_FAIL(try_generate_remove_redundant_replica_task_for_dup_ls_(replica_stat_desc, replica, index))) {
          LOG_WARN("fail to generate remove replica task for duplicate log stream",
                   KR(ret), K(replica_stat_desc), K(replica), K(index));
        }
      } else {
        // for common log stream, just generate remove replica task
        if (OB_FAIL(generate_remove_replica_task(replica_stat_desc))) {
          LOG_WARN("fail to generate remove replica task", KR(ret), K(replica_stat_desc));
        } else if (OB_FAIL(replica_stat_map_.remove(index))) {
          LOG_WARN("fail to remove", KR(ret), K(replica_stat_map_), K(index));
        }
      }
    } else {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("fail to get refactored", KR(ret), K(zone));
    }
  }
  return ret;
}

int ObDRWorker::LocalityAlignment::try_generate_locality_task_from_locality_map()
{
  int ret = OB_SUCCESS;
  LocalityMap::iterator iter = locality_map_.begin();
  for (; iter != locality_map_.end(); ++iter) {
    ReplicaDescArray *replica_desc_array = iter->second;
    if (OB_ISNULL(replica_desc_array)) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("zone locality ptr is null", KR(ret), "zone", iter->first);
    } else {
      for (int64_t i = 0; OB_SUCC(ret) && i < replica_desc_array->count(); ++i) {
        if (OB_FAIL(generate_add_replica_task(iter->first, replica_desc_array->at(i)))) {
          LOG_WARN("fail to generate add replica task", KR(ret));
        } else {
          LOG_INFO("success generate add replica task from locality", KR(ret));
        }
      }
    }
  }
  return ret;
}

int ObDRWorker::LocalityAlignment::try_generate_locality_task_from_paxos_replica_number()
{
  int ret = OB_SUCCESS;
  if (curr_paxos_replica_number_ == locality_paxos_replica_number_) {
    // bypass, paxos_replica_number match
  } else if (OB_FAIL(generate_modify_paxos_replica_number_task())) {
    LOG_WARN("fail to generate modify paxos replica number task", KR(ret));
  }
  return ret;
}

int ObDRWorker::LocalityAlignment::do_generate_locality_task_from_columnstore_replica(
    ReplicaStatDesc &replica_stat_desc,
    share::ObLSReplica &replica,
    const int64_t index)
{
  int ret = OB_SUCCESS;
  const common::ObZone &zone = replica.get_zone();
  if (REPLICA_TYPE_COLUMNSTORE != replica.get_replica_type()) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("replica type unexpected", KR(ret), K(replica));
  } else {
    ReplicaDescArray *zone_replica_desc = nullptr;
    int tmp_ret = locality_map_.get_refactored(zone, zone_replica_desc);
    if (OB_HASH_NOT_EXIST == tmp_ret) {
      if (OB_FAIL(generate_remove_replica_task(replica_stat_desc))) {
        LOG_WARN("fail to generate remove replica task", KR(ret), K(replica_stat_desc));
      } else if (OB_FAIL(replica_stat_map_.remove(index))) {
        LOG_WARN("fail to remove", KR(ret), K(replica_stat_map_), K(index));
      }
    } else if (OB_SUCCESS == tmp_ret && OB_NOT_NULL(zone_replica_desc)) {
      bool task_generated = false;
      lib::ob_sort(zone_replica_desc->begin(), zone_replica_desc->end());
      // defensive check
      for (int64_t i = zone_replica_desc->count() - 1; OB_SUCC(ret) && i >= 0; --i) {
        ReplicaDesc &replica_desc = zone_replica_desc->at(i);
        if (ObReplicaTypeCheck::is_columnstore_replica(replica_desc.replica_type_)) {
          ret = OB_ERR_UNEXPECTED;
          LOG_WARN("replica type unexpected", KR(ret), K(replica_desc), K(dr_ls_info_));
        }
      }
      // normal routine
      if (OB_SUCC(ret)) {
        if (zone_replica_desc->is_columnstore_all_server()) {
          // for duplicate log stream, if locality is C-replica, try to remove redudant C-replicas
          if (OB_FAIL(try_generate_remove_redundant_replica_task_for_dup_ls_(replica_stat_desc, replica, index))) {
            LOG_WARN("fail to generate remove replica task for duplicate log stream",
                    KR(ret), K(replica_stat_desc), K(replica), K(index));
          }
        } else {
          // handle two abnormal occasions:
          // 1. for duplicate ls, locality is R or F-replica;
          // 2. for non-duplicate ls, locality is other type than C-replica
          // in these both occasions, directly remove this C-replica,
          // because transform from C-replica to R/F is not supported.
          if (OB_FAIL(generate_remove_replica_task(replica_stat_desc))) {
            LOG_WARN("fail to generate remove replica task", KR(ret), K(replica_stat_desc));
          } else if (OB_FAIL(replica_stat_map_.remove(index))) {
            LOG_WARN("fail to remove", KR(ret), K(replica_stat_map_), K(index));
          }
        }
      }
    } else {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("fail to get refactored", KR(ret), K(zone));
    }
  }
  return ret;
}

void ObDRWorker::LocalityAlignment::print_locality_information()
{
  for (LocalityMap::iterator iter = locality_map_.begin();
       iter != locality_map_.end();
       ++iter) {
    if (nullptr == iter->second) {
      LOG_INFO("zone locality ptr is null", "zone", iter->first);
    } else {
      LOG_INFO("zone locality", "zone", iter->first, "locality_info", *iter->second);
    }
  }
  LOG_INFO("replica stat map not empty", K(replica_stat_map_));
}

int ObDRWorker::LocalityAlignment::do_generate_locality_task()
{
  int ret = OB_SUCCESS;
  // step0: generate task from replica stat map
  for (int64_t i = replica_stat_map_.count() - 1; OB_SUCC(ret) && i >= 0; --i) {
    ReplicaStatDesc &replica_stat_desc = replica_stat_map_.at(i);
    share::ObLSReplica *replica = replica_stat_desc.replica_;
    if (OB_UNLIKELY(!replica_stat_desc.is_valid())) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("replica stat desc unexpected", KR(ret));
    } else if (OB_ISNULL(replica)) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("replica ptr is null", KR(ret));
    } else if (REPLICA_TYPE_FULL == replica->get_replica_type()) {
      if (OB_FAIL(do_generate_locality_task_from_full_replica(
              replica_stat_desc,
              *replica,
              i))) {
        LOG_WARN("fail to generate locality task from full replica", KR(ret));
      }
    } else if (REPLICA_TYPE_LOGONLY == replica->get_replica_type()) {
      if (OB_FAIL(do_generate_locality_task_from_logonly_replica(
              replica_stat_desc,
              *replica,
              i))) {
        LOG_WARN("fail to generate locality task from logonly replica", KR(ret));
      }
    } else if (REPLICA_TYPE_ENCRYPTION_LOGONLY == replica->get_replica_type()) {
      if (OB_FAIL(do_generate_locality_task_from_encryption_logonly_replica(
              replica_stat_desc,
              *replica,
              i))) {
        LOG_WARN("fail to generate locality task from logonly replica", KR(ret));
      }
    } else if (REPLICA_TYPE_READONLY == replica->get_replica_type()) {
      if (OB_FAIL(do_generate_locality_task_from_readonly_replica(
              replica_stat_desc,
              *replica,
              i))) {
        LOG_WARN("fail to generate locality task from readonly replica", KR(ret));
      }
    } else if (REPLICA_TYPE_COLUMNSTORE == replica->get_replica_type()) {
      if (OB_FAIL(do_generate_locality_task_from_columnstore_replica(
              replica_stat_desc,
              *replica,
              i))) {
        LOG_WARN("fail to generate locality task from columnstore replica", KR(ret));
      }
    } else {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("replica type unexpected", KR(ret), KPC(replica));
    }
  }
  // step1: generate task from locality task
  if (OB_SUCC(ret)) {
    if (replica_stat_map_.count() <= 0) {
      if (OB_FAIL(try_generate_locality_task_from_locality_map())) {
        LOG_WARN("fail to do generate locality task from locality map", KR(ret));
      }
    } else {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("unexpected replica stat map", KR(ret));
      print_locality_information();
    }
  }
  // step2: generate modify paxos replica number task
  if (OB_SUCC(ret)) {
    if (OB_FAIL(try_generate_locality_task_from_paxos_replica_number())) {
      LOG_WARN("fail to generate task from paxos_replica_number", KR(ret));
    }
  }
  return ret;
}

int ObDRWorker::LocalityAlignment::generate_locality_task()
{
  int ret = OB_SUCCESS;
  if (OB_FAIL(prepare_generate_locality_task())) {
    LOG_WARN("fail to prepare generate locality task", KR(ret));
  } else if (OB_FAIL(do_generate_locality_task())) {
    LOG_WARN("fail to do generate locality task", KR(ret));
  }
  return ret;
}

int ObDRWorker::LocalityAlignment::generate_remove_replica_task(
    ReplicaStatDesc &replica_stat_desc)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!replica_stat_desc.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(replica_stat_desc));
  } else {
    void *raw_ptr = nullptr;
    RemoveReplicaLATask *task = nullptr;
    ObLSReplica *replica = replica_stat_desc.replica_;
    if (OB_ISNULL(replica)) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("replica ptr is null", KR(ret), KP(replica));
    } else if (nullptr == (raw_ptr = allocator_.alloc(sizeof(RemoveReplicaLATask)))) {
      ret = OB_ALLOCATE_MEMORY_FAILED;
      LOG_WARN("fail to alloc memory", KR(ret));
    } else if (nullptr == (task = new (raw_ptr) RemoveReplicaLATask())) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("construct task failed", KR(ret));
    } else {
      task->remove_server_ = replica->get_server();
      task->replica_type_ = replica->get_replica_type();
      task->memstore_percent_ = replica->get_memstore_percent();
      task->member_time_us_ = replica->get_member_time_us();
      task->orig_paxos_replica_number_ = replica->get_paxos_replica_number();
      task->paxos_replica_number_ = replica->get_paxos_replica_number();
      if (OB_FAIL(task_array_.push_back(task))) {
        LOG_WARN("fail to push back", KR(ret), KPC(task));
      } else {
        LOG_INFO("success to push a remove replica task to task_array", KR(ret), KPC(task));
      }
    }
    // btw: no need to free memory when failed for arena, just destruct
    if (OB_FAIL(ret) && OB_NOT_NULL(task)) {
      task->~RemoveReplicaLATask();
    }
  }
  return ret;
}

int ObDRWorker::LocalityAlignment::generate_type_transform_task(
    ReplicaStatDesc &replica_stat_desc,
    const ObReplicaType dst_replica_type,
    const int64_t dst_memstore_percent)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!replica_stat_desc.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(replica_stat_desc));
  } else {
    void *raw_ptr = nullptr;
    TypeTransformLATask *task = nullptr;
    share::ObLSReplica *replica = replica_stat_desc.replica_;
    DRUnitStatInfo *unit_stat_info = replica_stat_desc.unit_stat_info_;
    if (OB_UNLIKELY(nullptr == replica || nullptr == unit_stat_info)) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("replica ptr is null", KR(ret));
    } else if (nullptr == (raw_ptr = allocator_.alloc(sizeof(TypeTransformLATask)))) {
      ret = OB_ALLOCATE_MEMORY_FAILED;
      LOG_WARN("fail to alloc memory", KR(ret));
    } else if (nullptr == (task = new (raw_ptr) TypeTransformLATask())) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("construct task failed", KR(ret));
    } else {
      task->zone_ = replica->get_zone();
      task->dst_server_ = replica->get_server();
      task->unit_id_ = unit_stat_info->get_unit().unit_id_;
      task->unit_group_id_ = unit_stat_info->get_unit().unit_group_id_;
      task->src_replica_type_ = replica->get_replica_type();
      task->src_memstore_percent_ = replica->get_memstore_percent();
      task->src_member_time_us_ = replica->get_member_time_us();
      task->dst_replica_type_ = dst_replica_type;
      task->dst_memstore_percent_ = dst_memstore_percent;
      task->dst_member_time_us_ = ObTimeUtility::current_time();
      if (OB_FAIL(task_array_.push_back(task))) {
        LOG_WARN("fail to push back", KR(ret));
      } else {
        LOG_INFO("success to push a type transform task to task_array", KR(ret), KPC(task));
      }
    }
    // btw: no need to free memory when failed for arena, just destruct
    if (OB_FAIL(ret) && nullptr != task) {
      task->~TypeTransformLATask();
    }
  }
  return ret;
}

int ObDRWorker::LocalityAlignment::generate_add_replica_task(
    const common::ObZone &zone,
    ReplicaDesc &replica_desc)
{
  int ret = OB_SUCCESS;
  for (int64_t i = 0; OB_SUCC(ret) && i < replica_desc.replica_num_; ++i) {
    void *raw_ptr = nullptr;
    AddReplicaLATask *task = nullptr;
    if (nullptr == (raw_ptr = allocator_.alloc(sizeof(AddReplicaLATask)))) {
      ret = OB_ALLOCATE_MEMORY_FAILED;
      LOG_WARN("fail to alloc memory", KR(ret));
    } else if (nullptr == (task = new (raw_ptr) AddReplicaLATask())) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("construct task failed", KR(ret));
    } else {
      task->zone_ = zone;
      task->replica_type_ = replica_desc.replica_type_;
      task->memstore_percent_ = replica_desc.memstore_percent_;
      task->member_time_us_ = ObTimeUtility::current_time();
      if (OB_FAIL(task_array_.push_back(task))) {
        LOG_WARN("fail to push back", KR(ret));
      } else {
        LOG_INFO("success to push a add replica task to task_array", KR(ret), KPC(task));
      }
    }
    LOG_INFO("finish generate task from locality", KPC(task), K(replica_desc));
    // btw: no need to free memory when failed for arena, just destruct
    if (OB_FAIL(ret) && nullptr != task) {
      task->~AddReplicaLATask();
    }
  }
  return ret;
}

int ObDRWorker::LocalityAlignment::generate_modify_paxos_replica_number_task()
{
  int ret = OB_SUCCESS;

  void *raw_ptr = nullptr;
  ModifyPaxosReplicaNumberLATask *task = nullptr;
  if (nullptr == (raw_ptr = allocator_.alloc(sizeof(ModifyPaxosReplicaNumberLATask)))) {
    ret = OB_ALLOCATE_MEMORY_FAILED;
    LOG_WARN("fail to alloc memory", KR(ret));
  } else if (nullptr == (task = new (raw_ptr) ModifyPaxosReplicaNumberLATask())) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("construct task failed", KR(ret));
  } else {
    if (OB_FAIL(task_array_.push_back(task))) {
      LOG_WARN("fail to push back", KR(ret));
    } else {
        LOG_INFO("success to push a modify paxos replica number task to task_array", KR(ret), KPC(task));
      }
  }
  // btw: no need to free memory when failed for arena, just destruct
  if (OB_FAIL(ret) && nullptr != task) {
    task->~ModifyPaxosReplicaNumberLATask();
  }
  return ret;
}

int ObDRWorker::LocalityAlignment::build()
{
  int ret = OB_SUCCESS;
  uint64_t tenant_id = OB_INVALID_ID;
  if (OB_FAIL(locality_map_.create(LOCALITY_MAP_BUCKET_NUM, "LocAlign"))) {
    LOG_WARN("fail to create locality map", KR(ret));
  } else if (OB_FAIL(generate_paxos_replica_number())) {
    LOG_WARN("fail to generate paxos_replica_number", KR(ret));
  } else if (OB_FAIL(build_locality_stat_map())) {
    LOG_WARN("fail to build locality map", KR(ret));
  } else if (OB_FAIL(build_replica_stat_map())) {
    LOG_WARN("fail to build replica map", KR(ret));
  } else if (OB_FAIL(generate_locality_task())) {
    LOG_WARN("fail to generate locality task", KR(ret));
  } else if (OB_FAIL(dr_ls_info_.get_tenant_id(tenant_id))) {
    LOG_WARN("fail to get tenant id", KR(ret), K(tenant_id));
  } else if (OB_FAIL(unit_provider_.init(gen_user_tenant_id(tenant_id),
          dr_ls_info_))) {
    LOG_WARN("fail to init unit provider", KR(ret), K(tenant_id), K_(dr_ls_info));
  }
  return ret;
}

int ObDRWorker::LocalityAlignment::try_review_remove_replica_task(
    UnitProvider &unit_provider,
    LATask *this_task,
    const LATask *&output_task,
    bool &found)
{
  int ret = OB_SUCCESS;
  UNUSED(unit_provider);
  RemoveReplicaLATask *my_task = reinterpret_cast<RemoveReplicaLATask *>(this_task);
  if (OB_ISNULL(this_task) || OB_ISNULL(my_task)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), KP(this_task), KP(my_task));
  } else if (REPLICA_TYPE_FULL != my_task->replica_type_) {
    // no need to check when remove non-paxos replica
    my_task->orig_paxos_replica_number_ = curr_paxos_replica_number_;
    my_task->paxos_replica_number_ = curr_paxos_replica_number_;
    output_task = my_task;
    found = true;
  } else {
    found = false;
    int64_t new_paxos_replica_number = 0;
    if (OB_FAIL(ObDRWorker::generate_disaster_recovery_paxos_replica_number(
            dr_ls_info_,
            curr_paxos_replica_number_,
            locality_paxos_replica_number_,
            MEMBER_CHANGE_SUB,
            new_paxos_replica_number,
            found))) {
      LOG_WARN("fail to generate disaster recovery paxos_replica_number", KR(ret), K(found));
    } else if (!found) {
      // bypass
    } else {
      my_task->orig_paxos_replica_number_ = curr_paxos_replica_number_;
      my_task->paxos_replica_number_ = new_paxos_replica_number;
      output_task = my_task;
      found = true;
    }
  }
  return ret;
}

int ObDRWorker::LocalityAlignment::try_review_add_replica_task(
    UnitProvider &unit_provider,
    LATask *this_task,
    const LATask *&output_task,
    bool &found)
{
  int ret = OB_SUCCESS;
  const share::ObLSStatusInfo *ls_status_info = nullptr;
  AddReplicaLATask *my_task = reinterpret_cast<AddReplicaLATask *>(this_task);
  if (OB_UNLIKELY(nullptr == this_task || nullptr == my_task)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), KP(this_task), KP(my_task));
  } else if (OB_FAIL(dr_ls_info_.get_ls_status_info(ls_status_info))) {
    LOG_WARN("fail to get log stream status info", KR(ret));
  } else if (OB_UNLIKELY(nullptr == ls_status_info)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("ls status info ptr is null", KR(ret), KP(ls_status_info));
  } else {
    found = false;
    share::ObUnit unit;
    const common::ObZone &zone = my_task->zone_;
    int tmp_ret = unit_provider.allocate_unit(zone, ls_status_info->unit_group_id_, unit);
    if (OB_ITER_END == tmp_ret) {
      // bypass
    } else if (OB_SUCCESS == tmp_ret) {
      my_task->dst_server_ = unit.server_;
      my_task->unit_id_ = unit.unit_id_;
      my_task->unit_group_id_ = ls_status_info->unit_group_id_;
      if (ObReplicaTypeCheck::is_paxos_replica_V2(my_task->replica_type_)) {
        int64_t new_paxos_replica_number = 0;
        if (OB_FAIL(ObDRWorker::generate_disaster_recovery_paxos_replica_number(
                dr_ls_info_,
                curr_paxos_replica_number_,
                locality_paxos_replica_number_,
                MEMBER_CHANGE_ADD,
                new_paxos_replica_number,
                found))) {
          LOG_WARN("fail to generate disaster recovery paxos_replica_number", KR(ret), K(found));
        } else if (!found) {
          // bypass
        } else {
          my_task->orig_paxos_replica_number_ = curr_paxos_replica_number_;
          my_task->paxos_replica_number_ = new_paxos_replica_number;
          output_task = my_task;
          found = true;
        }
      } else {
        my_task->orig_paxos_replica_number_ = curr_paxos_replica_number_;
        my_task->paxos_replica_number_ = curr_paxos_replica_number_;
        output_task = my_task;
        found = true;
      }
    } else {
      ret = tmp_ret;
      LOG_WARN("fail to get unit", KR(ret), K(zone));
    }
  }
  return ret;
}

int ObDRWorker::LocalityAlignment::try_review_type_transform_task(
    UnitProvider &unit_provider,
    LATask *this_task,
    const LATask *&output_task,
    bool &found)
{
  int ret = OB_SUCCESS;
  UNUSED(unit_provider);
  TypeTransformLATask *my_task = reinterpret_cast<TypeTransformLATask *>(this_task);
  found = false;
  if (OB_UNLIKELY(nullptr == this_task || nullptr == my_task)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), KP(this_task), KP(my_task));
  } else if (ObReplicaTypeCheck::is_paxos_replica_V2(my_task->src_replica_type_)
             && ObReplicaTypeCheck::is_paxos_replica_V2(my_task->dst_replica_type_)) {
    my_task->orig_paxos_replica_number_ = curr_paxos_replica_number_;
    my_task->paxos_replica_number_ = curr_paxos_replica_number_;
    output_task = my_task;
    found = true;
  } else if (ObReplicaTypeCheck::is_paxos_replica_V2(my_task->src_replica_type_)
             && !ObReplicaTypeCheck::is_paxos_replica_V2(my_task->dst_replica_type_)) {
    int64_t new_paxos_replica_number = 0;
    if (OB_FAIL(ObDRWorker::generate_disaster_recovery_paxos_replica_number(
            dr_ls_info_,
            curr_paxos_replica_number_,
            locality_paxos_replica_number_,
            MEMBER_CHANGE_SUB,
            new_paxos_replica_number,
            found))) {
      LOG_WARN("fail to generate disaster recovery paxos_replica_number", KR(ret), K(found));
    } else if (!found) {
      // bypass
    } else {
      my_task->orig_paxos_replica_number_ = curr_paxos_replica_number_;
      my_task->paxos_replica_number_ = new_paxos_replica_number;
      output_task = my_task;
      found = true;
    }
  } else if (!ObReplicaTypeCheck::is_paxos_replica_V2(my_task->src_replica_type_)
             && ObReplicaTypeCheck::is_paxos_replica_V2(my_task->dst_replica_type_)) {
    int64_t new_paxos_replica_number = 0;
    if (OB_FAIL(ObDRWorker::generate_disaster_recovery_paxos_replica_number(
            dr_ls_info_,
            curr_paxos_replica_number_,
            locality_paxos_replica_number_,
            MEMBER_CHANGE_ADD,
            new_paxos_replica_number,
            found))) {
      LOG_WARN("fail to generate disaster recovery paxos_replica_number", KR(ret), K(found));
    } else if (!found) {
      // bypass
    } else {
      my_task->orig_paxos_replica_number_ = curr_paxos_replica_number_;
      my_task->paxos_replica_number_ = new_paxos_replica_number;
      output_task = my_task;
      found = true;
    }
  } else {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("invalid type transform task", KR(ret), KPC(this_task));
  }
  return ret;
}

int ObDRWorker::LocalityAlignment::try_review_modify_paxos_replica_number_task(
    UnitProvider &unit_provider,
    LATask *this_task,
    const LATask *&output_task,
    bool &found)
{
  int ret = OB_SUCCESS;
  UNUSED(unit_provider);
  ModifyPaxosReplicaNumberLATask *my_task = reinterpret_cast<ModifyPaxosReplicaNumberLATask *>(this_task);
  if (OB_UNLIKELY(nullptr == this_task || nullptr == my_task)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), KP(this_task), KP(my_task));
  } else {
    int64_t new_paxos_replica_number = 0;
    if (OB_FAIL(ObDRWorker::generate_disaster_recovery_paxos_replica_number(
            dr_ls_info_,
            curr_paxos_replica_number_,
            locality_paxos_replica_number_,
            MEMBER_CHANGE_NOP,
            new_paxos_replica_number,
            found))) {
      LOG_WARN("fail to generate disaster recovery paxos_replica_number", KR(ret), K(found));
    } else if (!found) {
      // bypass
    } else {
      my_task->orig_paxos_replica_number_ = curr_paxos_replica_number_;
      my_task->paxos_replica_number_ = new_paxos_replica_number;
      output_task = my_task;
      found = true;
    }
  }
  return ret;
}

int ObDRWorker::LocalityAlignment::try_get_normal_locality_alignment_task(
    UnitProvider &unit_provider,
    const LATask *&task)
{
  int ret = OB_SUCCESS;
  task = nullptr;
  bool found = false;
  int64_t index = 0;
  for (index = task_idx_; !found && OB_SUCC(ret) && index < task_array_.count(); ++index) {
    LATask *this_task = task_array_.at(index);
    if (OB_ISNULL(this_task)) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("this task ptr is null", KR(ret));
    } else {
      switch (this_task->get_task_type()) {
      case ObDRTaskType::LS_REMOVE_PAXOS_REPLICA:
      case ObDRTaskType::LS_REMOVE_NON_PAXOS_REPLICA:
        if (OB_FAIL(try_review_remove_replica_task(
                unit_provider,
                this_task,
                task,
                found))) {
          LOG_WARN("fail to try review remove replica task", KR(ret), KPC(this_task), K(found));
        } else {
          LOG_INFO("success to try review remove replica task", KR(ret), KPC(this_task), K(found));
        }
        break;
      case ObDRTaskType::LS_ADD_REPLICA:
        if (OB_FAIL(try_review_add_replica_task(
                unit_provider,
                this_task,
                task,
                found))) {
          LOG_WARN("fail to try review add replica task", KR(ret), KPC(this_task), K(found));
        } else {
          LOG_INFO("success to try review add replica task", KR(ret), KPC(this_task), K(found));
        }
        break;
      case ObDRTaskType::LS_TYPE_TRANSFORM:
        if (OB_FAIL(try_review_type_transform_task(
                unit_provider,
                this_task,
                task,
                found))) {
          LOG_WARN("fail to try review type transform task", KR(ret), KPC(this_task), K(found));
        } else {
          LOG_INFO("success to try review type transform task", KR(ret), KPC(this_task), K(found));
        }
        break;
      case ObDRTaskType::LS_MODIFY_PAXOS_REPLICA_NUMBER:
        if (OB_FAIL(try_review_modify_paxos_replica_number_task(
                unit_provider,
                this_task,
                task,
                found))) {
          LOG_WARN("fail to try review modify paxos replica number task", KR(ret), KPC(this_task), K(found));
        } else {
          LOG_INFO("success to try review modify paxos replica number task", KR(ret), KPC(this_task), K(found));
        }
        break;
      default:
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("task type unexpected", KR(ret), KPC(this_task));
        break;
      }
    }
  }
  if (OB_SUCC(ret)) {
    task_idx_ = index;
  }
  return ret;
}

int ObDRWorker::LocalityAlignment::try_get_readonly_or_columnstore_all_server_locality_alignment_task(
    UnitProvider &unit_provider,
    const LATask *&task)
{
  int ret = OB_SUCCESS;
  task = nullptr;
  for (LocalityMap::iterator iter = locality_map_.begin();
       OB_SUCC(ret) && iter != locality_map_.end();
       ++iter) {
    const common::ObZone &zone = iter->first;
    ReplicaDescArray *replica_desc_array = iter->second;
    const share::ObLSStatusInfo *ls_status_info = nullptr;
    if (OB_UNLIKELY(nullptr == replica_desc_array)) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("replica desc array ptr is null", KR(ret));
    } else if (!replica_desc_array->is_readonly_all_server()
               && !replica_desc_array->is_columnstore_all_server()) {
      // bypass
    } else if (OB_FAIL(dr_ls_info_.get_ls_status_info(ls_status_info))) {
      LOG_WARN("fail to get log stream info", KR(ret));
    } else if (OB_UNLIKELY(nullptr == ls_status_info)) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("ls status info ptr is null", KR(ret), KP(ls_status_info));
    } else {
      share::ObUnit unit;
      uint64_t unit_group_id = ls_status_info->unit_group_id_;
      int tmp_ret = unit_provider.allocate_unit(zone, unit_group_id, unit);
      // Although duplicate ls has specified unit_group_id,
      // R/C replicas still need to be added on all tenant units of the zone.
      if (OB_SUCC(ret)
          && OB_ITER_END == tmp_ret
          && ls_status_info->is_duplicate_ls()
          && unit_group_id > 0) {
        unit_group_id = 0;
        tmp_ret = unit_provider.allocate_unit(zone, unit_group_id, unit);
      }
      if (OB_ITER_END == tmp_ret) {
        // bypass
      } else if (OB_SUCCESS == tmp_ret) {
        add_replica_task_.zone_ = zone;
        add_replica_task_.dst_server_ = unit.server_;
        add_replica_task_.member_time_us_ = ObTimeUtility::current_time();
        add_replica_task_.unit_id_ = unit.unit_id_;
        add_replica_task_.unit_group_id_ = unit_group_id;
        add_replica_task_.replica_type_ = replica_desc_array->is_columnstore_all_server() ?
                                          REPLICA_TYPE_COLUMNSTORE : REPLICA_TYPE_READONLY;
        add_replica_task_.memstore_percent_ = replica_desc_array->get_readonly_memstore_percent();
        add_replica_task_.orig_paxos_replica_number_ = curr_paxos_replica_number_;
        add_replica_task_.paxos_replica_number_ = curr_paxos_replica_number_;
        task = &add_replica_task_;
        break;
      } else {
        ret = tmp_ret;
        LOG_WARN("fail to get unit", KR(ret), K(zone));
      }
    }
  }
  return ret;
}

int ObDRWorker::LocalityAlignment::get_next_locality_alignment_task(
    const LATask *&task)
{
  int ret = OB_SUCCESS;
  LATaskCmp cmp_operator(task_array_);
  if (OB_FAIL(cmp_operator.execute_sort())) {
    LOG_WARN("fail to sort task", KR(ret));
  } else {
    if (OB_FAIL(try_get_normal_locality_alignment_task(
            unit_provider_,
            task))) {
      LOG_WARN("fail to get normal locality alignment task", KR(ret));
    } else if (nullptr != task) {
      // got one
      LOG_INFO("success to get a normal task", KPC(task));
    } else if (OB_FAIL(try_get_readonly_or_columnstore_all_server_locality_alignment_task(
            unit_provider_,
            task))) {
      LOG_WARN("fail to get readonly all server locality alignment task", KR(ret));
    } else if (nullptr == task) {
      ret = OB_ITER_END;
    } else {
      LOG_INFO("success to get a readonly all server task", KPC(task));
    }
  }
  return ret;
}

int ObDRWorker::UnitProvider::init(
    const uint64_t tenant_id,
    DRLSInfo &dr_ls_info)
{
  int ret = OB_SUCCESS;
  int64_t replica_cnt = 0;
  if (OB_UNLIKELY(inited_)) {
    ret = OB_INIT_TWICE;
    LOG_WARN("init twice", KR(ret));
  } else if (OB_UNLIKELY(OB_INVALID_ID == tenant_id)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret),
             K(tenant_id));
  } else if (OB_FAIL(dr_ls_info.get_replica_cnt(replica_cnt))) {
    LOG_WARN("failed to get replica count", KR(ret));
  } else if (OB_ISNULL(GCTX.sql_proxy_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("sql_proxy_ is null", KR(ret), KP(GCTX.sql_proxy_));
  } else if (OB_FAIL(unit_operator_.init(*GCTX.sql_proxy_))) {
    LOG_WARN("init unit table operator failed", KR(ret));
  } else {
    const int64_t hash_count = max(replica_cnt, 1);
    if (OB_FAIL(unit_set_.create(hash::cal_next_prime(hash_count)))) {
      LOG_WARN("failed to create unit set", KR(ret), K(replica_cnt), K(hash_count));
    } else if (OB_FAIL(init_unit_set(dr_ls_info))) {
      LOG_WARN("failed to init unit set", KR(ret), K(dr_ls_info));
    } else {
      tenant_id_ = tenant_id;
      inited_ = true;
    }
  }
  return ret;
}

int ObDRWorker::UnitProvider::init_unit_set(
     DRLSInfo &dr_ls_info)
{
  int ret = OB_SUCCESS;
  int64_t replica_cnt = 0;
  if (OB_UNLIKELY(inited_)) {
    ret = OB_INIT_TWICE;
    LOG_WARN("init twice", KR(ret));
  } else if (OB_FAIL(dr_ls_info.get_replica_cnt(replica_cnt))) {
    LOG_WARN("fail to get replica cnt", KR(ret));
  } else {
    for (int64_t index = 0; OB_SUCC(ret) && index < replica_cnt; ++index) {
      share::ObLSReplica *ls_replica = nullptr;
      DRServerStatInfo *server_stat_info = nullptr;
      DRUnitStatInfo *unit_stat_info = nullptr;
      DRUnitStatInfo *unit_in_group_stat_info = nullptr;
      if (OB_FAIL(dr_ls_info.get_replica_stat(
              index,
              ls_replica,
              server_stat_info,
              unit_stat_info,
              unit_in_group_stat_info))) {
        LOG_WARN("fail to get replica stat", KR(ret));
      } else if (OB_UNLIKELY(nullptr == ls_replica
                             || nullptr == server_stat_info
                             || nullptr == unit_stat_info
                             || nullptr == unit_in_group_stat_info)) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("replica stat unexpected", KR(ret),
                 KP(ls_replica),
                 KP(server_stat_info),
                 KP(unit_stat_info),
                 KP(unit_in_group_stat_info));
      } else if ((ObReplicaTypeCheck::is_paxos_replica_V2(ls_replica->get_replica_type())
                  && ls_replica->get_in_member_list())
          || (!ObReplicaTypeCheck::is_paxos_replica_V2(ls_replica->get_replica_type()))) {
        if (OB_FAIL(unit_set_.set_refactored(unit_stat_info->get_unit().unit_id_))) {
          LOG_WARN("fail to set refactored", KR(ret));
        }
      }
    }
  }
  return ret;
}

int ObDRWorker::UnitProvider::inner_get_valid_unit_(
    const common::ObZone &zone,
    const common::ObArray<share::ObUnit> &unit_array,
    share::ObUnit &output_unit,
    const bool &force_get,
    bool &found)
{
  int ret = OB_SUCCESS;
  output_unit.reset();
  found = false;
  if (OB_UNLIKELY(!inited_)) {
    ret = OB_NOT_INIT;
    LOG_WARN("not init", KR(ret));
  } else if (OB_UNLIKELY(0 >= unit_array.count())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("unit mgr ptr is null", KR(ret), "unit_count", unit_array.count());
  } else {
    bool server_is_active = false;
    for (int64_t i = 0; OB_SUCC(ret) && i < unit_array.count(); ++i) {
      server_is_active = false;
      const share::ObUnit &unit = unit_array.at(i);
      const uint64_t unit_id = unit.unit_id_;
      int hash_ret = OB_SUCCESS;
      bool server_and_unit_status_is_valid = true;
      if (unit.zone_ != zone) {
        // bypass, because we do not support operation between different zones
      } else {
        if (!force_get) {
          if (OB_FAIL(SVR_TRACER.check_server_active(unit.server_, server_is_active))) {
            LOG_WARN("fail to check server active", KR(ret), "server", unit.server_);
          } else if (!server_is_active) {
            server_and_unit_status_is_valid = false;
            FLOG_INFO("server is not active", "server", unit.server_, K(server_is_active));
          } else if (!unit.is_active_status()) {
            server_and_unit_status_is_valid = false;
            FLOG_INFO("unit status is not normal", K(unit));
          } else {
            server_and_unit_status_is_valid = true;
          }
        }

        if (OB_FAIL(ret) || !server_and_unit_status_is_valid) {
        } else if (OB_HASH_EXIST == (hash_ret = unit_set_.set_refactored(unit_id, 0))) {
          FLOG_INFO("unit existed", K(unit_id));
        } else if (OB_FAIL(hash_ret)) {
          LOG_WARN("set refactored failed", KR(ret), KR(hash_ret));
        } else if (OB_FAIL(output_unit.assign(unit))) {
          LOG_WARN("fail to assign unit info", KR(ret), K(unit));
        } else {
          found = true;
          break;
        }
      }
    }
  }
  return ret;
}

int ObDRWorker::UnitProvider::allocate_unit(
    const common::ObZone &zone,
    const uint64_t unit_group_id,
    share::ObUnit &unit)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!inited_)) {
    ret = OB_NOT_INIT;
    LOG_WARN("not init", KR(ret));
  } else {
    common::ObArray<ObUnit> unit_array;
    bool found = false;
    bool force_get = true; // if unit_group_id is given, just allocate unit belongs to this unit group
    // 1. if unit_group_id is valid, try get valid unit in this unit group
    if (unit_group_id > 0) {
      force_get = true;
      if (OB_FAIL(unit_operator_.get_units_by_unit_group_id(unit_group_id, unit_array))) {
        LOG_WARN("fail to get unit group", KR(ret), K(unit_group_id));
      } else if (OB_FAIL(inner_get_valid_unit_(zone, unit_array, unit, force_get, found))) {
        LOG_WARN("fail to get valid unit from certain unit group", KR(ret), K(zone), K(unit_array), K(force_get));
      }
    } else {
      // 2. if unit_group_id = 0, try get from all units
      unit_array.reset();
      force_get = false;
      if (OB_FAIL(unit_operator_.get_units_by_tenant(tenant_id_, unit_array))) {
        LOG_WARN("fail to get ll unit infos by tenant", KR(ret), K(tenant_id_));
      } else if (OB_FAIL(inner_get_valid_unit_(zone, unit_array, unit, force_get, found))) {
        LOG_WARN("fail to get valid unit from all units in tenant", KR(ret), K(zone), K(unit_array), K(force_get));
      }
    }
    if (OB_SUCC(ret) && !found) {
      ret = OB_ITER_END;
      LOG_WARN("fail to get valid unit", KR(ret), K(zone), K(found));
    }
  }
  return ret;
}

bool ObDRWorker::LATaskCmp::operator()(const LATask *left, const LATask *right)
{
  bool bool_ret = false;
  if (OB_SUCCESS != ret_) {
    // bypass
  } else if (nullptr == left || nullptr == right) {
    ret_ = OB_ERR_UNEXPECTED;
  } else {
    bool_ret = left->get_task_priority() < right->get_task_priority();
  }
  return bool_ret;
}

int ObDRWorker::LATaskCmp::execute_sort()
{
  lib::ob_sort(task_array_.begin(), task_array_.end(), *this);
  return ret_;
}

#define FREE_DISASTER_RECOVERY_WORKER_TASK_MEMORY                             \
  for (int64_t i = 0; i < dr_tasks_.count(); ++i) {                           \
    ObDRTask *task = dr_tasks_.at(i);                                         \
    if (OB_NOT_NULL(task)) {                                                  \
      task->~ObDRTask();                                                      \
    }                                                                         \
  }                                                                           \
  task_alloc_.reset();                                                        \
  dr_tasks_.reset();                                                          \

#define COMMIT_DISASTER_RECOVERY_WORKER_TRANS                                \
  if (trans.is_started()) {                                                  \
    int tmp_ret = OB_SUCCESS;                                                \
    if (OB_SUCCESS != (tmp_ret = trans.end(OB_SUCC(ret)))) {                 \
      LOG_WARN("failed to commit trans", KR(ret), KR(tmp_ret));              \
      ret = OB_SUCC(ret) ? tmp_ret : ret;                                    \
    }                                                                        \
  }                                                                          \

ObDRWorker::ObDRWorker(const int64_t service_epoch, const uint64_t tenant_id)
  : service_epoch_(service_epoch),
    tenant_id_(tenant_id),
    task_count_statistic_(),
    display_tasks_(),
    display_tasks_rwlock_(ObLatchIds::DISPLAY_TASKS_LOCK),
    dr_tasks_(),
    task_alloc_("ObDRWorker", OB_MALLOC_NORMAL_BLOCK_SIZE, MTL_ID()),
    table_operator_()
{
}

ObDRWorker::~ObDRWorker()
{
  FREE_DISASTER_RECOVERY_WORKER_TASK_MEMORY
}

int ObDRWorker::check_tenant_locality_match(
    const uint64_t tenant_id,
    bool &locality_is_matched)
{
  int ret = OB_SUCCESS;
  locality_is_matched = true;
  if (OB_UNLIKELY(OB_INVALID_ID == tenant_id)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(tenant_id));
  } else if (OB_UNLIKELY(nullptr == GCTX.lst_operator_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("lst operator ptr is null", KR(ret));
  } else {
    LOG_INFO("start try check tenant locality match", K(tenant_id));
    share::ObLSStatusOperator ls_status_operator;
    common::ObArray<share::ObLSStatusInfo> ls_status_info_array;
    if (OB_FAIL(ls_status_operator.get_all_ls_status_by_order(tenant_id, ls_status_info_array, *GCTX.sql_proxy_))) {
      LOG_WARN("fail to get all ls status", KR(ret), K(tenant_id));
    } else if (ls_status_info_array.count() <= 0) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("ls_status_info_array has no member", KR(ret), K(ls_status_info_array));
    } else {
      for (int64_t i = 0; OB_SUCC(ret) && i < ls_status_info_array.count() && locality_is_matched; ++i) {
        share::ObLSInfo ls_info;
        share::ObLSStatusInfo &ls_status_info = ls_status_info_array.at(i);
        bool filter_readonly_replicas_with_flag = true;
        DRLSInfo dr_ls_info(gen_user_tenant_id(tenant_id), GCTX.schema_service_);
        if (ls_status_info.ls_is_creating()) {
          locality_is_matched = false;
        } else if (OB_FAIL(GCTX.lst_operator_->get(
                GCONF.cluster_id,
                ls_status_info.tenant_id_,
                ls_status_info.ls_id_,
                share::ObLSTable::COMPOSITE_MODE,
                ls_info))) {
          LOG_WARN("fail to get log stream info", KR(ret));
        } else if (OB_FAIL(dr_ls_info.init())) {
          LOG_WARN("fail to init dr log stream info", KR(ret));
        } else if (OB_FAIL(dr_ls_info.build_disaster_ls_info(
                               ls_info,
                               ls_status_info,
                               filter_readonly_replicas_with_flag))) {
          LOG_WARN("fail to generate dr log stream info", KR(ret), K(ls_info),
                   K(ls_status_info), K(filter_readonly_replicas_with_flag));
        } else if (OB_FAIL(check_ls_locality_match_(
                dr_ls_info, locality_is_matched))) {
          LOG_WARN("fail to try log stream disaster recovery", KR(ret));
        }
      }
    }
  }
  return ret;
}

int ObDRWorker::check_ls_locality_match_(
    DRLSInfo &dr_ls_info,
    bool &locality_is_matched)
{
  int ret = OB_SUCCESS;
  int tmp_ret = OB_SUCCESS;
  locality_is_matched = false;
  LOG_INFO("start to check ls locality match", K(dr_ls_info));
  LocalityAlignment locality_alignment(dr_ls_info, OB_SERVER_TENANT_ID);
  if (!dr_ls_info.has_leader()) {
    LOG_WARN("has no leader, maybe not report yet",
             KR(ret), K(dr_ls_info));
  } else if (dr_ls_info.get_paxos_replica_number() <= 0) {
    LOG_WARN("paxos_replica_number is invalid, maybe not report yet",
             KR(ret), K(dr_ls_info));
  } else if (OB_FAIL(locality_alignment.build())) {
    LOG_WARN("fail to build locality alignment", KR(ret));
  } else if (0 != locality_alignment.get_task_array_cnt()) {
    locality_is_matched = false;
  } else {
    // for dup-ls, add C/R replica task is not in task_array, also need to try getting next LA task
    const LATask *task = nullptr;
    if (OB_TMP_FAIL(locality_alignment.get_next_locality_alignment_task(task))) {
      if (OB_ITER_END == tmp_ret) {
        locality_is_matched = true;
      } else {
        LOG_WARN("fail to get next locality alignment task", KR(tmp_ret));
      }
    } else {
      locality_is_matched = false;
    }
  }
  ObTaskController::get().allow_next_syslog();
  LOG_INFO("the locality matched check for this logstream", KR(ret), K(locality_is_matched),
           K(dr_ls_info), "task_cnt", locality_alignment.get_task_array_cnt());
  return ret;
}

int ObDRWorker::check_whether_the_tenant_role_can_exec_dr_(const uint64_t tenant_id)
{
  int ret = OB_SUCCESS;
  ObAllTenantInfo tenant_info;
  if (OB_UNLIKELY(OB_INVALID_ID == tenant_id)
             || OB_ISNULL(GCTX.sql_proxy_)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(tenant_id));
  } else if (is_sys_tenant(tenant_id) || is_meta_tenant(tenant_id)) {
    // good, sys tenant and meta tenant can not in clone procedure
  } else if (OB_FAIL(ObAllTenantInfoProxy::load_tenant_info(
                         tenant_id,
                         GCTX.sql_proxy_,
                         false,
                         tenant_info))) {
    LOG_WARN("fail to load tenant info", KR(ret), K(tenant_id));
  } else if (tenant_info.is_clone()) {
    ret = OB_STATE_NOT_MATCH;
    LOG_INFO("the tenant is currently in the clone processing and disaster recovery will"
             " not be executed for the time being", KR(ret), K(tenant_id));
  } else {
    // good, can do dr-task
  }
  return ret;
}

ERRSIM_POINT_DEF(ERRSIM_SKIP_CHECK_TENANT_COMMON_DR_TASK)
int ObDRWorker::try_tenant_common_dr_task(
    const uint64_t tenant_id,
    const bool only_for_display,
    int64_t &acc_dr_task)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!is_valid_tenant_id(tenant_id))) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(tenant_id));
  } else if (OB_ISNULL(GCTX.sql_proxy_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("lst operator ptr or sql proxy is null", KR(ret), KP(GCTX.sql_proxy_));
  } else if (OB_UNLIKELY(ERRSIM_SKIP_CHECK_TENANT_COMMON_DR_TASK)) {
    LOG_INFO("errsim skip check tenant common dr task");
  } else if (OB_FAIL(check_whether_the_tenant_role_can_exec_dr_(tenant_id))) {
    LOG_INFO("fail to check_whether_the_tenant_role_can_exec_dr_", KR(ret), K(tenant_id));
  } else {
    FLOG_INFO("start try tenant disaster recovery", K(tenant_id), K(only_for_display));
    share::ObLSStatusOperator ls_status_operator;
    common::ObArray<share::ObLSStatusInfo> ls_status_info_array;
    if (OB_FAIL(ls_status_operator.get_all_ls_status_by_order(tenant_id, ls_status_info_array, *GCTX.sql_proxy_))) {
      LOG_WARN("fail to get all ls status", KR(ret), K(tenant_id));
    } else {
      for (int64_t i = 0; OB_SUCC(ret) && i < ls_status_info_array.count(); ++i) {
        int64_t ls_acc_dr_task = 0;
        int tmp_ret = OB_SUCCESS;
        const share::ObLSStatusInfo &ls_status_info = ls_status_info_array.at(i);
        // ignore ret for isolation different ls
        if (OB_TMP_FAIL(try_ls_disaster_recovery(tenant_id, ls_status_info, only_for_display, ls_acc_dr_task))) {
          LOG_WARN("fail to try ls disaster recovery", KR(tmp_ret), K(tenant_id), K(ls_status_info));
        }
        acc_dr_task += ls_acc_dr_task;
      }
    }
  }
  return ret;
}

int ObDRWorker::try_ls_disaster_recovery(
    const uint64_t tenant_id,
    const share::ObLSStatusInfo &ls_status_info,
    const bool only_for_display,
    int64_t &ls_acc_dr_task)
{
  int ret = OB_SUCCESS;
  FREE_DISASTER_RECOVERY_WORKER_TASK_MEMORY
  if (OB_UNLIKELY(!ls_status_info.is_valid()
               || !is_valid_tenant_id(tenant_id))) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(ls_status_info), K(tenant_id));
  } else if (OB_ISNULL(GCTX.sql_proxy_)
          || OB_ISNULL(GCTX.schema_service_)
          || OB_ISNULL(GCTX.lst_operator_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("sql_proxy_ or schema_service_ or lst_operator_ is nullptr",
              KR(ret), KP(GCTX.sql_proxy_), KP(GCTX.schema_service_), KP(GCTX.lst_operator_));
  } else if (ls_status_info.ls_is_creating()) {
    // skip, ls is creating
    LOG_TRACE("ls is creating", K(ls_status_info), K(tenant_id));
  } else {
    common::ObZone dest_zone; // not used
    ObServerInfoInTable source_server; // not used
    palf::LogConfigVersion config_version; // not used
    bool found_replace_task = false;
    share::ObLSInfo ls_info;
    // this structure is used to generate migrtion/locality alignment/shrink unit tasks
    DRLSInfo dr_ls_info_without_flag(gen_user_tenant_id(tenant_id), GCTX.schema_service_);
    // this structure is used to generate permanent offline tasks
    DRLSInfo dr_ls_info_with_flag(gen_user_tenant_id(tenant_id), GCTX.schema_service_);
    LOG_INFO("start try ls disaster recovery", K(ls_status_info));
    if (OB_FAIL(GCTX.lst_operator_->get(GCONF.cluster_id,
                                        ls_status_info.tenant_id_,
                                        ls_status_info.ls_id_,
                                        share::ObLSTable::COMPOSITE_MODE,
                                        ls_info))) {
      LOG_WARN("fail to get log stream info", KR(ret), K(ls_status_info));
    } else if (OB_FAIL(dr_ls_info_without_flag.init())) {
      LOG_WARN("fail to init dr log stream info", KR(ret));
    } else if (OB_FAIL(dr_ls_info_without_flag.build_disaster_ls_info(ls_info, ls_status_info, true/*filter_readonly_replica_with_flag*/))) {
      LOG_WARN("fail to generate dr log stream info", KR(ret), K(ls_info), K(ls_status_info));
    } else if (OB_FAIL(dr_ls_info_with_flag.init())) {
      LOG_WARN("fail to init dr log stream info with flag", KR(ret));
    } else if (OB_FAIL(dr_ls_info_with_flag.build_disaster_ls_info(ls_info, ls_status_info, false/*filter_readonly_replica_with_flag*/))) {
      LOG_WARN("fail to generate dr log stream info with flag", KR(ret), K(ls_info), K(ls_status_info));
    } else if (!only_for_display
            && ObRootUtils::is_dr_replace_deployment_mode_match()
            && OB_FAIL(check_ls_single_replica_dr_tasks(dr_ls_info_with_flag, config_version, source_server, dest_zone, found_replace_task))) {
      LOG_WARN("fail to try ls single replica task", KR(ret), K(dr_ls_info_with_flag), K(only_for_display));
    } else if (found_replace_task) {
      LOG_INFO("LS need generate replace replica task, skip check common task",
               "tenant_id", dr_ls_info_with_flag.get_tenant_id(), "ls_id", dr_ls_info_with_flag.get_ls_id());
    } else if (OB_FAIL(check_ls_common_dr_tasks_(only_for_display,
                                                 dr_ls_info_without_flag,
                                                 dr_ls_info_with_flag,
                                                 ls_acc_dr_task))) {
      LOG_WARN("fail to try log stream disaster recovery", KR(ret), K(only_for_display));
    } else if (only_for_display || is_zero_row(dr_tasks_.count())) {
      // skip
      LOG_INFO("skip persist dr_task", K(only_for_display), "task_count", dr_tasks_.count());
    } else if (OB_FAIL(persist_tasks_into_inner_table_(dr_tasks_, tenant_id, ls_status_info.ls_id_, false/*is_manual*/))) {
      LOG_WARN("fail to do end generated dr tasks", KR(ret), K(tenant_id), K(ls_status_info));
    }
  }
  FREE_DISASTER_RECOVERY_WORKER_TASK_MEMORY
  return ret;
}

int ObDRWorker::check_ls_common_dr_tasks_(
    const bool only_for_display,
    DRLSInfo &dr_ls_info,
    DRLSInfo &dr_ls_info_with_flag,
    int64_t &acc_dr_task_cnt)
{
  int ret = OB_SUCCESS;
  const int64_t start_time = ObTimeUtility::fast_current_time();
  // step0: set acc_dr_task_cnt
  acc_dr_task_cnt = 0;
  // step1: kick out the replica which is in some certain permanent-offline server
  if (OB_SUCC(ret)
      && acc_dr_task_cnt <= 0) {
    if (OB_FAIL(try_remove_permanent_offline_replicas(
            only_for_display, dr_ls_info_with_flag, acc_dr_task_cnt))) {
      LOG_WARN("fail to try remove permanent offline replicas",
               KR(ret), K(dr_ls_info_with_flag));
    }
  }
  // ATTENTION!!!
  // If this log stream has replicas only in member list, we need to have the
  // ability to let these replicas permanent offline. Because these replicas
  // may never be reported anymore(server is down) and disaster recovery module
  // regards these replicas as abnormal replicas and can not generate any task
  // for this log stream(locality alignment, replica migration etc.).
  // So we make sure log stream does not have replicas only in member_list AFTER try_remove_permanent_offline
  //
  // Also we DO NOT want to see replicas created during migration or rebuilding.
  // So we have to make sure those replicas with flag in learner_list not exists.
  // Please DO NOT change the order of try_remove_permanent_offline, filter_learner_with_flag, check_ls_only_in_member_list_or_with_flag_ and other operations.
  if (OB_SUCC(ret)
      && acc_dr_task_cnt <= 0) {
    bool is_only_in_member_list = true;
    if (OB_FAIL(check_ls_only_in_member_list_or_with_flag_(dr_ls_info))) {
      LOG_WARN("only_in_memberlist and flag replica check is failed", KR(ret), K(dr_ls_info));
    }
  }
  // step2: replicate to unit
  if (OB_SUCC(ret)
      && acc_dr_task_cnt <= 0) {
    if (OB_FAIL(try_replicate_to_unit(
            only_for_display, dr_ls_info, acc_dr_task_cnt))) {
      LOG_WARN("fail to try replicate to unit",
               KR(ret), K(dr_ls_info));
    }
  }
  // step3: locality alignment
  if (OB_SUCC(ret)
      && acc_dr_task_cnt <= 0) {
    if (OB_FAIL(try_locality_alignment(
            only_for_display, dr_ls_info, acc_dr_task_cnt))) {
      LOG_WARN("fail to try locality alignment",
               KR(ret), K(dr_ls_info));
    }
  }
  // step4: shrink resource pools
  // not supported now
  if (OB_SUCC(ret)
      && acc_dr_task_cnt <= 0) {
    if (OB_FAIL(try_shrink_resource_pools(
            only_for_display, dr_ls_info, acc_dr_task_cnt))) {
      LOG_WARN("fail to try shrink resource pools",
               KR(ret), K(dr_ls_info));
    }
  }
  // step5: cancel migrate unit
  if (OB_SUCC(ret)
      && acc_dr_task_cnt <= 0) {
    if (OB_FAIL(try_cancel_unit_migration(
            only_for_display, dr_ls_info, acc_dr_task_cnt))) {
      LOG_WARN("fail to try cancel unit migration",
               KR(ret), K(dr_ls_info));
    }
  }
  // step6: migrate to unit
  if (OB_SUCC(ret)
      && acc_dr_task_cnt <= 0) {
    if (OB_FAIL(try_migrate_to_unit(
            only_for_display, dr_ls_info, acc_dr_task_cnt))) {
      LOG_WARN("fail to try migrate to unit",
               KR(ret), K(dr_ls_info));
    }
  }
  const int64_t cost = ObTimeUtility::fast_current_time() - start_time;
  LOG_INFO("finish check dr tasks", KR(ret), K(cost),
            "tenant_id", dr_ls_info.get_tenant_id(), "ls_id", dr_ls_info.get_ls_id());
  return ret;
}

ERRSIM_POINT_DEF(ERRSIM_SKIP_CHECK_TENANT_SINGLE_REPLACE_REPLICA_TASK)
ERRSIM_POINT_DEF(ERRSIM_SKIP_PERSIST_REPLACE_REPLICA_TASK_INTO_INNER_TABLE)
int ObDRWorker::try_tenant_single_replica_task(
    const uint64_t tenant_id,
    const bool only_for_display,
    int64_t &acc_dr_task)
{
  int ret = OB_SUCCESS;
  int tmp_ret = OB_SUCCESS;
  if (OB_UNLIKELY(!is_valid_tenant_id(tenant_id))) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(tenant_id));
  } else if (OB_ISNULL(GCTX.sql_proxy_) || OB_ISNULL(GCTX.schema_service_) || OB_ISNULL(GCTX.config_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("schema svr or sql proxy or config_ is null",
              KR(ret), KP(GCTX.sql_proxy_), KP(GCTX.schema_service_), KP(GCTX.config_));
  } else if (OB_UNLIKELY(ERRSIM_SKIP_CHECK_TENANT_SINGLE_REPLACE_REPLICA_TASK)) {
    LOG_INFO("errsim skip check tenant single replace replica task");
  } else if (!GCTX.config_->is_rereplication_enabled()) {
    LOG_TRACE("config_ enable rereplication is false");
  } else {
    FLOG_INFO("start try tenant single replica", K(tenant_id), K(only_for_display));
    share::ObLSStatusOperator ls_status_operator;
    common::ObArray<share::ObLSStatusInfo> ls_status_info_array;
    if (OB_FAIL(ls_status_operator.get_all_ls_status_by_order(tenant_id, ls_status_info_array, *GCTX.sql_proxy_))) {
      LOG_WARN("fail to get all ls status", KR(ret), K(tenant_id));
    } else {
      for (int64_t i = 0; OB_SUCC(ret) && i < ls_status_info_array.count(); ++i) {
        FREE_DISASTER_RECOVERY_WORKER_TASK_MEMORY
        int64_t ls_acc_dr_task = 0;
        const share::ObLSStatusInfo &ls_status_info = ls_status_info_array.at(i);
        bool found_replace_task = false;
        common::ObZone dest_zone;
        ObServerInfoInTable source_server;
        palf::LogConfigVersion config_version;
        // reasons for using user tenant_id: get the tenant's available resources, meta tenant uses user tenant's unit
        DRLSInfo dr_ls_info_with_flag(gen_user_tenant_id(tenant_id), GCTX.schema_service_);
        if (OB_TMP_FAIL(build_ls_info_for_cross_az_dr(ls_status_info_array.at(i), dr_ls_info_with_flag))) {
          LOG_WARN("fail to init dr log stream info", KR(tmp_ret));
        } else if (OB_TMP_FAIL(check_ls_single_replica_dr_tasks(dr_ls_info_with_flag, config_version, source_server, dest_zone, found_replace_task))) {
          LOG_WARN("fail to check ls single replica tasks", KR(tmp_ret), K(dr_ls_info_with_flag));
        } else if (found_replace_task && OB_TMP_FAIL(generate_ls_single_replica_dr_tasks_(dr_ls_info_with_flag,
                                                                                          only_for_display,
                                                                                          config_version,
                                                                                          source_server,
                                                                                          dest_zone,
                                                                                          ls_acc_dr_task))) {
          LOG_WARN("fail to check ls single replica tasks", KR(tmp_ret), K(dr_ls_info_with_flag), K(only_for_display),
                    K(config_version), K(source_server), K(dest_zone));
        } else if (only_for_display || is_zero_row(dr_tasks_.count())) { // skip
          LOG_INFO("skip persist dr_task", K(only_for_display), "task_count", dr_tasks_.count());
        } else if (OB_UNLIKELY(ERRSIM_SKIP_PERSIST_REPLACE_REPLICA_TASK_INTO_INNER_TABLE)) {
          LOG_INFO("errsim skip persist replace replica task into inner table");
        } else if (OB_TMP_FAIL(persist_tasks_into_inner_table_(dr_tasks_, tenant_id, ls_status_info.ls_id_, false/*is_manual*/))) {
          LOG_WARN("fail to do end generated dr tasks", KR(ret), K(tenant_id), K(ls_status_info));
        }
        acc_dr_task += ls_acc_dr_task;
        FREE_DISASTER_RECOVERY_WORKER_TASK_MEMORY
      }
    }
  }
  return ret;
}

int ObDRWorker::build_ls_info_for_cross_az_dr(
    const share::ObLSStatusInfo &ls_status_info,
    DRLSInfo &dr_ls_info)
{
  int ret = OB_SUCCESS;
  share::ObLSInfo ls_info;
  if (OB_ISNULL(GCTX.lst_operator_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("lst_operator_ is nullptr", KR(ret), KP(GCTX.lst_operator_));
  } else if (OB_FAIL(GCTX.lst_operator_->get(GCONF.cluster_id,
                                          ls_status_info.tenant_id_,
                                          ls_status_info.ls_id_,
                                          share::ObLSTable::COMPOSITE_MODE,
                                          ls_info))) {
    LOG_WARN("fail to get log stream info", KR(ret), K(ls_status_info));
  } else if (OB_FAIL(dr_ls_info.init())) {
    LOG_WARN("fail to init dr log stream info", KR(ret));
  } else if (OB_FAIL(dr_ls_info.build_disaster_ls_info(ls_info, ls_status_info, false/*filter_readonly_replica_with_flag*/, true/*for_replace*/))) {
    // The read-only replica with the flag is the replica in migration,
    // filtering this type of replica requires the leader's member list information,
    // when checking the replace task, the LS may be no leader, and we cannot get the leader information in the LS meta table.
    LOG_WARN("fail to generate dr log stream info", KR(ret), K(ls_info), K(ls_status_info));
  }
  return ret;
}

int ObDRWorker::check_ls_single_replica_dr_tasks(
    DRLSInfo &dr_ls_info,
    palf::LogConfigVersion &config_version,
    share::ObServerInfoInTable &source_server_info,
    common::ObZone &dest_zone,
    bool &found)
{
  int ret = OB_SUCCESS;
  LOG_INFO("start check ls single replica dr task", "tenant_id", dr_ls_info.get_tenant_id(), "ls_id", dr_ls_info.get_ls_id());
  config_version.reset();
  source_server_info.reset();
  dest_zone.reset();
  found = false;
  common::ObAddr source_server_addr;
  common::ObMemberList member_list;
  bool pass_check = false;
  const common::ObIArray<share::ObZoneReplicaAttrSet> &cur_locality = dr_ls_info.get_locality();
  if (1 != cur_locality.count()) {
    LOG_TRACE("locality count not 1", K(cur_locality));
  } else if (0 == cur_locality.count()) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("locality count is 0", KR(ret), K(cur_locality));
  } else if (OB_FAIL(DisasterRecoveryUtils::get_member_info(dr_ls_info.get_tenant_id(),
                                                            dr_ls_info.get_ls_id(),
                                                            config_version,
                                                            member_list))) {
    LOG_WARN("failed to get member info", KR(ret), K(dr_ls_info));
  } else if (OB_FAIL(DisasterRecoveryUtils::check_member_list_for_single_replica(member_list, pass_check))) {
    LOG_WARN("failed to check member list", KR(ret), K(member_list));
  } else if (!pass_check) {
    LOG_INFO("not all server is inactive or same zone", K(member_list));
  } else if (0 == member_list.get_member_number()) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("member list count not 1", KR(ret), K(member_list));
  } else if (OB_FAIL(member_list.get_server_by_index(0, source_server_addr))) {
    LOG_WARN("failed to get server by index", KR(ret), K(source_server_addr));
  } else if (OB_FAIL(SVR_TRACER.get_server_info(source_server_addr, source_server_info))) {
    LOG_WARN("failed to get server info", KR(ret), K(source_server_addr));
  } else if (OB_FAIL(dest_zone.assign(cur_locality.at(0).zone_))) {
    LOG_WARN("failed to assign zone", KR(ret), K(cur_locality));
  } else if (source_server_info.get_zone() == dest_zone) {
    LOG_INFO("current replica is same zone with current locality, skip", K(dest_zone), K(source_server_info), K(member_list));
  } else {
    found = true;
  }
  return ret;
}

int ObDRWorker::generate_ls_single_replica_dr_tasks_(
    DRLSInfo &dr_ls_info,
    const bool only_for_display,
    const palf::LogConfigVersion &config_version,
    const share::ObServerInfoInTable &source_server_info,
    const common::ObZone &dest_zone,
    int64_t &ls_acc_dr_task)
{
  int ret = OB_SUCCESS;
  LOG_INFO("generate ls sinle replica task", K(config_version), K(source_server_info), K(dest_zone));
  UnitProvider unit_provider;
  share::ObUnit dest_unit;
  const share::ObLSStatusInfo *ls_status_info = nullptr;
  if (OB_FAIL(unit_provider.init(gen_user_tenant_id(dr_ls_info.get_tenant_id()), dr_ls_info))) {
    LOG_WARN("fail to init unit provider", KR(ret), K(dr_ls_info));
  } else if (OB_FAIL(dr_ls_info.get_ls_status_info(ls_status_info))) {
    LOG_WARN("fail to get log stream status info", KR(ret));
  } else if (OB_UNLIKELY(nullptr == ls_status_info)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("ls status info ptr is null", KR(ret), KP(ls_status_info));
  } else if (OB_FAIL(unit_provider.allocate_unit(dest_zone, ls_status_info->unit_group_id_, dest_unit))) {
    LOG_WARN("can not allocate valid unit", KR(ret), K(dest_zone), K(*ls_status_info));
  } else if (OB_FAIL(generate_single_replica_task_(only_for_display,
                                                   config_version,
                                                   source_server_info,
                                                   dest_unit,
                                                   dr_ls_info,
                                                   ls_acc_dr_task))) {
    LOG_WARN("failed to generate single replica task", KR(ret), K(only_for_display), K(config_version),
              K(source_server_info), K(dest_unit), K(dr_ls_info));
  }
  return ret;
}

int ObDRWorker::generate_single_replica_task_(
    const bool only_for_display,
    const palf::LogConfigVersion &config_version,
    const share::ObServerInfoInTable &src_server_info,
    const share::ObUnit &dest_unit,
    DRLSInfo &dr_ls_info,
    int64_t &acc_dr_task)
{
  int ret = OB_SUCCESS;
  ObReplicaMember dst_member(dest_unit.server_, ObTimeUtility::current_time(), REPLICA_TYPE_FULL);
  const char* task_comment = drtask::REPLACE_LOCALITY_REPLACE_REPLIACE;
  ObDRTaskKey task_key;
  uint64_t tenant_id = 0;
  share::ObLSID ls_id;
  share::ObTaskId task_id;
  bool can_generate = false;
  ObReplaceLSReplicaTask replace_task;
  if (OB_UNLIKELY(!dest_unit.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(dest_unit));
  } else if (OB_FAIL(dr_ls_info.get_ls_id(tenant_id, ls_id))) {
    LOG_WARN("failed to get tenant and ls id", KR(ret));
  } else if (only_for_display) {
    ObLSReplicaTaskDisplayInfo display_info;
    if (OB_FAIL(display_info.init(
                  tenant_id,
                  ls_id,
                  ObDRTaskType::LS_REPLACE_REPLICA,
                  ObDRTaskPriority::HIGH_PRI,
                  dest_unit.server_,
                  REPLICA_TYPE_FULL,
                  1/*target_replica_paxos_replica_number*/,
                  src_server_info.get_server(),
                  REPLICA_TYPE_FULL,
                  1/*source_replica_paxos_replica_number*/,
                  dest_unit.server_,
                  task_comment,
                  config_version))) {
      LOG_WARN("fail to init a ObLSReplicaTaskDisplayInfo", KR(ret));
    } else if (OB_FAIL(add_display_info(display_info))) {
      LOG_WARN("fail to add display info", KR(ret), K(display_info));
    } else {
      LOG_INFO("success to add display info", KR(ret), K(display_info));
    }
  } else if (FALSE_IT(task_id.init(GCONF.self_addr_))) {
  } else if (OB_FAIL(generate_task_key(dr_ls_info, dest_unit.server_, ObDRTaskType::LS_REPLACE_REPLICA, task_key))) {
    LOG_WARN("fail to generate task key", KR(ret), K(dr_ls_info), K(dest_unit));
  } else if (OB_FAIL(check_can_generate_task(
                          acc_dr_task,
                          false/*need_check_has_leader_while_remove_replica*/,
                          dest_unit.server_,
                          dr_ls_info,
                          ObDRTaskType::LS_REPLACE_REPLICA,
                          can_generate))) {
    LOG_WARN("fail to check can generate replicate to unit task", KR(ret));
  } else if (can_generate) {
    if (OB_FAIL(replace_task.build(task_key, task_id, task_comment, dst_member, config_version))) {
      LOG_WARN("fail to build replace task", KR(ret), K(task_key), K(task_id), K(task_comment), K(dst_member), K(config_version));
    } else if (OB_FAIL(add_task_(replace_task, acc_dr_task))) {
      LOG_WARN("fail to add task", KR(ret), K(replace_task));
    }
  }
  return ret;
}

int ObDRWorker::persist_tasks_into_inner_table_(
    const ObIArray<ObDRTask*> &dr_tasks,
    const uint64_t tenant_id,
    const share::ObLSID &ls_id,
    const bool is_manual)
{
  int ret = OB_SUCCESS;
  const int64_t start_time = ObTimeUtility::fast_current_time();
  ObSqlString sql;
  ObMySQLTransaction trans;
  ObArray<ObDRTaskKey> task_keys;
  int64_t service_epoch = OB_INVALID_ID;
  uint64_t service_epoch_tenant = OB_INVALID_TENANT_ID;
  uint64_t persistent_tenant = OB_INVALID_TENANT_ID;
  ObDRTask* task = nullptr;
  if (OB_UNLIKELY(!is_valid_tenant_id(tenant_id)
               || !ls_id.is_valid_with_tenant(tenant_id))
               || is_zero_row(dr_tasks.count())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(tenant_id), K(ls_id), "task_count", dr_tasks.count());
  } else if (OB_ISNULL(GCTX.sql_proxy_) || OB_ISNULL(dr_tasks.at(0))) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("sql_proxy_ is nullptr", KR(ret), KP(GCTX.sql_proxy_));
  } else if (FALSE_IT(task = dr_tasks.at(0))) {
  } else if (OB_FAIL(DisasterRecoveryUtils::get_service_epoch_value_to_check(*task, tenant_id_, service_epoch_, service_epoch))) {
    LOG_WARN("failed to get service epoch and tenant id", KR(ret), KPC(task), K_(service_epoch));
  } else if (OB_FAIL(DisasterRecoveryUtils::get_service_epoch_and_persistent_tenant(
                                              task->get_tenant_id(),
                                              task->get_disaster_recovery_task_type(),
                                              service_epoch_tenant,
                                              persistent_tenant))) {
  LOG_WARN("failed to get service epoch and persistent tenant id", KR(ret), K(task));
  } else if (OB_FAIL(trans.start(GCTX.sql_proxy_, persistent_tenant))) {
    LOG_WARN("failed to start trans", KR(ret), KPC(task), K(persistent_tenant));
  } else if (OB_FAIL(sql.append_fmt("SELECT * FROM %s WHERE tenant_id = %ld AND ls_id = %ld",
                                      share::OB_ALL_LS_REPLICA_TASK_TNAME,
                                      tenant_id,
                                      ls_id.id()))) {
    LOG_WARN("failed to assign sql", KR(ret), K(tenant_id), K(ls_id));
  } else if (OB_FAIL(DisasterRecoveryUtils::lock_service_epoch(trans, service_epoch_tenant, service_epoch))) {
    LOG_WARN("failed to lock server epoch", KR(ret), KPC(task), K(service_epoch_tenant), K(service_epoch));
  } else if (OB_FAIL(table_operator_.load_task_key_from_inner_table(*GCTX.sql_proxy_, sql, persistent_tenant, task_keys))) {
    LOG_WARN("failed to load task from inner table", KR(ret), KPC(task), K(persistent_tenant), K(sql));
  } else {
    bool is_conflict = false;
    for (int64_t i = 0; OB_SUCC(ret) && i < dr_tasks.count(); ++i) {
      is_conflict =false;
      const ObDRTask *task = dr_tasks.at(i);
      share::ObServerInfoInTable server_info;
      if (OB_ISNULL(task)) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("task is nullptr", KR(ret), KP(task));
      } else if (OB_FAIL(check_whether_task_conflict_(task_keys, task->get_task_key(), is_conflict))) {
        LOG_WARN("failed to check whether task conflict", KR(ret), K(*task));
      } else if (is_conflict) {
        if (is_manual) {
          ret = OB_ENTRY_EXIST;
          LOG_USER_ERROR(OB_ENTRY_EXIST, "LS has task executing, current operation is not allowed");
        }
        LOG_INFO("ls disaster recovery task conflict in list", KR(ret), K(*task));
      } else if (OB_FAIL(SVR_TRACER.get_server_info(task->get_dst_server(), server_info))) {
        LOG_WARN("fail to get server_info", KR(ret), K(*task));
      } else if (server_info.is_permanent_offline()) {
        if (is_manual) {
          ret = OB_OP_NOT_ALLOW;
          LOG_USER_ERROR(OB_OP_NOT_ALLOW, "Task execution server status is permanent offline and the current operation is");
        }
        LOG_WARN("dest server permanent offline, can't execute", K(*task), K(server_info));
      } else if (OB_FAIL(task_keys.push_back(task->get_task_key()))) {
        LOG_WARN("failed to push task_key", KR(ret), K(*task));
      } else if (OB_FAIL(check_clone_status_and_insert_task_(*task, persistent_tenant, trans))) {
        LOG_WARN("failed to insert task", KR(ret), K(*task));
      }
    }
  }
  COMMIT_DISASTER_RECOVERY_WORKER_TRANS
  const int64_t cost = ObTimeUtility::fast_current_time() - start_time;
  LOG_INFO("finish persist tasks into inner table", KR(ret), K(cost), K(tenant_id), K(ls_id));
  return ret;
}

int ObDRWorker::execute_manual_dr_task(const obrpc::ObAdminAlterLSReplicaArg &arg)
{
  FLOG_INFO("execute alter ls replica request", K(arg));
  int ret = OB_SUCCESS;
  int64_t start_time = ObTimeUtility::current_time();
  if (OB_UNLIKELY(!arg.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid arg", KR(ret), K(arg));
  } else {
    switch (arg.get_alter_task_type().get_type()) {
      case ObAlterLSReplicaTaskType::AddLSReplicaTask: {
        if (OB_FAIL(do_add_ls_replica_task(arg))) {
          LOG_WARN("add ls replica task failed", KR(ret), K(arg));
        }
        break;
      }
      case ObAlterLSReplicaTaskType::RemoveLSReplicaTask: {
        if (OB_FAIL(do_remove_ls_replica_task(arg))) {
          LOG_WARN("remove ls replica task failed", KR(ret), K(arg));
        }
        break;
      }
      case ObAlterLSReplicaTaskType::MigrateLSReplicaTask: {
        if (OB_FAIL(do_migrate_ls_replica_task(arg))) {
          LOG_WARN("migrate ls replica task failed", KR(ret), K(arg));
        }
        break;
      }
      case ObAlterLSReplicaTaskType::ModifyLSReplicaTypeTask: {
        if (OB_FAIL(do_modify_ls_replica_type_task(arg))) {
          LOG_WARN("modify ls replica task failed", KR(ret), K(arg));
        }
        break;
      }
      case ObAlterLSReplicaTaskType::ModifyLSPaxosReplicaNumTask: {
        if (OB_FAIL(do_modify_ls_paxos_replica_num_task(arg))) {
          LOG_WARN("modify ls paxos_replica_num task failed", KR(ret), K(arg));
        }
        break;
      }
      case ObAlterLSReplicaTaskType::CancelLSReplicaTask: {
        if (OB_FAIL(do_cancel_ls_replica_task(arg))) {
          LOG_WARN("cancel ls replica task failed", KR(ret), K(arg));
        }
        break;
      }
      default: {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("task type unexpected", KR(ret), K(arg));
        break;
      }
    }
  }
  int64_t cost_time = ObTimeUtility::current_time() - start_time;
  FLOG_INFO("execute alter ls replica request over", KR(ret), K(arg), K(cost_time));
  return ret;
}

int ObDRWorker::check_whether_task_conflict_(
    const ObIArray<ObDRTaskKey> &dr_task_keys,
    const ObDRTaskKey &task_key,
    bool &is_conflict)
{
  int ret = OB_SUCCESS;
  bool enable_parallel_migration = false;
  is_conflict = false;
  if (OB_UNLIKELY(!task_key.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(task_key));
  } else if (OB_FAIL(DisasterRecoveryUtils::check_tenant_enable_parallel_migration(task_key.get_tenant_id(), enable_parallel_migration))) {
    LOG_WARN("failed to check tenant enable_parallel_migration", KR(ret), K(task_key));
  } else {
    for (int64_t i = 0; OB_SUCC(ret) && i < dr_task_keys.count(); ++i) {
      const ObDRTaskKey &task_key_in_array = dr_task_keys.at(i);
      // tenant_id and ls_id is same
      if (task_key_in_array.get_tenant_id() == task_key.get_tenant_id()
       && task_key_in_array.get_ls_id() == task_key.get_ls_id()) {
        if (!enable_parallel_migration) {
          is_conflict = true;
        } else if (ObDRTaskType::LS_MIGRATE_REPLICA != task_key.get_task_type()
                || ObDRTaskType::LS_MIGRATE_REPLICA != task_key_in_array.get_task_type()) {
          // if one of the two task is not a migration task, it is conflict
          is_conflict = true;
        } else if (task_key_in_array.get_zone() == task_key.get_zone()) {
          is_conflict = true;
        }
        if (is_conflict) {
          LOG_INFO("conflict with existing tasks", K(task_key), K(task_key_in_array), K(enable_parallel_migration));
          break;
        }
      } // end if
    } //end for
  }
  return ret;
}

int ObDRWorker::check_clone_status_and_insert_task_(
    const ObDRTask &task,
    const uint64_t persistent_tenant,
    common::ObMySQLTransaction &trans)
{
  int ret = OB_SUCCESS;
  observer::ObInnerSQLConnection *conn = NULL;
  const int64_t timeout = GCONF.internal_sql_execute_timeout;
  if (ObDRTaskType::LS_REPLACE_REPLICA == task.get_disaster_recovery_task_type()) {
    // replace replace task no need check clone status
  } else {
    ObConflictCaseWithClone case_to_check(ObConflictCaseWithClone::MODIFY_REPLICA);
    if (OB_ISNULL(conn = static_cast<observer::ObInnerSQLConnection *>(trans.get_connection()))) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("conn_ is NULL", KR(ret));
    } else if (OB_FAIL(transaction::tablelock::ObInnerConnectionLockUtil::lock_table(persistent_tenant,
                                                             OB_ALL_LS_REPLICA_TASK_TID,
                                                             transaction::tablelock::EXCLUSIVE,
                                                             timeout,
                                                             conn))) {
      LOG_WARN("lock dest table failed", KR(ret), K(persistent_tenant));
    } else if (OB_FAIL(ObTenantSnapshotUtil::check_tenant_not_in_cloning_procedure(task.get_tenant_id(), case_to_check))) {
      LOG_WARN("fail to check whether tenant is in cloning procedure", KR(ret));
    }
  }
  if (OB_FAIL(ret)) {
  } else if (OB_FAIL(table_operator_.insert_task(trans, persistent_tenant, task, false/*record_history*/))) {
    LOG_WARN("failed to insert task", KR(ret), K(task));
  } else {
    LOG_INFO("succeed to insert task", K(task));
  }
  return ret;
}

int ObDRWorker::do_add_ls_replica_task(
    const obrpc::ObAdminAlterLSReplicaArg &arg)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!arg.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(arg));
  } else if (OB_ISNULL(GCTX.schema_service_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("schema_service_ is nullptr", KR(ret), KP(GCTX.schema_service_));
  } else {
    ObArray<ObDRTask*> dr_tasks;
    DRLSInfo dr_ls_info(gen_user_tenant_id(arg.get_tenant_id()), GCTX.schema_service_);
    ObAddLSReplicaTask add_replica_task;
    if (OB_FAIL(check_and_init_info_for_alter_ls_(arg, dr_ls_info))) {
      LOG_WARN("fail to check and init info for alter ls", KR(ret), K(arg), K(dr_ls_info));
    } else if (OB_FAIL(build_add_replica_task_(arg, dr_ls_info, add_replica_task))) {
      LOG_WARN("fail to build add replica task parameters", KR(ret), K(arg), K(dr_ls_info));
    } else if (OB_FAIL(dr_tasks.push_back(&add_replica_task))) {
      LOG_WARN("fail to build push back task", KR(ret), K(arg), K(add_replica_task));
    } else if (OB_FAIL(persist_tasks_into_inner_table_(dr_tasks, arg.get_tenant_id(), arg.get_ls_id(), true/*is_manual*/))) {
      LOG_WARN("fail to persist tasks into table", KR(ret), K(arg), K(add_replica_task));
    }
  }
  FLOG_INFO("ObDRWorker do add ls replica task", KR(ret), K(arg));
  return ret;
}

int ObDRWorker::do_remove_ls_replica_task(
    const obrpc::ObAdminAlterLSReplicaArg &arg)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!arg.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(arg));
  } else if (OB_ISNULL(GCTX.schema_service_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("schema_service_ is nullptr", KR(ret), KP(GCTX.schema_service_));
  } else {
    ObArray<ObDRTask*> dr_tasks;
    DRLSInfo dr_ls_info(gen_user_tenant_id(arg.get_tenant_id()), GCTX.schema_service_);
    ObRemoveLSReplicaTask remove_replica_task;
    if (OB_FAIL(check_and_init_info_for_alter_ls_(arg, dr_ls_info))) {
      LOG_WARN("fail to check and init info for alter ls", KR(ret), K(arg), K(dr_ls_info));
    } else if (OB_FAIL(build_remove_replica_task_(arg, dr_ls_info, remove_replica_task))) {
      LOG_WARN("fail to build remove replica task parameters", KR(ret), K(arg), K(dr_ls_info));
    } else if (OB_FAIL(dr_tasks.push_back(&remove_replica_task))) {
      LOG_WARN("fail to build push back task", KR(ret), K(arg), K(remove_replica_task));
    } else if (OB_FAIL(persist_tasks_into_inner_table_(dr_tasks, arg.get_tenant_id(), arg.get_ls_id(), true/*is_manual*/))) {
      LOG_WARN("fail to persist tasks into table", KR(ret), K(arg), K(remove_replica_task));
    }
  }
  FLOG_INFO("ObDRWorker do remove ls replica task", KR(ret), K(arg));
  return ret;
}

int ObDRWorker::do_modify_ls_replica_type_task(
    const obrpc::ObAdminAlterLSReplicaArg &arg)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!arg.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(arg));
  } else if (OB_ISNULL(GCTX.schema_service_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("schema_service_ is nullptr", KR(ret), KP(GCTX.schema_service_));
  } else {
    ObArray<ObDRTask*> dr_tasks;
    DRLSInfo dr_ls_info(gen_user_tenant_id(arg.get_tenant_id()), GCTX.schema_service_);
    ObLSTypeTransformTask modify_replica_type_task;
    if (OB_FAIL(check_and_init_info_for_alter_ls_(arg, dr_ls_info))) {
      LOG_WARN("fail to check and init info for alter ls", KR(ret), K(arg), K(dr_ls_info));
    } else if (OB_FAIL(build_modify_replica_type_task_(arg, dr_ls_info, modify_replica_type_task))) {
      LOG_WARN("fail to build modify replica task parameters", KR(ret), K(arg), K(dr_ls_info));
    } else if (OB_FAIL(dr_tasks.push_back(&modify_replica_type_task))) {
      LOG_WARN("fail to build push back task", KR(ret), K(arg), K(modify_replica_type_task));
    } else if (OB_FAIL(persist_tasks_into_inner_table_(dr_tasks, arg.get_tenant_id(), arg.get_ls_id(), true/*is_manual*/))) {
      LOG_WARN("fail to persist tasks into table", KR(ret), K(arg), K(modify_replica_type_task));
    }
  }
  FLOG_INFO("ObDRWorker do modify ls replica task", KR(ret), K(arg));
  return ret;
}

int ObDRWorker::do_migrate_ls_replica_task(
    const obrpc::ObAdminAlterLSReplicaArg &arg)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!arg.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(arg));
  } else if (OB_ISNULL(GCTX.schema_service_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("schema_service_ is nullptr", KR(ret), KP(GCTX.schema_service_));
  } else {
    ObArray<ObDRTask*> dr_tasks;
    DRLSInfo dr_ls_info(gen_user_tenant_id(arg.get_tenant_id()), GCTX.schema_service_);
    ObMigrateLSReplicaTask migrate_replica_task;
    if (OB_FAIL(check_and_init_info_for_alter_ls_(arg, dr_ls_info))) {
      LOG_WARN("fail to check and init info for alter ls", KR(ret), K(arg), K(dr_ls_info));
    } else if (OB_FAIL(build_migrate_replica_task_(arg, dr_ls_info, migrate_replica_task))) {
      LOG_WARN("fail to build migrate replica task", KR(ret), K(arg), K(dr_ls_info));
    } else if (OB_FAIL(dr_tasks.push_back(&migrate_replica_task))) {
      LOG_WARN("fail to build push back task", KR(ret), K(arg), K(migrate_replica_task));
    } else if (OB_FAIL(persist_tasks_into_inner_table_(dr_tasks, arg.get_tenant_id(), arg.get_ls_id(), true/*is_manual*/))) {
      LOG_WARN("fail to persist tasks into table", KR(ret), K(arg), K(migrate_replica_task));
    }
  }
  FLOG_INFO("ObDRWorker do migrate ls replica task", KR(ret), K(arg));
  return ret;
}

int ObDRWorker::do_modify_ls_paxos_replica_num_task(
    const obrpc::ObAdminAlterLSReplicaArg &arg)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!arg.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(arg));
  } else if (OB_ISNULL(GCTX.schema_service_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("schema_service_ is nullptr", KR(ret), KP(GCTX.schema_service_));
  } else {
    ObArray<ObDRTask*> dr_tasks;
    DRLSInfo dr_ls_info(gen_user_tenant_id(arg.get_tenant_id()), GCTX.schema_service_);
    ObLSModifyPaxosReplicaNumberTask modify_paxos_replica_number_task;
    if (OB_FAIL(check_and_init_info_for_alter_ls_(arg, dr_ls_info))) {
      LOG_WARN("fail to check and init info for alter ls", KR(ret), K(arg), K(dr_ls_info));
    } else if (OB_FAIL(build_modify_paxos_replica_num_task_(arg, dr_ls_info, modify_paxos_replica_number_task))) {
      LOG_WARN("fail to build modify paxos_replica_num task parameters", KR(ret), K(arg), K(dr_ls_info));
    } else if (OB_FAIL(dr_tasks.push_back(&modify_paxos_replica_number_task))) {
      LOG_WARN("fail to build push back task", KR(ret), K(arg), K(modify_paxos_replica_number_task));
    } else if (OB_FAIL(persist_tasks_into_inner_table_(dr_tasks, arg.get_tenant_id(), arg.get_ls_id(), true/*is_manual*/))) {
      LOG_WARN("fail to persist tasks into table", KR(ret), K(arg), K(modify_paxos_replica_number_task));
    }
  }
  FLOG_INFO("ObDRWorker do modify ls paxos_replica_num task", KR(ret), K(arg));
  return ret;
}

int ObDRWorker::do_cancel_ls_replica_task(
    const obrpc::ObAdminAlterLSReplicaArg &arg)
{
  int ret = OB_SUCCESS;
  ObSqlString sql;
  ObMySQLTransaction trans;
  const uint64_t tenant_id = arg.get_tenant_id();
  const uint64_t sql_tenant_id = gen_meta_tenant_id(tenant_id);
  ObArray<ObDRTask*> dr_tasks;
  ObDRTask *task = nullptr;
  common::ObArenaAllocator task_alloc("ObDRWorker", OB_MALLOC_NORMAL_BLOCK_SIZE, MTL_ID());
  if (OB_UNLIKELY(!arg.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(arg));
  } else if (OB_ISNULL(GCTX.sql_proxy_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("some ptr is null", KR(ret), KP(GCTX.sql_proxy_));
  } else if (OB_FAIL(trans.start(GCTX.sql_proxy_, sql_tenant_id))) {
    LOG_WARN("failed to start trans", KR(ret), K(sql_tenant_id));
  } else if (OB_FAIL(DisasterRecoveryUtils::lock_service_epoch(trans, tenant_id, service_epoch_))) {
    LOG_WARN("failed to lock server epoch", KR(ret), K(arg));
  } else if (OB_FAIL(sql.append_fmt("SELECT * FROM %s WHERE tenant_id = %ld AND task_id = '%s'",
                        share::OB_ALL_LS_REPLICA_TASK_TNAME, tenant_id, arg.get_task_id().ptr()))) {
    LOG_WARN("failed to assign sql", KR(ret), K(tenant_id));
  } else if (OB_FAIL(table_operator_.load_task_from_inner_table(*GCTX.sql_proxy_, tenant_id, sql, task_alloc, dr_tasks))) {
    LOG_WARN("failed to load task from inner table", KR(ret), K(tenant_id), K(sql));
  } else if (is_zero_row(dr_tasks.count())) {
    ret = OB_ENTRY_NOT_EXIST;
    LOG_USER_ERROR(OB_ENTRY_NOT_EXIST, "Task not exist");
    LOG_WARN("task_id not existd", KR(ret), K(arg));
  } else if (!is_single_row(dr_tasks.count())) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("task count unexpected", KR(ret), "count", dr_tasks.count());
  } else if (FALSE_IT(task = dr_tasks.at(0))) { // never fail
  } else if (OB_ISNULL(task)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("task is null ptr", KR(ret), KP(task));
  } else if (task->get_task_status().is_waiting_status()) {
    // check task status and delete task need in trans lock
    FLOG_INFO("cancel migrate task in wait status", K(task));
    if (OB_FAIL(table_operator_.delete_task(trans, tenant_id, *task))) {
      LOG_WARN("fail to delete task", KR(ret), K(*task));
    }
  }
  COMMIT_DISASTER_RECOVERY_WORKER_TRANS
  if (OB_FAIL(ret) || OB_ISNULL(task)) {
  } else if (task->get_task_status().is_inprogress_status()) {
    FLOG_INFO("cancel migrate task in inprogress status", K(*task));
    if (OB_FAIL(DisasterRecoveryUtils::send_rpc_to_cancel_task(*task))) {
      LOG_WARN("fail to delete task", KR(ret), K(*task));
    }
  } else if (task->get_task_status().is_waiting_status()) {
    // skip
  } else {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("task status unexpected", KR(ret), K(*task));
  }
  for (int64_t i = 0; i < dr_tasks.count(); ++i) {
    ObDRTask *task_in_array = dr_tasks.at(i);
    if (OB_NOT_NULL(task_in_array)) {
      task_in_array->~ObDRTask();
    }
  }
  task_alloc.reset();
  dr_tasks.reset();
  FLOG_INFO("ObDRWorker do cancel ls replica task over", KR(ret), K(arg));
  return ret;
}

int ObDRWorker::add_task_(
    const ObDRTask &task,
    int64_t &acc_dr_task)
{
  int ret = OB_SUCCESS;
  void *raw_ptr = nullptr;
  ObDRTask *new_task = nullptr;
  if (OB_UNLIKELY(!task.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(task));
  } else if (OB_ISNULL(raw_ptr = task_alloc_.alloc(task.get_clone_size()))) {
    ret = OB_ALLOCATE_MEMORY_FAILED;
    LOG_WARN("fail to alloc task", KR(ret), "size", task.get_clone_size());
  } else if (OB_FAIL(task.clone(raw_ptr, new_task))) {
    LOG_WARN("fail to clone task", KR(ret), K(task));
  } else if (OB_ISNULL(new_task)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("new task ptr is null", KR(ret));
  } else if (OB_FAIL(dr_tasks_.push_back(new_task))) {
    LOG_WARN("failed to push task key in array", KR(ret), K(task));
  } else {
    acc_dr_task++;
    LOG_INFO("success to add a task to dr tasks array", KR(ret), K(task));
  }
  if (OB_FAIL(ret)) {
    if (OB_NOT_NULL(new_task)) {
      new_task->~ObDRTask();
      // no need to free memory when failed for arena, just destruct
      new_task = nullptr;
    }
  }
  return ret;
}

int ObDRWorker::check_and_init_info_for_alter_ls_(
    const obrpc::ObAdminAlterLSReplicaArg& arg,
    DRLSInfo& dr_ls_info)
{
  int ret = OB_SUCCESS;
  share::ObLSInfo ls_info;
  share::ObLSStatusInfo ls_status_info;
  const share::ObLSReplica *leader_replica = nullptr;
  if (OB_UNLIKELY(!arg.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(arg));
  } else if (OB_FAIL(check_ls_exist_and_get_ls_info_(arg.get_ls_id(),
                        arg.get_tenant_id(), ls_info, ls_status_info))) {
    LOG_WARN("fail to check tenent ls", KR(ret), K(arg));
  } else if (OB_FAIL(ls_info.find_leader(leader_replica))) {
    LOG_WARN("fail to find leader", KR(ret), K(ls_info), K(arg));
    if (OB_ENTRY_NOT_EXIST == ret) {
      ret = OB_LEADER_NOT_EXIST;
      LOG_USER_ERROR(OB_LEADER_NOT_EXIST);
    }
  } else if (OB_FAIL(dr_ls_info.init())) {
    LOG_WARN("fail to init dr log stream info", KR(ret), K(dr_ls_info));
  } else if (OB_FAIL(dr_ls_info.build_disaster_ls_info(
                      ls_info, ls_status_info, false/*filter_readonly_replicas_with_flag*/))) {
    LOG_WARN("fail to generate dr log stream info", KR(ret), K(ls_info), K(ls_status_info));
  }
  return ret;
}

int ObDRWorker::check_ls_exist_and_get_ls_info_(
    const share::ObLSID& ls_id,
    const uint64_t tenant_id,
    share::ObLSInfo& ls_info,
    share::ObLSStatusInfo& ls_status_info)
{
  int ret = OB_SUCCESS;
  share::ObLSStatusOperator ls_status_operator;
  ls_info.reset();
  ls_status_info.reset();
  if (OB_UNLIKELY(!is_valid_tenant_id(tenant_id)) || OB_UNLIKELY(!ls_id.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(tenant_id), K(ls_id));
  } else if (!ls_id.is_valid_with_tenant(tenant_id)) {
    ret = OB_ENTRY_NOT_EXIST;
    LOG_USER_ERROR(OB_ENTRY_NOT_EXIST, "LS does not exist");
    LOG_WARN("check ls_id is_valid_with_tenant failed", KR(ret), K(tenant_id), K(ls_id));
  } else if (OB_ISNULL(GCTX.lst_operator_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("GCTX.lst_operator_ is null", KR(ret), KP(GCTX.lst_operator_));
  } else if (OB_FAIL(GCTX.lst_operator_->get(GCONF.cluster_id, tenant_id, ls_id,
                          share::ObLSTable::COMPOSITE_MODE, ls_info))) {
    LOG_WARN("get ls info failed", KR(ret), K(tenant_id), K(ls_id));
  } else if (ls_info.get_replicas().count() == 0) {
    ret = OB_ENTRY_NOT_EXIST;
    LOG_USER_ERROR(OB_ENTRY_NOT_EXIST, "LS does not exist");
    LOG_WARN("ls_info.get_replicas().count() == 0", KR(ret), K(tenant_id), K(ls_id), K(ls_info));
  } else if (OB_ISNULL(GCTX.sql_proxy_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("sql_proxy_ is null", KR(ret), KP(GCTX.sql_proxy_));
  } else if (OB_FAIL(ls_status_operator.get_ls_status_info(
                      tenant_id, ls_id, ls_status_info, *GCTX.sql_proxy_))) {
    if (OB_ENTRY_NOT_EXIST == ret) {
      LOG_USER_ERROR(OB_ENTRY_NOT_EXIST, "LS does not exist");
    }
    LOG_WARN("fail to get all ls status", KR(ret), K(tenant_id), K(ls_id), K(ls_info), KP(GCTX.sql_proxy_));
  } else if (ls_status_info.ls_is_creating() || ls_status_info.ls_is_create_abort()) {
    ret = OB_OP_NOT_ALLOW;
    LOG_USER_ERROR(OB_OP_NOT_ALLOW, "LS is in CREATING or CREATE_ABORT status, current operation is");
    LOG_WARN("LS is creating, current operation is", KR(ret), K(tenant_id), K(ls_id), K(ls_info));
  }
  return ret;
}

int ObDRWorker::check_unit_exist_and_get_unit_(
    const common::ObAddr &task_execute_server,
    const uint64_t tenant_id,
    const bool is_migrate_source_valid,
    share::ObUnit& unit)
{
  int ret = OB_SUCCESS;
  unit.reset();
  ObUnitTableOperator unit_operator;
  common::ObArray<share::ObUnit> unit_info_array;
  if (OB_UNLIKELY(!task_execute_server.is_valid()
                || OB_INVALID_TENANT_ID == tenant_id)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(task_execute_server), K(tenant_id));
  } else if (OB_ISNULL(GCTX.sql_proxy_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("sql_proxy_ is null", KR(ret), KP(GCTX.sql_proxy_));
  } else if (OB_FAIL(unit_operator.init(*GCTX.sql_proxy_))) {
    LOG_WARN("unit operator init failed", KR(ret), KP(GCTX.sql_proxy_));
  } else if (OB_FAIL(unit_operator.get_units_by_tenant(
                            gen_user_tenant_id(tenant_id), unit_info_array))) {
    LOG_WARN("fail to get unit info array", KR(ret), K(tenant_id));
  } else {
    bool found = false;
    for (int64_t i = 0; OB_SUCC(ret) && !found && i < unit_info_array.count(); ++i) {
      if ((unit_info_array.at(i).server_ == task_execute_server)
       || (is_migrate_source_valid && unit_info_array.at(i).migrate_from_server_ == task_execute_server)) {
        if (OB_FAIL(unit.assign(unit_info_array.at(i)))) {
          LOG_WARN("fail to assign unit", KR(ret), K(unit_info_array.at(i)));
        } else {
          found = true;
        }
      }
    }
    if (OB_SUCC(ret) && !found) {
      ret = OB_ENTRY_NOT_EXIST;
      LOG_USER_ERROR(OB_ENTRY_NOT_EXIST, "Tenant has no unit on the server");
      LOG_WARN("this tenant has no unit on the server",
                    KR(ret), K(tenant_id), K(task_execute_server), K(found));
    }
  }
  return ret;
}

int ObDRWorker::check_task_execute_server_status_(
    const common::ObAddr &task_execute_server,
    const bool need_check_can_migrate_in)
{
  int ret = OB_SUCCESS;
  ObServerInfoInTable server_info;
  if (OB_UNLIKELY(!task_execute_server.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(task_execute_server));
  } else {
    char err_msg[OB_MAX_ERROR_MSG_LEN] = {0};
    char addr_str_buf[128] = {0};
    if (OB_FAIL(task_execute_server.ip_port_to_string(addr_str_buf, 128))) {
      LOG_WARN("fail to get server addr string", KR(ret), K(task_execute_server));
    } else if (OB_FAIL(SVR_TRACER.get_server_info(task_execute_server, server_info))) {
      LOG_WARN("fail to check server active", KR(ret), K(task_execute_server));
    } else if (!server_info.is_alive()) {
      snprintf(err_msg, sizeof(err_msg),
      "The task needs to be executed on %s which status is not alive and the current operation is", addr_str_buf);
      ret = OB_OP_NOT_ALLOW;
      LOG_USER_ERROR(OB_OP_NOT_ALLOW, err_msg);
      LOG_WARN("server is not active", KR(ret), K(task_execute_server), K(server_info));
    } else if (need_check_can_migrate_in && !server_info.can_migrate_in()) {
      snprintf(err_msg, sizeof(err_msg),
      "The task needs to be executed on %s which can not migrate in and the current operation is", addr_str_buf);
      ret = OB_OP_NOT_ALLOW;
      LOG_USER_ERROR(OB_OP_NOT_ALLOW, err_msg);
      LOG_WARN("server can not migrate in now", KR(ret), K(task_execute_server), K(server_info));
    }
  }
  return ret;
}

int ObDRWorker::get_replica_type_by_leader_(
    const common::ObAddr& server_addr,
    const DRLSInfo &dr_ls_info,
    common::ObReplicaType& replica_type)
{
  // not leader replica get replica type may not right. when remove or modify replica,
  // replica type wrong may result in fatal error, so get it by leader
  int ret = OB_SUCCESS;
  replica_type = REPLICA_TYPE_INVALID;
  common::ObAddr leader_addr; // not used
  GlobalLearnerList learner_list;
  common::ObMemberList member_list;
  if (OB_UNLIKELY(!server_addr.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(server_addr));
  } else if (OB_UNLIKELY(0 >= dr_ls_info.get_member_list_cnt())) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("leader member list has no member", KR(ret), K(dr_ls_info));
  } else if (OB_FAIL(dr_ls_info.get_leader_and_member_list(leader_addr, member_list, learner_list))) {
    LOG_WARN("fail to get leader and member list", KR(ret), K(server_addr), K(dr_ls_info));
  } else if (member_list.contains(server_addr)) {
    replica_type = REPLICA_TYPE_FULL;
  } else if (learner_list.contains(server_addr)) {
    ObMember learner;
    if (OB_FAIL(learner_list.get_learner_by_addr(server_addr, learner))) {
      LOG_WARN("failed to get_learner_by_addr", KR(ret), K(server_addr));
    } else {
      if (learner.is_columnstore()) {
        replica_type = REPLICA_TYPE_COLUMNSTORE;
      } else {
        replica_type = REPLICA_TYPE_READONLY;
      }
    }
  } else {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("fail to find server in leader member list and learner list",
                KR(ret), K(server_addr), K(dr_ls_info), K(learner_list), K(member_list));
  }
  return ret;
}

int ObDRWorker::build_add_replica_task_(
    const obrpc::ObAdminAlterLSReplicaArg &arg,
    const DRLSInfo &dr_ls_info,
    ObAddLSReplicaTask &add_replica_task)
{
  int ret = OB_SUCCESS;
  share::ObUnit unit;
  share::ObLSReplica ls_replica;
  if (OB_UNLIKELY(!arg.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(arg));
  } else if (OB_FAIL(dr_ls_info.check_replica_exist_and_get_ls_replica(arg.get_server_addr(), ls_replica))) {
    LOG_WARN("fail to check and get replica by server", KR(ret), K(arg));
  } else if (ls_replica.is_valid()) {
    ret = OB_ENTRY_EXIST;
    LOG_USER_ERROR(OB_ENTRY_EXIST, "Target server already has a replica");
    LOG_WARN("server already has a replica of this type, cannot add more",
              KR(ret), K(arg), K(dr_ls_info), K(ls_replica));
  } else if (OB_FAIL(check_unit_exist_and_get_unit_(
              arg.get_server_addr(), arg.get_tenant_id(), false/*is_migrate_source_valid*/, unit))) {
    LOG_WARN("fail to check unit exist and get unit", KR(ret), K(arg));
  } else if (OB_FAIL(check_task_execute_server_status_(arg.get_server_addr(), true/*need_check_can_migrate_in*/))) {
    LOG_WARN("fail to check server status", KR(ret), K(arg), K(dr_ls_info));
  } else if (REPLICA_TYPE_FULL == arg.get_replica_type()
          && share::ObLSReplica::DEFAULT_REPLICA_COUNT == dr_ls_info.get_member_list_cnt()) {
    ret = OB_OP_NOT_ALLOW;
    LOG_USER_ERROR(OB_OP_NOT_ALLOW, "Member list count has reached the limit, alter ls replica is");
    LOG_WARN("number of F replica has reached the limit", KR(ret), K(arg), K(dr_ls_info));
  } else {
    ObDRTaskKey task_key;
    share::ObTaskId task_id;
    ObReplicaMember data_source;
    int64_t data_size = 0;
    ObReplicaMember force_data_source;
    int64_t new_paxos_replica_number = 0;
    ObReplicaMember dst_member(arg.get_server_addr(), ObTimeUtility::current_time(), arg.get_replica_type());
    if (FALSE_IT(task_id.init(GCONF.self_addr_))) {
    } else if (OB_FAIL(dr_ls_info.get_default_data_source(data_source, data_size))) {
      LOG_WARN("fail to get data_size", KR(ret), K(dr_ls_info));
    } else if (OB_FAIL(check_data_source_available_and_init_(arg, arg.get_replica_type(), dr_ls_info, force_data_source))) {
      LOG_WARN("fail to check and get data source", KR(ret), K(arg), K(dr_ls_info));
    } else if (OB_FAIL(check_and_generate_new_paxos_replica_num_(
                          arg, arg.get_replica_type(), dr_ls_info, new_paxos_replica_number))) {
      LOG_WARN("fail to check and generate new paxos replica num", KR(ret), K(arg), K(dr_ls_info));
    } else if (OB_FAIL(task_key.init(arg.get_tenant_id(), arg.get_ls_id(), unit.zone_, ObDRTaskType::LS_ADD_REPLICA))) {
      LOG_WARN("fail to init task key", KR(ret), K(arg), K(unit));
    } else if (OB_FAIL(add_replica_task.build(
                            task_key,
                            task_id,
                            ObString(drtask::ALTER_SYSTEM_COMMAND_ADD_REPLICA),
                            dst_member,
                            data_source,
                            force_data_source,
                            dr_ls_info.get_paxos_replica_number(),
                            new_paxos_replica_number,
                            ObDRTaskPriority::HIGH_PRI,
                            obrpc::ObAdminClearDRTaskArg::TaskType::MANUAL))) {
      LOG_WARN("fail to build add replica task", KR(ret), K(arg), K(task_key), K(task_id),
                K(dst_member), K(data_source), K(force_data_source), K(dr_ls_info), K(new_paxos_replica_number));
    }
  }
  return ret;
}

int ObDRWorker::build_remove_replica_task_(
    const obrpc::ObAdminAlterLSReplicaArg &arg,
    DRLSInfo &dr_ls_info,
    ObRemoveLSReplicaTask &remove_replica_task)
{
  int ret = OB_SUCCESS;
  common::ObReplicaType replica_type = REPLICA_TYPE_INVALID;
  common::ObAddr leader_addr;
  ObMember member_to_remove;
  if (OB_UNLIKELY(!arg.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(arg));
  } else if (OB_FAIL(dr_ls_info.get_member_by_server(arg.get_server_addr(), member_to_remove))) {
    if (OB_ENTRY_NOT_EXIST == ret) {
      LOG_USER_ERROR(OB_ENTRY_NOT_EXIST, "Target server does not have a replica of this LS");
    }
    LOG_WARN("server does not have a replica of this log stream", KR(ret), K(arg), K(dr_ls_info));
  } else if (OB_FAIL(dr_ls_info.get_leader(leader_addr))) {
    LOG_WARN("fail to get leader", KR(ret), K(dr_ls_info));
  } else if (OB_FAIL(check_task_execute_server_status_(leader_addr, false/*need_check_can_migrate_in*/))) {
    LOG_WARN("fail to check server status", KR(ret), K(leader_addr), K(dr_ls_info));
  } else if (OB_FAIL(get_replica_type_by_leader_(arg.get_server_addr(), dr_ls_info, replica_type))) {
    LOG_WARN("fail to get_replica_type_by_leader", KR(ret), K(dr_ls_info), K(arg));
  } else {
    share::ObTaskId task_id;
    int64_t new_paxos_replica_number = 0;
    bool has_leader = false;
    ObReplicaMember remove_member;
    ObDRTaskKey task_key;
    common::ObZone zone;
    ObDRTaskType task_type = ObReplicaTypeCheck::is_paxos_replica_V2(replica_type)
                           ? ObDRTaskType::LS_REMOVE_PAXOS_REPLICA : ObDRTaskType::LS_REMOVE_NON_PAXOS_REPLICA;
    if (FALSE_IT(task_id.init(GCONF.self_addr_))) {
    } else if (OB_FAIL(remove_member.init(member_to_remove, replica_type))) {
      LOG_WARN("fail to init remove_member", KR(ret), K(member_to_remove), K(replica_type));
    } else if (OB_FAIL(check_and_generate_new_paxos_replica_num_(
                          arg, replica_type, dr_ls_info, new_paxos_replica_number))) {
      LOG_WARN("fail to check and generate new paxos replica num", KR(ret), K(arg), K(replica_type), K(dr_ls_info));
    } else if (REPLICA_TYPE_FULL == replica_type
            && OB_FAIL(check_majority_for_remove_(arg.get_server_addr(), dr_ls_info, new_paxos_replica_number))) {
      LOG_WARN("check provided paxos_replica_num failed.", KR(ret),
                K(arg), K(dr_ls_info), K(new_paxos_replica_number));
    } else if (OB_FAIL(SVR_TRACER.get_server_zone(leader_addr, zone))) {
      LOG_WARN("get server zone failed", KR(ret), K(leader_addr));
    } else if (OB_FAIL(task_key.init(arg.get_tenant_id(), arg.get_ls_id(), zone, task_type))) {
      LOG_WARN("fail to init task key", KR(ret), K(arg), K(zone));
    } else if (OB_FAIL(remove_replica_task.build(
                            task_key,
                            task_id,
                            ObString(drtask::ALTER_SYSTEM_COMMAND_REMOVE_REPLICA),
                            leader_addr,
                            remove_member,
                            dr_ls_info.get_paxos_replica_number(),
                            new_paxos_replica_number,
                            replica_type,
                            ObDRTaskPriority::HIGH_PRI,
                            obrpc::ObAdminClearDRTaskArg::TaskType::MANUAL))) {
      LOG_WARN("fail to build task", KR(ret), K(arg), K(task_id), K(leader_addr), K(remove_member),
                K(dr_ls_info), K(new_paxos_replica_number), K(replica_type));
    }
  }
  return ret;
}

int ObDRWorker::build_modify_replica_type_task_(
    const obrpc::ObAdminAlterLSReplicaArg &arg,
    DRLSInfo &dr_ls_info,
    ObLSTypeTransformTask &modify_replica_task)
{
  int ret = OB_SUCCESS;
  share::ObUnit unit;
  share::ObLSReplica ls_replica;
  common::ObReplicaType replica_type = REPLICA_TYPE_INVALID;
  if (OB_UNLIKELY(!arg.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(arg));
  } else if (OB_FAIL(dr_ls_info.check_replica_exist_and_get_ls_replica(arg.get_server_addr(), ls_replica))) {
    LOG_WARN("fail to check and get replica by server", KR(ret), K(arg));
  } else if (!ls_replica.is_valid() || (ls_replica.is_valid() && !ls_replica.is_in_service())) {
    ret = OB_ENTRY_NOT_EXIST;
    LOG_USER_ERROR(OB_ENTRY_NOT_EXIST, "Target server does not have a replica of this LS");
    LOG_WARN("server does not have a replica of this log stream",
                KR(ret), K(arg), K(dr_ls_info), K(ls_replica));
  } else if (OB_FAIL(check_unit_exist_and_get_unit_(
              arg.get_server_addr(), arg.get_tenant_id(), true/*is_migrate_source_valid*/, unit))) {
    LOG_WARN("fail to check unit exist and get unit", KR(ret), K(arg));
  } else if (OB_FAIL(check_task_execute_server_status_(arg.get_server_addr(), false/*need_check_can_migrate_in*/))) {
    LOG_WARN("fail to check server status", KR(ret), K(arg), K(dr_ls_info));
  } else if (OB_FAIL(get_replica_type_by_leader_(arg.get_server_addr(), dr_ls_info, replica_type))) {
    LOG_WARN("fail to get_replica_type_by_leader", KR(ret), K(dr_ls_info), K(arg));
  } else if (arg.get_replica_type() == replica_type) {
    ret = OB_ENTRY_EXIST;
    LOG_USER_ERROR(OB_ENTRY_EXIST, "Current replica type is same as the target type, no need to modify");
    LOG_WARN("replica type is the same as the target type, no need to modify type",
                  KR(ret), K(arg), K(replica_type));
  } else if (ObReplicaTypeCheck::is_columnstore_replica(arg.get_replica_type())
             || ObReplicaTypeCheck::is_columnstore_replica(replica_type)) {
    ret = OB_NOT_SUPPORTED;
    LOG_USER_ERROR(OB_NOT_SUPPORTED, "Current or target replica-type is C-replica, type transform");
    LOG_WARN("Current or target replica_type is C-replica, type transform not supported",
                  KR(ret), K(arg), K(replica_type));
  } else if (REPLICA_TYPE_FULL == arg.get_replica_type()
          && share::ObLSReplica::DEFAULT_REPLICA_COUNT == dr_ls_info.get_member_list_cnt()) {
    ret = OB_OP_NOT_ALLOW;
    LOG_USER_ERROR(OB_OP_NOT_ALLOW, "Member list count has reached the limit, alter ls replica is");
    LOG_WARN("number of F replica has reached the limit", KR(ret), K(replica_type), K(dr_ls_info));
  } else {
    bool has_leader = false;
    int64_t new_paxos_replica_number = 0;
    share::ObTaskId task_id;
    int64_t data_size = 0;
    ObReplicaMember data_source;
    ObDRTaskKey task_key;
    ObReplicaMember src_member(ls_replica.get_server(),
                               ls_replica.get_member_time_us(),
                               replica_type,
                               ls_replica.get_memstore_percent());
    ObReplicaMember dst_member(ls_replica.get_server(),
                               ObTimeUtility::current_time(),
                               arg.get_replica_type(),
                               ls_replica.get_memstore_percent());
    if (FALSE_IT(task_id.init(GCONF.self_addr_))) {
    } else if (OB_FAIL(check_and_generate_new_paxos_replica_num_(
                        arg, arg.get_replica_type(), dr_ls_info, new_paxos_replica_number))) {
      LOG_WARN("fail to check and generate new paxos replica num", KR(ret), K(arg), K(dr_ls_info));
    } else if (REPLICA_TYPE_READONLY == arg.get_replica_type()
            && OB_FAIL(check_majority_for_remove_(arg.get_server_addr(), dr_ls_info, new_paxos_replica_number))) {
      LOG_WARN("check provided paxos_replica_num failed.", KR(ret),
                K(arg), K(dr_ls_info), K(new_paxos_replica_number));
    } else if (OB_FAIL(dr_ls_info.get_default_data_source(data_source, data_size))) {
      LOG_WARN("fail to get data_size", KR(ret), K(dr_ls_info));
    } else if (OB_FAIL(task_key.init(arg.get_tenant_id(), arg.get_ls_id(), ls_replica.get_zone(), ObDRTaskType::LS_TYPE_TRANSFORM))) {
      LOG_WARN("fail to init task key", KR(ret), K(arg), K(ls_replica));
    } else if (OB_FAIL(modify_replica_task.build(
                            task_key,
                            task_id,
                            ObString(drtask::ALTER_SYSTEM_COMMAND_MODIFY_REPLICA_TYPE),
                            dst_member,
                            src_member,
                            data_source,
                            dr_ls_info.get_paxos_replica_number(),
                            new_paxos_replica_number,
                            ObDRTaskPriority::HIGH_PRI,
                            obrpc::ObAdminClearDRTaskArg::TaskType::MANUAL))) {
      LOG_WARN("fail to build type transform task", KR(ret), K(arg), K(task_id), K(dst_member),
                K(src_member), K(data_source), K(dr_ls_info), K(new_paxos_replica_number));
    }
  }
  return ret;
}

int ObDRWorker::build_migrate_replica_task_(
    const obrpc::ObAdminAlterLSReplicaArg &arg,
    const DRLSInfo &dr_ls_info,
    ObMigrateLSReplicaTask &migrate_replica_task)
{
  int ret = OB_SUCCESS;
  share::ObUnit destination_unit;
  common::ObReplicaType replica_type = REPLICA_TYPE_INVALID;
  share::ObLSReplica dest_ls_replica;
  share::ObLSReplica source_ls_replica;
  if (OB_UNLIKELY(!arg.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(arg));
  } else if (OB_FAIL(dr_ls_info.check_replica_exist_and_get_ls_replica(
                      arg.get_server_addr(), source_ls_replica))) {
    LOG_WARN("fail to check and get replica by server", KR(ret), K(arg));
  } else if (!source_ls_replica.is_valid() || !source_ls_replica.is_in_service()) {
    ret = OB_ENTRY_NOT_EXIST;
    LOG_USER_ERROR(OB_ENTRY_NOT_EXIST, "Source server does not have a replica of this LS");
    LOG_WARN("source server does not have a replica of this LS",
                KR(ret), K(arg), K(dr_ls_info), K(source_ls_replica));
  } else if (OB_FAIL(dr_ls_info.check_replica_exist_and_get_ls_replica(
                      arg.get_destination_addr(), dest_ls_replica))) {
    LOG_WARN("fail to check and get replica by server", KR(ret), K(arg));
  } else if (dest_ls_replica.is_valid()) {
    ret = OB_ENTRY_EXIST;
    LOG_USER_ERROR(OB_ENTRY_EXIST, "The destination server already has a replica");
    LOG_WARN("target server already has a replica, no need migrate", KR(ret), K(arg), K(dr_ls_info));
  } else if (OB_FAIL(check_unit_exist_and_get_unit_(
      arg.get_destination_addr(), arg.get_tenant_id(), false/*is_migrate_source_valid*/, destination_unit))) {
    LOG_WARN("fail to check unit exist and get unit", KR(ret), K(arg));
  } else if (OB_FAIL(check_task_execute_server_status_(arg.get_destination_addr(), true/*need_check_can_migrate_in*/))) {
    LOG_WARN("fail to check server status", KR(ret), K(arg), K(dr_ls_info));
  } else if (OB_FAIL(get_replica_type_by_leader_(arg.get_server_addr(), dr_ls_info, replica_type))) {
    LOG_WARN("fail to get_replica_type_by_leader", KR(ret), K(dr_ls_info), K(arg));
  } else if (destination_unit.zone_ != source_ls_replica.get_zone()) {
    ret = OB_OP_NOT_ALLOW;
    LOG_USER_ERROR(OB_OP_NOT_ALLOW, "Migrate replica can only be in the same zone, current operation");
    LOG_WARN("migration replica can only be in the same zone", KR(ret),
              K(destination_unit), K(source_ls_replica.get_zone()));
  } else if (REPLICA_TYPE_FULL == replica_type
          && share::ObLSReplica::DEFAULT_REPLICA_COUNT == dr_ls_info.get_member_list_cnt()) {
    ret = OB_OP_NOT_ALLOW;
    LOG_USER_ERROR(OB_OP_NOT_ALLOW, "Member list count has reached the limit, alter ls replica is");
    LOG_WARN("number of F replica has reached the limit", KR(ret), K(replica_type), K(dr_ls_info));
  } else {
    share::ObTaskId task_id;
    ObReplicaMember data_source;
    int64_t data_size = 0;
    ObReplicaMember force_data_source;
    ObDRTaskKey task_key;
    ObReplicaMember src_member(source_ls_replica.get_server(),
                               source_ls_replica.get_member_time_us(),
                               replica_type,
                               source_ls_replica.get_memstore_percent());
    ObReplicaMember dst_member(arg.get_destination_addr(),
                               ObTimeUtility::current_time(),
                               replica_type,
                               source_ls_replica.get_memstore_percent());
    if (FALSE_IT(task_id.init(GCONF.self_addr_))) {
    } else if (OB_FAIL(check_data_source_available_and_init_(arg, replica_type, dr_ls_info, force_data_source))) {
      LOG_WARN("fail to check and get data source", KR(ret), K(arg), K(replica_type), K(dr_ls_info));
    } else if (OB_FAIL(dr_ls_info.get_default_data_source(data_source, data_size))) {
      LOG_WARN("fail to get data_size", KR(ret), K(dr_ls_info));
    } else if (OB_FAIL(task_key.init(arg.get_tenant_id(), arg.get_ls_id(), destination_unit.zone_, ObDRTaskType::LS_MIGRATE_REPLICA))) {
      LOG_WARN("fail to init task key", KR(ret), K(arg), K(destination_unit));
    } else if (OB_FAIL(migrate_replica_task.build(
                            task_key,
                            task_id,
                            ObString(drtask::ALTER_SYSTEM_COMMAND_MIGRATE_REPLICA),
                            dst_member,
                            src_member,
                            data_source,
                            force_data_source,
                            dr_ls_info.get_paxos_replica_number(),
                            ObDRTaskPriority::HIGH_PRI,
                            obrpc::ObAdminClearDRTaskArg::TaskType::MANUAL))) {
      LOG_WARN("fail to build migrate task", KR(ret), K(arg), K(task_id), K(dst_member),
                K(src_member), K(data_source), K(force_data_source), K(dr_ls_info));
    }
  }
  return ret;
}

int ObDRWorker::build_modify_paxos_replica_num_task_(
    const obrpc::ObAdminAlterLSReplicaArg &arg,
    DRLSInfo &dr_ls_info,
    ObLSModifyPaxosReplicaNumberTask &modify_paxos_replica_number_task)
{
  int ret = OB_SUCCESS;
  share::ObTaskId task_id;
  common::ObAddr leader_addr;
  GlobalLearnerList learner_list;
  common::ObMemberList member_list;
  ObDRTaskKey task_key;
  common::ObZone zone;
  if (OB_UNLIKELY(!arg.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(arg));
  } else if (FALSE_IT(task_id.init(GCONF.self_addr_))) {
  } else if (OB_FAIL(dr_ls_info.get_leader_and_member_list(leader_addr, member_list, learner_list))) {
    LOG_WARN("fail to get leader and member list", KR(ret), K(arg), K(dr_ls_info));
  } else if (dr_ls_info.get_paxos_replica_number() <= arg.get_paxos_replica_num()) {
    ret = OB_INVALID_ARGUMENT;
    LOG_USER_ERROR(OB_INVALID_ARGUMENT, "paxos_replica_num which should be less than current paxos_replica_num");
    LOG_WARN("paxos_replica_num invalid", KR(ret), K(arg), K(dr_ls_info));
  } else if (member_list.get_member_number() < majority(arg.get_paxos_replica_num())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_USER_ERROR(OB_INVALID_ARGUMENT, "paxos_replica_num which should be satisfy majority");
    LOG_WARN("number of replicas and paxos_replica_num do not satisfy majority",
              KR(ret), K(arg), K(dr_ls_info), K(member_list));
  } else if (member_list.get_member_number() > arg.get_paxos_replica_num()) {
    ret = OB_INVALID_ARGUMENT;
    LOG_USER_ERROR(OB_INVALID_ARGUMENT, "paxos_replica_num which should be greater or equal with member list count");
    LOG_WARN("member_list.get_member_number() > arg.get_paxos_replica_num()",
              KR(ret), K(arg), K(dr_ls_info), K(member_list));
  } else if (OB_FAIL(check_task_execute_server_status_(leader_addr, false/*need_check_can_migrate_in*/))) {
    LOG_WARN("fail to check server status", KR(ret), K(arg), K(dr_ls_info));
  } else if (OB_FAIL(SVR_TRACER.get_server_zone(leader_addr, zone))) {
    LOG_WARN("get server zone failed", KR(ret), K(leader_addr));
  } else if (OB_FAIL(task_key.init(arg.get_tenant_id(), arg.get_ls_id(), zone, ObDRTaskType::LS_MODIFY_PAXOS_REPLICA_NUMBER))) {
    LOG_WARN("fail to init task key", KR(ret), K(arg), K(zone));
  } else if (OB_FAIL(modify_paxos_replica_number_task.build(
                          task_key,
                          task_id,
                          ObString(drtask::ALTER_SYSTEM_COMMAND_MODIFY_PAXOS_REPLICA_NUM),
                          leader_addr,
                          dr_ls_info.get_paxos_replica_number(),
                          arg.get_paxos_replica_num(),
                          member_list,
                          ObDRTaskPriority::HIGH_PRI,
                          obrpc::ObAdminClearDRTaskArg::TaskType::MANUAL))) {
    LOG_WARN("fail to build a modify paxos replica number task",
              KR(ret), K(arg), K(task_id), K(leader_addr), K(dr_ls_info), K(member_list));
  }
  return ret;
}

// this func is only for basic checking.
// 1. if dest_replica_type is F/R replica, data_src can be same replica_type or F;
// 2. if dest_replica_type is C replica, data_src can be F/R/C.
bool can_as_data_source(const int32_t dest_replica_type, const int32_t src_replica_type)
{
  return  (dest_replica_type == src_replica_type
           || REPLICA_TYPE_FULL == src_replica_type
           || ObReplicaTypeCheck::is_columnstore_replica(dest_replica_type));
}

int ObDRWorker::check_data_source_available_and_init_(
    const obrpc::ObAdminAlterLSReplicaArg &arg,
    const common::ObReplicaType &replica_type,
    const DRLSInfo &dr_ls_info,
    ObReplicaMember &data_source)
{
  /*
  check if data_source is available, check the following conditions:
  1. replica exist and in service
  2. ls replica not restore failed
  3. F replica can be used as the data source of R replica and F replica,
     R replica can only used as the data source of R replica.
  4. server status is alive and not stopped
  */
  int ret = OB_SUCCESS;
  data_source.reset();
  share::ObLSReplica ls_replica;
  ObServerInfoInTable server_info;
  common::ObReplicaType provide_replica_type = REPLICA_TYPE_INVALID;
  if (!arg.get_data_source().is_valid()) {
    // passed
    LOG_INFO("data_source is not valid", KR(ret), K(arg));
  } else if (OB_UNLIKELY(!arg.is_valid()
          || OB_UNLIKELY(!ObReplicaTypeCheck::is_replica_type_valid(replica_type)))) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(arg), K(replica_type));
  } else {
    if (OB_FAIL(dr_ls_info.check_replica_exist_and_get_ls_replica(arg.get_data_source(), ls_replica))) {
      LOG_WARN("fail to get ls replica", KR(ret), K(dr_ls_info));
    } else if (!ls_replica.is_valid() || (ls_replica.is_valid() && !ls_replica.is_in_service())) {
      ret = OB_OP_NOT_ALLOW;
      LOG_USER_ERROR(OB_OP_NOT_ALLOW, "The data source server has no replica, which is");
      LOG_WARN("source server has no replica", KR(ret), K(arg), K(ls_replica));
    } else if (OB_FAIL(get_replica_type_by_leader_(arg.get_data_source(), dr_ls_info, provide_replica_type))) {
      LOG_WARN("get replica type by leader error", KR(ret), K(arg), K(dr_ls_info));
    } else if (ls_replica.get_restore_status().is_failed()) {
      ret = OB_OP_NOT_ALLOW;
      LOG_USER_ERROR(OB_OP_NOT_ALLOW, "Data source replica restore or clone failed, which is");
      LOG_WARN("ls replica restore failed", KR(ret), K(arg), K(ls_replica));
    } else if (!can_as_data_source(replica_type, provide_replica_type)) {
      ret = OB_OP_NOT_ALLOW;
      LOG_USER_ERROR(OB_OP_NOT_ALLOW, "R replica is not supported as the source of F replica, which is");
      LOG_WARN("provided data_source replica_type not supported", KR(ret), K(arg), K(replica_type), K(provide_replica_type));
    } else if (OB_FAIL(SVR_TRACER.get_server_info(arg.get_data_source(), server_info))) {
      LOG_WARN("fail to get server info", KR(ret), K(arg));
    } else if (!server_info.is_alive()) {
      ret = OB_OP_NOT_ALLOW;
      LOG_USER_ERROR(OB_OP_NOT_ALLOW, "The data source server is not alive, which is");
      LOG_WARN("data source server is not alive", KR(ret), K(arg), K(server_info));
    } else if (server_info.is_stopped()) {
      ret = OB_OP_NOT_ALLOW;
      LOG_USER_ERROR(OB_OP_NOT_ALLOW, "The data source server is stopped, which is");
      LOG_WARN("data source server is stopped", KR(ret), K(arg), K(server_info));
    } else {
      data_source = ObReplicaMember(ls_replica.get_server(),
                                    ls_replica.get_member_time_us(),
                                    provide_replica_type, // attention
                                    ls_replica.get_memstore_percent());
    }
  }
  return ret;
}

int ObDRWorker::check_for_alter_full_replica_(
    const int64_t member_list_count,
    const int64_t new_p)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(member_list_count <= 0 || new_p <= 0)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(member_list_count), K(new_p));
  } else if (member_list_count < majority(new_p)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_USER_ERROR(OB_INVALID_ARGUMENT, "paxos_replica_num which does not satisfy majority");
    LOG_WARN("paxos_replica_num is wrong", KR(ret), K(member_list_count), K(new_p));
  } else if (member_list_count > new_p) {
    ret = OB_INVALID_ARGUMENT;
    LOG_USER_ERROR(OB_INVALID_ARGUMENT, "paxos_replica_num which should be greater or equal with member list count");
    LOG_WARN("paxos_replica_num is wrong", KR(ret), K(member_list_count), K(new_p));
  }
  return ret;
}

int ObDRWorker::check_and_generate_new_paxos_replica_num_(
    const obrpc::ObAdminAlterLSReplicaArg &arg,
    const common::ObReplicaType &replica_type,
    const DRLSInfo &dr_ls_info,
    int64_t &new_p)
{
  /*
    If the user provides paxos_replica_num, use the user-provided paxos_replica_num,
    otherwise keep paxos_replica_num unchanged.
    The change range of paxos_replica_num is limited to 1.
    If the change is too large, an error will be reported to the user.
    Different member_change_types are generated based on the task type and the copy type of the operation.
    Special: modify R->F is equivalent to MEMBER_CHANGE_ADD, modify F->R is equivalent to MEMBER_CHANGE_SUB
  */
  int ret = OB_SUCCESS;
  int64_t curr_p = dr_ls_info.get_paxos_replica_number();
  int64_t provided_p = arg.get_paxos_replica_num();
  int64_t member_list_count = dr_ls_info.get_member_list_cnt();
  new_p = provided_p > 0 ? provided_p : curr_p;
  obrpc::ObAlterLSReplicaTaskType task_type = arg.get_alter_task_type();
  MemberChangeType member_change_type = MEMBER_CHANGE_NOP;
  if (OB_UNLIKELY(!arg.is_valid()
             || !ObReplicaTypeCheck::is_replica_type_valid(replica_type))) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(arg), K(replica_type));
  } else if (std::abs(new_p - curr_p) > 1) {
    ret = OB_INVALID_ARGUMENT;
    LOG_USER_ERROR(OB_INVALID_ARGUMENT, "paxos_replica_num which change cannot be greater than 1");
    LOG_WARN("paxos_replica_num is wrong", KR(ret), K(arg), K(new_p), K(curr_p));
  } else if (task_type.is_add_task()) {
    if (REPLICA_TYPE_FULL == replica_type) {
      member_change_type = MEMBER_CHANGE_ADD;
    } else if (ObReplicaTypeCheck::is_non_paxos_replica(replica_type)) {
      member_change_type = MEMBER_CHANGE_NOP;
    }
  } else if (task_type.is_remove_task()) {
    if (REPLICA_TYPE_FULL == replica_type) {
      member_change_type = MEMBER_CHANGE_SUB;
    } else if (ObReplicaTypeCheck::is_non_paxos_replica(replica_type)) {
      member_change_type = MEMBER_CHANGE_NOP;
    }
  } else if (task_type.is_modify_replica_task()) {
    if (REPLICA_TYPE_FULL == replica_type) {
      member_change_type = MEMBER_CHANGE_ADD;
    } else if (REPLICA_TYPE_READONLY == replica_type) {
      member_change_type = MEMBER_CHANGE_SUB;
    } else if (ObReplicaTypeCheck::is_columnstore_replica(replica_type)) {
      ret = OB_INVALID_ARGUMENT;
      LOG_WARN("invalid replica_type, do not support transforming to C-replica", KR(ret), K(arg));
    }
  } else {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("task type unexpected", KR(ret), K(arg), K(dr_ls_info), K(replica_type));
  }

  if (OB_FAIL(ret)) {
  } else if ((MEMBER_CHANGE_ADD == member_change_type)) {
    if (OB_FAIL(check_for_alter_full_replica_(member_list_count + 1, new_p))) {
      LOG_WARN("check failed", KR(ret), K(arg), K(replica_type), K(dr_ls_info));
    }
  } else if ((MEMBER_CHANGE_SUB == member_change_type)) {
    if (OB_FAIL(check_for_alter_full_replica_(member_list_count - 1, new_p))) {
      LOG_WARN("check failed", KR(ret), K(arg), K(replica_type), K(dr_ls_info));
    }
  } else if ((MEMBER_CHANGE_NOP == member_change_type)) {
    if (new_p != curr_p) {
      ret = OB_INVALID_ARGUMENT;
      LOG_USER_ERROR(OB_INVALID_ARGUMENT, "paxos_replica_num which should remain unchanged");
      LOG_WARN("paxos_replica_num is wrong", KR(ret), K(arg), K(curr_p), K(new_p));
    }
  } else {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("member change type unexpected", KR(ret), K(arg), K(dr_ls_info), K(member_change_type));
  }
  LOG_INFO("check and generate new paxos_replica_num over",
            KR(ret), K(arg), K(replica_type), K(new_p), K(curr_p), K(member_list_count));
  return ret;
}

int ObDRWorker::check_majority_for_remove_(
    const common::ObAddr& server_addr,
    const DRLSInfo &dr_ls_info,
    const int64_t new_p)
{
  int ret = OB_SUCCESS;
  int64_t inactive_count = 0;
  int64_t arb_replica_number = 0;
  ObLSID ls_id;
  uint64_t tenant_id = OB_INVALID_TENANT_ID;
  if (OB_UNLIKELY(!server_addr.is_valid()) || OB_UNLIKELY(new_p <= 0)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(server_addr), K(new_p));
  } else if (OB_FAIL(dr_ls_info.get_ls_id(tenant_id, ls_id))) {
    LOG_WARN("fail to get tenant and ls id", KR(ret), K(dr_ls_info));
  } else if (OB_FAIL(ObShareUtil::generate_arb_replica_num(
                      tenant_id, ls_id, arb_replica_number))) {
    LOG_WARN("fail to generate arb replica number", KR(ret), K(tenant_id), K(ls_id));
  } else if (OB_FAIL(check_other_inactive_server_count_(
                            server_addr, dr_ls_info, inactive_count))) {
    LOG_WARN("fail to check other permanent offline server",
              KR(ret), K(server_addr), K(dr_ls_info));
  } else if ((0 == dr_ls_info.get_member_list_cnt() - 1 - inactive_count) // no member
          || (dr_ls_info.get_member_list_cnt() - 1 - inactive_count + arb_replica_number < majority(new_p))) {
    ret = OB_OP_NOT_ALLOW;
    LOG_USER_ERROR(OB_OP_NOT_ALLOW, "Current operation may result in no leader, while is");
    LOG_WARN("not satisfy majority", KR(ret), K(new_p), K(arb_replica_number), K(inactive_count));
  }
  LOG_INFO("check majority for remove over", KR(ret), K(server_addr), K(new_p), K(arb_replica_number));
  return ret;
}

int ObDRWorker::check_other_inactive_server_count_(
    const common::ObAddr& desti_server_addr,
    const DRLSInfo &dr_ls_info,
    int64_t& other_inactive_server_count)
{
  /*
  When removing replicas, need to judge the majority and consider the number of replicas
  that have been inactive but are still in the member list. example: current paxos_replica_num = 3,
  member_list_count = 3 (a b c), a is down but not permanently offline yet, now a remove operation
  want remove another normal replica b, set paxos_replica_num = 2, at this time,
  there is only one available replica of the underlying layer. which may result in no leader.
  */
  int ret = OB_SUCCESS;
  bool active = false;
  common::ObAddr leader_addr; // not used
  GlobalLearnerList learner_list; // not used
  common::ObMemberList member_list;
  other_inactive_server_count = 0;
  if (OB_UNLIKELY(!desti_server_addr.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(desti_server_addr));
  } else if (OB_FAIL(dr_ls_info.get_leader_and_member_list(leader_addr, member_list, learner_list))) {
    LOG_WARN("fail to get leader and member list", KR(ret), K(desti_server_addr), K(dr_ls_info));
  } else {
    for (int64_t index = 0; OB_SUCC(ret) && index < member_list.get_member_number(); ++index) {
      ObMember member;
      active = false;
      if (OB_FAIL(member_list.get_member_by_index(index, member))) {
        LOG_WARN("fail to get member", KR(ret), K(index), K(dr_ls_info));
      } else if (OB_FAIL(SVR_TRACER.check_server_active(member.get_server(), active))) {
        LOG_WARN("fail to check server permanent offline", KR(ret), K(member.get_server()));
      } else if (!active && desti_server_addr != member.get_server()) {
        other_inactive_server_count = other_inactive_server_count + 1;
        // other_inactive_server_count not include desti_server_addr
      }
    }
  }
  LOG_INFO("check other_inactive_server_count over", KR(ret), K(dr_ls_info),
          K(desti_server_addr), K(other_inactive_server_count));
  return ret;
}

int ObDRWorker::generate_task_key(
    const DRLSInfo &dr_ls_info,
    const common::ObAddr &task_exe_server,
    const obrpc::ObDRTaskType &task_type,
    ObDRTaskKey &task_key) const
{
  int ret = OB_SUCCESS;
  uint64_t tenant_id = OB_INVALID_TENANT_ID;
  ObLSID ls_id;
  common::ObZone zone;
  task_key.reset();
  if (OB_UNLIKELY(ObDRTaskType::MAX_TYPE == task_type || !task_exe_server.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(task_type), K(task_exe_server));
  } else if (OB_FAIL(dr_ls_info.get_ls_id(tenant_id, ls_id))) {
    LOG_WARN("fail to get log stream id", KR(ret), K(dr_ls_info));
  } else if (OB_FAIL(SVR_TRACER.get_server_zone(task_exe_server, zone))) {
    LOG_WARN("get server zone failed", KR(ret), K(task_exe_server));
  } else if (OB_FAIL(task_key.init(tenant_id, ls_id, zone, task_type))) {
    LOG_WARN("fail to init task key", KR(ret), K(tenant_id), K(ls_id), K(zone), K(task_type));
  }
  return ret;
}

int ObDRWorker::check_has_leader_while_remove_replica(
    const common::ObAddr &server,
    DRLSInfo &dr_ls_info,
    bool &has_leader)
{
  int ret = OB_SUCCESS;
  int64_t replica_cnt = 0;
  if (OB_UNLIKELY(!server.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(server));
  } else if (OB_FAIL(dr_ls_info.get_replica_cnt(replica_cnt))) {
    LOG_WARN("fail to get replica cnt", KR(ret), K(dr_ls_info));
  } else {
    int64_t full_replica_count = 0;
    int64_t arb_replica_num = 0;
    uint64_t tenant_id = OB_INVALID_TENANT_ID;
    ObLSID ls_id;
    ObReplicaType replica_type = REPLICA_TYPE_INVALID;
    for (int64_t index = 0; OB_SUCC(ret) && index < replica_cnt; ++index) {
      share::ObLSReplica *ls_replica = nullptr;
      DRServerStatInfo *server_stat_info = nullptr;
      DRUnitStatInfo *unit_stat_info = nullptr;
      DRUnitStatInfo *unit_in_group_stat_info = nullptr;
      if (OB_FAIL(dr_ls_info.get_replica_stat(
              index,
              ls_replica,
              server_stat_info,
              unit_stat_info,
              unit_in_group_stat_info))) {
        LOG_WARN("fail to get replica stat", KR(ret));
      } else if (OB_ISNULL(ls_replica)
                 || OB_ISNULL(server_stat_info)
                 || OB_ISNULL(unit_stat_info)
                 || OB_ISNULL(unit_in_group_stat_info)) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("replica stat unexpected", KR(ret),
                 KP(ls_replica),
                 KP(server_stat_info),
                 KP(unit_stat_info),
                 KP(unit_in_group_stat_info));
      } else if (REPLICA_STATUS_NORMAL != ls_replica->get_replica_status()) {
        // bypass
      } else {
        if (server_stat_info->get_server() == server) {
          replica_type = ls_replica->get_replica_type();
        }
        if (REPLICA_TYPE_FULL == ls_replica->get_replica_type()) {
          ++full_replica_count;
        }
      }
    }
    if (OB_FAIL(ret)) {
      // failed
    } else if (OB_FAIL(dr_ls_info.get_ls_id(tenant_id, ls_id))) {
      LOG_WARN("fail to get tenant and ls id", KR(ret), K(dr_ls_info));
    } else if (OB_FAIL(ObShareUtil::generate_arb_replica_num(
                           tenant_id,
                           ls_id,
                           arb_replica_num))) {
      LOG_WARN("fail to generate arb replica number", KR(ret), K(tenant_id), K(ls_id));
    } else if (!dr_ls_info.has_leader()) {
      has_leader = false;
    } else if (!ObReplicaTypeCheck::is_paxos_replica_V2(replica_type)) {
      has_leader = true;
    } else if (1 == dr_ls_info.get_paxos_replica_number()) {
      // member_list count should be always less than paxos_replica_number
      // so member_list should have only one member (when 1 == paxos_replica_number)
      // we can not remove the only member
      has_leader = false;
    } else if (2 == dr_ls_info.get_paxos_replica_number()) {
      // member_list count should be always less than paxos_replica_number
      // although member_list count can be less than paxos_replica_number
      // member_list count can not be 1, because:
      //   1. 2F can not reduce to 1F with paxos_replica_number = 2
      //   2. 2F1A permanent offline 1F then paxos_replica_number will change from 2 to 1
      // so member_list should have 2 members (when 2 == paxos_replica_number)
      if (2 >= dr_ls_info.get_schema_replica_cnt()) {
        // if 2 == schema_replica_count, it means locality has 2F, we specialy support 2F to 1F without locality changes
        // if 2 > schema_replica_count, it means locality has 1F, but migration leads to 2F, we have to remove 1F
        has_leader = true;
      } else {
        // if 2 != schema_replica_count, it means locality is changing, we can not reduce 2F to 1F
        // Consider this case:
        //     tenant's initial locality is 3F(z1,z2,z3)
        //     member_list is reduced to 2F(z1,z2) paxos_replica_number reduced to 2 by using "alter system remove replica" command
        //     tenant locality is changing from 3F(z1,z2,z3) to 5F(z1,z2,z3,z4,z5), current member_list is (z1,z2) and paxos_replica_number = 2
        //     before add z3 replica, server in z2 is permanent offline, we do not want to remove z2 replica in this case
        has_leader = false;
      }
    } else {
      // we do not reduce member_list count less than majority of schema replica count
      // consider this case:
      // tenant's locality is changing from 3F to 4F
      // before add replica in z4, replica in z3 is permanent offline
      // if we remove replica in z3 first, member_list will have only 2 members, and
      // schema is 4F requiring majority is 3 members which is not good
      // So we prohibit removing replica in this case
      has_leader = (full_replica_count - 1 + arb_replica_num) >= majority(dr_ls_info.get_schema_replica_cnt());
    }
  }
  return ret;
}

int ObDRWorker::check_can_generate_task(
    const int64_t acc_dr_task,
    const bool need_check_has_leader_while_remove_replica,
    const ObAddr &server_addr,
    DRLSInfo &dr_ls_info,
    const obrpc::ObDRTaskType &task_type,
    bool &can_generate)
{
  int ret = OB_SUCCESS;
  bool has_leader_while_remove_replica = false;
  can_generate = true;
  if (ObDRTaskType::LS_MIGRATE_REPLICA != task_type && acc_dr_task != 0) {
    // if a disaster recovery task is generated in this round, or a task is currently executing in the mgr queue
    // and a conflict is detected, acc_dr_task++ will be used to prohibit subsequent task generation.
    // in order to implement parallel migration, when the task of the first replica of LS is generated or a conflict
    // in the migration of the first replica is detected, it is necessary to continue to generate the migration task of the next replica.
    can_generate = false;
    LOG_INFO("can not generate task because another task is generated", K(dr_ls_info), K(task_type), K(acc_dr_task));
  } else if (need_check_has_leader_while_remove_replica) {
    if (OB_FAIL(check_has_leader_while_remove_replica(
                         server_addr,
                         dr_ls_info,
                         has_leader_while_remove_replica))) {
      LOG_WARN("fail to check has leader while member change", KR(ret), K(dr_ls_info), K(server_addr));
    } else if (!has_leader_while_remove_replica) {
      can_generate = false;
      LOG_INFO("can not generate task because has no leader", K(dr_ls_info), K(task_type), K(acc_dr_task), K(has_leader_while_remove_replica));
    }
  }
  return ret;
}

int ObDRWorker::construct_extra_infos_to_build_remove_replica_task(
    const DRLSInfo &dr_ls_info,
    share::ObTaskId &task_id,
    int64_t &new_paxos_replica_number,
    int64_t &old_paxos_replica_number,
    common::ObAddr &leader_addr,
    const ObReplicaType &replica_type)
{
  int ret = OB_SUCCESS;
  bool found_new_paxos_replica_number = false;
  bool is_paxos_replica = ObReplicaTypeCheck::is_paxos_replica_V2(replica_type);
  if (FALSE_IT(task_id.init(GCONF.self_addr_))) {
  } else if (is_paxos_replica
             && OB_FAIL(generate_disaster_recovery_paxos_replica_number(
                         dr_ls_info,
                         dr_ls_info.get_paxos_replica_number(),
                         dr_ls_info.get_schema_replica_cnt(),
                         MEMBER_CHANGE_SUB,
                         new_paxos_replica_number,
                         found_new_paxos_replica_number))) {
    LOG_WARN("fail to generate disaster recovery paxos_replica_number", KR(ret), K(found_new_paxos_replica_number));
  } else if (is_paxos_replica && !found_new_paxos_replica_number) {
    LOG_WARN("paxos_replica_number not found", K(dr_ls_info));
  } else if (OB_FAIL(dr_ls_info.get_leader(leader_addr))) {
    LOG_WARN("fail to get leader", KR(ret));
  } else {
    old_paxos_replica_number = dr_ls_info.get_paxos_replica_number();
    new_paxos_replica_number = is_paxos_replica ? new_paxos_replica_number : old_paxos_replica_number;
  }
  return ret;
}

int ObDRWorker::generate_remove_permanent_offline_replicas_task(
    const ObDRTaskKey task_key,
    const share::ObTaskId &task_id,
    const common::ObAddr &leader_addr,
    const ObReplicaMember &remove_member,
    const int64_t &old_paxos_replica_number,
    const int64_t &new_paxos_replica_number,
    int64_t &acc_dr_task,
    const ObReplicaType &replica_type)
{
  int ret = OB_SUCCESS;
  ObRemoveLSReplicaTask remove_replica_task;
  if (OB_FAIL(remove_replica_task.build(
                task_key,
                task_id,
                ObString(drtask::REMOVE_PERMANENT_OFFLINE_REPLICA),
                leader_addr,
                remove_member,
                old_paxos_replica_number,
                new_paxos_replica_number,
                replica_type))) {
    LOG_WARN("fail to build remove member task", KR(ret), K(task_key), K(task_id), K(leader_addr),
             K(remove_member), K(old_paxos_replica_number), K(new_paxos_replica_number), K(replica_type));
  } else if (OB_FAIL(add_task_(remove_replica_task, acc_dr_task))) {
    LOG_WARN("fail to add task", KR(ret), K(remove_replica_task));
  }
  return ret;
}

int ObDRWorker::try_remove_permanent_offline_replicas(
    const bool only_for_display,
    DRLSInfo &dr_ls_info,
    int64_t &acc_dr_task)
{
  int ret = OB_SUCCESS;
  common::ObAddr leader;
  common::ObMemberList member_list;
  GlobalLearnerList learner_list;
  uint64_t tenant_id = OB_INVALID_TENANT_ID;
  share::ObLSID ls_id;
  if (!dr_ls_info.has_leader()) {
    LOG_WARN("has no leader, maybe not report yet", KR(ret), K(dr_ls_info));
  } else if (dr_ls_info.get_paxos_replica_number() <= 0) {
    LOG_WARN("paxos_replica_number is invalid, maybe not report yet", KR(ret), K(dr_ls_info));
  } else if (OB_FAIL(dr_ls_info.get_leader_and_member_list(leader, member_list, learner_list))) {
    LOG_WARN("fail to get leader and member list", KR(ret), K(dr_ls_info));
  } else if (OB_UNLIKELY(0 >= member_list.get_member_number())) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("leader member list is unexpected", KR(ret), K(dr_ls_info), K(leader), K(member_list));
  } else if (OB_FAIL(dr_ls_info.get_ls_id(tenant_id, ls_id))) {
    LOG_WARN("fail to get ls id", KR(ret), K(tenant_id), K(ls_id), K(dr_ls_info));
  } else {
    for (int64_t index = 0; OB_SUCC(ret) && index < member_list.get_member_number(); ++index) {
      ObMember member_to_remove;
      common::ObReplicaType replica_type = REPLICA_TYPE_FULL;
      if (OB_FAIL(member_list.get_member_by_index(index, member_to_remove))) {
          LOG_WARN("fail to get member by index", KR(ret), K(index), K(member_list));
      } else if (OB_FAIL(do_single_replica_permanent_offline_(
                             tenant_id,
                             ls_id,
                             dr_ls_info,
                             only_for_display,
                             replica_type,
                             member_to_remove,
                             acc_dr_task))) {
        LOG_WARN("fail to do single replica permanent offline task", KR(ret), K(tenant_id), K(ls_id),
                 K(dr_ls_info), K(only_for_display), K(replica_type), K(member_to_remove), K(acc_dr_task));
      }
    }
    // try generate permanent offline task for readonly replicas
    for (int64_t index = 0; OB_SUCC(ret) && index < learner_list.get_member_number(); ++index) {
      ObMember learner_to_remove;
      common::ObReplicaType replica_type = REPLICA_TYPE_READONLY;
      if (OB_FAIL(learner_list.get_member_by_index(index, learner_to_remove))) {
        LOG_WARN("fail to get learner by index", KR(ret), K(index));
      } else if (FALSE_IT(replica_type = learner_to_remove.is_columnstore() ?
                            REPLICA_TYPE_COLUMNSTORE : REPLICA_TYPE_READONLY)) {
        // shall never be here
      } else if (OB_FAIL(do_single_replica_permanent_offline_(
                             tenant_id,
                             ls_id,
                             dr_ls_info,
                             only_for_display,
                             replica_type,
                             learner_to_remove,
                             acc_dr_task))) {
        LOG_WARN("fail to do single replica permanent offline task for non-paxos replica", KR(ret), K(tenant_id),
                 K(ls_id), K(dr_ls_info), K(only_for_display), K(replica_type), K(learner_to_remove), K(acc_dr_task));
      }
    }
  }
  FLOG_INFO("finish try remove permanent offline replica", KR(ret), K(tenant_id), K(ls_id), K(acc_dr_task));
  return ret;
}

int ObDRWorker::do_single_replica_permanent_offline_(
    const uint64_t tenant_id,
    const share::ObLSID &ls_id,
    DRLSInfo &dr_ls_info,
    const bool only_for_display,
    const ObReplicaType &replica_type,
    const ObMember &member_to_remove,
    int64_t &acc_dr_task)
{
  int ret = OB_SUCCESS;
  bool is_offline = false;
  ObServerInfoInTable server_info;
  if (OB_UNLIKELY(!member_to_remove.is_valid()
               || OB_INVALID_TENANT_ID == tenant_id
               || !ls_id.is_valid_with_tenant(tenant_id))) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(member_to_remove), K(tenant_id), K(ls_id));
  } else if (OB_ISNULL(GCTX.sql_proxy_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("sql_proxy_ is null", KR(ret), KP(GCTX.sql_proxy_));
  } else if (OB_FAIL(ObServerTableOperator::get(*GCTX.sql_proxy_, member_to_remove.get_server(), server_info))) {
    // remove the member not in __all_server table
    if (OB_SERVER_NOT_IN_WHITE_LIST == ret) {
      is_offline = true;
      ret = OB_SUCCESS;
      LOG_INFO("found not in __all_server replica", K(tenant_id), K(ls_id), K(member_to_remove));
    } else {
      LOG_WARN("fail to get server info", KR(ret), K(member_to_remove));
    }
  } else {
    is_offline = server_info.is_permanent_offline();
  }
  if (OB_FAIL(ret)) {
  } else if (is_offline) {
    FLOG_INFO("found ls replica need to permanent offline", K(tenant_id), K(ls_id), K(member_to_remove), K(replica_type), K(dr_ls_info));
    share::ObTaskId task_id;
    int64_t new_paxos_replica_number = 0;
    int64_t old_paxos_replica_number = 0;
    common::ObAddr leader_addr;
    const common::ObAddr source_server; // not useful
    const bool need_check_has_leader_while_remove_replica = ObReplicaTypeCheck::is_paxos_replica_V2(replica_type);
    const int64_t memstore_percent = 100;
    ObDRTaskKey task_key;
    bool can_generate = false;
    ObReplicaMember remove_member;
    ObDRTaskType task_type = ObReplicaTypeCheck::is_paxos_replica_V2(replica_type)
                               ? ObDRTaskType::LS_REMOVE_PAXOS_REPLICA
                               : ObDRTaskType::LS_REMOVE_NON_PAXOS_REPLICA;
    if (OB_FAIL(remove_member.init(member_to_remove, replica_type))) {
      LOG_WARN("failed to init remove_member", KR(ret), K(member_to_remove), K(replica_type));
    } else if (OB_FAIL(construct_extra_infos_to_build_remove_replica_task(
                    dr_ls_info,
                    task_id,
                    new_paxos_replica_number,
                    old_paxos_replica_number,
                    leader_addr,
                    replica_type))) {
      LOG_WARN("fail to construct extra infos to build remove replica task",
               KR(ret), K(dr_ls_info), K(task_id), K(new_paxos_replica_number),
               K(old_paxos_replica_number), K(leader_addr), K(replica_type));
    } else if (only_for_display) {
      // only for display, no need to execute this task
      ObLSReplicaTaskDisplayInfo display_info;
      if (OB_FAIL(display_info.init(
                      tenant_id,
                      ls_id,
                      task_type,
                      ObDRTaskPriority::HIGH_PRI,
                      member_to_remove.get_server(),
                      replica_type,
                      new_paxos_replica_number,
                      source_server,
                      REPLICA_TYPE_INVALID/*source_replica_type*/,
                      old_paxos_replica_number,
                      leader_addr,
                      "remove permanent offline replica"))) {
        LOG_WARN("fail to init a ObLSReplicaTaskDisplayInfo", KR(ret), K(tenant_id), K(ls_id),
                 K(task_type), K(member_to_remove), K(replica_type), K(new_paxos_replica_number),
                 K(old_paxos_replica_number), K(leader_addr));
      } else if (OB_FAIL(add_display_info(display_info))) {
        LOG_WARN("fail to add display info", KR(ret), K(display_info));
      } else {
        LOG_INFO("success to add display info", KR(ret), K(display_info));
      }
    } else if (OB_FAIL(generate_task_key(dr_ls_info, leader_addr, task_type, task_key))) {
      LOG_WARN("fail to generate task key", KR(ret), K(dr_ls_info), K(leader_addr), K(task_type));
    } else if (OB_FAIL(check_can_generate_task(
                           acc_dr_task,
                           need_check_has_leader_while_remove_replica,
                           member_to_remove.get_server(),
                           dr_ls_info,
                           task_type,
                           can_generate))) {
      LOG_WARN("fail to check can generate remove permanent offline task", KR(ret), K(acc_dr_task),
               K(need_check_has_leader_while_remove_replica), K(member_to_remove),
               K(dr_ls_info), K(can_generate));
    } else if (can_generate) {
      if (OB_FAIL(generate_remove_permanent_offline_replicas_task(
                          task_key,
                          task_id,
                          leader_addr,
                          remove_member,
                          old_paxos_replica_number,
                          new_paxos_replica_number,
                          acc_dr_task,
                          replica_type))) {
        LOG_WARN("fail to generate remove permanent offline task", KR(ret), K(leader_addr),
                 K(remove_member), K(old_paxos_replica_number), K(new_paxos_replica_number), K(replica_type));
      }
    }
  }
  return ret;
}

int ObDRWorker::check_need_generate_replicate_to_unit(
    const int64_t index,
    DRLSInfo &dr_ls_info,
    share::ObLSReplica *&ls_replica,
    DRServerStatInfo *&server_stat_info,
    DRUnitStatInfo *&unit_stat_info,
    DRUnitStatInfo *&unit_in_group_stat_info,
    bool &need_generate)
{
  int ret = OB_SUCCESS;
  need_generate = false;
  if (OB_FAIL(dr_ls_info.get_replica_stat(
              index,
              ls_replica,
              server_stat_info,
              unit_stat_info,
              unit_in_group_stat_info))) {
    LOG_WARN("fail to get replica stat", KR(ret));
  } else if (OB_ISNULL(ls_replica)
                 || OB_ISNULL(server_stat_info)
                 || OB_ISNULL(unit_stat_info)
                 || OB_ISNULL(unit_in_group_stat_info)
                 || (unit_stat_info->is_in_pool()
                     && OB_ISNULL(unit_stat_info->get_server_stat()))) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("replica stat unexpected", KR(ret),
                 KP(ls_replica),
                 KP(server_stat_info),
                 KP(unit_stat_info),
                 KP(unit_in_group_stat_info));
  } else if (REPLICA_STATUS_NORMAL == ls_replica->get_replica_status()
             && unit_stat_info->is_in_pool()
             && !server_stat_info->is_alive()
             && unit_stat_info->get_unit().server_ != server_stat_info->get_server()
             && unit_stat_info->get_server_stat()->is_alive()
             && !unit_stat_info->get_server_stat()->is_block()) {
    need_generate = true;
    LOG_INFO("found replicate to unit replica", KPC(ls_replica));
  }
  return ret;
}

int ObDRWorker::construct_extra_infos_to_build_migrate_task(
    DRLSInfo &dr_ls_info,
    const share::ObLSReplica &ls_replica,
    uint64_t &tenant_id,
    share::ObLSID &ls_id,
    share::ObTaskId &task_id,
    int64_t &data_size,
    int64_t &old_paxos_replica_number)
{
  int ret = OB_SUCCESS;
  if (FALSE_IT(task_id.init(GCONF.self_addr_))) {
    //shall never be here
  } else {
    tenant_id = ls_replica.get_tenant_id();
    ls_id = ls_replica.get_ls_id();
    old_paxos_replica_number = dr_ls_info.get_paxos_replica_number();
  }
  return ret;
}

int ObDRWorker::generate_replicate_to_unit_task(
    const ObDRTaskKey task_key,
    const share::ObTaskId &task_id,
    const ObReplicaMember &dst_member,
    const ObReplicaMember &src_member,
    const ObReplicaMember &data_source,
    const int64_t &old_paxos_replica_number,
    const char* task_comment,
    int64_t &acc_dr_task)
{
  int ret = OB_SUCCESS;
  ObMigrateLSReplicaTask migrate_task;
  if (OB_ISNULL(task_comment)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("task comment is null", KR(ret));
  } else if (OB_FAIL(migrate_task.build(
                         task_key,
                         task_id,
                         task_comment,
                         dst_member,
                         src_member,
                         data_source,
                         ObReplicaMember()/*empty force_data_source*/,
                         old_paxos_replica_number))) {
    LOG_WARN("fail to build migrate task", KR(ret));
  } else if (OB_FAIL(add_task_(migrate_task, acc_dr_task))) {
    LOG_WARN("fail to add task", KR(ret), K(migrate_task));
  }
  return ret;
}

int ObDRWorker::try_replicate_to_unit(
    const bool only_for_display,
    DRLSInfo &dr_ls_info,
    int64_t &acc_dr_task)
{
  int ret = OB_SUCCESS;

  ObDRTaskKey task_key;
  bool task_exist = false;
  int64_t replica_cnt = 0;
  if (!dr_ls_info.has_leader()) {
    LOG_WARN("has no leader, maybe not report yet", KR(ret), K(dr_ls_info));
  } else if (dr_ls_info.get_paxos_replica_number() <= 0) {
    LOG_WARN("paxos_replica_number is invalid, maybe not report yet", KR(ret), K(dr_ls_info));
  } else if (OB_FAIL(dr_ls_info.get_replica_cnt(replica_cnt))) {
    LOG_WARN("fail to get replica cnt", KR(ret), K(dr_ls_info));
  } else {
    for (int64_t index = 0; OB_SUCC(ret) && index < replica_cnt; ++index) {
      bool need_generate = false;
      share::ObLSReplica *ls_replica = nullptr;
      DRServerStatInfo *server_stat_info = nullptr;
      DRUnitStatInfo *unit_stat_info = nullptr;
      DRUnitStatInfo *unit_in_group_stat_info = nullptr;
      if (OB_FAIL(check_need_generate_replicate_to_unit(
                      index,
                      dr_ls_info,
                      ls_replica,
                      server_stat_info,
                      unit_stat_info,
                      unit_in_group_stat_info,
                      need_generate))) {
        LOG_WARN("fail to check need generate replicate to unit task", KR(ret));
      } else if (need_generate) {
        ObReplicaMember dst_member(unit_stat_info->get_unit().server_,
                                   ObTimeUtility::current_time(),
                                   ls_replica->get_replica_type(),
                                   ls_replica->get_memstore_percent());
        if (OB_FAIL(generate_migrate_ls_task(
                only_for_display, drtask::REPLICATE_REPLICA, *ls_replica,
                *server_stat_info, *unit_stat_info, *unit_in_group_stat_info,
                dst_member, dr_ls_info, acc_dr_task))) {
          LOG_WARN("failed to generate migrate ls task", KR(ret), K(dst_member),
                   K(only_for_display), KPC(ls_replica));
        }
      }
    }
  }
  LOG_INFO("finish try replicate to unit", KR(ret), K(acc_dr_task));
  return ret;
}

int ObDRWorker::generate_migrate_ls_task(
      const bool only_for_display,
      const char* task_comment,
      const share::ObLSReplica &ls_replica,
      const DRServerStatInfo &server_stat_info,
      const DRUnitStatInfo &unit_stat_info,
      const DRUnitStatInfo &unit_in_group_stat_info,
      const ObReplicaMember &dst_member,
      DRLSInfo &dr_ls_info,
      int64_t &acc_dr_task)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!ls_replica.is_valid() || !dst_member.is_valid())
      || OB_ISNULL(task_comment)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), KP(task_comment), K(ls_replica));
  } else {
    ObDRTaskKey task_key;
    bool task_exist = false;
    uint64_t tenant_id = 0;
    share::ObLSID ls_id;
    share::ObTaskId task_id;
    ObReplicaMember data_source;
    int64_t data_size = 0;
    int64_t old_paxos_replica_number = 0;
    const bool need_check_has_leader_while_remove_replica = false;
    bool can_generate = false;
    ObReplicaMember src_member(
        ls_replica.get_server(), ls_replica.get_member_time_us(),
        ls_replica.get_replica_type(), ls_replica.get_memstore_percent());
    if (OB_FAIL(dr_ls_info.get_default_data_source(data_source, data_size))) {
      LOG_WARN("fail to get data_size", KR(ret), K(dr_ls_info));
    } else if (OB_FAIL(construct_extra_infos_to_build_migrate_task(
                          dr_ls_info, ls_replica, tenant_id, ls_id, task_id,
                          data_size, old_paxos_replica_number))) {
      LOG_WARN("fail to construct extra infos to build migrate task", KR(ret));
    } else if (only_for_display) {
      ObLSReplicaTaskDisplayInfo display_info;
      if (OB_FAIL(display_info.init(
              tenant_id, ls_id, ObDRTaskType::LS_MIGRATE_REPLICA,
              ObDRTaskPriority::HIGH_PRI,
              unit_stat_info.get_unit().server_,
              ls_replica.get_replica_type(), old_paxos_replica_number,
              ls_replica.get_server(), ls_replica.get_replica_type(),
              old_paxos_replica_number,
              unit_stat_info.get_unit().server_,
              task_comment))) {
        LOG_WARN("fail to init a ObLSReplicaTaskDisplayInfo", KR(ret));
      } else if (OB_FAIL(add_display_info(display_info))) {
        LOG_WARN("fail to add display info", KR(ret), K(display_info));
      } else {
        LOG_INFO("success to add display info", KR(ret), K(display_info));
      }
    } else if (OB_FAIL(generate_task_key(dr_ls_info, dst_member.get_server(), ObDRTaskType::LS_MIGRATE_REPLICA, task_key))) {
      LOG_WARN("fail to generate task key", KR(ret), K(dr_ls_info), K(dst_member));
    } else if (OB_FAIL(check_can_generate_task(
                        /*when shrink unit num, both dup log stream and normal log stream are here*/
                            acc_dr_task,
                            need_check_has_leader_while_remove_replica,
                            server_stat_info.get_server(),
                            dr_ls_info,
                            ObDRTaskType::LS_MIGRATE_REPLICA,
                            can_generate))) {
      LOG_WARN("fail to check can generate replicate to unit task", KR(ret));
    } else if (can_generate) {
      if (OB_FAIL(generate_replicate_to_unit_task(
              task_key, task_id, dst_member, src_member, data_source,
              old_paxos_replica_number, task_comment, acc_dr_task))) {
        LOG_WARN("fail to generate replicate to unit task", KR(ret));
      }
    }
  }
  return ret;
}

int ObDRWorker::try_generate_remove_replica_locality_alignment_task(
    DRLSInfo &dr_ls_info,
    const LATask *task,
    int64_t &acc_dr_task)
{
  int ret = OB_SUCCESS;
  bool task_exist = false;
  bool sibling_task_executing = false;
  uint64_t tenant_id = OB_INVALID_ID;
  share::ObLSID ls_id;
  if (OB_ISNULL(task)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), KP(task));
  } else if (OB_FAIL(dr_ls_info.get_ls_id(tenant_id, ls_id))) {
    LOG_WARN("fail to get ls id", KR(ret));
  } else {
    ObDRTaskKey task_key;
    const RemoveReplicaLATask *my_task = reinterpret_cast<const RemoveReplicaLATask *>(task);
    ObReplicaMember remove_member(my_task->remove_server_,
                                  my_task->member_time_us_,
                                  my_task->replica_type_,
                                  my_task->memstore_percent_);
    ObRemoveLSReplicaTask remove_paxos_task;
    bool has_leader = false;
    common::ObAddr leader_addr;
    share::ObTaskId task_id;
    ObString comment_to_set = "";
    if (ObReplicaTypeCheck::is_paxos_replica_V2(my_task->replica_type_)) {
      comment_to_set.assign_ptr(drtask::REMOVE_LOCALITY_PAXOS_REPLICA,
                                strlen(drtask::REMOVE_LOCALITY_PAXOS_REPLICA));
    } else {
      comment_to_set.assign_ptr(drtask::REMOVE_LOCALITY_NON_PAXOS_REPLICA,
                                strlen(drtask::REMOVE_LOCALITY_NON_PAXOS_REPLICA));
    }

    if (FALSE_IT(task_id.init(GCONF.self_addr_))) {
      //shall never be here
    } else if (OB_FAIL(check_has_leader_while_remove_replica(
            my_task->remove_server_,
            dr_ls_info,
            has_leader))) {
      LOG_WARN("fail to check has leader while member change", KR(ret), K(dr_ls_info));
    } else if (!has_leader) {
      LOG_INFO("may has no leader while member change", K(dr_ls_info));
    } else if (OB_FAIL(dr_ls_info.get_leader(leader_addr))) {
      LOG_WARN("fail to get leader", KR(ret));
    } else if (OB_FAIL(generate_task_key(dr_ls_info, leader_addr, task->get_task_type(), task_key))) {
      LOG_WARN("fail to generate task key", KR(ret));
    } else if (OB_FAIL(remove_paxos_task.build(
            task_key,
            task_id,
            comment_to_set,
            leader_addr,
            remove_member,
            my_task->orig_paxos_replica_number_,
            my_task->paxos_replica_number_,
            my_task->replica_type_))) {
      LOG_WARN("fail to build task", KR(ret), K(task_key), K(task_id),
               K(leader_addr), K(remove_member), KPC(my_task));
    } else if (OB_FAIL(add_task_(remove_paxos_task, acc_dr_task))) {
      LOG_WARN("fail to add task", KR(ret), K(remove_paxos_task));
    }
  }
  return ret;
}

int ObDRWorker::try_generate_add_replica_locality_alignment_task(
    DRLSInfo &dr_ls_info,
    const LATask *task,
    int64_t &acc_dr_task)
{
  int ret = OB_SUCCESS;
  bool task_exist = false;
  bool sibling_task_executing = false;
  uint64_t tenant_id = OB_INVALID_ID;
  share::ObLSID ls_id;
  if (OB_ISNULL(task)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), KP(task));
  } else if (OB_FAIL(dr_ls_info.get_ls_id(tenant_id, ls_id))) {
    LOG_WARN("fail to get ls id", KR(ret));
  } else {
    ObDRTaskKey task_key;
    const AddReplicaLATask *my_task = reinterpret_cast<const AddReplicaLATask *>(task);
    int64_t data_size = 0;
    ObReplicaMember data_source;
    ObReplicaMember dst_member(my_task->dst_server_,
                               my_task->member_time_us_,
                               my_task->replica_type_,
                               my_task->memstore_percent_);
    ObAddLSReplicaTask add_replica_task;
    share::ObTaskId task_id;
    ObString comment_to_set;
    if (ObReplicaTypeCheck::is_paxos_replica_V2(my_task->replica_type_)) {
      comment_to_set.assign_ptr(drtask::ADD_LOCALITY_PAXOS_REPLICA,
                                strlen(drtask::ADD_LOCALITY_PAXOS_REPLICA));
    } else {
      comment_to_set.assign_ptr(drtask::ADD_LOCALITY_NON_PAXOS_REPLICA,
                                strlen(drtask::ADD_LOCALITY_NON_PAXOS_REPLICA));
    }

    if (FALSE_IT(task_id.init(GCONF.self_addr_))) {
      //shall never be here
    } else if (OB_FAIL(dr_ls_info.get_default_data_source(data_source, data_size))) {
      LOG_WARN("fail to get data_size", KR(ret), K(dr_ls_info));
    } else if (OB_FAIL(generate_task_key(dr_ls_info, my_task->dst_server_, task->get_task_type(), task_key))) {
      LOG_WARN("fail to generate task key", KR(ret));
    } else if (OB_FAIL(add_replica_task.build(
            task_key,
            task_id,
            comment_to_set,
            dst_member,
            data_source,
            ObReplicaMember()/*empty force_data_source*/,
            my_task->orig_paxos_replica_number_,
            my_task->paxos_replica_number_))) {
      LOG_WARN("fail to build add replica task", KR(ret));
    } else if (OB_FAIL(add_task_(add_replica_task, acc_dr_task))) {
      LOG_WARN("fail to add task", KR(ret), K(add_replica_task));
    }
  }
  return ret;
}

int ObDRWorker::try_generate_type_transform_locality_alignment_task(
    DRLSInfo &dr_ls_info,
    const LATask *task,
    int64_t &acc_dr_task)
{
  int ret = OB_SUCCESS;
  bool task_exist = false;
  bool sibling_task_executing = false;
  uint64_t tenant_id = OB_INVALID_ID;
  share::ObLSID ls_id;
  if (OB_ISNULL(task)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), KP(task));
  } else if (OB_FAIL(dr_ls_info.get_ls_id(tenant_id, ls_id))) {
    LOG_WARN("fail to get ls id", KR(ret));
  } else {
    ObDRTaskKey task_key;
    const TypeTransformLATask *my_task = reinterpret_cast<const TypeTransformLATask *>(task);
    bool has_leader = false;
    int64_t data_size = 0;
    ObReplicaMember data_source;
    ObReplicaMember src_member(my_task->dst_server_,
                               my_task->src_member_time_us_,
                               my_task->src_replica_type_,
                               my_task->src_memstore_percent_);
    ObReplicaMember dst_member(my_task->dst_server_,
                               my_task->dst_member_time_us_,
                               my_task->dst_replica_type_,
                               my_task->dst_memstore_percent_);
    ObLSTypeTransformTask type_transform_task;
    share::ObTaskId task_id;
    if (FALSE_IT(task_id.init(GCONF.self_addr_))) {
      //shall never be here
    } else if (OB_FAIL(check_has_leader_while_remove_replica(
            my_task->dst_server_,
            dr_ls_info,
            has_leader))) {
      LOG_WARN("fail to check has leader while member change", KR(ret), K(dr_ls_info));
    } else if (!has_leader) {
      LOG_INFO("may has no leader while member change", K(dr_ls_info));
    } else if (OB_FAIL(dr_ls_info.get_default_data_source(data_source, data_size))) {
      LOG_WARN("fail to get data_size", KR(ret), K(dr_ls_info));
    } else if (OB_FAIL(generate_task_key(dr_ls_info, my_task->dst_server_, task->get_task_type(), task_key))) {
      LOG_WARN("fail to generate task key", KR(ret));
    } else if (OB_FAIL(type_transform_task.build(
            task_key,
            task_id,
            ObString(drtask::TRANSFORM_LOCALITY_REPLICA_TYPE),
            dst_member,
            src_member,
            data_source,
            my_task->orig_paxos_replica_number_,
            my_task->paxos_replica_number_))) {
      LOG_WARN("fail to build type transform task", KR(ret));
    } else if (OB_FAIL(add_task_(type_transform_task, acc_dr_task))) {
      LOG_WARN("fail to add task", KR(ret), K(type_transform_task));
    }
  }
  return ret;
}

int ObDRWorker::try_generate_modify_paxos_replica_number_locality_alignment_task(
    DRLSInfo &dr_ls_info,
    const LATask *task,
    int64_t &acc_dr_task_cnt)
{
  int ret = OB_SUCCESS;
  bool task_exist = false;
  bool sibling_task_executing = false;
  uint64_t tenant_id = OB_INVALID_ID;
  share::ObLSID ls_id;
  GlobalLearnerList learner_list;
  if (OB_ISNULL(task)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), KP(task));
  } else if (OB_FAIL(dr_ls_info.get_ls_id(tenant_id, ls_id))) {
    LOG_WARN("fail to get ls id", KR(ret));
  } else {
    ObDRTaskKey task_key;
    const ModifyPaxosReplicaNumberLATask *my_task = reinterpret_cast<const ModifyPaxosReplicaNumberLATask *>(task);
    common::ObAddr leader_addr;
    common::ObMemberList member_list;
    ObLSModifyPaxosReplicaNumberTask modify_paxos_replica_number_task;
    share::ObTaskId task_id;
    if (FALSE_IT(task_id.init(GCONF.self_addr_))) {
      //shall never be here
    } else if (OB_FAIL(dr_ls_info.get_leader_and_member_list(
            leader_addr,
            member_list,
            learner_list))) {
      LOG_WARN("fail to get leader", KR(ret));
    } else if (OB_FAIL(generate_task_key(dr_ls_info, leader_addr, task->get_task_type(), task_key))) {
      LOG_WARN("fail to generate task key", KR(ret));
    } else if (OB_FAIL(modify_paxos_replica_number_task.build(
            task_key,
            task_id,
            ObString(drtask::MODIFY_PAXOS_REPLICA_NUMBER),
            leader_addr,
            my_task->orig_paxos_replica_number_,
            my_task->paxos_replica_number_,
            member_list))) {
      LOG_WARN("fail to build a modify paxos replica number task", KR(ret));
    } else if (OB_FAIL(add_task_(modify_paxos_replica_number_task, acc_dr_task_cnt))) {
      LOG_WARN("fail to add task", KR(ret), K(modify_paxos_replica_number_task));
    }
  }
  return ret;
}

int ObDRWorker::try_generate_locality_alignment_task(
    DRLSInfo &dr_ls_info,
    const LATask *task,
    int64_t &acc_dr_task_cnt)
{
  int ret = OB_SUCCESS;
  if (OB_ISNULL(task)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), KP(task));
  } else {
    switch (task->get_task_type()) {
      case ObDRTaskType::LS_REMOVE_PAXOS_REPLICA:
      case ObDRTaskType::LS_REMOVE_NON_PAXOS_REPLICA: {
        if (OB_FAIL(try_generate_remove_replica_locality_alignment_task(
                dr_ls_info,
                task,
                acc_dr_task_cnt))) {
          LOG_WARN("fail to try generate remove replica task",
                    KR(ret), KPC(task));
        }
        break;
      }
      case ObDRTaskType::LS_ADD_REPLICA: {
        if (OB_FAIL(try_generate_add_replica_locality_alignment_task(
                dr_ls_info,
                task,
                acc_dr_task_cnt))) {
          LOG_WARN("fail to try generate add replica paxos task",
                    KR(ret), KPC(task));
        }
        break;
      }
      case ObDRTaskType::LS_TYPE_TRANSFORM: {
        if (OB_FAIL(try_generate_type_transform_locality_alignment_task(
                dr_ls_info,
                task,
                acc_dr_task_cnt))) {
          LOG_WARN("fail to try generate type transform paxos task",
                    KR(ret), KPC(task));
        }
        break;
      }
      case ObDRTaskType::LS_MODIFY_PAXOS_REPLICA_NUMBER: {
        if (OB_FAIL(try_generate_modify_paxos_replica_number_locality_alignment_task(
                dr_ls_info,
                task,
                acc_dr_task_cnt))) {
          LOG_WARN("fail to try generate modify paxos replica number task",
                    KR(ret), KPC(task));
        }
        break;
      }
      default: {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("unexpected task type", KR(ret), KPC(task));
        break;
      }
    }
  }
  return ret;
}

int ObDRWorker::record_task_plan_for_locality_alignment(
    DRLSInfo &dr_ls_info,
    const LATask *task)
{
  int ret = OB_SUCCESS;
  ObDRTaskType task_type = ObDRTaskType::MAX_TYPE;
  uint64_t tenant_id = OB_INVALID_ID;
  share::ObLSID ls_id;
  ObReplicaType source_replica_type = REPLICA_TYPE_INVALID;
  ObReplicaType target_replica_type = REPLICA_TYPE_INVALID;
  ObDRTaskPriority task_priority = ObDRTaskPriority::MAX_PRI;
  common::ObAddr leader_addr;
  common::ObAddr source_svr;
  common::ObAddr target_svr;
  common::ObAddr execute_svr;
  int64_t source_replica_paxos_replica_number = OB_INVALID_COUNT;
  int64_t target_replica_paxos_replica_number = OB_INVALID_COUNT;
  int64_t data_size = 0;
  ObString comment = "";
  ObReplicaMember data_source;
  if (OB_ISNULL(task)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("task is nullptr", KR(ret), KP(task));
  } else if (OB_FAIL(dr_ls_info.get_ls_id(tenant_id, ls_id))) {
    LOG_WARN("fail to get ls id", KR(ret));
  } else if (OB_FAIL(dr_ls_info.get_leader(leader_addr))) {
    LOG_WARN("fail to get leader", KR(ret));
  } else {
    ObLSReplicaTaskDisplayInfo display_info;
    switch (task->get_task_type()) {
      case ObDRTaskType::LS_REMOVE_PAXOS_REPLICA:
      case ObDRTaskType::LS_REMOVE_NON_PAXOS_REPLICA: {
        const RemoveReplicaLATask *my_task = reinterpret_cast<const RemoveReplicaLATask *>(task);
        task_type = task->get_task_type();
        source_replica_type = REPLICA_TYPE_INVALID;
        target_replica_type = my_task->replica_type_;
        task_priority = task_type == ObDRTaskType::LS_REMOVE_PAXOS_REPLICA ? ObDRTaskPriority::HIGH_PRI : ObDRTaskPriority::LOW_PRI;
        target_svr = my_task->remove_server_;
        execute_svr = leader_addr;
        source_replica_paxos_replica_number = my_task->orig_paxos_replica_number_;
        target_replica_paxos_replica_number = my_task->paxos_replica_number_;
        if (task_type == ObDRTaskType::LS_REMOVE_PAXOS_REPLICA) {
          comment.assign_ptr(drtask::REMOVE_LOCALITY_PAXOS_REPLICA, strlen(drtask::REMOVE_LOCALITY_PAXOS_REPLICA));
        } else {
          comment.assign_ptr(drtask::REMOVE_LOCALITY_NON_PAXOS_REPLICA, strlen(drtask::REMOVE_LOCALITY_NON_PAXOS_REPLICA));
        }
        break;
      }
      case ObDRTaskType::LS_ADD_REPLICA: {
        const AddReplicaLATask *my_task = reinterpret_cast<const AddReplicaLATask *>(task);
        if (OB_FAIL(dr_ls_info.get_default_data_source(data_source, data_size))) {
          LOG_WARN("fail to get data_size", KR(ret), K(dr_ls_info));
        } else {
          task_type = ObDRTaskType::LS_ADD_REPLICA;
          source_replica_type = data_source.get_replica_type();
          target_replica_type = my_task->replica_type_;
          task_priority = ObDRTaskPriority::HIGH_PRI;
          source_svr = data_source.get_server();
          target_svr = my_task->dst_server_;
          execute_svr = my_task->dst_server_;
          source_replica_paxos_replica_number = my_task->orig_paxos_replica_number_;
          target_replica_paxos_replica_number = my_task->paxos_replica_number_;
          if (ObReplicaTypeCheck::is_paxos_replica_V2(target_replica_type)) {
            comment.assign_ptr(drtask::ADD_LOCALITY_PAXOS_REPLICA, strlen(drtask::ADD_LOCALITY_PAXOS_REPLICA));
          } else {
            comment.assign_ptr(drtask::ADD_LOCALITY_NON_PAXOS_REPLICA, strlen(drtask::ADD_LOCALITY_NON_PAXOS_REPLICA));
          }
        }
        break;
      }
      case ObDRTaskType::LS_TYPE_TRANSFORM: {
        const TypeTransformLATask *my_task = reinterpret_cast<const TypeTransformLATask *>(task);
        if (OB_FAIL(dr_ls_info.get_default_data_source(data_source, data_size))) {
          LOG_WARN("fail to get data_size", KR(ret), K(dr_ls_info));
        } else {
          task_type = ObDRTaskType::LS_TYPE_TRANSFORM;
          source_replica_type = my_task->src_replica_type_;
          target_replica_type = my_task->dst_replica_type_;
          task_priority = ObDRTaskPriority::HIGH_PRI;
          source_svr = data_source.get_server();
          target_svr = my_task->dst_server_;
          execute_svr = my_task->dst_server_;
          source_replica_paxos_replica_number = my_task->orig_paxos_replica_number_;
          target_replica_paxos_replica_number = my_task->paxos_replica_number_;
          comment.assign_ptr(drtask::TRANSFORM_LOCALITY_REPLICA_TYPE, strlen(drtask::TRANSFORM_LOCALITY_REPLICA_TYPE));
        }
        break;
      }
      case ObDRTaskType::LS_MODIFY_PAXOS_REPLICA_NUMBER: {
        const ModifyPaxosReplicaNumberLATask *my_task = reinterpret_cast<const ModifyPaxosReplicaNumberLATask *>(task);
        task_type = ObDRTaskType::LS_MODIFY_PAXOS_REPLICA_NUMBER;
        source_replica_type = REPLICA_TYPE_FULL;
        target_replica_type = REPLICA_TYPE_FULL;
        task_priority = ObDRTaskPriority::HIGH_PRI;
        target_svr = leader_addr;
        execute_svr = leader_addr;
        source_replica_paxos_replica_number = my_task->orig_paxos_replica_number_;
        target_replica_paxos_replica_number = my_task->paxos_replica_number_;
        comment.assign_ptr(drtask::MODIFY_PAXOS_REPLICA_NUMBER, strlen(drtask::MODIFY_PAXOS_REPLICA_NUMBER));
        break;
      }
      default: {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("unexpected task type", KR(ret), KPC(task));
        break;
      }
    }

    if (FAILEDx(display_info.init(
                    tenant_id,
                    ls_id,
                    task_type,
                    task_priority,
                    target_svr,
                    target_replica_type,
                    target_replica_paxos_replica_number,
                    source_svr,
                    source_replica_type,
                    source_replica_paxos_replica_number,
                    execute_svr,
                    comment))) {
      LOG_WARN("fail to init a ObLSReplicaTaskDisplayInfo", KR(ret), K(tenant_id), K(ls_id), K(task_type),
               K(task_priority), K(target_svr), K(target_replica_type), K(target_replica_paxos_replica_number),
               K(source_svr), K(source_replica_type), K(source_replica_paxos_replica_number), K(execute_svr), K(comment));
    } else if (OB_FAIL(add_display_info(display_info))) {
      FLOG_WARN("fail to add display info", KR(ret), K(display_info));
    } else {
      FLOG_INFO("success to add display info", KR(ret), K(display_info));
    }
  }
  return ret;
}

int ObDRWorker::try_locality_alignment(
    const bool only_for_display,
    DRLSInfo &dr_ls_info,
    int64_t &acc_dr_task)
{
  int ret = OB_SUCCESS;
  DEBUG_SYNC(BEFORE_TRY_LOCALITY_ALIGNMENT);
  LOG_INFO("try locality alignment", K(dr_ls_info), K(only_for_display));
  LocalityAlignment locality_alignment(dr_ls_info, MTL_ID());
  const LATask *task = nullptr;
  if (OB_ISNULL(GCTX.config_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("config_ ptr is null", KR(ret), KP(GCTX.config_));
  } else if (!GCTX.config_->is_rereplication_enabled()) {
    // bypass
  } else if (!dr_ls_info.has_leader()) {
    LOG_WARN("has no leader, maybe not report yet",
             KR(ret), K(dr_ls_info));
  } else if (dr_ls_info.get_paxos_replica_number() <= 0) {
    LOG_WARN("paxos_replica_number is invalid, maybe not report yet",
             KR(ret), K(dr_ls_info));
  } else if (OB_FAIL(locality_alignment.build())) {
    LOG_WARN("fail to build locality alignment", KR(ret));
  } else {
    while (OB_SUCC(ret) && OB_SUCC(locality_alignment.get_next_locality_alignment_task(task))) {
      bool can_generate = false;
      const bool need_check_has_leader_while_remove_replica = false;
      ObAddr server_addr; //useless
      if (OB_ISNULL(task)) {
        // bypass, there is no task to generate
      } else if (only_for_display) {
        if (OB_FAIL(record_task_plan_for_locality_alignment(dr_ls_info, task))) {
          LOG_WARN("fail to record task plan", KR(ret), KPC(task));
        } else {
          LOG_INFO("success to record task plan", KR(ret), KPC(task));
        }
      } else if (OB_FAIL(check_can_generate_task(
                               acc_dr_task,
                               need_check_has_leader_while_remove_replica,
                               server_addr,
                               dr_ls_info,
                               task->get_task_type(),
                               can_generate))) {
        LOG_WARN("fail to check can generate locality alignment task", KR(ret));
      } else if (!can_generate) {
        //bypass
      } else if (OB_FAIL(try_generate_locality_alignment_task(
              dr_ls_info, task, acc_dr_task))) {
        LOG_WARN("fail to try generate locality alignment task", KR(ret));
      }
    }
    if (OB_ITER_END == ret) {
      ret = OB_SUCCESS;
    }
  }
  // no need to print task key, since the previous log contains that
  LOG_INFO("finish try locality alignment", KR(ret), K(acc_dr_task));
  return ret;
}

int ObDRWorker::try_shrink_resource_pools(
    const bool &only_for_display,
    DRLSInfo &dr_ls_info,
    int64_t &acc_dr_task)
{
  int ret = OB_SUCCESS;
  int64_t replica_cnt = 0;
  const share::ObLSStatusInfo *ls_status_info = NULL;
  if (OB_ISNULL(GCTX.config_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("config_ ptr is null", KR(ret), KP(GCTX.config_));
  } else if (!GCTX.config_->is_rereplication_enabled()) {
    // bypass
  } else if (!dr_ls_info.has_leader()) {
    LOG_WARN("has no leader, maybe not report yet",
             KR(ret), K(dr_ls_info));
  } else if (dr_ls_info.get_paxos_replica_number() <= 0) {
    LOG_WARN("paxos_replica_number is invalid, maybe not report yet",
             KR(ret), K(dr_ls_info));
  } else if (OB_FAIL(dr_ls_info.get_replica_cnt(replica_cnt))) {
    LOG_WARN("fail to get replica cnt", KR(ret), K(dr_ls_info));
  } else if (OB_FAIL(dr_ls_info.get_ls_status_info(ls_status_info))) {
    LOG_WARN("failed to get ls status info", KR(ret), K(dr_ls_info));
  } else if (OB_ISNULL(ls_status_info)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("failed to get ls status info", KR(ret));
  } else {
    ObDRWorker::UnitProvider unit_provider;
    const uint64_t tenant_id = ls_status_info->tenant_id_;
    if (OB_FAIL(unit_provider.init(gen_user_tenant_id(tenant_id), dr_ls_info))) {
      LOG_WARN("fail to init unit provider", KR(ret), K(tenant_id), K(dr_ls_info));
    }
    for (int64_t index = 0; OB_SUCC(ret) && index < replica_cnt; ++index) {
      share::ObLSReplica *ls_replica = nullptr;
      DRServerStatInfo *server_stat_info = nullptr;
      DRUnitStatInfo *unit_stat_info = nullptr;
      DRUnitStatInfo *unit_in_group_stat_info = nullptr;
      bool need_generate = false;
      bool is_unit_in_group_related = false;
      if (OB_FAIL(dr_ls_info.get_replica_stat(
                         index,
                         ls_replica,
                         server_stat_info,
                         unit_stat_info,
                         unit_in_group_stat_info))) {
        LOG_WARN("fail to get replica stat", KR(ret), K(index));
      } else if (OB_ISNULL(ls_replica)
                 || OB_ISNULL(server_stat_info)
                 || OB_ISNULL(unit_stat_info)
                 || OB_ISNULL(unit_in_group_stat_info)
                 || (unit_stat_info->is_in_pool() && OB_ISNULL(unit_stat_info->get_server_stat()))
                 || (unit_in_group_stat_info->is_in_pool() && OB_ISNULL(unit_in_group_stat_info->get_server_stat()))) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("replica stat unexpected",
                 KR(ret),
                 KP(ls_replica),
                 KP(server_stat_info),
                 KP(unit_stat_info),
                 KP(unit_in_group_stat_info));
      } else if (REPLICA_STATUS_NORMAL == ls_replica->get_replica_status()
          && share::ObUnit::UNIT_STATUS_DELETING == unit_stat_info->get_unit().status_) {
        // replica is still in member_list, but unit is in DELETING status
        // If this is a duplicate log stream
        //   1.1 deleting unit on specific unit group: do nothing because specific unit group should have replica
        //   1.2 for non-paxos(R or C) replica: execute remove_learner task directly
        //   1.3 for F-replica: try to execute migrate-replica first,
        //                      if migrate-replica task can not generate then try to type_transform another R to F
        // If this is a normal log stream
        //   2.1 try to execute migrate-replica task for replica of any type
        if (dr_ls_info.is_duplicate_ls()) {
          if (0 != ls_status_info->ls_group_id_
              && unit_in_group_stat_info->get_unit_id() == unit_stat_info->get_unit_id()) {
            // do nothing
            LOG_INFO("deleting unit is on specific unit group, do not process replica for deleting unit",
                KPC(ls_replica), K(only_for_display), K(acc_dr_task), KP(ls_status_info),
                KP(unit_in_group_stat_info), KP(unit_stat_info), K(dr_ls_info));
          } else if (ObReplicaTypeCheck::is_non_paxos_replica(ls_replica->get_replica_type())) {
            // 1.1 try to generate and execute remove learner task
            if (OB_FAIL(try_remove_non_paxos_replica_for_deleting_unit_(
                            *ls_replica,
                            only_for_display,
                            dr_ls_info,
                            acc_dr_task))) {
              LOG_WARN("fail to try remove readonly replica for deleting unit", KPC(ls_replica),
                       K(only_for_display), K(dr_ls_info), K(acc_dr_task));
            }
          } else if (REPLICA_TYPE_FULL == ls_replica->get_replica_type()) {
            // 1.2 try to generate and execute migrate replica task
            int64_t previous_acc_dr_task = acc_dr_task; // to check whether migrate task generated
            bool migrate_task_generated = false;
            if (OB_FAIL(try_migrate_replica_for_deleting_unit_(
                            unit_provider,
                            dr_ls_info,
                            *ls_replica,
                            *ls_status_info,
                            *server_stat_info,
                            *unit_stat_info,
                            *unit_in_group_stat_info,
                            only_for_display,
                            acc_dr_task))) {
              LOG_WARN("fail to try migrate replica for deleting unit", KR(ret), K(dr_ls_info),
                       KPC(ls_replica), KPC(ls_status_info), KPC(server_stat_info),
                       KPC(unit_stat_info), KPC(unit_in_group_stat_info), K(only_for_display));
            } else if (FALSE_IT(migrate_task_generated = acc_dr_task != previous_acc_dr_task)) {
              // 1.2 A migrate task already generated, do nothing.
              //     If migrate-task not generated, try to do type transform
            } else if (OB_FAIL(try_type_transform_for_deleting_unit_(
                                   dr_ls_info,
                                   *ls_replica,
                                   only_for_display,
                                   acc_dr_task))) {
              LOG_WARN("fail to try type transform for deleting unit", KR(ret), K(dr_ls_info),
                       KPC(ls_replica), K(only_for_display));
            }
          } else {
            ret = OB_NOT_SUPPORTED;
            LOG_WARN("unexpected replica type", KR(ret), KPC(ls_replica));
          }
        } else {
          // generate task for normal log stream replica
          if (0 == ls_status_info->ls_group_id_
          || ls_status_info->unit_group_id_ != unit_stat_info->get_unit().unit_group_id_) {
            //If the Unit Group is in the DELETING status, we need to migrate out the LS that do not belong to that Unit Group.
            //LS belonging to this Unit Group will be automatically processed by Balance module
            // 2.1 try generate and execute migrate replica for normal log stream
            if (OB_FAIL(try_migrate_replica_for_deleting_unit_(
                            unit_provider,
                            dr_ls_info,
                            *ls_replica,
                            *ls_status_info,
                            *server_stat_info,
                            *unit_stat_info,
                            *unit_in_group_stat_info,
                            only_for_display,
                            acc_dr_task))) {
              LOG_WARN("fail to try migrate replica for deleting unit", KR(ret), K(dr_ls_info),
                       KPC(ls_replica), KPC(ls_status_info), KPC(server_stat_info),
                       KPC(unit_stat_info), KPC(unit_in_group_stat_info), K(only_for_display));
            }
          }
        }
      }
    }//end for each ls replica
  }
  LOG_INFO("finish try shrink resource pool", KR(ret), K(acc_dr_task));
  return ret;
}

int ObDRWorker::try_remove_non_paxos_replica_for_deleting_unit_(
    const share::ObLSReplica &ls_replica,
    const bool &only_for_display,
    DRLSInfo &dr_ls_info,
    int64_t &acc_dr_task)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!ls_replica.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(ls_replica));
  } else {
    share::ObTaskId task_id;
    int64_t new_paxos_replica_number = 0;
    int64_t old_paxos_replica_number = 0;
    common::ObAddr leader_addr;
    const common::ObAddr source_server; // not useful
    const bool need_check_has_leader_while_remove_replica = false;
    const bool is_high_priority_task = true;
    const int64_t memstore_percent = 100;
    ObDRTaskKey task_key;
    bool can_generate = false;
    uint64_t tenant_id = OB_INVALID_TENANT_ID;
    share::ObLSID ls_id;
    ObDRTaskType task_type = ObDRTaskType::LS_REMOVE_NON_PAXOS_REPLICA;
    ObReplicaMember remove_learner(ls_replica.get_server(),
                                   ls_replica.get_member_time_us(),
                                   ls_replica.get_replica_type(),
                                   memstore_percent);
    if (OB_FAIL(construct_extra_infos_to_build_remove_replica_task(
                    dr_ls_info,
                    task_id,
                    new_paxos_replica_number,
                    old_paxos_replica_number,
                    leader_addr,
                    ls_replica.get_replica_type()))) {
              LOG_WARN("fail to construct extra infos to build remove replica task", KR(ret), K(dr_ls_info), K(ls_replica));
    } else if (OB_FAIL(dr_ls_info.get_ls_id(tenant_id, ls_id))) {
      LOG_WARN("fail to get ls id", KR(ret), K(dr_ls_info), K(tenant_id), K(ls_id));
    } else if (only_for_display) {
      // only for display, no need to execute this task
      ObLSReplicaTaskDisplayInfo display_info;
      if (OB_FAIL(display_info.init(
                      tenant_id,
                      ls_id,
                      task_type,
                      is_high_priority_task ? ObDRTaskPriority::HIGH_PRI : ObDRTaskPriority::LOW_PRI,
                      ls_replica.get_server(),
                      ls_replica.get_replica_type(),
                      new_paxos_replica_number,
                      source_server,
                      REPLICA_TYPE_INVALID/*source_replica_type*/,
                      old_paxos_replica_number,
                      leader_addr,
                      "shrink unit task"))) {
        LOG_WARN("fail to init a ObLSReplicaTaskDisplayInfo", KR(ret), K(tenant_id), K(ls_id),
                 K(task_type), K(ls_replica), K(new_paxos_replica_number),
                 K(old_paxos_replica_number), K(leader_addr));
      } else if (OB_FAIL(add_display_info(display_info))) {
        LOG_WARN("fail to add display info", KR(ret), K(display_info));
      } else {
        LOG_INFO("success to add display info", KR(ret), K(display_info));
      }
    } else if (OB_FAIL(generate_task_key(dr_ls_info, leader_addr, task_type, task_key))) {
      LOG_WARN("fail to generate task key", KR(ret), K(dr_ls_info), K(leader_addr), K(task_type));
    } else if (OB_FAIL(check_can_generate_task(
                           acc_dr_task,
                           need_check_has_leader_while_remove_replica,
                           ls_replica.get_server(),
                           dr_ls_info,
                           task_type,
                           can_generate))) {
      LOG_WARN("fail to check can generate remove permanent offline task", KR(ret), K(acc_dr_task),
               K(need_check_has_leader_while_remove_replica), K(ls_replica),
               K(dr_ls_info), K(can_generate));
    } else if (can_generate) {
      ObRemoveLSReplicaTask remove_replica_task;
      if (OB_FAIL(remove_replica_task.build(
                      task_key,
                      task_id,
                      "shrink unit task",
                      leader_addr,
                      remove_learner,
                      old_paxos_replica_number,
                      new_paxos_replica_number,
                      ls_replica.get_replica_type()))) {
        LOG_WARN("fail to build remove member task", KR(ret), K(task_key), K(leader_addr),
                 K(remove_learner), K(old_paxos_replica_number), K(new_paxos_replica_number));
      } else if (OB_FAIL(add_task_(remove_replica_task, acc_dr_task))) {
        LOG_WARN("fail to add task", KR(ret), K(remove_replica_task));
      }
    }
  }
  return ret;
}

int ObDRWorker::try_migrate_replica_for_deleting_unit_(
    ObDRWorker::UnitProvider &unit_provider,
    DRLSInfo &dr_ls_info,
    const share::ObLSReplica &ls_replica,
    const share::ObLSStatusInfo &ls_status_info,
    const DRServerStatInfo &server_stat_info,
    const DRUnitStatInfo &unit_stat_info,
    const DRUnitStatInfo &unit_in_group_stat_info,
    const bool &only_for_display,
    int64_t &acc_dr_task)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(!ls_replica.is_valid()
                         || !ls_status_info.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(ls_replica), K(ls_status_info));
  } else {
    share::ObUnit dest_unit;
    if (OB_FAIL(unit_provider.allocate_unit(
                ls_replica.get_zone(),
                ls_status_info.unit_group_id_,
                dest_unit))) {
      if (OB_ITER_END == ret) {
        ret = OB_SUCCESS;
        LOG_INFO("can not allocate valid unit for this ls replica to do migration",
                 K(ls_replica), K(ls_status_info));
      } else {
        LOG_WARN("failed to allocate unit for this log stream", KR(ret),
                 K(ls_replica), K(ls_status_info));
      }
    } else {
      ObReplicaMember dst_member(
          dest_unit.server_,
          ObTimeUtility::current_time(),
          ls_replica.get_replica_type(),
          ls_replica.get_memstore_percent());
      if (OB_FAIL(generate_migrate_ls_task(
                      only_for_display, "shrink unit task", ls_replica,
                      server_stat_info, unit_stat_info,
                      unit_in_group_stat_info, dst_member, dr_ls_info,
                      acc_dr_task))) {
        LOG_WARN("failed to generate migrate ls task", KR(ret),
                 K(dst_member), K(only_for_display), K(ls_replica));
      }
    }
  }
  return ret;
}

int ObDRWorker::try_type_transform_for_deleting_unit_(
    DRLSInfo &dr_ls_info,
    const share::ObLSReplica &ls_replica,
    const bool &only_for_display,
    int64_t &acc_dr_task)
{
  int ret = OB_SUCCESS;
  DRUnitStatInfo *unit_in_group_stat_info = NULL; // no target unit
  bool find_a_valid_readonly_replica = false;
  ObString comment(drtask::TRANSFORM_DUE_TO_UNIT_DELETING);
  if (OB_FAIL(try_gen_type_transform_task_(
      dr_ls_info,
      ls_replica,
      unit_in_group_stat_info,
      only_for_display,
      comment,
      acc_dr_task,
      find_a_valid_readonly_replica))) {
    LOG_WARN("try gen type transform task failed", KR(ret),
        K(dr_ls_info), K(ls_replica), K(only_for_display), K(acc_dr_task));
  } else if (!find_a_valid_readonly_replica) {
    LOG_INFO("no valid readonly replica found to do type transform task for deleting unit",
        KR(ret), K(dr_ls_info), K(ls_replica), K(only_for_display), K(comment), K(acc_dr_task));
  }
  return ret;
}

int ObDRWorker::try_gen_type_transform_task_(
    DRLSInfo &dr_ls_info,
    const share::ObLSReplica &ls_replica,
    const DRUnitStatInfo *unit_in_group_stat_info,
    const bool only_for_display,
    const ObString &comment,
    int64_t &acc_dr_task,
    bool &find_a_valid_readonly_replica)
{
  int ret = OB_SUCCESS;
  ObLSReplica target_replica;
  uint64_t target_unit_id = 0;
  uint64_t target_unit_group_id = 0;
  find_a_valid_readonly_replica = false;
  if (OB_FAIL(find_valid_readonly_replica_(
                         dr_ls_info,
                         ls_replica,
                         ls_replica.get_zone(),
                         unit_in_group_stat_info,
                         target_replica,
                         target_unit_id,
                         target_unit_group_id,
                         find_a_valid_readonly_replica))) {
    LOG_WARN("fail to find a valid readonly replica", KR(ret), K(dr_ls_info), K(ls_replica));
  } else if (!find_a_valid_readonly_replica) {
    // do nothing, because no valid R-replica to do type transform
  } else {
    // try to generate type transform task
    share::ObTaskId task_id;
    ObDRTaskKey task_key;
    uint64_t tenant_id = 0;
    share::ObLSID ls_id;
    common::ObAddr leader_addr;
    int64_t old_paxos_replica_number = 0;
    int64_t new_paxos_replica_number = 0;
    ObReplicaMember data_source;
    int64_t data_size = 0;
    bool can_generate  = false;
    ObReplicaMember src_member(target_replica.get_server(),
                               target_replica.get_member_time_us(),
                               target_replica.get_replica_type(),
                               target_replica.get_memstore_percent());
    ObReplicaMember dst_member(target_replica.get_server(),
                               target_replica.get_member_time_us(),
                               REPLICA_TYPE_FULL,
                               target_replica.get_memstore_percent());
    if (OB_FAIL(dr_ls_info.get_default_data_source(data_source, data_size))) {
      LOG_WARN("fail to get data_size", KR(ret), K(dr_ls_info));
    } else if (OB_FAIL(construct_extra_info_to_build_type_transform_task_(
                         dr_ls_info,
                         task_id,
                         tenant_id,
                         ls_id,
                         leader_addr,
                         old_paxos_replica_number,
                         new_paxos_replica_number))) {
      LOG_WARN("fail to construct extra info to build a type transform task", KR(ret),
               K(dr_ls_info), K(ls_replica), K(dst_member),
               K(target_unit_id), K(target_unit_group_id));
    } else if (only_for_display) {
      ObLSReplicaTaskDisplayInfo display_info;
      if (OB_FAIL(display_info.init(
                      tenant_id,
                      ls_id,
                      ObDRTaskType::LS_TYPE_TRANSFORM,
                      ObDRTaskPriority::HIGH_PRI,
                      target_replica.get_server(),
                      REPLICA_TYPE_FULL/*target_replica_type*/,
                      new_paxos_replica_number,
                      data_source.get_server(),
                      REPLICA_TYPE_READONLY/*source_replica_type*/,
                      old_paxos_replica_number,
                      leader_addr,
                      comment))) {
        LOG_WARN("fail to init a ObLSReplicaTaskDisplayInfo", KR(ret), K(tenant_id),
                 K(ls_id), K(target_replica), K(new_paxos_replica_number), K(data_source),
                 K(old_paxos_replica_number), K(leader_addr), K(comment));
      } else if (OB_FAIL(add_display_info(display_info))) {
        LOG_WARN("fail to add display info", KR(ret), K(display_info));
      } else {
        LOG_INFO("success to add display info", KR(ret), K(display_info));
      }
    } else if (OB_FAIL(generate_task_key(dr_ls_info, target_replica.get_server(), ObDRTaskType::LS_TYPE_TRANSFORM, task_key))) {
      LOG_WARN("fail to generate task key", KR(ret), K(dr_ls_info), K(target_replica));
    } else if (OB_FAIL(check_can_generate_task(
                           acc_dr_task,
                           false/*need_check_has_leader_while_remove_replica*/,
                           target_replica.get_server(),
                           dr_ls_info,
                           ObDRTaskType::LS_TYPE_TRANSFORM,
                           can_generate))) {
      LOG_WARN("fail to check whether can generate task", KR(ret), K(acc_dr_task),
               K(target_replica), K(dr_ls_info));
    } else if (!can_generate) {
      LOG_INFO("can not generate type transform task");
    } else if (OB_FAIL(generate_type_transform_task_(
                           task_key,
                           task_id,
                           dst_member,
                           src_member,
                           data_source,
                           old_paxos_replica_number,
                           new_paxos_replica_number,
                           comment,
                           acc_dr_task))) {
      LOG_WARN("fail to generate type transform task", KR(ret), K(task_key), K(task_id), K(data_source), K(dst_member),
               K(src_member), K(old_paxos_replica_number), K(new_paxos_replica_number), K(acc_dr_task));
    }
  }
  return ret;
}

int ObDRWorker::try_type_transform_for_migrate_to_unit_(
    DRLSInfo &dr_ls_info,
    const share::ObLSReplica &ls_replica,
    const DRUnitStatInfo *unit_in_group_stat_info,
    const bool only_for_display,
    int64_t &acc_dr_task)
{
  int ret = OB_SUCCESS;
  bool find_a_valid_readonly_replica = false;
  ObString comment(drtask::TRANSFORM_DUE_TO_UNIT_NOT_MATCH);
  if (OB_ISNULL(unit_in_group_stat_info)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid args", KR(ret), KP(unit_in_group_stat_info));
  } else if (OB_FAIL(try_gen_type_transform_task_(
      dr_ls_info,
      ls_replica,
      unit_in_group_stat_info,
      only_for_display,
      comment,
      acc_dr_task,
      find_a_valid_readonly_replica))) {
    LOG_WARN("try gen type transform task failed", KR(ret),
        K(dr_ls_info), K(ls_replica), K(only_for_display), K(acc_dr_task));
  } else if (!find_a_valid_readonly_replica) {
    LOG_INFO("no valid readonly replica found to do type transform task for migrate to unit",
        KR(ret), K(dr_ls_info), K(ls_replica), KPC(unit_in_group_stat_info),
        K(only_for_display), K(comment), K(acc_dr_task));
  }
  return ret;
}

// if specified_unit_in_group is NULL, means find a valid automatically
int ObDRWorker::find_valid_readonly_replica_(
    DRLSInfo &dr_ls_info,
    const share::ObLSReplica &exclude_replica,
    const ObZone &target_zone,
    const DRUnitStatInfo *specified_unit_in_group,
    share::ObLSReplica &target_replica,
    uint64_t &unit_id,
    uint64_t &unit_group_id,
    bool &find_a_valid_readonly_replica)
{
  int ret = OB_SUCCESS;
  find_a_valid_readonly_replica = false;
  int64_t replica_cnt = 0;
  target_replica.reset();
  unit_id = 0;
  unit_group_id = 0;
  if (OB_FAIL(dr_ls_info.get_replica_cnt(replica_cnt))) {
    LOG_WARN("fail to get replica count", KR(ret), K(dr_ls_info));
  } else {
    for (int64_t i = 0; OB_SUCC(ret) && i < replica_cnt; i++) {
      share::ObLSReplica *replica = nullptr;
      DRServerStatInfo *server_stat_info = nullptr;
      DRUnitStatInfo *unit_stat_info = nullptr;
      DRUnitStatInfo *unit_in_group_stat_info = nullptr;
      if (OB_FAIL(dr_ls_info.get_replica_stat(
              i,
              replica,
              server_stat_info,
              unit_stat_info,
              unit_in_group_stat_info))) {
        LOG_WARN("fail to get replica stat", KR(ret));
      } else if (OB_ISNULL(replica)
                 || OB_ISNULL(server_stat_info)
                 || OB_ISNULL(unit_stat_info)
                 || OB_ISNULL(unit_in_group_stat_info)) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("replica related ptrs are null", KR(ret),
                 KP(replica),
                 KP(server_stat_info),
                 KP(unit_stat_info),
                 KP(unit_in_group_stat_info));
      } else if (target_zone != replica->get_zone()
                 || common::REPLICA_TYPE_READONLY != replica->get_replica_type()
                 || exclude_replica.get_server() == replica->get_server()) {
        // bypass
      } else if (OB_NOT_NULL(specified_unit_in_group)
          && specified_unit_in_group->get_unit().server_ != replica->get_server()) {
        // replica is not on target unit_group, just ignore
      } else if (replica->is_in_service()
                 && server_stat_info->is_alive()
                 && !server_stat_info->is_stopped()
                 && !replica->get_restore_status().is_failed()
                 && (unit_stat_info->get_unit().is_active_status() || OB_NOT_NULL(specified_unit_in_group))
                 && unit_stat_info->get_server_stat()->is_alive()
                 && !unit_stat_info->get_server_stat()->is_block()) {
        // if unit_in_group is specified, we ignore unit status
        // if unit_in_group not specified, it means any locations is ok, so try to find a ACTIVE unit as possible
        if (OB_FAIL(target_replica.assign(*replica))) {
          LOG_WARN("fail to assign replica", KR(ret), KPC(replica));
        } else {
          unit_id = unit_stat_info->get_unit().unit_id_;
          unit_group_id = unit_stat_info->get_unit().unit_group_id_;
          find_a_valid_readonly_replica = true;
          LOG_INFO("find a valid readonly replica to do type transform", K(dr_ls_info),
                   K(exclude_replica), K(target_zone), K(target_replica));
          break;
        }
      }
    }
  }
  return ret;
}

int ObDRWorker::construct_extra_info_to_build_type_transform_task_(
    DRLSInfo &dr_ls_info,
    share::ObTaskId &task_id,
    uint64_t &tenant_id,
    share::ObLSID &ls_id,
    common::ObAddr &leader_addr,
    int64_t &old_paxos_replica_number,
    int64_t &new_paxos_replica_number)
{
  int ret = OB_SUCCESS;
  tenant_id = OB_INVALID_TENANT_ID;
  ls_id = OB_INVALID_ID;
  leader_addr.reset();
  old_paxos_replica_number = 0;
  new_paxos_replica_number = 0;
  if (FALSE_IT(task_id.init(GCONF.self_addr_))) {
    //shall never be here
  } else if (OB_FAIL(dr_ls_info.get_leader(leader_addr))) {
    LOG_WARN("fail to get leader address", KR(ret), K(dr_ls_info));
  } else if (OB_FAIL(dr_ls_info.get_ls_id(tenant_id, ls_id))) {
    LOG_WARN("fail to get tenant and ls id", KR(ret), K(dr_ls_info), K(tenant_id), K(ls_id));
  } else {
    old_paxos_replica_number = dr_ls_info.get_paxos_replica_number();
    new_paxos_replica_number = dr_ls_info.get_paxos_replica_number() + 1;
  }
  return ret;
}

int ObDRWorker::generate_type_transform_task_(
    const ObDRTaskKey &task_key,
    const share::ObTaskId &task_id,
    const ObReplicaMember &dst_member,
    const ObReplicaMember &src_member,
    const ObReplicaMember &data_source,
    const int64_t old_paxos_replica_number,
    const int64_t new_paxos_replica_number,
    const ObString &comment,
    int64_t &acc_dr_task)
{
  int ret = OB_SUCCESS;
  ObLSTypeTransformTask type_transform_task;
  if (OB_FAIL(type_transform_task.build(
                 task_key,
                 task_id,
                 comment,
                 dst_member,
                 src_member,
                 data_source,
                 old_paxos_replica_number,
                 new_paxos_replica_number))) {
    LOG_WARN("fail to build type transform task", KR(ret), K(task_key), K(dst_member), K(src_member),
            K(data_source), K(old_paxos_replica_number), K(new_paxos_replica_number));
  } else if (OB_FAIL(add_task_(type_transform_task, acc_dr_task))) {
    LOG_WARN("fail to add task", KR(ret), K(type_transform_task));
  }
  return ret;
}

int ObDRWorker::check_need_generate_cancel_unit_migration_task(
    const int64_t index,
    DRLSInfo &dr_ls_info,
    share::ObLSReplica *&ls_replica,
    DRServerStatInfo *&server_stat_info,
    DRUnitStatInfo *&unit_stat_info,
    DRUnitStatInfo *&unit_in_group_stat_info,
    bool &is_paxos_replica_related,
    bool &need_generate)
{
  int ret = OB_SUCCESS;
  need_generate = false;
  is_paxos_replica_related = false;
  if (OB_FAIL(dr_ls_info.get_replica_stat(
              index,
              ls_replica,
              server_stat_info,
              unit_stat_info,
              unit_in_group_stat_info))) {
    LOG_WARN("fail to get replica stat", KR(ret));
  } else if (OB_ISNULL(ls_replica)
                 || OB_ISNULL(server_stat_info)
                 || OB_ISNULL(unit_stat_info)
                 || OB_ISNULL(unit_in_group_stat_info)
                 || (unit_stat_info->is_in_pool()
                     && OB_ISNULL(unit_stat_info->get_server_stat()))) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("replica stat unexpected", KR(ret),
                 KP(ls_replica),
                 KP(server_stat_info),
                 KP(unit_stat_info),
                 KP(unit_in_group_stat_info));
  } else if (unit_stat_info->is_in_pool()
          && unit_stat_info->get_unit().migrate_from_server_.is_valid()
          && unit_stat_info->get_server_stat()->is_block()
          && ls_replica->get_server() == unit_stat_info->get_server_stat()->get_server()) {
    if (ObReplicaTypeCheck::is_paxos_replica_V2(ls_replica->get_replica_type())
        && ((dr_ls_info.get_member_list_cnt() - 1) >= dr_ls_info.get_schema_replica_cnt())) {
      need_generate = true;
      is_paxos_replica_related = true;
    } else if (!ObReplicaTypeCheck::is_paxos_replica_V2(ls_replica->get_replica_type())) {
      need_generate = true;
      is_paxos_replica_related = false;
    }
  }
  return ret;
}

int ObDRWorker::construct_extra_info_to_build_cancel_migration_task(
    const bool &is_paxos_replica_related,
    DRLSInfo &dr_ls_info,
    const share::ObLSReplica &ls_replica,
    share::ObTaskId &task_id,
    uint64_t &tenant_id,
    share::ObLSID &ls_id,
    common::ObAddr &leader_addr,
    int64_t &old_paxos_replica_number,
    int64_t &new_paxos_replica_number)
{
  int ret = OB_SUCCESS;
  if (FALSE_IT(task_id.init(GCONF.self_addr_))) {
    //shall never be here
  } else if (is_paxos_replica_related) {
    if (OB_FAIL(dr_ls_info.get_leader(leader_addr))) {
      LOG_WARN("fail to get leader address", KR(ret));
    } else {
      tenant_id = ls_replica.get_tenant_id();
      ls_id = ls_replica.get_ls_id();
      old_paxos_replica_number = dr_ls_info.get_paxos_replica_number();
      new_paxos_replica_number = dr_ls_info.get_paxos_replica_number();
    }
  } else {
    tenant_id = ls_replica.get_tenant_id();
    ls_id = ls_replica.get_ls_id();
  }
  return ret;
}

int ObDRWorker::generate_cancel_unit_migration_task(
    const bool &is_paxos_replica_related,
    const ObDRTaskKey &task_key,
    const share::ObTaskId &task_id,
    const common::ObAddr &leader_addr,
    const ObReplicaMember &remove_member,
    const int64_t &old_paxos_replica_number,
    const int64_t &new_paxos_replica_number,
    int64_t &acc_dr_task)
{
  int ret = OB_SUCCESS;
  ObRemoveLSReplicaTask remove_member_task;
  ObString comment_to_set = "";
  ObReplicaType replica_type = remove_member.get_replica_type();
  if (is_paxos_replica_related) {
    comment_to_set.assign_ptr(drtask::CANCEL_MIGRATE_UNIT_WITH_PAXOS_REPLICA,
                              strlen(drtask::CANCEL_MIGRATE_UNIT_WITH_PAXOS_REPLICA));
  } else {
    comment_to_set.assign_ptr(drtask::CANCEL_MIGRATE_UNIT_WITH_NON_PAXOS_REPLICA,
                              strlen(drtask::CANCEL_MIGRATE_UNIT_WITH_NON_PAXOS_REPLICA));
  }
  if (OB_FAIL(remove_member_task.build(
                  task_key,
                  task_id,
                  comment_to_set,
                  leader_addr,
                  remove_member,
                  old_paxos_replica_number,
                  new_paxos_replica_number,
                  replica_type))) {
    LOG_WARN("fail to build remove member task", KR(ret), K(task_key), K(task_id));
  } else if (OB_FAIL(add_task_(remove_member_task, acc_dr_task))) {
    LOG_WARN("fail to add task", KR(ret), K(remove_member_task));
  }
  return ret;
}

int ObDRWorker::try_cancel_unit_migration(
    const bool only_for_display,
    DRLSInfo &dr_ls_info,
    int64_t &acc_dr_task)
{
  int ret = OB_SUCCESS;

  ObDRTaskKey task_key;
  bool task_exist = false;
  bool sibling_task_executing = false;
  int64_t replica_cnt = 0;
  DEBUG_SYNC(BEFORE_TRY_MIGRATE_UNIT);
  if (OB_ISNULL(GCTX.config_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("config_ ptr is null", KR(ret), KP(GCTX.config_));
  } else if (!GCTX.config_->is_rereplication_enabled()) {
    // bypass
  } else if (!dr_ls_info.has_leader()) {
    LOG_WARN("has no leader, maybe not report yet",
             KR(ret), K(dr_ls_info));
  } else if (dr_ls_info.get_paxos_replica_number() <= 0) {
    LOG_WARN("paxos_replica_number is invalid, maybe not report yet",
             KR(ret), K(dr_ls_info));
  } else if (OB_FAIL(dr_ls_info.get_replica_cnt(replica_cnt))) {
    LOG_WARN("fail to get replica cnt", KR(ret), K(dr_ls_info));
  } else {
    for (int64_t index = 0; OB_SUCC(ret) && index < replica_cnt; ++index) {
      share::ObLSReplica *ls_replica = nullptr;
      DRServerStatInfo *server_stat_info = nullptr;
      DRUnitStatInfo *unit_stat_info = nullptr;
      DRUnitStatInfo *unit_in_group_stat_info = nullptr;
      bool is_paxos_replica_related = false;
      bool need_generate = false;
      if (OB_FAIL(check_need_generate_cancel_unit_migration_task(
                      index,
                      dr_ls_info,
                      ls_replica,
                      server_stat_info,
                      unit_stat_info,
                      unit_in_group_stat_info,
                      is_paxos_replica_related,
                      need_generate))) {
        LOG_WARN("fail to check need generate cancel unit migration task", KR(ret));
      } else if (need_generate) {
        share::ObTaskId task_id;
        uint64_t tenant_id = 0;
        share::ObLSID ls_id;
        common::ObAddr leader_addr;
        int64_t old_paxos_replica_number = 0;
        int64_t new_paxos_replica_number = 0;
        bool can_generate = false;
        common::ObAddr source_svr; // not useful
        const bool need_check_has_leader_while_remove_replica = true;
        ObReplicaMember remove_member(ls_replica->get_server(),
                                      ls_replica->get_member_time_us(),
                                      ls_replica->get_replica_type(),
                                      ls_replica->get_memstore_percent());
        ObDRTaskType task_type = is_paxos_replica_related
                               ? ObDRTaskType::LS_REMOVE_PAXOS_REPLICA
                               : ObDRTaskType::LS_REMOVE_NON_PAXOS_REPLICA;
        ObDRTaskPriority task_priority = is_paxos_replica_related
                                       ? ObDRTaskPriority::HIGH_PRI
                                       : ObDRTaskPriority::LOW_PRI;
        ObString comment_to_set = "";
        if (is_paxos_replica_related) {
          comment_to_set.assign_ptr(drtask::CANCEL_MIGRATE_UNIT_WITH_PAXOS_REPLICA,
                                    strlen(drtask::CANCEL_MIGRATE_UNIT_WITH_PAXOS_REPLICA));
        } else {
          comment_to_set.assign_ptr(drtask::CANCEL_MIGRATE_UNIT_WITH_NON_PAXOS_REPLICA,
                                    strlen(drtask::CANCEL_MIGRATE_UNIT_WITH_NON_PAXOS_REPLICA));
        }

        if (OB_FAIL(construct_extra_info_to_build_cancel_migration_task(
                        is_paxos_replica_related,
                        dr_ls_info,
                        *ls_replica,
                        task_id,
                        tenant_id,
                        ls_id,
                        leader_addr,
                        old_paxos_replica_number,
                        new_paxos_replica_number))) {
          LOG_WARN("fail to construct extra info to build cancel unit migration task", KR(ret));
        } else if (only_for_display) {
          ObLSReplicaTaskDisplayInfo display_info;
          if (OB_FAIL(display_info.init(
                        tenant_id,
                        ls_id,
                        task_type,
                        task_priority,
                        ls_replica->get_server(),
                        ls_replica->get_replica_type(),
                        new_paxos_replica_number,
                        source_svr,
                        REPLICA_TYPE_INVALID,
                        old_paxos_replica_number,
                        leader_addr,
                        comment_to_set))) {
            LOG_WARN("fail to init a ObLSReplicaTaskDisplayInfo", KR(ret));
          } else if (OB_FAIL(add_display_info(display_info))) {
            LOG_WARN("fail to add display info", KR(ret), K(display_info));
          } else {
            LOG_INFO("success to add display info", KR(ret), K(display_info));
          }
        } else if (OB_FAIL(generate_task_key(dr_ls_info, leader_addr, task_type, task_key))) {
          LOG_WARN("fail to generate task key", KR(ret), K(dr_ls_info), K(leader_addr), K(task_type));
        } else if (OB_FAIL(check_can_generate_task(
                               acc_dr_task,
                               need_check_has_leader_while_remove_replica,
                               ls_replica->get_server(),
                               dr_ls_info,
                               task_type,
                               can_generate))) {
          LOG_WARN("fail to check can generate cancel unit migration task", KR(ret));
        } else if (can_generate) {
          if (OB_FAIL(generate_cancel_unit_migration_task(
                          is_paxos_replica_related,
                          task_key,
                          task_id,
                          leader_addr,
                          remove_member,
                          old_paxos_replica_number,
                          new_paxos_replica_number,
                          acc_dr_task))) {
            LOG_WARN("fail to build cancel unit migration task", KR(ret));
          }
        }
      }
    }
  }
  // no need to print task key, since the previous log contains that
  LOG_INFO("finish try cancel migrate unit", KR(ret), K(acc_dr_task));
  return ret;
}

int ObDRWorker::check_need_generate_migrate_to_unit_task(
    const int64_t index,
    DRLSInfo &dr_ls_info,
    share::ObLSReplica *&ls_replica,
    DRServerStatInfo *&server_stat_info,
    DRUnitStatInfo *&unit_stat_info,
    DRUnitStatInfo *&unit_in_group_stat_info,
    bool &need_generate,
    bool &is_unit_in_group_related)
{
  int ret = OB_SUCCESS;
  need_generate = false;
  is_unit_in_group_related = false;
  if (OB_FAIL(dr_ls_info.get_replica_stat(
                         index,
                         ls_replica,
                         server_stat_info,
                         unit_stat_info,
                         unit_in_group_stat_info))) {
    LOG_WARN("fail to get replica stat", KR(ret));
  } else if (OB_ISNULL(ls_replica)
                 || OB_ISNULL(server_stat_info)
                 || OB_ISNULL(unit_stat_info)
                 || OB_ISNULL(unit_in_group_stat_info)
                 || (unit_stat_info->is_in_pool() && OB_ISNULL(unit_stat_info->get_server_stat()))
                 || (unit_in_group_stat_info->is_in_pool() && OB_ISNULL(unit_in_group_stat_info->get_server_stat()))) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("replica stat unexpected",
                 KR(ret),
                 KP(ls_replica),
                 KP(server_stat_info),
                 KP(unit_stat_info),
                 KP(unit_in_group_stat_info));
  } else if ((REPLICA_TYPE_READONLY == ls_replica->get_replica_type()
      || REPLICA_TYPE_COLUMNSTORE == ls_replica->get_replica_type())
      && dr_ls_info.is_duplicate_ls()) {
    // only process F replica for duplicate ls, ignore R/C replica
    need_generate = false;
  } else if (REPLICA_STATUS_NORMAL == ls_replica->get_replica_status()
          && unit_stat_info->is_in_pool()
          && server_stat_info->get_server() != unit_stat_info->get_unit().server_
          && unit_stat_info->get_server_stat()->is_alive()
          && !unit_stat_info->get_server_stat()->is_block()) {
    need_generate = true;
    is_unit_in_group_related = false;
  } else if (REPLICA_STATUS_NORMAL == ls_replica->get_replica_status()
          && unit_in_group_stat_info->is_in_pool()
          && server_stat_info->get_server() != unit_in_group_stat_info->get_unit().server_
          && unit_in_group_stat_info->get_server_stat()->is_alive()
          && !unit_in_group_stat_info->get_server_stat()->is_block()) {
    need_generate = true;
    is_unit_in_group_related = true;
  }
  return ret;
}

int ObDRWorker::construct_extra_infos_for_generate_migrate_to_unit_task(
    DRLSInfo &dr_ls_info,
    const share::ObLSReplica &ls_replica,
    uint64_t &tenant_id,
    share::ObLSID &ls_id,
    share::ObTaskId &task_id,
    int64_t &data_size,
    int64_t &old_paxos_replica_number)
{
  int ret = OB_SUCCESS;
  if (FALSE_IT(task_id.init(GCONF.self_addr_))) {
    //shall never be here
  } else {
    tenant_id = ls_replica.get_tenant_id();
    ls_id = ls_replica.get_ls_id();
    old_paxos_replica_number = dr_ls_info.get_paxos_replica_number();
  }
  return ret;
}

int ObDRWorker::generate_migrate_to_unit_task(
    const ObDRTaskKey task_key,
    const share::ObTaskId &task_id,
    const ObReplicaMember &dst_member,
    const ObReplicaMember &src_member,
    const ObReplicaMember &data_source,
    const int64_t &old_paxos_replica_number,
    const bool is_unit_in_group_related,
    int64_t &acc_dr_task)
{
  int ret = OB_SUCCESS;
  ObMigrateLSReplicaTask migrate_task;
  ObString comment_to_set;
  if (is_unit_in_group_related) {
    comment_to_set.assign_ptr(drtask::MIGRATE_REPLICA_DUE_TO_UNIT_GROUP_NOT_MATCH,
                              strlen(drtask::MIGRATE_REPLICA_DUE_TO_UNIT_GROUP_NOT_MATCH));
  } else {
    comment_to_set.assign_ptr(drtask::MIGRATE_REPLICA_DUE_TO_UNIT_NOT_MATCH,
                              strlen(drtask::MIGRATE_REPLICA_DUE_TO_UNIT_NOT_MATCH));
  }
  if (OB_FAIL(migrate_task.build(
                         task_key,
                         task_id,
                         comment_to_set,
                         dst_member,
                         src_member,
                         data_source,
                         ObReplicaMember()/*empty force_data_source*/,
                         old_paxos_replica_number,
                         ObDRTaskPriority::LOW_PRI))) {
    LOG_WARN("fail to build migrate task", KR(ret));
  } else if (OB_FAIL(add_task_(migrate_task, acc_dr_task))) {
    LOG_WARN("fail to add task", KR(ret), K(migrate_task));
  }
  return ret;
}

int ObDRWorker::try_migrate_to_unit(
    const bool only_for_display,
    DRLSInfo &dr_ls_info,
    int64_t &acc_dr_task)
{
  int ret = OB_SUCCESS;
  int64_t replica_cnt = 0;
  if (OB_ISNULL(GCTX.config_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("config_ ptr is null", KR(ret), KP(GCTX.config_));
  } else if (!GCTX.config_->is_rereplication_enabled()) {
    // bypass
  } else if (!dr_ls_info.has_leader()) {
    LOG_WARN("has no leader, maybe not report yet",
             KR(ret), K(dr_ls_info));
  } else if (dr_ls_info.get_paxos_replica_number() <= 0) {
    LOG_WARN("paxos_replica_number is invalid, maybe not report yet",
             KR(ret), K(dr_ls_info));
  } else if (OB_FAIL(dr_ls_info.get_replica_cnt(replica_cnt))) {
    LOG_WARN("fail to get replica cnt", KR(ret), K(dr_ls_info));
  } else {
    for (int64_t index = 0; OB_SUCC(ret) && index < replica_cnt; ++index) {
      share::ObLSReplica *ls_replica = nullptr;
      DRServerStatInfo *server_stat_info = nullptr;
      DRUnitStatInfo *unit_stat_info = nullptr;
      DRUnitStatInfo *unit_in_group_stat_info = nullptr;
      bool need_generate = false;
      bool is_unit_in_group_related = false;
      if (OB_FAIL(check_need_generate_migrate_to_unit_task(
                      index,
                      dr_ls_info,
                      ls_replica,
                      server_stat_info,
                      unit_stat_info,
                      unit_in_group_stat_info,
                      need_generate,
                      is_unit_in_group_related))) {
        LOG_WARN("fail to check need generate migrate to unit task", KR(ret));
      } else if (need_generate) {
        // normal ls replica: need migrate task if the replica is not in the correct unit or unit_group
        // dup ls replica: need migrate task if the F replica is not in the correct unit (migrate unit scenario)
        //                 need type transform task if the F replica is not int the correct unit_group (alter ls ls_group_id scenario)
        if (is_unit_in_group_related && dr_ls_info.is_duplicate_ls()) {
          if (OB_FAIL(try_type_transform_for_migrate_to_unit_(
              dr_ls_info,
              *ls_replica,
              unit_in_group_stat_info,
              only_for_display,
              acc_dr_task))) {
            LOG_WARN("fail to try type transform for migrate to unit", KR(ret),
                KPC(ls_replica), K(only_for_display), K(dr_ls_info), K(acc_dr_task));
          }
        }
        // for nonduplicate ls or duplicate ls replica which can not do type transform
        if (OB_FAIL(ret)) {
        } else if (OB_FAIL(try_migrate_replica_for_migrate_to_unit_(
            dr_ls_info,
            ls_replica,
            unit_stat_info,
            unit_in_group_stat_info,
            is_unit_in_group_related,
            only_for_display,
            acc_dr_task))) {
          LOG_WARN("fail to try migrate replica for migrate to unit",
              KR(ret), K(dr_ls_info), KPC(ls_replica), KPC(unit_stat_info),
              KPC(unit_in_group_stat_info), K(only_for_display), K(acc_dr_task));
        }
      } // end need_generate
    } // end for
  }
  // no need to print task key, since the previous log contains that
  LOG_INFO("finish try migrate to unit", KR(ret), K(acc_dr_task));
  return ret;
}

int ObDRWorker::try_migrate_replica_for_migrate_to_unit_(
    DRLSInfo &dr_ls_info,
    const share::ObLSReplica *ls_replica,
    const DRUnitStatInfo *unit_stat_info,
    const DRUnitStatInfo *unit_in_group_stat_info,
    const bool is_unit_in_group_related,
    const bool only_for_display,
    int64_t &acc_dr_task)
{
  int ret = OB_SUCCESS;
  if (OB_ISNULL(ls_replica)
      || OB_ISNULL(unit_stat_info)
      || OB_ISNULL(unit_in_group_stat_info)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid args", KR(ret), KP(ls_replica), KP(unit_stat_info), KP(unit_in_group_stat_info));
  } else {
    ObDRTaskKey task_key;
    uint64_t tenant_id = 0;
    share::ObLSID ls_id;
    share::ObTaskId task_id;
    ObReplicaMember data_source;
    int64_t data_size = 0;
    int64_t old_paxos_replica_number = 0;
    bool can_generate = false;
    const bool need_check_has_leader_while_remove_replica = false;
    ObReplicaMember src_member(ls_replica->get_server(),
                                ls_replica->get_member_time_us(),
                                ls_replica->get_replica_type(),
                                ls_replica->get_memstore_percent());
    ObReplicaMember dst_member(is_unit_in_group_related
                                ? unit_in_group_stat_info->get_unit().server_
                                : unit_stat_info->get_unit().server_,
                                ObTimeUtility::current_time(),
                                ls_replica->get_replica_type(),
                                ls_replica->get_memstore_percent());
    ObString comment_to_set = "";

    if (is_unit_in_group_related) {
      comment_to_set.assign_ptr(drtask::MIGRATE_REPLICA_DUE_TO_UNIT_GROUP_NOT_MATCH,
                                strlen(drtask::MIGRATE_REPLICA_DUE_TO_UNIT_GROUP_NOT_MATCH));
    } else {
      comment_to_set.assign_ptr(drtask::MIGRATE_REPLICA_DUE_TO_UNIT_NOT_MATCH,
                                strlen(drtask::MIGRATE_REPLICA_DUE_TO_UNIT_NOT_MATCH));
    }
    if (OB_FAIL(dr_ls_info.get_default_data_source(data_source, data_size))) {
      LOG_WARN("fail to get data_size", KR(ret), K(dr_ls_info));
    } else if (OB_FAIL(construct_extra_infos_for_generate_migrate_to_unit_task(
                    dr_ls_info,
                    *ls_replica,
                    tenant_id,
                    ls_id,
                    task_id,
                    data_size,
                    old_paxos_replica_number))) {
      LOG_WARN("fail to construct extra infos for generate migrate to unit task", KR(ret));
    } else if (only_for_display) {
      ObLSReplicaTaskDisplayInfo display_info;
      if (OB_FAIL(display_info.init(
                    tenant_id,
                    ls_id,
                    ObDRTaskType::LS_MIGRATE_REPLICA,
                    ObDRTaskPriority::LOW_PRI,
                    is_unit_in_group_related
                      ? unit_in_group_stat_info->get_unit().server_
                      : unit_stat_info->get_unit().server_,
                    ls_replica->get_replica_type(),
                    old_paxos_replica_number,
                    ls_replica->get_server(),
                    ls_replica->get_replica_type(),
                    old_paxos_replica_number,
                    is_unit_in_group_related
                      ? unit_in_group_stat_info->get_unit().server_
                      : unit_stat_info->get_unit().server_,
                    comment_to_set))) {
        LOG_WARN("fail to init a ObLSReplicaTaskDisplayInfo", KR(ret));
      } else if (OB_FAIL(add_display_info(display_info))) {
        LOG_WARN("fail to add display info", KR(ret), K(display_info));
      } else {
        LOG_INFO("success to add display info", KR(ret), K(display_info));
      }
    } else if (OB_FAIL(generate_task_key(dr_ls_info, dst_member.get_server(), ObDRTaskType::LS_MIGRATE_REPLICA, task_key))) {
      LOG_WARN("fail to generate task key", KR(ret), K(dr_ls_info), K(dst_member));
    } else if (OB_FAIL(check_can_generate_task(
                            acc_dr_task,
                            need_check_has_leader_while_remove_replica,
                            ls_replica->get_server(),
                            dr_ls_info,
                            ObDRTaskType::LS_MIGRATE_REPLICA,
                            can_generate))) {
      LOG_WARN("fail to check can generate migrate to unir task", KR(ret));
    } else if (can_generate) {
      if (OB_FAIL(generate_migrate_to_unit_task(
                      task_key,
                      task_id,
                      dst_member,
                      src_member,
                      data_source,
                      old_paxos_replica_number,
                      is_unit_in_group_related,
                      acc_dr_task))) {
        LOG_WARN("fail to generate migrate to unit task", KR(ret));
      }
    }
  }
  return ret;
}

int ObDRWorker::get_task_plan_display(
    common::ObSArray<ObLSReplicaTaskDisplayInfo> &task_plan)
{
  int ret = OB_SUCCESS;
  SpinWLockGuard guard(display_tasks_rwlock_);
  task_plan.reset();
  if (OB_FAIL(task_plan.assign(display_tasks_))) {
    LOG_WARN("fail to get dsplay task stat", KR(ret), K_(display_tasks));
  }
  reset_task_plans_();
  return ret;
}

int ObDRWorker::add_display_info(const ObLSReplicaTaskDisplayInfo &display_info)
{
  int ret = OB_SUCCESS;
  SpinWLockGuard guard(display_tasks_rwlock_);
  if (!display_info.is_valid()) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(display_info));
  } else if (OB_FAIL(display_tasks_.push_back(display_info))) {
    LOG_WARN("insert replica failed", KR(ret), K(display_info));
  }
  return ret;
}

int ObDRWorker::generate_disaster_recovery_paxos_replica_number(
    const DRLSInfo &dr_ls_info,
    const int64_t curr_paxos_replica_number,
    const int64_t locality_paxos_replica_number,
    const MemberChangeType member_change_type,
    int64_t &new_paxos_replica_number,
    bool &found)
{
  int ret = OB_SUCCESS;
  found = false;
  int64_t member_list_cnt = dr_ls_info.get_member_list_cnt();
  uint64_t tenant_id = OB_INVALID_TENANT_ID;
  ObLSID ls_id;
  if (OB_UNLIKELY(member_list_cnt <= 0
                  || curr_paxos_replica_number <= 0
                  || locality_paxos_replica_number <= 0)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(member_list_cnt), K(curr_paxos_replica_number),
             K(locality_paxos_replica_number));
  } else if (MEMBER_CHANGE_ADD == member_change_type) {
    const int64_t member_list_cnt_after = member_list_cnt + 1;
    if (curr_paxos_replica_number == locality_paxos_replica_number) {
      if (locality_paxos_replica_number >= member_list_cnt_after) {
        // For example: locality is F@z1,F@z2,F@z3, suppose a member is permanently offline, curr_paxos_replica_number = 3.
        // current member_list_cnt = 2, member_list_cnt_after = 3, locality_paxos_replica_number = 3.
        // set new paxos_replica_number = 3(do not change curr_paxos_replica_number).
        new_paxos_replica_number = curr_paxos_replica_number;
        found = true;
      } else if (locality_paxos_replica_number + 1 == member_list_cnt_after) {
        // For example: locality is F@z1,F@z2,F@z3, current replica distribution is F@z2, F@z3, F@z4, curr_paxos_replica_number = 3.
        // current member_list_cnt = 3, member_list_cnt_after = 4. locality_paxos_replica_number = 3.
        // set new paxos_replica_number = 4(curr_paxos_replica_number + 1).
        // in this scenario, replica in F@z1 is added first and then replica in F@z4 is deleted.
        new_paxos_replica_number = curr_paxos_replica_number + 1;
        found = true;
      } else {
        // No need process this scenario.
        // For example: locality is F@z1,F@z2, current replica distribution is F@z2, F@z3, F@z4, curr_paxos_replica_number = 3.
        // current member_list_cnt = 3, member_list_cnt_after = 4. locality_paxos_replica_number = 2.
        // in this scenario, first we remove replica in F@z3 or F@z4(for example z3), then add replica in F@z1, finally remove replica in F@z4.
      }
    } else if (curr_paxos_replica_number > locality_paxos_replica_number) {
      if (curr_paxos_replica_number >= member_list_cnt_after) {
        // For example: locality is F@z1,F@z2, current replica distribution is F@z2, F@z3, curr_paxos_replica_number = 3.
        // current member_list_cnt = 2, member_list_cnt_after = 3. locality_paxos_replica_number = 2.
        // set new paxos_replica_number = 3(do not change curr_paxos_replica_number).
        new_paxos_replica_number = curr_paxos_replica_number;
        found = true;
      } else {
        // new member cnt greater than paxos_replica_number, not good
      }
    } else { // curr_paxos_replica_number < locality_paxos_replica_number
      if (majority(curr_paxos_replica_number + 1) <= member_list_cnt_after) {
        new_paxos_replica_number = curr_paxos_replica_number + 1;
        found = true;
      } else {
        // majority not satisfied
      }
    }
  } else if (MEMBER_CHANGE_NOP == member_change_type) {
    // 2. MEMBER_LIST not changed operation
    //    When current paxos_replica_number == locality paxos_replica_number
    //         we do not change paxos_replica_number
    //    When current paxos_replica_number > locality paxos_replica_number
    //         we try to reduce paxos_replica_number towards locality
    //    When current paxos_replica_number < locality paxos_replica_number
    //         we try to increase paxos_replica_number towards locality
    if (curr_paxos_replica_number == locality_paxos_replica_number) {
      new_paxos_replica_number = curr_paxos_replica_number;
      found = true;
    } else if (curr_paxos_replica_number > locality_paxos_replica_number) {
      if (member_list_cnt <= curr_paxos_replica_number - 1) {
        new_paxos_replica_number = curr_paxos_replica_number - 1;
        found = true;
      }
    } else {
      if (member_list_cnt > majority(curr_paxos_replica_number + 1)) {
        new_paxos_replica_number = curr_paxos_replica_number + 1;
        found = true;
      }
    }
  } else if (MEMBER_CHANGE_SUB == member_change_type) {
    int64_t member_list_cnt_after = 0;
    int64_t arb_replica_number = 0;
    // 3. REMOVE MEMBER_LIST operation
    //    When current paxos_replica_number == locality paxos_replica_number
    //         we do not change paxos_replica_number and ensure majority is satisfied
    //    When current paxos_replica_number > locality paxos_replica_number
    //         we try to reduce paxos_replica_number towards locality and ensure majority is satisfied
    //    When current paxos_replica_number < locality paxos_replica_number
    //         we do not change paxos_replica_number and ensure majority is satisfied
    //    SPECIALLY, we support 2F reduce to 1F aotumatically
    if (2 == curr_paxos_replica_number) {
      if (2 == member_list_cnt) {
        // Specially, we support 2F to 1F to solve these cases:
        // CASE 1:
        //     tenant's locality is 2F1A, F-replica in zone1 migrate and leads to source replica remains in member_list,
        //     member_list like (F-z1, F-z1, F-z2) and paxos_replica_number = 3
        //     then F-replica in zone2 permanent offline,
        //     member_list like (F-z1, F-z1) and paxos_replica_number = 2
        //     Under this case, we have to support remove one F in zone1 and later add one F in zone2
        // CASE 2:
        //     tenant's locality is 1F, F-replica in zone1 migrate and leads to source replica remains in member_list,
        //     member_list like (F-z1, F-z1) and paxos_replica_number = 2
        //     Under this case, we have to support remove one F in zone1
        new_paxos_replica_number = curr_paxos_replica_number - 1;
        found = true;
      } else {
        // do nothing
        // When current paxos_replica_number = 2
        // member_list_cnt can not be less than 2, because it will leads to no-leader
        // member_list_cnt can not be larger than 2, because member_list_cnt should <= paxos_replica_number
        // we do not raise error and remain found = false
      }
    } else if (OB_FAIL(dr_ls_info.get_ls_id(tenant_id, ls_id))) {
      LOG_WARN("fail to get tenant and ls id", KR(ret), K(dr_ls_info));
    } else if (OB_FAIL(ObShareUtil::generate_arb_replica_num(tenant_id, ls_id, arb_replica_number))) {
      LOG_WARN("fail to generate arb replica number", KR(ret), K(tenant_id), K(ls_id));
    } else {
      member_list_cnt_after = member_list_cnt - 1 + arb_replica_number;
      if (curr_paxos_replica_number <= locality_paxos_replica_number) {
        if (majority(curr_paxos_replica_number) <= member_list_cnt_after) {
          new_paxos_replica_number = curr_paxos_replica_number;
          found = true;
        }
      } else if (curr_paxos_replica_number > locality_paxos_replica_number) {
        if (majority(curr_paxos_replica_number - 1) <= member_list_cnt_after) {
          new_paxos_replica_number = curr_paxos_replica_number - 1;
          found = true;
        }
      }
    }
  } else {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid member change type", KR(ret), K(member_change_type));
  }
  FLOG_INFO("finish generating disaster recovery paxos replica number", KR(ret),
           K(dr_ls_info), K(found), K(member_list_cnt), K(curr_paxos_replica_number),
           K(locality_paxos_replica_number), K(member_change_type), K(new_paxos_replica_number));
  return ret;
}

int ObDRWorker::check_ls_only_in_member_list_or_with_flag_(
    const DRLSInfo &dr_ls_info)
{
  int ret = OB_SUCCESS;
  const share::ObLSReplica *leader_replica = nullptr;
  share::ObLSInfo inner_ls_info;
  if (OB_FAIL(dr_ls_info.get_inner_ls_info(inner_ls_info))) {
    LOG_WARN("fail to get inner ls info", KR(ret), K(dr_ls_info));
  } else if (OB_UNLIKELY(!inner_ls_info.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(inner_ls_info));
  } else if (OB_FAIL(inner_ls_info.find_leader(leader_replica))) {
    LOG_WARN("fail to find leader", KR(ret), K(inner_ls_info));
  } else if (OB_ISNULL(leader_replica)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("leader replica is null", KR(ret));
  } else if (OB_UNLIKELY(0 >= leader_replica->get_member_list().count())) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("leader member list has no member", KR(ret), "member_lsit", leader_replica->get_member_list());
  } else {
    // check member list
    for (int64_t i = 0; OB_SUCC(ret) && i < leader_replica->get_member_list().count(); ++i) {
      const share::ObLSReplica *replica = nullptr;
      const common::ObAddr &server = leader_replica->get_member_list().at(i).get_server();
      const int64_t member_time_us = leader_replica->get_member_list().at(i).get_timestamp();
      if (OB_FAIL(inner_ls_info.find(server, replica))) {
        LOG_WARN("fail to find replica", KR(ret), K(inner_ls_info), K(server));
      }
    }
    // check learner list
    for (int64_t index = 0; OB_SUCC(ret) && index < leader_replica->get_learner_list().get_member_number(); ++index) {
      common::ObMember learner_to_check;
      const share::ObLSReplica *replica = nullptr;
      if (OB_FAIL(leader_replica->get_learner_list().get_member_by_index(index, learner_to_check))) {
        LOG_WARN("fail to get learner by index", KR(ret), K(index));
      } else if (learner_to_check.is_migrating()) {
        if (OB_FAIL(inner_ls_info.find(learner_to_check.get_server(), replica))) {
          if (OB_ENTRY_NOT_EXIST == ret) {
            // good, learner with flag should not appear in inner_ls_info
            ret = OB_SUCCESS;
          } else {
            LOG_WARN("fail to find replica", KR(ret), K(inner_ls_info), K(learner_to_check));
          }
        } else {
          ret = OB_STATE_NOT_MATCH;
          LOG_WARN("read only replica with migrating-flag should not appear in inner_ls_info",
                   KR(ret), K(learner_to_check), K(inner_ls_info));
        }
      } else if (OB_FAIL(inner_ls_info.find(learner_to_check.get_server(), replica))) {
        LOG_WARN("fail to find read only replica", KR(ret), K(inner_ls_info), K(learner_to_check));
      }
    }
  }
  return ret;
}
} // end namespace rootserver
} // end namespace oceanbase
