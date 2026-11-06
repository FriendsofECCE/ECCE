#ifndef SECRETSTORE_HH
#define SECRETSTORE_HH

#include <string>
#include <vector>

// The system's password store for ssh logins ("Remember on this computer"):
// secret-tool (libsecret) on Linux, `security` (Keychain) on macOS, the
// Credential Manager on Windows.  The secret only ever travels on the
// helper's stdin or stdout, never in argv or the environment, and every
// call is bounded (a locked keyring may prompt or hang).  A failure reads as
// "not found", so callers fall through to the dialog.
// ECCE_SECRET_TOOL names the secret-tool program (tests).
namespace SecretStore {

struct Login {
  std::string user, host, jump;
  int port = 22;
};

bool available();
bool lookup(const Login& l, std::string& password);
bool save(const Login& l, const std::string& password);
bool forget(const Login& l);

// A non-secret list of what was remembered, so Preferences can offer Forget
// without enumerating the store: ~/.ECCE/remembered-logins.
std::vector<Login> remembered();
void noteRemembered(const Login& l);
void noteForgotten(const Login& l);
// Forget from the store and the list.
void forgetRemembered(const Login& l);

}  // namespace SecretStore

#endif
