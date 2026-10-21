// The wx view of a details::Model: builds the widgets, forwards user edits
// to the model and mirrors the model's state back.  Also the UDP transport
// to the Calculation Editor.
#ifndef ECCE_DETAILS_PANEL_H
#define ECCE_DETAILS_PANEL_H

#include "DetailsModel.h"

#include <wx/wx.h>

class wxDatagramSocket;
class wxIPV4address;

namespace details {

// UDP datagrams to 127.0.0.1:port, as the Python dialogs send them.
class UdpTransport : public Transport {
public:
  explicit UdpTransport(long port);
  ~UdpTransport();
  void send(const std::string& datagram) override;
private:
  wxDatagramSocket *sock_;
  wxIPV4address *peer_;
};

class DetailsPanel : public wxPanel, public View {
public:
  DetailsPanel(wxFrame *frame, Model& model);
  void itemUpdated(int index) override;
  void status(const std::string& text, Severity s) override;
  void resetEnabled(bool on) override;
  void layoutChanged() override;
  void closeRequested() override;

private:
  struct W {
    wxWindow *w = 0, *w2 = 0;
    wxStaticText *lab = 0, *unitLab = 0;
  };
  Model& model_;
  wxFrame *frame_;
  std::vector<W> w_;
  wxPanel *footer_ = 0;
  wxTextCtrl *msg_ = 0;
  wxButton *resetBtn_ = 0;
  wxTimer *colorTimer_ = 0, *messageTimer_ = 0;
  std::string helpURL_;

  void build();
  void buildFooter();
  void edited(int index);
  void onReset();
};

wxFont themeFont();

}  // namespace details
#endif
