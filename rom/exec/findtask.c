/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:27:09  digulla
    Added copyright notics and made headers conform

    Desc:
    Lang:
*/
#include <exec/execbase.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

__AROS_LH1(struct Task *, FindTask,

/*  SYNOPSIS */
	__AROS_LA(STRPTR, name, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 49, Exec)

/*  FUNCTION
	Find a task with a given name or get the address of the current task.
	Finding the address of the current task is a very quick function
	call, but finding a special task is a very CPU intensive instruction.
	Note that generally a task may already be gone when this function
	returns.

    INPUTS
	name - Pointer to name or NULL for current task.

    RESULT
	Address of task structure found.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    __AROS_FUNC_INIT

    struct Task *ret;

    /* Quick return for a quick argument */
    if(name==NULL)
	return SysBase->ThisTask;

    /* Always protect task lists with a Disable(). */
    Disable();

    /* First look into the ready list. */
    ret=(struct Task *)FindName(&SysBase->TaskReady,name);
    if(ret==NULL)
    {
	/* Then into the waiting list. */
	ret=(struct Task *)FindName(&SysBase->TaskWait,name);
	if(ret==NULL)
	{
	    /*
		Finally test the current task. Note that generally
		you know the name of your own task - so it is close
		to nonsense to look for it this way.
	    */
	    char *s1=SysBase->ThisTask->tc_Node.ln_Name;
	    char *s2=name;

	    /* Check as long as the names are identical. */
	    while(*s1++==*s2)
		/* Terminator found? */
		if(!*s2++)
		{
		    /* Got it. */
		    ret=SysBase->ThisTask;
		    break;
		}
	}
    }

    /* Return whatever I found. */
    Enable();
    return ret;
    __AROS_FUNC_EXIT
} /* FindTask */

