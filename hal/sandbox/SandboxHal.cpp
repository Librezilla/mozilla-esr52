/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Hal.h"
#include "HalLog.h"
#include "mozilla/AppProcessChecker.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/hal_sandbox/PHalChild.h"
#include "mozilla/hal_sandbox/PHalParent.h"
#include "mozilla/dom/TabParent.h"
#include "mozilla/dom/TabChild.h"
#include "mozilla/dom/network/Types.h"
#include "mozilla/dom/ScreenOrientation.h"
#include "mozilla/Observer.h"
#include "mozilla/Unused.h"
#include "nsAutoPtr.h"
#include "WindowIdentifier.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::hal;

namespace mozilla {
namespace hal_sandbox {

static bool sHalChildDestroyed = false;

bool
HalChildDestroyed()
{
  return sHalChildDestroyed;
}

static PHalChild* sHal;
static PHalChild*
Hal()
{
  if (!sHal) {
    sHal = ContentChild::GetSingleton()->SendPHalConstructor();
  }
  return sHal;
}

void
EnableNetworkNotifications()
{
  Hal()->SendEnableNetworkNotifications();
}

void
DisableNetworkNotifications()
{
  Hal()->SendDisableNetworkNotifications();
}

void
GetCurrentNetworkInformation(NetworkInformation* aNetworkInfo)
{
  Hal()->SendGetCurrentNetworkInformation(aNetworkInfo);
}

void
EnableScreenConfigurationNotifications()
{
  Hal()->SendEnableScreenConfigurationNotifications();
}

void
DisableScreenConfigurationNotifications()
{
  Hal()->SendDisableScreenConfigurationNotifications();
}

void
GetCurrentScreenConfiguration(ScreenConfiguration* aScreenConfiguration)
{
  Hal()->SendGetCurrentScreenConfiguration(aScreenConfiguration);
}

bool
LockScreenOrientation(const dom::ScreenOrientationInternal& aOrientation)
{
  bool allowed;
  Hal()->SendLockScreenOrientation(aOrientation, &allowed);
  return allowed;
}

void
UnlockScreenOrientation()
{
  Hal()->SendUnlockScreenOrientation();
}

void
AdjustSystemClock(int64_t aDeltaMilliseconds)
{
  Hal()->SendAdjustSystemClock(aDeltaMilliseconds);
}

void
SetTimezone(const nsCString& aTimezoneSpec)
{
  Hal()->SendSetTimezone(nsCString(aTimezoneSpec));
}

nsCString
GetTimezone()
{
  nsCString timezone;
  Hal()->SendGetTimezone(&timezone);
  return timezone;
}

int32_t
GetTimezoneOffset()
{
  int32_t timezoneOffset;
  Hal()->SendGetTimezoneOffset(&timezoneOffset);
  return timezoneOffset;
}

void
EnableSystemClockChangeNotifications()
{
  Hal()->SendEnableSystemClockChangeNotifications();
}

void
DisableSystemClockChangeNotifications()
{
  Hal()->SendDisableSystemClockChangeNotifications();
}

void
EnableSystemTimezoneChangeNotifications()
{
  Hal()->SendEnableSystemTimezoneChangeNotifications();
}

void
DisableSystemTimezoneChangeNotifications()
{
  Hal()->SendDisableSystemTimezoneChangeNotifications();
}

#ifdef MOZ_WAKELOCK
void
EnableWakeLockNotifications()
{
  Hal()->SendEnableWakeLockNotifications();
}

void
DisableWakeLockNotifications()
{
  Hal()->SendDisableWakeLockNotifications();
}

void
ModifyWakeLock(const nsAString &aTopic,
               WakeLockControl aLockAdjust,
               WakeLockControl aHiddenAdjust,
               uint64_t aProcessID)
{
  MOZ_ASSERT(aProcessID != CONTENT_PROCESS_ID_UNKNOWN);
  Hal()->SendModifyWakeLock(nsString(aTopic), aLockAdjust, aHiddenAdjust, aProcessID);
}

void
GetWakeLockInfo(const nsAString &aTopic, WakeLockInformation *aWakeLockInfo)
{
  Hal()->SendGetWakeLockInfo(nsString(aTopic), aWakeLockInfo);
}
#endif /* MOZ_WAKELOCK */

bool
EnableAlarm()
{
  NS_RUNTIMEABORT("Alarms can't be programmed from sandboxed contexts.  Yet.");
  return false;
}

void
DisableAlarm()
{
  NS_RUNTIMEABORT("Alarms can't be programmed from sandboxed contexts.  Yet.");
}

bool
SetAlarm(int32_t aSeconds, int32_t aNanoseconds)
{
  NS_RUNTIMEABORT("Alarms can't be programmed from sandboxed contexts.  Yet.");
  return false;
}

void
SetProcessPriority(int aPid, ProcessPriority aPriority, uint32_t aLRU)
{
  NS_RUNTIMEABORT("Only the main process may set processes' priorities.");
}

void
SetCurrentThreadPriority(ThreadPriority aThreadPriority)
{
  NS_RUNTIMEABORT("Setting current thread priority cannot be called from sandboxed contexts.");
}

void
SetThreadPriority(PlatformThreadId aThreadId,
                  ThreadPriority aThreadPriority)
{
  NS_RUNTIMEABORT("Setting thread priority cannot be called from sandboxed contexts.");
}

void
StartDiskSpaceWatcher()
{
  NS_RUNTIMEABORT("StartDiskSpaceWatcher() can't be called from sandboxed contexts.");
}

void
StopDiskSpaceWatcher()
{
  NS_RUNTIMEABORT("StopDiskSpaceWatcher() can't be called from sandboxed contexts.");
}

nsresult StartSystemService(const char* aSvcName, const char* aArgs)
{
  NS_RUNTIMEABORT("System services cannot be controlled from sandboxed contexts.");
  return NS_ERROR_NOT_IMPLEMENTED;
}

void StopSystemService(const char* aSvcName)
{
  NS_RUNTIMEABORT("System services cannot be controlled from sandboxed contexts.");
}

bool SystemServiceIsRunning(const char* aSvcName)
{
  NS_RUNTIMEABORT("System services cannot be controlled from sandboxed contexts.");
  return false;
}

class HalParent : public PHalParent
                , public NetworkObserver
#ifdef MOZ_WAKELOCK
                , public WakeLockObserver
#endif
                , public ScreenConfigurationObserver
                , public SystemClockChangeObserver
                , public SystemTimezoneChangeObserver
{
public:
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override
  {
    // NB: you *must* unconditionally unregister your observer here,
    // if it *may* be registered below.
    hal::UnregisterNetworkObserver(this);
    hal::UnregisterScreenConfigurationObserver(this);
#ifdef MOZ_WAKELOCK
    hal::UnregisterWakeLockObserver(this);
#endif
    hal::UnregisterSystemClockChangeObserver(this);
    hal::UnregisterSystemTimezoneChangeObserver(this);
  }

  virtual bool
  RecvEnableNetworkNotifications() override {
    // We give all content access to this network-status information.
    hal::RegisterNetworkObserver(this);
    return true;
  }

  virtual bool
  RecvDisableNetworkNotifications() override {
    hal::UnregisterNetworkObserver(this);
    return true;
  }

  virtual bool
  RecvGetCurrentNetworkInformation(NetworkInformation* aNetworkInfo) override {
    hal::GetCurrentNetworkInformation(aNetworkInfo);
    return true;
  }

  void Notify(const NetworkInformation& aNetworkInfo) override {
    Unused << SendNotifyNetworkChange(aNetworkInfo);
  }

  virtual bool
  RecvEnableScreenConfigurationNotifications() override {
    // Screen configuration is used to implement CSS and DOM
    // properties, so all content already has access to this.
    hal::RegisterScreenConfigurationObserver(this);
    return true;
  }

  virtual bool
  RecvDisableScreenConfigurationNotifications() override {
    hal::UnregisterScreenConfigurationObserver(this);
    return true;
  }

  virtual bool
  RecvGetCurrentScreenConfiguration(ScreenConfiguration* aScreenConfiguration) override {
    hal::GetCurrentScreenConfiguration(aScreenConfiguration);
    return true;
  }

  virtual bool
  RecvLockScreenOrientation(const dom::ScreenOrientationInternal& aOrientation, bool* aAllowed) override
  {
    // FIXME/bug 777980: unprivileged content may only lock
    // orientation while fullscreen.  We should check whether the
    // request comes from an actor in a process that might be
    // fullscreen.  We don't have that information currently.
    *aAllowed = hal::LockScreenOrientation(aOrientation);
    return true;
  }

  virtual bool
  RecvUnlockScreenOrientation() override
  {
    hal::UnlockScreenOrientation();
    return true;
  }

  void Notify(const ScreenConfiguration& aScreenConfiguration) override {
    Unused << SendNotifyScreenConfigurationChange(aScreenConfiguration);
  }

  virtual bool
  RecvAdjustSystemClock(const int64_t &aDeltaMilliseconds) override
  {
    if (!AssertAppProcessPermission(this, "time")) {
      return false;
    }
    hal::AdjustSystemClock(aDeltaMilliseconds);
    return true;
  }

  virtual bool
  RecvSetTimezone(const nsCString& aTimezoneSpec) override
  {
    if (!AssertAppProcessPermission(this, "time")) {
      return false;
    }
    hal::SetTimezone(aTimezoneSpec);
    return true;
  }

  virtual bool
  RecvGetTimezone(nsCString *aTimezoneSpec) override
  {
    if (!AssertAppProcessPermission(this, "time")) {
      return false;
    }
    *aTimezoneSpec = hal::GetTimezone();
    return true;
  }

  virtual bool
  RecvGetTimezoneOffset(int32_t *aTimezoneOffset) override
  {
    if (!AssertAppProcessPermission(this, "time")) {
      return false;
    }
    *aTimezoneOffset = hal::GetTimezoneOffset();
    return true;
  }

  virtual bool
  RecvEnableSystemClockChangeNotifications() override
  {
    hal::RegisterSystemClockChangeObserver(this);
    return true;
  }

  virtual bool
  RecvDisableSystemClockChangeNotifications() override
  {
    hal::UnregisterSystemClockChangeObserver(this);
    return true;
  }

  virtual bool
  RecvEnableSystemTimezoneChangeNotifications() override
  {
    hal::RegisterSystemTimezoneChangeObserver(this);
    return true;
  }

  virtual bool
  RecvDisableSystemTimezoneChangeNotifications() override
  {
    hal::UnregisterSystemTimezoneChangeObserver(this);
    return true;
  }

#ifdef MOZ_WAKELOCK
  virtual bool
  RecvModifyWakeLock(const nsString& aTopic,
                     const WakeLockControl& aLockAdjust,
                     const WakeLockControl& aHiddenAdjust,
                     const uint64_t& aProcessID) override
  {
    MOZ_ASSERT(aProcessID != CONTENT_PROCESS_ID_UNKNOWN);

    // We allow arbitrary content to use wake locks.
    hal::ModifyWakeLock(aTopic, aLockAdjust, aHiddenAdjust, aProcessID);
    return true;
  }

  virtual bool
  RecvEnableWakeLockNotifications() override
  {
    // We allow arbitrary content to use wake locks.
    hal::RegisterWakeLockObserver(this);
    return true;
  }

  virtual bool
  RecvDisableWakeLockNotifications() override
  {
    hal::UnregisterWakeLockObserver(this);
    return true;
  }

  virtual bool
  RecvGetWakeLockInfo(const nsString &aTopic, WakeLockInformation *aWakeLockInfo) override
  {
    hal::GetWakeLockInfo(aTopic, aWakeLockInfo);
    return true;
  }

  void Notify(const WakeLockInformation& aWakeLockInfo) override
  {
    Unused << SendNotifyWakeLockChange(aWakeLockInfo);
  }
#endif

  void Notify(const int64_t& aClockDeltaMS) override
  {
    Unused << SendNotifySystemClockChange(aClockDeltaMS);
  }

  void Notify(const SystemTimezoneChangeInformation& aSystemTimezoneChangeInfo) override
  {
    Unused << SendNotifySystemTimezoneChange(aSystemTimezoneChangeInfo);
  }
};

class HalChild : public PHalChild {
public:
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override
  {
    sHalChildDestroyed = true;
  }

  virtual bool
  RecvNotifyNetworkChange(const NetworkInformation& aNetworkInfo) override {
    hal::NotifyNetworkChange(aNetworkInfo);
    return true;
  }

#ifdef MOZ_WAKELOCK
  virtual bool
  RecvNotifyWakeLockChange(const WakeLockInformation& aWakeLockInfo) override {
    hal::NotifyWakeLockChange(aWakeLockInfo);
    return true;
  }
#endif

  virtual bool
  RecvNotifyScreenConfigurationChange(const ScreenConfiguration& aScreenConfiguration) override {
    hal::NotifyScreenConfigurationChange(aScreenConfiguration);
    return true;
  }

  virtual bool
  RecvNotifySystemClockChange(const int64_t& aClockDeltaMS) override {
    hal::NotifySystemClockChange(aClockDeltaMS);
    return true;
  }

  virtual bool
  RecvNotifySystemTimezoneChange(
    const SystemTimezoneChangeInformation& aSystemTimezoneChangeInfo) override {
    hal::NotifySystemTimezoneChange(aSystemTimezoneChangeInfo);
    return true;
  }
};

PHalChild* CreateHalChild() {
  return new HalChild();
}

PHalParent* CreateHalParent() {
  return new HalParent();
}

} // namespace hal_sandbox
} // namespace mozilla
