#ifndef COMPAIROPS_H
#define COMPAIROPS_H

#include <string.h>

#include <string>
  using std::string;

struct equstring {
  bool operator()(const string& s1, const string& s2) const {
    return s1 ==  s2;
  }
};

struct ltstringPtr {
  bool operator()(const string* s1, const string* s2) const {
    return (*s1) < (*s2);
  }
};

struct ltstring {
  bool operator()(const string& s1, const string& s2) const {
    return s1 < s2;
  }
};
struct gtstring {
  bool operator()(const string& s1, const string& s2) const {
    return s1 >  s2;
  }
};

struct equstr {
  bool operator()(const char* s1, const char* s2) const {
    return strcmp(s1, s2) == 0;
  }
};
struct ltstr {
  bool operator()(const char* s1, const char* s2) const {
    return strcmp(s1, s2) < 0;
  }
};
struct gtstr {
  bool operator()(const char* s1, const char* s2) const {
    return strcmp(s1, s2) > 0;
  }
};

struct equint {
  bool operator()(const int s1, const int s2) const {
    return s1 == s2;
  }
};
struct ltint {
  bool operator()(const int s1, const int s2) const {
    return s1 < s2;
  }
};
struct gtint {
  bool operator()(const int s1, const int s2) const {
    return s1 > s2;
  }
};

struct equfloat {
  bool operator()(const float& s1, const float& s2) const {
    return s1 == s2;
  }
};
struct ltfloat {
  bool operator()(const float& s1, const float& s2) const {
    return s1 < s2;
  }
};
struct gtfloat {
  bool operator()(const float& s1, const float& s2) const {
    return s1 > s2;
  }
};

#endif







