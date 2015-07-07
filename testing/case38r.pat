diff -r -u -N "testing/READ ME!" "test ink/READ ME!"
--- "testing/READ ME!"	2004-12-18 07:19:21.000000000 -0500
+++ "test ink/READ ME!"	1969-12-31 19:00:00.000000000 -0500
@@ -1,5 +0,0 @@
--- @Id: README,v 1.1 2004/12/18 12:19:21 tom Exp @
-
-The files in this directory are used for regression-checks of diffstat.  The
-full test-suite is not distributed since some of the test-data is not freely
-distributable.
diff -r -u -N "testing/README ?" "test ink/README ?"
--- "testing/README ?"	1969-12-31 19:00:00.000000000 -0500
+++ "test ink/README ?"	2004-12-18 07:19:21.000000000 -0500
@@ -0,0 +1,5 @@
+-- @Id: README,v 1.1 2004/12/18 12:19:21 tom Exp @
+
+The files in this directory are used for regression-checks of diffstat.  The
+full test-suite is not distributed since some of the test-data is not freely
+distributable.
diff -r -u -N "testing/run atac.sh" "test ink/run atac.sh"
--- "testing/run atac.sh"	1998-01-16 20:10:06.000000000 -0500
+++ "test ink/run atac.sh"	1969-12-31 19:00:00.000000000 -0500
@@ -1,6 +0,0 @@
-#!/bin/sh
-# @Id: run_atac.sh,v 1.2 1998/01/17 01:10:06 tom Exp @
-rm -f /tmp/atac_dir/*
-run_test.sh
-atac -u ../*.atac /tmp/atac_dir/*
-atacmin ../*.atac /tmp/atac_dir/*
diff -r -u -N testing/run_atac.sh "test ink/run_atac.sh"
--- testing/run_atac.sh	1969-12-31 19:00:00.000000000 -0500
+++ "test ink/run_atac.sh"	1998-01-16 20:10:06.000000000 -0500
@@ -0,0 +1,6 @@
+#!/bin/sh
+# @Id: run_atac.sh,v 1.2 1998/01/17 01:10:06 tom Exp @
+rm -f /tmp/atac_dir/*
+run_test.sh
+atac -u ../*.atac /tmp/atac_dir/*
+atacmin ../*.atac /tmp/atac_dir/*
diff -r -u -N "testing/run test.sh" "test ink/run test.sh"
--- "testing/run test.sh"	2012-01-03 05:18:14.000000000 -0500
+++ "test ink/run test.sh"	1969-12-31 19:00:00.000000000 -0500
@@ -1,51 +0,0 @@
-#!/bin/sh
-# @Id: run_test.sh,v 1.15 2012/01/03 10:18:14 tom Exp @
-# Test-script for DIFFSTAT
-
-# change this for ad hoc testing of compression
-#TYPE=.pat.Z
-#TYPE=.pat.gz
-#TYPE=.pat.bz2
-TYPE=.pat
-
-if [ $# = 0 ]
-then
-	eval '"$0" *${TYPE}'
-	exit
-fi
-PATH=`cd ..;pwd`:$PATH; export PATH
-# Sanity check, remembering that not every system has `which'.
-(which diffstat) >/dev/null 2>/dev/null && echo "Checking `which diffstat`"
-
-for item in $*
-do
-	echo "testing `basename $item $TYPE`"
-	for OPTS in "" "-p1" "-p9" "-f0" "-u" "-k" "-r1" "-r2" "-b" "-R" "-Rp0"
-	do
-		NAME=`echo $item | sed -e 's/'$TYPE'$//'`
-		DATA=${NAME}${TYPE}
-		if [ ".$OPTS" != "." ] ; then
-			NAME=$NAME`echo ./$OPTS|sed -e 's@./-@@'`
-		fi
-		TEST=`basename $NAME`
-		diffstat -e $TEST.err -o $TEST.out $OPTS $DATA
-		if [ -f $NAME.ref ]
-		then
-			diff -b $NAME.ref $TEST.out >check.out
-			if test -s check.out
-			then
-				echo "?? fail: $TEST"
-				ls -l check.out
-				cat check.out
-			else
-				echo "** ok: $TEST"
-				rm -f $TEST.out
-				rm -f $TEST.err
-			fi
-		else
-			echo "** save: $TEST"
-			mv $TEST.out $NAME.ref
-			rm -f $TEST.err
-		fi
-	done
-done
diff -r -u -N testing/run_test.sh "test ink/run_test.sh"
--- testing/run_test.sh	1969-12-31 19:00:00.000000000 -0500
+++ "test ink/run_test.sh"	2012-01-03 05:18:14.000000000 -0500
@@ -0,0 +1,51 @@
+#!/bin/sh
+# @Id: run_test.sh,v 1.15 2012/01/03 10:18:14 tom Exp @
+# Test-script for DIFFSTAT
+
+# change this for ad hoc testing of compression
+#TYPE=.pat.Z
+#TYPE=.pat.gz
+#TYPE=.pat.bz2
+TYPE=.pat
+
+if [ $# = 0 ]
+then
+	eval '"$0" *${TYPE}'
+	exit
+fi
+PATH=`cd ..;pwd`:$PATH; export PATH
+# Sanity check, remembering that not every system has `which'.
+(which diffstat) >/dev/null 2>/dev/null && echo "Checking `which diffstat`"
+
+for item in $*
+do
+	echo "testing `basename $item $TYPE`"
+	for OPTS in "" "-p1" "-p9" "-f0" "-u" "-k" "-r1" "-r2" "-b" "-R" "-Rp0"
+	do
+		NAME=`echo $item | sed -e 's/'$TYPE'$//'`
+		DATA=${NAME}${TYPE}
+		if [ ".$OPTS" != "." ] ; then
+			NAME=$NAME`echo ./$OPTS|sed -e 's@./-@@'`
+		fi
+		TEST=`basename $NAME`
+		diffstat -e $TEST.err -o $TEST.out $OPTS $DATA
+		if [ -f $NAME.ref ]
+		then
+			diff -b $NAME.ref $TEST.out >check.out
+			if test -s check.out
+			then
+				echo "?? fail: $TEST"
+				ls -l check.out
+				cat check.out
+			else
+				echo "** ok: $TEST"
+				rm -f $TEST.out
+				rm -f $TEST.err
+			fi
+		else
+			echo "** save: $TEST"
+			mv $TEST.out $NAME.ref
+			rm -f $TEST.err
+		fi
+	done
+done
