/******************************************************************************
 * Copyright (c) 1994 by Thomas E. Dickey.  All Rights Reserved.              *
 *                                                                            *
 * You may freely copy or redistribute this software, so long as there is no  *
 * profit made from its use, sale trade or reproduction. You may not change   *
 * this copyright notice, and it must be included in any copy made.           *
 ******************************************************************************/
#if	!defined(NO_IDENT)
static	char	*Id = "$Id: diffstat.c,v 1.13 1994/06/17 23:32:17 tom Exp $";
#endif

/*
 * Title:	diffstat.c
 * Author:	T.E.Dickey
 * Created:	02 Feb 1992
 * Modified:
 *		17 Jun 1994, ifdef-<string.h>
 *		12 Jun 1994, recognize unified diff, and output of makepatch.
 *		04 Oct 1993, merge multiple diff-files, busy message when the
 *			     output is piped to a file.
 *
 * Function:	this program reads the output of 'diff' and displays a histogram
 *		of the insertions/deletions/modifications per-file.
 */

#if	defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdio.h>
#include <ctype.h>

#if HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#define strchr index
#define strrchr rindex
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#else
extern	int	atoi();
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#else
extern int	isatty();
#endif

#if HAVE_MALLOC_H
#include <malloc.h>
#else
extern	char	*malloc();
#endif

#if HAVE_GETOPT_H
#include <getopt.h>
#else
extern	int	getopt();
extern	char	*optarg;
extern	int	optind;
#endif

#if !defined(TRUE) || (TRUE != 1)
#undef  TRUE
#undef  FALSE
#define	TRUE		1
#define	FALSE		0
#endif

#if !defined(EXIT_SUCCESS)
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#endif

/******************************************************************************/

#define PATHSEP '/'
#define EOS     '\0'
#define BLANK   ' '

typedef	enum comment { Normal, Only, Binary } Comment;

typedef	struct	_data	{
	struct	_data	*link;
	char		*name;
	Comment		cmt;
	long		ins,	/* "+" count inserted lines */
			del,	/* "-" count deleted lines */
			mod;	/* "!" count modified lines */
	} DATA;

static	DATA	*all_data;
static	int	piped_output;
static	int	max_width;	/* the specified width-limit */
static	int	name_wide;	/* the amount reserved for filenames */
static	int	plot_width;	/* the amount left over for histogram */

/******************************************************************************/
#if	__STDC__
static	void	failed (char *s);
static	void	blip (int c);
static	char *	new_string(char *s);
static	DATA *	new_data(char *name);
static	int	match(char *s, char *p);
static	void	do_file(FILE *fp);
static	void	plot(long num, long max, int c);
static	void	summarize(void);
static	void	usage(void);
	int	main(int argc, char *argv[]);
#endif
/******************************************************************************/

static
void	failed(s)
	char	*s;
{
	perror(s);
	exit(EXIT_FAILURE);
}

static
void	blip(c)
	int	c;
{
	if (piped_output) {
		(void)fputc(c, stderr);
		(void)fflush(stderr);
	}
}

static
char *	new_string(s)
	char	*s;
{
	return strcpy(malloc((unsigned)(strlen(s)+1)), s);
}

static
DATA *	new_data(name)
	char	*name;
{
	register DATA *p, *q, *r;

	/* insert into sorted list */
	for (p = all_data, q = 0; p != 0; q = p, p = p->link) {
		int	cmp = strcmp(p->name, name);
		if (cmp == 0)
			return p;
		if (cmp > 0) {
			break;
		}
	}
	r = (DATA *)malloc(sizeof(DATA));
	if (q != 0)
		q->link = r;
	else
		all_data = r;

	r->link = p;
	r->name = new_string(name);
	r->cmt = Normal;
	r->ins =
	r->del =
	r->mod = 0;

	return r;
}

/* like strncmp, but without the 3rd argument */
static
int	match(s, p)
	char	*s;
	char	*p;
{
	while (*s != EOS) {
		if (*p == EOS)
			break;
		if (*s++ != *p++)
			return FALSE;
	}
	return TRUE;
}

static
void	do_file(fp)
	FILE	*fp;
{
	DATA	dummy, *this = &dummy;
	char	buffer[BUFSIZ];
	int	ok = FALSE;
	register char *s;

	while (fgets(buffer, sizeof(buffer), fp)) {
		for (s = buffer + strlen(buffer); s > buffer; s--) {
			if (isspace(s[-1]))
				s[-1] = EOS;
			else
				break;
		}

		/*
		 * The markers for unified diff are a little different from the
		 * normal context-diff:
		 */
		if (match(buffer, "+++ ")
		 || match(buffer, "--- "))
		 	(void)strncpy(buffer, "***", 3);

		/*
		 * Use the first character of the input line to determine its
		 * type:
		 */
		switch (*buffer) {
		case 'O':	/* Only */
			if (match(buffer, "Only in ")) {
				char *path = buffer + 8;
				int found = FALSE;
				for (s = path; *s != EOS; s++) {
					if (match(s, ": ")) {
						found = TRUE;
						*s++ = PATHSEP;
						while ((s[0] = s[1]) != EOS)
							s++;
						break;
					}
				}
				if (found) {
					blip('.');
					this = new_data(path);
					this->cmt = Only;
					ok = TRUE;
				}
			}
			break;

		case 'I':	/* Index (e.g., from makepatch) */
			if (!match(buffer, "Index: "))
				break;
			if ((s = strrchr(buffer, BLANK)) != 0) {
				blip('.');
				this = new_data(s+1);
				ok = TRUE;
			}
			break;

		case 'd':	/* diff command trace */
			if (!match(buffer, "diff "))
				break;
			if ((s = strrchr(buffer, BLANK)) != 0) {
				blip('.');
				this = new_data(s+1);
				ok = TRUE;
			}
			break;

		case '*':
			if (ok <= 0) {
				char	fname[BUFSIZ];
				char	wday[BUFSIZ], mmm[BUFSIZ];
				int	ddd, hour, minute, second, year;

				if (sscanf(buffer,
				    "*** %[^\t]\t%[^ ] %[^ ] %d %d:%d:%d %d",
				    fname,
				    wday, mmm, &ddd,
				    &hour, &minute, &second, &year) == 8) {
					ok = -TRUE;
					if (!(s = strrchr(fname, PATHSEP)))
						s = fname;
					else
						s++;
					this = new_data(s);
				}
			}
			break;

		case '+':
			/* fall-thru */
		case '>':
			if (!ok)
				break;
			this->ins += 1;
			break;

		case '-':
			if (!ok)
				break;
			if (buffer[1] == '-')
				break;
			/* fall-thru */
		case '<':
			if (!ok)
				break;
			this->del += 1;
			break;

		case '!':
			if (!ok)
				break;
			this->mod += 1;
			break;

		case 'B':	/* Binary */
			/* fall-thru */
		case 'b':	/* binary */
			if (match(buffer+1, "inary files "))
				this->cmt = Binary;
			break;
		}
	}
	blip('\n');
}

static
void	plot(num, max, c)
	long	num;
	long	max;
	int	c;
{
	num = (((plot_width * num) + (plot_width/2)) / max);
	while (--num >= 0)
		(void)putchar(c);
}

static
void	summarize()
{
	register DATA *p;
	long	scale = 0,
		total_ins = 0,
		total_del = 0,
		total_mod = 0,
		temp;
	int	num_files = 0;

	for (p = all_data; p; p = p->link) {
		int	len = strlen(p->name);
		if (len > name_wide)
			name_wide = len;
		num_files++;
		total_ins += p->ins;
		total_del += p->del;
		total_mod += p->mod;
		temp = p->ins + p->del + p->mod;
		if (temp > scale)
			scale = temp;
	}

	name_wide++;	/* make sure it's nonzero */
	plot_width = (max_width - name_wide - 8);
	if (plot_width < 10)
		plot_width = 10;

	for (p = all_data; p; p = p->link) {
		printf(" %-*.*s|", name_wide, name_wide, p->name);
		if (p->cmt == Normal) {
			printf("%5ld ", p->ins + p->del + p->mod);
			plot(p->ins, scale, '+');
			plot(p->del, scale, '-');
			plot(p->mod, scale, '!');
		} else if (p->cmt == Binary) {
			printf("binary");
		} else if (p->cmt == Only) {
			printf("only");
		}
		printf("\n");
	}

	printf(" %d files changed", num_files);
	if (total_ins) printf(", %ld insertions", total_ins);
	if (total_del) printf(", %ld deletions", total_del);
	if (total_mod) printf(", %ld modifications", total_mod);
	printf("\n");
}

static
void	usage()
{
	static	char	*msg[] = {
	"Usage: diffstat [options] [files]",
	"",
	"Reads from one or more input files which contain output from 'diff',",
	"producing a histgram of total lines changed for each file referenced.",
	"If no filename is given on the command line, reads from stdin.",
	"",
	"Options:",
	"  -w NUM  specify maximum width of the output (default: 80)"
	};
	register int j;
	for (j = 0; j < sizeof(msg)/sizeof(msg[0]); j++)
		fprintf(stderr, "%s\n", msg[j]);
	exit (EXIT_FAILURE);
}

int	main(argc, argv)
	int	argc;
	char	*argv[];
{
	register int	j;

	max_width = 80;
	piped_output = !isatty(fileno(stdout))
		     && isatty(fileno(stderr));

	while ((j = getopt(argc, argv, "w:")) != EOF) {
		switch (j) {
		case 'w':
			max_width = atoi(optarg);
			break;
		default:
			usage();
			/*NOTREACHED*/
		}
	}

	if (optind < argc) {
		while (optind < argc) {
			FILE *fp;
			char *name = argv[optind++];
			if ((fp = fopen(name, "r")) != 0) {
				if (piped_output) {
					(void)fprintf(stderr, "%s\n", name);
					(void)fflush(stderr);
				}
				do_file(fp);
			} else {
				failed(name);
			}
		}
	} else {
		do_file(stdin);
	}
	summarize();
	exit(EXIT_SUCCESS);
	/*NOTREACHED*/
	return (EXIT_SUCCESS);
}
