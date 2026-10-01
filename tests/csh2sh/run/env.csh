setenv A one
setenv B "two words"
set c = three
set d = "four words"
setenv E $c
unsetenv HOME_UNUSED
setenv F `echo from backticks`
echo "A=$A B=$B c=$c d=$d E=$E F=$F"
