#!/bin/bash


BIN=$(dirname $0)/build/
filename=$1
objfile=${filename}.o
binfile=`echo ${filename} | sed -r 's/(.*)\.[^.]+/\1/g'`
libfile=${BIN}/libimp_runtime.a

${BIN}/imp_compiler $filename | tee ${binfile}.S | as -o $objfile

if [ $? -ne 0 ]; then
    echo "Compile Failed"
    exit $?
fi
cat ${binfile}.S

g++ -static -o ${binfile} ${objfile} ${libfile}

rm -rf $objfile
