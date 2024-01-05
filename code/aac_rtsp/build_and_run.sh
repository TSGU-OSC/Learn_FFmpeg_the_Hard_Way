#!/bin/bash

# run make command to compile create goal file
make

# check if the target file has been generated
if [ -f main ]; then
    #execute the generated main executable file
    ./main
else
    echo "fail to compile, can not generated main executable file."
fi