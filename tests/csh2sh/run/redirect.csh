echo one >& out.txt
echo two >>& out.txt
echo three >! out2.txt
echo four >> out2.txt
cat out.txt out2.txt |& cat
echo to-stderr-and-out >& /dev/null
