/*
    Copyright � 2004, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function basename().
*/

#include "__upath.h"

#define DEBUG 1
#include <aros/debug.h>

char *basename(char *filename)
{
    char *uname;
    char *pos;

    if (!filename || *filename == '\0')
    {
	D(bug("basename()=.\n"));
        return ".";
    }

    uname = (char *)__path_a2u(filename);

    pos = uname;

    if (pos[0] == '/' && pos[1] == '\0')
    {
	D(bug("basename(/)=/\n"));
	return uname;
    }

    D(bug("basename(%s)=", filename));

    pos = uname + strlen(uname);
    while (pos[-1] == '/')
    {
	--pos;
	pos[0] = '\0';
    }
    while (--pos > uname)
    {
	if (pos[0] == '/')
	{
	    uname = ++pos;
	    break;
	}
    }

    D(bug("%s\n", uname));
    return uname;
}

