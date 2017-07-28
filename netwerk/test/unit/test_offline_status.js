function run_test() {
  var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                            .getService(Components.interfaces.nsIIOService);

    do_check_eq(ioService.offline, false);
}
