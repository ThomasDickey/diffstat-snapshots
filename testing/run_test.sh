#!/bin/sh
# $Id: run_test.sh,v 1.8 2004/12/18 15:57:56 tom Exp $
# Test-script for DIFFSTAT
if [ $# = 0 ]
then
	eval $0 *.pat
	exit
fi
PATH=`cd ..;pwd`:$PATH; export PATH

for item in $*
do
	echo "testing `basename $item .pat`"
	for OPTS in "" "-p1" "-p9" "-f0" "-u" "-k"
	do
		NAME=`echo $item | sed -e 's/.pat$//'`
		DATA=$NAME.pat
		if [ ".$OPTS" != "." ] ; then
			NAME=$NAME`echo ./$OPTS|sed -e 's@./-@@'`
		fi
		TEST=`basename $NAME`
		diffstat -e $TEST.err -o $TEST.out $OPTS $DATA
		if [ -f $NAME.ref ]
		then
			if ( cmp -s $TEST.out $NAME.ref )
			then
				echo "** ok: $TEST"
				rm -f $TEST.out
				rm -f $TEST.err
			else
				echo "?? fail: $TEST"
				diff -b $NAME.ref $TEST.out
			fi
		else
			echo "** save: $TEST"
			mv $TEST.out $NAME.ref
			rm -f $TEST.err
		fi
	done
done
