#if	!defined(NO_IDENT)
static	char	*Id = "$Id: diffstat.c,v 1.9 1994/06/12 23:34:07 tom Exp $";
#endif

/*
 * Title:	diffstat.c
 * Author:	T.E.Dickey
 * Created:	02 Feb 1992
 * Modified:
 *		12 Jun 1994, recognize unified diff.
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
#include <string.h>
#include <ctype.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
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

#define EOS '\0'

#define	PLOT_WIDTH	(max_width - name_wide - 9)

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
static	int	name_wide;
static	int	max_width;

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
int	match(s,p)
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
						*s++ = '/';
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
			if ((s = strrchr(buffer, ' ')) != 0) {
				blip('.');
				this = new_data(s+1);
				ok = TRUE;
			}
			break;

		case 'd':	/* diff command trace */
			if (!match(buffer, "diff "))
				break;
			if ((s = strrchr(buffer, ' ')) != 0) {
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
					if (!(s = strrchr(fname, '/')))
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
void	plot(num,max,c)
	long	num, max;
	int	c;
{
	num = (((PLOT_WIDTH * num) + (PLOT_WIDTH/2)) / max);
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
	"Options:"
	};
	register int j;
	for (j = 0; j < sizeof(msg)/sizeof(msg[0]); j++)
		fprintf(stderr, "%s\n", msg);
	exit (EXIT_FAILURE);
}

int	main(argc, argv)
	int	argc;
	char	*argv[];
{
	FILE	*fp;
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
