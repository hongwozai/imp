#!/bin/bash


BIN=$(dirname $0)/build/
filename=$1
objfile=${filename}.o
binfile=`echo ${filename} | sed -r 's/(.*)\.[^.]+/\1/g'`
libfile=${BIN}/libimp_runtime.a

echo "filename: ${filename} objfile: ${objfile} binfile: ${binfile}"

${BIN}/imp_compiler $filename 2>&1 1>/dev/null | tee ${binfile}.S | as -o $objfile

if [ $? -ne 0 ] || [ ! -f "${objfile}" ]; then
    echo "Compile Failed"
    exit $?
fi
# cat ${binfile}.S

g++ -static -o ${binfile} ${objfile} ${libfile}

rm -rf $objfile ${binfile}.S

echo "start:  ./${binfile}"
./${binfile}
echo -e "\nend"
