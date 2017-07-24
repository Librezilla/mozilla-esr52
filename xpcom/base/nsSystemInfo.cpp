/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/ArrayUtils.h"

#include "nsSystemInfo.h"
#include "prsystem.h"
#include "prio.h"
#include "prprf.h"
#include "mozilla/SSE.h"
#include "mozilla/arm.h"
#include "mozilla/Sprintf.h"

#ifdef XP_MACOSX
#include "MacHelpers.h"
#endif

#ifdef MOZ_WIDGET_GTK
#include <gtk/gtk.h>
#include <dlfcn.h>
#endif

#if defined (XP_LINUX) && !defined (ANDROID)
#include <unistd.h>
#include <fstream>
#include "mozilla/Tokenizer.h"
#include "nsCharSeparatedTokenizer.h"

#include <map>
#include <string>
#endif

#ifdef MOZ_WIDGET_ANDROID
#include "AndroidBridge.h"
#include "mozilla/dom/ContentChild.h"
#endif

#ifdef MOZ_WIDGET_GONK
#include <sys/system_properties.h>
#include "mozilla/Preferences.h"
#include "nsPrintfCString.h"
#endif

#ifdef ANDROID
extern "C" {
NS_EXPORT int android_sdk_version;
}
#endif

#ifdef XP_MACOSX
#include <sys/sysctl.h>
#endif

#if defined(XP_LINUX) && defined(MOZ_SANDBOX)
#include "mozilla/SandboxInfo.h"
#endif

// Slot for NS_InitXPCOM2 to pass information to nsSystemInfo::Init.
// Only set to nonzero (potentially) if XP_UNIX.  On such systems, the
// system call to discover the appropriate value is not thread-safe,
// so we must call it before going multithreaded, but nsSystemInfo::Init
// only happens well after that point.
uint32_t nsSystemInfo::gUserUmask = 0;

#if defined (XP_LINUX) && !defined (ANDROID)
static void
SimpleParseKeyValuePairs(const std::string& aFilename,
                         std::map<nsCString, nsCString>& aKeyValuePairs)
{
  std::ifstream input(aFilename.c_str());
  for (std::string line; std::getline(input, line); ) {
    nsAutoCString key, value;

    nsCCharSeparatedTokenizer tokens(nsDependentCString(line.c_str()), ':');
    if (tokens.hasMoreTokens()) {
      key = tokens.nextToken();
      if (tokens.hasMoreTokens()) {
        value = tokens.nextToken();
      }
      // We want the value even if there was just one token, to cover the
      // case where we had the key, and the value was blank (seems to be
      // a valid scenario some files.)
      aKeyValuePairs[key] = value;
    }
  }
}
#endif

using namespace mozilla;

nsSystemInfo::nsSystemInfo()
{
}

nsSystemInfo::~nsSystemInfo()
{
}

// CPU-specific information.
static const struct PropItems
{
  const char* name;
  bool (*propfun)(void);
} cpuPropItems[] = {
  // x86-specific bits.
  { "hasMMX", mozilla::supports_mmx },
  { "hasSSE", mozilla::supports_sse },
  { "hasSSE2", mozilla::supports_sse2 },
  { "hasSSE3", mozilla::supports_sse3 },
  { "hasSSSE3", mozilla::supports_ssse3 },
  { "hasSSE4A", mozilla::supports_sse4a },
  { "hasSSE4_1", mozilla::supports_sse4_1 },
  { "hasSSE4_2", mozilla::supports_sse4_2 },
  { "hasAVX", mozilla::supports_avx },
  { "hasAVX2", mozilla::supports_avx2 },
  // ARM-specific bits.
  { "hasEDSP", mozilla::supports_edsp },
  { "hasARMv6", mozilla::supports_armv6 },
  { "hasARMv7", mozilla::supports_armv7 },
  { "hasNEON", mozilla::supports_neon }
};

nsresult
nsSystemInfo::Init()
{
  nsresult rv;

  static const struct
  {
    PRSysInfo cmd;
    const char* name;
  } items[] = {
    { PR_SI_SYSNAME, "name" },
    { PR_SI_HOSTNAME, "host" },
    { PR_SI_ARCHITECTURE, "arch" },
    { PR_SI_RELEASE, "version" }
  };

  for (uint32_t i = 0; i < (sizeof(items) / sizeof(items[0])); i++) {
    char buf[SYS_INFO_BUFFER_LENGTH];
    if (PR_GetSystemInfo(items[i].cmd, buf, sizeof(buf)) == PR_SUCCESS) {
      rv = SetPropertyAsACString(NS_ConvertASCIItoUTF16(items[i].name),
                                 nsDependentCString(buf));
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    } else {
      NS_WARNING("PR_GetSystemInfo failed");
    }
  }

  rv = SetPropertyAsBool(NS_ConvertASCIItoUTF16("hasWindowsTouchInterface"),
                         false);
  NS_ENSURE_SUCCESS(rv, rv);

  // Additional informations not available through PR_GetSystemInfo.
  SetInt32Property(NS_LITERAL_STRING("pagesize"), PR_GetPageSize());
  SetInt32Property(NS_LITERAL_STRING("pageshift"), PR_GetPageShift());
  SetInt32Property(NS_LITERAL_STRING("memmapalign"), PR_GetMemMapAlignment());
  SetUint64Property(NS_LITERAL_STRING("memsize"), PR_GetPhysicalMemorySize());
  SetUint32Property(NS_LITERAL_STRING("umask"), nsSystemInfo::gUserUmask);

  uint64_t virtualMem = 0;
  nsAutoCString cpuVendor;
  int cpuSpeed = -1;
  int cpuFamily = -1;
  int cpuModel = -1;
  int cpuStepping = -1;
  int logicalCPUs = -1;
  int physicalCPUs = -1;
  int cacheSizeL2 = -1;
  int cacheSizeL3 = -1;

#if defined (XP_MACOSX)
  // CPU speed
  uint64_t sysctlValue64 = 0;
  uint32_t sysctlValue32 = 0;
  size_t len = 0;
  len = sizeof(sysctlValue64);
  if (!sysctlbyname("hw.cpufrequency_max", &sysctlValue64, &len, NULL, 0)) {
    cpuSpeed = static_cast<int>(sysctlValue64/1000000);
  }
  MOZ_ASSERT(sizeof(sysctlValue64) == len);

  len = sizeof(sysctlValue32);
  if (!sysctlbyname("hw.physicalcpu_max", &sysctlValue32, &len, NULL, 0)) {
    physicalCPUs = static_cast<int>(sysctlValue32);
  }
  MOZ_ASSERT(sizeof(sysctlValue32) == len);

  len = sizeof(sysctlValue32);
  if (!sysctlbyname("hw.logicalcpu_max", &sysctlValue32, &len, NULL, 0)) {
    logicalCPUs = static_cast<int>(sysctlValue32);
  }
  MOZ_ASSERT(sizeof(sysctlValue32) == len);

  len = sizeof(sysctlValue64);
  if (!sysctlbyname("hw.l2cachesize", &sysctlValue64, &len, NULL, 0)) {
    cacheSizeL2 = static_cast<int>(sysctlValue64/1024);
  }
  MOZ_ASSERT(sizeof(sysctlValue64) == len);

  len = sizeof(sysctlValue64);
  if (!sysctlbyname("hw.l3cachesize", &sysctlValue64, &len, NULL, 0)) {
    cacheSizeL3 = static_cast<int>(sysctlValue64/1024);
  }
  MOZ_ASSERT(sizeof(sysctlValue64) == len);

  if (!sysctlbyname("machdep.cpu.vendor", NULL, &len, NULL, 0)) {
    char* cpuVendorStr = new char[len];
    if (!sysctlbyname("machdep.cpu.vendor", cpuVendorStr, &len, NULL, 0)) {
      cpuVendor = cpuVendorStr;
    }
    delete [] cpuVendorStr;
  }

  len = sizeof(sysctlValue32);
  if (!sysctlbyname("machdep.cpu.family", &sysctlValue32, &len, NULL, 0)) {
    cpuFamily = static_cast<int>(sysctlValue32);
  }
  MOZ_ASSERT(sizeof(sysctlValue32) == len);

  len = sizeof(sysctlValue32);
  if (!sysctlbyname("machdep.cpu.model", &sysctlValue32, &len, NULL, 0)) {
    cpuModel = static_cast<int>(sysctlValue32);
  }
  MOZ_ASSERT(sizeof(sysctlValue32) == len);

  len = sizeof(sysctlValue32);
  if (!sysctlbyname("machdep.cpu.stepping", &sysctlValue32, &len, NULL, 0)) {
    cpuStepping = static_cast<int>(sysctlValue32);
  }
  MOZ_ASSERT(sizeof(sysctlValue32) == len);

#elif defined (XP_LINUX) && !defined (ANDROID)
  // Get vendor, family, model, stepping, physical cores, L3 cache size
  // from /proc/cpuinfo file
  {
    std::map<nsCString, nsCString> keyValuePairs;
    SimpleParseKeyValuePairs("/proc/cpuinfo", keyValuePairs);

    // cpuVendor from "vendor_id"
    cpuVendor.Assign(keyValuePairs[NS_LITERAL_CSTRING("vendor_id")]);

    {
      // cpuFamily from "cpu family"
      Tokenizer::Token t;
      Tokenizer p(keyValuePairs[NS_LITERAL_CSTRING("cpu family")]);
      if (p.Next(t) && t.Type() == Tokenizer::TOKEN_INTEGER &&
          t.AsInteger() <= INT32_MAX) {
        cpuFamily = static_cast<int>(t.AsInteger());
      }
    }

    {
      // cpuModel from "model"
      Tokenizer::Token t;
      Tokenizer p(keyValuePairs[NS_LITERAL_CSTRING("model")]);
      if (p.Next(t) && t.Type() == Tokenizer::TOKEN_INTEGER &&
          t.AsInteger() <= INT32_MAX) {
        cpuModel = static_cast<int>(t.AsInteger());
      }
    }

    {
      // cpuStepping from "stepping"
      Tokenizer::Token t;
      Tokenizer p(keyValuePairs[NS_LITERAL_CSTRING("stepping")]);
      if (p.Next(t) && t.Type() == Tokenizer::TOKEN_INTEGER &&
          t.AsInteger() <= INT32_MAX) {
        cpuStepping = static_cast<int>(t.AsInteger());
      }
    }

    {
      // physicalCPUs from "cpu cores"
      Tokenizer::Token t;
      Tokenizer p(keyValuePairs[NS_LITERAL_CSTRING("cpu cores")]);
      if (p.Next(t) && t.Type() == Tokenizer::TOKEN_INTEGER &&
          t.AsInteger() <= INT32_MAX) {
        physicalCPUs = static_cast<int>(t.AsInteger());
      }
    }

    {
      // cacheSizeL3 from "cache size"
      Tokenizer::Token t;
      Tokenizer p(keyValuePairs[NS_LITERAL_CSTRING("cache size")]);
      if (p.Next(t) && t.Type() == Tokenizer::TOKEN_INTEGER &&
          t.AsInteger() <= INT32_MAX) {
        cacheSizeL3 = static_cast<int>(t.AsInteger());
        if (p.Next(t) && t.Type() == Tokenizer::TOKEN_WORD &&
            t.AsString() != NS_LITERAL_CSTRING("KB")) {
          // If we get here, there was some text after the cache size value
          // and that text was not KB.  For now, just don't report the
          // L3 cache.
          cacheSizeL3 = -1;
        }
      }
    }
  }

  {
    // Get cpuSpeed from another file.
    std::ifstream input("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
    std::string line;
    if (getline(input, line)) {
      Tokenizer::Token t;
      Tokenizer p(line.c_str());
      if (p.Next(t) && t.Type() == Tokenizer::TOKEN_INTEGER &&
          t.AsInteger() <= INT32_MAX) {
        cpuSpeed = static_cast<int>(t.AsInteger()/1000);
      }
    }
  }

  {
    // Get cacheSizeL2 from yet another file
    std::ifstream input("/sys/devices/system/cpu/cpu0/cache/index2/size");
    std::string line;
    if (getline(input, line)) {
      Tokenizer::Token t;
      Tokenizer p(line.c_str(), nullptr, "K");
      if (p.Next(t) && t.Type() == Tokenizer::TOKEN_INTEGER &&
          t.AsInteger() <= INT32_MAX) {
        cacheSizeL2 = static_cast<int>(t.AsInteger());
      }
    }
  }

  SetInt32Property(NS_LITERAL_STRING("cpucount"), PR_GetNumberOfProcessors());
#else
  SetInt32Property(NS_LITERAL_STRING("cpucount"), PR_GetNumberOfProcessors());
#endif

  if (virtualMem) SetUint64Property(NS_LITERAL_STRING("virtualmemsize"), virtualMem);
  if (cpuSpeed >= 0) SetInt32Property(NS_LITERAL_STRING("cpuspeed"), cpuSpeed);
  if (!cpuVendor.IsEmpty()) SetPropertyAsACString(NS_LITERAL_STRING("cpuvendor"), cpuVendor);
  if (cpuFamily >= 0) SetInt32Property(NS_LITERAL_STRING("cpufamily"), cpuFamily);
  if (cpuModel >= 0) SetInt32Property(NS_LITERAL_STRING("cpumodel"), cpuModel);
  if (cpuStepping >= 0) SetInt32Property(NS_LITERAL_STRING("cpustepping"), cpuStepping);

  if (logicalCPUs >= 0) SetInt32Property(NS_LITERAL_STRING("cpucount"), logicalCPUs);
  if (physicalCPUs >= 0) SetInt32Property(NS_LITERAL_STRING("cpucores"), physicalCPUs);

  if (cacheSizeL2 >= 0) SetInt32Property(NS_LITERAL_STRING("cpucachel2"), cacheSizeL2);
  if (cacheSizeL3 >= 0) SetInt32Property(NS_LITERAL_STRING("cpucachel3"), cacheSizeL3);

  for (uint32_t i = 0; i < ArrayLength(cpuPropItems); i++) {
    rv = SetPropertyAsBool(NS_ConvertASCIItoUTF16(cpuPropItems[i].name),
                           cpuPropItems[i].propfun());
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

#if defined(XP_MACOSX)
  nsAutoString countryCode;
  if (NS_SUCCEEDED(GetSelectedCityInfo(countryCode))) {
    rv = SetPropertyAsAString(NS_LITERAL_STRING("countryCode"), countryCode);
    NS_ENSURE_SUCCESS(rv, rv);
  }
#endif

#if defined(MOZ_WIDGET_GTK)
  // This must be done here because NSPR can only separate OS's when compiled, not libraries.
  // 64 bytes is going to be well enough for "GTK " followed by 3 integers
  // separated with dots.
  char gtkver[64];
  ssize_t gtkver_len = 0;

#if MOZ_WIDGET_GTK == 2
  extern int gtk_read_end_of_the_pipe;

  if (gtk_read_end_of_the_pipe != -1) {
    do {
      gtkver_len = read(gtk_read_end_of_the_pipe, &gtkver, sizeof(gtkver));
    } while (gtkver_len < 0 && errno == EINTR);
    close(gtk_read_end_of_the_pipe);
  }
#endif

  if (gtkver_len <= 0) {
    gtkver_len = SprintfLiteral(gtkver, "GTK %u.%u.%u", gtk_major_version,
                                gtk_minor_version, gtk_micro_version);
  }

  nsAutoCString secondaryLibrary;
  if (gtkver_len > 0) {
    secondaryLibrary.Append(nsDependentCSubstring(gtkver, gtkver_len));
  }

  void* libpulse = dlopen("libpulse.so.0", RTLD_LAZY);
  const char* libpulseVersion = "not-available";
  if (libpulse) {
    auto pa_get_library_version = reinterpret_cast<const char* (*)()>
      (dlsym(libpulse, "pa_get_library_version"));

    if (pa_get_library_version) {
      libpulseVersion = pa_get_library_version();
    }
  }

  secondaryLibrary.AppendPrintf(",libpulse %s", libpulseVersion);

  if (libpulse) {
    dlclose(libpulse);
  }

  rv = SetPropertyAsACString(NS_LITERAL_STRING("secondaryLibrary"),
                             secondaryLibrary);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
#endif

#ifdef MOZ_WIDGET_ANDROID
  AndroidSystemInfo info;
  if (XRE_IsContentProcess()) {
    dom::ContentChild* child = dom::ContentChild::GetSingleton();
    if (child) {
      child->SendGetAndroidSystemInfo(&info);
      SetupAndroidInfo(info);
    }
  } else {
    GetAndroidSystemInfo(&info);
    SetupAndroidInfo(info);
  }
#endif

#ifdef MOZ_WIDGET_GONK
  char sdk[PROP_VALUE_MAX];
  if (__system_property_get("ro.build.version.sdk", sdk)) {
    android_sdk_version = atoi(sdk);
    SetPropertyAsInt32(NS_LITERAL_STRING("sdk_version"), android_sdk_version);

    SetPropertyAsACString(NS_LITERAL_STRING("secondaryLibrary"),
                          nsPrintfCString("SDK %u", android_sdk_version));
  }

  char characteristics[PROP_VALUE_MAX];
  if (__system_property_get("ro.build.characteristics", characteristics)) {
    if (!strcmp(characteristics, "tablet")) {
      SetPropertyAsBool(NS_LITERAL_STRING("tablet"), true);
    } else if (!strcmp(characteristics, "tv")) {
      SetPropertyAsBool(NS_LITERAL_STRING("tv"), true);
    }
  }

  nsAutoString str;
  rv = GetPropertyAsAString(NS_LITERAL_STRING("version"), str);
  if (NS_SUCCEEDED(rv)) {
    SetPropertyAsAString(NS_LITERAL_STRING("kernel_version"), str);
  }

  const nsAdoptingString& b2g_os_name =
    mozilla::Preferences::GetString("b2g.osName");
  if (b2g_os_name) {
    SetPropertyAsAString(NS_LITERAL_STRING("name"), b2g_os_name);
  }

  const nsAdoptingString& b2g_version =
    mozilla::Preferences::GetString("b2g.version");
  if (b2g_version) {
    SetPropertyAsAString(NS_LITERAL_STRING("version"), b2g_version);
  }
#endif

#if defined(XP_LINUX) && defined(MOZ_SANDBOX)
  SandboxInfo sandInfo = SandboxInfo::Get();

  SetPropertyAsBool(NS_LITERAL_STRING("hasSeccompBPF"),
                    sandInfo.Test(SandboxInfo::kHasSeccompBPF));
  SetPropertyAsBool(NS_LITERAL_STRING("hasSeccompTSync"),
                    sandInfo.Test(SandboxInfo::kHasSeccompTSync));
  SetPropertyAsBool(NS_LITERAL_STRING("hasUserNamespaces"),
                    sandInfo.Test(SandboxInfo::kHasUserNamespaces));
  SetPropertyAsBool(NS_LITERAL_STRING("hasPrivilegedUserNamespaces"),
                    sandInfo.Test(SandboxInfo::kHasPrivilegedUserNamespaces));

  if (sandInfo.Test(SandboxInfo::kEnabledForContent)) {
    SetPropertyAsBool(NS_LITERAL_STRING("canSandboxContent"),
                      sandInfo.CanSandboxContent());
  }

  if (sandInfo.Test(SandboxInfo::kEnabledForMedia)) {
    SetPropertyAsBool(NS_LITERAL_STRING("canSandboxMedia"),
                      sandInfo.CanSandboxMedia());
  }
#endif // XP_LINUX && MOZ_SANDBOX

  return NS_OK;
}

#ifdef MOZ_WIDGET_ANDROID
// Prerelease versions of Android use a letter instead of version numbers.
// Unfortunately this breaks websites due to the user agent.
// Chrome works around this by hardcoding an Android version when a
// numeric version can't be obtained. We're doing the same.
// This version will need to be updated whenever there is a new official
// Android release.
// See: https://cs.chromium.org/chromium/src/base/sys_info_android.cc?l=61
#define DEFAULT_ANDROID_VERSION "6.0.99"

/* static */
void
nsSystemInfo::GetAndroidSystemInfo(AndroidSystemInfo* aInfo)
{
  MOZ_ASSERT(XRE_IsParentProcess());

  if (!mozilla::AndroidBridge::Bridge()) {
    aInfo->sdk_version() = 0;
    return;
  }

  nsAutoString str;
  if (mozilla::AndroidBridge::Bridge()->GetStaticStringField(
      "android/os/Build", "MODEL", str)) {
    aInfo->device() = str;
  }
  if (mozilla::AndroidBridge::Bridge()->GetStaticStringField(
      "android/os/Build", "MANUFACTURER", str)) {
    aInfo->manufacturer() = str;
  }
  if (mozilla::AndroidBridge::Bridge()->GetStaticStringField(
      "android/os/Build$VERSION", "RELEASE", str)) {
    int major_version;
    int minor_version;
    int bugfix_version;
    int num_read = sscanf(NS_ConvertUTF16toUTF8(str).get(), "%d.%d.%d", &major_version, &minor_version, &bugfix_version);
    if (num_read == 0) {
      aInfo->release_version() = NS_LITERAL_STRING(DEFAULT_ANDROID_VERSION);
    } else {
      aInfo->release_version() = str;
    }
  }
  if (mozilla::AndroidBridge::Bridge()->GetStaticStringField(
      "android/os/Build", "HARDWARE", str)) {
    aInfo->hardware() = str;
  }
  int32_t sdk_version;
  if (!mozilla::AndroidBridge::Bridge()->GetStaticIntField(
      "android/os/Build$VERSION", "SDK_INT", &sdk_version)) {
    sdk_version = 0;
  }
  aInfo->sdk_version() = sdk_version;
  aInfo->isTablet() = java::GeckoAppShell::IsTablet();
}

void
nsSystemInfo::SetupAndroidInfo(const AndroidSystemInfo& aInfo)
{
  if (!aInfo.device().IsEmpty()) {
    SetPropertyAsAString(NS_LITERAL_STRING("device"), aInfo.device());
  }
  if (!aInfo.manufacturer().IsEmpty()) {
    SetPropertyAsAString(NS_LITERAL_STRING("manufacturer"), aInfo.manufacturer());
  }
  if (!aInfo.release_version().IsEmpty()) {
    SetPropertyAsAString(NS_LITERAL_STRING("release_version"), aInfo.release_version());
  }
  SetPropertyAsBool(NS_LITERAL_STRING("tablet"), aInfo.isTablet());
  // NSPR "version" is the kernel version. For Android we want the Android version.
  // Rename SDK version to version and put the kernel version into kernel_version.
  nsAutoString str;
  nsresult rv = GetPropertyAsAString(NS_LITERAL_STRING("version"), str);
  if (NS_SUCCEEDED(rv)) {
    SetPropertyAsAString(NS_LITERAL_STRING("kernel_version"), str);
  }
  // When AndroidBridge is not available (eg. in xpcshell tests), sdk_version is 0.
  if (aInfo.sdk_version() != 0) {
    android_sdk_version = aInfo.sdk_version();
    if (android_sdk_version >= 8 && !aInfo.hardware().IsEmpty()) {
      SetPropertyAsAString(NS_LITERAL_STRING("hardware"), aInfo.hardware());
    }
    SetPropertyAsInt32(NS_LITERAL_STRING("version"), android_sdk_version);
  }
}
#endif // MOZ_WIDGET_ANDROID

void
nsSystemInfo::SetInt32Property(const nsAString& aPropertyName,
                               const int32_t aValue)
{
  NS_WARNING_ASSERTION(aValue > 0, "Unable to read system value");
  if (aValue > 0) {
#ifdef DEBUG
    nsresult rv =
#endif
      SetPropertyAsInt32(aPropertyName, aValue);
    NS_WARNING_ASSERTION(NS_SUCCEEDED(rv), "Unable to set property");
  }
}

void
nsSystemInfo::SetUint32Property(const nsAString& aPropertyName,
                                const uint32_t aValue)
{
  // Only one property is currently set via this function.
  // It may legitimately be zero.
#ifdef DEBUG
  nsresult rv =
#endif
    SetPropertyAsUint32(aPropertyName, aValue);
  NS_WARNING_ASSERTION(NS_SUCCEEDED(rv), "Unable to set property");
}

void
nsSystemInfo::SetUint64Property(const nsAString& aPropertyName,
                                const uint64_t aValue)
{
  NS_WARNING_ASSERTION(aValue > 0, "Unable to read system value");
  if (aValue > 0) {
#ifdef DEBUG
    nsresult rv =
#endif
      SetPropertyAsUint64(aPropertyName, aValue);
    NS_WARNING_ASSERTION(NS_SUCCEEDED(rv), "Unable to set property");
  }
}
