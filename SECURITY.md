# Security

ECCE handles logins: ssh to compute machines, and passwords for the data
server and the message broker. Please report a security problem privately,
not in a public issue, so that it can be fixed before it is known.

**How to report**

- On GitHub: the repository's **Security** tab, **Report a vulnerability**.
  Only the maintainers see the report.
- Or by email to andy.ohlin@ik.me.

Say what the problem is, which version of ECCE (`ecce --version`) and
system it affects, and how to reproduce it. You will get an answer, and
when the problem is fixed the release notes will say so, with credit to
you if you wish.

**Which versions**

Fixes go into the current 9.x preview and, for problems that also affect
it, into the current 8.18.x release.

**Before you deploy a central server**

The data server's and the broker's passwords cross the network
unencrypted unless TLS is set up (`ecce-remote-setup --tls`). The
passwords keep users' work apart on a shared server; they are not meant
to protect a server open to the internet. See
[Installing ECCE](docs/INSTALLING.md#deployment-modes).
