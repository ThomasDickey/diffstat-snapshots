#!/bin/sh
# $Id: run_test.sh,v 1.7 2003/02/15 00:58:40 tom Exp $
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
		diffstat -e $N.err -o $N.out $j $I
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
