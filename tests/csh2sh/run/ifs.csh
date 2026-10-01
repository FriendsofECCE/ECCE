# env:
# env: FOO=1
# env: FOO= MODE=fast
# env: MODE=slow TMPDIR=/tmp
if (-e /etc/hostname) then
  echo hostname-file
endif
if (! $?MODE) setenv MODE default
echo mode=$MODE
if ($?FOO) then
  echo foo-set
else if ("$MODE" == "slow") then
  echo mode-slow
else
  echo neither
endif
if (-d /tmp && ! -f /tmp/no-such-file) echo tmp-ok
if ("$MODE" != "fast" || -e /no/such) then
  echo not-fast
endif
if ($totalprocs > 1) then
  echo parallel
else
  echo serial
endif
if (`echo $runDir | grep -c nothere` == "0") then
  echo not-there
endif
