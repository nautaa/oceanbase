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

// DO NOT EDIT. This file is automatically generated from ob_errno.def.
// DO NOT EDIT. This file is automatically generated from ob_errno.def.
// DO NOT EDIT. This file is automatically generated from ob_errno.def.
// To add errno in this header file, you should use DEFINE_***_DEP to define errno in ob_errno.def
// For any question, call fyy280124
#ifndef OB_ERRNO_H
#define OB_ERRNO_H

namespace oceanbase {
namespace common {

constexpr int OB_MAX_ERROR_CODE                      = 65535;

constexpr int OB_SUCCESS = 0;
constexpr int OB_ERROR = -4000;
constexpr int OB_OBJ_TYPE_ERROR = -4001;
constexpr int OB_INVALID_ARGUMENT = -4002;
constexpr int OB_ARRAY_OUT_OF_RANGE = -4003;
constexpr int OB_SERVER_LISTEN_ERROR = -4004;
constexpr int OB_INIT_TWICE = -4005;
constexpr int OB_NOT_INIT = -4006;
constexpr int OB_NOT_SUPPORTED = -4007;
constexpr int OB_ITER_END = -4008;
constexpr int OB_IO_ERROR = -4009;
constexpr int OB_ERROR_FUNC_VERSION = -4010;
constexpr int OB_TIMEOUT = -4012;
constexpr int OB_ALLOCATE_MEMORY_FAILED = -4013;
constexpr int OB_INNER_STAT_ERROR = -4014;
constexpr int OB_ERR_SYS = -4015;
constexpr int OB_ERR_UNEXPECTED = -4016;
constexpr int OB_ENTRY_EXIST = -4017;
constexpr int OB_ENTRY_NOT_EXIST = -4018;
constexpr int OB_SIZE_OVERFLOW = -4019;
constexpr int OB_REF_NUM_NOT_ZERO = -4020;
constexpr int OB_CONFLICT_VALUE = -4021;
constexpr int OB_ITEM_NOT_SETTED = -4022;
constexpr int OB_EAGAIN = -4023;
constexpr int OB_BUF_NOT_ENOUGH = -4024;
constexpr int OB_PARTIAL_FAILED = -4025;
constexpr int OB_READ_NOTHING = -4026;
constexpr int OB_FILE_NOT_EXIST = -4027;
constexpr int OB_DISCONTINUOUS_LOG = -4028;
constexpr int OB_SERIALIZE_ERROR = -4033;
constexpr int OB_DESERIALIZE_ERROR = -4034;
constexpr int OB_AIO_TIMEOUT = -4035;
constexpr int OB_NEED_RETRY = -4036;
constexpr int OB_NOT_MASTER = -4038;
constexpr int OB_DECRYPT_FAILED = -4041;
constexpr int OB_NOT_THE_OBJECT = -4050;
constexpr int OB_LAST_LOG_RUINNED = -4052;
constexpr int OB_INVALID_ERROR = -4055;
constexpr int OB_DECIMAL_OVERFLOW_WARN = -4057;
constexpr int OB_EMPTY_RANGE = -4063;
constexpr int OB_DIR_NOT_EXIST = -4066;
constexpr int OB_INVALID_DATA = -4070;
constexpr int OB_CANCELED = -4072;
constexpr int OB_LOG_NOT_ALIGN = -4074;
constexpr int OB_NOT_IMPLEMENT = -4077;
constexpr int OB_DIVISION_BY_ZERO = -4078;
constexpr int OB_EXCEED_MEM_LIMIT = -4080;
constexpr int OB_QUEUE_OVERFLOW = -4085;
constexpr int OB_START_LOG_CURSOR_INVALID = -4099;
constexpr int OB_LOCK_NOT_MATCH = -4100;
constexpr int OB_DEAD_LOCK = -4101;
constexpr int OB_CHECKSUM_ERROR = -4103;
constexpr int OB_INIT_FAIL = -4104;
constexpr int OB_ROWKEY_ORDER_ERROR = -4105;
constexpr int OB_PHYSIC_CHECKSUM_ERROR = -4108;
constexpr int OB_STATE_NOT_MATCH = -4109;
constexpr int OB_IN_STOP_STATE = -4114;
constexpr int OB_LOG_NOT_CLEAR = -4116;
constexpr int OB_FILE_ALREADY_EXIST = -4117;
constexpr int OB_UNKNOWN_PACKET = -4118;
constexpr int OB_RPC_PACKET_TOO_LONG = -4119;
constexpr int OB_LOG_TOO_LARGE = -4120;
constexpr int OB_RPC_SEND_ERROR = -4121;
constexpr int OB_RPC_POST_ERROR = -4122;
constexpr int OB_LIBEASY_ERROR = -4123;
constexpr int OB_CONNECT_ERROR = -4124;
constexpr int OB_RPC_PACKET_INVALID = -4128;
constexpr int OB_BAD_ADDRESS = -4144;
constexpr int OB_ERR_MIN_VALUE = -4150;
constexpr int OB_ERR_MAX_VALUE = -4151;
constexpr int OB_ERR_NULL_VALUE = -4152;
constexpr int OB_RESOURCE_OUT = -4153;
constexpr int OB_ERR_SQL_CLIENT = -4154;
constexpr int OB_OPERATE_OVERFLOW = -4157;
constexpr int OB_INVALID_DATE_FORMAT = -4158;
constexpr int OB_INVALID_ARGUMENT_NUM = -4161;
constexpr int OB_EMPTY_RESULT = -4165;
constexpr int OB_LOG_INVALID_MOD_ID = -4168;
constexpr int OB_LOG_MODULE_UNKNOWN = -4169;
constexpr int OB_LOG_LEVEL_INVALID = -4170;
constexpr int OB_LOG_PARSER_SYNTAX_ERR = -4171;
constexpr int OB_UNKNOWN_CONNECTION = -4174;
constexpr int OB_ERROR_OUT_OF_RANGE = -4175;
constexpr int OB_OP_NOT_ALLOW = -4179;
constexpr int OB_ERR_ALREADY_EXISTS = -4181;
constexpr int OB_SEARCH_NOT_FOUND = -4182;
constexpr int OB_ITEM_NOT_MATCH = -4187;
constexpr int OB_INVALID_DATE_FORMAT_END = -4190;
constexpr int OB_DDL_TASK_EXECUTE_TOO_MUCH_TIME = -4192;
constexpr int OB_HASH_EXIST = -4200;
constexpr int OB_HASH_NOT_EXIST = -4201;
constexpr int OB_HASH_GET_TIMEOUT = -4204;
constexpr int OB_HASH_PLACEMENT_RETRY = -4205;
constexpr int OB_HASH_FULL = -4206;
constexpr int OB_WAIT_NEXT_TIMEOUT = -4208;
constexpr int OB_MAJOR_FREEZE_NOT_FINISHED = -4213;
constexpr int OB_CURL_ERROR = -4216;
constexpr int OB_INVALID_DATE_VALUE = -4219;
constexpr int OB_INACTIVE_SQL_CLIENT = -4220;
constexpr int OB_INACTIVE_RPC_PROXY = -4221;
constexpr int OB_INTERVAL_WITH_MONTH = -4222;
constexpr int OB_TOO_MANY_DATETIME_PARTS = -4223;
constexpr int OB_DATA_OUT_OF_RANGE = -4224;
constexpr int OB_ERR_TRUNCATED_WRONG_VALUE_FOR_FIELD = -4226;
constexpr int OB_ERR_OUT_OF_LOWER_BOUND = -4233;
constexpr int OB_ERR_OUT_OF_UPPER_BOUND = -4234;
constexpr int OB_BAD_NULL_ERROR = -4235;
constexpr int OB_FILE_NOT_OPENED = -4243;
constexpr int OB_ERR_DATA_TRUNCATED = -4249;
constexpr int OB_NOT_RUNNING = -4250;
constexpr int OB_ERR_COMPRESS_DECOMPRESS_DATA = -4257;
constexpr int OB_ERR_INCORRECT_STRING_VALUE = -4258;
constexpr int OB_DATETIME_FUNCTION_OVERFLOW = -4261;
constexpr int OB_ERR_DOUBLE_TRUNCATED = -4262;
constexpr int OB_CACHE_FREE_BLOCK_NOT_ENOUGH = -4273;
constexpr int OB_LAST_LOG_NOT_COMPLETE = -4278;
constexpr int OB_HEAP_TABLE_EXAUSTED = -4379;
constexpr int OB_UNEXPECT_INTERNAL_ERROR = -4388;
constexpr int OB_ERR_TOO_MUCH_TIME = -4389;
constexpr int OB_ERR_THREAD_PANIC = -4396;
constexpr int OB_INVALID_LICENSE = -4405;
constexpr int OB_LICENSE_SCOPE_EXCEEDED = -4407;
constexpr int OB_ERR_INTERVAL_PARTITION_EXIST = -4728;
constexpr int OB_ERR_INTERVAL_PARTITION_ERROR = -4729;
constexpr int OB_FROZEN_INFO_ALREADY_EXIST = -4744;
constexpr int OB_CREATE_STANDBY_TENANT_FAILED = -4765;
constexpr int OB_LS_WAITING_SAFE_DESTROY = -4766;
constexpr int OB_LS_LOCK_CONFLICT = -4768;
constexpr int OB_INVALID_ROOT_KEY = -4769;
constexpr int OB_ERR_PARSER_SYNTAX = -5006;
constexpr int OB_ERR_COLUMN_NOT_FOUND = -5031;
constexpr int OB_ERR_SYS_VARIABLE_UNKNOWN = -5044;
constexpr int OB_ERR_READ_ONLY = -5081;
constexpr int OB_INTEGER_PRECISION_OVERFLOW = -5088;
constexpr int OB_DECIMAL_PRECISION_OVERFLOW = -5089;
constexpr int OB_NUMERIC_OVERFLOW = -5093;
constexpr int OB_ERR_SYS_CONFIG_UNKNOWN = -5099;
constexpr int OB_INVALID_ARGUMENT_FOR_EXTRACT = -5106;
constexpr int OB_INVALID_ARGUMENT_FOR_IS = -5107;
constexpr int OB_INVALID_ARGUMENT_FOR_LENGTH = -5108;
constexpr int OB_INVALID_ARGUMENT_FOR_SUBSTR = -5109;
constexpr int OB_INVALID_ARGUMENT_FOR_TIME_TO_USEC = -5110;
constexpr int OB_INVALID_ARGUMENT_FOR_USEC_TO_TIME = -5111;
constexpr int OB_INVALID_NUMERIC = -5114;
constexpr int OB_ERR_REGEXP_ERROR = -5115;
constexpr int OB_ERR_UNKNOWN_CHARSET = -5142;
constexpr int OB_ERR_UNKNOWN_COLLATION = -5143;
constexpr int OB_ERR_COLLATION_MISMATCH = -5144;
constexpr int OB_ERR_WRONG_VALUE_FOR_VAR = -5145;
constexpr int OB_TENANT_NOT_IN_SERVER = -5150;
constexpr int OB_TENANT_NOT_EXIST = -5157;
constexpr int OB_ERR_DATA_TOO_LONG = -5167;
constexpr int OB_ERR_WRONG_VALUE_COUNT_ON_ROW = -5168;
constexpr int OB_CANT_AGGREGATE_2COLLATIONS = -5177;
constexpr int OB_ERR_UNKNOWN_TIME_ZONE = -5192;
constexpr int OB_ERR_TOO_BIG_PRECISION = -5203;
constexpr int OB_ERR_M_BIGGER_THAN_D = -5204;
constexpr int OB_ERR_TRUNCATED_WRONG_VALUE = -5222;
constexpr int OB_ERR_WRONG_VALUE = -5241;
constexpr int OB_ERR_UNEXPECTED_TZ_TRANSITION = -5297;
constexpr int OB_ERR_INVALID_TIMEZONE_REGION_ID = -5341;
constexpr int OB_ERR_INVALID_HEX_NUMBER = -5342;
constexpr int OB_ERR_FIELD_NOT_FOUND_IN_DATETIME_OR_INTERVAL = -5352;
constexpr int OB_CANT_AGGREGATE_3COLLATIONS = -5356;
constexpr int OB_ERR_INVALID_JSON_TEXT = -5411;
constexpr int OB_ERR_INVALID_JSON_TEXT_IN_PARAM = -5412;
constexpr int OB_ERR_INVALID_JSON_BINARY_DATA = -5413;
constexpr int OB_ERR_INVALID_JSON_PATH = -5414;
constexpr int OB_ERR_INVALID_JSON_CHARSET = -5415;
constexpr int OB_ERR_INVALID_JSON_CHARSET_IN_FUNCTION = -5416;
constexpr int OB_ERR_INVALID_TYPE_FOR_JSON = -5417;
constexpr int OB_ERR_INVALID_CAST_TO_JSON = -5418;
constexpr int OB_ERR_INVALID_JSON_PATH_CHARSET = -5419;
constexpr int OB_ERR_INVALID_JSON_PATH_WILDCARD = -5420;
constexpr int OB_ERR_JSON_VALUE_TOO_BIG = -5421;
constexpr int OB_ERR_JSON_KEY_TOO_BIG = -5422;
constexpr int OB_ERR_JSON_USED_AS_KEY = -5423;
constexpr int OB_ERR_JSON_VACUOUS_PATH = -5424;
constexpr int OB_ERR_JSON_BAD_ONE_OR_ALL_ARG = -5425;
constexpr int OB_ERR_NUMERIC_JSON_VALUE_OUT_OF_RANGE = -5426;
constexpr int OB_ERR_INVALID_JSON_VALUE_FOR_CAST = -5427;
constexpr int OB_ERR_JSON_OUT_OF_DEPTH = -5428;
constexpr int OB_ERR_JSON_DOCUMENT_NULL_KEY = -5429;
constexpr int OB_ERR_BLOB_CANT_HAVE_DEFAULT = -5430;
constexpr int OB_ERR_INVALID_JSON_PATH_ARRAY_CELL = -5431;
constexpr int OB_ERR_MISSING_JSON_VALUE = -5432;
constexpr int OB_ERR_MULTIPLE_JSON_VALUES = -5433;
constexpr int OB_INVALID_ARGUMENT_FOR_TIMESTAMP_TO_SCN = -5436;
constexpr int OB_INVALID_ARGUMENT_FOR_SCN_TO_TIMESTAMP = -5437;
constexpr int OB_ERR_INVALID_JSON_TYPE = -5441;
constexpr int OB_ERR_JSON_PATH_SYNTAX_ERROR = -5442;
constexpr int OB_ERR_JSON_VALUE_NO_SCALAR = -5444;
constexpr int OB_ERR_DUPLICATE_KEY = -5453;
constexpr int OB_ERR_JSON_PATH_EXPRESSION_SYNTAX_ERROR = -5454;
constexpr int OB_ERR_NOT_ISO_8601_FORMAT = -5472;
constexpr int OB_ERR_VALUE_EXCEEDED_MAX = -5473;
constexpr int OB_ERR_BOOL_NOT_CONVERT_NUMBER = -5475;
constexpr int OB_ERR_JSON_KEY_NOT_FOUND = -5485;
constexpr int OB_ERR_SESSION_VAR_CHANGED = -5540;
constexpr int OB_ERR_YEAR_CONFLICTS_WITH_JULIAN_DATE = -5629;
constexpr int OB_ERR_DAY_OF_YEAR_CONFLICTS_WITH_JULIAN_DATE = -5630;
constexpr int OB_ERR_MONTH_CONFLICTS_WITH_JULIAN_DATE = -5631;
constexpr int OB_ERR_DAY_OF_MONTH_CONFLICTS_WITH_JULIAN_DATE = -5632;
constexpr int OB_ERR_DAY_OF_WEEK_CONFLICTS_WITH_JULIAN_DATE = -5633;
constexpr int OB_ERR_HOUR_CONFLICTS_WITH_SECONDS_IN_DAY = -5634;
constexpr int OB_ERR_MINUTES_OF_HOUR_CONFLICTS_WITH_SECONDS_IN_DAY = -5635;
constexpr int OB_ERR_SECONDS_OF_MINUTE_CONFLICTS_WITH_SECONDS_IN_DAY = -5636;
constexpr int OB_ERR_DATE_NOT_VALID_FOR_MONTH_SPECIFIED = -5637;
constexpr int OB_ERR_INPUT_VALUE_NOT_LONG_ENOUGH = -5638;
constexpr int OB_ERR_INVALID_YEAR_VALUE = -5639;
constexpr int OB_ERR_INVALID_QUARTER_VALUE = -5640;
constexpr int OB_ERR_INVALID_MONTH = -5641;
constexpr int OB_ERR_INVALID_DAY_OF_THE_WEEK = -5642;
constexpr int OB_ERR_INVALID_DAY_OF_YEAR_VALUE = -5643;
constexpr int OB_ERR_INVALID_HOUR12_VALUE = -5644;
constexpr int OB_ERR_INVALID_HOUR24_VALUE = -5645;
constexpr int OB_ERR_INVALID_MINUTES_VALUE = -5646;
constexpr int OB_ERR_INVALID_SECONDS_VALUE = -5647;
constexpr int OB_ERR_INVALID_SECONDS_IN_DAY_VALUE = -5648;
constexpr int OB_ERR_INVALID_JULIAN_DATE_VALUE = -5649;
constexpr int OB_ERR_AM_OR_PM_REQUIRED = -5650;
constexpr int OB_ERR_BC_OR_AD_REQUIRED = -5651;
constexpr int OB_ERR_FORMAT_CODE_APPEARS_TWICE = -5652;
constexpr int OB_ERR_DAY_OF_WEEK_SPECIFIED_MORE_THAN_ONCE = -5653;
constexpr int OB_ERR_SIGNED_YEAR_PRECLUDES_USE_OF_BC_AD = -5654;
constexpr int OB_ERR_JULIAN_DATE_PRECLUDES_USE_OF_DAY_OF_YEAR = -5655;
constexpr int OB_ERR_YEAR_MAY_ONLY_BE_SPECIFIED_ONCE = -5656;
constexpr int OB_ERR_HOUR_MAY_ONLY_BE_SPECIFIED_ONCE = -5657;
constexpr int OB_ERR_AM_PM_CONFLICTS_WITH_USE_OF_AM_DOT_PM_DOT = -5658;
constexpr int OB_ERR_BC_AD_CONFLICT_WITH_USE_OF_BC_DOT_AD_DOT = -5659;
constexpr int OB_ERR_MONTH_MAY_ONLY_BE_SPECIFIED_ONCE = -5660;
constexpr int OB_ERR_DAY_OF_WEEK_MAY_ONLY_BE_SPECIFIED_ONCE = -5661;
constexpr int OB_ERR_FORMAT_CODE_CANNOT_APPEAR = -5662;
constexpr int OB_ERR_NON_NUMERIC_CHARACTER_VALUE = -5663;
constexpr int OB_INVALID_MERIDIAN_INDICATOR_USE = -5664;
constexpr int OB_ERR_DAY_OF_MONTH_RANGE = -5667;
constexpr int OB_ERR_ARGUMENT_OUT_OF_RANGE = -5674;
constexpr int OB_ERR_INTERVAL_INVALID = -5676;
constexpr int OB_ERR_THE_LEADING_PRECISION_OF_THE_INTERVAL_IS_TOO_SMALL = -5708;
constexpr int OB_ERR_INVALID_TIME_ZONE_HOUR = -5709;
constexpr int OB_ERR_INVALID_TIME_ZONE_MINUTE = -5710;
constexpr int OB_ERR_NOT_A_VALID_TIME_ZONE = -5711;
constexpr int OB_ERR_DATE_FORMAT_IS_TOO_LONG_FOR_INTERNAL_BUFFER = -5712;
constexpr int OB_ERR_OPERATOR_CANNOT_BE_USED_WITH_LIST = -5729;
constexpr int OB_INVALID_ROWID = -5802;
constexpr int OB_ERR_NUMERIC_NOT_MATCH_FORMAT_LENGTH = -5873;
constexpr int OB_ERR_DATETIME_INTERVAL_INTERNAL_ERROR = -5898;
constexpr int OB_ERR_DBLINK_REMOTE_ECODE = -5975;
constexpr int OB_ERR_DBLINK_NO_LIB = -5976;
constexpr int OB_SWITCHING_TO_FOLLOWER_GRACEFULLY = -6202;
constexpr int OB_MASK_SET_NO_NODE = -6203;
constexpr int OB_TRANS_TIMEOUT = -6210;
constexpr int OB_TRANS_KILLED = -6211;
constexpr int OB_TRANS_STMT_TIMEOUT = -6212;
constexpr int OB_TRANS_CTX_NOT_EXIST = -6213;
constexpr int OB_TRANS_UNKNOWN = -6225;
constexpr int OB_ERR_READ_ONLY_TRANSACTION = -6226;
constexpr int OB_ERR_UNSUPPROTED_REF_IN_JSON_SCHEMA = -6284;
constexpr int OB_ERR_TYPE_OF_JSON_SCHEMA = -6285;
constexpr int OB_PARTITION_ALREADY_BALANCED = -7124;
constexpr int OB_ERR_GIS_DIFFERENT_SRIDS = -7201;
constexpr int OB_ERR_GIS_UNSUPPORTED_ARGUMENT = -7202;
constexpr int OB_ERR_GIS_UNKNOWN_ERROR = -7203;
constexpr int OB_ERR_GIS_UNKNOWN_EXCEPTION = -7204;
constexpr int OB_ERR_GIS_INVALID_DATA = -7205;
constexpr int OB_ERR_BOOST_GEOMETRY_EMPTY_INPUT_EXCEPTION = -7206;
constexpr int OB_ERR_BOOST_GEOMETRY_CENTROID_EXCEPTION = -7207;
constexpr int OB_ERR_BOOST_GEOMETRY_OVERLAY_INVALID_INPUT_EXCEPTION = -7208;
constexpr int OB_ERR_BOOST_GEOMETRY_TURN_INFO_EXCEPTION = -7209;
constexpr int OB_ERR_BOOST_GEOMETRY_SELF_INTERSECTION_POINT_EXCEPTION = -7210;
constexpr int OB_ERR_BOOST_GEOMETRY_UNKNOWN_EXCEPTION = -7211;
constexpr int OB_ERR_GIS_DATA_WRONG_ENDIANESS = -7212;
constexpr int OB_ERR_ALTER_OPERATION_NOT_SUPPORTED_REASON_GIS = -7213;
constexpr int OB_ERR_BOOST_GEOMETRY_INCONSISTENT_TURNS_EXCEPTION = -7214;
constexpr int OB_ERR_GIS_MAX_POINTS_IN_GEOMETRY_OVERFLOWED = -7215;
constexpr int OB_ERR_UNEXPECTED_GEOMETRY_TYPE = -7216;
constexpr int OB_ERR_SRS_PARSE_ERROR = -7217;
constexpr int OB_ERR_SRS_PROJ_PARAMETER_MISSING = -7218;
constexpr int OB_ERR_WARN_SRS_NOT_FOUND = -7219;
constexpr int OB_ERR_SRS_NOT_CARTESIAN = -7220;
constexpr int OB_ERR_SRS_NOT_CARTESIAN_UNDEFINED = -7221;
constexpr int OB_ERR_SRS_NOT_FOUND = -7222;
constexpr int OB_ERR_WARN_SRS_NOT_FOUND_AXIS_ORDER = -7223;
constexpr int OB_ERR_NOT_IMPLEMENTED_FOR_GEOGRAPHIC_SRS = -7224;
constexpr int OB_ERR_WRONG_SRID_FOR_COLUMN = -7225;
constexpr int OB_ERR_CANNOT_ALTER_SRID_DUE_TO_INDEX = -7226;
constexpr int OB_ERR_WARN_USELESS_SPATIAL_INDEX = -7227;
constexpr int OB_ERR_ONLY_IMPLEMENTED_FOR_SRID_0_AND_4326 = -7228;
constexpr int OB_ERR_NOT_IMPLEMENTED_FOR_CARTESIAN_SRS = -7229;
constexpr int OB_ERR_NOT_IMPLEMENTED_FOR_PROJECTED_SRS = -7230;
constexpr int OB_ERR_SRS_MISSING_MANDATORY_ATTRIBUTE = -7231;
constexpr int OB_ERR_SRS_MULTIPLE_ATTRIBUTE_DEFINITIONS = -7232;
constexpr int OB_ERR_SRS_NAME_CANT_BE_EMPTY_OR_WHITESPACE = -7233;
constexpr int OB_ERR_SRS_ORGANIZATION_CANT_BE_EMPTY_OR_WHITESPACE = -7234;
constexpr int OB_ERR_SRS_ID_ALREADY_EXISTS = -7235;
constexpr int OB_ERR_WARN_SRS_ID_ALREADY_EXISTS = -7236;
constexpr int OB_ERR_CANT_MODIFY_SRID_0 = -7237;
constexpr int OB_ERR_WARN_RESERVED_SRID_RANGE = -7238;
constexpr int OB_ERR_CANT_MODIFY_SRS_USED_BY_COLUMN = -7239;
constexpr int OB_ERR_SRS_INVALID_CHARACTER_IN_ATTRIBUTE = -7240;
constexpr int OB_ERR_SRS_ATTRIBUTE_STRING_TOO_LONG = -7241;
constexpr int OB_ERR_SRS_NOT_GEOGRAPHIC = -7242;
constexpr int OB_ERR_POLYGON_TOO_LARGE = -7243;
constexpr int OB_ERR_SPATIAL_UNIQUE_INDEX = -7244;
constexpr int OB_ERR_GEOMETRY_PARAM_LONGITUDE_OUT_OF_RANGE = -7246;
constexpr int OB_ERR_GEOMETRY_PARAM_LATITUDE_OUT_OF_RANGE = -7247;
constexpr int OB_ERR_SRS_GEOGCS_INVALID_AXES = -7248;
constexpr int OB_ERR_SRS_INVALID_SEMI_MAJOR_AXIS = -7249;
constexpr int OB_ERR_SRS_INVALID_INVERSE_FLATTENING = -7250;
constexpr int OB_ERR_SRS_INVALID_ANGULAR_UNIT = -7251;
constexpr int OB_ERR_SRS_INVALID_PRIME_MERIDIAN = -7252;
constexpr int OB_ERR_TRANSFORM_SOURCE_SRS_NOT_SUPPORTED = -7253;
constexpr int OB_ERR_TRANSFORM_TARGET_SRS_NOT_SUPPORTED = -7254;
constexpr int OB_ERR_TRANSFORM_SOURCE_SRS_MISSING_TOWGS84 = -7255;
constexpr int OB_ERR_TRANSFORM_TARGET_SRS_MISSING_TOWGS84 = -7256;
constexpr int OB_ERR_FUNCTIONAL_INDEX_ON_JSON_OR_GEOMETRY_FUNCTION = -7257;
constexpr int OB_ERR_SPATIAL_FUNCTIONAL_INDEX = -7258;
constexpr int OB_ERR_GEOMETRY_IN_UNKNOWN_LENGTH_UNIT = -7259;
constexpr int OB_ERR_INVALID_CAST_TO_GEOMETRY = -7260;
constexpr int OB_ERR_INVALID_CAST_POLYGON_RING_DIRECTION = -7261;
constexpr int OB_ERR_GIS_DIFFERENT_SRIDS_AGGREGATION = -7262;
constexpr int OB_ERR_LONGITUDE_OUT_OF_RANGE = -7263;
constexpr int OB_ERR_LATITUDE_OUT_OF_RANGE = -7264;
constexpr int OB_ERR_STD_BAD_ALLOC_ERROR = -7265;
constexpr int OB_ERR_STD_DOMAIN_ERROR = -7266;
constexpr int OB_ERR_STD_LENGTH_ERROR = -7267;
constexpr int OB_ERR_STD_INVALID_ARGUMENT = -7268;
constexpr int OB_ERR_STD_OUT_OF_RANGE_ERROR = -7269;
constexpr int OB_ERR_STD_OVERFLOW_ERROR = -7270;
constexpr int OB_ERR_STD_RANGE_ERROR = -7271;
constexpr int OB_ERR_STD_UNDERFLOW_ERROR = -7272;
constexpr int OB_ERR_STD_LOGIC_ERROR = -7273;
constexpr int OB_ERR_STD_RUNTIME_ERROR = -7274;
constexpr int OB_ERR_STD_UNKNOWN_EXCEPTION = -7275;
constexpr int OB_ERR_CANT_CREATE_GEOMETRY_OBJECT = -7276;
constexpr int OB_ERR_SRID_WRONG_USAGE = -7277;
constexpr int OB_ERR_INDEX_ORDER_WRONG_USAGE = -7278;
constexpr int OB_ERR_SPATIAL_MUST_HAVE_GEOM_COL = -7279;
constexpr int OB_ERR_SPATIAL_CANT_HAVE_NULL = -7280;
constexpr int OB_ERR_INDEX_TYPE_NOT_SUPPORTED_FOR_SPATIAL_INDEX = -7281;
constexpr int OB_ERR_UNIT_NOT_FOUND = -7282;
constexpr int OB_ERR_INVALID_OPTION_KEY_VALUE_PAIR = -7283;
constexpr int OB_ERR_NONPOSITIVE_RADIUS = -7284;
constexpr int OB_ERR_SRS_EMPTY = -7285;
constexpr int OB_ERR_INVALID_OPTION_KEY = -7286;
constexpr int OB_ERR_INVALID_OPTION_VALUE = -7287;
constexpr int OB_ERR_INVALID_GEOMETRY_TYPE = -7288;
constexpr int OB_ERR_FTS_MUST_HAVE_TEXT_COL = -7289;
constexpr int OB_ERR_INVALID_GTYPE_IN_SDO_GEROMETRY = -7291;
constexpr int OB_ERR_INVALID_NULL_SDO_GEOMETRY = -7294;
constexpr int OB_ERR_INVALID_DATA_IN_SDO_ELEM_INFO_ARRAY = -7295;
constexpr int OB_ERR_INVALID_DATA_IN_SDO_ORDINATE_ARRAY = -7296;
constexpr int OB_ERR_VALUE_NOT_ALLOWED = -7297;
constexpr int OB_ERR_PARSE_XQUERY_EXPR = -7422;
constexpr int OB_ERR_TOO_MANY_PREFIX_DECLARE = -7424;
constexpr int OB_ERR_XPATH_INVALID_NODE = -7426;
constexpr int OB_ERR_XPATH_NO_NODE = -7427;
constexpr int OB_ERR_DUP_DEF_NAMESPACE = -7431;
constexpr int OB_ERR_INVALID_VECTOR_DIM = -7600;
constexpr int OB_ERR_ARRAY_TYPE_MISMATCH = -7602;
constexpr int OB_PACKET_CLUSTER_ID_NOT_MATCH = -8004;
constexpr int OB_TENANT_ID_NOT_MATCH = -8005;
constexpr int OB_URI_ERROR = -9001;
constexpr int OB_FINAL_MD5_ERROR = -9002;
constexpr int OB_OSS_ERROR = -9003;
constexpr int OB_INIT_MD5_ERROR = -9004;
constexpr int OB_OUT_OF_ELEMENT = -9005;
constexpr int OB_UPDATE_MD5_ERROR = -9006;
constexpr int OB_FILE_LENGTH_INVALID = -9007;
constexpr int OB_BACKUP_FILE_NOT_EXIST = -9011;
constexpr int OB_INVALID_BACKUP_DEST = -9026;
constexpr int OB_COS_ERROR = -9060;
constexpr int OB_IO_LIMIT = -9061;
constexpr int OB_BACKUP_BACKUP_REACH_COPY_LIMIT = -9062;
constexpr int OB_BACKUP_IO_PROHIBITED = -9063;
constexpr int OB_BACKUP_PERMISSION_DENIED = -9071;
constexpr int OB_ESI_OBS_ERROR = -9073;
constexpr int OB_BACKUP_META_INDEX_NOT_EXIST = -9076;
constexpr int OB_BACKUP_FORMAT_FILE_NOT_EXIST = -9080;
constexpr int OB_BACKUP_FORMAT_FILE_NOT_MATCH = -9081;
constexpr int OB_BACKUP_DEVICE_OUT_OF_SPACE = -9082;
constexpr int OB_BACKUP_PWRITE_OFFSET_NOT_MATCH = -9083;
constexpr int OB_BACKUP_PWRITE_CONTENT_NOT_MATCH = -9084;
constexpr int OB_CLOUD_OBJECT_NOT_APPENDABLE = -9098;
constexpr int OB_RESTORE_TENANT_FAILED = -9099;
constexpr int OB_S3_ERROR = -9105;
constexpr int OB_TENANT_SNAPSHOT_NOT_EXIST = -9106;
constexpr int OB_TENANT_SNAPSHOT_EXIST = -9107;
constexpr int OB_TENANT_SNAPSHOT_TIMEOUT = -9108;
constexpr int OB_CLONE_TENANT_TIMEOUT = -9109;
constexpr int OB_ERR_CLONE_TENANT = -9110;
constexpr int OB_ERR_TENANT_SNAPSHOT = -9111;
constexpr int OB_TENANT_SNAPSHOT_LOCK_CONFLICT = -9112;
constexpr int OB_CHECKSUM_TYPE_NOT_SUPPORTED = -9113;
constexpr int OB_INVALID_STORAGE_DEST = -9114;
constexpr int OB_OBJECT_STORAGE_PERMISSION_DENIED = -9116;
constexpr int OB_S3_REGION_MISMATCH = -9117;
constexpr int OB_INVALID_OBJECT_STORAGE_ENDPOINT = -9118;
constexpr int OB_RESTORE_SOURCE_NOT_ENOUGH = -9119;
constexpr int OB_OBJECT_NOT_EXIST = -9120;
constexpr int OB_S2_REUSE_VERSION_MISMATCH = -9121;
constexpr int OB_S2_ENTRY_NOT_EXIST = -9122;
constexpr int OB_ALLOCATE_TMP_FILE_PAGE_FAILED = -9124;
constexpr int OB_SS_MICRO_CACHE_DISABLED = -9125;
constexpr int OB_SS_CACHE_REACH_MEM_LIMIT = -9126;
constexpr int OB_OBJECT_STORAGE_IO_ERROR = -9129;
constexpr int OB_OBJECT_STORAGE_PWRITE_OFFSET_NOT_MATCH = -9130;
constexpr int OB_OBJECT_STORAGE_PWRITE_CONTENT_NOT_MATCH = -9131;
constexpr int OB_OBJECT_STORAGE_CHECKSUM_ERROR = -9132;
constexpr int OB_BACKUP_ZONE_IDC_REGION_INVALID = -9133;
constexpr int OB_ERR_TMP_FILE_ALREADY_SEALED = -9134;
constexpr int OB_TMP_FILE_EXCEED_DISK_QUOTA = -9135;
constexpr int OB_ERR_DUPLICATE_INDEX = -9137;
constexpr int OB_INVALID_KMS_DEST = -9139;
constexpr int OB_OBJECT_STORAGE_OBJECT_LOCKED_BY_WORM = -9140;
constexpr int OB_OBJECT_STORAGE_OVERWRITE_CONTENT_MISMATCH = -9142;
constexpr int OB_DAG_TASK_IS_SUSPENDED = -9143;
constexpr int OB_ERR_XML_PARSE = -9549;
constexpr int OB_ERR_XSLT_PARSE = -9574;
constexpr int OB_HDFS_CONNECT_FS_ERROR = -11019;
constexpr int OB_HDFS_OPEN_FILE_ERROR = -11020;
constexpr int OB_HDFS_READ_FILE_ERROR = -11021;
constexpr int OB_HDFS_CREATE_IO_SERVICE_ERROR = -11022;
constexpr int OB_HDFS_CREATE_FILE_SYSTEM_ERROR = -11023;
constexpr int OB_HDFS_CONNECT_DEFAULTFS_ERROR = -11024;
constexpr int OB_HDFS_LOAD_DEFAULT_RESOURCES_ERROR = -11025;
constexpr int OB_HDFS_VALIDATE_RESOURCES_ERROR = -11026;
constexpr int OB_HDFS_LOAD_OBJECT_ERROR = -11027;
constexpr int OB_HDFS_RESOURCE_UNAVAILABLE = -11028;
constexpr int OB_HDFS_FUNC_UNIMPLEMENTED = -11029;
constexpr int OB_HDFS_OPERATION_CANCELED = -11030;
constexpr int OB_HDFS_PERMISSION_DENIED = -11031;
constexpr int OB_HDFS_PATH_NOT_FOUND = -11032;
constexpr int OB_HDFS_FILE_ALREADY_EXISTS = -11033;
constexpr int OB_HDFS_PATH_IS_NOT_EMPTY_DIRECTORY = -11034;
constexpr int OB_HDFS_BUSY = -11035;
constexpr int OB_HDFS_AUTHENTICATION_FAILED = -11036;
constexpr int OB_HDFS_ACCESSCONTROL_ERROR = -11037;
constexpr int OB_HDFS_STANDNDBY_ERROR = -11038;
constexpr int OB_HDFS_SNAPSHOT_PROTOCOL_ERROR = -11039;
constexpr int OB_HDFS_INVALID_OFFSET = -11040;
constexpr int OB_HDFS_NOT_A_DIRECTORY = -11041;
constexpr int OB_HDFS_MALFORMED_URI = -11042;
constexpr int OB_HDFS_INVALID_ARGUMENT = -11043;
constexpr int OB_HDFS_NOT_IMPLEMENT = -11044;
constexpr int OB_HDFS_ERROR = -11045;
constexpr int OB_JNI_ERROR = -11053;
constexpr int OB_JNI_CLASS_NOT_FOUND_ERROR = -11054;
constexpr int OB_JNI_METHOD_NOT_FOUND_ERROR = -11055;
constexpr int OB_JNI_ENV_ERROR = -11056;
constexpr int OB_JNI_DELETE_REF_ERROR = -11057;
constexpr int OB_JNI_JAVA_HOME_NOT_FOUND_ERROR = -11058;
constexpr int OB_JNI_ENSURE_CAPACTIY_ERROR = -11059;
constexpr int OB_JNI_FIELD_NOT_FOUND_ERROR = -11060;
constexpr int OB_JNI_OBJECT_NOT_FOUND_ERROR = -11061;
constexpr int OB_JNI_JAVA_OPTS_NOT_FOUND_ERROR = -11072;
constexpr int OB_JNI_CONNECTOR_PATH_NOT_FOUND_ERROR = -11073;
constexpr int OB_JNI_NOT_ENABLE_JAVA_ENV_ERROR = -11074;
constexpr int OB_LOGSERVICE_RPC_ERROR = -11093;
constexpr int OB_MAX_RAISE_APPLICATION_ERROR         = -20000;
constexpr int OB_MIN_RAISE_APPLICATION_ERROR         = -20999;

} // common
using namespace common; // maybe someone can fix
} // oceanbase

#endif /* OB_ERRNO_H */
