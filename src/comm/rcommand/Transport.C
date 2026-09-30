#include "comm/Transport.H"

Transport::~Transport() {}

void Transport::setEnv(const std::string& name, const std::string& value)
{
  p_unset.erase(name);
  p_env[name] = value;
}

void Transport::unsetEnv(const std::string& name)
{
  p_env.erase(name);
  p_unset[name] = true;
}

std::string Transport::withDir(const std::string& script) const
{
  if (p_dir.empty()) return script;
  std::string q = "'";
  for (size_t i = 0; i < p_dir.size(); i++) {
    if (p_dir[i] == '\'') q += "'\\''";
    else q += p_dir[i];
  }
  q += "'";
  return "cd -- " + q + " || exit 97\n" + script;
}
