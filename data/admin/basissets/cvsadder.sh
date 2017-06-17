#!/bin/sh

# Use this script to make sure that every basis set file is included
# in the CVS repository
for i in *.BAS *.BAS.meta *.POT *.POT.meta ; do
  cvs add $i
done


