/* -*- Mode: c++; c-basic-offset: 4; tab-width: 20; indent-tabs-mode: nil; -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef GeckoNetworkManager_h
#define GeckoNetworkManager_h

#include "GeneratedJNINatives.h"
#include "nsAppShell.h"
#include "nsCOMPtr.h"

#include "mozilla/Services.h"

namespace mozilla {

class GeckoNetworkManager final
    : public java::GeckoNetworkManager::Natives<GeckoNetworkManager>
{
    GeckoNetworkManager() = delete;
};

} // namespace mozilla

#endif // GeckoNetworkManager_h
