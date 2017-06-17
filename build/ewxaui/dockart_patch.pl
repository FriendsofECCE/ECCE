#!/usr/bin/env perl
#
# This script will "patch" dockart.[CH] for ewxAUI.  See the README file in
# this directory for instructions to build ewxAUI.
#

$DOCKART_H = "include/wx/ewxaui/dockart.H";
$DOCKART_C = "dockart.C";

print "Patching $DOCKART_H\n";

open(FILE, $DOCKART_H) || die "Sorry, couldn't open $DOCKART_H\n";
open(NEWFILE, ">auigen_tmpfile") || die "Sorry, couldn't open auigen_tmpfile\n";
while (<FILE>) {
  if (/colour.h/) {
    print (NEWFILE $_);
    print (NEWFILE "#include <wx/ewxaui/framemanager.H>\n");
  } elsif (/wxBitmap m_active_restore_bitmap/) {
    $tmp = <<EOF;
$_
    wxBitmap m_inactive_open_in_bitmap;
    wxBitmap m_inactive_open_out_bitmap;
    wxBitmap m_inactive_take_focus_in_bitmap;
    wxBitmap m_inactive_take_focus_out_bitmap;
    wxBitmap m_inactive_add_focus_in_bitmap;
    wxBitmap m_inactive_add_focus_out_bitmap;
    wxBitmap m_inactive_options_bitmap;
    wxBitmap m_active_open_in_bitmap;
    wxBitmap m_active_open_out_bitmap;
    wxBitmap m_active_take_focus_in_bitmap;
    wxBitmap m_active_take_focus_out_bitmap;
    wxBitmap m_active_add_focus_in_bitmap;
    wxBitmap m_active_add_focus_out_bitmap;
    wxBitmap m_active_options_bitmap;
EOF
    print(NEWFILE $tmp);
  } else {
    print(NEWFILE $_);
  }
}
close(FILE);
close(NEWFILE);
system("/bin/rm -f $DOCKART_H");
system("/bin/mv -f auigen_tmpfile $DOCKART_H");

print "Patching $DOCKART_C\n";

open(FILE, $DOCKART_C) || die "Sorry, couldn't open $DOCKART_C\n";
open(NEWFILE, ">auigen_tmpfile") || die "Sorry, couldn't open auigen_tmpfile\n";
while (<FILE>) {
  if (/clip_rect.width -= 2;/) {
    $tmp = <<EOF;
$_
    int rectMult = 0;
    int i, button_count;
    for (i = 0, button_count = pane.buttons.GetCount(); i < button_count; ++i) {
        ewxAuiPaneButton& button = pane.buttons.Item(i);
        int button_id = button.button_id;
        if (button_id == ewxAUI_BUTTON_OPEN
                || button_id == ewxAUI_BUTTON_TAKE_FOCUS
                || button_id == ewxAUI_BUTTON_ADD_FOCUS)
        {
            rectMult++;
            clip_rect.x += m_button_size;
        } else {
            clip_rect.width -= m_button_size;
        }
    }
EOF
    print(NEWFILE $tmp);
  } elsif (/pane.Has.*Button/) {
    #skip
  } elsif (/clip_rect.width -= m_button_size/) {
    #skip
  } elsif (/DrawText/) {
    print(NEWFILE "    dc.DrawText(draw_text, rect.x+3+rectMult*m_button_size, rect.y+(rect.height/2)-(h/2)-1);\n");
  } elsif (/case ewxAUI_BUTTON_PIN/) {
    $tmp = <<EOF;
        case ewxAUI_BUTTON_OPEN:
            if (pane.state & ewxAuiPaneInfo::optionOpen)
                if (pane.state & ewxAuiPaneInfo::optionActive)
                    bmp = m_active_open_in_bitmap;
                else
                    bmp = m_inactive_open_in_bitmap;
            else
                if (pane.state & ewxAuiPaneInfo::optionActive)
                    bmp = m_active_open_out_bitmap;
                else
                    bmp = m_inactive_open_out_bitmap;
            break;
        case ewxAUI_BUTTON_TAKE_FOCUS:
            if (pane.state & ewxAuiPaneInfo::optionFocus)
                if (pane.state & ewxAuiPaneInfo::optionActive)
                    bmp = m_active_take_focus_in_bitmap;
                else
                    bmp = m_inactive_take_focus_in_bitmap;
            else 
                if (pane.state & ewxAuiPaneInfo::optionActive)
                    bmp = m_active_take_focus_out_bitmap;
                else
                    bmp = m_inactive_take_focus_out_bitmap;
            break;
        case ewxAUI_BUTTON_ADD_FOCUS:
            if (pane.state & ewxAuiPaneInfo::optionPin)
                if (pane.state & ewxAuiPaneInfo::optionActive)
                    bmp = m_active_add_focus_in_bitmap;
                else
                    bmp = m_inactive_add_focus_in_bitmap;
            else
                if (pane.state & ewxAuiPaneInfo::optionActive)
                    bmp = m_active_add_focus_out_bitmap;
                else
                    bmp = m_inactive_add_focus_out_bitmap;
            break;
        case ewxAUI_BUTTON_OPTIONS:
            if (pane.state & ewxAuiPaneInfo::optionActive)
                bmp = m_active_options_bitmap;
            else
                bmp = m_inactive_options_bitmap;
            break;
$_
EOF
    print(NEWFILE $tmp);
  } elsif (/m_gradient_type = ewxAUI_GRADIENT_VERTICAL/) {
    $tmp = <<EOF;
$_
    m_inactive_open_in_bitmap = ewxAuiBitmapFromBits(pin_bits, 16, 16, m_active_caption_text_colour);
    m_inactive_open_out_bitmap = ewxAuiBitmapFromBits(pin_bits, 16, 16, m_active_caption_text_colour);
    m_inactive_take_focus_in_bitmap = ewxAuiBitmapFromBits(pin_bits, 16, 16, m_active_caption_text_colour);
    m_inactive_take_focus_out_bitmap = ewxAuiBitmapFromBits(pin_bits, 16, 16, m_active_caption_text_colour);
    m_inactive_add_focus_in_bitmap = ewxAuiBitmapFromBits(pin_bits, 16, 16, m_active_caption_text_colour);
    m_inactive_add_focus_out_bitmap = ewxAuiBitmapFromBits(pin_bits, 16, 16, m_active_caption_text_colour);
    m_inactive_options_bitmap = ewxAuiBitmapFromBits(pin_bits, 16, 16, m_active_caption_text_colour);
    m_active_open_in_bitmap = ewxAuiBitmapFromBits(pin_bits, 16, 16, m_active_caption_text_colour);
    m_active_open_out_bitmap = ewxAuiBitmapFromBits(pin_bits, 16, 16, m_active_caption_text_colour);
    m_active_take_focus_in_bitmap = ewxAuiBitmapFromBits(pin_bits, 16, 16, m_active_caption_text_colour);
    m_active_take_focus_out_bitmap = ewxAuiBitmapFromBits(pin_bits, 16, 16, m_active_caption_text_colour);
    m_active_add_focus_in_bitmap = ewxAuiBitmapFromBits(pin_bits, 16, 16, m_active_caption_text_colour);
    m_active_add_focus_out_bitmap = ewxAuiBitmapFromBits(pin_bits, 16, 16, m_active_caption_text_colour);
    m_active_options_bitmap = ewxAuiBitmapFromBits(pin_bits, 16, 16, m_active_caption_text_colour);
EOF
    print(NEWFILE $tmp);
  } else {
    print(NEWFILE $_);
  }
}
close(FILE);
close(NEWFILE);
system("/bin/rm -f $DOCKART_C");
system("/bin/mv -f auigen_tmpfile $DOCKART_C");

