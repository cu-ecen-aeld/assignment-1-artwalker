#!/bin/sh
# Tester script for assignment 2
# Author: Siddhant Jajoo

set -e
set -u

NUMFILES=10
WRITESTR=AELD_IS_FUN
WRITEDIR=/tmp/aeld-data
#username=$(cat conf/username.txt)
username=$(cat /home/xinxingwang/Projects/Coursera/13_LinuxSystemProgramming/assignment-4-artwalker/conf/username.txt)

if [ $# -lt 3 ]
then
    echo "Using default value ${WRITESTR} for string to write"
    if [ $# -lt 1 ]
    then
        echo "Using default value ${NUMFILES} for number of files to write"
    else
        NUMFILES=$1
    fi    
else
    NUMFILES=$1
    WRITESTR=$2
    WRITEDIR=/tmp/aeld-data/$3
fi

MATCHSTR="The number of files are ${NUMFILES} and the number of matching lines are ${NUMFILES}"

echo "Writing ${NUMFILES} files containing string ${WRITESTR} to ${WRITEDIR}"

rm -rf "${WRITEDIR}"

# create $WRITEDIR if not assignment1
# assignment=$(cat ../conf/assignment.txt)
# assignment=$(cat conf/assignment.txt)
assignment=$(cat /home/xinxingwang/Projects/Coursera/13_LinuxSystemProgramming/assignment-4-artwalker/conf/assignment.txt)

if [ $assignment != 'assignment1' ]
then
    mkdir -p "$WRITEDIR"

    if [ -d "$WRITEDIR" ]
    then
        echo "$WRITEDIR created"
    else
        exit 1
    fi
fi

# echo "Removing the old writer utility and compiling as a native application"

# Clean any previous build artifacts
#make clean

# Compile the writer application using native compilation
#make

for i in $(seq 1 $NUMFILES)
do
    writer "$WRITEDIR/${username}$i.txt" "$WRITESTR"
done

touch /tmp/assignment4-result.txt
OUTPUTSTRING=$(finder.sh "$WRITEDIR" "$WRITESTR")
finder.sh "$WRITEDIR" "$WRITESTR" > /tmp/assignment4-result.txt

# remove temporary directories
rm -rf /tmp/aeld-data

set +e
echo ${OUTPUTSTRING} | grep "${MATCHSTR}"
if [ $? -eq 0 ]; then
    echo "success"
    exit 0
else
    echo "failed: expected ${MATCHSTR} in ${OUTPUTSTRING} but instead found"
    exit 1
fi
