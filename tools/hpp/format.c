#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <toollib/error.h>
#include <toollib/vstring.h>
#include "db.h"
#include "var.h"

enum modes
{
    m_space = 0,
    m_punct,
    m_comment,
    m_html_comment,
    m_cpp,
    m_identifier,
    m_string,
    m_header,
    m_value,
    m_cppmacro
};

static const char * ModeDelim[] =
{
    "",
    "cpunctuation",
    "ccomment",
    "htmlcomment",
    "cppkeyword",
    "cidentifier",
    "cstring",
    "cheader",
    "cvalue",
    "cppmacro",
};

static DB * symbols;
static int  label = 1;
#define LABELSTACKMAX	256
static int  labelstack[LABELSTACKMAX+1];
static int  labelsp = LABELSTACKMAX;

static void
emit_html_char (int c, FILE * out)
{
    switch (c)
    {
    case '&': fputs ("&amp;", out); break;
    case '<': fputs ("&lt;", out); break;
    case '>': fputs ("&gt;", out); break;
    case '$': fputs ("&#36;", out); break;
    case '%': fputs ("&#37;", out); break;
    case '\\': fputs ("&#92;", out); break;
    case '{': fputs ("&#123;", out); break;
    case '}': fputs ("&#125;", out); break;
    case ' ': fputs ("&nbsp;", out); break;
    default: fputc (c, out); break;
    }
}

static void
emit_html_string (const char * str, FILE * out)
{
    while (*str)
	emit_html_char (*str ++, out);
}

int
main (int argc, char ** argv)
{
    int    c;
    FILE * in;
    char * type;
    char * outformat;
    char * infilename;
    int    t;
    enum modes mode = m_space;
    String ident;
    char * data;
    int    column;

    Var_Init ();
    DB_Init ();
    DB_Add ("c-html", "c-html.db");
    symbols = DB_New ("symbols");

    labelstack[labelsp] = -1;

#define NEWMODE(nm)             \
    if ((nm) != mode)           \
    {				\
	if (ModeDelim[mode][0]) \
	    printf ("</%s>", ModeDelim[mode]);    \
	mode = (nm);            \
	if (ModeDelim[mode][0]) \
	    printf ("<%s>", ModeDelim[mode]);    \
    }

    type = outformat = infilename = NULL;

    for (t=1; t<argc; t++)
    {
	if (!strcmp (argv[t], "-type"))
	    type = argv[++t];
	else if (!strcmp (argv[t], "-outformat"))
	    outformat = argv[++t];
	else
	    infilename = argv[t];
    }

    if (!type)
    {
	Error ("Missing argument -type");
	ErrorExit (10);
    }

    if (!outformat)
    {
	Error ("Missing argument -outformat");
	ErrorExit (10);
    }

    if (infilename)
    {
	in = fopen (infilename, "r");

	if (!in)
	{
	    StdError ("Opening %s for reading", infilename);
	    ErrorExit (10);
	}
    }
    else
	in = stdin;

    if (strcasecmp (type, "c"))
    {
	Error ("Unknown type %s. Supported types: C", type);
	ErrorExit (10);
    }

    if (strcasecmp (outformat, "html"))
    {
	Error ("Unknown output format %s. Supported formats: HTML", outformat);
	ErrorExit (10);
    }

    ident = VS_New (NULL);
    column = 1;

    while ((c = getc (in)) != EOF)
    {
	switch (c)
	{
	case '&':
	case '<':
	case '>':
	case '.': case '!': case '^': case '%':
	case '(': case ')': case '=': case '?':
	case '[': case ']': case '+': case '*':
	case '~': case '-': case ':': case ',':
	case ';': case '|':
	    NEWMODE(m_punct);
	    emit_html_char (c, stdout);
	    column ++;
	    break;

	case '{':
	    if (labelsp)
	    {
		labelstack[--labelsp] = -1;
	    }

	    NEWMODE(m_punct);
	    emit_html_char (c, stdout);
	    column ++;
	    break;

	case '}':
	    if (labelsp != LABELSTACKMAX)
		labelsp ++;

	    NEWMODE(m_punct);
	    emit_html_char (c, stdout);
	    column ++;
	    break;

	case '\'':
	    NEWMODE(m_value);
	    emit_html_char (c, stdout);

	    while ((c = getc (in)) != EOF)
	    {
		emit_html_char (c, stdout);
		if (c == '\\')
		{
		    emit_html_char (getc (in), stdout);
		}
		else if (c == '\'')
		    break;
	    }
	    break;

	case '\n':
	    NEWMODE(m_space);
	    fputs ("<BR>\n", stdout);
	    column = 1;
	    break;

	case ' ':
	    NEWMODE(m_space);
	    emit_html_char (c, stdout);
	    break;

	case '\t':
	    if (!(column % 8) )
	    {
		fputs ("&nbsp;", stdout);
		column ++;
	    }

	    while (column % 8)
	    {
		fputs ("&nbsp;", stdout);
		column ++;
	    }
	    break;

	case '/': /* Comment or / */
	    c = getc (in);

	    if (c == '*')
	    {
		int htmlmode = 0, cc = 0;

		c = getc (in);
		if (c == 'H')
		{
		    cc ++;
		    c = getc (in);
		    if (c == 'T')
		    {
			cc ++;
			c = getc (in);
			if (c == 'M')
			{
			    cc ++;
			    c = getc (in);
			    if (c == 'L')
				htmlmode = 1;
			}
		    }
		}

		if (!htmlmode)
		{
		    NEWMODE(m_comment);
		}
		else
		{
		    NEWMODE(m_html_comment);
		}


		if (!htmlmode)
		{
		    char * ptr = "HTML";

		    while (cc--)
		    {
			emit_html_char (*ptr, stdout);
			ptr ++;
		    }

		    ungetc (c, in);
		}

		while ((c = getc (in)) != EOF)
		{
rem_again:
		    if (c == '*')
		    {
			c = getc (in);

			if (c == '/')
			{
			    NEWMODE(m_space);
			    break;
			}
			else
			{
			    emit_html_char ('*', stdout);
			    goto rem_again;
			}
		    }
		    else if (c == '$')
		    {
			fputs ("&#36;", stdout);
		    }
		    else if (!htmlmode)
		    {
			emit_html_char (c, stdout);
		    }
		    else
			putchar (c);
		}
	    }
	    else
	    {
		NEWMODE(m_punct);
		emit_html_char ('/', stdout);
		column ++;
		ungetc (c, in);
	    }
	    break;

	case '"':
	    NEWMODE(m_string);
	    emit_html_char (c, stdout);
	    column ++;

	    while ((c = getc (in)) != EOF)
	    {
		emit_html_char (c, stdout);
		column ++;

		if (c == '\\')
		{
		    c = getc (in);
		    if (c == '\n')
		    {
			fputs ("<BR>", stdout);
			column = 1;
		    }
		    emit_html_char (c, stdout);
		    column ++;
		}
		else if (c == '"')
		{
		    break;
		}
	    }

	    break;

	case '#':
	    VS_Clear (ident);
	    VS_AppendChar (ident, c);

	    while ((c = getc (in)) != EOF)
	    {
		if (!isspace (c))
		    break;
	    }

	    VS_AppendChar (ident, c);

	    while ((c = getc (in)) != EOF)
	    {
		if (!isalpha (c))
		{
		    ungetc (c, in);
		    break;
		}

		VS_AppendChar (ident, c);
	    }

	    if (!strcmp (ident->buffer, "#define"))
	    {
		printf ("<A NAME=\"%d\">", label);
	    }

	    data = DB_FindData ("c-html", ident->buffer);

	    if (data)
	    {
		NEWMODE(m_space);
		fputs (data, stdout);
	    }
	    else
	    {
		NEWMODE(m_cpp);
		fputs (ident->buffer, stdout);
	    }

	    if (!strcmp (ident->buffer, "#include"))
	    {
		emit_html_char (' ', stdout);
		VS_Clear (ident);

		while ((c = getc (in)) != EOF)
		{
		    if (!isspace (c))
			break;
		}

		VS_AppendChar (ident, c);

		while ((c = getc (in)) != EOF)
		{
		    if (isspace (c))
		    {
			ungetc (c, in);
			break;
		    }

		    VS_AppendChar (ident, c);
		}

		data = DB_FindData ("c-html", ident->buffer);

		if (data)
		{
		    NEWMODE(m_space);
		    fputs (data, stdout);
		}
		else
		{
		    NEWMODE(m_header);
		    emit_html_string (ident->buffer, stdout);
		}
	    }
	    else if (!strcmp (ident->buffer, "#define"))
	    {
		char buffer[64];

		emit_html_char (' ', stdout);
		VS_Clear (ident);

		while ((c = getc (in)) != EOF)
		{
		    if (!isspace (c))
			break;
		}

		VS_AppendChar (ident, c);

		while ((c = getc (in)) != EOF)
		{
		    if (!(isalnum (c) || c == '_') )
		    {
			ungetc (c, in);
			break;
		    }

		    VS_AppendChar (ident, c);
		}

		sprintf (buffer, "<A HREF=\"#%d\"><cppmacro>%s</cppmacro></A>", label, ident->buffer);
		DB_AddData (symbols, ident->buffer, buffer);
		label ++;

		NEWMODE(m_cppmacro);
		emit_html_string (ident->buffer, stdout);
	    }

	    break;

	default:
	    if (isalpha (c) || c == '_')
	    {
		VS_Clear (ident);
		VS_AppendChar (ident, c);

		while ((c = getc (in)) != EOF)
		{
		    if (!isalnum (c) && c != '_')
		    {
			ungetc (c, in);
			break;
		    }

		    VS_AppendChar (ident, c);
		}

		data = DB_FindData ("symbols", ident->buffer);

		if (!data)
		    data = DB_FindData ("c-html", ident->buffer);

		if (!strcmp (ident->buffer, "typedef"))
		{
		    printf ("<A NAME=\"%d\">", label);
		    labelstack[labelsp] = label++;
		}

		if (data)
		{
		    NEWMODE(m_space);
		    fputs (data, stdout);
		}
		else
		{
		    if (labelstack[labelsp] != -1)
		    {
			char buffer[256];
			sprintf (buffer, "<A HREF=\"#%d\"><cusertype>%s</cusertype></A>", labelstack[labelsp], ident->buffer);
			DB_AddData (symbols, ident->buffer, buffer);
			labelstack[labelsp] = -1;
		    }

		    NEWMODE(m_identifier);
		    fputs (ident->buffer, stdout);
		}
	    }
	    else if (isdigit (c))
	    {
		NEWMODE(m_value);
		emit_html_char (c, stdout);

		while ((c = getc (in)) != EOF)
		{
		    if (!(isxdigit (c)
			|| c == '.'
			|| c == '+'
			|| c == '-'
			|| tolower(c) == 'x'
			|| tolower(c) == 'l'
			|| tolower(c) == 'f'
			|| tolower(c) == 'u'
		    ) )
		    {
			ungetc (c, in);
			break;
		    }

		    emit_html_char (c, stdout);
		}
	    }
	    else
		emit_html_char (c, stdout);

	    break;

	} /* switch (c) */
    } /* while !EOF */

    if (ModeDelim[mode][0])
	printf ("</%s>\n", ModeDelim[mode]);

    if (infilename)
	fclose (in);

    DB_Exit ();
    Var_Exit ();

    ErrorExit (0);
}

