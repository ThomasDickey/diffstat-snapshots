#!/bin/sh
# $Id: run_test.sh,v 1.1 1996/03/16 22:32:02 tom Exp $
# Test-script for DIFFSTAT
for i in *.pat
do
	N=`basename $i .pat`
	../diffstat $i >$N.out
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
