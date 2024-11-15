/******************************************************************************
 * Copyright 1994-2023,2024 by Thomas E. Dickey                               *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL   *
 * THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
 *                                                                            *
 * Except as contained in this notice, the name(s) of the above copyright     *
 * holders shall not be used in advertising or otherwise to promote the sale, *
 * use or other dealings in this Software without prior written               *
 * authorization.                                                             *
 ******************************************************************************/

#ifndef	NO_IDENT
static const char *Id = "$Id: diffstat.c,v 1.67 2024/11/11 13:01:06 tom Exp $";
#endif

/*
 * Title:	diffstat.c
 * Author:	T.E.Dickey
 * Created:	02 Feb 1992
 * Modified:
 *		11 Nov 2024, add decompression for zstd
 *		28 Jan 2024, fixes for stricter gcc warnings
 *		01 Mar 2023, ignore ".git" directories, etc., in -S/-D options.
 *		09 Oct 2022, trim trailing '/' from directories.  Correct case
 *			     where there is no unified-context.
 *		12 Jan 2021, check for git --binary diffs.
 *		29 Nov 2019, eliminate fixed buffer when decoding range.
 *		28 Nov 2019, use locale in computing filename column-width.
 *			     improve parsing for git diffs.
 *			     use terminal-width as default for -w to tty.
 *			     minor fix in do_merging (Miloslaw Smyk).
 *		27 Nov 2019, improve relative-pathname matching in count_lines()
 *			     add a parsing-case for svn diff.
 *			     quote filenames in -t/-T output.
 *		24 Nov 2019, fix cppcheck-warnings about sscanf.
 *		14 Aug 2018, revise -S/-D option to improve count of unmodified
 *			     files.
 *		14 Jan 2016, extend -S option to count unmodified files.
 *			     add -T option to show values with histogram
 *		06 Jul 2015, handle double-quotes, e.g., from diffutils 3.3
 *			     when filenames have embedded spaces.
 *		05 Jun 2014, add -E option to filter colordiff output.
 *		28 Oct 2013, portability improvements for MinGW.
 *		15 Apr 2013, modify to accommodate output of "diff -q", which
 *			     tells only if the files are different.  Work
 *			     around the equivalent ambiguous message introduced
 *			     in diffutils 2.8.4 and finally removed for 3.0
 *		11 Feb 2013, add -K option.  Use strtol() to provide error
 *			     checking of optarg values.
 *		10 Feb 2013, document -b, -C, -s option in usage (patch by
 *			     Tim Waugh, Red Hat #852770).  Improve pathname
 *			     merging.
 *		02 Jun 2012, fix for svn diff with spaces in path (patch by
 *			     Stuart Prescott, Debian #675465).
 *		03 Jan 2012, Correct case for "xz" suffix in is_compressed()
 *			     (patch from Frederic Culot in FreeBSD ports).  Add
 *			     "-R" option.  Improve dequoting of filenames in
 *			     headers.
 *		10 Oct 2010, correct display of new files when -S/-D options
 *			     are used.  Remove the temporary directory on
 *			     error, introduced in 1.48+ (patch by Solar
 *			     Designer).
 *		19 Jul 2010, add missing "break" statement which left "-c"
 *			     option falling-through into "-C".
 *		16 Jul 2010, configure "xz" path explicitly, in case lzcat
 *			     does not support xz format.  Add "-s" (summary)
 *			     and "-C" (color) options.
 *		15 Jul 2010, fix strict gcc warnings, e.g., using const.
 *		10 Jan 2010, improve a case where filenames have embedded blanks
 *			     (patch by Reinier Post).
 *		07 Nov 2009, correct suffix-check for ".xz" files as
 *			     command-line parameters rather than as piped
 *			     input (report by Moritz Barsnick).
 *		06 Oct 2009, fixes to build/run with MSYS or MinGW.  use
 *			     $TMPDIR for path of temporary file used in
 *			     decompression.  correct else-condition for
 *			     detecting compression type (patch by Zach Hirsch).
 *		31 Aug 2009, improve lzma support, add support for xz (patch by
 *			     Eric Blake).  Add special case for no-newline
 *			     message from some diff's (Ubuntu #269895).
 *			     Improve configure check for getopt().
 *		11 Aug 2009, Add logic to check standard input, decompress if
 *			     possible.  Add -N option, to truncate long names.
 *			     Add pack/pcat as a compression type.
 *			     Add lzma/lzcat as a compression type.
 *			     Allow overriding program paths with environment.
 *		10 Aug 2009, modify to work with Perforce-style diffs (patch
 *			     by Ed Schouten).
 *		29 Mar 2009, modify to work with patch ".rej" files, which have
 *			     no filename header (use the name of the ".rej"
 *			     file if it is available).
 *		29 Sep 2008, fix typo in usage message.
 *		06 Aug 2008, add "-m", "-S" and "-D" options.
 *		05 Aug 2008, add "-q" option to suppress 0-files-changed
 *			     message (patch by Greg Norris).
 *		04 Sep 2007, add "-b" option to suppress binary-files (patch
 *			     by Greg Norris).
 *		26 Aug 2007, add "-d" option to show debugging traces, rather
 *			     than by defining DEBUG.  Add check after
 *			     unified-diff chunk to avoid adding non-diff text
 *			     (report by Adrian Bunk).  Quote pathname passed
 *			     in command to gzip/uncompress.  Add a check for
 *			     default-diff output without the "diff" command
 *			     supplied to provide filename, mark as "unknown".
 *		16 Jul 2006, fix to avoid modifying which is being used by
 *			     tsearch() for ordering the binary tree (report by
 *			     Adrian Bunk).
 *		02 Jul 2006, do not ignore pathnames in /tmp/, since some tools
 *			     create usable pathnames for both old/new files
 *			     there (Debian #376086).  Correct ifdef for
 *			     fgetc_unlocked().  Add configure check for
 *			     compress, gzip and bzip2 programs that may be used
 *			     to decompress files.
 *		24 Aug 2005, update usage message for -l, -r changes.
 *		15 Aug 2005, apply PLURAL() to num_files (Jean Delvare).
 *			     add -l option (request by Michael Burian).
 *			     Use fgetc_locked() if available.
 *		14 Aug 2005, add -r2 option (rounding with adjustment to ensure
 *			     that nonzero values always display a histogram
 *			     bar), adapted from patch by Jean Delvare.  Extend
 *			     the -f option (2=filled, 4=verbose).
 *		12 Aug 2005, modify to use tsearch() for sorted lists.
 *		11 Aug 2005, minor fixes to scaling of modified lines.  Add
 *			     -r (round) option.
 *		05 Aug 2005, add -t (table) option.
 *		10 Apr 2005, change order of merging and prefix-stripping so
 *			     stripping all prefixes, e.g., with -p9, will be
 *			     sorted as expected (Patch by Jean Delvare
 *			     <khali@linux-fr.org>).
 *		10 Jan 2005, add support for '--help' and '--version' (Patch
 *			     by Eric Blake <ebb9@byu.net>.)
 *		16 Dec 2004, fix a different case for data beginning with "--"
 *			     which was treated as a header line.
 *		14 Dec 2004, Fix allocation problems.  Open files in binary
 *			     mode for reading.  Getopt returns -1, not
 *			     necessarily EOF.  Add const where useful.  Use
 *			     NO_IDENT where necessary.  malloc() comes from
 *			     <stdlib.h> in standard systems (Patch by Eric
 *			     Blake <ebb9@byu.net>.)
 *		08 Nov 2004, minor fix for resync of unified diffs checks for
 *			     range (line beginning with '@' without header
 *			     lines (successive lines beginning with "---" and
 *			     "+++").  Fix a few problems reported by valgrind.
 *		09 Nov 2003, modify check for lines beginning with '-' or '+'
 *			     to treat only "---" in old-style diffs as a
 *			     special case.
 *		14 Feb 2003, modify check for filenames to allow for some cases
 *			     of incomplete dates (the reported example omitted
 *			     the day of the month).  Correct a typo in usage().
 *			     Add -e, -h, -o options.
 *		04 Jan 2003, improve tracking of chunks in unified diff, in
 *			     case the original files contained a '+' or '-' in
 *			     the first column (Debian #155000).  Add -v option
 *			     (Debian #170947).  Modify to allocate buffers big
 *			     enough for long input lines.  Do additional
 *			     merging to handle unusual Index/diff constructs in
 *			     recent makepatch script.
 *		20 Aug 2002, add -u option to tell diffstat to preserve the
 *			     order of filenames as given rather than sort them
 *			     (request by H Peter Anvin <hpa@zytor.com>).  Add
 *			     -k option for completeness.
 *		09 Aug 2002, allow either '/' or '-' as delimiters in dates,
 *			     to accommodate diffutils 2.8 (report by Rik van
 *			     Riel <riel@conectiva.com.br>).
 *		10 Oct 2001, add bzip2 (.bz2) suffix as suggested by
 *			     Gregory T Norris <haphazard@socket.net> in Debian
 *			     bug report #82969).
 *			     add check for diff from RCS archive where the
 *			     "diff" lines do not reference a filename.
 *		29 Mar 2000, add -c option.  Check for compressed input, read
 *			     via pipe.  Change to ANSI C.  Adapted change from
 *			     Troy Engel to add option that displays a number
 *			     only, rather than a histogram.
 *		17 May 1998, handle Debian diff files, which do not contain
 *			     dates on the header lines.
 *		16 Jan 1998, accommodate patches w/o tabs in header lines (e.g.,
 *			     from cut/paste).  Strip suffixes such as ".orig".
 *		24 Mar 1996, corrected -p0 logic, more fixes in do_merging.
 *		16 Mar 1996, corrected state-change for "Binary".  Added -p
 *			     option.
 *		17 Dec 1995, corrected matching algorithm in 'do_merging()'
 *		11 Dec 1995, mods to accommodate diffs against /dev/null or
 *			     /tmp/XXX (tempfiles).
 *		06 May 1995, limit scaling -- only shrink-to-fit.
 *		29 Apr 1995, recognize 'rcsdiff -u' format.
 *		26 Dec 1994, strip common pathname-prefix.
 *		13 Nov 1994, added '-n' option.  Corrected logic of 'match'.
 *		17 Jun 1994, ifdef-<string.h>
 *		12 Jun 1994, recognize unified diff, and output of makepatch.
 *		04 Oct 1993, merge multiple diff-files, busy message when the
 *			     output is piped to a file.
 *
 * Function:	this program reads the output of 'diff' and displays a histogram
 *		of the insertions/deletions/modifications per-file.
 */

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#if defined(WIN32) && !defined(HAVE_CONFIG_H)
#define HAVE_STDLIB_H
#define HAVE_STRING_H
#define HAVE_MALLOC_H
#define HAVE_GETOPT_H
#endif

#include <stdio.h>
#include <ctype.h>

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#define strchr index
#define strrchr rindex
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
extern int atoi(const char *);
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
extern int isatty(int);
#endif

#ifdef HAVE_OPENDIR
#include <dirent.h>
#endif

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#if defined(HAVE_SEARCH_H) && defined(HAVE_TSEARCH)
#include <search.h>
#else
#undef HAVE_TSEARCH
#endif

#ifdef HAVE_MBSTOWCWIDTH
#include <locale.h>
#include <wchar.h>
#endif

#ifdef HAVE_GETC_UNLOCKED
#define MY_GETC getc_unlocked
#else
#define MY_GETC getc
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#elif !defined(HAVE_GETOPT_HEADER)
extern int getopt(int, char *const *, const char *);
extern char *optarg;
extern int optind;
#endif

#include <sys/types.h>
#include <sys/stat.h>

#if defined(HAVE_TERMIOS_H) && defined(HAVE_TCGETATTR)
#ifdef HAVE_IOCTL_H
#include <ioctl.h>
#else
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#endif
#if !defined(sun) || !defined(NL0)
#include <termios.h>
#endif
#endif /* HAVE_TERMIOS_H */

#if defined(HAVE_POPEN) && !defined(HAVE_POPEN_PROTOTYPE)
extern FILE *popen(const char *, const char *);
extern int pclose(FILE *);
#endif

#if !defined(EXIT_SUCCESS)
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#endif

#ifndef BZCAT_PATH
#define BZCAT_PATH ""
#endif

#ifndef BZIP2_PATH
#define BZIP2_PATH ""
#endif

#ifndef COMPRESS_PATH
#define COMPRESS_PATH ""
#endif

#ifndef GZIP_PATH
#define GZIP_PATH ""
#endif

#ifndef LZCAT_PATH
#define LZCAT_PATH ""
#endif

#ifndef PCAT_PATH
#define PCAT_PATH ""
#endif

#ifndef UNCOMPRESS_PATH
#define UNCOMPRESS_PATH ""
#endif

#ifndef XZ_PATH
#define XZ_PATH ""
#endif

#ifndef ZCAT_PATH
#define ZCAT_PATH ""
#endif

#ifndef ZSTD_PATH
#define ZSTD_PATH ""
#endif

/******************************************************************************/

#if defined(__MINGW32__) || defined(WIN32)
#define MKDIR(name,mode) mkdir(name)
#else
#define MKDIR(name,mode) mkdir(name,mode)
#endif

#if defined(WIN32) && !defined(__MINGW32__)
#define PATHSEP '\\'
#else
#define PATHSEP '/'
#endif

#define BACKSL  '\\'
#define LPAREN  '('
#define RPAREN  ')'
#define DQUOTE  '"'
#define SQUOTE  '\''
#define ESCAPE  '\033'
#define EOS     '\0'
#define TAB     '\t'
#define BLANK   ' '
#define DEL     '\177'

#define UC(c)   ((unsigned char)(c))

#define isoctal(c) (((c) >= '0') && ((c) <= '7'))

#ifndef OPT_TRACE
#define OPT_TRACE 1
#endif

#if OPT_TRACE
#define TRACE(p) if (trace_opt) printf p
#else
#define TRACE(p)		/*nothing */
#endif

#define NonNull(s)   (((s) != NULL ? (s) : "<null>"))

#define contain_any(s,reject) (strcspn(s,reject) != strlen(s))
#define maximum(a,b) ((a) < (b) ? (b) : (a))

#define HAVE_NOTHING 0
#define HAVE_GENERIC 1		/* e.g., "Index: foo" w/o pathname */
#define HAVE_PATH    2		/* reference-file from "diff dirname/foo" */
#define HAVE_PATH2   4		/* comparison-file from "diff dirname/foo" */

#define FMT_CONCISE  0
#define FMT_NORMAL   1
#define FMT_FILLED   2
#define FMT_VERBOSE  4

typedef enum comment {
    Normal, Only, OnlyLeft, OnlyRight, Binary, Differs, Either
} Comment;

#define MARKS 4			/* each of +, - and ! */

typedef enum {
    cInsert = 0,
    cDelete,
    cModify,
    cEquals
} Change;

#define InsOf(p) (p)->count[cInsert]	/* "+" count inserted lines */
#define DelOf(p) (p)->count[cDelete]	/* "-" count deleted lines */
#define ModOf(p) (p)->count[cModify]	/* "!" count modified lines */
#define EqlOf(p) (p)->count[cEquals]	/* "=" count unmodified lines */

#define TotalOf(p) (InsOf(p) + DelOf(p) + ModOf(p) + EqlOf(p))
#define for_each_mark(n) for (n = 0; n < num_marks; ++n)

typedef struct _data {
    struct _data *link;
    char *original;		/* the original filename */
    char *modified;		/* the modified filename */
    int copy;			/* true if filename is const-literal */
    int base;			/* beginning of name if -p option used */
    Comment cmt;
    int pending;
    long chunks;		/* total number of chunks */
    long chunk[MARKS];		/* counts for the current chunk */
    long count[MARKS];		/* counts for the file */
} DATA;

typedef enum {
    dcNone = 0,
    dcBzip,
    dcCompress,
    dcGzip,
    dcLzma,
    dcPack,
    dcXz,
    dcZstd,
    dcEmpty
} Decompress;

static const char marks[MARKS + 1] = "+-!=";
static const int colors[MARKS + 1] =
{2, 1, 6, 4};

static DATA *all_data;
static char *S_option = 0;
static char *D_option = 0;
static const char *comment_opt = "";
static char *path_opt = 0;
static int count_files;		/* true if we count added/deleted files */
static int format_opt = FMT_NORMAL;
static int max_name_wide;	/* maximum amount reserved for filenames */
static int max_width = 80;	/* the specified width-limit */
static int merge_names = 1;	/* true if we merge similar filenames */
static int merge_opt = 0;	/* true if we merge ins/del as modified */
static int min_name_wide;	/* minimum amount reserved for filenames */
static int names_only;		/* true if we list filenames only */
static int num_marks = 3;	/* 3 or 4, according to "-P" option */
static int path_dest;		/* true if path_opt is destination (patched) */
static int plot_width;		/* the amount left over for histogram */
static int prefix_opt = -1;	/* if positive, controls stripping of PATHSEP */
static int quiet = 0;		/* -q option */
static int reverse_opt;		/* true if results are reversed */
static int round_opt = 0;	/* if nonzero, round data for histogram */
static int show_colors;		/* true if showing SGR colors */
static int show_progress;	/* if not writing to tty, show progress */
static int sort_names = 1;	/* true if we sort filenames */
static int summary_only = 0;	/* true if only summary line is shown */
static int suppress_binary = 0;	/* -b option */
static int trim_escapes = 0;	/* -E option */
static int table_opt = 0;	/* if 1/2, write table instead/also plot */
static int trace_opt = 0;	/* if nonzero, write debugging information */
static int unchanged = 0;	/* special-case for -S vs modified-files */
static int verbose = 0;		/* -v option */
static long plot_scale;		/* the effective scale (1:maximum) */

#ifdef HAVE_TSEARCH
static int use_tsearch;
static void *sorted_data;
#endif

static int number_len = 5;
static int prefix_len = -1;

/******************************************************************************/

#ifdef GCC_NORETURN
static void failed(const char *) GCC_NORETURN;
#endif

static void
failed(const char *s)
{
    perror(s);
    exit(EXIT_FAILURE);
}

/* malloc wrapper that never returns NULL */
static void *
xmalloc(size_t s)
{
    void *p;
    if ((p = malloc(s)) == NULL)
	failed("malloc");
    return p;
}

static int
do_stat(const char *name, struct stat *sb)
{
    int rc;
    if (name != 0) {
#ifdef HAVE_LSTAT
	rc = lstat(name, sb);
#else
	rc = stat(name, sb);
#endif
    } else {
	rc = -1;
    }
    return rc;
}

static mode_t
get_stat(const char *name)
{
    struct stat sb;
    int rc = do_stat(name, &sb);
    return ((rc == 0) ? (sb.st_mode & S_IFMT) : 0);
}

static int
is_dir(const char *name)
{
    return get_stat(name) == S_IFDIR;
}

static int
is_file(const char *name)
{
    return get_stat(name) == S_IFREG;
}

static int
same_file(const char *source, const char *target)
{
    int rc = 0;
    struct stat ssb;
    struct stat dsb;

    if (do_stat(source, &ssb) == 0 && S_ISREG(ssb.st_mode)
	&& do_stat(target, &dsb) == 0 && S_ISREG(dsb.st_mode)
	&& ssb.st_size == dsb.st_size) {
	FILE *ip = fopen(source, "r");
	if (ip != 0) {
	    FILE *op = fopen(target, "r");
	    if (op != 0) {
		int a = EOF;
		int b = EOF;
		rc = 1;
		while (1) {
		    a = fgetc(ip);
		    b = fgetc(op);
		    if (a != b) {
			rc = 0;
			break;
		    }
		    if (a == EOF) {
			break;
		    }
		}
		if (a != b) {
		    rc = 0;
		}
		fclose(op);
	    }
	    fclose(ip);
	}
    }
    return rc;
}

static void
blip(int c)
{
    if (show_progress) {
	(void) fflush(stdout);
	(void) fputc(c, stderr);
	(void) fflush(stderr);
    }
}

#ifdef HAVE_STRDUP
#define new_string(s) strdup(s)
#else
static char *
new_string(const char *s)
{
    return strcpy((char *) xmalloc((size_t) (strlen(s) + 1)), s);
}
#endif

static int
compare_data(const void *a, const void *b)
{
    const DATA *p = (const DATA *) a;
    const DATA *q = (const DATA *) b;
    return ((p != NULL)
	    ? ((q != NULL)
	       ? strcmp(p->modified + p->base, q->modified + q->base)
	       : 1)
	    : -1);
}

static void
init_data(DATA * data, const char *original, const char *modified, int copy, int base)
{
    memset(data, 0, sizeof(*data));
    data->original = (char *) original;
    data->modified = (char *) modified;
    data->copy = copy;
    data->base = base;
    data->cmt = Normal;
}

static DATA *
new_data(const char *original, const char *modified, int base)
{
    DATA *r = (DATA *) xmalloc(sizeof(DATA));

    if (original == NULL) {
	TRACE(("new_data: no original for %s\n", modified));
	original = modified;	/* FIXME */
    }
    init_data(r, new_string(original), new_string(modified), 0, base);

    return r;
}

#ifdef HAVE_TSEARCH
static DATA *
add_tsearch_data(const char *original, const char *modified, int base)
{
    DATA find;
    DATA *result;
    void *pp;

    init_data(&find, NULL, modified, 1, base);
    if ((pp = tfind(&find, &sorted_data, compare_data)) != 0) {
	result = *(DATA **) pp;
	return result;
    }
    result = new_data(original, modified, base);
    (void) tsearch(result, &sorted_data, compare_data);
    result->link = all_data;
    all_data = result;

    return result;
}
#endif

static int
count_prefix(const char *name)
{
    int count = 0;
    const char *s;
    while ((s = strchr(name, PATHSEP)) != 0) {
	name = s + 1;
	++count;
    }
    return count;
}

static const char *
skip_prefix(const char *name, int prefix, int *base)
{
    if (prefix >= 0) {
	int n;
	*base = 0;

	for (n = prefix; n > 0; n--) {
	    const char *s = strchr(name + *base, PATHSEP);
	    if (s == 0 || *++s == EOS) {
		name = s;
		break;
	    }
	    *base = (int) (s - name);
	}
	TRACE(("** base set to %d\n", *base));
    }
    return name;
}

static DATA *
find_data(const char *original, const char *modified)
{
    DATA *r;
    int base = 0;

    TRACE(("** find_data(%s => %s)\n", NonNull(original), NonNull(modified)));

    /* Compute the base offset if the prefix option is used */
    if (prefix_opt >= 0) {
	(void) skip_prefix(modified, prefix_opt, &base);
    }

    /* Insert into sorted list (usually sorted).  If we are not sorting or
     * merging names, we fall off the end and link the new entry to the end of
     * the list.  If the prefix option is used, the prefix is ignored by the
     * merge and sort operations.
     *
     * If we have tsearch(), we will maintain the sorted list using it and
     * tfind().
     */
#ifdef HAVE_TSEARCH
    if (use_tsearch) {
	r = add_tsearch_data(original, modified, base);
    } else
#endif
    {
	DATA *p;
	DATA find;
	DATA *q;

	init_data(&find, original, modified, 1, base);
	for (p = all_data, q = 0; p != 0; q = p, p = p->link) {
	    int cmp = compare_data(p, &find);
	    if (merge_names && (cmp == 0))
		return p;
	    if (sort_names && (cmp > 0))
		break;
	}
	r = new_data(original, modified, base);
	if (q != 0)
	    q->link = r;
	else
	    all_data = r;

	r->link = p;
    }

    return r;
}

/*
 * Remove a unneeded data item from the linked list.  Free the name as well.
 */
static int
delink(DATA * data)
{
    DATA *p, *q;

    TRACE(("** delink '%s'\n", data->modified));

#ifdef HAVE_TSEARCH
    if (use_tsearch) {
	if (tdelete(data, &sorted_data, compare_data) == 0)
	    return 0;
    }
#endif
    for (p = all_data, q = 0; p != 0; q = p, p = p->link) {
	if (p == data) {
	    if (q != 0)
		q->link = p->link;
	    else
		all_data = p->link;
	    if (!p->copy) {
		free(p->original);
		free(p->modified);
	    }
	    free(p);
	    return 1;
	}
    }
    return 0;
}

/*
 * Compare string 's' against a constant, returning either a pointer just
 * past the matched part of 's' if it matches exactly, or null if a mismatch
 * was found.
 */
static char *
match(char *s, const char *p)
{
    int ok = 0;

    while (*s != EOS) {
	if (*p == EOS) {
	    ok = 1;
	    break;
	}
	if (*s++ != *p++)
	    break;
	if (*s == EOS && *p == EOS) {
	    ok = 1;
	    break;
	}
    }
    return ok ? s : 0;
}

static int
version_num(const char *s)
{
    int main_ver, sub_ver;
    char temp[2];
    return (sscanf(s, "%d.%d%c", &main_ver, &sub_ver, temp) == 2);
}

/*
 * Check for a range of line-numbers, used in editing scripts.
 */
static int
edit_range(const char *s)
{
    int first, last;
    char temp[2];
    return (sscanf(s, "%d,%d%c", &first, &last, temp) == 2)
	|| (sscanf(s, "%d%c", &first, temp) == 1);
}

/*
 * Decode a range for default diff.
 */
static int
decode_default(char *s,
	       long *first, long *first_size,
	       long *second, long *second_size)
{
    int rc = 0;
    char *next;

    if (isdigit(UC(*s))) {
	*first_size = 1;
	*second_size = 1;

	*first = strtol(s, &next, 10);
	if (next != 0 && next != s) {
	    if (*next == ',') {
		s = ++next;
		*first_size = strtol(s, &next, 10) + 1 - *first;
	    }
	}
	if (next != 0 && next != s) {
	    switch (*next++) {
	    case 'a':
	    case 'c':
	    case 'd':
		s = next;
		*second = strtol(s, &next, 10);
		if (next != 0 && next != s) {
		    if (*next == ',') {
			s = ++next;
			*second_size = strtol(s, &next, 10) + 1 - *second;
		    }
		}
		if (next != 0 && next != s && *next == EOS)
		    rc = 1;
		break;
	    }
	}
    }
    return rc;
}

/*
 * Decode a range for unified diff.  Oddly, the comments in diffutils code
 * claim that both numbers are line-numbers.  However, inspection of the output
 * shows that the numbers are a line-number followed by a count.
 */
static char *
decode_range(char *s, int *first, int *second)
{
    if (isdigit(UC(*s))) {
	int count = 0;
	int value[2];

	value[0] = 0;
	value[1] = 0;
	while (*s != EOS) {
	    int ch = UC(*s);
	    if (isdigit(ch)) {
		value[count] = (10 * value[count]) + (ch - '0');
	    } else if (ch == ',') {
		if (++count > 1) {
		    s = NULL;
		    break;
		}
		value[count] = 0;
	    } else {
		break;
	    }
	    ++s;
	}
	if (s != NULL) {
	    *first = value[0];
	    if (count == 0) {
		*second = 1;
	    } else {
		*second = value[1];
	    }
#if OPT_TRACE
	    if (*second) {
		TRACE(("** decode_range [%d..%d]\n",
		       *first, *first + *second - 1));
	    } else {
		TRACE(("** decode_range [%d]\n",
		       *first));
	    }
#endif
	}
    }
    return s;
}

static int
HadDiffs(const DATA * data)
{
    return InsOf(data) != 0
	|| DelOf(data) != 0
	|| ModOf(data) != 0
	|| data->cmt != Normal;
}

/*
 * If the given path is not one of the "ignore" paths, then return true.
 */
static int
can_be_merged(const char *path)
{
    int result = 0;
    if (strcmp(path, "")
	&& strcmp(path, "/dev/null"))
	result = 1;
    return result;
}

static int
is_leaf(const char *theLeaf, const char *path)
{
    const char *s;

    if (strchr(theLeaf, PATHSEP) == 0
	&& (s = strrchr(path, PATHSEP)) != 0
	&& !strcmp(++s, theLeaf))
	return 1;
    return 0;
}

static char *
trim_datapath(DATA ** datap, size_t length, int *localp)
{
    char *target = (*datap)->modified;

    (void) localp;
#ifdef HAVE_TSEARCH
    /*
     * If we are using tsearch(), make a local copy of the data
     * so we can trim it without interfering with tsearch's
     * notion of the ordering of data.  That will create some
     * spurious empty data, so we add the changed() macro in a
     * few places to skip over those.
     */
    if (use_tsearch) {
	char *trim = new_string(target);
	trim[length] = EOS;
	*datap = add_tsearch_data(NULL, trim, (*datap)->base);
	target = (*datap)->modified;
	free(trim);
	*localp = 1;
    } else
#endif
	target[length] = EOS;

    return target;
}

static size_t
compare_tails(const char *target, const char *source, int *diff)
{
    size_t len1 = strlen(target);
    size_t len2 = strlen(source);
    size_t n;
    size_t matched = 0;

    *diff = 0;
    for (n = 1; n <= len1 && n <= len2; n++) {
	if (target[len1 - n] != source[len2 - n]) {
	    *diff = (int) n;
	    break;
	}
	if (source[len2 - n] == PATHSEP) {
	    matched = n;
	}
    }
    return matched;
}

/*
 * The 'data' parameter points to the first of two markers, while
 * 'path' is the pathname from the second marker.
 *
 * On the first call for
 * a given file, the 'data' parameter stores no differences.
 */
static char *
do_merging(DATA * data, char *path, int *freed)
{
    char *target = reverse_opt ? path : data->modified;
    char *source = reverse_opt ? data->modified : path;
    char *result = source;
    int diff;

    TRACE(("** do_merging(\"%s\" -> \"%s\",\"%s\") diffs:%d\n",
	   NonNull(data->original),
	   NonNull(data->modified),
	   path, HadDiffs(data)));

    *freed = 0;
    if (!HadDiffs(data)) {

	if (is_leaf(target, source)) {
	    TRACE(("** is_leaf: \"%s\" vs \"%s\"\n", target, source));
	    if (reverse_opt) {
		TRACE((".. no action @%d\n", __LINE__));
	    } else {
		TRACE((".. will delink @%d\n", __LINE__));
		*freed = delink(data);
	    }
	} else if (can_be_merged(target)
		   && can_be_merged(source)) {
	    size_t len1 = strlen(target);
	    size_t len2 = strlen(source);
	    int local = 0;

	    /*
	     * If the source/target differ only by some suffix, e.g., ".orig"
	     * or ".bak", strip that off.  The target may may also be a
	     * temporary filename (which would not be merged since it has no
	     * apparent relationship to the current).
	     */
	    if (len1 > len2) {
		if (!strncmp(target, source, len2)) {
		    TRACE(("** trimming data \"%s\" to \"%.*s\"\n",
			   target, (int) len2, target));
		    if (reverse_opt) {
			TRACE((".. no action @%d\n", __LINE__));
		    } else {
			target = trim_datapath(&data, len2, &local);
		    }
		}
	    } else if (len1 < len2) {
		if (!strncmp(target, source, len1)) {
		    TRACE(("** trimming source \"%s\" to \"%.*s\"\n",
			   source, (int) len1, source));
		    if (reverse_opt) {
			TRACE((".. no action @%d\n", __LINE__));
		    } else {
			source[len2 = len1] = EOS;
		    }
		}
	    }

	    /*
	     * If there was no "-p" option, look for the best match by
	     * stripping prefixes from both source/target strings.
	     */
	    if (prefix_opt < 0) {
		int matched = 0;
		/*
		 * Now (whether or not we trimmed a suffix), scan back from the
		 * end of source/target strings to find if they happen to share
		 * a common ending, e.g., a/b/c versus d/b/c.  If the strings
		 * are not identical, then 'diff' will be set, but if they have
		 * a common ending then 'matched' will be set.
		 */
		diff = 0;
		matched = (int) compare_tails(target, source, &diff);

		TRACE(("** merge @%d, prefix_opt=%d matched=%d diff=%d\n",
		       __LINE__, prefix_opt, matched, diff));
		if (matched != 0 && diff) {
		    if (reverse_opt) {
			TRACE((".. no action @%d\n", __LINE__));
		    } else {
			result = source + ((int) len2 - matched + 1);
		    }
		}
	    }

	    if (!local) {
		if (reverse_opt) {
		    TRACE((".. no action @%d\n", __LINE__));
		} else {
		    TRACE((".. will delink @%d\n", __LINE__));
		    *freed = delink(data);
		}
	    }
	} else if (reverse_opt) {
	    TRACE((".. no action @%d\n", __LINE__));
	    if (can_be_merged(source)) {
		TRACE(("** merge @%d\n", __LINE__));
	    } else {
		TRACE(("** do not merge, retain @%d\n", __LINE__));
		/* must not merge, retain existing name */
		result = target;
	    }
	} else {
	    if (can_be_merged(source)) {
		TRACE(("** merge @%d\n", __LINE__));
		*freed = delink(data);
	    } else {
		TRACE(("** do not merge, retain @%d\n", __LINE__));
		/* must not merge, retain existing name */
		result = target;
	    }
	}
    } else if (reverse_opt) {
	TRACE((".. no action @%d\n", __LINE__));
	if (can_be_merged(source)) {
	    TRACE(("** merge @%d\n", __LINE__));
	    result = target;
	} else {
	    TRACE(("** do not merge, retain @%d\n", __LINE__));
	}
    } else {
	if (can_be_merged(source)) {
	    TRACE(("** %smerge @%d\n", merge_names ? "" : "do not ", __LINE__));
	    if (merge_names
		&& *target != EOS
		&& prefix_opt < 0) {
		size_t matched = compare_tails(target, source, &diff);
		if (matched && !diff)
		    result = target + (int) (strlen(target) - matched);
	    }
	} else {
	    TRACE(("** do not merge, retain @%d\n", __LINE__));
	    result = target;
	}
    }
    TRACE(("** finish do_merging ->\"%s\"\n", result));
    return result;
}

static int
begin_data(const DATA * p)
{
    TRACE(("...begin_data(\"%s\" -> \"%s\")\n",
	   NonNull(p->original),
	   NonNull(p->modified)));

    if (!can_be_merged(p->modified)
	&& strchr(p->modified, PATHSEP) != 0) {
	TRACE(("** begin_data:HAVE_PATH\n"));
	return HAVE_PATH;
    }
    TRACE(("** begin_data:HAVE_GENERIC\n"));
    return HAVE_GENERIC;
}

static char *
skip_blanks(char *s)
{
    while (isspace(UC(*s)))
	++s;
    return s;
}

/*
 * Skip a filename, which may be in quotes, to allow embedded blanks in the
 * name.
 */
static char *
skip_filename(char *s)
{
    int delim = (*s == SQUOTE) ? SQUOTE : DQUOTE;

    if ((*s == delim) && (s[1] != EOS) && (strchr) (s + 1, delim) != 0) {
	++s;
	while (*s != EOS && (*s != delim) && isprint(UC(*s))) {
	    ++s;
	}
	++s;
    } else {
	while (*s != EOS && isgraph(UC(*s))) {
	    ++s;
	}
    }
    return s;
}

static char *
skip_options(char *params)
{
    while (*params != EOS) {
	params = skip_blanks(params);
	if (*params == '-') {
	    while (isgraph(UC(*params)))
		params++;
	} else {
	    break;
	}
    }
    return skip_blanks(params);
}

/*
 * Strip single-quotes from a name (needed for recent makepatch versions).
 */
static void
dequote(char *s)
{
    size_t len = strlen(s);
    int delim = (*s == SQUOTE) ? SQUOTE : DQUOTE;

    if (*s == delim && len > 2 && s[len - 1] == delim) {
	int n;

	for (n = 0; (s[n] = s[n + 1]) != EOS; ++n) {
	    ;
	}
	s[len - 2] = EOS;
    }
}

/*
 * Allocate a fixed-buffer
 */
static void
fixed_buffer(char **buffer, size_t want)
{
    *buffer = (char *) xmalloc(want);
}

/*
 * Reallocate a fixed-buffer
 */
static void
adjust_buffer(char **buffer, size_t want)
{
    if ((*buffer = (char *) realloc(*buffer, want)) == 0)
	failed("realloc");
}

/*
 * Read until newline or end-of-file, allocating the line-buffer so it is long
 * enough for the input.
 */
static int
get_line(char **buffer, size_t *have, FILE *fp)
{
    int ch;
    size_t used = 0;

    while ((ch = MY_GETC(fp)) != EOF) {
	if (used + 2 > *have) {
	    adjust_buffer(buffer, *have *= 2);
	}
	(*buffer)[used++] = (char) ch;
	if (ch == '\n')
	    break;
    }
    (*buffer)[used] = EOS;
    return (used != 0);
}

static const char *
data_filename(const DATA * p)
{
    return p ? (p->modified + (prefix_opt >= 0 ? p->base : prefix_len)) : "";
}

static int
count_lines2(const char *filename)
{
    int result = 0;
    FILE *fp;

    TRACE(("count_lines \"%s\"\n", filename));

    if ((fp = fopen(filename, "r")) != 0) {
	int ch;

	result = 0;
	while ((ch = MY_GETC(fp)) != EOF) {
	    if (ch == '\n')
		++result;
	}
	(void) fclose(fp);
	TRACE(("->%d lines\n", result));
    } else {
	(void) fflush(stdout);
	fprintf(stderr, "Cannot open \"%s\"\n", filename);
    }
    return result;
}

/*
 * Count the (new)lines in a file, return -1 if the file is not found.
 */
static int
count_lines(const DATA * p)
{
    int result = -1;
    char *filename = 0;
    const char *filetail = data_filename(p);
    size_t want = strlen(path_opt) + 2 + strlen(filetail) + strlen(p->modified);

    if ((filename = xmalloc(want)) != 0) {
	int merge = 0;

	if (path_dest && *path_opt != EOS && *filetail != PATHSEP) {
	    size_t path_len = strlen(path_opt);
	    size_t tail_len = strlen(filetail);
	    const char *tail_sep = strchr(filetail, PATHSEP);
	    size_t n;

	    for (n = path_len - 1; (int) n >= 0; --n) {
		if ((path_len - n) > tail_len)
		    break;
		if ((n == 0 || path_opt[n - 1] == PATHSEP)
		    && filetail[path_len - n] == PATHSEP) {
		    if (!strncmp(path_opt + n, filetail, path_len - n)) {
			merge = 1;
			strcpy(filename, path_opt);
			strcpy(filename + n, filetail);
			break;
		    }
		}
	    }

	    if (merge == 0 && tail_sep != 0) {
		tail_len = (size_t) (tail_sep - filetail);
		if (tail_len != 0 && tail_len <= path_len) {
		    if (tail_len < path_len
			&& path_opt[path_len - tail_len - 1] != PATHSEP) {
			merge = 0;
		    } else if (!strncmp(path_opt + path_len - tail_len,
					filetail,
					tail_len - 1)) {
			merge = 1;
			if (path_len > tail_len) {
			    sprintf(filename, "%.*s%c%s",
				    (int) (path_len - tail_len),
				    path_opt,
				    PATHSEP,
				    filetail);
			} else {
			    strcpy(filename, filetail);
			}
		    }
		}
	    }
	}
	if (!merge) {
	    if (!path_opt) {
		strcpy(filename, p->modified);
	    } else {
		sprintf(filename, "%s%c%s", path_opt, PATHSEP, filetail);
	    }
	}

	result = count_lines2(filename);
	free(filename);
    } else {
	failed("count_lines");
    }
    return result;
}

static void
update_chunk(DATA * p, Change change)
{
    if (merge_opt) {
	p->pending += 1;
	p->chunk[change] += 1;
    } else {
	p->count[change] += 1;
    }
}

static void
finish_chunk(DATA * p)
{
    if (p->pending) {
	int i;

	p->pending = 0;
	p->chunks += 1;
	if (merge_opt) {
	    /*
	     * This is crude, but to make it really precise we would have
	     * to keep an array of line-numbers to which which in a chunk
	     * are marked as insert/delete.
	     */
	    if (p->chunk[cInsert] && p->chunk[cDelete]) {
		long change;
		if (p->chunk[cInsert] > p->chunk[cDelete]) {
		    change = p->chunk[cDelete];
		} else {
		    change = p->chunk[cInsert];
		}
		p->chunk[cInsert] -= change;
		p->chunk[cDelete] -= change;
		p->chunk[cModify] += change;
	    }
	}
	for_each_mark(i) {
	    p->count[i] += p->chunk[i];
	    p->chunk[i] = 0;
	}
    }
}

static char *
copy_notabs(char *target, char *source, size_t limit)
{
    char *result = 0;
    if (limit-- != 0) {		/* count trailing null */
	char ch;
	int found = 0;
	while ((ch = *source) != EOS) {
	    if (ch == TAB) {
		if (found)
		    result = source;
		break;
	    } else if (limit-- == 0) {
		break;
	    }
	    *target++ = ch;
	    *target = EOS;
	    ++source;
	    found = 1;
	}
    }
    return result;
}

static char *
copy_graphs(char *target, char *source, size_t limit)
{
    int found = 0;
    if (limit-- != 0) {		/* count trailing null */
	char ch;
	while ((ch = *source) != EOS) {
	    if (ch == TAB || ch == BLANK) {
		break;
	    } else if (limit-- == 0) {
		found = 0;
		break;
	    }
	    *target++ = ch;
	    *target = EOS;
	    ++source;
	    found = 1;
	}
    }
    return found ? source : NULL;
}

/*
 * Tested with git 2.11:
 * git uses dummy directory-names "a" and "b" rather than the actual working
 * directory.  Also, it allows non-printable characters, encoded in C-style
 * backslash sequences.  When those are used, it double-quotes the string.
 */
static char *
copy_git_name(char *target, char *source, size_t limit)
{
    int found = 0;
    int quoted = 0;

    /*
     * Account for double-quote.
     */
    if (*source == DQUOTE) {
	quoted = 1;
	++source;
	limit--;
    }

    /*
     * Check for the dummy directory paths, and quit if not used.
     */
    if (limit <= 2 || (strncmp(source, "a/", 2) && strncmp(source, "b/", 2))) {
	limit = 0;
    } else {
	if (path_dest && !strncmp(source, "b/", 2)) {
	    source += 2;	/* tweak to help with counting lines */
	}
    }

    if (limit-- != 0) {		/* count trailing null */
	char ch;
	while ((ch = *source) != EOS) {
	    if (quoted) {
		if (ch == DQUOTE) {
		    if (*++source != EOS)
			found = 0;
		    break;
		} else if (ch == BACKSL) {
		    int fail = 0;
		    if ((ch = *++source) == EOS) {
			fail = 1;
		    } else if (isoctal(UC(ch))) {
			int need = 3;
			int value = 0;
			/* decode octal escapes into UTF-8 bytes */
			while (need-- > 0) {
			    if (isoctal(*source)) {
				value <<= 3;
				value |= (UC(*source) - '0');
				if (need) {
				    ++source;
				}
			    } else {
				fail = 1;
				break;
			    }
			}
			ch = (char) value;
		    } else {
			--limit;
			switch (ch) {
			case BACKSL:
			    /* FALLTHRU */
			case DQUOTE:
			    break;
			case 'b':
			    ch = '\b';
			    break;
			case 'n':
			    ch = '\n';
			    break;
			case 'r':
			    ch = '\r';
			    break;
			case 't':
			    ch = '\t';
			    break;
			default:
			    fail = 1;
			    break;
			}
		    }
		    if (fail) {
			found = 0;
			break;
		    }
		}
	    } else if (!isprint(UC(ch))) {
		break;
	    }
	    if (limit-- == 0) {
		found = 0;
		break;
	    }
	    *target++ = ch;
	    *target = EOS;
	    ++source;
	    found = 1;
	}
    }
    return found ? source : NULL;
}

/* perforce */
static char *
copy_p4_name(char *target, char *source, size_t limit)
{
    int found = 0;
    if (limit-- != 0) {		/* count trailing null */
	char ch;
	while ((ch = *source) != EOS) {
	    if (ch == TAB || ch == BLANK || ch == '#') {
		break;
	    } else if (limit-- == 0) {
		found = 0;
		break;
	    }
	    *target++ = ch;
	    *target = EOS;
	    ++source;
	    found = 1;
	}
    }
    return found ? source : NULL;
}

static char *
copy_integer(int *target, const char *source)
{
    char *next = NULL;
    long value = strtol(source, &next, 10);
    *target = (int) value;
    return next;
}

static char *
need_blanks(char *source)
{
    int found = 0;
    while (*source != EOS) {
	char ch = *source++;
	if (ch == BLANK || ch == TAB)
	    found = 1;
    }
    return found ? source : NULL;
}

static char *
need_graphs(char *source)
{
    char *result = NULL;
    int found = 0;
    while (*source != EOS) {
	char ch = *source;
	if (ch == BLANK || ch == TAB || ch == EOS) {
	    if (found)
		result = source;
	    break;
	}
	++source;
	found = 1;
    }
    return result;
}

static char *
need_nospcs(char *source)
{
    char *result = NULL;
    int found = 0;
    while (*source != EOS) {
	char ch = *source;
	if (ch == BLANK || ch == EOS) {
	    if (found)
		result = source;
	    break;
	}
	++source;
	found = 1;
    }
    return result;
}

/* this is used with SVN */
static char *
need_parens(char *source)
{
    char *result = NULL;
    if (*source++ == LPAREN) {
	while (*source != EOS) {
	    if (*source++ == RPAREN) {
		result = source;
		break;
	    }
	}
    }
    return result;
}

#define date_delims(a,b) (((a)=='/' && (b)=='/') || ((a) == '-' && (b) == '-'))
#define CASE_TRACE() TRACE(("** handle case for '%c' %d:%s\n", *buffer, ok, that ? that->modified : ""))

static void
do_file(FILE *fp, const char *default_name)
{
    static const char *only_stars = "***************";

    DATA dummy;
    DATA *that = &dummy;
    DATA *prev = 0;
    char *buffer = 0;
    char *b_fname = 0;
    size_t length = 0;
    size_t fixed = 0;
    int ok = HAVE_NOTHING;
    int marker;
    int freed = 0;

    int unified = 0;
    int old_unify = 0;
    int new_unify = 0;
    int expect_unify = 0;

    long old_dft = 0;
    long new_dft = 0;

    int context = 1;
    int either = 0;

    int first_ch;
    int git_diff = 0;

    char *s;
#if OPT_TRACE
    int line_no = 0;
#endif

    init_data(&dummy, "", "", 1, 0);

    fixed_buffer(&buffer, fixed = length = BUFSIZ);
    fixed_buffer(&b_fname, length);

    while (get_line(&buffer, &length, fp)) {
	/*
	 * Adjust size of fixed-buffers so that a sscanf cannot overflow.
	 */
	if (length > fixed) {
	    fixed = length;
	    adjust_buffer(&b_fname, length);
	}

	/*
	 * Trim trailing newline.
	 */
	for (s = buffer + strlen(buffer); s != buffer; s--) {
	    if ((UC(s[-1]) == '\n') || (UC(s[-1]) == '\r'))
		s[-1] = EOS;
	    else
		break;
	}

	/*
	 * Trim escapes from colordiff.
	 */
#define isFINAL(c) (UC(*s) >= '\140' && UC(*s) <= '\176')
	if (trim_escapes && (strchr(buffer, '\033') != 0)) {
	    char *d = buffer;
	    s = d;
	    while (*s != EOS) {
		if (*s == '\033') {
		    while (*s != EOS && !isFINAL(*s)) {
			++s;
		    }
		    if (*s != EOS) {
			++s;
			continue;
		    } else {
			break;
		    }
		}
		*d++ = *s++;
	    }
	    *d = EOS;
	}
	++line_no;
	TRACE(("[%05d] %s\n", line_no, buffer));

	/*
	 * "patch -U" can create ".rej" files lacking a filename header,
	 * in unified format.  Check for those.
	 */
	if (line_no == 1 && !strncmp(buffer, "@@", (size_t) 2)) {
	    unified = 2;
	    that = find_data(default_name, default_name);
	    ok = begin_data(that);
	}

	/*
	 * The lines identifying files in a context diff depend on how it was
	 * invoked.  But after the header, each chunk begins with a line
	 * containing 15 *'s.  Each chunk may contain a line-range with '***'
	 * for the "before", and a line-range with '---' for the "after".  The
	 * part of the chunk depicting the deletion may be absent, though the
	 * edit line is present.
	 *
	 * The markers for unified diff are a little different from the normal
	 * context-diff.  Also, the edit-lines in a unified diff won't have a
	 * space in column 2.  Because of the missing space, we have to count
	 * lines to ensure we do not confuse the marker lines.
	 */
	marker = 0;
	if (that != &dummy && !strcmp(buffer, only_stars)) {
	    finish_chunk(that);
	    TRACE(("** begin context chunk\n"));
	    context = 2;
	} else if (line_no == 1 && !strcmp(buffer, only_stars)) {
	    TRACE(("** begin context chunk\n"));
	    context = 2;
	    that = find_data(default_name, default_name);
	    ok = begin_data(that);
	} else if (context == 2 && match(buffer, "*** ")) {
	    context = 1;
	} else if (context == 1 && match(buffer, "--- ")) {
	    marker = 1;
	    context = 0;
	} else if (match(buffer, "*** ")) {
	} else if ((old_unify + new_unify) == 0 && match(buffer, "==== ")) {
	    finish_chunk(that);
	    unified = 2;
	} else if ((old_unify + new_unify) == 0 && match(buffer, "--- ")) {
	    finish_chunk(that);
	    marker = unified = 1;
	} else if ((old_unify + new_unify) == 0 && match(buffer, "+++ ")) {
	    marker = unified = 2;
	} else if (unified == 2
		   || ((old_unify + new_unify) == 0 && (*buffer == '@'))) {
	    finish_chunk(that);
	    unified = 0;
	    if (*buffer == '@') {
		int old_base, new_base;
		int old_size = 0;
		int new_size = 0;
		char *sp;

		old_unify = new_unify = 0;
		if ((sp = match(buffer, "@@ -")) != NULL
		    && (sp = decode_range(sp, &old_base, &old_size)) != NULL
		    && (sp = match(sp, " +")) != NULL
		    && (sp = decode_range(sp, &new_base, &new_size)) != NULL
		    && match(sp, " @") != NULL) {
		    old_unify = old_size;
		    new_unify = new_size;
		    unified = -1;
		}
	    }
	} else if (unified == 1 && !context) {
	    /*
	     * If unified==1, we guessed we would find a "+++" line, but since
	     * we are here, we did not find that.  The context check ensures
	     * we do not mistake the "---" for a unified diff with that for
	     * a context diff's "after" line-range.
	     *
	     * If we guessed wrong, then we probably found a data line with
	     * "--" in the first two columns of the diff'd file.
	     */
	    unified = 0;
	    TRACE(("?? Expected \"+++\" for unified diff\n"));
	    if (prev != 0
		&& prev != that
		&& InsOf(that) == 0
		&& DelOf(that) == 0
		&& strcmp(prev->modified, that->modified)) {
		TRACE(("?? giveup on %ld/%ld %s\n", InsOf(that),
		       DelOf(that), that->modified));
		TRACE(("?? revert to %ld/%ld %s\n", InsOf(prev),
		       DelOf(prev), prev->modified));
		(void) delink(that);
		that = prev;
		update_chunk(that, cDelete);
	    }
	} else if (old_unify + new_unify) {
	    switch (*buffer) {
	    case '-':
		if (old_unify)
		    --old_unify;
		break;
	    case '+':
		if (new_unify)
		    --new_unify;
		break;
	    case EOS:
	    case ' ':
		if (old_unify)
		    --old_unify;
		if (new_unify)
		    --new_unify;
		break;
	    case BACKSL:
		if (strstr(buffer, "newline") != 0) {
		    break;
		}
		/* FALLTHRU */
	    default:
		TRACE(("?? expected more in chunk\n"));
		old_unify = new_unify = 0;
		break;
	    }
	    if (!(old_unify + new_unify)) {
		expect_unify = 2;
	    }
	} else {
	    long old_base, new_base;

	    unified = 0;

	    if (line_no == 1
		&& decode_default(buffer,
				  &old_base, &old_dft,
				  &new_base, &new_dft)) {
		TRACE(("DFT %ld,%ld -> %ld,%ld\n",
		       old_base, old_base + old_dft - 1,
		       new_base, new_base + new_dft - 1));
		finish_chunk(that);
		that = find_data("unknown", "unknown");
		ok = begin_data(that);
	    }
	}

	/*
	 * If the previous line ended a chunk of a unified diff, we may begin
	 * another chunk, or begin another type of diff.  If neither, do not
	 * continue to accumulate counts for the unified diff which has ended.
	 */
	if (expect_unify != 0) {
	    if (expect_unify-- == 1) {
		if (unified == 0) {
		    TRACE(("?? did not get chunk\n"));
		    finish_chunk(that);
		    that = &dummy;
		}
	    }
	}

	/*
	 * Override the beginning of the line to simplify the case statement
	 * below.
	 */
	if (marker > 0) {
	    TRACE(("** have marker=%d, override %s\n", marker, buffer));
	    (void) memcpy(buffer, "***", (size_t) 3);
	}

	first_ch = *buffer;

	/*
	 * GIT binary diffs can contain blocks of data that might be confused
	 * with the ordinary line-oriented sections in diff output.  Skip the
	 * case statement if we are processing a GIT binary diff.
	 */
	switch (git_diff) {
	default:
	    break;
	case 1:
	    /* expect "index" */
	    if (match(buffer, "index") != 0
		|| match(buffer, "rename") != 0
		|| match(buffer, "similarity") != 0) {
		git_diff = 2;
		continue;
	    } else {
		git_diff = 0;
	    }
	    break;
	case 2:
	    /* perhaps "GIT binary patch" */
	    if (match(buffer, "GIT binary patch") != 0) {
		git_diff = 3;
		that->cmt = Binary;
		continue;
	    } else if (match(buffer, "Binary files ") != 0) {
		git_diff = 0;
		that->cmt = Binary;
		continue;
	    }
	    break;
	case 3:
	    /* had "GIT binary patch", wait for next "diff" line */
	    if (first_ch != 'd')
		continue;
	    break;
	}

	/*
	 * Use the first character of the input line to determine its
	 * type:
	 */
	switch (first_ch) {
	case 'O':		/* Only */
	    CASE_TRACE();
	    if (match(buffer, "Only in ")) {
		char *path = buffer + 8;
		int found = 0;
		for (s = path; *s != EOS; s++) {
		    if (match(s, ": ")) {
			found = 1;
			if ((s - path) >= 2 && s[-1] == PATHSEP) {
			    while ((s[0] = s[2]) != EOS)
				s++;
			} else {
			    *s++ = PATHSEP;
			    while ((s[0] = s[1]) != EOS)
				s++;
			}
			break;
		    }
		}
		if (found) {
		    blip('.');
		    finish_chunk(that);
		    that = find_data(NULL, path);
		    that->cmt = Only;
		    ok = HAVE_NOTHING;
		}
	    }
	    break;

	    /*
	     * Several different scripts produce "Index:" lines
	     * (e.g., "makepatch").  Not all bother to put the
	     * pathname of the files; some put only the leaf names.
	     */
	case 'I':
	    CASE_TRACE();
	    if ((s = match(buffer, "Index: ")) != 0) {
		s = skip_blanks(s);
		dequote(s);
		blip('.');
		finish_chunk(that);
		s = do_merging(that, s, &freed);
		that = find_data(NULL, s);
		ok = begin_data(that);
	    }
	    break;

	case 'd':		/* diff command trace */
	    CASE_TRACE();
	    if ((s = match(buffer, "diff ")) != 0
		&& *(s = skip_options(s)) != EOS) {
		char *original = NULL;
		char *modified = NULL;
		char *to_blank = NULL;
		if (reverse_opt) {
		    modified = s;
		    to_blank = skip_filename(s);
		    original = skip_blanks(to_blank);
		    *to_blank = EOS;
		} else {
		    original = s;
		    to_blank = skip_filename(s);
		    s = skip_blanks(to_blank);
		    modified = s;
		    *to_blank = EOS;
		}
		if (match(buffer, "diff --git ") != 0) {
		    size_t old_len = strlen(original);
		    size_t new_len = strlen(modified);
		    char *temp = xmalloc(old_len + new_len + 1);
		    git_diff = 1;
		    if (copy_git_name(temp, original, old_len) != NULL)
			original = new_string(temp);
		    if (copy_git_name(temp, modified, new_len) != NULL)
			modified = new_string(temp);
		    free(temp);
		} else {
		    git_diff = 0;
		    dequote(original);
		    dequote(modified);
		}
		blip('.');
		finish_chunk(that);
		modified = do_merging(that, modified, &freed);
		that = find_data(original, modified);
		ok = begin_data(that);
	    }
	    break;

	case '*':
	    CASE_TRACE();
	    if (!(ok & HAVE_PATH)) {
		int ddd, hour, minute, second;
		int day, month, year;
		char yrmon, monday;
		char *stars = match(buffer, "*** ");
		char *sp;

		if (stars == NULL)
		    break;	/* ignore */

		/* check for tab-delimited first, so we can
		 * accept filenames containing spaces.
		 */
		if (((sp = copy_notabs(b_fname, stars, length)) != NULL
		     && (sp = match(sp, "\t")) != NULL
		     && (sp = need_nospcs(sp)) != NULL
		     && (sp = match(sp, " ")) != NULL
		     && (sp = need_nospcs(sp)) != NULL
		     && sscanf(sp,
			       " %d %d:%d:%d %d",
			       &ddd,
			       &hour, &minute, &second, &year) == 5)
		    || ((sp = copy_notabs(b_fname, stars, length)) != NULL
			&& sscanf(sp,
				  "\t%d%c%d%c%d %d:%d:%d",
				  &year, &yrmon, &month, &monday, &day,
				  &hour, &minute, &second) == 8
			&& date_delims(yrmon, monday)
			&& !version_num(b_fname))
		    || ((sp = copy_notabs(b_fname, stars, length)) != NULL
			&& (sp = match(sp, "\t")) != NULL
			&& (sp = need_parens(sp)) != NULL
			&& (sp = match(sp, "\t")) != NULL
			&& need_parens(sp) != NULL
			&& !version_num(b_fname))
		    || ((sp = copy_notabs(b_fname, stars, length)) != NULL
			&& (sp = match(sp, "\t")) != NULL
			&& (sp = need_parens(sp)) != NULL
			&& (*skip_blanks(sp) == EOS))
		    || ((sp = copy_graphs(b_fname, stars, length)) != NULL
			&& (sp = need_blanks(sp)) != NULL
			&& (sp = need_nospcs(sp)) != NULL
			&& (sp = match(sp, " ")) != NULL
			&& (sp = need_nospcs(sp)) != NULL
			&& sscanf(sp,
				  " %d %d:%d:%d %d",
				  &ddd, &hour, &minute, &second, &year) == 5)
		    || ((sp = copy_graphs(b_fname, stars, length)) != NULL
			&& (sp = need_blanks(sp)) != NULL
			&& sscanf(sp,
				  "%d%c%d%c%d %d:%d:%d",
				  &year, &yrmon, &month, &monday, &day,
				  &hour, &minute, &second) == 8
			&& date_delims(yrmon, monday)
			&& !version_num(b_fname))
		    || ((sp = copy_git_name(b_fname, stars, length)) != NULL
			&& *skip_blanks(sp) == EOS)
		    || ((sp = copy_graphs(b_fname, stars, length)) != NULL
			&& (*sp == EOS || *sp == BLANK || *sp == TAB)
			&& !version_num(b_fname)
			&& !contain_any(b_fname, "*")
			&& !edit_range(b_fname))
		    ) {
		    const char *git_source = NULL;

		    prev = that;
		    finish_chunk(that);
		    dequote(b_fname);

		    /*
		     * Git diff's may reflect a renamed file.  For this case,
		     * we want to keep track of the original file, for the
		     * -S/-D comparison.
		     */
		    if ((marker > 1) && git_diff) {
			char *tt = that->original;
			if (strlen(tt) > 2
			    && (!strncmp("a/", tt, 2)
				|| !strncmp("b/", tt, 2))) {
			    tt += 2;
			}
			git_source = new_string(tt);
		    }

		    s = do_merging(that, b_fname, &freed);
		    if (freed)
			prev = 0;
		    that = find_data(git_source, s);
		    ok = begin_data(that);
		    TRACE(("** after merge:%d:%s\n", ok, s));
		}
	    }
	    break;

	case '=':
	    CASE_TRACE();
	    if (!(ok & HAVE_PATH)) {
		int rev;
		char *bars, *sp;

		if ((bars = match(buffer, "==== ")) != NULL
		    && (bars = copy_p4_name(b_fname, bars, length)) != NULL
		    && (bars = match(bars, "#")) != NULL
		    && (bars = copy_integer(&rev, bars)) != NULL
		    && (((sp = match(bars, " - ")) != NULL
			 && need_graphs(sp) != NULL)
			|| (((sp = match(bars, " ")) != NULL
			     && (sp = need_parens(sp)) != NULL
			     && (sp = match(sp, " - ")) != NULL
			     && need_graphs(sp) != NULL)))
		    && !version_num(b_fname)
		    && !contain_any(b_fname, "*")
		    && !edit_range(b_fname)) {
		    TRACE(("** found p4-diff\n"));
		    prev = that;
		    finish_chunk(that);
		    dequote(b_fname);
		    s = do_merging(that, b_fname, &freed);
		    if (freed)
			prev = 0;
		    that = find_data(NULL, s);
		    ok = begin_data(that);
		    TRACE(("** after merge:%d:%s\n", ok, s));
		}
	    }
	    break;

	case '+':
	    /* FALL-THRU */
	case '>':
	    CASE_TRACE();
	    if (ok) {
		update_chunk(that, cInsert);
	    }
	    break;

	case '-':
	    if (!ok) {
		CASE_TRACE();
		break;
	    }
	    if (!unified && !strcmp(buffer, "---")) {
		CASE_TRACE();
		break;
	    }
	    /* fall-thru */
	case '<':
	    CASE_TRACE();
	    if (ok) {
		update_chunk(that, cDelete);
	    }
	    break;

	case '!':
	    CASE_TRACE();
	    if (ok) {
		update_chunk(that, cModify);
	    }
	    break;

	    /* Expecting "Files XXX and YYY differ" */
	case 'F':		/* FALL-THRU */
	case 'f':
	    CASE_TRACE();
	    if ((s = match(buffer + 1, "iles ")) != 0) {
		char *first = skip_blanks(s);
		/* blindly assume the first filename does not contain " and " */
		char *at_and = strstr(s, " and ");
		s = strrchr(buffer, BLANK);
		if ((at_and != NULL) && !strcmp(s, " differ")) {
		    char *second = skip_blanks(at_and + 5);

		    if (reverse_opt) {
			*at_and = EOS;
			s = first;
		    } else {
			*s = EOS;
			s = second;
		    }
		    blip('.');
		    finish_chunk(that);
		    that = find_data(NULL, s);
		    that->cmt = Either;
		    ok = HAVE_NOTHING;
		    either = 1;
		}
	    }
	    break;
	    /* Expecting "Binary files XXX and YYY differ" */
	case 'B':		/* FALL-THRU */
	case 'b':
	    CASE_TRACE();
	    if ((s = match(buffer + 1, "inary files ")) != 0) {
		char *first = skip_blanks(s);
		/* blindly assume the first filename does not contain " and " */
		char *at_and = strstr(s, " and ");
		s = strrchr(buffer, BLANK);
		if ((at_and != NULL) && !strcmp(s, " differ")) {
		    char *second = skip_blanks(at_and + 5);

		    if (reverse_opt) {
			*at_and = EOS;
			s = first;
		    } else {
			*s = EOS;
			s = second;
		    }
		    blip('.');
		    finish_chunk(that);
		    that = find_data(NULL, s);
		    that->cmt = Binary;
		    ok = HAVE_NOTHING;
		}
	    }
	    break;
	}
    }
    blip('\n');

    finish_chunk(that);
    finish_chunk(&dummy);

    if (either) {
	int pass;
	int fixup_diffs = 0;

	for (pass = 0; pass < 2; ++pass) {
	    DATA *p;
	    for (p = all_data; p; p = p->link) {
		switch (p->cmt) {
		default:
		    break;
		case Normal:
		    fixup_diffs = 1;
		    break;
		case Either:
		    if (pass) {
			if (fixup_diffs) {
			    p->cmt = Binary;
			} else {
			    p->cmt = Differs;
			}
		    }
		    break;
		}
	    }
	}
    }

    free(buffer);
    free(b_fname);
}

static void
show_color(int color)
{
    if (color >= 0)
	printf("\033[%dm", color + 30);
    else
	printf("\033[0;39m");
}

static long
plot_bar(long count, int c, int color)
{
    long result = count;

    if (show_colors && result != 0)
	show_color(color);

    while (--count >= 0)
	(void) putchar(c);

    if (show_colors && result != 0)
	show_color(-1);

    return result;
}

/*
 * Each call to 'plot_num()' prints a scaled bar of 'c' characters.  The
 * 'extra' parameter is used to keep the accumulated error in the bar's total
 * length from getting large.
 */
static long
plot_num(long num_value, int c, int color, long *extra)
{
    long result = 0;

    /* the value to plot */
    /* character to display in the bar */
    /* accumulated error in the bar */
    if (num_value) {
	long product = (plot_width * num_value);
	result = ((product + *extra) / plot_scale);
	*extra = product - (result * plot_scale) - *extra;
	plot_bar(result, c, color);
    }
    return result;
}

static long
plot_round1(const long num[MARKS])
{
    long result = 0;
    long scaled[MARKS];
    long remain[MARKS];
    long want = 0;
    long have = 0;
    long half = (plot_scale / 2);
    int i;

    memset(scaled, 0, sizeof(scaled));
    memset(remain, 0, sizeof(remain));

    for_each_mark(i) {
	long product = (plot_width * num[i]);
	scaled[i] = (product / plot_scale);
	remain[i] = (product % plot_scale);
	want += product;
	have += product - remain[i];
    }
    while (want > have) {
	int j = -1;
	for_each_mark(i) {
	    if (remain[i] != 0
		&& (remain[i] > (j >= 0 ? remain[j] : half))) {
		j = i;
	    }
	}
	if (j >= 0) {
	    have += remain[j];
	    remain[j] = 0;
	    scaled[j] += 1;
	} else {
	    break;
	}
    }
    for_each_mark(i) {
	plot_bar(scaled[i], marks[i], colors[i]);
	result += scaled[i];
    }
    return result;
}

/*
 * Print a scaled bar of characters, where c[0] is for insertions, c[1]
 * for deletions and c[2] for modifications. The num array contains the
 * count for each type of change, in the same order.
 */
static long
plot_round2(const long num[MARKS])
{
    long result = 0;
    long scaled[MARKS];
    long remain[MARKS];
    long total = 0;
    int i;

    for (i = 0; i < MARKS; i++)
	total += num[i];

    if (total == 0)
	return result;

    total = (total * plot_width + (plot_scale / 2)) / plot_scale;
    /* display at least one character */
    if (total == 0)
	total++;

    for_each_mark(i) {
	scaled[i] = num[i] * plot_width / plot_scale;
	remain[i] = num[i] * plot_width - scaled[i] * plot_scale;
	total -= scaled[i];
    }

    /* assign the missing chars using the largest remainder algo */
    while (total) {
	int largest, largest_count;	/* largest is a bit field */
	long max_remain;

	/* search for the largest remainder */
	largest = largest_count = 0;
	max_remain = 0;
	for_each_mark(i) {
	    if (remain[i] > max_remain) {
		largest = 1 << i;
		largest_count = 1;
		max_remain = remain[i];
	    } else if (remain[i] == max_remain) {	/* ex aequo */
		largest |= 1 << i;
		largest_count++;
	    }
	}

	/* if there are more greatest remainders than characters
	   missing, don't assign them at all */
	if (total < largest_count)
	    break;

	/* allocate the extra characters */
	for_each_mark(i) {
	    if (largest & (1 << i)) {
		scaled[i]++;
		total--;
		remain[i] -= plot_width;
	    }
	}
    }

    for_each_mark(i) {
	result += plot_bar(scaled[i], marks[i], colors[i]);
    }

    return result;
}

static void
plot_numbers(const DATA * p)
{
    long temp = 0;
    int i;

    printf("%5ld ", TotalOf(p));

    if (format_opt & FMT_VERBOSE) {
	printf("%5ld ", InsOf(p));
	printf("%5ld ", DelOf(p));
	printf("%5ld ", ModOf(p));
	if (path_opt)
	    printf("%5ld ", EqlOf(p));
    }

    if (format_opt == FMT_CONCISE) {
	for_each_mark(i) {
	    printf("\t%ld %c", p->count[i], marks[i]);
	}
    } else {
	long used = 0;

	switch (round_opt) {
	default:
	    for_each_mark(i) {
		used += plot_num(p->count[i], marks[i], colors[i], &temp);
	    }
	    break;
	case 1:
	    used = plot_round1(p->count);
	    break;

	case 2:
	    used = plot_round2(p->count);
	    break;
	}

	if ((format_opt & FMT_FILLED) != 0) {
	    if (used > plot_width)
		printf("%ld", used - plot_width);	/* oops */
	    else
		plot_bar(plot_width - used, '.', 0);
	}
    }
}

static int
columns_of(const char *value)
{
    int result;
    int n;
    int ch;
#ifdef HAVE_MBSTOWCWIDTH
    int fixup = 0;
    for (n = 0; (ch = UC(value[n])) != EOS; ++n) {
	if (ch >= DEL || ch < BLANK) {
	    fixup = 1;
	    break;
	}
    }
    result = (int) strlen(value);
    if (fixup) {
	size_t needed;
	mbstate_t state;
	const char *source;
	size_t length = strlen(value);

	memset(&state, 0, sizeof(state));
	source = value;
	needed = mbsrtowcs(NULL, &source, length, &state);
	if (needed != (size_t) (-1)) {
	    wchar_t *target = calloc(1 + needed, sizeof(wchar_t));
	    memset(&state, 0, sizeof(state));
	    source = value;
	    if (mbsrtowcs(target, &source, length, &state) == needed) {
		size_t n2;
		result = 0;
		for (n2 = 0; n2 < needed; ++n2) {
		    int nw = wcwidth(target[n2]);
		    if (nw > 0)
			result += nw;
		    else if (target[n2] < BLANK || target[n2] == DEL)
			result += 2;
		    else
			result += 4;	/* show as octal */
		}
	    }
	    free(target);
	}
    }
#else
    result = (int) strlen(value);
    for (n = 0; (ch = UC(value[n])) != EOS; ++n) {
	if (ch == DEL || ch < BLANK) {
	    result += 1;
	} else if (ch > DEL) {
	    result += 3;	/* show as octal */
	}
    }
#endif
    return result;
}

#define adjustwide(width,name) width += (int) strlen(name) - columns_of(name)

static void
show_quoted(const char *value)
{
    int ch;

    putchar(DQUOTE);
    while ((ch = UC(*value++)) != EOS) {
	if (ch == DQUOTE)
	    putchar(DQUOTE);
	putchar(ch);
    }
    putchar(DQUOTE);
}

static void
show_unquoted(const char *value, int limit)
{
    int ch;
    while ((ch = UC(*value++)) != EOS) {
	if (ch < BLANK) {
	    if (strchr("\b\n\r\t\\\"", ch) != NULL) {
		putchar(BACKSL);
		switch (ch) {
		case '\b':
		    ch = 'b';
		    break;
		case '\n':
		    ch = 'n';
		    break;
		case '\r':
		    ch = 'r';
		    break;
		case '\t':
		    ch = 't';
		    break;
		}
	    } else {
		putchar('^');
		ch |= '@';
	    }
	} else if (ch == DEL) {
	    putchar('^');
	    ch = '?';
	}
#ifndef HAVE_MBSTOWCWIDTH
	else if (ch > DEL) {
	    char temp[5];
	    sprintf(temp, "\\%03o", ch & 0xff);
	    ch = temp[3];
	    temp[3] = EOS;
	    fputs(temp, stdout);
	}
#endif
	putchar(ch);
	--limit;
    }
    while (limit-- > 0) {
	putchar(BLANK);
    }
}

#define changed(p) (!merge_names \
		    || (p)->cmt != Normal \
		    || (TotalOf(p)) != 0)

static void
show_data(const DATA * p)
{
    const char *name = data_filename(p);
    int width;

    if (summary_only) {
	;
    } else if (!changed(p)) {
	;
    } else if (p->cmt == Binary && suppress_binary == 1) {
	;
    } else if (table_opt == 1) {
	if (names_only) {
	    show_quoted(name);
	} else {
	    printf("%ld,%ld,%ld,",
		   InsOf(p),
		   DelOf(p),
		   ModOf(p));
	    if (path_opt)
		printf("%ld,", EqlOf(p));
	    if (count_files && !reverse_opt)
		printf("%d,%d,%d,",
		       (p->cmt == OnlyRight),
		       (p->cmt == OnlyLeft),
		       (p->cmt == Binary));
	    show_quoted(name);
	}
	printf("\n");
    } else if (names_only) {
	printf("%s\n", name);
    } else {
	printf("%s ", comment_opt);
	if (max_name_wide > 0
	    && max_name_wide < min_name_wide
	    && max_name_wide < ((width = (int) columns_of(name)))) {
	    printf("%.*s", max_name_wide, name + (width - max_name_wide));
	} else {
	    width = ((max_name_wide > 0 && max_name_wide < min_name_wide)
		     ? max_name_wide
		     : min_name_wide);
	    adjustwide(width, name);
	    show_unquoted(name, width);
	}
	if (table_opt == 2) {
	    putchar('|');
	    if (path_opt)
		printf("%*ld ", number_len, EqlOf(p));
	    printf("%*ld ", number_len, InsOf(p));
	    printf("%*ld ", number_len, DelOf(p));
	    printf("%*ld", number_len, ModOf(p));
	}
	putchar('|');
	switch (p->cmt) {
	default:
	case Normal:
	    plot_numbers(p);
	    break;
	case Binary:
	    printf("binary");
	    break;
	case Differs:
	    printf("differ");
	    break;
	case Only:
	    printf("only");
	    break;
	case OnlyLeft:
	    printf(count_files ? "deleted" : "only");
	    break;
	case OnlyRight:
	    printf(count_files ? "added" : "only");
	    break;
	}
	printf("\n");
    }
}

#ifdef HAVE_TSEARCH
static void
show_tsearch(const void *nodep, const VISIT which, const int depth)
{
    const DATA *p = *(DATA * const *) nodep;
    (void) depth;
    if (which == postorder || which == leaf)
	show_data(p);
}
#endif

static int
ignore_data(DATA * p)
{
    return ((!changed(p))
	    || (p->cmt == Binary && suppress_binary));
}

/*
 * Return the length of any directory-prefix from the given path.
 */
static size_t
path_length(const char *path)
{
    size_t result = 0;
    char *mark = strrchr(path, PATHSEP);
    if (mark != 0 && mark != path)
	result = (size_t) (mark + 1 - path);
    return result;
}

/*
 * If we have an "only" filename, we can guess whether it was added or removed
 * by looking at its directory and comparing that to other files' directories.
 *
 * TODO: -K -R combination is not yet supported because that relies on storing
 * both left-/right-paths for each file; only the right-path is currently used.
 */
static Comment
resolve_only(const DATA * p)
{
    Comment result = p->cmt;
    if (result == Only && !reverse_opt) {
	DATA *q;
	size_t len1 = path_length(p->modified);
	if (len1 != 0) {
	    for (q = all_data; q; q = q->link) {
		result = OnlyLeft;
		if (q->cmt == Normal || q->cmt == Binary) {
		    size_t len2 = path_length(q->modified);
		    if (len2 >= len1) {
			if (!strncmp(p->modified, q->modified, len1)) {
			    result = OnlyRight;
			    break;
			}
		    }
		}
	    }
	}
    }
    return result;
}

#ifdef HAVE_OPENDIR
static void
count_unmodified_files(const char *pathname, long *files, long *lines)
{
    DATA *p;
    char *name;

    TRACE(("count_unmodified_files \"%s\"\n", pathname));
    if (is_dir(pathname)) {
	DIR *dp = opendir(pathname);

	if (dp != 0) {
	    const struct dirent *de;

	    while ((de = readdir(dp)) != 0) {
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
		    continue;
		name = xmalloc(strlen(pathname) + 2 + strlen(de->d_name));
		if (name != 0) {
		    sprintf(name, "%s%c%s", pathname, PATHSEP, de->d_name);
		    if ((strcmp(de->d_name, ".git")
			 && strcmp(de->d_name, ".svn")
			 && strcmp(de->d_name, "CVS")
			 && strcmp(de->d_name, "RCS")) || !is_dir(name)) {
			count_unmodified_files(name, files, lines);
		    }
		    free(name);
		}
	    }
	    closedir(dp);
	}
    } else if (is_file(pathname)) {
	/*
	 * Given the pathname from the (-D) source directory, derive a
	 * corresponding path for the source directory.  Then check if
	 * that path appears in the list of modified files.
	 */
	const char *ref_name = ((all_data && !unchanged)
				? all_data->modified
				: pathname);
	char *source = 0;

	if (ref_name == NULL)
	    return;

	if (prefix_opt >= 0) {
	    int level_s = count_prefix(path_opt);
	    int base_s = 0;
	    int base_d = 0;

	    (void) skip_prefix(pathname, level_s + 1, &base_s);
	    (void) skip_prefix(ref_name, level_s + 1, &base_d);
	    name = xmalloc(2 + strlen(pathname) + strlen(ref_name));
	    sprintf(name, "%.*s%s", base_d, ref_name, base_s + pathname);
	    source = xmalloc(strlen(ref_name) + 2 + strlen(pathname) +
			     strlen(S_option));
	    sprintf(source, "%s%c%s",
		    S_option,
		    PATHSEP,
		    base_s + pathname);
	} else {
	    const char *mark = unchanged ? ref_name : data_filename(all_data);
	    int skip = 1 + (int) strlen(path_opt);

	    name = xmalloc(strlen(ref_name) + 2 + strlen(pathname));
	    sprintf(name, "%.*s%s",
		    (int) (mark - ref_name),
		    ref_name,
		    pathname + skip);
	    source = xmalloc(strlen(ref_name) + 2 + strlen(pathname) +
			     strlen(S_option));
	    sprintf(source, "%s%c%.*s%s",
		    S_option,
		    PATHSEP,
		    (int) (mark - ref_name),
		    ref_name,
		    pathname + skip);
	}

	if (same_file(source, pathname)) {
	    int found = 0;

	    for (p = all_data; p != 0 && !found; p = p->link) {
		if (!strcmp(name, p->modified)) {
		    found = 1;
		}
	    }
	    if (!found) {
		p = find_data(NULL, name);
		*files += 1;
		EqlOf(p) = count_lines(p);
		*lines += EqlOf(p);

		if (unchanged) {
		    int len = columns_of(p->modified);
		    if (min_name_wide < (len - p->base))
			min_name_wide = (len - p->base);
		}
	    }
	}
	free(name);
	free(source);
    }
}
#endif

static void
update_min_name_wide(long longest_name)
{
    if (prefix_opt < 0) {
	if (prefix_len < 0)
	    prefix_len = 0;
	if ((longest_name - prefix_len) > min_name_wide)
	    min_name_wide = (int) (longest_name - prefix_len);
    }

    if (min_name_wide < 1)
	min_name_wide = 0;
    min_name_wide++;		/* make sure it's nonzero */
}

static void
summarize(void)
{
    DATA *p;
    long total_ins = 0;
    long total_del = 0;
    long total_mod = 0;
    long total_eql = 0;
    long files_added = 0;
    long files_equal = 0;
    long files_binary = 0;
    long files_removed = 0;
    long temp;
    int num_files = 0, shortest_name = -1, longest_name = -1;

    plot_scale = 0;
    for (p = all_data; p; p = p->link) {
	int len = columns_of(p->modified);

	if (ignore_data(p))
	    continue;

	/*
	 * If "-pX" option is given, prefix_opt is positive.
	 *
	 * "-p0" gives the whole pathname unmodified.  "-p1" strips
	 * through the first path-separator, etc.
	 */
	if (prefix_opt >= 0) {
	    /* p->base has been computed at node creation */
	    if (min_name_wide < (len - p->base))
		min_name_wide = (len - p->base);
	} else {
	    /*
	     * If "-pX" option is not given, strip off any prefix which is
	     * shared by all of the names.
	     */
	    if (len < prefix_len || prefix_len < 0)
		prefix_len = len;
	    while (prefix_len > 0) {
		if (p->modified[prefix_len - 1] != PATHSEP)
		    prefix_len--;
		else if (strncmp(all_data->modified, p->modified, (size_t) prefix_len))
		    prefix_len--;
		else
		    break;
	    }

	    if (len > longest_name)
		longest_name = len;
	    if (len < shortest_name || shortest_name < 0)
		shortest_name = len;
	}
    }

    /*
     * Get additional counts for files where we cannot count lines changed.
     */
    if (count_files) {
	for (p = all_data; p; p = p->link) {
	    switch (p->cmt) {
	    case Binary:
		files_binary++;
		break;
	    case Only:
		switch (resolve_only(p)) {
		case OnlyRight:
		    p->cmt = OnlyRight;
		    files_added++;
		    break;
		case OnlyLeft:
		    p->cmt = OnlyLeft;
		    files_removed++;
		    break;
		default:
		    /* ignore - we could not guess */
		    break;
		}
	    default:
		break;
	    }
	}
    }

    /*
     * Use a separate loop after computing prefix_len so we can apply the "-S"
     * or "-D" options to find files that we can use as reference for the
     * unchanged-count.
     */
    for (p = all_data; p; p = p->link) {
	if (!ignore_data(p)) {
	    EqlOf(p) = 0;
	    if (reverse_opt) {
		long save_ins = InsOf(p);
		long save_del = DelOf(p);
		InsOf(p) = save_del;
		DelOf(p) = save_ins;
	    }
	    if (path_opt != 0) {
		int count = count_lines(p);

		if (count >= 0) {
		    EqlOf(p) = count - ModOf(p);
		    if (path_dest != 0) {
			EqlOf(p) -= InsOf(p);
		    } else {
			EqlOf(p) -= DelOf(p);
		    }
		    if (EqlOf(p) < 0)
			EqlOf(p) = 0;
		}
	    }
	    num_files++;
	    total_ins += InsOf(p);
	    total_del += DelOf(p);
	    total_mod += ModOf(p);
	    total_eql += EqlOf(p);
	    temp = TotalOf(p);
	    if (temp > plot_scale)
		plot_scale = temp;
	}
    }

    update_min_name_wide(longest_name);

#ifdef HAVE_OPENDIR
    if (S_option != 0 && D_option != 0) {
	unchanged = (all_data == 0);
	count_unmodified_files(D_option, &files_equal, &total_eql);
	if (unchanged) {
	    for (p = all_data; p; p = p->link) {
		int len = columns_of(p->modified);
		if (longest_name < len)
		    longest_name = len;
		temp = TotalOf(p);
		if (temp > plot_scale)
		    plot_scale = temp;
	    }
	    update_min_name_wide(longest_name);
	}
    }
#endif

    plot_width = (max_width - min_name_wide - 8);
    if (plot_width < 10)
	plot_width = 10;

    if (plot_scale < plot_width)
	plot_scale = plot_width;	/* 1:1 */

    if (table_opt == 1) {
	if (!names_only) {
	    printf("INSERTED,DELETED,MODIFIED,");
	    if (path_opt)
		printf("UNCHANGED,");
	    if (count_files && !reverse_opt)
		printf("FILE-ADDED,FILE-DELETED,FILE-BINARY,");
	}
	printf("FILENAME\n");
    } else if (table_opt == 2) {
	long largest = 0;
	for (p = all_data; p; p = p->link) {
	    if (path_opt)
		largest = maximum(largest, EqlOf(p));
	    largest = maximum(largest, InsOf(p));
	    largest = maximum(largest, DelOf(p));
	    largest = maximum(largest, ModOf(p));
	}
	number_len = 0;
	while (largest > 0) {
	    number_len++;
	    largest /= 10;
	}
	number_len = maximum(number_len, 3);
    }
#ifdef HAVE_TSEARCH
    if (use_tsearch) {
	twalk(sorted_data, show_tsearch);
    } else
#endif
	for (p = all_data; p; p = p->link) {
	    show_data(p);
	}

    if ((table_opt != 1) && !names_only) {
#define PLURAL(n) n, n != 1 ? "s" : ""
	if (num_files > 0 || !quiet) {
	    printf("%s %d file%s changed", comment_opt, PLURAL(num_files));
	    if (total_ins)
		printf(", %ld insertion%s(+)", PLURAL(total_ins));
	    if (total_del)
		printf(", %ld deletion%s(-)", PLURAL(total_del));
	    if (total_mod)
		printf(", %ld modification%s(!)", PLURAL(total_mod));
	    if (total_eql && path_opt != 0)
		printf(", %ld unchanged line%s(=)", PLURAL(total_eql));
	    if (count_files) {
		if (files_added)
		    printf(", %ld file%s added", PLURAL(files_added));
		if (files_removed)
		    printf(", %ld file%s removed", PLURAL(files_removed));
		if (files_binary)
		    printf(", %ld binary file%s", PLURAL(files_binary));
	    }
	    (void) putchar('\n');
	}
    }
}

#ifdef HAVE_POPEN
static const char *
get_program(const char *name, const char *dft)
{
    const char *result = getenv(name);
    if (result == 0 || *result == EOS)
	result = dft;
    TRACE(("get_program(%s) = %s\n", name, result));
    return result;
}
#define GET_PROGRAM(name) get_program("DIFFSTAT_" #name, name)

static char *
decompressor(Decompress which, const char *name)
{
    const char *verb = 0;
    const char *opts = "";
    char *result = 0;

    switch (which) {
    case dcBzip:
	verb = GET_PROGRAM(BZCAT_PATH);
	if (*verb == EOS) {
	    verb = GET_PROGRAM(BZIP2_PATH);
	    opts = "-dc";
	}
	break;
    case dcCompress:
	verb = GET_PROGRAM(ZCAT_PATH);
	if (*verb == EOS) {
	    verb = GET_PROGRAM(UNCOMPRESS_PATH);
	    opts = "-c";
	    if (*verb == EOS) {
		/* not all compress's recognize the options, test this last */
		verb = GET_PROGRAM(COMPRESS_PATH);
		opts = "-dc";
	    }
	}
	break;
    case dcGzip:
	verb = GET_PROGRAM(GZIP_PATH);
	opts = "-dc";
	break;
    case dcLzma:
	verb = GET_PROGRAM(LZCAT_PATH);
	opts = "-dc";
	break;
    case dcPack:
	verb = GET_PROGRAM(PCAT_PATH);
	break;
    case dcXz:
	verb = GET_PROGRAM(XZ_PATH);
	opts = "-dc";
	break;
    case dcZstd:
	verb = GET_PROGRAM(ZSTD_PATH);
	opts = "-qdcf";
	break;
    case dcEmpty:
	/* FALLTHRU */
    case dcNone:
	break;
    }
    if (verb != 0 && *verb != EOS) {
	result = (char *) xmalloc(strlen(verb) + 10 + strlen(name));
	sprintf(result, "%s %s", verb, opts);
	if (*name != EOS) {
	    sprintf(result + strlen(result), " \"%s\"", name);
	}
    }
    return result;
}

static char *
is_compressed(const char *name)
{
    size_t len = strlen(name);
    Decompress which;

#define CHKEND(end) \
    (len > (sizeof(end) - 1) && \
     !strcmp(name + len - (sizeof(end) - 1), end))

    if (CHKEND(".Z")) {
	which = dcCompress;
    } else if (CHKEND(".z")) {
	which = dcPack;
    } else if (CHKEND(".gz")) {
	which = dcGzip;
    } else if (CHKEND(".bz2")) {
	which = dcBzip;
    } else if (CHKEND(".lzma")) {
	which = dcLzma;
    } else if (CHKEND(".xz")) {
	which = dcXz;
    } else if (CHKEND(".zst")) {
	which = dcZstd;
    } else {
	which = dcNone;
    }
    return decompressor(which, name);
}

#ifdef HAVE_MKDTEMP
#define MY_MKDTEMP(path) mkdtemp(path)
#else
/*
 * mktemp is supposedly marked obsolete at the same point that mkdtemp is
 * introduced.
 */
static char *
my_mkdtemp(char *path)
{
    char *result = mktemp(path);
    if (result != 0) {
	if (MKDIR(result, 0700) < 0) {
	    result = 0;
	}
    }
    return path;
}
#define MY_MKDTEMP(path) my_mkdtemp(path)
#endif

static char *
copy_stdin(char **dirpath)
{
    const char *tmp = getenv("TMPDIR");
    char *result = 0;
    if (tmp == 0)
	tmp = "/tmp/";
    *dirpath = xmalloc(strlen(tmp) + 12);

    strcpy(*dirpath, tmp);
    strcat(*dirpath, "/diffXXXXXX");

    if (MY_MKDTEMP(*dirpath) != 0) {
	FILE *fp;

	result = xmalloc(strlen(*dirpath) + 10);
	sprintf(result, "%s/stdin", *dirpath);

	if ((fp = fopen(result, "w")) != 0) {
	    int ch;

	    while ((ch = MY_GETC(stdin)) != EOF) {
		fputc(ch, fp);
	    }
	    (void) fclose(fp);
	} else {
	    free(result);
	    result = 0;
	    rmdir(*dirpath);	/* Assume that the /stdin file was not created */
	    free(*dirpath);
	    *dirpath = 0;
	}
    } else {
	free(*dirpath);
	*dirpath = 0;
    }
    return result;
}
#endif

static void
set_path_opt(char *value, int destination)
{
    path_opt = value;
    path_dest = destination;
    if (*path_opt != 0) {
	if (is_dir(path_opt)) {
	    num_marks = 4;
	} else {
	    (void) fflush(stdout);
	    fprintf(stderr, "Not a directory:%s\n", path_opt);
	    exit(EXIT_FAILURE);
	}
    }
}

static void
usage(FILE *fp)
{
    static const char *msg[] =
    {
	"Usage: diffstat [options] [files]",
	"",
	"Reads from one or more input files which contain output from 'diff',",
	"producing a histogram of total lines changed for each file referenced.",
	"If no filename is given on the command line, reads from standard input.",
	"",
	"Options:",
	"  -b      ignore lines matching \"Binary files XXX and YYY differ\"",
	"  -c      prefix each line with comment (#)",
	"  -C      add SGR color escape sequences to highlight the histogram",
#if OPT_TRACE
	"  -d      debug - prints a lot of information",
#endif
	"  -D PATH specify location of patched files, use for unchanged-count",
	"  -e FILE redirect standard error to FILE",
	"  -E      trim escape-sequences, e.g., from colordiff",
	"  -f NUM  format (0=concise, 1=normal, 2=filled, 4=values)",
	"  -h      print this message",
	"  -k      do not merge filenames",
	"  -K      resolve ambiguity of \"only\" filenames",
	"  -l      list filenames only",
	"  -m      merge insert/delete data in chunks as modified-lines",
	"  -n NUM  specify minimum width for the filenames (default: auto)",
	"  -N NUM  specify maximum width for the filenames (default: auto)",
	"  -o FILE redirect standard output to FILE",
	"  -O      inspect only files listed in diff for -S/-D options",
	"  -p NUM  specify number of pathname-separators to strip (default: common)",
	"  -q      suppress the \"0 files changed\" message for empty diffs",
	"  -r NUM  specify rounding for histogram (0=none, 1=simple, 2=adjusted)",
	"  -R      assume patch was created with old and new files swapped",
	"  -s      show only the summary line",
	"  -S PATH specify location of original files, use for unchanged-count",
	"  -t      print a table (comma-separated-values) rather than histogram",
	"  -T      print amounts (like -t option) in addition to histogram",
	"  -u      do not sort the input list",
	"  -v      show progress if output is redirected to a file",
	"  -V      prints the version number",
	"  -w NUM  specify maximum width of the output (default: 80)",
    };
    unsigned j;
    for (j = 0; j < sizeof(msg) / sizeof(msg[0]); j++)
	fprintf(fp, "%s\n", msg[j]);
}

/* Wrapper around getopt that also parses "--help" and "--version".  
 * argc, argv, opts, return value, and globals optarg, optind,
 * opterr, and optopt are as in getopt().  help and version designate
 * what should be returned if --help or --version are encountered. */
static int
getopt_helper(int argc, char *const argv[], const char *opts,
	      int help, int version)
{
    if (optind < argc && argv[optind] != NULL) {
	if (strcmp(argv[optind], "--help") == 0) {
	    optind++;
	    return help;
	} else if (strcmp(argv[optind], "--version") == 0) {
	    optind++;
	    return version;
	}
    }
    return getopt(argc, argv, opts);
}

static int
getopt_value(void)
{
    char *next = 0;
    long value = strtol(optarg, &next, 0);
    if (next == 0 || *next != EOS) {
	(void) fflush(stdout);
	fprintf(stderr, "expected a number, have '%s'\n", optarg);
	exit(EXIT_FAILURE);
    }
    return (int) value;
}

#define OPTIONS "\
b\
cC\
dD:\
e:E\
f:\
h\
kK\
l\
m\
n:N:\
o:\
p:\
q\
r:R\
sS:\
tT\
uv\
Vw:"

int
main(int argc, char *argv[])
{
    int j;
    char version[80];

#if defined(HAVE_TCGETATTR) && defined(TIOCGWINSZ)
    if (isatty(fileno(stdout))) {
	struct winsize data;
	if (ioctl(fileno(stdout), TIOCGWINSZ, &data) == 0) {
	    max_width = data.ws_col;
	}
    }
#endif

#ifdef HAVE_MBSTOWCWIDTH
    setlocale(LC_CTYPE, "");
#endif

    while ((j = getopt_helper(argc, argv, OPTIONS, 'h', 'V'))
	   != -1) {
	switch (j) {
	case 'b':
	    suppress_binary = 1;
	    break;
	case 'c':
	    comment_opt = "#";
	    break;
	case 'C':
	    show_colors = 1;
	    break;
#if OPT_TRACE
	case 'd':
	    trace_opt = 1;
	    break;
#endif
	case 'D':
	    D_option = optarg;
	    break;
	case 'e':
	    if (freopen(optarg, "w", stderr) == 0)
		failed(optarg);
	    break;
	case 'E':
	    trim_escapes = 1;
	    break;
	case 'f':
	    format_opt = getopt_value();
	    break;
	case 'h':
	    usage(stdout);
	    return (EXIT_SUCCESS);
	case 'k':
	    merge_names = 0;
	    break;
	case 'K':
	    count_files = 1;
	    break;
	case 'l':
	    names_only = 1;
	    break;
	case 'm':
	    merge_opt = 1;
	    break;
	case 'n':
	    min_name_wide = getopt_value();
	    break;
	case 'N':
	    max_name_wide = getopt_value();
	    break;
	case 'o':
	    if (freopen(optarg, "w", stdout) == 0)
		failed(optarg);
	    break;
	case 'p':
	    prefix_opt = getopt_value();
	    break;
	case 'r':
	    round_opt = getopt_value();
	    break;
	case 'R':
	    reverse_opt = 1;
	    break;
	case 's':
	    summary_only = 1;
	    break;
	case 'S':
	    S_option = optarg;
	    break;
	case 't':
	    table_opt = 1;
	    break;
	case 'T':
	    table_opt = 2;
	    break;
	case 'u':
	    sort_names = 0;
	    break;
	case 'v':
	    verbose = 1;
	    break;
	case 'V':
#ifndef	NO_IDENT
	    if (!sscanf(Id, "%*s %*s %30s", version))
#endif
		(void) strcpy(version, "?");
	    printf("diffstat version %s\n", version);
	    return (EXIT_SUCCESS);
	case 'w':
	    max_width = getopt_value();
	    break;
	case 'q':
	    quiet = 1;
	    break;
	default:
	    usage(stderr);
	    return (EXIT_FAILURE);
	}
    }

    /*
     * The numbers from -S/-D options will only be useful if the merge option
     * is added.
     */
    if (S_option)
	set_path_opt(S_option, 0);
    if (D_option)
	set_path_opt(D_option, 1);
    if (path_opt)
	merge_opt = 1;

    show_progress = verbose && (!isatty(fileno(stdout))
				&& isatty(fileno(stderr)));

#ifdef HAVE_TSEARCH
    use_tsearch = (sort_names && merge_names);
#endif

    if (optind < argc) {
	while (optind < argc) {
	    FILE *fp;
	    const char *name = argv[optind++];
#ifdef HAVE_POPEN
	    char *command = is_compressed(name);
	    if (command != 0) {
		if ((fp = popen(command, "r")) != 0) {
		    if (show_progress) {
			(void) fflush(stdout);
			(void) fprintf(stderr, "%s\n", name);
			(void) fflush(stderr);
		    }
		    do_file(fp, name);
		    (void) pclose(fp);
		}
		free(command);
	    } else
#endif
	    if ((fp = fopen(name, "rb")) != 0) {
		if (show_progress) {
		    (void) fflush(stdout);
		    (void) fprintf(stderr, "%s\n", name);
		    (void) fflush(stderr);
		}
		do_file(fp, name);
		(void) fclose(fp);
	    } else {
		failed(name);
	    }
	}
    } else {
#ifdef HAVE_POPEN
	Decompress which = dcEmpty;
	char *stdin_dir = 0;
	char *myfile;
	char sniff[8];
	int ch;
	unsigned got = 0;

	if ((ch = MY_GETC(stdin)) != EOF) {
	    which = dcNone;
	    if (ch == 'B') {	/* perhaps bzip2 (poor magic design...) */
		sniff[got++] = (char) ch;
		while (got < 5) {
		    if ((ch = MY_GETC(stdin)) == EOF)
			break;
		    sniff[got++] = (char) ch;
		}
		if (got == 5
		    && !strncmp(sniff, "BZh", (size_t) 3)
		    && isdigit(UC(sniff[3]))
		    && isdigit(UC(sniff[4]))) {
		    which = dcBzip;
		}
	    } else if (ch == ']') {	/* perhaps lzma */
		sniff[got++] = (char) ch;
		while (got < 4) {
		    if ((ch = MY_GETC(stdin)) == EOF)
			break;
		    sniff[got++] = (char) ch;
		}
		if (got == 4
		    && !memcmp(sniff, "]\0\0\200", (size_t) 4)) {
		    which = dcLzma;
		}
	    } else if (ch == 0xfd) {	/* perhaps xz */
		sniff[got++] = (char) ch;
		while (got < 6) {
		    if ((ch = MY_GETC(stdin)) == EOF)
			break;
		    sniff[got++] = (char) ch;
		}
		if (got == 6
		    && !memcmp(sniff, "\3757zXZ\0", (size_t) 6)) {
		    which = dcXz;
		}
	    } else if (ch >= 0x22 && ch <= 0x28) {	/* perhaps zstd */
		sniff[got++] = (char) ch;
		while (got < 4) {
		    if ((ch = MY_GETC(stdin)) == EOF)
			break;
		    sniff[got++] = (char) ch;
		}
		if (got == 4	/* vi:{ */
		    && !memcmp(sniff + 1, "\265/\375", (size_t) 3)) {
		    which = dcZstd;
		}
	    } else if (ch == '\037') {	/* perhaps compress, etc. */
		sniff[got++] = (char) ch;
		if ((ch = MY_GETC(stdin)) != EOF) {
		    sniff[got++] = (char) ch;
		    switch (ch) {
		    case 0213:
			which = dcGzip;
			break;
		    case 0235:
			which = dcCompress;
			break;
		    case 0036:
			which = dcPack;
			break;
		    }
		}
	    } else {
		sniff[got++] = (char) ch;
	    }
	}
	/*
	 * The C standard only guarantees one ungetc;
	 * virtually everyone allows more.
	 */
	while (got != 0) {
	    ungetc(sniff[--got], stdin);
	}
	if (which != dcNone
	    && which != dcEmpty
	    && (myfile = copy_stdin(&stdin_dir)) != 0) {
	    FILE *fp;
	    char *command;

	    /* open pipe to decompress temporary file */
	    command = decompressor(which, myfile);
	    if ((fp = popen(command, "r")) != 0) {
		do_file(fp, "stdin");
		(void) pclose(fp);
	    }
	    free(command);

	    unlink(myfile);
	    free(myfile);
	    myfile = 0;
	    rmdir(stdin_dir);
	    free(stdin_dir);
	    stdin_dir = 0;
	} else if (which != dcEmpty)
#endif
	    do_file(stdin, "stdin");
    }
    summarize();
#if defined(NO_LEAKS)
    while (all_data != 0) {
	delink(all_data);
    }
#endif
    return (EXIT_SUCCESS);
}
