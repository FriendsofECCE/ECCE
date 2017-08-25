/**
 * @file
 *
 *
 */
#ifndef WXCALCIMPORT_H_
#define WXCALCIMPORT_H_

#include "wxgui/ewxFileDialog.H"

class WxCalcImportClient;


/**
 * This class provides an interface for the user to import a 
 * calculation.  The user picks a calculation output file and the
 * code is determined automatially.
 */
class WxCalcImport : public ewxFileDialog
{

   public:
      WxCalcImport(wxWindow* parent);

      virtual ~WxCalcImport();

      void registerListener(WxCalcImportClient *l);
      void importCalc();

   protected:

      /** The client that is interested in import operations */
      WxCalcImportClient *p_listener;

};

#endif
