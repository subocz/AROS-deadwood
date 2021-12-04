/*
    Copyright (C) 2012-2018, The AROS Development Team. All rights reserved.

    This file defines the private part of StdCBase.
    This should only be used internally in stdc.library code so
    changes can be made to this structure without breaking backwards
    compatibility.
*/
#ifndef __STDC_INTBASE_H
#define __STDC_INTBASE_H

#include <libraries/stdc.h>
#include <libraries/locale.h>
#include <intuition/intuitionbase.h>
#include <devices/timer.h>
#include <dos/bptr.h>
#include <dos/dos.h>

#include <time.h>

/* Some structs that are defined privately */
struct signal_func_data;

struct StdCIntBase
{
    struct StdCBase             StdCBase;
    struct StdCIntBase          *StdCRootBase;
    BPTR                        StdCSegList;

    struct DosLibrary           *StdCDOSBase;           // Needed for strerror->Fault
    /* optional libs */
    struct LocaleBase           *StdCLocaleBase;
    struct IntuitionBase        *StdCIntuitionBase;
    struct Device               *StdCTimerBase;

    /* stdlib.h */
    APTR                        mempool;
    unsigned int                srand_seed;

    /* time.h and it's functions */
    struct timerequest          timereq;
    struct MsgPort              timeport;
    char                        timebuffer[26];
    struct tm                   tmbuffer;
    struct DateStamp            starttime;

    /* signal.c & co. */
    struct signal_func_data     *sigfunc_array;

    /* strerror.c */
    char                        *fault_buf;

    /* strtok.c */
    char                        *last;

    /* private for fenv.c/softfloat implementations */
    APTR                        __fe_private;
    APTR                        __fe_dfl_env;
    APTR                        __fe_nomask_env;
    int                         __fe_round;
};

#endif //__STDC_INTBASE_H
