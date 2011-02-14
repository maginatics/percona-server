#!/bin/sh
# Usage: build-dpkg.sh [target dir]
# The default target directory is the current directory. If it is not
# supplied and the current directory is not empty, it will issue an error in
# order to avoid polluting the current directory after a test run.
#
# The program will setup the dpkg building environment and ultimately call
# dpkg-buildpackage with the appropiate parameters.
#

# Bail out on errors, be strict
set -ue

# Examine parameters
go_out="$(getopt --options "k:" --longoptions key: \
    --name "$(basename "$0")" -- "$@")"
test $? -eq 0 || exit 1
eval set -- $go_out

BUILDPKG_KEY=''

for arg
do
    case "$arg" in
    -- ) shift; break;;
    -k | --key ) shift; BUILDPKG_KEY="-k$1"; shift;;
    esac
done

# Working directory
if test "$#" -eq 0
then
    WORKDIR="$(pwd)"

    # Check that the current directory is not empty
    if test "x$(echo *)" != "x*"
    then
        echo >&2 \
            "Current directory is not empty. Use $0 . to force build in ."
        exit 1
    fi

elif test "$#" -eq 1
then
    WORKDIR="$1"

    # Check that the provided directory exists and is a directory
    if ! test -d "$WORKDIR"
    then
        echo >&2 "$WORKDIR is not a directory"
        exit 1
    fi

else
    echo >&2 "Usage: $0 [target dir]"
    exit 1

fi

SOURCEDIR="$(cd $(dirname "$0"); cd ..; pwd)"
test -e "$SOURCEDIR/Makefile" || exit 2

# Extract version from the Makefile
MYSQL_VERSION="$(grep ^MYSQL_VERSION= "$SOURCEDIR/Makefile" \
    | cut -d = -f 2)"
PRODUCT="Percona-Server-$MYSQL_VERSION"

# Build information
export REVISION="$(cd "$SOURCEDIR"; bzr log -r-1 | grep ^revno: | cut -d ' ' -f 2)"
export DEB_BUILD_OPTIONS='nostrip debug nocheck'

(
    cd "$WORKDIR"

    # Download MySQL source
    if ! test -e "mysql-$MYSQL_VERSION.tar.gz"
    then
        wget "http://www.percona.com/downloads/community/mysql-$MYSQL_VERSION.tar.gz"
    fi

    rm -rf "mysql-$MYSQL_VERSION/"
    tar xzf "mysql-$MYSQL_VERSION.tar.gz"
    
    (
        cd "mysql-$MYSQL_VERSION/"

        # Copy debian files from source
        cp -R "$SOURCEDIR/build/debian" .
        chmod +x debian/rules
    
        # Apply patches
        for p in $(cat "$SOURCEDIR/series")
        do
            patch -p1 < "$SOURCEDIR/$p"
        done

        dpkg-buildpackage -rfakeroot -b $BUILDPKG_KEY

    )

)

# Issue rpmbuild command


