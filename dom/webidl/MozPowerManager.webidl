/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

interface MozWakeLockListener;

/**
 * This interface implements navigator.mozPower
 */
[ChromeOnly]
interface MozPowerManager
{
    /**
     * The listeners are notified when a resource changes its lock state to:
     *  - unlocked
     *  - locked but not visible
     *  - locked and visible
     */
    void    addWakeLockListener(MozWakeLockListener aListener);
    void    removeWakeLockListener(MozWakeLockListener aListener);

    /**
     * Query the wake lock state of the topic.
     *
     * Possible states are:
     *
     *  - "unlocked" - nobody holds the wake lock.
     *
     *  - "locked-foreground" - at least one window holds the wake lock,
     *    and it is visible.
     *
     *  - "locked-background" - at least one window holds the wake lock,
     *    but all of them are hidden.
     *
     * @param aTopic The resource name related to the wake lock.
     */
    [Throws]
    DOMString getWakeLockState(DOMString aTopic);
};
