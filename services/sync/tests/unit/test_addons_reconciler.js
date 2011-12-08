/* Any copyright is dedicated to the Public Domain.
   http://creativecommons.org/publicdomain/zero/1.0/ */

"use strict";

Cu.import("resource://services-sync/addonsreconciler.js");
Cu.import("resource://gre/modules/AddonManager.jsm");

loadAddonTestFunctions();
startupManager();

function run_test() {
  initTestLogging("Trace");
  Log4Moz.repository.getLogger("Sync.AddonsReconciler").level = Log4Moz.Level.Trace;

  run_next_test();
}

add_test(function test_defaults() {
  _("Ensure new objects have reasonable defaults.");

  let reconciler = new AddonsReconciler();

  do_check_true(reconciler._listening);
  do_check_eq("object", typeof(reconciler.addons));
  do_check_eq(0, Object.keys(reconciler.addons).length);
  do_check_eq(0, reconciler._changes.length);
  do_check_eq(0, reconciler._listeners.length);

  run_next_test();
});

add_test(function test_load_state_empty_file() {
  _("Ensure loading from a missing file results in defaults being set.");

  let reconciler = new AddonsReconciler();

  reconciler.loadState(null, function(error, loaded) {
    do_check_eq(null, error);
    do_check_false(loaded);

    do_check_eq("object", typeof(reconciler.addons));
    do_check_eq(0, Object.keys(reconciler.addons).length);
    do_check_eq(0, reconciler._changes.length);

    run_next_test();
  });
});

add_test(function test_install_detection() {
  _("Ensure that add-on installation results in appropriate side-effects.");

  let reconciler = new AddonsReconciler();

  let before = new Date();
  let addon = installAddon("test_bootstrap1_1");
  let after = new Date();

  do_check_eq(1, Object.keys(reconciler.addons).length);
  do_check_true(addon.id in reconciler.addons);
  let record = reconciler.addons[addon.id];

  const KEYS = ["id", "guid", "enabled", "installed", "modified", "type",
                "scope", "foreignInstall"];
  for each (let key in KEYS) {
    do_check_true(key in record);
    do_check_neq(null, record[key]);
  }

  do_check_eq(addon.id, record.id);
  do_check_eq(addon.syncGUID, record.guid);
  do_check_true(record.enabled);
  do_check_true(record.installed);
  do_check_true(record.modified >= before && record.modified <= after);
  do_check_eq("extension", record.type);
  do_check_false(record.foreignInstall);

  do_check_eq(1, reconciler._changes.length);
  let change = reconciler._changes[0];
  do_check_true(change[0] >= before && change[1] <= after);
  do_check_eq(CHANGE_INSTALLED, change[1]);
  do_check_eq(addon.id, change[2]);

  uninstallAddon(addon);

  run_next_test();
});

add_test(function test_uninstall_detection() {
  _("Ensure that add-on uninstallation results in appropriate side-effects.");

  let reconciler = new AddonsReconciler();
  reconciler._addons = {};
  reconciler._changes = [];

  let addon = installAddon("test_bootstrap1_1");
  let id = addon.id;
  let guid = addon.syncGUID;

  reconciler._changes = [];
  uninstallAddon(addon);

  do_check_eq(1, Object.keys(reconciler.addons).length);
  do_check_true(id in reconciler.addons);

  let record = reconciler.addons[id];
  do_check_false(record.installed);

  do_check_eq(1, reconciler._changes.length);
  let change = reconciler._changes[0];
  do_check_eq(CHANGE_UNINSTALLED, change[1]);
  do_check_eq(id, change[2]);

  run_next_test();
});

add_test(function test_load_state_future_version() {
  _("Ensure loading a file from a future version results in no data loaded.");

  const FILENAME = "TEST_LOAD_STATE_FUTURE_VERSION";

  let reconciler = new AddonsReconciler();

  // First we populate our new file.
  let state = {version: 100, addons: {foo: {}}, changes: [[1, 1, "foo"]]};
  let cb = Async.makeSyncCallback();

  // jsonSave() expects an object with ._log, so we give it a reconciler
  // instance.
  Utils.jsonSave(FILENAME, reconciler, state, cb);
  Async.waitForSyncCallback(cb);

  reconciler.loadState(FILENAME, function(error, loaded) {
    do_check_eq(null, error);
    do_check_false(loaded);

    do_check_eq("object", typeof(reconciler.addons));
    do_check_eq(0, Object.keys(reconciler.addons).length);
    do_check_eq(0, reconciler._changes);

    run_next_test();
  });
});
