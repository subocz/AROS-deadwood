/*
    Copyright (C) 1995-2013, The AROS Development Team. All rights reserved.
*/

#include "__stdc_intbase.h"

#include <aros/symbolsets.h>
#include <exec/lists.h>
#include "__exitfunc.h"

int __addexitfunc(struct AtExitNode *aen)
{
    struct StdCIntBase *StdCBase =
        (struct StdCIntBase *)__aros_getbase_StdCBase();
    
    ADDHEAD((struct List *)&StdCBase->atexit_list, (struct Node *)aen);

    return 0;
}

#include "../posixc/__crtext_intbase.h"

int __init_atexit(struct CrtExtIntBase *CrtExtBase)
{
    struct StdCIntBase *StdCBase = CrtExtBase->StdCBase;

    NEWLIST((struct List *)&StdCBase->atexit_list);

    return 1;
}

void __progonly_callexitfuncs(void)
{
    struct StdCIntBase *StdCBase =
        (struct StdCIntBase *)__aros_getbase_StdCBase();
    struct AtExitNode *aen;

    while (
        (aen = (struct AtExitNode *) REMHEAD((struct List *) &StdCBase->atexit_list))
    )
    {
        switch (aen->node.ln_Type)
        {
        case AEN_VOID:
            aen->func.fvoid();
            break;

        case AEN_PTR:
            {
                int *errorptr = __progonly_get_errorptr();
                aen->func.fptr(errorptr != NULL ? *errorptr : 0, aen->ptr);
            }
            break;
        }
    }
}

ADD2OPENLIB(__init_atexit, 100);
