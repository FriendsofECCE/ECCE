/**
 * @file 
 *
 *
 */
#ifndef WXCALCIMPORTCLIENT_H
#define WXCALCIMPORTCLIENT_H


#include <string>
  using std::string;

class TaskJob;

/**
 * For applications that use the WxCalcImport dialog, when the user says to
 * do it, getContainer will be called.  The client supplies the object
 * into which the data will be loaded.  When basic validation of the import
 * file has been done (code, chemical system, basis set, etc. recognized),
 * importValidationComplete will be called.  The status argument indicates
 * whether the job monitoring step has been.  If not, the client is responsible
 * for deleting the object when basic validation fails.
 */
class WxCalcImportClient 
{
  public:
    virtual TaskJob *getContainer(const string& name) = 0;
    virtual void importValidationComplete(TaskJob *, bool status,
                                          string message) = 0;
};

#endif
