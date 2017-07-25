/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

"use strict";

const { Cc, Ci, Cu } = require("chrome");

const Environment = require("sdk/system/environment").env;
const EventEmitter = require("devtools/shared/event-emitter");
const promise = require("promise");
const Subprocess = require("sdk/system/child_process/subprocess");
const Services = require("Services");

loader.lazyGetter(this, "OS", () => {
  const Runtime = require("sdk/system/runtime");
  switch (Runtime.OS) {
    case "Darwin":
      return "mac64";
    case "Linux":
      if (Runtime.XPCOMABI.indexOf("x86_64") === 0) {
        return "linux64";
      } else {
        return "linux32";
      }
    case "WINNT":
      return "win32";
    default:
      return "";
  }
});

function SimulatorProcess() {}
SimulatorProcess.prototype = {

  // Check if B2G is running.
  get isRunning() {
    return !!this.process;
  },

  // Start the process and connect the debugger client.
  run() {

    // Ensure Gaia profile exists.
    let gaia = this.gaiaProfile;
    if (!gaia || !gaia.exists()) {
      throw Error("Gaia profile directory not found.");
    }

    this.once("stdout", function () {
      if (OS == "mac64") {
        console.debug("WORKAROUND run osascript to show b2g-desktop window on OS=='mac64'");
        // Escape double quotes and escape characters for use in AppleScript.
      }
    });

    let logHandler = (e, data) => this.log(e, data.trim());
    this.on("stdout", logHandler);
    this.on("stderr", logHandler);
    this.once("exit", () => {
      this.off("stdout", logHandler);
      this.off("stderr", logHandler);
    });

    let environment;
    if (OS.indexOf("linux") > -1) {
      environment = ["TMPDIR=" + Services.dirsvc.get("TmpD", Ci.nsIFile).path];
      ["DISPLAY", "XAUTHORITY"].forEach(key => {
        if (key in Environment) {
          environment.push(key + "=" + Environment[key]);
        }
      });
    }

  },

  // Request a B2G instance kill.
  kill() {
  },

  // Maybe log output messages.
  log(level, message) {
    if (!Services.prefs.getBoolPref("devtools.webide.logSimulatorOutput")) {
      return;
    }
    if (level === "stderr" || level === "error") {
      console.error(message);
      return;
    }
    console.log(message);
  },

  // Compute B2G CLI arguments.
  get args() {
    let args = [];

    // Gaia profile.
    args.push("-profile", this.gaiaProfile.path);

    // Debugger server port.
    let port = parseInt(this.options.port);
    args.push("-start-debugger-server", "" + port);

    // Screen size.
    let width = parseInt(this.options.width);
    let height = parseInt(this.options.height);
    if (width && height) {
      args.push("-screen", width + "x" + height);
    }

    // Ignore eventual zombie instances of b2g that are left over.
    args.push("-no-remote");

    return args;
  },
};

EventEmitter.decorate(SimulatorProcess.prototype);


function CustomSimulatorProcess(options) {
  this.options = options;
}

var CSPp = CustomSimulatorProcess.prototype = Object.create(SimulatorProcess.prototype);

// Compute Gaia profile file handle.
Object.defineProperty(CSPp, "gaiaProfile", {
  get: function () {
    let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
    file.initWithPath(this.options.gaiaProfile);
    return file;
  }
});

exports.CustomSimulatorProcess = CustomSimulatorProcess;


function AddonSimulatorProcess(addon, options) {
  this.addon = addon;
  this.options = options;
}

var ASPp = AddonSimulatorProcess.prototype = Object.create(SimulatorProcess.prototype);

// Compute Gaia profile file handle.
Object.defineProperty(ASPp, "gaiaProfile", {
  get: function () {
    let file;

    // Custom profile from simulator configuration.
    if (this.options.gaiaProfile) {
      file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
      file.initWithPath(this.options.gaiaProfile);
      return file;
    }

    // Custom profile from addon prefs.
    try {
      let pref = "extensions." + this.addon.id + ".gaiaProfile";
      file = Services.prefs.getComplexValue(pref, Ci.nsIFile);
      return file;
    } catch (e) {}

    // Default profile from addon.
    file = this.addon.getResourceURI().QueryInterface(Ci.nsIFileURL).file;
    file.append("profile");
    return file;
  }
});

exports.AddonSimulatorProcess = AddonSimulatorProcess;


function OldAddonSimulatorProcess(addon, options) {
  this.addon = addon;
  this.options = options;
}

var OASPp = OldAddonSimulatorProcess.prototype = Object.create(AddonSimulatorProcess.prototype);

// Compute B2G CLI arguments.
Object.defineProperty(OASPp, "args", {
  get: function () {
    let args = [];

    // Gaia profile.
    args.push("-profile", this.gaiaProfile.path);

    // Debugger server port.
    let port = parseInt(this.options.port);
    args.push("-dbgport", "" + port);

    // Ignore eventual zombie instances of b2g that are left over.
    args.push("-no-remote");

    return args;
  }
});

exports.OldAddonSimulatorProcess = OldAddonSimulatorProcess;
