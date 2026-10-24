---
type: pitfall
title: passdialog's exit status is part of ecce-askpass's contract
area: services
paths: [src/apps/passdialog/PasswordDialogApp.C, scripts/ecce-askpass, tests/transport/sshd/installed_askpass_test.sh]
issues: [487]
---
A wx program whose `OnInit` returns false exits with 255. passdialog did that
after OK as well as after Cancel, and `ecce-askpass` read any non-zero status
as a cancel, so every ECCE-owned master (#487) failed to open on a
password-only host and the user got the "no shared ssh connection" text.
The libssh path never noticed: it reads the output and ignores the status.

`tests/transport/sshd/owned_master_test.sh` used a stub passdialog that exits
0, so it passed. `installed_askpass_test.sh` runs the staged install
(`cmake --install` with DESTDIR) with the real passdialog, answered through
`ECCE_TEST_AUTH_ANSWER`. Any helper a script calls should be tested as
installed, not as a stub.
