/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// This file is a .cpp file meant to be included in nsBrowserApp.cpp and other
// similar bootstrap code. It converts wide-character windows wmain into UTF-8
// narrow-character strings.

#error This file only makes sense on Windows.
