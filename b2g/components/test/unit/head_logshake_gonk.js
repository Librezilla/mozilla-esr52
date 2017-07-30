/**
 * Boostrap LogShake's tests that need gonk support.
 * This is creating a fake sdcard for LogShake tests and importing LogShake and
 * osfile
 */

/* jshint moz: true */
/* global Components, LogCapture, LogShake, ok, add_test, run_next_test, dump,
   do_get_profile, OS, equal, XPCOMUtils */
/* exported setup_logshake_mocks */

/* disable use strict warning */
/* jshint -W097 */

"use strict";

var Cu = Components.utils;
var Ci = Components.interfaces;
var Cc = Components.classes;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/osfile.jsm");

function setup_logshake_mocks() {
  do_get_profile();
  setup_fs();
}

function setup_fs() {
  OS.File.makeDir("/data/local/tmp/sdcard/", {from: "/data"}).then(function() {
    setup_sdcard();
  });
}

function setup_sdcard() {
  ensure_sdcard();
}

function ensure_sdcard() {
  run_next_test();
}
