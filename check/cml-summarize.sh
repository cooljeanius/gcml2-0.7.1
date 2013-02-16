#!@BASH@

VERBOSE=0
FILES=

usage ()
{
    echo "Usage: $0 [--verbose[=level]] [file...]"
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
    --version) echo "cml-summarize @VERSION@" ; exit 0 ;;
    --verbose) : $(( VERBOSE++ )) ;;
    --verbose=*) VERBOSE=${1#*=} ;;
    -*) usage 1 ;;
    *) FILES="$FILES $1" ;;
    esac
    shift
done

# echo VERBOSE=$VERBOSE
# echo FILES=$FILES
# exit

pkgdatadir=
for d in @pkgdatadir@ $(dirname $0) ; do
    if [ -f $d/summarize.awk ]; then
    	pkgdatadir=$d
    	break
    fi
done
[ -z "$pkgdatadir" ] && fatal "Can't find summarize.awk"


awk -v verbose=$VERBOSE -f $pkgdatadir/summarize.awk $FILES
