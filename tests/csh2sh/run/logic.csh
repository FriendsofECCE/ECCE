# env: A=1 B=1 C=1
# env: A=1
# env: B=1 C=1
# env: C=1
# env:
if ($?A || $?B && $?C) echo or-and
if (($?A || $?B) && $?C) echo grouped
if ((! $?A) || ($?B && ! $?C)) echo negated
if (! ($?A && $?B)) echo not-both
