
#ifndef _WXMEASUREPROMPT_H
#define _WXMEASUREPROMPT_H

#include "wx/defs.h"


#include "wx/dialog.h"

#if wxUSE_VALIDATORS
#include "wx/valtext.h"
#endif

#include "wxgui/ewxStyledWindow.H"

class ewxCheckBox;
class ewxTextCtrl;

#define WxMeasurePromptStyle (wxOK | wxCANCEL | wxCENTRE | wxWS_EX_VALIDATE_RECURSIVELY)


/**
 * A dialog to prompt for a measure value to change a distance, angle, torsion.
 * The user supplies a value and decides which atoms will be affected
 * by the operation.
 */
class WxMeasurePrompt : public wxDialog, public ewxStyledWindow
{
   public:
      enum MeasureType { DISTANCE, ANGLE, TORSION };

      WxMeasurePrompt(wxWindow *parent,
            MeasureType type,
            double value,
            long style = WxMeasurePromptStyle,
            const wxPoint& pos = wxDefaultPosition);

      void SetValue(double val);
      double GetValue() const;

      bool isToggleSet() const;
      void setToggle(bool value);

   protected:
      void SetValidator( const wxValidator& validator );
      wxValidator* GetValidator();
      void OnOK(wxCommandEvent& event);

      ewxTextCtrl *m_textctrl;
      ewxCheckBox *p_tgl;
      double      m_value;
      long        m_dialogStyle;

   private:
      DECLARE_EVENT_TABLE()
         DECLARE_DYNAMIC_CLASS(WxMeasurePrompt)
         DECLARE_NO_COPY_CLASS(WxMeasurePrompt)
};



#endif
