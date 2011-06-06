// |jit-test| debug
// frame.eval() throws if frame is not live

load(libdir + "asserts.js");

var g = newGlobal('new-compartment');
var dbg = new Debug(g);
var f;
dbg.hooks = {debuggerHandler: function (frame) { f = frame; }};
g.eval("debugger;");
assertThrowsInstanceOf(function () { f.eval("2 + 2"); }, Error);
