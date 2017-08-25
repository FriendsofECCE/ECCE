#ifndef TYPEDFILE_HH
#define TYPEDFILE_HH

#include <string>
  using std::string;

// A TypedFile is essentially a file represented as a name and a mime
// type both of which are strings.
class TypedFile
{

  public:

  TypedFile();
  TypedFile(const string& name, const string& type);
  TypedFile(const TypedFile& rhs);
  ~TypedFile();

  int operator==(const TypedFile& rhs) const;

  // Accessors
  string                     name() const;
  string                     type() const;

  // Modifiers
  void                       setName(const string& name);
  void                       setType(const string& type);

  private:
  string p_name;
  string p_type;

};

#endif 
