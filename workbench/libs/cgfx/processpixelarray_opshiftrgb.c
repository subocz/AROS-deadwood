/*
    Copyright � 2013-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <aros/debug.h>

#include <proto/cybergraphics.h>

#include <hidd/gfx.h>
#include <cybergraphx/cybergraphics.h>
#include <exec/types.h>

#include "cybergraphics_intern.h"

void ProcessPixelArrayShiftRGBFunc(struct RastPort *opRast, struct Rectangle *opRect, ULONG tintval, struct Library *CyberGfxBase)
{
    D(bug("[Cgfx] %s(0x%08x)\n", __PRETTY_FUNCTION__, tintval));
    D(bug("[Cgfx] %s: ShiftRGB operator not implemented\n", __PRETTY_FUNCTION__));
}
