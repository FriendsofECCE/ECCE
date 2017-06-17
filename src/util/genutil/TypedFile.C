#include "util/TypedFile.H"

TypedFile::TypedFile()
{
  p_name = "";
  p_type = "";
}

TypedFile::TypedFile(const string& name, const string& type)
{
  p_name = name;
  p_type = type;
}

TypedFile::TypedFile(const TypedFile& rhs)
{
  p_name = rhs.name();
  p_type = rhs.type();
}

TypedFile::~TypedFile()
{
}

int TypedFile::operator==(const TypedFile& rhs) const
{
  return (rhs.name() == p_name && rhs.type() == p_type);
}

string TypedFile::name() const
{
  return p_name;
}

string TypedFile::type() const
{
  return p_type;
}

void TypedFile::setName(const string& name)
{
  p_name = name;
}


void TypedFile::setType(const string& type)
{
  p_type = type;
}

