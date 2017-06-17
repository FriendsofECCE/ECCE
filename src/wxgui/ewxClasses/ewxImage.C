#include <algorithm>
  using std::find;
#include <fstream>
  using std::ifstream;
#include <iostream>
  using std::cerr;
  using std::cout;
  using std::endl;
#include <stdlib.h>

#include "util/BadValueException.H"
#include "util/Ecce.H"
#include "util/KeyValueReader.H"
#include "util/SFile.H"
#include "util/STLUtil.H"
#include "util/StringTokenizer.H"

#include "wxgui/ewxImage.H"


bool operator < (const wxSize& first, const wxSize& second)
{
  if ((first.x == second.x) && (first.y < second.y)) {
    return true;
  } else if (first.x < second.x) {
    return true;
  }
  return false;
}


/*
ostream& operator << (ostream &os, const wxSize& size)
{
  os << "(" << size.x << "," << size.y << ")";
  return os;
}
*/


// make sure image keys are all in caps
const string ewxImage::ANIMATION_NEXT("ANIMATION_NEXT");
const string ewxImage::ANIMATION_PREVIOUS("ANIMATION_PREVIOUS");
const string ewxImage::ANIMATION_START("ANIMATION_START");
const string ewxImage::ANIMATION_STOP("ANIMATION_STOP");
const string ewxImage::ARROW_DOWN("ARROW_DOWN");
const string ewxImage::ARROW_RIGHT("ARROW_RIGHT");
const string ewxImage::CLOSE("CLOSE");
const string ewxImage::FLOAT("FLOAT");
const string ewxImage::MAXIMIZE("MAXIMIZE");
const string ewxImage::MINUS("MINUS");
const string ewxImage::MENU("MENU");
const string ewxImage::PLUS("PLUS");
const string ewxImage::PLAYER_END("PLAYER_END");
const string ewxImage::PLAYER_FWD("PLAYER_FWD");
const string ewxImage::PLAYER_PAUSE("PLAYER_PAUSE");
const string ewxImage::PLAYER_PLAY("PLAYER_PLAY");
const string ewxImage::PLAYER_RECORD("PLAYER_RECORD");
const string ewxImage::PLAYER_REW("PLAYER_REW");
const string ewxImage::PLAYER_START("PLAYER_START");
const string ewxImage::PLAYER_STOP("PLAYER_STOP");
const string ewxImage::REFRESH("REFRESH");
const string ewxImage::RESTORE("RESTORE");
const string ewxImage::TEAR("TEAR");
const string ewxImage::VIZ_FOCUS_IN("VIZ_FOCUS_IN");
const string ewxImage::VIZ_FOCUS_OUT("VIZ_FOCUS_OUT");
const string ewxImage::VIZ_PIN_IN("VIZ_PIN_IN");
const string ewxImage::VIZ_PIN_OUT("VIZ_PIN_OUT");

const string ewxImage::MISSING_IMAGE("delete.xpm");


ewxImage::ewxImage()
{
  //::wxInitAllImageHandlers();

  parseImagesText();

  p_names[MISSING_IMAGE][wxDefaultSize] = MISSING_IMAGE;
}


ewxImage::ewxImage(const ewxImage&)
{
}


ewxImage& ewxImage::operator = (const ewxImage&)
{
  return *this;
}


void ewxImage::parseImagesText()
{
  SFile file;
  file = string(Ecce::ecceDataPrefPath()) + "/images.txt";

  string line;

  ifstream ifs(file.path().c_str());
  KeyValueReader kvr(ifs);
  string key,value;
  while (kvr.getpair(key,value)) {
    int width=-1,height=-1;
    StringTokenizer tokenizer(key);
    vector<string> tokens = tokenizer.tokenize(".");
    if (tokens.size() < 1 || tokens.size() > 3) {
      cerr << "Warning: Image key " << key
           << " invalid syntax, skipping" << endl;
      continue;
    }
    key = tokens[0];
    STLUtil::toUpper(key);
    /*
    if (find(KEYS, KEYS+KEYS_SIZE, key) == KEYS+KEYS_SIZE) {
      cerr << "Warning: Image key " << key
           << " not known, skipping" << endl;
      continue;
    }
    */
    if (tokens.size() == 2) {
      width = height = strtol(tokens[1].c_str(), NULL, 0);
    }
    if (tokens.size() == 3) {
      width  = strtol(tokens[1].c_str(), NULL, 0);
      height = strtol(tokens[2].c_str(), NULL, 0);
    }
    wxSize size(width,height);
    if (size != wxDefaultSize && !size.IsFullySpecified()) {
      cerr << "Warning: Image key " << key
           << " specifies a negative size, skipping" << endl;
      continue;
    }
    p_names[key][size] = value;
  }
  ifs.close();
}


/**
 * Optimized for single-threaded applications.
 * Multi-threaded applications will need to use operator new instead.
 */
ewxImage* ewxImage::Instance()
{
  static ewxImage instance;
  return &instance;
}


wxString ewxImage::fullPath(const string& name) 
{
  wxString ret = Ecce::ecceDataPath();
  ret.append("/client/pixmaps/");
  ret.append(name);
  return ret;
}


wxImage& ewxImage::get(const string& name, const int& widthAndHeight)
{
  return ewxImage::get(name, wxSize(widthAndHeight,widthAndHeight));
}


wxImage& ewxImage::get(const string& name, const wxSize& size)
{
  ewxImage *self = ewxImage::Instance();

  if (self->p_names.find(name) == self->p_names.end()) {
    cerr << "Warning: image key " << name << " not known" << endl;
    return self->get(MISSING_IMAGE, size);
  }

  if (!(size == wxDefaultSize || size.IsFullySpecified())) {
    throw BadValueException("", __FILE__, __LINE__);
  }

  map<wxSize,wxImage> &cache = self->p_images[name];
  map<wxSize,wxImage>::iterator imageIt;

  // first, check image cache for the image
  if ((imageIt = cache.find(size)) != cache.end()) {
    return imageIt->second;
  }

  // If the requested size is the default, get the image, cache it, and return
  // This is "lazy" instatiation of default-sized images...
  if (size == wxDefaultSize) {
    wxImage image = wxImage(self->getFileName(name));
    cache[size] = image;
    cache[wxSize(image.GetWidth(),image.GetHeight())] = image;
    return cache[size];
  }

  // If the requested size is specific, get the image, cache it, and return
  // This is "lazy" instatiation of specific-sized images...
  string specificSizeName = self->getFileName(name,size);
  if (!specificSizeName.empty()) {
    wxImage image = wxImage(specificSizeName);
    // there's a chance that the image is still not the specified size...
    image.Rescale(size.GetWidth(),size.GetHeight());
    cache[size] = image;
    return cache[size];
  }
  
  // Since we don't have an image of the specific size, we need to create one.
  wxImage &defaultImage = ewxImage::get(name);
  cache[size] = defaultImage.Scale(size.GetWidth(),size.GetHeight());
  return cache[size];
}


string ewxImage::getFileName(const string& name, const wxSize& size)
{
  string ret;

  map<wxSize,string> &cache = p_names[name];
  map<wxSize,string>::iterator nameIt;
  if ((nameIt = cache.find(size)) != cache.end()) {
    ret = nameIt->second;
  } else {
    ret = cache[wxDefaultSize];
  }

  if (ret.find('/') != 0) {
    ret = fullPath(ret);
  }

  SFile file(ret);
  if (!file.exists()) {
    ret = getFileName(MISSING_IMAGE, size);
  }

  return ret;
}

