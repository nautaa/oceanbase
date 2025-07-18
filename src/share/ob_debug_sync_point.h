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

#ifndef OCEANBASE_COMMON_OB_DEBUG_SYNC_POINT_H_
#define OCEANBASE_COMMON_OB_DEBUG_SYNC_POINT_H_

#include "lib/utility/ob_macro_utils.h"

namespace oceanbase
{
namespace common
{
class ObString;

#define OB_DEBUG_SYNC_POINT_DEF(ACT)                               \
    ACT(INVALID_DEBUG_SYNC_POINT, = 0)                             \
    ACT(NOW,)                                                      \
    ACT(MAJOR_FREEZE_BEFORE_SYS_COORDINATE_COMMIT,)                \
    ACT(BEFORE_REBALANCE_TASK_EXECUTE,)                            \
    ACT(REBALANCE_TASK_MGR_BEFORE_EXECUTE_OVER,)                   \
    ACT(UNIT_BALANCE_BEFORE_PARTITION_BALANCE,)                    \
    ACT(BEFORE_UNIT_MANAGER_LOAD,)                                 \
    ACT(BEFORE_INNER_SQL_COMMIT,)                                  \
    ACT(BEFORE_ASYNC_PT_UPDATE_TASK_EXECUTE,)                      \
    ACT(AFTER_ASYNC_PT_UPDATE_TASK_EXECUTE,)                       \
    ACT(DAILY_MERGE_BEFORE_RESTORE_LEADER_POS,)                    \
    ACT(SWITCH_LEADER_BEFORE_SYS_COORDINATE_COMMIT,)               \
    ACT(START_UNIT_BALANCE,)                                       \
    ACT(BEFORE_TRY_DELETE_SERVERS,)                                \
    ACT(BEFORE_TRY_FREEZE_PARTITION_TABLE,)                        \
    ACT(BEFORE_WRITE_CHECK_POINT,)                                 \
    ACT(BEFORE_COMMIT_CHECK_POINT,)                                \
    ACT(BEFORE_PARTITION_MERGE_COMMIT,)                            \
    ACT(BEFORE_PARTITION_MIGRATE_COMMIT,)                          \
    ACT(BEFORE_SEND_UPDATE_INDEX_STATUS,)                          \
    ACT(AFTER_INSERT_FLAG_REPLICA,)                                \
    ACT(OBSERVICE_GET_LEADER_CANDIDATES,)                          \
    ACT(CHECK_NEW_TENANT,)                                         \
    ACT(BEFORE_CHECK_MAJOR_FREEZE_DONE,)                           \
    ACT(UPDATE_WITH_PARTITION_FLAG_DONE,)                          \
    ACT(MAJOR_FREEZE_AFTER_SYS_COMMIT,)                            \
    ACT(MAJOR_FREEZE_AFTER_ROOTSERVER_COMMIT,)                     \
    ACT(SWITCH_LEADER_AFTER_SYS_COMMIT,)                           \
    ACT(SWITCH_LEADER_AFTER_ROOTSERVER_COMMIT,)                    \
    ACT(AFTER_WRITE_MAJOR_FREEZE_COMMIT_LOG,)                      \
    ACT(BEFORE_AUTO_COORDINATE,)                                   \
    ACT(DELAY_PARTITION_SERVICE_FREEZE_LOG_TASK,)                  \
    ACT(MINOR_MERGE_TIMER_TASK,)                                   \
    ACT(MERGE_TASK_PROCESS,)                                       \
    ACT(MAJOR_MERGE_TASK_PROCESS,)                                 \
    ACT(MAJOR_MERGE_PREPARE_TASK_PROCESS,)                         \
    ACT(COMPACTION_REPORT_PROCESS,)                                \
    ACT(MINOR_MERGE_SCHEDULE,)                                     \
    ACT(DELAY_INDEX_WRITE,)                                        \
    ACT(BEFORE_MINOR_FREEZE_GET_BASE_STORAGE_INFO,)                \
    ACT(BEFORE_IS_IN_SYNC_SET,)                                    \
    ACT(BEFORE_MIGRATE_PROCESS,)                                    \
    ACT(BEFORE_MIGRATE_COPY_BASE_DATA,)                            \
    ACT(BEFORE_MIGRATE_COPY_LOGIC_DATA,)                            \
    ACT(BEFORE_MIGRATE_WAIT_REPLAY,)                               \
    ACT(BEFORE_MIGRATE_ENABLE_REPLAY,)                               \
    ACT(BEFORE_MIGRATE_FINISH,)                               \
    ACT(BEFORE_BUILD_LOCAL_INDEX,)               \
    ACT(BEFORE_BALANCE_SEND_RPC,) \
    ACT(END_PRODUCE_PG_REPLICATE_TASK,) \
    ACT(BEFORE_SWEEP_MACRO_BLOCK,) \
    ACT(BEFORE_REPORT_BUILD_INDEX,) \
    ACT(DELAY_SCHEDULE_MERGE,) \
    ACT(DELAY_WRITE_CHECKPOINT,) \
    ACT(BEFORE_DO_GLOBAL_MAJOR_FREEZE,) \
    ACT(MAJOR_MERGE_PREPARE,)    \
    ACT(STOP_BALANCE_EXECUTOR,) \
    ACT(BEFORE_STOP_RS,) \
    ACT(BEFORE_ANSWER_RS_STATUS, )\
    ACT(DELAY_CHANGE_REPLICA_CALLBACK, )\
    ACT(AFTER_POST_REMOVE_REPLICA_MC_MSG, )\
    ACT(AFTER_POST_ADD_REPLICA_MC_MSG, )\
    ACT(DELAY_REMOVE_REPLICA, )\
    ACT(BEFORE_CHECK_ALL_PARTITION_MERGED, )\
    ACT(BEFORE_UPDATE_INDEX_STATUS, )\
    ACT(AFTER_MIGRATION_MARK, )\
    ACT(BEFORE_CHECK_FREEZING,)\
    ACT(AFTER_RELEASE_STORES,)\
    ACT(AFTER_SET_FREEZING,)\
    ACT(BEFORE_REPORT_SELF,)\
    ACT(BEFORE_START_RS,) \
    ACT(BEFORE_MAIN_SSTBALE_FINISH_TASK,)\
    ACT(BEFORE_INDEX_SSTBALE_FINISH_TASK,)\
    ACT(HANG_MERGE,)\
    ACT(BEFORE_BACKGROUND_WASH,) \
    ACT(HANG_HEART_BEAT_ON_RS,)\
    ACT(AFTER_BUILD_INDEX_FINISH,)\
    ACT(BEFORE_PARTITION_REPORT_CALLBACK,)\
    ACT(BEFORE_REBALANCE_SEND_TASK_RPC,)\
    ACT(BEFORE_BATCH_REMOVE_MEMBER,)\
    ACT(BEFORE_BATCH_ADD_MEMBER,)\
    ACT(UNIT_MANAGER_WAIT_FOR_TIMEOUT,)\
    ACT(BEFORE_RECYCLE_INTERM_RESULT,)\
    ACT(BEFORE_UPDATE_INDEX_BUILD_VERSION,)\
    ACT(AFTER_GLOBAL_INDEX_GET_SNAPSHOT,)\
    ACT(BEFORE_CHECK_GLOBAL_UNIQUE_INDEX,)\
    ACT(BEFORE_COPY_GLOBAL_INDEX,)\
    ACT(BEFORE_LOCAL_INDEX_WAIT_TRANS_END,)\
    ACT(BEFORE_LOCAL_INDEX_WAIT_TRANS_END_MID,)\
    ACT(BEFORE_LOCAL_INDEX_WAIT_SNAPSHOT_READY,)\
    ACT(BEFORE_LOCAL_INDEX_WAIT_SNAPSHOT_READY_MID,)\
    ACT(BEFORE_LOCAL_INDEX_CHOOSE_BUILD_INDEX_REPLICA,)\
    ACT(BEFORE_LOCAL_INDEX_CHOOSE_BUILD_INDEX_REPLICA_MID,)\
    ACT(BEFORE_LOCAL_INDEX_WAIT_CHOOSE_OR_BUILD_INDEX_END,)\
    ACT(BEFORE_LOCAL_INDEX_WAIT_CHOOSE_OR_BUILD_INDEX_END_MID,)\
    ACT(BEFORE_LOCAL_INDEX_COPY_BUILD_INDEX_DATA,)\
    ACT(BEFORE_LOCAL_INDEX_COPY_BUILD_INDEX_DATA_MID,)\
    ACT(BEFORE_LOCAL_INDEX_UNIQUE_INDEX_CHECKING,)\
    ACT(BEFORE_LOCAL_INDEX_UNIQUE_INDEX_CHECKING_MID,)\
    ACT(BEFORE_LOCAL_INDEX_WAIT_REPORT_STATUS,)\
    ACT(BEFORE_LOCAL_INDEX_WAIT_REPORT_STATUS_MID,)\
    ACT(BEFORE_LOCAL_INDEX_END,)\
    ACT(BEFORE_LOCAL_INDEX_END_MID,)\
    ACT(DEFORE_OBS_CREATE_PARTITION,)\
    ACT(DEFORE_FETCH_LOGIC_ROW_SRC,)\
    ACT(BEFORE_BUILD_MIGRATE_PARTITION_INFO,)\
    ACT(BEFORE_BUILD_MIGRATE_PARTITION_INFO_USER_TABLE,)\
    ACT(BEFORE_GET_MAJOR_MGERGE_TABLES,)\
    ACT(BEFORE_GET_MINOR_MGERGE_TABLES,)\
    ACT(BEFORE_GET_MAJOR_MERGE_TABLE_IDS,)\
    ACT(BEFORE_GET_MINOR_MERGE_TABLE_IDS,)\
    ACT(DAILY_MERGE_SCHEDULER_IDLE,)\
    ACT(BEFORE_CHECK_LOCALITY,)\
    ACT(BLOCK_GARBAGE_COLLECTOR,)\
    ACT(BEFORE_CREATE_INDEX_TASK,)\
    ACT(BEFORE_CLEAR_MIGRATE_STATUS,)\
    ACT(BEFORE_MERGE_CHECK_TASK,)\
    ACT(BEFORE_FINISH_MIGRATE_TASK,)\
    ACT(BEFORE_BUILD_LOCAL_INDEX_SCAN,)\
    ACT(BEFORE_CREATE_TABLE_TRANS_COMMIT,) \
    ACT(AFTER_TENANT_BALANCE_GATHER_STAT,)\
    ACT(BEFORE_RESTORE_PARTITIONS,)\
    ACT(BEFORE_BATCH_PROCESS_TASK,)\
    ACT(BEFORE_INSERT_ROWS,)\
    ACT(AFTER_INSERT_ROWS,)\
    ACT(AFTER_INSERT_ROW,)\
    ACT(AFTER_TABLE_SCAN,)\
    ACT(BEFORE_BUILD_LOCAL_INDEX_REFRESH_TABLES,)\
    ACT(BEFORE_BUILD_LOCAL_INDEX_REFRESH_TABLES_MID,)\
    ACT(BEFORE_SLOG_UPDATE_FLUSH_CURSOR,)\
    ACT(BEFORE_SCHEDULE_ALL_PARTITIONS,)\
    ACT(BEFORE_OBSERVER_SCHEDULE_MIGRATE,)\
    ACT(BEFORE_CREATE_TENANT_TRANS_TWO,)\
    ACT(MERGE_PARTITION_TASK,)\
    ACT(TRUNCATE_PARTITION_TRANS,)\
    ACT(BEFORE_CHANGE_MEMBER_LIST,)\
    ACT(BEFORE_FOLLOWER_CLUSTER_SYNC_SCHEMA,)\
    ACT(BEFORE_UPDATE_STANBY_FREEZE_INFO,)\
    ACT(BEFORE_PROCESS_USER_SCHEMA,)\
    ACT(BEFORE_CREATE_TENANT_TRANS_THREE,)\
    ACT(BEFORE_GEN_NEXT_SCHEMA_VERSION,)\
    ACT(BLOCK_CALC_WRS,)\
    ACT(BEFORE_UPDATE_CORE_TABLE,)\
    ACT(BEFORE_REPLAY_ADD_PARTITION_TO_PG_CLOG,)\
    ACT(BEFORE_SEND_FLASHBACK_USER_RPC,)\
    ACT(BEFORE_SEND_FLASHBACK_RPC,)\
    ACT(BEFORE_UPDATE_GLOBAL_INDEX_STATUS,)\
    ACT(BEFORE_UPDATE_LOCAL_INDEX_STATUS,)\
    ACT(FINISH_ABORT_CREATE_INDEX,)\
    ACT(BEFORE_SWITCHOVER_FROM_PENDING_TO_PRIMARY,)\
    ACT(AFTER_WRITE_USER_TABLE_CUTDATA_CLOG,)\
    ACT(BEFORE_SET_CLUSTER_NAME_CONFIGURL,)\
    ACT(AFTER_SET_CLUSTER_NAME_CONFIGURL,)\
    ACT(AFTER_REMOVE_OTHER_CLUSTER,)\
    ACT(AFTER_SET_ALL_CLUSTER_CLUSTER_NAME,)\
    ACT(AFTER_SET_ALL_ZONE_CLUSTER_NAME,)\
    ACT(SENDING_OBS_DISCONNECT_CLUSTER,)\
    ACT(AFTER_OBS_DISCONNECT_CLUSTER,)\
    ACT(AFTER_WRITE_INNER_TABLE_CUTDATA_CLOG,)\
    ACT(INNER_TABLE_IN_CUTDATA_STATUS,)\
    ACT(USER_TABLE_IN_CUTDATA_STATUS,)\
    ACT(INNER_TABLE_CUTDATA_FINISH,)\
    ACT(USER_TABLE_CUTDATA_FINISH,)\
    ACT(BEFORE_FAILOVER_FROM_PENDING_TO_FLASHBACK,)\
    ACT(BEFORE_SWITCHOVER_TO_PENDING,)\
    ACT(BEFORE_ADD_SWITCHOVER_TASK,)\
    ACT(WAIT_REFRESH_TENANT_SCHEMA_VER,)\
    ACT(BEFORE_WRITE_PARTITION_CHECK_POINT,)\
    ACT(BEFORE_UPDATE_FREEZE_SNAPSHOT_INFO,)\
    ACT(BEFORE_CREATE_PG_PARTITION,)\
    ACT(BEFORE_MERGE_FINISH,)\
    ACT(BEFORE_FREEZE_ASYNC_TASK,)\
    ACT(END_SEND_FLASHBACK_INNER_RPC,)\
    ACT(DEBUG_FAILOVER_FLASHBACK_INNER,)\
    ACT(CREATE_TABLE_BEFORE_PUBLISH_SCHEMA,)\
    ACT(BEFORE_DAILY_MERGE_RUN,)\
    ACT(AFTER_ADD_SWITCHOVER_TASK,)\
    ACT(HUNG_SWITCH_TASK_QUEUE,)\
    ACT(FETCH_MACRO_BLOCK,)\
    ACT(BEFORE_WRITE_START_WORKING,)\
    ACT(BEFORE_SYNC_LOG_SUCCESS,)\
    ACT(BEFORE_DIST_COMMIT,)\
    ACT(BLOCK_FREEZE_INFO_UPDATE,)\
    ACT(BEFROE_DO_ROOT_BACKUP,)\
    ACT(BEFROE_DO_LOG_ARCHIVE_SCHEDULER,)\
    ACT(BEFROE_DO_STOP_TENANT_ARCHIVE,)\
    ACT(WRTIE_EXTERN_LOG_ARCHIVE_BACKUP_INFO,)\
    ACT(FAILED_TO_PROCESS_TO_PRIMARY,)\
    ACT(BEFORE_REFRESH_CLUSTER_ID,)\
    ACT(BEFORE_FIX_TENANT_SCHEMA_VERSION,)\
    ACT(BACKUP_INFO_PREPARE,)\
    ACT(BACKUP_INFO_SCHEDULER,)\
    ACT(BACKUP_INFO_BEFOR_DOING,)\
    ACT(BACKUP_INFO_BEFOR_CLEANUP,)\
    ACT(BACKUP_INFO_BEFOR_NORMAL_TENNAT_STOP,)\
    ACT(BACKUP_INFO_BEFOR_SYS_TENNAT_STOP,)\
    ACT(BACKUP_TASK_BEFOR_DOING,)\
    ACT(BACKUP_TASK_BEFOR_FINISH,)\
    ACT(BEFORE_PHYSICAL_RESTORE_TENANT,)\
    ACT(BEFORE_PHYSICAL_RESTORE_SYS_REPLICA,)\
    ACT(BEFORE_PHYSICAL_RESTORE_UPGRADE_PRE,)\
    ACT(BEFORE_PHYSICAL_RESTORE_DO_UPGRADE_PRE,)\
    ACT(BEFORE_PHYSICAL_RESTORE_UPGRADE_POST,)\
    ACT(BEFORE_PHYSICAL_RESTORE_DO_UPGRADE_POST,)\
    ACT(BEFORE_PHYSICAL_RESTORE_USER_REPLICA,)\
    ACT(BEFORE_PHYSICAL_RESTORE_REPLICA,)\
    ACT(BEFORE_PHYSICAL_RESTORE_SET_MEMBER_LIST,)\
    ACT(BEFORE_PHYSICAL_RESTORE_MODIFY_SCHEMA,)\
    ACT(BEFORE_PHYSICAL_RESTORE_USER_PARTITIONS,)\
    ACT(BEFORE_PHYSICAL_RESTORE_REBUILD_INDEX,)\
    ACT(BEFORE_PHYSICAL_RESTORE_POST_CHECK,)\
    ACT(BEFORE_PHYSICAL_RESTORE_INIT_LS,)\
    ACT(BEFORE_PHYSICAL_RESTORE_WAIT_LS_FINISH,)\
    ACT(BACKUP_BEFROE_CHOOSE_SRC,)\
    ACT(BACKUP_BEFROE_BUILD_TABLE_PARTITION_INFO,)\
    ACT(HANG_UPDATE_RS_LIST,)\
    ACT(PARTITION_BACKUP_TASK_BEFORE_ADD_TASK_IN_MGR,)\
    ACT(HANG_BEFORE_RESOLVER_FINISH,)\
    ACT(AFTER_GET_MERGE_LOG_ID,)\
    ACT(FAST_MIGRATE_AFTER_MIGRATE_OUT_CREATED,)\
    ACT(FAST_MIGRATE_AFTER_SUSPEND_SRC,)\
    ACT(NOTIFY_START_ARCHIVE_SUCC,)\
    ACT(LOG_ARCHIVE_SENDER_HANDLE,)\
    ACT(LOG_ARCHIVE_PUSH_LOG,)\
    ACT(END_PROCESS_USER_SCHEMA,)\
    ACT(BEFROE_LOG_ARCHIVE_SCHEDULE_PREPARE,)\
    ACT(BEFROE_LOG_ARCHIVE_SCHEDULE_BEGINNING,)\
    ACT(BEFROE_LOG_ARCHIVE_SCHEDULE_DOING,)\
    ACT(BEFROE_LOG_ARCHIVE_SCHEDULE_SUSPENDING,)\
    ACT(BEFROE_LOG_ARCHIVE_SCHEDULE_STOPPING,)\
    ACT(BEFROE_LOG_ARCHIVE_DO_CHECKPOINT,)\
    ACT(AFTER_FREEZE_BEFORE_MINI_MERGE,)\
    ACT(BEFORE_SEND_HB,)\
    ACT(HUNG_HEARTBEAT_CHECK,)\
    ACT(BLOCK_WEAK_READ_TIMESTAMP,)\
    ACT(REPLAY_REDO_LOG,)\
    ACT(BEFORE_RECYCLE_PHYSICAL_RESTORE_JOB,)\
    ACT(BACKUP_DELETE_STATUS_DOING,)\
    ACT(BACKUP_BEFORE_TRIGGER_FREEZE_PIECES,)\
    ACT(BACKUP_BEFORE_FROZEN_PIECES,)\
    ACT(BACKUP_DATA_SYS_CLEAN_STATUS_DOING,)\
    ACT(BACKUP_DATA_NORMAL_TENANT_CLEAN_STATUS_DOING,)\
    ACT(BACKUP_DELETE_STATUS_INIT,)\
    ACT(BACKUP_DELETE_STATUS_COMPLETED,)\
    ACT(BACKUP_DELETE_TASK_STATUS_DOING,)\
    ACT(BACKUP_DELETE_TASK_DEAL_FAILED,)\
    ACT(BACKUP_DELETE_LS_TASK_STATUS_DOING,)\
    ACT(SYNC_PG_AND_REPLAY_ENGINE_DEADLOCK,)\
    ACT(BACKUP_DATA_VALIDATE_STATUS_SCHEDULE,)\
    ACT(BACKUP_DATA_VALIDATE_STATUS_DOING,)\
    ACT(MIGRATE_BEFORE_CREATE_REPLICA_SUB,)\
    ACT(MIGRATE_AFTER_CREATE_REPLICA_SUB,)\
    ACT(MIGRATE_AFTER_CREATE_PG_LOCK_DIR,)\
    ACT(MIGRATE_AFTER_CREATE_TASK_FILE,)\
    ACT(MIGRATE_BEFORE_CREATE_MIGRATE_IN,)\
    ACT(MIGRATE_AFTER_CREATE_MIGRATE_IN,)\
    ACT(MIGRATE_BEFORE_MARK_TASK_STATUS,)\
    ACT(MIGRATE_AFTER_MARK_TASK_STATUS,)\
    ACT(MIGRATE_AFTER_DELETE_TASK_FILE,)\
    ACT(MIGRATE_AFTER_DELETE_PG_LOCK_DIR,)\
    ACT(MIGRATE_AFTER_REMOVE_REPLICA_SUB,)\
    ACT(MIGRATE_AFTER_CREATE_MIGRATE_OUT,)\
    ACT(FAST_RECOVER_BEFORE_PREPROCESS,)\
    ACT(FAST_RECOVER_AFTER_PREPROCESS,)\
    ACT(FAST_RECOVER_BEFORE_PROCESS,)\
    ACT(FAST_RECOVER_MID_PROCESS1,)\
    ACT(FAST_RECOVER_MID_PROCESS2,)\
    ACT(FAST_RECOVER_MID_PROCESS3,)\
    ACT(FAST_RECOVER_MID_PROCESS4,)\
    ACT(FAST_RECOVER_AFTER_PROCESS,)\
    ACT(FAST_RECOVER_REPLAY_CLOG,)\
    ACT(ADD_TRIGGER_BEFORE_MAP,)\
    ACT(DEL_TRIGGER_BEFORE_MAP,)\
    ACT(STANDBY_CLUSTER_PARTITION_CREATE,)\
    ACT(AFTER_PREPARE_TENANT_BEGINNING_STATUS,)\
    ACT(AFTER_PREPARE_TENANT_BACKUP_BACKUP_BEGINNING,)\
    ACT(PREPARE_TENANT_BEGINNING_STATUS,)\
    ACT(DOING_MARK_AND_SWEEP,)\
    ACT(BEFORE_ALTER_TABLE_PARTITION,)\
    ACT(BEFORE_FINISH_SET_PROTECTION_MODE,)\
    ACT(BEFORE_STANDBY_HEARTBEAT,)\
    ACT(BEFORE_UPRADE_SYSTEM_VARIABLE,)\
    ACT(BEFORE_DO_MINOR_FREEZE,)\
    ACT(BEFORE_UPDATE_RESTORE_FLAG_RESTORE_LOG,)\
    ACT(SLOW_TXN_DURING_2PC_COMMIT_PHASE_FOR_PHYSICAL_BACKUP_1055,)\
    ACT(BEFORE_DEAL_WITH_FAILED_BACKUP_BACKUPSET_TASK,)\
    ACT(BEFORE_START_BACKUP_ARCHIVELOG_TASK,)\
    ACT(SYNC_REPORT,)\
    ACT(BEFORE_STANDBY_FINISH_REPLY_SNAPSHOT_SCHEMA,)\
    ACT(MIGRATE_TASK_EXEC_POINT,)\
    ACT(BEFORE_PERSIST_MEMBER_LIST,)\
    ACT(BEFORE_SEND_SET_MEMBER_LIST_RPC,)\
    ACT(BEFORE_ALTER_TABLE_COLUMN,)\
    ACT(BEFORE_REPORT_BACKUP_BACKUPSET_TASK,)\
    ACT(BEFORE_FINISH_BACKUP_ARCHIVELOG_TASK,)\
    ACT(BEFORE_BACKUP_BACKUPSET_FINISH,)\
    ACT(BACKUP_BACKUPSET_COPYING,)\
    ACT(BEFORE_BACKUP_BACKUPPIECE_TASK_COMMIT,)\
    ACT(BEFORE_CHECK_BACKUP_TASK_DATA_AVAILABLE,)\
    ACT(BLOCK_CLOG_PRIMARY_RECONFIRM,)\
    ACT(DROP_COLUMN_NOT_STORED_IN_MINOR,)\
    ACT(DROP_COLUMN_NOT_STORED_IN_MAJOR,)\
    ACT(BEFORE_FORCE_DROP_SCHEMA,)\
    ACT(DDL_REDEFINITION_LOCK_TABLE,)\
    ACT(DDL_REDEFINITION_WAIT_TRANS_END,)\
    ACT(DDL_REDEFINITION_HOLD_SNAPSHOT,)\
    ACT(DDL_REDEFINITION_WRITE_BARRIER_LOG,)\
    ACT(BEFORE_DDL_TABLE_MERGE_TASK,)\
    ACT(BEFORE_LOB_META_TABELT_DDL_MERGE_TASK,)\
    ACT(TABLE_REDEFINITION_REPLICA_BUILD,)\
    ACT(TABLE_REDEFINITION_COPY_TABLE_INDEXES,)\
    ACT(TABLE_REDEFINITION_COPY_TABLE_FOREIGN_KEYS,)\
    ACT(TABLE_REDEFINITION_COPY_TABLE_CONSTRAINTS,)\
    ACT(TABLE_REDEFINITION_TAKE_EFFECT,)\
    ACT(TABLE_REDEFINITION_WRITE_BARRIER_LOG,)\
    ACT(TABLE_REDEFINITION_FAIL,)\
    ACT(TABLE_REDEFINITION_SUCCESS,)\
    ACT(COLUMN_REDEFINITION_REPLICA_BUILD,)\
    ACT(COLUMN_REDEFINITION_COPY_TABLE_INDEXES,)\
    ACT(COLUMN_REDEFINITION_COPY_TABLE_CONSTRAINTS,)\
    ACT(COLUMN_REDEFINITION_COPY_TABLE_FOREIGN_KEYS,)\
    ACT(COLUMN_REDEFINITION_TAKE_EFFECT,)\
    ACT(PARTITION_SPLIT_PREPARE,) \
    ACT(PARTITION_SPLIT_WAIT_FREEZE_END,)\
    ACT(PARTITION_SPLIT_WAIT_COMPACTION_END,)\
    ACT(PARTITION_SPLIT_WAIT_DATA_TABLET_SPLIT_END,)\
    ACT(PARTITION_SPLIT_WAIT_LOCAL_INDEX_SPLIT_END,)\
    ACT(PARTITION_SPLIT_WAIT_LOB_TABLET_SPLIT_END,)\
    ACT(PARTITION_SPLIT_WAIT_TRANS_END,)\
    ACT(PARTITION_SPLIT_TAKE_EFFECT,)\
    ACT(PARTITION_SPLIT_SUCCESS,)\
    ACT(PARTITION_SPLIT_REPLAY_CREATE_TABLET,)\
    ACT(CREATE_INDEX_WAIT_TRANS_END,)\
    ACT(CREATE_INDEX_REPLICA_BUILD,)\
    ACT(CREATE_INDEX_VERIFY_CHECKSUM,)\
    ACT(CREATE_INDEX_TAKE_EFFECT,)\
    ACT(CREATE_INDEX_FAILED,)\
    ACT(CREATE_INDEX_SUCCESS,)\
    ACT(CONSTRAINT_WAIT_TRANS_END,)\
    ACT(CONSTRAINT_VALIDATE,)\
    ACT(CONSTRAINT_SET_VALID,)\
    ACT(CONSTRAINT_FAIL,)\
    ACT(CONSTRAINT_ROLLBACK_FAILED_CHECK_CONSTRAINT_BEFORE_ALTER_TABLE,)\
    ACT(CONSTRAINT_ROLLBACK_FAILED_FK_BEFORE_ALTER_TABLE,)\
    ACT(CONSTRAINT_BEFORE_SET_FK_VALIDATED_BEFORE_ALTER_TABLE,)\
    ACT(CONSTRAINT_BEFORE_SET_CHECK_CONSTRAINT_VALIDATED_BEFORE_ALTER_TABLE,)\
    ACT(CONSTRAINT_SUCCESS,)\
    ACT(BEFORE_PERSIST_LS_TASK,)\
    ACT(BEFORE_SEND_RESTORE_PARTITIONS_RPC,)\
    ACT(BEFORE_CREATE_USER_TENANT,)\
    ACT(BEFORE_CREATE_META_TENANT,)\
    ACT(BEFORE_CREATE_TENANT_END,)\
    ACT(BEFORE_CREATE_USER_LS,)\
    ACT(BEFORE_BACKUP_META,)\
    ACT(BEFORE_BACKUP_DATA,)\
    ACT(BEFORE_BACKUP_PREPARE_TASK,)\
    ACT(BEFORE_BACKUP_MAJOR_SSTABLE,)\
    ACT(BEFORE_BACKUP_BUILD_INDEX,)\
    ACT(BEFORE_BACKUP_COMPLEMENT_LOG,)\
    ACT(BEFORE_BACKUP_FINISH,)\
    ACT(BEFORE_MIGRATE_FETCH_TABLET_INFO,)\
    ACT(BEFORE_MIGRATE_FETCH_SSTABLE_MACRO_INFO,)\
    ACT(BEFORE_MIGRATE_FETCH_MACRO_BLOCK,)\
    ACT(BEFORE_ADD_BACKUP_TASK_INTO_SCHEDULER,)\
    ACT(BEFORE_MIGRATION_BUILD_TABLET_SSTABLE_INFO,)\
    ACT(BEFORE_MIGRATION_TABLET_COPY_SSTABLE,)\
    ACT(AFTER_MIGRATION_LOAD_LS_INNER_TABLET,)\
    ACT(BEFORE_MIGRATION_ENABLE_LOG,)\
    ACT(AFTER_MIGRATION_ENABLE_LOG,)\
    ACT(BEFORE_RELEASE_DDL_KV,)\
    ACT(BEFORE_DDL_LOB_META_TABLET_MDS_DUMP,)\
    ACT(BEFORE_DDL_CHECKPOINT,)\
    ACT(AFTER_DDL_WRITE_MACRO_BLOCK,)\
    ACT(BEFORE_REPLAY_DDL_MACRO_BLOCK,)\
    ACT(BEFORE_REPLAY_DDL_PREPRARE,)\
    ACT(BEFORE_REPLAY_DDL_COMMIT,)\
    ACT(BEFORE_BACKUP_UESR_META,)\
    ACT(BEFORE_BACKUP_META_FINISH,)\
    ACT(AFTER_BACKUP_META_FINISH,)\
    ACT(BEFORE_INSERT_UERR_RECOVER_TABLE_JOB,)\
    ACT(BEFORE_GENERATE_IMPORT_TABLE_TASK,)\
    ACT(BEFORE_RECOVER_UESR_RECOVER_TABLE_JOB,)\
    ACT(BEFORE_RESTORE_AUX_TENANT,)\
    ACT(BEFORE_BACKUP_SYS_TABLETS,)\
    ACT(BEFORE_WRITE_DDL_PREPARE_LOG,)\
    ACT(AFTER_REMOTE_WRITE_DDL_PREPARE_LOG,)\
    ACT(BEFORE_CHECK_ALL_LS_HAS_LEADER,)\
    ACT(BEFORE_INDEX_SSTABLE_BUILD_TASK_SEND_SQL,)\
    ACT(BEFORE_CHECK_FK_DATA_VALID_SEND_SQL,)\
    ACT(BEFORE_CHECK_CONSTRAINT_VALID_SEND_SQL,)\
    ACT(BEFORE_EXECUTE_CTAS_CLEAR_SESSION_ID,)\
    ACT(BEFORE_RESTORE_START,)\
    ACT(BEFORE_RESTORE_SYS_TABLETS,)\
    ACT(BEFORE_RESTORE_TABLETS_META,)\
    ACT(BEFORE_RESTORE_MINOR,)\
    ACT(BEFORE_DO_FLASHBACK,)\
    ACT(PREPARE_FLASHBACK_FOR_SWITCH_TO_PRIMARY,)\
    ACT(SWITCHING_TO_STANDBY,)\
    ACT(BEFORE_RECOVER_USER_LS,)\
    ACT(BEFORE_PREPARE_FLASHBACK,)\
    ACT(BLOCK_STANDBY_REFRESH_SCHEMA,)\
    ACT(BLOCK_CREATE_STANDBY_TENANT_END,)\
    ACT(BEFORE_LS_RESTORE_SYS_TABLETS,)\
    ACT(BEFORE_WAIT_RESTORE_SYS_TABLETS,)\
    ACT(BEFORE_WAIT_RESTORE_TABLETS_META,)\
    ACT(BEFORE_WAIT_LS_RESTORE_TO_CONSISTENT_SCN,)\
    ACT(BEFORE_WAIT_QUICK_RESTORE,)\
    ACT(BEFORE_WAIT_MAJOR_RESTORE,)\
    ACT(SWAP_ORIG_AND_HIDDEN_TABLE_BEFORE_PUBLISH_SCHEMA,)\
    ACT(BEFORE_RESTORE_MAJOR,)\
    ACT(BEFORE_UPDATE_RECOVERY_UNTIL_SCN,)\
    ACT(BEFORE_ARCHIVE_FETCH_LOG,)\
    ACT(WHILE_LEADER_RESTORE_GROUP_TABLET,)\
    ACT(BEFORE_LEADER_RESTORE_GROUP_TABLET,)\
    ACT(AFTER_DATA_TABLETS_MIGRATION,)\
    ACT(MERGE_PARTITION_FINISH_TASK,)\
    ACT(RS_VALIDATE_CHECKSUM,)\
    ACT(RS_CHECK_MERGE_PROGRESS,)\
    ACT(AFTER_CHANGE_MIGRATION_STATUS_HOLD,)\
    ACT(AFTER_CREATE_META_TENANT_SYS_LOGSTREAM,)\
    ACT(AFTER_CREATE_USER_TENANT_SYS_LOGSTREAM,)\
    ACT(AFTER_SCHEDULE_RESTORE_MINOR_DAG_NET,)\
    ACT(BEFORE_WAIT_RESTORE_TENANT_FINISH,)\
    ACT(BEFORE_SEND_DRTASK_RPC,)\
    ACT(BEFORE_SEND_MIGRATE_REPLICA_DRTASK,)\
    ACT(BEFORE_RS_DEAL_WITH_RPC,)\
    ACT(BEFORE_DELETE_DRTASK_FROM_INNER_TABLE,)\
    ACT(BEFORE_FINISH_LOCALITY,)\
    ACT(BEFORE_TRY_MIGRATE_UNIT,)\
    ACT(BEFORE_TRY_DISASTER_RECOVERY,)\
    ACT(BEFORE_TRY_LOCALITY_ALIGNMENT,)\
    ACT(BEFORE_TABLET_MIGRATION_GENERATE_NEXT_DAG,)\
    ACT(BEFORE_TABLET_GROUP_MIGRATION_GENERATE_NEXT_DAG,)\
    ACT(BEFORE_BACKUP_TASK_FINISH,)\
    ACT(BEFORE_UPDATE_LS_META_TABLE,)\
    ACT(BLOCK_TENANT_SYNC_SNAPSHOT_INC,)\
    ACT(AFTER_FLASHBACK_CLOG,)\
    ACT(BEFORE_LOAD_ARCHIVE_ROUND,)\
    ACT(BEFORE_PREPARE_MIGRATION_TASK,)\
    ACT(BEFORE_INITIAL_MIGRATION_TASK,)\
    ACT(BEFORE_START_TRANSFER_TRANS,)\
    ACT(START_TRANSFER_TRANS,)\
    ACT(SWITCH_LEADER_BEFORE_TRANSFER_DOING_START_TRANS,)\
    ACT(SWITCH_LEADER_AFTER_TRANSFER_DOING_START_TRANS,)\
    ACT(SWITCH_LEADER_BETWEEN_FINISH_TRANSFER_IN_AND_OUT,)\
    ACT(TRANSFER_BACKFILL_TX_BEFORE,)\
    ACT(TRANSFER_REPLACE_TABLE_BEFORE,)\
    ACT(BEFORE_MIG_DDL_TABLE_MERGE_TASK,)\
    ACT(BEFORE_COPY_DDL_SSTABLE,)\
    ACT(BEFORE_DDL_WRITE_PREPARE_LOG,)\
    ACT(AFTER_BACKUP_FETCH_MACRO_BLOCK_FAILED,)\
    ACT(BEFORE_TABLE_REDEFINITION_TASK_EFFECT,)\
    ACT(ALTER_LS_CHOOSE_SRC,)\
    ACT(BEFORE_LOCK_SERVICE_UNLOCK,)\
    ACT(DDL_CHECK_TABLET_MERGE_STATUS,)\
    ACT(BEFORE_LOCK_LS_WHEN_CREATE_TABLE,)\
    ACT(AFTER_BLOCK_TABLET_IN_WHEN_LS_MERGE,)\
    ACT(TABLE_LOCK_AFTER_LOCK_TABLE_BEFORE_LOCK_TABLET,)\
    ACT(BEFORE_TENANT_BALANCE_SERVICE,)\
    ACT(BEFORE_TENANT_BALANCE_SERVICE_EXECUTE,)\
    ACT(BEFORE_PROCESS_BALANCE_TASK_INIT,)\
    ACT(BEFORE_PROCESS_BALANCE_TASK_CREATE_LS,)\
    ACT(BEFORE_PROCESS_BALANCE_TASK_TRANSFER,)\
    ACT(BEFORE_PROCESS_BALANCE_TASK_ALTER_LS,)\
    ACT(BEFORE_PROCESS_BALANCE_TASK_SET_MERGE,)\
    ACT(BEFORE_PROCESS_BALANCE_TASK_DROP_LS,)\
    ACT(BEFORE_PROCESS_BALANCE_TASK_TRANSFER_END,)\
    ACT(MODIFY_HIDDEN_TABLE_NOT_NULL_COLUMN_STATE_BEFORE_PUBLISH_SCHEMA,)\
    ACT(AFTER_MIGRATION_FETCH_TABLET_INFO,)\
    ACT(AFTER_LOCK_LS_AND_BEFORE_CHANGE_LS_FLAG,)\
    ACT(AFTER_LOCK_ALL_BALANCE_JOB,)\
    ACT(BEFORE_TRANSFER_START_LOCK_MEMBER_LIST,)\
    ACT(AFTER_TRANSFER_START_LOCK_MEMBER_LIST,)\
    ACT(BEFORE_ON_REDO_START_TRANSFER_OUT,)\
    ACT(AFTER_ON_REDO_START_TRANSFER_OUT,)\
    ACT(AFTER_PART_ON_REDO_FINISH_TRANSFER_OUT,)\
    ACT(AFTER_ON_REDO_FINISH_TRANSFER_OUT,)\
    ACT(BEFORE_ON_REDO_START_TRANSFER_IN,)\
    ACT(AFTER_ON_REDO_START_TRANSFER_IN,)\
    ACT(AFTER_ON_COMMIT_START_TRANSFER_IN,)\
    ACT(BEFORE_TRANSFER_LOCK_TABLE_AND_PART,)\
    ACT(AFTER_TRANSFER_LOCK_TABLE_FOR_GLOBAL_INDEX,)\
    ACT(AFTER_TRANSFER_LOCK_TABLE_FOR_NORMAL_TABLE,)\
    ACT(AFTER_TRANSFER_PROCESS_INIT_TASK_AND_BEFORE_NOTIFY_STORAGE,)\
    ACT(BEFORE_TRANSFER_ADD_ONLINE_DDL_LOCK,)\
    ACT(BEFORE_TRANSFER_GET_RELATED_TABLE_SCHEMAS,)\
    ACT(BEFORE_DROPPING_LS_IN_BALANCE_MERGE_TASK,)\
    ACT(BEFORE_WAIT_LOG_SYNC,)\
    ACT(BEFORE_WAIT_LOG_REPLAY_SYNC,)\
    ACT(BEFORE_FOREIGN_KEY_CONSTRAINT_CHECK,)\
    ACT(BEFORE_EXECUTE_ARB_REPLICA_TASK,)\
    ACT(ARCHIVE_SENDER_HANDLE_TASK_DONE,)\
    ACT(BEFORE_SET_LS_MEMBER_LIST,)\
    ACT(BEFORE_MIGRATION_FETCH_TABLET_INFO,)\
    ACT(BEFORE_BUILD_TABLET_GROUP_INFO,)\
    ACT(BEFORE_RESTORE_SERVICE_PUSH_FETCH_DATA,)\
    ACT(AFTER_MIGRATION_REPORT_LS_META_TABLE,)\
    ACT(BEFORE_RESTORE_HANDLE_FETCH_LOG_TASK,)\
    ACT(BEFORE_DATA_TABLETS_MIGRATION_TASK,)\
    ACT(AFTER_LS_GC_DELETE_ALL_TABLETS,)\
    ACT(BEFORE_ARCHIVE_ADD_LS_TASK,)\
    ACT(AFTER_UPDATE_INDEX_STATUS,)\
    ACT(BEFORE_COMPLETE_MIGRATION_UPDATE_STATUS,)\
    ACT(AFTER_TRANSFER_DUMP_MDS_TABLE,)\
    ACT(BEFORE_PROCESS_BALANCE_EXECUTE_WORK,)\
    ACT(BEFORE_WAIT_RESTORE_TO_CONSISTENT_SCN,)\
    ACT(AFTER_WAIT_RESTORE_TO_CONSISTENT_SCN,)\
    ACT(BEFORE_BACKUP_1001_META,)\
    ACT(BEFORE_BACKUP_1002_META,)\
    ACT(BEFORE_BACKUP_CONSISTENT_SCN,)\
    ACT(BEFORE_TRANSFER_UPDATE_TABLET_TO_LS,)\
    ACT(AFTER_CHANGE_BACKUP_TURN_ID,)\
    ACT(AFTER_START_TRANSFER_WAIT_REPLAY_TO_START_SCN,)\
    ACT(AFTER_START_TRANSFER_GET_TABLET_META,)\
    ACT(AFTER_START_TRANSFER_OUT,)\
    ACT(AFTER_START_TRANSFER_GET_START_SCN,)\
    ACT(AFTER_START_TRANSFER_IN,)\
    ACT(AFTER_UPDATE_TABLET_TO_LS,)\
    ACT(AFTER_DOING_TRANSFER_LOCK_MEMBER_LIST,)\
    ACT(AFTER_DOING_TRANSFER_WAIT_REPLAY_SCN,)\
    ACT(AFTER_FINISH_TRANSFER_OUT,)\
    ACT(BEFORE_DOING_TRANSFER_COMMIT,)\
    ACT(BEFORE_BACKUP_MAJOR,)\
    ACT(BEFORE_TABLET_MDS_FLUSH,)\
    ACT(BEFORE_CHECKPOINT_TASK,)\
    ACT(AFTER_EMPTY_SHELL_TABLET_CREATE,)\
    ACT(AFTER_MACRO_BLOCK_WRITER_DDL_CALLBACK_WAIT,)\
    ACT(AFTER_RESTORE_TABLET_TASK,)\
    ACT(BEFORE_TABLET_GC,)\
    ACT(AFTER_TRANSFER_BLOCK_AND_KILL_TX,)\
    ACT(AFTER_TRANSFER_UNBLOCK_TX,)\
    ACT(AFTER_CHECKPOINT_GET_CURSOR,)\
    ACT(BEFORE_EXECUTE_BALANCE_TASK,)\
    ACT(BEFORE_CHANGE_BACKUP_TURN,)\
    ACT(BEFORE_PROCESS_AFTER_HAS_MEMBER_LIST,)\
    ACT(END_DELETE_SERVER_BEFORE_CHECK_META_TABLE,)\
    ACT(BEFORE_MIGRATION_DISABLE_VOTE,)\
    ACT(MEMBERLIST_CHANGE_MEMBER,)\
    ACT(BEFORE_CHECK_CLEAN_DRTASK,)\
    ACT(BEFORE_UNIQ_TASK_RUN,)\
    ACT(BEFORE_PARELLEL_TRUNCATE,)\
    ACT(END_DDL_IN_PX_SUBCOORD,)\
    ACT(BEFORE_SEND_ADD_REPLICA_DRTASK,)\
    ACT(BETWEEN_INSERT_LOCK_INFO_AND_TRY_LOCK_CONFIG_CHANGE,)\
    ACT(BEFORE_CHECK_SHRINK_RESOURCE_POOL,)\
    ACT(STOP_RECOVERY_LS_THREAD0,)\
    ACT(STOP_RECOVERY_LS_THREAD1,)\
    ACT(STOP_TRANSFER_LS_LOGICAL_TABLE_REPLACED,)\
    ACT(BEFORE_TRANSFER_DOING,)\
    ACT(AFTER_PARALLEL_DDL_LOCK_OBJ_BY_NAME,)\
    ACT(BEFORE_BUILD_LS_MIGRATION_DAG_NET,)\
    ACT(AFTER_JOIN_LEARNER_LIST,)\
    ACT(BEFORE_TRANSFER_START_COMMIT,)\
    ACT(STOP_PRIMARY_LS_THREAD,)\
    ACT(TRANSFER_GET_BACKFILL_TABLETS_BEFORE,)\
    ACT(AFTER_CHANGE_ACCESS_MODE,)\
    ACT(STOP_LS_RECOVERY_THREAD,)\
    ACT(BEFORE_FINISH_PRIMARY_ZONE,)\
    ACT(BEFORE_FINISH_UNIT_NUM,)\
    ACT(BEFORE_CHECK_PRIMARY_ZONE,)\
    ACT(BEFORE_RELOAD_UNIT,)\
    ACT(BEFORE_PROCESS_EVENT_TASK,)\
    ACT(BEFORE_CHECK_LS_TRANSFER_SCN_FOR_STANDBY,)\
    ACT(LS_GC_BEFORE_SUBMIT_OFFLINE_LOG,)\
    ACT(BEFORE_GET_CONFIG_VERSION_AND_TRANSFER_SCN,)\
    ACT(LS_GC_BEFORE_OFFLINE,)\
    ACT(AFTER_LOCK_SNAPSHOT_MUTEX,)\
    ACT(BEFORE_FETCH_SIMPLE_TABLES,)\
    ACT(BEFORE_SEND_PARALLEL_CREATE_TABLE,)\
    ACT(BEFORE_DROP_TENANT,)\
    ACT(BEFORE_WAIT_SYS_LS_END_SCN,)\
    ACT(BEFORE_CREATE_CLONE_TENANT_END,)\
    ACT(BEFORE_CALC_CONSISTENT_SCN,)\
    ACT(RENAME_TABLE_BEFORE_PUBLISH_SCHEMA,)\
    ACT(BEFORE_SET_NEW_SCHEMA_VERSION,)\
    ACT(REPLAY_SWITCH_TO_FOLLOWER_BEFORE_PUSH_SUBMIT_TASK,)\
    ACT(BEFORE_CHECK_TTL_TASK_FINISH,)\
    ACT(BEFORE_TTL_SCHEDULER_RUN,)\
    ACT(BEFORE_MERGE_BACKUP_META_INFO,)\
    ACT(HANG_IN_CLONE_SYS_LOCK,)\
    ACT(HANG_IN_CLONE_SYS_CREATE_INNER_RESOURCE_POOL,)\
    ACT(HANG_IN_CLONE_SYS_CREATE_SNAPSHOT,)\
    ACT(HANG_IN_CLONE_SYS_WAIT_CREATE_SNAPSHOT,)\
    ACT(HANG_IN_CLONE_SYS_CREATE_TENANT,)\
    ACT(HANG_IN_CLONE_SYS_WAIT_TENANT_RESTORE_FINISH,)\
    ACT(HANG_IN_CLONE_SYS_RELEASE_RESOURCE,)\
    ACT(HANG_IN_CLONE_SYS_SUCCESS,)\
    ACT(HANG_IN_CLONE_SYS_FAILED_STATUS,)\
    ACT(BEFORE_BACKUP_PREFETCH_TASK,)\
    ACT(BEFORE_BACKUP_DATA_TASK,)\
    ACT(BEFORE_CHECK_NEED_TABLET_GC,)\
    ACT(BEFORE_TABLET_FULL_DIRECT_LOAD_MGR_CLOSE,)\
    ACT(BEFORE_WAIT_TRANSFER_OUT_TABLET_READY,)\
    ACT(BEFORE_CHECK_TABLET_READY,)\
    ACT(BEFORE_CHECK_TABLET_TRANSFER_TABLE_READY,)\
    ACT(BEFORE_LOG_REPLAY_TO_MAX_MINOR_END_SCN,)\
    ACT(HOLD_DDL_COMPLEMENT_DAG_WHEN_APPEND_ROW,)\
    ACT(HOLD_DDL_COMPLEMENT_DAG_BEFORE_REPORT_FINISH,)\
    ACT(HOLD_DDL_COMPLEMENT_DAG_AFTER_REPORT_FINISH,)\
    ACT(BEFORE_ALTER_TABLE_EXCHANGE_PARTITION,)\
    ACT(AFTER_REPORT_BACKUP_COMPL_LOG,)\
    ACT(BEFORE_WRITE_TABLE_LIST_META_INFO,)\
    ACT(BEFORE_START_TRANSFER_GET_TABLET_META,)\
    ACT(BEFORE_ADD_REFRESH_SCHEMA_TASK,)\
    ACT(BEFORE_ADD_ASYNC_REFRESH_SCHEMA_TASK,)\
    ACT(BEFORE_MIGRATE_WARMUP_RETRY_FETCH_CACHE,)\
    ACT(BEFORE_MIGRATE_WARMUP_RETRY_FETCH_DAG,)\
    ACT(BEFORE_ADD_SERVER_TRANS,)\
    ACT(AFTER_MEMBERLIST_CHANGED,)\
    ACT(AFTER_FIRST_CLONE_CHECK_FOR_STANDBY,)\
    ACT(BEFORE_REMOVE_BALANCE_TASK_HELPER,)\
    ACT(BEFORE_CHOOSE_SOURCE,)\
    ACT(AFTER_CHECK_LOG_NEED_REBUILD,)\
    ACT(REBUILD_VEC_INDEX_PREPARE,)\
    ACT(REBUILD_VEC_INDEX_WAIT_CREATE_NEW_INDEX,)\
    ACT(REBUILD_VEC_INDEX_WAIT_DROP_OLD_INDEX,)\
    ACT(REBUILD_VEC_INDEX_SWITCH_INDEX_NAME,)\
    ACT(BUILD_VECTOR_INDEX_PREPARE_STATUS,)\
    ACT(BUILD_VECTOR_INDEX_PREPARE_ROWKEY_VID,)\
    ACT(BUILD_VECTOR_INDEX_PREPARE_AUX_INDEX,)\
    ACT(BUILD_VECTOR_INDEX_PREPARE_VID_ROWKEY,)\
    ACT(DROP_VECTOR_INDEX_PREPARE_STATUS,)\
    ACT(HANDLE_VECTOR_INDEX_ASYNC_TASK,)\
    ACT(CREATE_AUX_INDEX_TABLE,)\
    ACT(BEFORE_SEND_ALTER_TABLE,)\
    ACT(BEFOR_EXEC_REBUILD_TASK,)\
    ACT(BEFORE_CREATE_HIDDEN_TABLE_IN_LOAD,)\
    ACT(BEFORE_PARALLEL_DDL_LOCK,)\
    ACT(AFTER_PARALLEL_DDL_LOCK,)\
    ACT(BEFORE_START_TRANSFER_IN_ON_PREPARE,)\
    ACT(BEFORE_TRANSFER_SERVICE_RUNNING,)\
    ACT(BEFORE_RESTORE_CREATE_TABLETS_SSTABLE,)\
    ACT(BEFORE_CLOSE_BACKUP_INDEX_BUILDER,)\
    ACT(BEFROE_UPDATE_DATA_VERSION,)\
    ACT(BEFORE_TABLET_MIGRATION_DAG_INNER_RETRY,)\
    ACT(BEFORE_DBMS_VECTOR_REFRESH,)\
    ACT(BEFORE_DBMS_VECTOR_REBUILD,)\
    ACT(BEFORE_DATA_DICT_DUMP_FINISH,)\
    ACT(AFTER_PHYSICAL_RESTORE_CREATE_TENANT,)\
    ACT(BEFROE_UPDATE_MIG_TABLET_CONVERT_CO_PROGRESSING,)\
    ACT(AFTER_SET_CO_CONVERT_RETRY_EXHUASTED,)\
    ACT(BEFORE_FOLLOWER_REPLACE_REMOTE_SSTABLE,)\
    ACT(BEFORE_MIGRATION_LS_OFFLINE,)\
    ACT(AFTER_CREATE_SPLIT_TASK,)\
    ACT(BEFORE_PARTITION_SPLIT_TASK_CLEANUP,)\
    ACT(BEFORE_TABLET_SPLIT_PREPARE_TASK,)\
    ACT(BEFORE_TABLET_SPLIT_WRITE_TASK,)\
    ACT(BEFORE_TABLET_SPLIT_MERGE_TASK,)\
    ACT(AFTER_TABLET_SPLIT_MERGE_TASK,)\
    ACT(START_DDL_PRE_SPLIT_PARTITION,)\
    ACT(BEFORE_PREFETCH_BACKUP_INFO_TASK,)\
    ACT(RS_CHANGE_TURN_DEBUG_SYNC,)\
    ACT(AFTER_MIGRATION_CREATE_ALL_TABLET,)\
    ACT(BEFORE_PARALLEL_BUILD_TABLET_INFO_TABLET,)\
    ACT(BUILD_FTS_INDEX_PREPARE_STATUS,)\
    ACT(DROP_FTS_INDEX_PREPARE_STATUS,)\
    ACT(CREATE_TFS_INDEX_ROWKEY_DOC_STATUS,)\
    ACT(BEFOR_PREPARE_CREATE_TFS_INDEX_DOC_ROWKEY,)\
    ACT(BEFOR_PREPARE_CREATE_TFS_INDEX_WORD_DOC,)\
    ACT(BEFOR_PREPARE_CREATE_TFS_INDEX_DOC_WORD,)\
    ACT(BEFOR_EXECUTE_CREATE_TABLE_WITH_FTS_INDEX,)\
    ACT(AFTER_JOIN_LEARNER_LIST_FOR_SPECIFIED_SERVER,)\
    ACT(BEFORE_MV_FINISH_COMPLETE_REFRESH,)\
    ACT(BEFORE_MIGRATION_CREATE_TABLE_STORE,)\
    ACT(BEFORE_FILL_AUTO_SPLIT_PARAMS,)\
    ACT(BEFORE_UPDATE_TABLET_HA_STATUS,)\
    ACT(BEFORE_LOAD_DICTIONARY_IN_BACKGROUND,)\
    ACT(BEFORE_MIGRATION_DO_INIT_STATUS,)\
    ACT(BEFORE_MIGRATION_DO_PREPARE_LS_STATUS,)\
    ACT(BEFORE_MIGRATION_DO_BUILD_LS_STATUS,)\
    ACT(BEFORE_ADD_PREPARE_LS_MIGRATION_DAG_NET,)\
    ACT(BEFORE_ADD_BUILD_LS_MIGRATION_DAG_NET,)\
    ACT(BEFORE_ADD_COMPLETE_LS_MIGRATION_DAG_NET,)\
    ACT(BEFORE_REBUILD_TABLET_INITAL_TASK,)\
    ACT(BEFORE_REBUILD_TABLET_COPY_SSTABLE_TASK,)\
    ACT(BEFORE_REBUILD_TABLET_INIT_DAG,)\
    ACT(BEFORE_EXECUTE_ADMIN_SET_CONFIG,)\
    ACT(BEFORE_UPGRADE_FINISH_STAGE,)\
    ACT(AFTER_CREATE_META_TENANT_TABLETS,)\
    ACT(AFTER_CREATE_USER_TENANT_TABLETS,)\
    ACT(BEFORE_CREATE_USER_NORMAL_TENANT,)\
    ACT(BEFORE_CREATE_META_NORMAL_TENANT,)\
    ACT(AFTER_CREATE_USER_NORMAL_TENANT,)\
    ACT(AFTER_CREATE_META_NORMAL_TENANT,)\
    ACT(BEFORE_CHECK_CREATE_NORMAL_TENANT_RESULT,)\
    ACT(AFTER_CREATE_TENANT_SCHEMA,)\
    ACT(BEFORE_CREATE_TENANT_FILL_USER_LS_INFO,)\
    ACT(AFTER_TRANSFER_ABORT_UPDATE_STATUS,)\
    ACT(BEFORE_CREATE_TENANT_TABLE_SCHEMA,)\
    ACT(BEFORE_REFRESH_SCHEMA,)\
    ACT(BEFORE_FINISH_BROADCAST_SCHEMA,)\
    ACT(BEFORE_DROP_INDEX,)\
    ACT(AFTER_IVF_CENTROID_TABLE,)\
    ACT(AFTER_PQ_CENTROID_TABLE,)\
    ACT(AFTER_SQ_META_TABLE,)\
    ACT(BEFORE_RENAME_INDEX,)\
    ACT(AFTER_DROP_FTS_SUBMIT_SUBTASK,)\
    ACT(AFTER_DIRECT_LOAD_FIRST_CHECK_IS_SUPPORT,)\
    ACT(AFTER_START_MIGRATION_TASK,)\
    ACT(BEFORE_GET_BACKUP_END_SCN,)\
    ACT(BEFORE_MANAGE_DYNAMIC_PARTITION,)\
    ACT(BEFORE_MANAGE_DYNAMIC_PARTITION_ON_TABLE,)\
    ACT(BEFORE_SWAP_ORIG_AND_HIDDEN_TABLE_STATE,)\
    ACT(BEFORE_MV_FINISH_RUNNING_JOB,)\
    ACT(BEFORE_MV_LOAD_DATA,)\
    ACT(BEFORE_ALTER_TABLE_IN_TRANS,)\
    ACT(BEFORE_RECOVER_TABLE_DDL_TASK_SUCCESS,)\
    ACT(BEFORE_CHECK_TABLET_VALID,)\
    ACT(CANCEL_VEC_TASK_ADD_SNAP_INDEX,)\
    ACT(BEFORE_SPLIT_DOWNLOAD_SSTABLE,)\
    ACT(BEFORE_DEL_LS_AFTER_CREATE_LS_FAILED,)\
    ACT(MAX_DEBUG_SYNC_POINT,)

DECLARE_ENUM(ObDebugSyncPoint, debug_sync_point, OB_DEBUG_SYNC_POINT_DEF);

} // end namespace common
} // end namespace oceanbase

#endif // OCEANBASE_COMMON_OB_DEBUG_SYNC_POINT_H_
