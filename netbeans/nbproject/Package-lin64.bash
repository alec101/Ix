#!/bin/bash -x

#
# Generated - do not edit!
#

# Macros
TOP=`pwd`
CND_PLATFORM=CLang-Linux
CND_CONF=lin64
CND_DISTDIR=dist
CND_BUILDDIR=build
CND_DLIB_EXT=so
NBTMPDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}/tmp-packaging
TMPDIRNAME=tmp-packaging
OUTPUT_PATH=../lib/ix.lin64.so
OUTPUT_BASENAME=ix.lin64.so
PACKAGE_TOP_DIR=ix.lin64/

# Functions
function checkReturnCode
{
    rc=$?
    if [ $rc != 0 ]
    then
        exit $rc
    fi
}
function makeDirectory
# $1 directory path
# $2 permission (optional)
{
    mkdir -p "$1"
    checkReturnCode
    if [ "$2" != "" ]
    then
      chmod $2 "$1"
      checkReturnCode
    fi
}
function copyFileToTmpDir
# $1 from-file path
# $2 to-file path
# $3 permission
{
    cp "$1" "$2"
    checkReturnCode
    if [ "$3" != "" ]
    then
        chmod $3 "$2"
        checkReturnCode
    fi
}

# Setup
cd "${TOP}"
mkdir -p ../package
rm -rf ${NBTMPDIR}
mkdir -p ${NBTMPDIR}

# Copy files and create directories and links
cd "${TOP}"
makeDirectory "${NBTMPDIR}/ix.lin64/lib"
copyFileToTmpDir "${OUTPUT_PATH}" "${NBTMPDIR}/${PACKAGE_TOP_DIR}lib/${OUTPUT_BASENAME}" 0644

cd "${TOP}"
makeDirectory "${NBTMPDIR}/ix.lin64/lib"
copyFileToTmpDir "${OUTPUT_PATH}" "${NBTMPDIR}/${PACKAGE_TOP_DIR}lib/${OUTPUT_BASENAME}" 0644


# Generate tar file
cd "${TOP}"
rm -f ../package/ix.lin64.tar
cd ${NBTMPDIR}
tar -vcf ../../../../../package/ix.lin64.tar *
checkReturnCode

# Cleanup
cd "${TOP}"
rm -rf ${NBTMPDIR}
