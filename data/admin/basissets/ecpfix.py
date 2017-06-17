#!/usr/bin/python
import sys

infile=sys.argv[1]
outfile=sys.argv[2]

unprocessed=open(infile,'r')
processed=open(outfile,'w')
ecp=False
for line in unprocessed:
        if 'nelec' in line:
                if ecp==True:
                    processed.write('END\n')
                    processed.write('ECP\n')
                    processed.write(line)
                else:
                    ecp=True
                    processed.write(line)
        else:
                processed.write(line)
processed.close()
unprocessed.close()
