diff --git a/install.sh b/install-sh
similarity index 100%
rename from install.sh
rename to install-sh
diff --git a/package/debian/cdialog-dev.install b/package/debian/cdialog-dev.install
index a186c7f..ffbd246 100755
--- a/package/debian/cdialog-dev.install
+++ b/package/debian/cdialog-dev.install
@@ -1,7 +1,8 @@
 #! /usr/bin/dh-exec
 
-/usr/bin/*-config       /usr/bin
-/usr/include/*.h        /usr/include
-/usr/include/*/*.h      /usr/include/cdialog
-*.so                    /lib/${DEB_HOST_MULTIARCH}
-/usr/share/man/man3/*.3 /usr/share/man/man3
+/usr/bin/*-config               /usr/bin
+/usr/include/*.h                /usr/include
+/usr/include/*/*.h              /usr/include/cdialog
+*.so                            /lib/${DEB_HOST_MULTIARCH}
+*.so.[1-9].[0-9]                /lib/${DEB_HOST_MULTIARCH}
+/usr/share/man/man3/*.3         /usr/share/man/man3
diff --git a/package/debian/cdialog.links b/package/debian/cdialog-dev.links
similarity index 100%
rename from package/debian/cdialog.links
rename to package/debian/cdialog-dev.links
diff --git a/package/debian/cdialog.install b/package/debian/cdialog.install
index ebceeb9..739d761 100755
--- a/package/debian/cdialog.install
+++ b/package/debian/cdialog.install
@@ -1,6 +1,7 @@
 #! /usr/bin/dh-exec
 
-/usr/bin/cdialog                       /usr/bin
-/usr/lib/*/*.so.*                      /lib/${DEB_HOST_MULTIARCH}
-/usr/share/locale/*/*/cdialog.mo
-/usr/share/man/man1/*.1                /usr/share/man/man1
+/usr/bin/*dialog                /usr/bin
+*.so.[1-9][0-9].[0-9].[0-9]     /lib/${DEB_HOST_MULTIARCH}
+/usr/share/man/man1/*.1         /usr/share/man/man1
+/usr/share/locale/*             /usr/share/locale
+
diff --git a/package/debian/cdialog.lintian-overrides b/package/debian/cdialog.lintian-overrides
index 7ba1edc..6d0a4a4 100644
--- a/package/debian/cdialog.lintian-overrides
+++ b/package/debian/cdialog.lintian-overrides
@@ -1,5 +1,5 @@
 # the source filename was misspelled in the original version
-cdialog: spelling-error-in-binary usr/bin/cdialog guage gauge
+# cdialog: spelling-error-in-binary usr/bin/cdialog guage gauge
 
 # the soname name is for the shared library used by cdialog-dev
 cdialog: package-name-doesnt-match-sonames libcdialog15.0.0
diff --git a/headers.sh b/headers-sh.in
similarity index 75%
rename from headers.sh
rename to headers-sh.in
index ec2839a..930f420 100755
--- a/headers.sh
+++ b/headers-sh.in
@@ -1,5 +1,5 @@
 #! /bin/sh
-# $Id: case47.pat,v 1.1 2023/03/01 12:37:59 tom Exp $
+# $Id: case47.pat,v 1.1 2023/03/01 12:37:59 tom Exp $
 ##############################################################################
 # Copyright (c) 2004,2007 Thomas E. Dickey                                   #
 #                                                                            #
@@ -40,17 +40,27 @@
 #	$3 is the source directory
 #	$4 is the file to install, editing source/target/etc.
 
-PACKAGE=DIALOG
-PKGNAME=DLG
-CONFIGH=dlg_config.h
+PACKAGE=@PACKAGE@
+PKGNAME=@PACKAGE_PREFIX@
+CONFIGH=@PACKAGE_CONFIG@
 
 TMPSED=headers.sed
 
+DIGIT=0123456789
+alpha=abcdefghijklmnopqrstuvwxyz
+ALPHA=ABCDEFGHIJKLMNOPQRSTUVWXYZ
+
+alnum=_${DIGIT}${alpha}
+ALNUM=_${DIGIT}${ALPHA}
+MIXED=_${DIGIT}${ALPHA}${alpha}
+
 if test $# = 2 ; then
 	rm -f $TMPSED
 	DST=$1
 	REF=$2
 	LEAF=`basename $DST`
+
+	# map the include-directory, if needed, to the subdirectory
 	case $DST in
 	/*/include/$LEAF)
 		END=`basename $DST`
@@ -64,8 +74,41 @@ if test $# = 2 ; then
 		echo "" >> $TMPSED
 		;;
 	esac
+
+	# cannot do _this_ in -e options:
+	cat >headers.tmp <<EOF
+s/^#[^ ][^ ]*//
+s/[^'$MIXED']/ /g
+s/[ 	][ 	]*/ /g
+s/^ //
+s/ $//
+:split
+	h
+	s/ .*//
+	p
+	t next
+	b done
+:next
+	x
+	s/^[^ ][^ ]* //
+	t split
+:done
+EOF
+	# pick up autoconf-style symbols used in the application's headers
+	for i in $REF/*.h
+	do
+		sed -e 's/^[ 	][ 	]*#[ 	][ 	]*/#/' $i \
+		| egrep '^#(if|ifdef|ifndef|elif)' \
+		| sed	-f headers.tmp \
+		| sort -u \
+		| egrep '^(HAVE_|NEED_|NO_|ENABLE_|DISABLE_)' \
+		| sed	-e 's%^\(.*\)%s/\\<\1\\>/'${PKGNAME}'_\1/g%' >>$TMPSED
+	done
+	rm -f headers.tmp
+
+	# pick up autoconf-defined symbols in the config.h file
 	for name in `
-	egrep '^#define[ 	][ 	]*[_ABCDEFGHIJKLMNOPQRSTUVWXYZ]' $REF/$CONFIGH \
+	egrep '^#define[ 	][ 	]*['$ALNUM']' $REF/$CONFIGH \
 		| sed	-e 's/^#define[ 	][ 	]*//' \
 			-e 's/[ 	].*//' \
 		| egrep -v "^${PACKAGE}_" \
@@ -74,6 +117,10 @@ if test $# = 2 ; then
 	do
 		echo "s/\\<$name\\>/${PKGNAME}_$name/g" >>$TMPSED
 	done
+
+	# reduce the count if possible, since some old sed's limit is 100 lines
+	sort -u $TMPSED >headers.tmp
+	mv headers.tmp $TMPSED
 else
 	PRG=""
 	while test $# != 3
diff --git a/samples/inputbox6 b/samples/inputbox6-utf8
similarity index 89%
rename from samples/inputbox6
rename to samples/inputbox6-utf8
index 4c09317..a78232e 100755
--- a/samples/inputbox6
+++ b/samples/inputbox6-utf8
@@ -1,5 +1,7 @@
 #!/bin/sh
-DIALOG=${DIALOG=dialog}
+# $Id: case47.pat,v 1.1 2023/03/01 12:37:59 tom Exp $
+: ${DIALOG=dialog}
+
 tempfile=`tempfile 2>/dev/null` || tempfile=/tmp/test$$
 trap "rm -f $tempfile" 0 1 2 5 15
 
diff --git a/po/Makefile.inn b/po/makefile.inn
similarity index 95%
rename from po/Makefile.inn
rename to po/makefile.inn
index 4cd91d8..59b5097 100644
--- a/po/Makefile.inn
+++ b/po/makefile.inn
@@ -47,7 +47,7 @@ COMPILE = $(CC) -c $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) $(XCFLAGS)
 SOURCES = cat-id-tbl.c
 POFILES = @POFILES@
 GMOFILES = @GMOFILES@
-DISTFILES = ChangeLog Makefile.inn POTFILES.in $(PACKAGE).pot \
+DISTFILES = ChangeLog makefile.inn POTFILES.in $(PACKAGE).pot \
 stamp-cat-id $(POFILES) $(GMOFILES) $(SOURCES)
 
 POTFILES = \
@@ -158,8 +158,8 @@ install-data-yes: all
 	  else \
 	    $(SHELL) $(top_srcdir)/mkdirs.sh $(DESTDIR)$(gettextsrcdir); \
 	  fi; \
-	  $(INSTALL_DATA) $(srcdir)/Makefile.inn \
-			  $(DESTDIR)$(gettextsrcdir)/Makefile.inn; \
+	  $(INSTALL_DATA) $(srcdir)/makefile.inn \
+			  $(DESTDIR)$(gettextsrcdir)/makefile.inn; \
 	else \
 	  : ; \
 	fi
@@ -177,7 +177,7 @@ uninstall:
 	  rm -f $(DESTDIR)$(gnulocaledir)/$$lang/LC_MESSAGES/$(PACKAGE)$(INSTOBJEXT); \
 	  rm -f $(DESTDIR)$(gnulocaledir)/$$lang/LC_MESSAGES/$(PACKAGE)$(INSTOBJEXT).m; \
 	done
-	rm -f $(gettextsrcdir)/po-Makefile.inn
+	rm -f $(gettextsrcdir)/po-makefile.inn
 
 check: all
 
@@ -193,7 +193,7 @@ clean: mostlyclean
 
 distclean: clean
 	rm -f cat-id-tbl.c stamp-cat-id *.gmo
-	rm -f Makefile Makefile.in POTFILES *.mo *.msg *.cat *.cat.m
+	rm -f makefile makefile.in POTFILES *.mo *.msg *.cat *.cat.m
 
 maintainer-clean: distclean
 	@echo "This command is intended for maintainers to use;"
@@ -208,7 +208,7 @@ dist distdir: update-po $(DISTFILES)
 	    || cp -p $(srcdir)/$$file $(distdir); \
 	done
 
-update-po: Makefile
+update-po: makefile
 	$(MAKE) $(PACKAGE).pot
 	PATH=`pwd`/../src:$$PATH; \
 	cd $(srcdir); \
@@ -240,7 +240,7 @@ POTFILES: POTFILES.in
 	    && chmod a-w $@-t \
 	    && mv $@-t $@ )
 
-Makefile: Makefile.inn ../config.status POTFILES
+makefile: makefile.inn ../config.status POTFILES
 	cd .. \
 	  && CONFIG_FILES=$(subdir)/$@.in:$(subdir)/$@.inn CONFIG_HEADERS= \
 	       $(SHELL) ./config.status
