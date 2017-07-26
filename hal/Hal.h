/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_Hal_h
#define mozilla_Hal_h

#include "base/basictypes.h"
#include "base/platform_thread.h"
#include "nsTArray.h"
#include "mozilla/dom/network/Types.h"
#include "mozilla/dom/ScreenOrientation.h"
#include "mozilla/hal_sandbox/PHal.h"
#include "mozilla/HalScreenConfiguration.h"
#include "mozilla/HalTypes.h"
#include "mozilla/Observer.h"
#include "mozilla/Types.h"

#ifdef MOZ_POWER
#include "mozilla/dom/MozPowerManagerBinding.h"
#include "mozilla/dom/power/Types.h"
#endif

/*
 * Hal.h contains the public Hal API.
 *
 * By default, this file defines its functions in the hal namespace, but if
 * MOZ_HAL_NAMESPACE is defined, we'll define our functions in that namespace.
 *
 * This is used by HalImpl.h and HalSandbox.h, which define copies of all the
 * functions here in the hal_impl and hal_sandbox namespaces.
 */

class nsPIDOMWindowInner;

#ifndef MOZ_HAL_NAMESPACE
# define MOZ_HAL_NAMESPACE hal
# define MOZ_DEFINED_HAL_NAMESPACE 1
#endif

namespace mozilla {

namespace hal {

typedef Observer<void_t> AlarmObserver;

class WindowIdentifier;

typedef Observer<int64_t> SystemClockChangeObserver;
typedef Observer<SystemTimezoneChangeInformation> SystemTimezoneChangeObserver;

} // namespace hal

namespace MOZ_HAL_NAMESPACE {

/**
 * Inform the network backend there is a new network observer.
 * @param aNetworkObserver The observer that should be added.
 */
void RegisterNetworkObserver(NetworkObserver* aNetworkObserver);

/**
 * Inform the network backend a network observer unregistered.
 * @param aNetworkObserver The observer that should be removed.
 */
void UnregisterNetworkObserver(NetworkObserver* aNetworkObserver);

/**
 * Returns the current network information.
 */
void GetCurrentNetworkInformation(hal::NetworkInformation* aNetworkInfo);

/**
 * Notify of a change in the network state.
 * @param aNetworkInfo The new network information.
 */
void NotifyNetworkChange(const hal::NetworkInformation& aNetworkInfo);

/**
 * Adjusting system clock.
 * @param aDeltaMilliseconds The difference compared with current system clock.
 */
void AdjustSystemClock(int64_t aDeltaMilliseconds);

/**
 * Set timezone
 * @param aTimezoneSpec The definition can be found in
 * http://en.wikipedia.org/wiki/List_of_tz_database_time_zones
 */
void SetTimezone(const nsCString& aTimezoneSpec);

/**
 * Get timezone
 * http://en.wikipedia.org/wiki/List_of_tz_database_time_zones
 */
nsCString GetTimezone();

/**
 * Get timezone offset
 * returns the timezone offset relative to UTC in minutes (DST effect included)
 */
int32_t GetTimezoneOffset();

/**
 * Register observer for system clock changed notification.
 * @param aObserver The observer that should be added.
 */
void RegisterSystemClockChangeObserver(
  hal::SystemClockChangeObserver* aObserver);

/**
 * Unregister the observer for system clock changed.
 * @param aObserver The observer that should be removed.
 */
void UnregisterSystemClockChangeObserver(
  hal::SystemClockChangeObserver* aObserver);

/**
 * Notify of a change in the system clock.
 * @param aClockDeltaMS
 */
void NotifySystemClockChange(const int64_t& aClockDeltaMS);

/**
 * Register observer for system timezone changed notification.
 * @param aObserver The observer that should be added.
 */
void RegisterSystemTimezoneChangeObserver(
  hal::SystemTimezoneChangeObserver* aObserver);

/**
 * Unregister the observer for system timezone changed.
 * @param aObserver The observer that should be removed.
 */
void UnregisterSystemTimezoneChangeObserver(
  hal::SystemTimezoneChangeObserver* aObserver);

/**
 * Notify of a change in the system timezone.
 * @param aSystemTimezoneChangeInfo
 */
void NotifySystemTimezoneChange(
  const hal::SystemTimezoneChangeInformation& aSystemTimezoneChangeInfo);

#ifdef MOZ_WAKELOCK
/**
 * Enable wake lock notifications from the backend.
 *
 * This method is only used by WakeLockObserversManager.
 */
void EnableWakeLockNotifications();

/**
 * Disable wake lock notifications from the backend.
 *
 * This method is only used by WakeLockObserversManager.
 */
void DisableWakeLockNotifications();

/**
 * Inform the wake lock backend there is a new wake lock observer.
 * @param aWakeLockObserver The observer that should be added.
 */
void RegisterWakeLockObserver(WakeLockObserver* aObserver);

/**
 * Inform the wake lock backend a wake lock observer unregistered.
 * @param aWakeLockObserver The observer that should be removed.
 */
void UnregisterWakeLockObserver(WakeLockObserver* aObserver);

/**
 * Adjust a wake lock's counts on behalf of a given process.
 *
 * In most cases, you shouldn't need to pass the aProcessID argument; the
 * default of CONTENT_PROCESS_ID_UNKNOWN is probably what you want.
 *
 * @param aTopic        lock topic
 * @param aLockAdjust   to increase or decrease active locks
 * @param aHiddenAdjust to increase or decrease hidden locks
 * @param aProcessID    indicates which process we're modifying the wake lock
 *                      on behalf of.  It is interpreted as
 *
 *                      CONTENT_PROCESS_ID_UNKNOWN: The current process
 *                      CONTENT_PROCESS_ID_MAIN: The root process
 *                      X: The process with ContentChild::GetID() == X
 */
void ModifyWakeLock(const nsAString &aTopic,
                    hal::WakeLockControl aLockAdjust,
                    hal::WakeLockControl aHiddenAdjust,
                    uint64_t aProcessID = hal::CONTENT_PROCESS_ID_UNKNOWN);

/**
 * Query the wake lock numbers of aTopic.
 * @param aTopic        lock topic
 * @param aWakeLockInfo wake lock numbers
 */
void GetWakeLockInfo(const nsAString &aTopic, hal::WakeLockInformation *aWakeLockInfo);

/**
 * Notify of a change in the wake lock state.
 * @param aWakeLockInfo The new wake lock information.
 */
void NotifyWakeLockChange(const hal::WakeLockInformation& aWakeLockInfo);
#endif /* WAKELOCK */

/**
 * Inform the backend there is a new screen configuration observer.
 * @param aScreenConfigurationObserver The observer that should be added.
 */
void RegisterScreenConfigurationObserver(hal::ScreenConfigurationObserver* aScreenConfigurationObserver);

/**
 * Inform the backend a screen configuration observer unregistered.
 * @param aScreenConfigurationObserver The observer that should be removed.
 */
void UnregisterScreenConfigurationObserver(hal::ScreenConfigurationObserver* aScreenConfigurationObserver);

/**
 * Returns the current screen configuration.
 */
void GetCurrentScreenConfiguration(hal::ScreenConfiguration* aScreenConfiguration);

/**
 * Notify of a change in the screen configuration.
 * @param aScreenConfiguration The new screen orientation.
 */
void NotifyScreenConfigurationChange(const hal::ScreenConfiguration& aScreenConfiguration);

/**
 * Lock the screen orientation to the specific orientation.
 * @return Whether the lock has been accepted.
 */
MOZ_MUST_USE bool LockScreenOrientation(const dom::ScreenOrientationInternal& aOrientation);

/**
 * Unlock the screen orientation.
 */
void UnlockScreenOrientation();

/**
 * Register an observer that is notified when a programmed alarm
 * expires.
 *
 * Currently, there can only be 0 or 1 alarm observers.
 */
MOZ_MUST_USE bool RegisterTheOneAlarmObserver(hal::AlarmObserver* aObserver);

/**
 * Unregister the alarm observer.  Doing so will implicitly cancel any
 * programmed alarm.
 */
void UnregisterTheOneAlarmObserver();

/**
 * Notify that the programmed alarm has expired.
 *
 * This API is internal to hal; clients shouldn't call it directly.
 */
void NotifyAlarmFired();

/**
 * Program the real-time clock to expire at the time |aSeconds|,
 * |aNanoseconds|.  These specify a point in real time relative to the
 * UNIX epoch.  The alarm will fire at this time point even if the
 * real-time clock is changed; that is, this alarm respects changes to
 * the real-time clock.  Return true iff the alarm was programmed.
 *
 * The alarm can be reprogrammed at any time.
 *
 * This API is currently only allowed to be used from non-sandboxed
 * contexts.
 */
MOZ_MUST_USE bool SetAlarm(int32_t aSeconds, int32_t aNanoseconds);

/**
 * Set the priority of the given process.
 *
 * Exactly what this does will vary between platforms.  On *nix we might give
 * background processes higher nice values.  On other platforms, we might
 * ignore this call entirely.
 */
void SetProcessPriority(int aPid,
                        hal::ProcessPriority aPriority,
                        uint32_t aLRU = 0);


/**
 * Set the current thread's priority to appropriate platform-specific value for
 * given functionality. Instead of providing arbitrary priority numbers you
 * must specify a type of function like THREAD_PRIORITY_COMPOSITOR.
 */
void SetCurrentThreadPriority(hal::ThreadPriority aThreadPriority);

/**
 * Set a thread priority to appropriate platform-specific value for
 * given functionality. Instead of providing arbitrary priority numbers you
 * must specify a type of function like THREAD_PRIORITY_COMPOSITOR.
 */
void SetThreadPriority(PlatformThreadId aThreadId,
                       hal::ThreadPriority aThreadPriority);

/**
 * Determine whether the headphone switch event is from input device
 */
bool IsHeadphoneEventFromInputDev();

/**
 * Start the system service with the specified name and arguments.
 */
nsresult StartSystemService(const char* aSvcName, const char* aArgs);

/**
 * Stop the system service with the specified name.
 */
void StopSystemService(const char* aSvcName);

/**
 * Determine whether the system service with the specified name is running.
 */
bool SystemServiceIsRunning(const char* aSvcName);

} // namespace MOZ_HAL_NAMESPACE
} // namespace mozilla

#ifdef MOZ_DEFINED_HAL_NAMESPACE
# undef MOZ_DEFINED_HAL_NAMESPACE
# undef MOZ_HAL_NAMESPACE
#endif

#endif  // mozilla_Hal_h
