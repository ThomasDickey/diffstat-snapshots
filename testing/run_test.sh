#!/bin/sh
# $Id: run_test.sh,v 1.6 2003/01/04 19:33:15 tom Exp $
# Test-script for DIFFSTAT
if [ $# = 0 ]
then
	eval $0 *.pat
	exit
fi
PATH=`cd ..;pwd`:$PATH; export PATH

for i in $*
do
	N=`basename $i .pat`
	echo "testing $N"
	for j in "" "-p1" "-p9" "-f0" "-u" "-k"
	do
		N=`basename $i .pat`
		I=$N.pat
		if [ ".$j" != "." ] ; then
			N=$N`echo ./$j|sed -e 's@./-@@'`
		fi
		diffstat $j $I >$N.out 2>$N.err
		if [ -f $N.ref ]
		then
			if ( cmp -s $N.out $N.ref )
			then
				echo '** ok: '$N
				rm -f $N.out
			else
				echo '?? fail: '$N
				diff -b $N.ref $N.out
			fi
		else
			echo '** save: '$N
			mv $N.out $N.ref
		fi
		rm -f $N.err
	done
done
