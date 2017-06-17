#!/usr/bin/env perl
#
# This script will "patch" floatpane.C for ewxAUI.  See the README file in
# this directory for instructions to build ewxAUI.
#

$FLOATPANE_C = "floatpane.C";

print "Patching $FLOATPANE_C\n";

open(FILE, $FLOATPANE_C) || die "Sorry, couldn't open $FLOATPANE_C\n";
open(NEWFILE, ">auigen_tmpfile") || die "Sorry, couldn't open auigen_tmpfile\n";
while (<FILE>) {
  if (/pane.IsFixed\(\)\?0/) {
    print(NEWFILE "(pane.IsAlwaysFixed()?0:wxRESIZE_BORDER)\n");
  } elsif (/CaptionVisible/) {
    $tmp = <<EOF;
            CaptionVisible(true).
            CloseButton(false).
            MaximizeButton(false).
            Open().OpenButton(false).
            AlwaysFixed(false).Resizable().
EOF
    print(NEWFILE $tmp);
  } elsif (/SetManagedWindow/) {
    $tmp = <<EOF;
$_
    m_mgr.SetArtProvider(m_owner_mgr->GetArtProvider(), false);
EOF
    print(NEWFILE $tmp);
  } elsif (/Layer/) {
    $tmp = <<EOF;
$_
    contained_pane.buttons.Clear();
EOF
    print(NEWFILE $tmp);
  } elsif (/wxSize pane_min_size/) {
    $tmp = <<EOF;
    wxSize pane_min_size = pane.window->GetMinSize();
    int caption_size = \\
            m_owner_mgr->m_art->GetMetric(wxAUI_DOCKART_CAPTION_SIZE);
    pane_min_size.y += caption_size;
EOF
    print(NEWFILE $tmp);
  } elsif (/SetMinSize/) {
    $tmp = <<EOF;
    SetMinSize(pane_min_size);
    SetSize(pane_min_size);
EOF
    print(NEWFILE $tmp);
  } elsif (/wxSize size = pane.best_size/) {
    $tmp = <<EOF;
$_
        if (pane.IsShown() && !pane.IsOpen())
            size = wxSize(0,0);
EOF
    print(NEWFILE $tmp);
  } elsif (/SetClientSize/) {
    $tmp = <<EOF;
        if (pane.HasCaption())
        {
            size.y += m_owner_mgr->m_art->GetMetric(wxAUI_DOCKART_CAPTION_SIZE);
        }
        int smallestAllowedSize = 20;
        if (size.x < smallestAllowedSize) {
          size.x = smallestAllowedSize;
        }
        if (size.y < smallestAllowedSize) {
          size.y = smallestAllowedSize;
        }

$_
EOF
    print(NEWFILE $tmp);
  } elsif (/if \(pane.IsFixed/) {
    print(NEWFILE "    if (pane.IsAlwaysFixed())\n");
  } else {
    print(NEWFILE $_);
  }
}
close(FILE);
close(NEWFILE);
system("/bin/rm -f \'$FLOATPANE_C\'");
system("/bin/mv -f auigen_tmpfile \'$FLOATPANE_C\'");

