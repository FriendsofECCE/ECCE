// Lists on-screen windows: pid, owner, title, width, height. The title is
// empty without the Screen Recording permission; owner and size are not.
import CoreGraphics
import Foundation

let list = CGWindowListCopyWindowInfo([.optionOnScreenOnly], kCGNullWindowID) as? [[String: Any]] ?? []
for w in list {
    let pid = w[kCGWindowOwnerPID as String] as? Int ?? 0
    let owner = w[kCGWindowOwnerName as String] as? String ?? ""
    let name = w[kCGWindowName as String] as? String ?? ""
    let b = w[kCGWindowBounds as String] as? [String: Any] ?? [:]
    let wd = (b["Width"] as? NSNumber)?.intValue ?? 0
    let ht = (b["Height"] as? NSNumber)?.intValue ?? 0
    print("\(pid)\t\(owner)\t\(name)\t\(wd)\t\(ht)")
}
