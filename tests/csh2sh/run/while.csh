touch flag.tmp
while (-e flag.tmp)
  echo removing
  rm -f flag.tmp
end
echo gone
