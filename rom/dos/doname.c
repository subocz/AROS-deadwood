/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Prepare an IO message for use with a specific filename.
    Lang: english
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <string.h>
#include "dos_intern.h"

LONG DoName(struct IOFileSys *iofs, STRPTR name, struct DosLibrary *DOSBase)
{
    STRPTR volname, pathname, s1 = NULL;
    BPTR cur, lock = (BPTR)0;
    struct DosList *dl;
    struct Device *device;
    struct Unit *unit;
    struct FileHandle *fh;
    struct Process *me = (struct Process *)FindTask(NULL);

    if (!Strnicmp(name, "PROGDIR:", 8))
    {
	cur = me->pr_HomeDir;
	volname = NULL;
	pathname = name+8;
    } else if (*name == ':')
    {
	cur = me->pr_CurrentDir;
	volname = NULL;
	pathname = name+1;
    }else
    {
	/* Copy volume name */
	cur = me->pr_CurrentDir;
	s1 = name;
	pathname = name;
	volname = NULL;
	while (*s1)
	{
	    if (*s1++ == ':')
	    {
		volname = (STRPTR)AllocMem(s1-name, MEMF_ANY);
		if (volname==NULL)
		{
		    SetIoErr(ERROR_NO_FREE_STORE);
		    return ERROR_NO_FREE_STORE;
		}
		CopyMem(name, volname, s1-name-1);
		volname[s1-name-1] = '\0';
		pathname = s1;
		break;
	    }
	}
    }

    dl = LockDosList(LDF_ALL|LDF_READ);
    if (volname != NULL)
    {
	/* Find logical device */
	dl = FindDosEntry(dl, volname, LDF_ALL);
	if (dl == NULL)
	{
	    UnLockDosList(LDF_ALL|LDF_READ);
	    FreeMem(volname, s1-name);
	    SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
	    return ERROR_DEVICE_NOT_MOUNTED;
	} else if(dl->dol_Type == DLT_LATE)
	{
	    lock = Lock(dl->dol_misc.dol_assign.dol_AssignName, SHARED_LOCK);
	    UnLockDosList(LDF_ALL|LDF_READ);
	    if (lock)
	    {
		AssignLock(volname, lock);
		dl = LockDosList(LDF_ALL|LDF_READ);
		dl = FindDosEntry(dl, volname, LDF_ALL);
		if (dl == NULL)
		{
		    UnLockDosList(LDF_ALL|LDF_READ);
		    FreeMem(volname, s1-name);
		    SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
		    return ERROR_DEVICE_NOT_MOUNTED;
	        }

		device = dl->dol_Device;
		unit = dl->dol_Unit;
	    } else
	    {
		FreeMem(volname, s1-name);
		return IoErr();
	    }
	} else if (dl->dol_Type == DLT_NONBINDING)
	{
	    lock = Lock(dl->dol_misc.dol_assign.dol_AssignName, SHARED_LOCK);
	    fh = (struct FileHandle *)BADDR(lock);
	    if (fh != NULL)
	    {
		device = fh->fh_Device;
		unit = fh->fh_Unit;
	    } else
	    {
		UnLockDosList(LDF_ALL|LDF_READ);
		FreeMem(volname, s1-name);
		return IoErr();
	    }
	} else {
	    device=dl->dol_Device;
	    unit  =dl->dol_Unit;
	}
    } else if(cur)
    {
	fh = (struct FileHandle *)BADDR(cur);
	device = fh->fh_Device;
	unit = fh->fh_Unit;
    } else
    {
	device = DOSBase->dl_NulHandler;
	unit = DOSBase->dl_NulLock;
    }

    iofs->IOFS.io_Device = device;
    iofs->IOFS.io_Unit = unit;
    iofs->io_Union.io_NamedFile.io_Filename = pathname;

    /* Send the request. */
    DoIO(&iofs->IOFS);

    if (iofs->io_DosError == ERROR_OBJECT_NOT_FOUND &&
	dl->dol_Type == DLT_DIRECTORY)
    {
	struct AssignList *al;

	for (al = dl->dol_misc.dol_assign.dol_List;
	     al && iofs->io_DosError == ERROR_OBJECT_NOT_FOUND;
	     al = al->al_Next)
	{
	    fh = BADDR(al->al_Lock);
	    iofs->IOFS.io_Device = device;
	    iofs->IOFS.io_Unit = unit;
	    DoIO(&iofs->IOFS);
	}
    }

    if (dl != NULL)
    {
	if (dl->dol_Type == DLT_NONBINDING)
	    UnLock(lock);
	UnLockDosList(LDF_ALL|LDF_READ);
    }

    if (volname != NULL)
	FreeMem(volname, s1-name);

    SetIoErr(iofs->io_DosError);
    return iofs->io_DosError;
} /* DoName */
