#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "util/StringConverter.H"
#include "util/STLUtil.H"


bool StringConverter::isAllWhite(const string& str)
{
  return STLUtil::trim(str).empty();
}


bool StringConverter::toValue(const string& str, bool& value)
{
  bool ret = false;

  const char *ptr = str.c_str();

  // skip white space
  while (isspace(*ptr)) ptr++;

  // look for 1 or 0 or true or false
  if (*ptr == '0') {
    ret = true;
    value = false;
  } else if (*ptr == '1') {
    ret = true;
    value = true;
  } else if (strcasecmp(ptr,"true") == 0) {
    ret = true;
    value = true;
  } else if (strcasecmp(ptr,"false") == 0) {
    ret = true;
    value = false;
  }

  return ret;
}


bool StringConverter::toValue(const string& str, int& value)
{
  if (isAllWhite(str)) return false;

  const char *s = str.c_str();
  char *endp;
  int tmp = (int)strtol(s,&endp,10);
  if (s != endp && errno != ERANGE &&
      (*endp == '\0' || *endp == ' ' || *endp == '\t')) {
    value = tmp;
    return true;
  }

  return false;
}


bool StringConverter::toValue(const string& str, long& value)
{
  if (isAllWhite(str)) return false;

  const char *s = str.c_str();
  char *endp;
  long tmp = strtol(s,&endp,10);
  if (s != endp && errno != ERANGE &&
      (*endp == '\0' || *endp == ' ' || *endp == '\t')) {
    value = tmp;
    return true;
  }

  return false;
}


bool StringConverter::toValue(const string& str, size_t& value)
{
  if (isAllWhite(str)) return false;

  const char *s = str.c_str();
  char *endp;
  size_t tmp = (size_t)strtoul(s,&endp,10);
  if (s != endp && (*endp == '\0' || *endp == ' ' || *endp == '\t')) {
    value = tmp;
    return true;
  }

  return false;

}


#if 000
bool StringConverter::toValue(const string& str, unsigned long& value)
{
  if (isAllWhite(str)) return false;

  const char *s = str.c_str();
  char *endp;
  unsigned long tmp = strtoul(s,&endp,10);
  if (s != endp && (*endp == '\0' || *endp == ' ' || *endp == '\t')) {
    value = tmp;
    return true;
  }

  return false;

}
#endif


bool StringConverter::toValue(const string& str, float& value)
{
  if (isAllWhite(str)) return false;

  const char *s = str.c_str();
  char *endp;
  float tmp = strtof(s,&endp);
  if (s != endp && (*endp == '\0' || *endp == ' ' || *endp == '\t')) {
    value = tmp;
    return true;
  }

  return false;

}


bool StringConverter::toValue(const string& str, double& value)
{
  if (isAllWhite(str)) return false;

  const char *s = str.c_str();
  char *endp;
  double tmp = strtod(s,&endp);
  if (s != endp && (*endp == '\0' || *endp == ' ' || *endp == '\t')) {
    value = tmp;
    return true;
  }

  return false;
}


bool StringConverter::toBoolean(const string& str, bool& value)
{
  return toValue(str,value);
}


bool StringConverter::toInt(const string& str, int& value)
{
  return toValue(str,value);
}


bool StringConverter::toLong(const string& str, long& value)
{
  return toValue(str,value);
}


bool StringConverter::toSize_t(const string& str, size_t& value)
{
  return toValue(str,value);
}


bool StringConverter::toFloat(const string& str, float& value)
{
  return toValue(str,value);
}


bool StringConverter::toDouble(const string& str, double& value)
{
  return toValue(str,value);
}


bool StringConverter::toLower(const string& str, string& value)
{
  bool ret = true;
  value = "";
  int strSize = str.size();
  for (int idx=0; idx<strSize; idx++) {
    value += tolower(str[idx]);
  }
  return ret;
}


bool StringConverter::capitalize(const string& str, string& value)
{
  bool ret = true;
  value = "";
  int strSize = str.size();
  value += toupper(str[0]);
  for (int idx=1; idx<strSize; idx++) {
    value += tolower(str[idx]);
  }
  return ret;
}


bool StringConverter::toUpper(const string& str, string& value)
{
  bool ret = true;
  value = "";
  int strSize = str.size();
  for (int idx=0; idx< strSize; idx++) {
    value += toupper(str[idx]);
  }
  return ret;
}


int StringConverter::compare(const string& str1, const string& str2,
                              bool caseSensitive)
{
  string left = str1;
  string right = str2;
  if (!caseSensitive) {
    toLower(str1, left);
    toLower(str2, right);
  }

  return left.compare(right);
}


bool StringConverter::toIntList(const string& str,vector<int>& value,
                                 const char *tokenizer)
{
  bool ret = false;

          // strtok modifies string so gotta do this
  char *cstr = strdup(str.c_str());

  char *token = strtok(cstr,tokenizer);
  int ival;

  if (token) {
    ret = StringConverter::toInt(token,ival);
    if (ret) value.push_back(ival);

    while (ret && (token = strtok(NULL,tokenizer)) != NULL) {
      ret = StringConverter::toInt(token,ival);
      if (ret) value.push_back(ival);
    }
  }

  free(cstr);

  if (!ret) value.clear();
  return ret;
}

bool StringConverter::toDoubleList(const string& str,vector<double>& value,
                                 const char *tokenizer)
{
  bool ret = false;
          // strtok modifies string so gotta do this
  char *cstr = strdup(str.c_str());

  char *token = strtok(cstr,tokenizer);
  double dval;

  if (token) {
    ret = StringConverter::toDouble(token,dval);
    if (ret) value.push_back(dval);

    while (ret && (token = strtok(NULL,tokenizer)) != NULL) {
      ret = StringConverter::toDouble(token,dval);
      if (ret) value.push_back(dval);
    }
  }
  free(cstr);
  if (!ret) value.clear();
  return ret;
}

bool StringConverter::toBoolList(const string& str,vector<bool>& value,
                                 const char *tokenizer)
{
  bool ret = false;
          // strtok modifies string so gotta do this
  char *cstr = strdup(str.c_str());

  char *token = strtok(cstr,tokenizer);
  bool bval;

  if (token) {
    ret = StringConverter::toBoolean(token,bval);
    if (ret) value.push_back(bval);

    while (ret && (token = strtok(NULL,tokenizer)) != NULL) {
      ret = StringConverter::toBoolean(token,bval);
      if (ret) value.push_back(bval);
    }
  }
  free(cstr);
  if (!ret) value.clear();
  return ret;
}

bool StringConverter::toStringList(const string& str,vector<string>& value,
                                 const char *tokenizer)
{
  bool ret = false;

          // strtok modifies string so gotta do this
  char *cstr = strdup(str.c_str());

  char *token = strtok(cstr,tokenizer);

  if (token) {
    ret = true;
    if (ret) value.push_back(token);

    while ((token = strtok(NULL,tokenizer)) != NULL) {
      if (ret) value.push_back(token);
    }
  }
  free(cstr);

  if (!ret) value.clear();
  return ret;
}

// Ranges have the form:
//
// [lo..up]
//
// Where lo is the validation lower limit
//       up is the validation upper limit
//
//       Brackets can be either [ or ( at the lower end and
//                              ] or ) at the upper end where
//                              [] represents a closed set: >=, <=
//                              () represents an open set: >, <
//                              types can be mixed: [) or (]
//
//       At least one limit is necessary (lower or upper) to define a
//       range.
//
//       To define a limit only on one end, e.g. greater than 0, leave
//       the other end limit empty, e.g. (0..)
//       To be strictly correct mathematically, an unconstrained limit
//       should be expressed as an open set meaning () and not []
//       although this rule is not enforced.
//
//       There must be two dots separating lower and upper limits.
//       The first and last characters of the range must be the set
//       notation brackets.
//       No white space is allowed within the entire range expression.
//
//       A range expression that is parsed as invalid based on these
//       rules will result in validation failing and the error message
//       returned indicating why the range is invalid.  The parsing is
//       rather rudimentary with regards to recognizing variations but
//       this quality also removes ambiguity from range expressions.

bool StringConverter::validateRange(const bool& float_flag,
                                    const bool& hard_flag,
                                    const string& value, const string& range,
                                    string& message)
{
  bool ret = false;
  int int_val;
  double dbl_val;

  char* tail;

  if (float_flag) {
    dbl_val = strtod(value.c_str(), &tail);
    if (*tail != '\0') {
      message = "Value \"" + value + "\" is not a valid floating point number.";
      return ret;
    }
  } else {
    int_val = (int)strtol(value.c_str(), &tail, 10);
    if (*tail != '\0') {
      message = "Value \"" + value + "\" is not a valid integer.";
      return ret;
    }
  }

  if (range != "") {
    // parse range into a plethora of variables aborting if errors are
    // encountered.  Bufferes need to large.  MAXFLOAT is
    // 340282346638528859811704183484516925440.000000 not to mention what
    // MAXDOUBLE might look like.
    bool lo_gt = false;
    bool up_lt = false;
    bool lo_flag = false;
    bool up_flag = false;
    int ilo_val, iup_val;
    double dlo_val, dup_val;
    string lo_str;
    string up_str;

    static char rbuf[256];
    static char tbuf[256];
    strcpy(rbuf, range.c_str());
    char* rptr = rbuf;
    char* tptr;
    char* tail;

    if (*rptr!='[' && *rptr!='(') {
      message = "Invalid range \"" + range + "\" (no opening bracket)";
      return ret;
    }

    lo_gt = *rptr=='(';

    rptr++;

    if (!(*rptr=='.' && *(rptr+1)=='.')) {
      lo_flag = true;

      strcpy(tbuf, rptr);

      if ((tptr = strstr(tbuf, ".."))!=NULL) {
        *tptr = '\0';
        lo_str = tbuf;

        if (float_flag)
          dlo_val = strtod(tbuf, &tail);
        else
          ilo_val = (int)strtol(tbuf, &tail, 10);

        if (*tail != '\0') {
          message = "Invalid range \"" + range +
                    "\" (lower limit not a valid integer)";
          return ret;
        }
      } else {
        message = "Invalid range \"" + range +
                  "\" (no \"..\" separating lower and upper limits)";
        return ret;
      }

      rptr = strstr(rbuf, "..") + 2;
    } else
      rptr += 2;

    if (*rptr!=']' && *rptr!=')') {
      up_flag = true;

      if ((tptr = strchr(rptr, ']'))!=NULL ||
          (tptr = strchr(rptr, ')'))!=NULL) {
        up_lt = *tptr==')';
        *tptr = '\0';
        up_str = rptr;

        if (float_flag)
          dup_val = strtod(rptr, &tail);
        else
          iup_val = (int)strtol(rptr, &tail, 10);

        if (*tail != '\0') {
          message = "Invalid range \"" + range +
                    "\" (upper limit not a valid integer)";
          return ret;
        }
      } else {
        message = "Invalid range \"" + range + "\" (no closing bracket)";
        return ret;
      }
    } else
      up_lt = *rptr==')';

    string msg_prefix = hard_flag? "Value entered must be ":
                        "It is recommended that the value entered be ";
    message = "";

    if (lo_flag) {
      if (lo_gt) {
        if ((float_flag && dbl_val<=dlo_val) ||
            (!float_flag && int_val<=ilo_val))
          message = msg_prefix + "greater than " + lo_str;
      } else {
        if ((float_flag && dbl_val<dlo_val) ||
            (!float_flag && int_val<ilo_val))
          message = msg_prefix + "greater than or equal to " + lo_str;
      }
    }

    if (up_flag) {
      if (up_lt) {
        if ((float_flag && dbl_val>=dup_val) ||
            (!float_flag && int_val>=iup_val))
          message = msg_prefix + "less than " + up_str;
      } else {
        if ((float_flag && dbl_val>dup_val) ||
            (!float_flag && int_val>iup_val))
          message = msg_prefix + "less than or equal to " + up_str;
      }
    }

    if (message == "")
      ret = true;
  } else {
    // no range validation -- accept anything that meets int/float validation
    ret = true;
  }

  return ret;
}

bool StringConverter::streamToText(istream& is, string& text)
{
  bool ret = false;

  if (is) {
    char buf[256];
    if (is.getline(buf,255)) {
      ret = true;
      text = buf;
      if (is.peek() != EOF) text.append("\n");
    }
    while (is.getline(buf,255)) {
      text += buf;
      if (is.peek() != EOF) text.append("\n");
    }
  }

  return ret;
}


/**
 *  Returns a string representation of a boolean value
 */
string StringConverter::toString(bool val)
{
    return (val ? "true" : "false");
}


/**
 *  Returns a string representation of an integer value
 */
string StringConverter::toString(int val)
{
    int size = 32;
    char buf[size];
    buf[0] = '\0';

    if (val != INT_MAX)
        snprintf(buf, size, "%d", val);

    return buf;
}


/**
 *  Returns a string representation of a long integer value
 */
string StringConverter::toString(long val)
{
    int size = 32;
    char buf[size];
    buf[0] = '\0';

    if (val != LONG_MAX)
        snprintf(buf, size, "%ld", val);

    return buf;
}


/**
 *  Returns a string representation of a size_t value
 */
string StringConverter::toString(size_t val)
{
    int size = 32;
    char buf[size];
    buf[0] = '\0';

    if (val != INT_MAX)
        snprintf(buf, size, "%zd", val);

    return buf;
}


/**
 *  Returns a string representation of an unsigned long value
 */
#if 000
string StringConverter::toString(unsigned long val)
{
    int size = 32;
    char buf[size];
    buf[0] = '\0';

    if (val != ULONG_MAX)
        snprintf(buf, size, "%lu", val);

    return buf;
}
#endif


string StringConverter::toString(float val)
{
    double dval = val;
    return toString(dval);
}


/**
 *  Returns a string representation of a double value
 */
string StringConverter::toString(double val)
{
    int size = 32;
    char buf[size];
    buf[0] = '\0';

    if (val != DBL_MAX) {
        snprintf(buf, size, "%#0.7g", val);
    }

    return buf;
}


/**
 * Returns a string representation of a list of int values.
 */
string StringConverter::toString(const vector<int>& value, const char *delim)
{
    string ret = "";
    size_t idx;
    for (idx = 0; idx < value.size() - 1; idx++) {
        ret += toString(value[idx]) + delim;
    }
    ret += toString(value[idx]);
    return ret;
}


/**
 * Returns a string representation of a list of string values.
 */
string StringConverter::toString(const vector<double>& value, const char *delim)
{
    string ret = "";
    size_t idx;
    for (idx = 0; idx < value.size() - 1; idx++) {
        ret += toString(value[idx]) + delim;
    }
    ret += toString(value[idx]);
    return ret;
}


/**
 * Returns a string representation of a list of string values.
 */
string StringConverter::toString(const vector<bool>& value, const char *delim)
{
    string ret = "";
    size_t idx;
    for (idx = 0; idx < value.size() - 1; idx++) {
        ret += toString(value[idx]) + delim;
    }
    ret += toString(value[idx]);
    return ret;
}


/**
 * Returns a string representation of a list of string values.
 */
string StringConverter::toString(const vector<string>& value, const char *delim)
{
    string ret = "";
    size_t idx;
    for (idx = 0; idx < value.size() - 1; idx++) {
        ret += value[idx] + delim;
    }
    ret += value[idx];
    return ret;
}


/**
 * Get file name extension.
 *
 * StringConvertor::GetFileNameExt("my.picture.JPEG") returns "JPEG".
 * StringConvertor::GetFileNameExt("my.picture.JPEG", true) returns "jpeg".
 * StringConvertor::GetFileNameExt("no_dot") returns "".
 */
string StringConverter::getFileNameExt(string fileName, bool toLowerCase)
{
  string fileExt = "";
  if (fileName.rfind('.') != string::npos) {
    fileExt = fileName.substr(fileName.rfind('.')+1);
  }
  if (toLowerCase && fileExt != "") {
    string tmp;
    toLower(fileExt, tmp);
    fileExt = tmp;
  }
  return fileExt;
}


/**
 * Get file name base.
 *
 * StringConvertor::GetFileNameExt("my.picture.jpeg") returns "my.picture".
 * StringConvertor::GetFileNameExt("no_dot") returns "no_dot".
 */
string StringConverter::getFileNameBase(string fileName)
{
  string fileBase = fileName;
  if (fileName.rfind('.') != string::npos) {
    fileBase = fileName.substr(0, fileName.rfind('.'));
  }
  return fileBase;
}

