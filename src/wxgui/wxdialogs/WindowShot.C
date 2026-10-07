// A wxClientDC blit misses everything Cocoa draws in native subviews
// (labels, tab pages, panel backgrounds: black shots). On macOS the
// window is therefore read back from the window server, which is allowed
// for a program's own windows without the Screen Recording permission.
#include "wx/wxprec.h"
#include "wx/wx.h"
#include "wx/dcmemory.h"
#include "wx/image.h"

#include "wxgui/WindowShot.H"

#ifdef __WXOSX__
#include <unistd.h>
#include <vector>
#include <ApplicationServices/ApplicationServices.h>

static bool cgWindowShot(wxWindow* win, const wxString& file)
{
    wxWindow* top = wxGetTopLevelParent(win);
    wxRect r(top->GetScreenPosition(), top->GetSize());

    CFArrayRef list = CGWindowListCopyWindowInfo(
        kCGWindowListOptionOnScreenOnly, kCGNullWindowID);
    if (list == NULL)
        return false;
    CGWindowID id = kCGNullWindowID;
    long best = 1L << 30;
    for (CFIndex i = 0; i < CFArrayGetCount(list); i++)
    {
        CFDictionaryRef d = (CFDictionaryRef)CFArrayGetValueAtIndex(list, i);
        int pid = 0, layer = 0, num = 0;
        CFNumberRef n = (CFNumberRef)CFDictionaryGetValue(d, kCGWindowOwnerPID);
        if (n) CFNumberGetValue(n, kCFNumberIntType, &pid);
        n = (CFNumberRef)CFDictionaryGetValue(d, kCGWindowLayer);
        if (n) CFNumberGetValue(n, kCFNumberIntType, &layer);
        if (pid != (int)getpid() || layer != 0)
            continue;
        CGRect b;
        CFDictionaryRef bd = (CFDictionaryRef)CFDictionaryGetValue(d, kCGWindowBounds);
        if (bd == NULL || !CGRectMakeWithDictionaryRepresentation(bd, &b))
            continue;
        n = (CFNumberRef)CFDictionaryGetValue(d, kCGWindowNumber);
        if (n) CFNumberGetValue(n, kCFNumberIntType, &num);
        // Our frame's outer rectangle, in the same top-left point space.
        long dist = labs((long)b.origin.x - r.x) + labs((long)b.origin.y - r.y) +
                    labs((long)b.size.width - r.width) +
                    labs((long)b.size.height - r.height);
        if (dist < best)
        {
            best = dist;
            id = (CGWindowID)num;
        }
    }
    CFRelease(list);
    if (id == kCGNullWindowID)
        return false;

    CGImageRef img = CGWindowListCreateImage(
        CGRectNull, kCGWindowListOptionIncludingWindow, id,
        kCGWindowImageBoundsIgnoreFraming);
    if (img == NULL)
        return false;
    size_t w = CGImageGetWidth(img), h = CGImageGetHeight(img);
    std::vector<unsigned char> px(w * h * 4);
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(&px[0], w, h, 8, w * 4, cs,
        kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    bool ok = false;
    if (ctx != NULL)
    {
        CGContextDrawImage(ctx, CGRectMake(0, 0, w, h), img);
        wxImage out((int)w, (int)h);
        unsigned char* rgb = out.GetData();
        for (size_t i = 0; i < w * h; i++)
        {
            // Premultiplied over black is what a window on black looks like;
            // the window's own pixels are opaque.
            rgb[3 * i] = px[4 * i];
            rgb[3 * i + 1] = px[4 * i + 1];
            rgb[3 * i + 2] = px[4 * i + 2];
        }
        ok = out.SaveFile(file, wxBITMAP_TYPE_PNG);
        CGContextRelease(ctx);
    }
    CGColorSpaceRelease(cs);
    CGImageRelease(img);
    return ok;
}
#endif


bool ecceWindowShot(wxWindow* win, const wxString& file)
{
    win->Raise();
    for (int i = 0; i < 3; i++)
    {
        win->Update();
        wxTheApp->Yield(true);
        wxMilliSleep(50);
    }
#ifdef __WXOSX__
    if (cgWindowShot(win, file))
        return true;
#endif
    wxSize sz = win->GetClientSize();
    wxClientDC screen(win);
    wxBitmap bmp(sz.x, sz.y);
    wxMemoryDC mem(bmp);
    mem.Blit(0, 0, sz.x, sz.y, &screen, 0, 0);
    mem.SelectObject(wxNullBitmap);
    return bmp.ConvertToImage().SaveFile(file, wxBITMAP_TYPE_PNG);
}
