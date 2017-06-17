#ifndef EWXIMAGELIST_H_
#define EWXIMAGELIST_H_

#include <string>
  using std::string;
#include <vector>
  using std::vector;

#include "wx/imaglist.h"

class ewxDialog;
class wxWindow;

class ewxImageList : public wxImageList
{
  public:
    ewxImageList();
    ewxImageList(int width, int height,
                 const bool mask=true, int initialCount=1);
    bool Create(int width, int height,
                const bool mask=true, int initialCount=1);
    ~ewxImageList();

    static bool readPixmap(const string& file, string& decLine, 
                           int& width, int& height, int& colors,
                           vector<string>& pixDefLines,
                           vector<string>& pixDataLines);

    ewxDialog* debug(wxWindow *parent) const;
};

#endif // EWXIMAGELIST_H_
