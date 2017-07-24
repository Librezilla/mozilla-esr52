/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "SpecialSystemDirectory.h"
#include "nsString.h"
#include "nsDependentString.h"
#include "nsAutoPtr.h"

#if defined(XP_UNIX)

#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/param.h>
#include "prenv.h"
#if defined(MOZ_WIDGET_COCOA)
#include "CocoaFileUtils.h"
#endif

#endif

#ifndef MAXPATHLEN
#ifdef PATH_MAX
#define MAXPATHLEN PATH_MAX
#elif defined(MAX_PATH)
#define MAXPATHLEN MAX_PATH
#elif defined(_MAX_PATH)
#define MAXPATHLEN _MAX_PATH
#elif defined(CCHMAXPATH)
#define MAXPATHLEN CCHMAXPATH
#else
#define MAXPATHLEN 1024
#endif
#endif

void
StartupSpecialSystemDirectory()
{
}

#if defined(XP_UNIX)
static nsresult
GetUnixHomeDir(nsIFile** aFile)
{
#if defined(ANDROID)
  // XXX no home dir on android; maybe we should return the sdcard if present?
  return NS_ERROR_FAILURE;
#else
  return NS_NewNativeLocalFile(nsDependentCString(PR_GetEnv("HOME")),
                               true, aFile);
#endif
}

/*
  The following license applies to the xdg_user_dir_lookup function:

  Copyright (c) 2007 Red Hat, Inc.

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation files
  (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge,
  publish, distribute, sublicense, and/or sell copies of the Software,
  and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

static char*
xdg_user_dir_lookup(const char* aType)
{
  FILE* file;
  char* home_dir;
  char* config_home;
  char* config_file;
  char buffer[512];
  char* user_dir;
  char* p;
  char* d;
  int len;
  int relative;

  home_dir = getenv("HOME");

  if (!home_dir) {
    goto error;
  }

  config_home = getenv("XDG_CONFIG_HOME");
  if (!config_home || config_home[0] == 0) {
    config_file = (char*)malloc(strlen(home_dir) +
                                strlen("/.config/user-dirs.dirs") + 1);
    if (!config_file) {
      goto error;
    }

    strcpy(config_file, home_dir);
    strcat(config_file, "/.config/user-dirs.dirs");
  } else {
    config_file = (char*)malloc(strlen(config_home) +
                                strlen("/user-dirs.dirs") + 1);
    if (!config_file) {
      goto error;
    }

    strcpy(config_file, config_home);
    strcat(config_file, "/user-dirs.dirs");
  }

  file = fopen(config_file, "r");
  free(config_file);
  if (!file) {
    goto error;
  }

  user_dir = nullptr;
  while (fgets(buffer, sizeof(buffer), file)) {
    /* Remove newline at end */
    len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
      buffer[len - 1] = 0;
    }

    p = buffer;
    while (*p == ' ' || *p == '\t') {
      p++;
    }

    if (strncmp(p, "XDG_", 4) != 0) {
      continue;
    }
    p += 4;
    if (strncmp(p, aType, strlen(aType)) != 0) {
      continue;
    }
    p += strlen(aType);
    if (strncmp(p, "_DIR", 4) != 0) {
      continue;
    }
    p += 4;

    while (*p == ' ' || *p == '\t') {
      p++;
    }

    if (*p != '=') {
      continue;
    }
    p++;

    while (*p == ' ' || *p == '\t') {
      p++;
    }

    if (*p != '"') {
      continue;
    }
    p++;

    relative = 0;
    if (strncmp(p, "$HOME/", 6) == 0) {
      p += 6;
      relative = 1;
    } else if (*p != '/') {
      continue;
    }

    if (relative) {
      user_dir = (char*)malloc(strlen(home_dir) + 1 + strlen(p) + 1);
      if (!user_dir) {
        goto error2;
      }

      strcpy(user_dir, home_dir);
      strcat(user_dir, "/");
    } else {
      user_dir = (char*)malloc(strlen(p) + 1);
      if (!user_dir) {
        goto error2;
      }

      *user_dir = 0;
    }

    d = user_dir + strlen(user_dir);
    while (*p && *p != '"') {
      if ((*p == '\\') && (*(p + 1) != 0)) {
        p++;
      }
      *d++ = *p++;
    }
    *d = 0;
  }
error2:
  fclose(file);

  if (user_dir) {
    return user_dir;
  }

error:
  return nullptr;
}

static const char xdg_user_dirs[] =
  "DESKTOP\0"
  "DOCUMENTS\0"
  "DOWNLOAD\0"
  "MUSIC\0"
  "PICTURES\0"
  "PUBLICSHARE\0"
  "TEMPLATES\0"
  "VIDEOS";

static const uint8_t xdg_user_dir_offsets[] = {
  0,
  8,
  18,
  27,
  33,
  42,
  54,
  64
};

static nsresult
GetUnixXDGUserDirectory(SystemDirectories aSystemDirectory,
                        nsIFile** aFile)
{
  char* dir = xdg_user_dir_lookup(
    xdg_user_dirs + xdg_user_dir_offsets[aSystemDirectory - Unix_XDG_Desktop]);

  nsresult rv;
  nsCOMPtr<nsIFile> file;
  if (dir) {
    rv = NS_NewNativeLocalFile(nsDependentCString(dir), true,
                               getter_AddRefs(file));
    free(dir);
  } else if (Unix_XDG_Desktop == aSystemDirectory) {
    // for the XDG desktop dir, fall back to HOME/Desktop
    // (for historical compatibility)
    rv = GetUnixHomeDir(getter_AddRefs(file));
    if (NS_FAILED(rv)) {
      return rv;
    }

    rv = file->AppendNative(NS_LITERAL_CSTRING("Desktop"));
  } else {
    // no fallback for the other XDG dirs
    rv = NS_ERROR_FAILURE;
  }

  if (NS_FAILED(rv)) {
    return rv;
  }

  bool exists;
  rv = file->Exists(&exists);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (!exists) {
    rv = file->Create(nsIFile::DIRECTORY_TYPE, 0755);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  *aFile = nullptr;
  file.swap(*aFile);

  return NS_OK;
}
#endif

nsresult
GetSpecialSystemDirectory(SystemDirectories aSystemSystemDirectory,
                          nsIFile** aFile)
{
  char path[MAXPATHLEN];

  switch (aSystemSystemDirectory) {
    case OS_CurrentWorkingDirectory:
      if (!getcwd(path, MAXPATHLEN)) {
        return NS_ERROR_FAILURE;
      }

      return NS_NewNativeLocalFile(nsDependentCString(path),
                                   true,
                                   aFile);

    case OS_DriveDirectory:
    return NS_NewNativeLocalFile(nsDependentCString("/"),
                                 true,
                                 aFile);


    case OS_TemporaryDirectory:
#if defined(MOZ_WIDGET_COCOA)
    {
      return GetOSXFolderType(kUserDomain, kTemporaryFolderType, aFile);
    }

#elif defined(XP_UNIX)
    {
      static const char* tPath = nullptr;
      if (!tPath) {
        tPath = PR_GetEnv("TMPDIR");
        if (!tPath || !*tPath) {
          tPath = PR_GetEnv("TMP");
          if (!tPath || !*tPath) {
            tPath = PR_GetEnv("TEMP");
            if (!tPath || !*tPath) {
              tPath = "/tmp/";
            }
          }
        }
      }
      return NS_NewNativeLocalFile(nsDependentCString(tPath),
                                   true,
                                   aFile);
    }
#else
    break;
#endif

#if defined(XP_UNIX)
    case Unix_LocalDirectory:
      return NS_NewNativeLocalFile(nsDependentCString("/usr/local/netscape/"),
                                   true,
                                   aFile);
    case Unix_LibDirectory:
      return NS_NewNativeLocalFile(nsDependentCString("/usr/local/lib/netscape/"),
                                   true,
                                   aFile);

    case Unix_HomeDirectory:
      return GetUnixHomeDir(aFile);

    case Unix_XDG_Desktop:
    case Unix_XDG_Documents:
    case Unix_XDG_Download:
    case Unix_XDG_Music:
    case Unix_XDG_Pictures:
    case Unix_XDG_PublicShare:
    case Unix_XDG_Templates:
    case Unix_XDG_Videos:
      return GetUnixXDGUserDirectory(aSystemSystemDirectory, aFile);
#endif

    default:
      break;
  }
  return NS_ERROR_NOT_AVAILABLE;
}

#if defined (MOZ_WIDGET_COCOA)
nsresult
GetOSXFolderType(short aDomain, OSType aFolderType, nsIFile** aLocalFile)
{
  nsresult rv = NS_ERROR_FAILURE;

  if (aFolderType == kTemporaryFolderType) {
    NS_NewLocalFile(EmptyString(), true, aLocalFile);
    nsCOMPtr<nsILocalFileMac> localMacFile(do_QueryInterface(*aLocalFile));
    if (localMacFile) {
      rv = localMacFile->InitWithCFURL(
             CocoaFileUtils::GetTemporaryFolderCFURLRef());
    }
    return rv;
  }

  OSErr err;
  FSRef fsRef;
  err = ::FSFindFolder(aDomain, aFolderType, kCreateFolder, &fsRef);
  if (err == noErr) {
    NS_NewLocalFile(EmptyString(), true, aLocalFile);
    nsCOMPtr<nsILocalFileMac> localMacFile(do_QueryInterface(*aLocalFile));
    if (localMacFile) {
      rv = localMacFile->InitWithFSRef(&fsRef);
    }
  }
  return rv;
}
#endif

