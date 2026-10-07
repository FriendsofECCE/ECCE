// On-screen windows as pid, owner, title (JXA; the title is empty without
// the Screen Recording permission).
ObjC.import("CoreGraphics");
var l = ObjC.castRefToObject($.CGWindowListCopyWindowInfo(1, 0));
var o = [];
for (var i = 0; i < l.count; i++) {
  var d = l.objectAtIndex(i);
  o.push(ObjC.unwrap(d.objectForKey("kCGWindowOwnerPID")) + "\t" +
         ObjC.unwrap(d.objectForKey("kCGWindowOwnerName")) + "\t" +
         (ObjC.unwrap(d.objectForKey("kCGWindowName")) || ""));
}
o.join("\n");
