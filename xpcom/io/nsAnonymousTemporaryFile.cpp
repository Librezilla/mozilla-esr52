/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "mozilla/dom/ContentChild.h"
#include "mozilla/SyncRunnable.h"
#include "nsAnonymousTemporaryFile.h"
#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsXULAppAPI.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsAppDirectoryServiceDefs.h"
#include "prio.h"
#include "private/pprio.h"

using namespace mozilla;

// We store the temp files in the system temp dir.
//
// On Windows systems in particular we use a sub-directory of the temp
// directory, because:
//   1. DELETE_ON_CLOSE is unreliable on Windows, in particular if we power
//      cycle (and perhaps if we crash) the files are not deleted. We store
//      the temporary files in a known sub-dir so that we can find and delete
//      them easily and quickly.
//   2. On Windows NT the system temp dir is in the user's $HomeDir/AppData,
//      so we can be sure the user always has write privileges to that directory;
//      if the sub-dir for our temp files was in some shared location and
//      was created by a privileged user, it's possible that other users
//      wouldn't have write access to that sub-dir. (Non-Windows systems
//      don't store their temp files in a sub-dir, so this isn't an issue on
//      those platforms).
//   3. Content processes can access the system temp dir
//      (NS_GetSpecialDirectory fails on NS_APP_USER_PROFILE_LOCAL_50_DIR
//      for content process for example, which is where we previously stored
//      temp files on Windows). This argument applies to all platforms, not
//      just Windows.
static nsresult
GetTempDir(nsIFile** aTempDir)
{
  if (NS_WARN_IF(!aTempDir)) {
    return NS_ERROR_INVALID_ARG;
  }
  nsCOMPtr<nsIFile> tmpFile;
  nsresult rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(tmpFile));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  tmpFile.forget(aTempDir);

  return NS_OK;
}

namespace {

class nsRemoteAnonymousTemporaryFileRunnable : public Runnable
{
public:
  dom::FileDescOrError *mResultPtr;
  explicit nsRemoteAnonymousTemporaryFileRunnable(dom::FileDescOrError *aResultPtr)
  : mResultPtr(aResultPtr)
  { }

protected:
  NS_IMETHOD Run() override {
    dom::ContentChild* child = dom::ContentChild::GetSingleton();
    MOZ_ASSERT(child);
    child->SendOpenAnonymousTemporaryFile(mResultPtr);
    return NS_OK;
  }
};

} // namespace

nsresult
NS_OpenAnonymousTemporaryFile(PRFileDesc** aOutFileDesc)
{
  if (NS_WARN_IF(!aOutFileDesc)) {
    return NS_ERROR_INVALID_ARG;
  }

  if (dom::ContentChild* child = dom::ContentChild::GetSingleton()) {
    dom::FileDescOrError fd = NS_OK;
    if (NS_IsMainThread()) {
      child->SendOpenAnonymousTemporaryFile(&fd);
    } else {
      nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
      MOZ_ASSERT(mainThread);
      SyncRunnable::DispatchToThread(mainThread,
        new nsRemoteAnonymousTemporaryFileRunnable(&fd));
    }
    if (fd.type() == dom::FileDescOrError::Tnsresult) {
      nsresult rv = fd.get_nsresult();
      MOZ_ASSERT(NS_FAILED(rv));
      return rv;
    }
    auto rawFD = fd.get_FileDescriptor().ClonePlatformHandle();
    *aOutFileDesc = PR_ImportFile(PROsfd(rawFD.release()));
    return NS_OK;
  }

  nsresult rv;
  nsCOMPtr<nsIFile> tmpFile;
  rv = GetTempDir(getter_AddRefs(tmpFile));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Give the temp file a name with a random element. CreateUnique will also
  // append a counter to the name if it encounters a name collision. Adding
  // a random element to the name reduces the likelihood of a name collision,
  // so that CreateUnique() doesn't end up trying a lot of name variants in
  // its "try appending an incrementing counter" loop, as file IO can be
  // expensive on some mobile flash drives.
  nsAutoCString name("mozilla-temp-");
  name.AppendInt(rand());

  rv = tmpFile->AppendNative(name);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = tmpFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0700);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = tmpFile->OpenNSPRFileDesc(PR_RDWR | nsIFile::DELETE_ON_CLOSE,
                                 PR_IRWXU, aOutFileDesc);

  return rv;
}
