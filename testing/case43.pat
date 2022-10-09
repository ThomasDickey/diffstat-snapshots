diff '--unified=0' -r ncurses-6.3-20221001/configure ncurses-6.3-20221008/configure
--- ncurses-6.3-20221001/configure	2022-10-01 09:25:01.000000000 -0400
+++ ncurses-6.3-20221008/configure	2022-10-08 12:55:47.000000000 -0400
@@ -2 +2 @@
-# From configure.in Revision: 1.749 .
+# From configure.in Revision: 1.750 .
@@ -15412 +15412 @@
-(789)
+([789])
diff '--unified=0' -r ncurses-6.3-20221001/configure.in ncurses-6.3-20221008/configure.in
--- ncurses-6.3-20221001/configure.in	2022-10-01 09:16:18.000000000 -0400
+++ ncurses-6.3-20221008/configure.in	2022-10-08 11:54:35.000000000 -0400
@@ -32 +32 @@
-dnl $Id: configure.in,v 1.749 2022/10/01 13:16:18 tom Exp $
+dnl $Id: configure.in,v 1.750 2022/10/08 15:54:35 tom Exp $
@@ -41 +41 @@
-AC_REVISION($Revision: 1.749 $)
+AC_REVISION($Revision: 1.750 $)
@@ -1275 +1275 @@
-([789])
+([[789]])
diff '--unified=0' -r ncurses-6.3-20221001/dist.mk ncurses-6.3-20221008/dist.mk
--- ncurses-6.3-20221001/dist.mk	2022-10-01 09:15:31.000000000 -0400
+++ ncurses-6.3-20221008/dist.mk	2022-10-08 06:25:44.000000000 -0400
@@ -29 +29 @@
-# $Id: dist.mk,v 1.1505 2022/10/01 13:15:31 tom Exp $
+# $Id: dist.mk,v 1.1506 2022/10/08 10:25:44 tom Exp $
@@ -41 +41 @@
-NCURSES_PATCH = 20221001
+NCURSES_PATCH = 20221008
diff '--unified=0' -r ncurses-6.3-20221001/misc/gen-pkgconfig.in ncurses-6.3-20221008/misc/gen-pkgconfig.in
--- ncurses-6.3-20221001/misc/gen-pkgconfig.in	2022-08-27 15:07:03.000000000 -0400
+++ ncurses-6.3-20221008/misc/gen-pkgconfig.in	2022-10-08 12:45:20.000000000 -0400
@@ -2 +2 @@
-# $Id: gen-pkgconfig.in,v 1.55 2022/08/27 19:07:03 tom Exp $
+# $Id: gen-pkgconfig.in,v 1.56 2022/10/08 16:45:20 tom Exp $
@@ -106 +106,5 @@
-		[ -d "$lib_check" ] || continue
+		# on a new/nonstandard install, $libdir may not yet exist at this point
+		if [ "$libdir" != "$lib_check" ]
+		then
+			[ -d "$lib_check" ] || continue
+		fi
diff '--unified=0' -r ncurses-6.3-20221001/NEWS ncurses-6.3-20221008/NEWS
--- ncurses-6.3-20221001/NEWS	2022-10-01 18:20:28.000000000 -0400
+++ ncurses-6.3-20221008/NEWS	2022-10-08 12:50:03.000000000 -0400
@@ -29 +29 @@
--- $Id: NEWS,v 1.3862 2022/10/01 22:20:28 tom Exp $
+-- $Id: NEWS,v 1.3864 2022/10/08 16:50:03 tom Exp $
@@ -47,0 +48,7 @@
+
+20221008
+	+ correct a switch-statement case in configure script to allow for test
+	  builds with ABI=7.
+	+ modify misc/gen-pkgconfig.in to allow for the case where the library
+	  directory does not yet exist, since this is processed before doing an
+	  install (report by Michal Liszcz).
diff '--unified=0' -r ncurses-6.3-20221001/package/debian/changelog ncurses-6.3-20221008/package/debian/changelog
--- ncurses-6.3-20221001/package/debian/changelog	2022-10-01 05:34:57.000000000 -0400
+++ ncurses-6.3-20221008/package/debian/changelog	2022-10-08 06:25:44.000000000 -0400
@@ -1 +1 @@
-ncurses6 (6.3+20221001) unstable; urgency=low
+ncurses6 (6.3+20221008) unstable; urgency=low
@@ -5 +5 @@
- -- Thomas E. Dickey <dickey@invisible-island.net>  Fri, 30 Sep 2022 20:16:53 -0400
+ -- Thomas E. Dickey <dickey@invisible-island.net>  Sat, 08 Oct 2022 06:25:44 -0400
diff '--unified=0' -r ncurses-6.3-20221001/package/debian-mingw/changelog ncurses-6.3-20221008/package/debian-mingw/changelog
--- ncurses-6.3-20221001/package/debian-mingw/changelog	2022-10-01 05:34:57.000000000 -0400
+++ ncurses-6.3-20221008/package/debian-mingw/changelog	2022-10-08 06:25:44.000000000 -0400
@@ -1 +1 @@
-ncurses6 (6.3+20221001) unstable; urgency=low
+ncurses6 (6.3+20221008) unstable; urgency=low
@@ -5 +5 @@
- -- Thomas E. Dickey <dickey@invisible-island.net>  Fri, 30 Sep 2022 20:16:53 -0400
+ -- Thomas E. Dickey <dickey@invisible-island.net>  Sat, 08 Oct 2022 06:25:44 -0400
diff '--unified=0' -r ncurses-6.3-20221001/package/debian-mingw64/changelog ncurses-6.3-20221008/package/debian-mingw64/changelog
--- ncurses-6.3-20221001/package/debian-mingw64/changelog	2022-10-01 05:34:57.000000000 -0400
+++ ncurses-6.3-20221008/package/debian-mingw64/changelog	2022-10-08 06:25:44.000000000 -0400
@@ -1 +1 @@
-ncurses6 (6.3+20221001) unstable; urgency=low
+ncurses6 (6.3+20221008) unstable; urgency=low
@@ -5 +5 @@
- -- Thomas E. Dickey <dickey@invisible-island.net>  Fri, 30 Sep 2022 20:16:53 -0400
+ -- Thomas E. Dickey <dickey@invisible-island.net>  Sat, 08 Oct 2022 06:25:44 -0400
diff '--unified=0' -r ncurses-6.3-20221001/package/mingw-ncurses.nsi ncurses-6.3-20221008/package/mingw-ncurses.nsi
--- ncurses-6.3-20221001/package/mingw-ncurses.nsi	2022-10-01 05:34:57.000000000 -0400
+++ ncurses-6.3-20221008/package/mingw-ncurses.nsi	2022-10-08 06:25:44.000000000 -0400
@@ -1 +1 @@
-; $Id: mingw-ncurses.nsi,v 1.544 2022/10/01 09:34:57 tom Exp $
+; $Id: mingw-ncurses.nsi,v 1.545 2022/10/08 10:25:44 tom Exp $
@@ -13 +13 @@
-!define VERSION_MMDD  "1001"
+!define VERSION_MMDD  "1008"
diff '--unified=0' -r ncurses-6.3-20221001/package/mingw-ncurses.spec ncurses-6.3-20221008/package/mingw-ncurses.spec
--- ncurses-6.3-20221001/package/mingw-ncurses.spec	2022-10-01 05:34:57.000000000 -0400
+++ ncurses-6.3-20221008/package/mingw-ncurses.spec	2022-10-08 06:25:44.000000000 -0400
@@ -6 +6 @@
-Release: 20221001
+Release: 20221008
diff '--unified=0' -r ncurses-6.3-20221001/package/ncurses.spec ncurses-6.3-20221008/package/ncurses.spec
--- ncurses-6.3-20221001/package/ncurses.spec	2022-10-01 05:34:57.000000000 -0400
+++ ncurses-6.3-20221008/package/ncurses.spec	2022-10-08 06:25:44.000000000 -0400
@@ -4 +4 @@
-Release: 20221001
+Release: 20221008
diff '--unified=0' -r ncurses-6.3-20221001/package/ncursest.spec ncurses-6.3-20221008/package/ncursest.spec
--- ncurses-6.3-20221001/package/ncursest.spec	2022-10-01 05:34:57.000000000 -0400
+++ ncurses-6.3-20221008/package/ncursest.spec	2022-10-08 06:25:44.000000000 -0400
@@ -4 +4 @@
-Release: 20221001
+Release: 20221008
diff '--unified=0' -r ncurses-6.3-20221001/VERSION ncurses-6.3-20221008/VERSION
--- ncurses-6.3-20221001/VERSION	2022-10-01 05:34:57.000000000 -0400
+++ ncurses-6.3-20221008/VERSION	2022-10-08 06:25:44.000000000 -0400
@@ -1 +1 @@
-5:0:10	6.3	20221001
+5:0:10	6.3	20221008
