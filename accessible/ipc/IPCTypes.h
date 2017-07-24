/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_a11y_IPCTypes_h
#define mozilla_a11y_IPCTypes_h

/**
 * Since IPDL does not support preprocessing, this header file allows us to
 * define types used by PDocAccessible differently depending on platform.
 */

namespace mozilla {
namespace a11y {

typedef uint32_t IAccessibleHolder;

} // namespace a11y
} // namespace mozilla

#endif // mozilla_a11y_IPCTypes_h

