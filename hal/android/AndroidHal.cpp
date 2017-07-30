/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Hal.h"
#include "HalImpl.h"
#include "WindowIdentifier.h"
#include "AndroidBridge.h"
#include "mozilla/dom/network/Constants.h"
#include "nsIScreenManager.h"
#include "nsServiceManagerUtils.h"

using namespace mozilla::dom;
using namespace mozilla::hal;

namespace java = mozilla::java;

namespace mozilla {
namespace hal_impl {

void
EnableNetworkNotifications()
{
  java::GeckoAppShell::EnableNetworkNotifications();
}

void
DisableNetworkNotifications()
{
  java::GeckoAppShell::DisableNetworkNotifications();
}

void
GetCurrentNetworkInformation(hal::NetworkInformation* aNetworkInfo)
{
  AndroidBridge::Bridge()->GetCurrentNetworkInformation(aNetworkInfo);
}

void
EnableScreenConfigurationNotifications()
{
}

void
DisableScreenConfigurationNotifications()
{
}

void
GetCurrentScreenConfiguration(ScreenConfiguration* aScreenConfiguration)
{
  AndroidBridge* bridge = AndroidBridge::Bridge();
  if (!bridge) {
    return;
  }

  nsresult rv;
  nsCOMPtr<nsIScreenManager> screenMgr =
    do_GetService("@mozilla.org/gfx/screenmanager;1", &rv);
  if (NS_FAILED(rv)) {
    NS_ERROR("Can't find nsIScreenManager!");
    return;
  }

  nsIntRect rect;
  int32_t colorDepth, pixelDepth;
  int16_t angle;
  nsCOMPtr<nsIScreen> screen;

  screenMgr->GetPrimaryScreen(getter_AddRefs(screen));
  screen->GetRect(&rect.x, &rect.y, &rect.width, &rect.height);
  screen->GetColorDepth(&colorDepth);
  screen->GetPixelDepth(&pixelDepth);
  angle = 0;

  *aScreenConfiguration =
    hal::ScreenConfiguration(rect, angle, colorDepth, pixelDepth);
}

} // hal_impl
} // mozilla

