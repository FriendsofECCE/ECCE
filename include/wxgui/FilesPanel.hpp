#ifndef _FILESPANEL_H_
#define _FILESPANEL_H_ 
/*!
 * Includes
 */

#include "wxgui/FilesPanelGUI.H"

class FilesModel;
class MDPanelHelper;

/**
 *  GUI implementation of FilesPanel in MD editors
 */

class FilesPanel: public FilesPanelGUI
{    

public:
    /// Constructors
    FilesPanel(MDPanelHelper *helper);
    FilesPanel(MDPanelHelper *helper, wxWindow* parent,
	           wxWindowID id = SYMBOL_FILESPANELGUI_IDNAME,
		   const wxPoint& pos = SYMBOL_FILESPANELGUI_POSITION,
		   const wxSize& size = SYMBOL_FILESPANELGUI_SIZE,
		   long style = SYMBOL_FILESPANELGUI_STYLE );

    virtual ~FilesPanel();

    void setFilesModel(FilesModel *model);
    FilesModel* getFilesModel() const;
    void initializeGUI();
    void refreshGUI();
    void showFilesPanel();
    void configEnergyCalc();
    void disableBatchAndKeepOptions();
    void constrainSteps(const int& steps);
    void constrainFreqs(const int& steps);

    /**
     * Files panel event handlers
     */

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_RSTFRQ
    virtual void OnTextctrlFileRstfrqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_KEEP
    virtual void OnCheckboxFileKeepClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_FILE_TRJSTR
    virtual void OnChoiceFileTrjstrSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_USEBTC
    virtual void OnCheckboxFileUsebtcClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_BTCFRQ
    virtual void OnTextctrlFileBtcszEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLTCRD
    virtual void OnCheckboxFileSltcrdClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_CSVFRQ
    virtual void OnTextctrlFileCsufrqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLVCRD
    virtual void OnCheckboxFileSlvcrdClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_CSVFRQ
    virtual void OnTextctrlFileCsvfrqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLTVEL
    virtual void OnCheckboxFileSltvelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_VSUFRQ
    virtual void OnTextctrlFileVsufrqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLVVEL
    virtual void OnCheckboxFileSlvvelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_VSVFRQ
    virtual void OnTextctrlFileVsvfrqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLTFRC
    virtual void OnCheckboxFileSltfrcClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_FSUFRQ
    virtual void OnTextctrlFileFsufrqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLVFRC
    virtual void OnCheckboxFileSlvfrcClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_FSVFRQ
    virtual void OnTextctrlFileFsvfrqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_USEAVG
    virtual void OnCheckboxFileUseavgClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_PRPFRQ
    virtual void OnTextctrlFilePrpfrqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_PRTMD
    virtual void OnCheckboxFilePrtmdClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_MDFREQ
    virtual void OnTextctrlFileMdfreqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_ESOL
    virtual void OnCheckboxFileEsolClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_EBOND
    virtual void OnCheckboxFileEbondClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_STAT
    virtual void OnCheckboxFileStatClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_STFREQ
    virtual void OnTextctrlFileStfreqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SUBOND
    virtual void OnCheckboxFileSubondClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_SEFREQ
    virtual void OnTextctrlFileSefreqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_FRC
    virtual void OnCheckboxFileFrcClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_FFREQ
    virtual void OnTextctrlFileSffreqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_DATA
    virtual void OnCheckboxFilePmfClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_PMFFREQ
    virtual void OnTextctrlFilePmffreqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_TOP
    virtual void OnCheckboxFileTopClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_NBOND
    virtual void OnCheckboxFileNbondClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLVPAR
    virtual void OnCheckboxFileSlvparClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLUPAR
    virtual void OnCheckboxFileSluparClick( wxCommandEvent& event );


  protected:
    FilesModel *p_model;
    MDPanelHelper *p_helper;
    int p_dataSteps;
};
#endif // _FILESPANEL_H_
