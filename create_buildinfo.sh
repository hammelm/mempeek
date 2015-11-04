#!/bin/sh

build_header="generated/buildinfo.h"
build_source="generated/buildinfo.c"

BUILD_NO=`git rev-parse --short HEAD` || BUILD_NO="X"
BUILD_DATE=`date +"%F %T"`

if [ "$BUILD_NO" != "X" ]; then
    if [ `git status --porcelain --untracked-files=no | wc -l` != 0 ]; then
        BUILD_NO="$BUILD_NO-dirty"
    fi
fi

test -e $build_header || cat <<EOF > $build_header
#ifndef __buildinfo_h__
#define __buildinfo_h__

#define BUILD_NO __buildinfo_build_no
#define BUILD_DATE __buildinfo_build_date

#ifdef __cplusplus
extern "C" {
#endif

extern const char* __buildinfo_build_no;
extern const char* __buildinfo_build_date;

#ifdef __cplusplus
}
#endif

#endif // __buildinfo_h__
EOF

cat <<EOF > $build_source
#ifdef __cplusplus
extern "C" {
#endif

const char* __buildinfo_build_no = "$BUILD_NO";
const char* __buildinfo_build_date = "$BUILD_DATE";

#ifdef __cplusplus
}
#endif
EOF
