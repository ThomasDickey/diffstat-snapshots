#!/bin/sh
# $Id: run_test.sh,v 1.2 1996/03/17 00:32:41 tom Exp $
# Test-script for DIFFSTAT
for i in *.pat
do
	for j in "" "-p1" "-p9"
	do
		N=`basename $i .pat`
		if [ ".$j" != "." ] ; then
			N=$N`echo ./$j|sed -e 's@./-@@'`
		fi
		../diffstat $j $i >$N.out
		if [ -f $N.ref ]
		then
			if ( cmp -s $N.out $N.ref )
			then
				echo '** ok: '$N
				rm -f $N.out
			else
				echo '?? fail: '$N
				diff -b $N.out $N.ref
			fi
		else
			echo '** save: '$N
			mv $N.out $N.ref
		fi
	done
done
