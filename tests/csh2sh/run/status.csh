ls /no/such/directory >& /dev/null
if ($status != 0) then
  echo failed
else
  echo fine
endif
true
if ($status == 0) echo true-ok
set st = $status
echo st=$st
