#ifndef	lint
static	char	*Id = "$Id: diffstat.c,v 1.3 1993/10/12 21:00:02 dickey Exp $";
#endif

/*
 * Title:	diffstat.c
 * Author:	T.E.Dickey
 * Created:	02 Feb 1992
 * Modified:
 *		04 Oct 1993, merge multiple diff-files, busy message when the
 *			     output is piped to a file.
 *
 * Function:	this program reads the output of 'diff' and displays a histo
 *		graph of the insertions/deletions/modifications per-file.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
extern	char	*malloc();

#define	TRUE		1
#define	FALSE		0

#define	MAX_WIDTH	80
#define	NAME_WIDTH	14
#define	PLOT_WIDTH	(MAX_WIDTH-NAME_WIDTH-9)

typedef	struct	_data	{
	struct	_data	*link;
	char		*name;
	long		ins,
			del,
			mod;
	} DATA;

static	DATA	*all_data;
static	int	piped_output;

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
	r->ins =
	r->del =
	r->mod = 0;

	return r;
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
				s[-1] = '\0';
			else
				break;
		}
		switch (*buffer) {
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
		case 'b':	/* binary */
		case 'O':	/* Only */
			break;
		case 'd':	/* diff */
			if (!strncmp(buffer, "diff ", 5)) {
				ok = TRUE;
			} else
				break;
			if (piped_output) {
				(void)fputc('.', stderr);
				(void)fflush(stderr);
			}
			if ((s = strrchr(buffer, '/'))
			 || (s = strrchr(buffer, ' ')))
				this = new_data(s+1);
			break;
		}
	}
	if (piped_output) {
		(void)fputc('\n', stderr);
		(void)fflush(stderr);
	}
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
		num_files++;
		total_ins += p->ins;
		total_del += p->del;
		total_mod += p->mod;
		temp = p->ins + p->del + p->mod;
		if (temp > scale)
			scale = temp;
	}
		
	for (p = all_data; p; p = p->link) {
		printf("%*s:%5d ", NAME_WIDTH, p->name, p->ins + p->del + p->mod);
		plot(p->ins, scale, '+');
		plot(p->del, scale, '-');
		plot(p->mod, scale, '!');
		printf("\n");
	}

	printf("%d files changed", num_files);
	if (total_ins) printf(", %d insertions", total_ins);
	if (total_del) printf(", %d deletions", total_del);
	if (total_mod) printf(", %d modifications", total_mod);
	printf("\n");
}

void
main(argc, argv)
char	*argv[];
{
	FILE	*fp;
	register int	j;

	piped_output = !isatty(fileno(stdout))
		     && isatty(fileno(stderr));

	if (argc > 1) {
		for (j = 1; j < argc; j++) {
			char	*s = argv[j];
			if ((fp = fopen(s, "r")) != 0) {
				if (piped_output) {
					(void)fprintf(stderr, "%s\n", s);
					(void)fflush(stderr);
				}
				do_file(fp);
			} else
				perror(s);
		}
	} else
		do_file(stdin);
	summarize();
	exit(0);
	/*NOTREACHED*/
}
