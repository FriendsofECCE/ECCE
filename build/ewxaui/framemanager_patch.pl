#!/usr/bin/env perl
#
# This script will "patch" framemanager.[CH] for ewxAUI.  See the README file
# in this directory for instructions to build ewxAUI.
#

$FRAMEMANAGER_H = "include/wx/ewxaui/framemanager.H";
$FRAMEMANAGER_C = "framemanager.C";

print "Patching $FRAMEMANAGER_H\n";

open(FILE, $FRAMEMANAGER_H) || die "Sorry, couldn't open $FRAMEMANAGER_H\n";
open(NEWFILE, ">auigen_tmpfile") || die "Sorry, couldn't open auigen_tmpfile\n";
while (<FILE>) {
  if (/sizer.h/) {
    print(NEWFILE $_);
    print(NEWFILE "#include \"wx/aui/framemanager.h\"\n");
  } elsif (/m_art/) {
    $tmp = <<EOF;
$_
    bool m_we_own_m_art;
EOF
    print(NEWFILE $tmp);
  } elsif (/SetArtProvider/) {
    print (NEWFILE "    void SetArtProvider(ewxAuiDockArt* art_provider, bool take_ownership=true);\n");
  } elsif (/^(\W+)optionDockFixed/) {
    $tmp = <<EOF;
$_
$1optionAlwaysFixed = 1 << 29,
$1optionOpen = 1 << 18,
$1optionFocus = 1 << 19,
$1optionPin = 1 << 20,
$1buttonOptions = 1 << 25,
EOF
    print(NEWFILE $tmp);
  } elsif (/^(\W+)bool HasGripperTop/) {
    $tmp = <<EOF;
$_
$1bool IsAlwaysFixed() const { return HasFlag(optionAlwaysFixed); }
$1bool IsOpen() const { return HasFlag(optionOpen); }
$1bool HasFocus() const { return HasFlag(optionFocus); }
$1bool IsPinned() const { return HasFlag(optionPin); }
$1bool HasOptionsButton() const { return HasFlag(buttonOptions); }
$1bool HasOpenButton() const { return HasFlag(buttonOpen); }
$1bool HasTakeFocusButton() const { return HasFlag(buttonTakeFocus); }
$1bool HasAddFocusButton() const { return HasFlag(buttonAddFocus); }
EOF
    print(NEWFILE $tmp);
  } elsif (/^(\W+)ewxAuiPaneInfo& Movable/) {
    $tmp = <<EOF;
$_
$1ewxAuiPaneInfo& AlwaysFixed(bool b = true) { if (b) { Fixed(); } else { Resizable(); } return SetFlag(optionAlwaysFixed, b); }
$1ewxAuiPaneInfo& Open(bool b = true) { return SetFlag(optionOpen, b); }
$1ewxAuiPaneInfo& Focus(bool b = true) { return SetFlag(optionFocus, b); }
$1ewxAuiPaneInfo& Pin(bool b = true) { return SetFlag(optionPin, b); }
$1ewxAuiPaneInfo& OptionsButton(bool b = true) { return SetFlag(buttonOptions, b); }
$1ewxAuiPaneInfo& OpenButton(bool b = true) { return SetFlag(buttonOpen, b); }
$1ewxAuiPaneInfo& TakeFocusButton(bool b = true) { return SetFlag(buttonTakeFocus, b); }
$1ewxAuiPaneInfo& AddFocusButton(bool b = true) { return SetFlag(buttonAddFocus, b); }
EOF
    print(NEWFILE $tmp);
  } elsif (/optionFloatable \|/) {
    print(NEWFILE $_);
    print(NEWFILE "            buttonOpen |\n");
  } elsif (/END_DECLARE_EVENT_TYPES/) {
    $tmp = <<EOF;
    DECLARE_EVENT_TYPE(wxEVT_EWXAUI_PANE_OPEN, 0)
    DECLARE_EVENT_TYPE(wxEVT_EWXAUI_PANE_OPTIONS, 0)
    DECLARE_EVENT_TYPE(wxEVT_EWXAUI_PANE_TAKE_FOCUS, 0)
    DECLARE_EVENT_TYPE(wxEVT_EWXAUI_PANE_ADD_FOCUS, 0)
    DECLARE_EVENT_TYPE(wxEVT_EWXAUI_UPDATE, 0)
$_
EOF
    print(NEWFILE $tmp);
  } elsif (/wx__DECLARE_EVT0\(wxEVT_EWXAUI_FIND_MANAGER/) {
    $tmp = <<EOF;
$_
#define EVT_EWXAUI_PANE_OPEN(func) \\
   wx__DECLARE_EVT0(wxEVT_EWXAUI_PANE_OPEN, ewxAuiManagerEventHandler(func))
#define EVT_EWXAUI_PANE_OPTIONS(func) \\
   wx__DECLARE_EVT0(wxEVT_EWXAUI_PANE_OPTIONS, ewxAuiManagerEventHandler(func))
#define EVT_EWXAUI_PANE_TAKE_FOCUS(func) \\
   wx__DECLARE_EVT0(wxEVT_EWXAUI_PANE_TAKE_FOCUS, ewxAuiManagerEventHandler(func))
#define EVT_EWXAUI_PANE_ADD_FOCUS(func) \\
   wx__DECLARE_EVT0(wxEVT_EWXAUI_PANE_ADD_FOCUS, ewxAuiManagerEventHandler(func))
#define EVT_EWXAUI_UPDATE(func) \\
   wx__DECLARE_EVT0(wxEVT_EWXAUI_UPDATE, ewxAuiManagerEventHandler(func))
EOF
    print(NEWFILE $tmp);
  } else {
    print(NEWFILE $_);
  }
}
close(FILE);
close(NEWFILE);
system("/bin/rm -f \'$FRAMEMANAGER_H\'");
system("/bin/mv -f auigen_tmpfile \'$FRAMEMANAGER_H\'");

print "Patching $FRAMEMANAGER_C\n";

open(FILE, $FRAMEMANAGER_C) || die "Sorry, couldn't open $FRAMEMANAGER_C\n";
open(NEWFILE, ">auigen_tmpfile") || die "Sorry, couldn't open auigen_tmpfile\n";
while (<FILE>) {
  if (/wx\/gtk\/private.h/) {
    print(NEWFILE "#include \"gtk/gtk.h\"");
  } elsif (/DEFINE_EVENT_TYPE\(wxEVT_EWXAUI_FIND_MANAGER\)/) {
    $tmp = <<EOF;
$_
DEFINE_EVENT_TYPE(wxEVT_EWXAUI_PANE_OPEN)
DEFINE_EVENT_TYPE(wxEVT_EWXAUI_PANE_OPTIONS)
DEFINE_EVENT_TYPE(wxEVT_EWXAUI_PANE_TAKE_FOCUS)
DEFINE_EVENT_TYPE(wxEVT_EWXAUI_PANE_ADD_FOCUS)
DEFINE_EVENT_TYPE(wxEVT_EWXAUI_UPDATE)
EOF
    print(NEWFILE $tmp);
  } elsif (/ewxAuiManager::SetArtProvider/) {
    $tmp = <<EOF;
void ewxAuiManager::SetArtProvider(ewxAuiDockArt* art_provider, bool take_ownership)
EOF
    print(NEWFILE $tmp);
  } elsif (/m_art = new/) {
    $tmp = <<EOF;
$_
    m_we_own_m_art = true;
EOF
    print(NEWFILE $tmp);
  } elsif (/m_art = art_provider/) {
    $tmp = <<EOF;
$_
    m_we_own_m_art = take_ownership;
EOF
    print(NEWFILE $tmp);
  } elsif (/delete m_art/) {
    $tmp = <<EOF;
    if (m_we_own_m_art) 
        delete m_art;
    else
        m_art = NULL;
EOF
    print(NEWFILE $tmp);
  } elsif (/^    ProcessEvent/) {
    $tmp = <<EOF;
    // next, if we are managing an ewxAuiFloatingFrame window, then
    // we are an embedded ewxAuiManager inside the ewxAuiFloatingFrame.
    // We want to allow the owner manager a chance to override, too
    wxWindow* managed_wnd = GetManagedWindow();
    if (managed_wnd && managed_wnd->IsKindOf(CLASSINFO(ewxAuiFloatingFrame)))
    {
        ewxAuiFloatingFrame* floating_frame = (ewxAuiFloatingFrame*)managed_wnd;
        ewxAuiManager* owner_mgr = floating_frame->GetOwnerManager();
        if (owner_mgr->GetManagedWindow())
        {
            if (owner_mgr->GetManagedWindow()->ProcessEvent(event))
                return;
        }
    }

$_
EOF
    print(NEWFILE $tmp);
  } elsif (/^        pinfo.dock_proportion = 100000;/) {
    $tmp = <<EOF;
$_

    if (pinfo.HasAddFocusButton()) {
        ewxAuiPaneButton button;
        button.button_id = ewxAUI_BUTTON_ADD_FOCUS;
        pinfo.buttons.Add(button);
    }

    if (pinfo.HasTakeFocusButton()) {
        ewxAuiPaneButton button;
        button.button_id = ewxAUI_BUTTON_TAKE_FOCUS;
        pinfo.buttons.Add(button);
    }

    if (pinfo.HasOpenButton()) {
        ewxAuiPaneButton button;
        button.button_id = ewxAUI_BUTTON_OPEN;
        pinfo.buttons.Add(button);
    }

    if (pinfo.HasOptionsButton()) {
        ewxAuiPaneButton button;
        button.button_id = ewxAUI_BUTTON_OPTIONS;
        pinfo.buttons.Add(button);
    }

EOF
    print(NEWFILE $tmp);
  } elsif (/add pane buttons to the caption/) {
    $tmp = <<EOF;
        int prebuttons[] = { ewxAUI_BUTTON_OPEN,
                             ewxAUI_BUTTON_TAKE_FOCUS,
                             ewxAUI_BUTTON_ADD_FOCUS };

        // add first buttons to the caption
        // notice the use of Insert instead of Add
        for (int i = 0, button_count = pane.buttons.GetCount();
             i < button_count; ++i)
        {
            ewxAuiPaneButton& button = pane.buttons.Item(i);

            bool isPreButton = false;
            for (int prebutton_idx = 0; prebutton_idx < 3; ++prebutton_idx) {
                if (button.button_id == prebuttons[prebutton_idx]) {
                    isPreButton = true;
                    break;
                }
            }
            if (!isPreButton) continue;

            sizer_item = caption_sizer->Insert(0, pane_button_size,
                    caption_size, 0, wxEXPAND);

            part.type = ewxAuiDockUIPart::typePaneButton;
            part.dock = &dock;
            part.pane = &pane;
            part.button = &button;
            part.orientation = orientation;
            part.cont_sizer = caption_sizer;
            part.sizer_item = sizer_item;
            uiparts.Add(part);
        }

$_
EOF
    print(NEWFILE $tmp);
  } elsif (/sizer_item = caption_sizer->Add\(pane_button_size/) {
    $tmp = <<EOF;
            bool isPreButton = false;
            for (int prebutton_idx = 0; prebutton_idx < 3; ++prebutton_idx) {
                if (button.button_id == prebuttons[prebutton_idx]) {
                    isPreButton = true;
                    break;
                }
            }
            if (isPreButton) continue;

$_
EOF
    print(NEWFILE $tmp);
  } elsif (/if \(min_size != wxDefaultSize/) {
    $tmp = <<EOF;
    if (!pane.IsOpen()) {
        min_size.y = 0;
    }

$_
EOF
    print(NEWFILE $tmp);
  } elsif (/if the dock mode is proportional/) {
    $tmp = <<EOF;
        bool realFixedValue = dock.fixed;
        dock.fixed = false;

$_
EOF
    print(NEWFILE $tmp);
  } elsif (/if the dock mode is fixed, and none of the panes/) {
    $tmp = <<EOF;
        dock.fixed = realFixedValue;
$_
EOF
    print(NEWFILE $tmp);
  } elsif (/if \(!p.IsFloating/) {
    $tmp = <<EOF;
        // HACK??
        // reset the best_size for each pane
        if (p.window)
        {
            p.best_size = p.window->GetBestSize();

            if (p.min_size != wxDefaultSize)
            {
                if (p.best_size.x < p.min_size.x)
                    p.best_size.x = p.min_size.x;
                if (p.best_size.y < p.min_size.y)
                    p.best_size.y = p.min_size.y;
            }
        }

$_
EOF
    print(NEWFILE $tmp);
  } elsif (/p.window->IsShown\(\) != p.IsShown/) {
    $tmp = <<EOF;
            if (p.window->IsShown() != (p.IsShown() && p.IsOpen()))
EOF
    print(NEWFILE $tmp);
  } elsif (/p.window->Show\(p.IsShown/) {
    $tmp = <<EOF;
            p.window->Show(p.IsShown() && p.IsOpen());
EOF
    print(NEWFILE $tmp);
  } elsif (/owner_mgr->StartPaneDrag/) {
    $tmp = <<EOF;
/* Do nothing in this case since it causes the contents of the pane to disappear
$_
EOF
    print(NEWFILE $tmp);
  } elsif (/                                                     event.m_y - part->rect.y/) {
    $tmp = <<EOF;
$_
*/
EOF
    print(NEWFILE $tmp);
  } elsif (/else if \(evt.button == ewxAUI_BUTTON_PIN/) {
    $tmp = <<EOF;
    else if (evt.button == ewxAUI_BUTTON_OPEN)
    {
        // fire pane open event
        ewxAuiManagerEvent e(wxEVT_EWXAUI_PANE_OPEN);
        e.SetManager(this);
        e.SetPane(evt.pane);
        ProcessMgrEvent(e);

        if (!e.GetVeto())
        {
            // toggle whether this pane is open
            pane.Open(!pane.IsOpen());

            // adjust whether this pane is fixed/resizable
            // based on its open status
            if (pane.IsOpen()) {
                if (pane.IsAlwaysFixed()) {
                    pane.Fixed();
                }
                else {
                    pane.Resizable();
                }
            }
            else {
                pane.Fixed();
            }

            // an open is an implicit "take focus"
            if (pane.HasTakeFocusButton() && pane.IsOpen()) {
                int i = 0, pane_count = m_panes.GetCount();
                while (i < pane_count)
                {
                    ewxAuiPaneInfo& p = m_panes.Item(i);
                    p.Focus(false);
                    p.Pin(false);
                    ++i;
                }
            }

            // now set whether our pane has focus
            pane.Focus(pane.IsOpen());
            pane.Pin(false);

            Update();
        }
    }
    else if (evt.button == ewxAUI_BUTTON_TAKE_FOCUS)
    {
        // fire pane take focus event
        ewxAuiManagerEvent e(wxEVT_EWXAUI_PANE_TAKE_FOCUS);
        e.SetManager(this);
        e.SetPane(evt.pane);
        ProcessMgrEvent(e);

        if (!e.GetVeto())
        {
            // if receiving focus, unset focus and pin on all panes
            if (!pane.HasFocus()) { // we are receiving focus
                int i = 0, pane_count = m_panes.GetCount();
                while (i < pane_count)
                {
                    ewxAuiPaneInfo& p = m_panes.Item(i);
                    p.Focus(false);
                    p.Pin(false);
                    ++i;
                }
            }

            // now set whether our pane has focus
            pane.Focus(!pane.HasFocus());
            pane.Pin(false);

            // a "take focus" is an implicit "open"
            if (pane.HasFocus() && !pane.IsOpen()) {
                pane.Open();
                if (pane.IsAlwaysFixed()) {
                    pane.Fixed();
                }
                else {
                    pane.Resizable();
                }
            }

            Update();
        }
    }
    else if (evt.button == ewxAUI_BUTTON_ADD_FOCUS)
    {
        // fire pane take focus event
        ewxAuiManagerEvent e(wxEVT_EWXAUI_PANE_ADD_FOCUS);
        e.SetManager(this);
        e.SetPane(evt.pane);
        ProcessMgrEvent(e);

        if (!e.GetVeto())
        {
            // if receiving focus, unset focus on all panes that aren't pinned
            if (!pane.HasFocus()) { // we are receiving focus
                int i = 0, pane_count = m_panes.GetCount();
                while (i < pane_count)
                {
                    ewxAuiPaneInfo& p = m_panes.Item(i);
                    if (!p.IsPinned())
                        p.Focus(false);
                    ++i;
                }
            }

            // now set whether our pane has focus
            pane.Pin(!pane.IsPinned());
            pane.Focus(pane.IsPinned());

            // an "add focus" is an implicit "open"
            if (pane.HasFocus() && !pane.IsOpen()) {
                pane.Open();
                if (pane.IsAlwaysFixed()) {
                    pane.Fixed();
                }
                else {
                    pane.Resizable();
                }
            }

            Update();
        }
    }
    else if (evt.button == ewxAUI_BUTTON_OPTIONS)
    {
        // fire pane take focus event
        ewxAuiManagerEvent e(wxEVT_EWXAUI_PANE_OPTIONS);
        e.SetManager(this);
        e.SetPane(evt.pane);
        ProcessMgrEvent(e);
    }
$_
EOF
    print(NEWFILE $tmp);
  } elsif (/^    Repaint\(\)/) {
# kindof hacky, but this is our way into the tail end of the Update function
    $tmp = <<EOF;
$_
    ewxAuiManagerEvent e(wxEVT_EWXAUI_UPDATE);
    e.SetManager(this);
    ProcessMgrEvent(e);
EOF
    print(NEWFILE $tmp);
  } else {
    print(NEWFILE $_);
  }
}
close(FILE);
close(NEWFILE);
system("/bin/rm -f \'$FRAMEMANAGER_C\'");
system("/bin/mv -f auigen_tmpfile \'$FRAMEMANAGER_C\'");

