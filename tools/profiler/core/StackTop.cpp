/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifdef XP_MACOSX
#include <mach/task.h>
#include <mach/thread_act.h>
#include <pthread.h>
#endif

#include "StackTop.h"

void *GetStackTop(void *guess) {
#if defined(XP_MACOSX)
  pthread_t thread = pthread_self();
  return pthread_get_stackaddr_np(thread);
#else
  return guess;
#endif
}
