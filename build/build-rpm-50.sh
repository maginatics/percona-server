#!/bin/sh
#
# Execute this tool to setup and build RPMs for Percona-Server starting
# from a fresh tree
#
# Usage: build-rpm-50.sh [target dir]
# The default target directory is the current directory. If it is not
# supplied and the current directory is not empty, it will issue an error in
# order to avoid polluting the current directory after a test run.
#
# The program will setup the rpm building environment and ultimately call
# rpmbuild with the appropiate parameters.
#

# Bail out on errors, be strict
set -ue

# Examine parameters
TARGET=''
TARGET_CFLAGS=''

# Check if we have a functional getopt(1)
if ! getopt --test
then
    go_out="$(getopt --options="i" --longoptions=i686 \
        --name="$(basename "$0")" -- "$@")"
    test $? -eq 0 || exit 1
    eval set -- $go_out
fi

for arg
do
    case "$arg" in
    -- ) shift; break;;
    -i | --i686 )
        shift
        TARGET="--target i686"
        TARGET_CFLAGS="-m32 -march=i686"
        ;;
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

    WORKDIR_ABS="$(cd "$WORKDIR"; pwd)"

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
REDHAT_RELEASE="$(grep -o 'release [0-9][0-9]*' /etc/redhat-release | \
    cut -d ' ' -f 2)"
PATCHSET="$(grep ^PATCHSET= "$SOURCEDIR/Makefile" | cut -d = -f 2)"
REVISION="$(cd "$SOURCEDIR"; bzr log -r-1 | grep ^revno: | cut -d ' ' -f 2)"

# Compilation flags
export CC=gcc
export CXX=gcc
export CFLAGS="-fPIC -Wall -O3 -g -static-libgcc -fno-omit-frame-pointer $TARGET_CFLAGS"
export CXXFLAGS="-O2 -fno-omit-frame-pointer -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fno-exceptions $TARGET_CFLAGS"
export MAKE_JFLAG=-j4

# Create directories for rpmbuild if these don't exist
(cd "$WORKDIR" && mkdir -p BUILD RPMS SOURCES SPECS SRPMS)

(
    cd "$WORKDIR"

    # Download mysql, prepare patches
    (
        cd SOURCES

        if ! test -e "mysql-$MYSQL_VERSION.tar.gz"
        then
            wget "http://www.percona.com/downloads/community/mysql-$MYSQL_VERSION.tar.gz"
        fi

        cp "$SOURCEDIR/"*.patch .

    )

    # Issue RPM command
    rpmbuild --sign -ba --clean --with yassl $TARGET \
        "$SOURCEDIR/build/percona-sql-50.spec" \
        --define "_topdir $WORKDIR_ABS" --define "percona 1" \
        --define "mysqlversion $MYSQL_VERSION" \
        --define "redhat_version $REDHAT_RELEASE" \
        --define "revision $REVISION" \
        --define "patchset $PATCHSET" \
        --define "buildnumber $REVISION"

)

