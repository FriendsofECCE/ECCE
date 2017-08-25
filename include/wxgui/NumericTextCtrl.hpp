#ifndef NUMERICTEXTCONTROL_H_
#define NUMERICTEXTCONTROL_H_

#include <wx/textctrl.h>

#include "wxgui/ewxPanel.H"

class ewxNonBoldLabel;
class ewxTextCtrl;

/**
 * A panel that combines a text control and two static labels.
 *
 * The labels are meant to display the units and the range of the contorl.
 */
class NumericTextCtrl : public ewxPanel
{
  DECLARE_DYNAMIC_CLASS(NumericTextCtrl)

  public:
    NumericTextCtrl();
    NumericTextCtrl(wxWindow* parent,
            wxWindowID id=wxID_ANY,
            const wxString& value = "", 
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = 0,
            const wxValidator& validator = ewxDefaultValidator,
            const wxString& name = "NumericTextCtrl");
    ~NumericTextCtrl();
    bool Create(wxWindow* parent,
            wxWindowID id=wxID_ANY,
            const wxString& value = "",
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = 0,
            const wxValidator& validator = ewxDefaultValidator,
            const wxString& name = "NumericTextCtrl");

    // wxTextCtrl forwards
    void AppendText(const wxString& text) { return p_text->AppendText(text); }
    virtual bool CanCopy() { return p_text->CanCopy(); }
    virtual bool CanCut() { return p_text->CanCut(); }
    virtual bool CanPaste() { return p_text->CanPaste(); }
    virtual bool CanRedo() { return p_text->CanRedo(); }
    virtual bool CanUndo() { return p_text->CanUndo(); }
    virtual void Clear() { return p_text->Clear(); }
    virtual void Copy() { return p_text->Copy(); }
    virtual void Cut() { return p_text->Cut(); }
    void DiscardEdits() { return p_text->DiscardEdits(); }
    bool EmulateKeyPress(const wxKeyEvent& event) { return p_text->EmulateKeyPress(event); }
    const wxTextAttr& GetDefaultStyle() const { return p_text->GetDefaultStyle(); }
    virtual long GetInsertionPoint() const { return p_text->GetInsertionPoint(); }
    virtual wxTextPos GetLastPosition() const { return p_text->GetLastPosition(); }
    int GetLineLength(long lineNo) const { return p_text->GetLineLength(lineNo); }
    wxString GetLineText(long lineNo) const { return p_text->GetLineText(lineNo); }
    int GetNumberOfLines() const { return p_text->GetNumberOfLines(); }
    virtual wxString GetRange(long from, long to) const { return p_text->GetRange(from,to); }
    virtual void GetSelection(long* from, long* to) const { return p_text->GetSelection(from,to); }
    virtual wxString GetStringSelection() { return p_text->GetStringSelection(); }
    bool GetStyle(long position, wxTextAttr& style) { return p_text->GetStyle(position, style); }
    wxString GetValue() const { return p_text->GetValue(); }
    wxTextCtrlHitTestResult HitTest(const wxPoint& pt, wxTextCoord* col, wxTextCoord* row) const { return p_text->HitTest(pt,col,row); }
    bool IsEditable() const { return p_text->IsEditable(); }
    bool IsEmpty() const { return p_text->IsEmpty(); }
    bool IsModified() const { return p_text->IsModified(); }
    bool IsMultiLine() const { return p_text->IsMultiLine(); }
    bool IsSingleLine() const { return p_text->IsSingleLine(); }
    bool LoadFile(const wxString& filename, int fileType=wxTEXT_TYPE_ANY) { return p_text->LoadFile(filename,fileType); }
    void MarkDirty() { return p_text->MarkDirty(); }
    void OnDropFiles(wxDropFilesEvent& event) { return p_text->OnDropFiles(event); }
    virtual void Paste() { return p_text->Paste(); }
    bool PositionToXY(long pos, long* x, long* y) const { return p_text->PositionToXY(pos,x,y); }
    virtual void Redo() { return p_text->Redo(); }
    virtual void Remove(long from, long to) { return p_text->Remove(from,to); }
    virtual void Replace(long from, long to, const wxString& value) { return p_text->Replace(from,to,value); }
    bool SaveFile(const wxString& filename, int fileType=wxTEXT_TYPE_ANY) { return p_text->SaveFile(filename,fileType); }
    bool SetDefaultStyle(const wxTextAttr& style) { return p_text->SetDefaultStyle(style); }
    virtual void SetEditable(const bool& editable) { return p_text->SetEditable(editable); }
    virtual void SetInsertionPoint(long pos) { return p_text->SetInsertionPoint(pos); }
    virtual void SetInsertionPointEnd() { return p_text->SetInsertionPointEnd(); }
    virtual void SetMaxLength(unsigned long len) { return p_text->SetMaxLength(len); }
    void SetModified(bool modified) { return p_text->SetModified(modified); }
    virtual void SetSelection(long from, long to) { return p_text->SetSelection(from,to); }
    bool SetStyle(long start, long end, const wxTextAttr& style) { return p_text->SetStyle(start,end,style); }
    virtual void SetValue(const wxString& value) { return p_text->SetValue(value); }
    virtual void ChangeValue(const wxString& value) { return p_text->ChangeValue(value); }
    void ShowPosition(long pos) { return p_text->ShowPosition(pos); }
    virtual void Undo() { return p_text->Undo(); }
    void WriteText(const wxString& text) { return p_text->WriteText(text); }
    long XYToPosition(long x, long y) { return p_text->XYToPosition(x,y); }
    wxTextCtrl& operator <<(const wxString& s) { return (*p_text) << s; }

    // wxWindow forwards
    virtual void SetValidator(const wxValidator& validator);

    ewxTextCtrl     * GetText() const { return p_text; }
    ewxNonBoldLabel * GetUnits() const { return p_units; }
    ewxNonBoldLabel * GetRange() const { return p_range; }

  protected:
    ewxTextCtrl     * p_text;
    ewxNonBoldLabel * p_units;
    ewxNonBoldLabel * p_range;
};

#endif // NUMERICTEXTCONTROL_H_
