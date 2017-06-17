#ifndef EWXIMAGE_H_
#define EWXIMAGE_H_

#include <map>
  using std::map;
#include <string>
  using std::string;

#include <wx/gdicmn.h>
#include <wx/image.h>

class ewxImage
{
  public:
    static wxImage& get(const string& name, const int& widthAndHeight);
    static wxImage& get(const string& name, const wxSize& size=wxDefaultSize);

    static const string ANIMATION_NEXT;
    static const string ANIMATION_PREVIOUS;
    static const string ANIMATION_START;
    static const string ANIMATION_STOP;
    static const string ARROW_DOWN;
    static const string ARROW_RIGHT;
    static const string CLOSE;
    static const string FLOAT;
    static const string MAXIMIZE;
    static const string MINUS;
    static const string MENU;
    static const string PLUS;
    static const string PLAYER_END;
    static const string PLAYER_FWD;
    static const string PLAYER_PAUSE;
    static const string PLAYER_PLAY;
    static const string PLAYER_RECORD;
    static const string PLAYER_REW;
    static const string PLAYER_START;
    static const string PLAYER_STOP;
    static const string REFRESH;
    static const string RESTORE;
    static const string TEAR;
    static const string VIZ_FOCUS_IN;
    static const string VIZ_FOCUS_OUT;
    static const string VIZ_PIN_IN;
    static const string VIZ_PIN_OUT;

  protected:
    ewxImage();
    ewxImage(const ewxImage&);
    ewxImage& operator = (const ewxImage&);

    void parseImagesText();

    static ewxImage* Instance();
    static wxString fullPath(const string& name);

    string getFileName(const string& name, const wxSize& size=wxDefaultSize);

    static const string MISSING_IMAGE;

    map<string, map<wxSize, wxImage> > p_images;
    map<string, map<wxSize, string> >  p_names;
};

#endif // EWXIMAGE_H_
