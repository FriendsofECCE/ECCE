#ifndef SLIDERCOMBO_H_
#define SLIDERCOMBO_H_

#include <wx/combo.h>
#include <wx/textctrl.h>

#include "wxgui/ewxSlider.H"

class SliderCombo;

class SliderComboPopup : public ewxSlider,
                         public wxComboPopup
{
  public:
    SliderComboPopup();
    virtual ~SliderComboPopup();

    // the wxComboPopup required API
    virtual void Init();
    virtual bool Create(wxWindow *parent);
    virtual wxWindow* GetControl();
    virtual void SetStringValue(const wxString& value);
    virtual wxString GetStringValue() const;
    virtual void OnComboKeyEvent(wxKeyEvent &event);

  private:
    bool p_isInt;
    friend class SliderCombo;  // to access p_isInt for text entry
};



class SliderCombo : public wxComboCtrl
{
  public:
    SliderCombo();
    SliderCombo(wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxString& value="",
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTE_PROCESS_ENTER,
            const wxValidator& validator=ewxDefaultValidator,
            const wxString& name="SliderCombo");
    ~SliderCombo();

    bool Create(wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxString& value="",
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTE_PROCESS_ENTER,
            const wxValidator& validator=ewxDefaultValidator,
            const wxString& name="SliderCombo");

    void SetValue(int value);
    void SetValue(double value);
    void SetRange(int min, int max);
    void SetRange(double min, double max);

    int GetIntValue() const;
    float GetFloatValue() const;

  protected:
    void OnScroll(wxScrollEvent &event);
    void OnValidate(wxCommandEvent &event);

  private:
    /* This is the slider which also implements PopupCtrl */
    SliderComboPopup *p_slider;

    DECLARE_EVENT_TABLE()
};

#endif // SLIDERCOMBO_H_
