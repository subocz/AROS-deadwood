/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Intuition function ClearPointer
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH1(void, ClearPointer,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, window, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 10, Intuition)

/*  FUNCTION
	Reset the mousepointer of this window to the default one.
	If the window is active during this call the pointer will
	immediately change its shape.
	Set custom mousepointers with SetPointer().

    INPUTS
	window - The window of which the mousepointer will be cleared

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	SetPointer()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    if( window )
    {
	window->Pointer = NULL;
    }

    /* Tell driver to reset mouse pointer immediately */

#warning TODO: Write intuition/ClearPointer()
    aros_print_not_implemented ("ClearPointer");

    return;

    AROS_LIBFUNC_EXIT
} /* ClearPointer */
