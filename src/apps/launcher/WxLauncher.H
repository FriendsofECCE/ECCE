/**
 * @file
 *
 * The Launcher window, or its tab in the Organizer (#983): a frame around a
 * LaunchPanel, which holds the controls and the launch logic.
 */
#ifndef WXLAUNCHER_HH
#define WXLAUNCHER_HH

#include "wxgui/ewxFrame.H"
#include "wxgui/WxJMSSubscriber.H"
#include "util/Preferences.H"

#include "LaunchPanel.H"

class WxLauncher : public ewxFrame, public LaunchHost
{
    DECLARE_CLASS(WxLauncher)

    public:
        WxLauncher(wxWindow* parent);
        ~WxLauncher();

        LaunchPanel *panel() const { return p_panel; }

        //  The handlers the window owns and deletes (also in the Organizer's process).
        void setMessages(WxJMSSubscriber *msgs) { p_msgs = msgs; }
        void loadSettings(const string& fname);

        //  As a tab of the Organizer's window (#983, ToolTabs.H) the frame is
        //  hidden and its panel is in the tab's page.
        bool Show(bool show = true) override;
        void Raise() override;
        void SetTitle(const wxString& title) override;
        bool Destroy() override;
        void Iconize(bool iconize = true) override;
        bool Layout() override;
        wxMenuBar *GetMenuBar() const;
        wxWindow *FindWindow(long id) const;

        void closeRequest() override { Close(); }
        void deferMessages(bool on) override;
        void relayout() override;
        void launchAllowed(bool launch, bool finish) override;

    private:
        void closeCB(wxCloseEvent& event);
        void saveSettings();

        LaunchPanel      *p_panel;
        WxJMSSubscriber  *p_msgs = NULL;
        Preferences      *p_toolPrefs = NULL;
};

#endif
