/* exp_tty_comm.c - tty support routines common to both Expect program
   and library */

#include <stdio.h>

#include "comm/tcl.h"
#include "comm/exp_tty_in.h"
#include "comm/exp_rename.h"
#include "comm/expect_comm.h"
#include "comm/exp_command.h"
#include "comm/exp_log.h"

#ifndef TRUE
#define FALSE 0
#define TRUE 1
#endif

int exp_disconnected = FALSE;		/* not disc. from controlling tty */

/*static*/ exp_tty exp_tty_current, exp_tty_cooked;
#define tty_current exp_tty_current
#define tty_cooked exp_tty_cooked

void
exp_init_tty()
{
	extern exp_tty exp_tty_original;

	/* save original user tty-setting in 'cooked', just in case user */
	/* asks for it without earlier telling us what cooked means to them */
	tty_cooked = exp_tty_original;

	/* save our current idea of the terminal settings */
	tty_current = exp_tty_original;
}

