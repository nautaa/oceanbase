/**
 * Copyright (c) 2023 OceanBase
 * OceanBase CE is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#include <sys/socket.h>
typedef struct listenfd_t {
  SOCK_COMMON;
  bool is_pipe;
  eloop_t* ep;
  sf_t* sf;
} listenfd_t;

typedef struct listenfd_dispatch_t {
  int fd;
  int tid;
  void* sock_ptr;
} listenfd_dispatch_t;

extern int listenfd_init(eloop_t* ep, listenfd_t* s, sf_t* sf, int fd);
