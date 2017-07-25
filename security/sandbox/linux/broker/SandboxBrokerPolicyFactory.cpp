/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "SandboxBrokerPolicyFactory.h"
#include "SandboxInfo.h"

#include "mozilla/ClearOnShutdown.h"
#include "mozilla/Preferences.h"
#include "nsPrintfCString.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"
#include "SpecialSystemDirectory.h"

#ifdef ANDROID
#include "cutils/properties.h"
#endif

namespace mozilla {

/* static */ bool
SandboxBrokerPolicyFactory::IsSystemSupported() {
#ifdef ANDROID
  char hardware[PROPERTY_VALUE_MAX];
  int length = property_get("ro.hardware", hardware, nullptr);
  // "goldfish" -> emulator.  Other devices can be added when we're
  // reasonably sure they work.  Eventually this won't be needed....
  if (length > 0 && strcmp(hardware, "goldfish") == 0) {
    return true;
  }

  // When broker is running in permissive mode, we enable it
  // automatically regardless of the device.
  if (SandboxInfo::Get().Test(SandboxInfo::kPermissive)) {
    return true;
  }
#endif
  return false;
}

#if defined(MOZ_CONTENT_SANDBOX)
namespace {
static const int rdonly = SandboxBroker::MAY_READ;
static const int wronly = SandboxBroker::MAY_WRITE;
static const int rdwr = rdonly | wronly;
static const int rdwrcr = rdwr | SandboxBroker::MAY_CREATE;
}
#endif

SandboxBrokerPolicyFactory::SandboxBrokerPolicyFactory()
{
  // Policy entries that are the same in every process go here, and
  // are cached over the lifetime of the factory.
#if defined(MOZ_CONTENT_SANDBOX)
  SandboxBroker::Policy* policy = new SandboxBroker::Policy;
  policy->AddDir(rdonly, "/");
  policy->AddDir(rdwrcr, "/dev/shm");
  // Add write permissions on the temporary directory. This can come
  // from various environment variables (TMPDIR,TMP,TEMP,...) so
  // make sure to use the full logic.
  nsCOMPtr<nsIFile> tmpDir;
  nsresult rv = GetSpecialSystemDirectory(OS_TemporaryDirectory,
                                          getter_AddRefs(tmpDir));
  if (NS_SUCCEEDED(rv)) {
    nsAutoCString tmpPath;
    rv = tmpDir->GetNativePath(tmpPath);
    if (NS_SUCCEEDED(rv)) {
      policy->AddDir(rdwrcr, tmpPath.get());
    }
  }
  // If the above fails at any point, fall back to a very good guess.
  if (NS_FAILED(rv)) {
    policy->AddDir(rdwrcr, "/tmp");
  }

  // Bug 1308851: NVIDIA proprietary driver when using WebGL
  policy->AddPrefix(rdwr, "/dev", "nvidia");

  // Bug 1312678: radeonsi/Intel with DRI when using WebGL
  policy->AddDir(rdwr, "/dev/dri");

  mCommonContentPolicy.reset(policy);
#endif
}

#ifdef MOZ_CONTENT_SANDBOX
UniquePtr<SandboxBroker::Policy>
SandboxBrokerPolicyFactory::GetContentPolicy(int aPid)
{
  // Policy entries that vary per-process (currently the only reason
  // that can happen is because they contain the pid) are added here.

  MOZ_ASSERT(NS_IsMainThread());
  // File broker usage is controlled through a pref.
  if (Preferences::GetInt("security.sandbox.content.level") <= 1) {
    return nullptr;
  }

  MOZ_ASSERT(mCommonContentPolicy);
  UniquePtr<SandboxBroker::Policy>
    policy(new SandboxBroker::Policy(*mCommonContentPolicy));
  // Return the common policy.
  return policy;
}

#endif // MOZ_CONTENT_SANDBOX
} // namespace mozilla
