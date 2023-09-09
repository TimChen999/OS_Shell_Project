#!/bin/bash
#Script needs permissions to run, given by "chmod +x autoBuild.sh". Then build by running "./autoBuild.sh"
make
./yash 

#ISSUE: calling ctrl+Z interrupts this bash script, wont work with that interrupt