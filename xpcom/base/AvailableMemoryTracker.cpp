/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/AvailableMemoryTracker.h"

#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIRunnable.h"
#include "nsISupports.h"
#include "nsThreadUtils.h"

#include "mozilla/Preferences.h"
#include "mozilla/Services.h"

#if defined(MOZ_MEMORY)
#   include "mozmemory.h"
#endif  // MOZ_MEMORY

using namespace mozilla;

namespace {

/**
 * This runnable is executed in response to a memory-pressure event; we spin
 * the event-loop when receiving the memory-pressure event in the hope that
 * other observers will synchronously free some memory that we'll be able to
 * purge here.
 */
class nsJemallocFreeDirtyPagesRunnable final : public nsIRunnable
{
  ~nsJemallocFreeDirtyPagesRunnable() {}

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE
};

NS_IMPL_ISUPPORTS(nsJemallocFreeDirtyPagesRunnable, nsIRunnable)

NS_IMETHODIMP
nsJemallocFreeDirtyPagesRunnable::Run()
{
  MOZ_ASSERT(NS_IsMainThread());

#if defined(MOZ_MEMORY)
  jemalloc_free_dirty_pages();
#endif

  return NS_OK;
}

/**
 * The memory pressure watcher is used for listening to memory-pressure events
 * and reacting upon them. We use one instance per process currently only for
 * cleaning up dirty unused pages held by jemalloc.
 */
class nsMemoryPressureWatcher final : public nsIObserver
{
  ~nsMemoryPressureWatcher() {}

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  void Init();

private:
  static bool sFreeDirtyPages;
};

NS_IMPL_ISUPPORTS(nsMemoryPressureWatcher, nsIObserver)

bool nsMemoryPressureWatcher::sFreeDirtyPages = false;

/**
 * Initialize and subscribe to the memory-pressure events. We subscribe to the
 * observer service in this method and not in the constructor because we need
 * to hold a strong reference to 'this' before calling the observer service.
 */
void
nsMemoryPressureWatcher::Init()
{
  nsCOMPtr<nsIObserverService> os = services::GetObserverService();

  if (os) {
    os->AddObserver(this, "memory-pressure", /* ownsWeak */ false);
  }

  Preferences::AddBoolVarCache(&sFreeDirtyPages, "memory.free_dirty_pages",
                               false);
}

/**
 * Reacts to all types of memory-pressure events, launches a runnable to
 * free dirty pages held by jemalloc.
 */
NS_IMETHODIMP
nsMemoryPressureWatcher::Observe(nsISupports* aSubject, const char* aTopic,
                                 const char16_t* aData)
{
  MOZ_ASSERT(!strcmp(aTopic, "memory-pressure"), "Unknown topic");

  if (sFreeDirtyPages) {
    nsCOMPtr<nsIRunnable> runnable = new nsJemallocFreeDirtyPagesRunnable();

    NS_DispatchToMainThread(runnable);
  }

  return NS_OK;
}

} // namespace

namespace mozilla {
namespace AvailableMemoryTracker {

void
Activate()
{
  // This object is held alive by the observer service.
  RefPtr<nsMemoryPressureWatcher> watcher = new nsMemoryPressureWatcher();
  watcher->Init();
}

void
Init()
{
  // Do nothing on x86-64, because nsWindowsDllInterceptor is not thread-safe
  // on 64-bit.  (On 32-bit, it's probably thread-safe.)  Even if we run Init()
  // before any other of our threads are running, another process may have
  // started a remote thread which could call VirtualAlloc!
  //
  // Moreover, the benefit of this code is less clear when we're a 64-bit
  // process, because we aren't going to run out of virtual memory, and the
  // system is likely to have a fair bit of physical memory.
}

} // namespace AvailableMemoryTracker
} // namespace mozilla
