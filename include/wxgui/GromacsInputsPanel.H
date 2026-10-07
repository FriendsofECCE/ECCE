/**
 * @file
 */
#ifndef _GROMACSINPUTSPANEL_H_
#define _GROMACSINPUTSPANEL_H_

#include <string>
#include <vector>
using std::string;
using std::vector;

#include "wxgui/ewxPanel.H"

class MDEdBase;
class MdTask;
class ewxButton;
class ewxListBox;
class ewxStaticText;

/**
 * The "Inputs" page of a GROMACS task editor: the topology, the starting
 * structure and the include files grompp reads.  A GROMACS study has no
 * Prepare task, so these are brought in by the user and attached here.
 *
 * Files are stored in the task's input collection as they are attached; they
 * are not part of the model and Save has nothing to do with them.  A task
 * that has none of its own uses those of the nearest earlier task in the
 * study that does, which is also what Launch does.
 */
class GromacsInputsPanel : public ewxPanel
{
   public:
      GromacsInputsPanel(MDEdBase *owner, wxWindow *parent);
      virtual ~GromacsInputsPanel();

      /** Show the files of the task at this url. */
      void setContext(const string& url);

      /** False once the task is submitted: files can no longer change. */
      void setEditable(bool editable);

      /**
       * True when a topology and a starting structure are found for the
       * task.  If not, why is what is missing, in words for the user.
       */
      bool inputsReady(string& why);

      /** Reread the task's files and redisplay them. */
      void refresh();

      enum Kind { TOPOLOGY_FILE, STRUCTURE_FILE, INCLUDE_FILE };

      /**
       * Attach the file at path as the given kind, as the Attach buttons
       * do once a file is chosen.  Returns false with the reason in
       * message (the Attach buttons show it in a dialog).
       */
      bool attachFile(Kind kind, const string& path, string& message);

      /** What the page shows, in words: for the test hook. */
      string summary();

   protected:
      void OnAttachTopology(wxCommandEvent& event);
      void OnAttachStructure(wxCommandEvent& event);
      void OnAttachInclude(wxCommandEvent& event);
      void OnRemoveTopology(wxCommandEvent& event);
      void OnRemoveStructure(wxCommandEvent& event);
      void OnRemoveInclude(wxCommandEvent& event);
      void OnIncludeSelected(wxCommandEvent& event);

   private:
      MdTask *task() const;
      bool describe(int fileType, string& text, bool& own) const;
      bool pickFile(const wxString& title, const wxString& wildcard,
                    string& path);
      bool attach(int fileType, const string& storedName,
                  const string& contents);
      void removeOwn(int fileType);
      void report(const string& message, bool error);

      MDEdBase      *p_owner;
      string         p_url;
      bool           p_editable;
      ewxStaticText *p_topology;
      ewxStaticText *p_structure;
      ewxListBox    *p_includes;
      ewxStaticText *p_status;
      ewxButton     *p_attachTop;
      ewxButton     *p_removeTop;
      ewxButton     *p_attachGro;
      ewxButton     *p_removeGro;
      ewxButton     *p_attachInc;
      ewxButton     *p_removeInc;
};

#endif
