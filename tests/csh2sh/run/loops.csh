# env: list=a
# env: list=a b c
foreach f (x "y z" $list `echo p q`)
  echo item $f
end
foreach d (1 2 3)
  if ($d == 2) echo two
end
