#!/bin/bash

ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $ROOT/

TSTATUS=0

info() {
    echo "$(tput setaf 2)[  OK  ]$(tput sgr0) $TNAME $1"
}

error() {
    echo "$(tput setaf 3)[ FAIL ]$(tput sgr0) $TNAME $1"
    TSTATUS=1
}

for TDIR in test_*; 
do
   cd $ROOT/$TDIR

   ./test.sh
   GOTSTATUS="$?"
   if [ "$GOTSTATUS" != "0" ] ; then
       error "$TDIR failed"
       continue
   fi
   info "$TDIR passed"
done

exit $TSTATUS
