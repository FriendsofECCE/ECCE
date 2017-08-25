#ifndef EXPORTTABLEDIALOG_H
#define EXPORTTABLEDIALOG_H

#include "wxgui/ewxGenericFileDialog.H"

class ewxGrid;


class ExportTableDialog : public ewxGenericFileDialog
{
  DECLARE_CLASS(ExportTableDialog)

  public:
    ExportTableDialog();
    ExportTableDialog(wxWindow * parent, ewxGrid * grid);
    virtual ~ExportTableDialog();

    bool Create(wxWindow * parent, ewxGrid * grid);

    virtual void EndModal(int retCode);

  protected:
    bool writeCsv();

    void notifyCreate(const string& url) const;

  private:
    ewxGrid * p_grid;
};

#endif // EXPORTTABLEDIALOG_H
