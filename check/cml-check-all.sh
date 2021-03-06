#!@BASH@

bindir=$(dirname $0)
[ "$bindir" = "." ] && bindir=$PWD

LINUXDIR=
CHECK="$bindir/cml-check"
CHECK_FLAGS=
SUMMARIZE="$bindir/cml-summarize"
SUMMARIZE_FLAGS=
DO_MERGE=yes
DO_SUMMARY=yes
NO_ARCHES="dummy,merge"
WARNINGS=

usage ()
{
    echo "Usage: $0 [--no-merge] [--no-arch ARCH[,ARCH...]] [--raw] linux-source-dir"
    exit ${1:-1}
}

fatal ()
{
    echo "$0: $*"
    exit 1
}

while [ $# -gt 0 ]; do
    case "$1" in
    --help) usage 0 ;;
    --version) echo "cml-check-all @VERSION@" ; exit 0 ;;
    --verbose) SUMMARIZE_FLAGS="$SUMMARIZE_FLAGS $1" ;;
    --verbose=*) SUMMARIZE_FLAGS="$SUMMARIZE_FLAGS $1" ;;
    --debug) CHECK_FLAGS="$CHECK_FLAGS $1 $2" ; shift ;;
    --debug=*) CHECK_FLAGS="$CHECK_FLAGS $1" ;;
    --no-merge) DO_MERGE= ;;
    --raw) DO_SUMMARY= ;;
    --no-arch) NO_ARCHES="$NO_ARCHES,$2" ; shift ;;
    --no-arch=*) NO_ARCHES="$NO_ARCHES,${1#*=}" ;;
    --warning) WARNINGS="$WARNINGS -W$2" shift ; ;;
    --warning=*) WARNINGS="$WARNINGS -W${1#*=}" ;;
    -W*) WARNINGS="$WARNINGS $1" ;;
    -*) usage 1 ;;
    *)
    	[ -z "$LINUXDIR" ] || usage 1
	LINUXDIR="$1"
	;;
    esac
    shift
done
[ -z "$LINUXDIR" ] && usage 1

# echo "LINUXDIR=$LINUXDIR"
# echo "CHECK=$CHECK"
# echo "CHECK_FLAGS=$CHECK_FLAGS"
# echo "SUMMARIZE=$SUMMARIZE"
# echo "SUMMARIZE_FLAGS=$SUMMARIZE_FLAGS"
# echo "DO_MERGE=$DO_MERGE"
# echo "NO_ARCHES=$NO_ARCHES"
# echo "WARNINGS=$WARNINGS"
# exit

[ -d "$LINUXDIR/arch" ] || fatal "${LINUXDIR}: No such directory"
ARCHES=$(cd $LINUXDIR/arch ; ls | egrep -v ^\("${NO_ARCHES//,/|}"\)\$)

if [ "$WARNINGS" ]; then
    CHECK_SINGLE_FLAGS="$WARNINGS"
    CHECK_MERGE_FLAGS="$WARNINGS"
else
    MERGE_WARNINGS="\
undeclared-symbol \
different-banner \
different-parent \
"

    CHECK_SINGLE_FLAGS="-Wall $(echo $MERGE_WARNINGS | sed -e 's|\([^[:blank:]]\+\)|-Wno-\1|g')"
    CHECK_MERGE_FLAGS="-Wno-all $(echo $MERGE_WARNINGS | sed -e 's|\([^[:blank:]]\+\)|-W\1|g')"
fi

# echo "CHECK_SINGLE_FLAGS=$CHECK_SINGLE_FLAGS"
# echo "CHECK_MERGE_FLAGS=$CHECK_MERGE_FLAGS"
# echo "ARCHES=$ARCHES"
# exit

dochecks ()
{
    [ "@PROFILE@" = "1" ] && rm cml-check.time
    for arch in $ARCHES ${DO_MERGE:+merge} ; do
	(
    	    cd $LINUXDIR
	    echo
	    echo "===== $arch"
	    if [ $arch = merge ]; then
		mode_flags="$CHECK_MERGE_FLAGS"
		target_arches="$ARCHES"
	    else
		mode_flags="$CHECK_SINGLE_FLAGS"
		target_arches=$arch
	    fi
    	    [ "@PROFILE@" = "1" -a -f gmon.out ] && rm -f gmon.out
    	    [ "@DEBUG@" = "1" -a -f core ] && rm -f core
	    $CHECK $CHECK_FLAGS \
	    	    $mode_flags \
	    	    $(echo $target_arches | sed -e 's|\([^[:blank:]]\+\)|arch/\1/config.in|g')
    	    [ "@PROFILE@" = "1" -a -f gmon.out ] && mv gmon.out gmon.$arch.out
    	    [ "@DEBUG@" = "1" -a -f core ] && mv core core.$arch
	)
    done

    # Merge the profile data into one
    (
	cd $LINUXDIR
	GMON_OUT=
	for arch in $ARCHES ${DO_MERGE:+merge} ; do
	    [ -f gmon.$arch.out ] && GMON_OUT="$GMON_OUT gmon.$arch.out"
	done
	gprof -s $CHECK $GMON_OUT
    	rm $GMON_OUT
    )

}

if [ "$DO_SUMMARY" ]; then
    dochecks 2>&1 | $SUMMARIZE $SUMMARIZE_FLAGS
else
    dochecks
fi

