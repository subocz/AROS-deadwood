#ifndef HIDD_GRAPHICS_H
#define HIDD_GRAPHICS_H

/*
    Copyright (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Definitions for the Gfx HIDD system.
    Lang: english
*/
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef HIDD_HIDD_H
#   include <hidd/hidd.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif
#include <utility/tagitem.h>


#define CLID_Hidd_Gfx           "gfx.hidd"
#define CLID_Hidd_BitMap        "bitmap.hidd"

#define IID_Hidd_Gfx    "I_GfxHidd"
#define IID_Hidd_GC     "I_HiddGC"
#define IID_Hidd_BitMap "I_HiddBitMap"

typedef struct Object *HIDDT_BitMap;
typedef struct Object *HIDDT_GC;


/* Attrbases */
#define HiddGCAttrBase          __IHidd_GC
#define HiddGfxAttrBase         __IHidd_Gfx
#define HiddBitMapAttrBase      __IHidd_BitMap

extern AttrBase HiddGCAttrBase;
extern AttrBase HiddGfxAttrBase;
extern AttrBase HiddBitMapAttrBase;


/***************
**  Gfx defs  **
***************/


    /* Method offsets */
enum
{

    moHidd_SpeedTest = 0,         /* method for speedtest */
    moHidd_Gfx_CreateGC,
    moHidd_Gfx_DeleteGC,
};


    /* Attr offsets */
enum {
    /* Attributes for a bitmap */
    aoHidd_BitMap_BitMap,  /* [..G] pointer to bitmap structure        */
    aoHidd_BitMap_Width,         /* [ISG] Bitmap with                        */
    aoHidd_BitMap_Height,        /* [ISG] Bitmap height                      */
    aoHidd_BitMap_Depth,         /* [I.G] Bitmap depth                       */
    aoHidd_BitMap_Displayable,   /* [I.G] BOOL bitmap is displayable         */
    aoHidd_BitMap_Visible,       /* [..G] Check if a bitmap is visible       */
    aoHidd_BitMap_Mode,          /* [ISG] The display mode of this bitmap    */
    aoHidd_BitMap_BaseAddress,   /* [ISG] Bitmap adress in RAM               */
    aoHidd_BitMap_Format,        /* [..G] Tell the format of the bitmap data */
    aoHidd_BitMap_BytesPerRow,   /* [..G] Number of bytes in a row           */
    aoHidd_BitMap_BytesPerPixel, /* [..G] Number of byter per pixel          */
    aoHidd_BitMap_BestSize,      /* [..G] Best size for depth                */
    aoHidd_BitMap_LeftEdge,      /* [I.G] Left edge position of the bitmap   */
    aoHidd_BitMap_TopEdge,       /* [I.G] Top edge position of the bitmap    */
    
    num_Hidd_BitMap_Attrs
};    

#define aHidd_BitMap_BitMap        (HiddBitMapAttrBase + aoHidd_BitMap_BitMap)
#define aHidd_BitMap_Width         (HiddBitMapAttrBase + aoHidd_BitMap_Width)
#define aHidd_BitMap_Height        (HiddBitMapAttrBase + aoHidd_BitMap_Height)
#define aHidd_BitMap_Depth         (HiddBitMapAttrBase + aoHidd_BitMap_Depth)
#define aHidd_BitMap_Displayable   (HiddBitMapAttrBase + aoHidd_BitMap_Displayable)
#define aHidd_BitMap_Visible       (HiddBitMapAttrBase + aoHidd_BitMap_Visible)
#define aHidd_BitMap_Mode          (HiddBitMapAttrBase + aoHidd_BitMap_Mode)
#define aHidd_BitMap_BaseAddress   (HiddBitMapAttrBase + aoHidd_BitMap_BaseAddress)
#define aHidd_BitMap_Format        (HiddBitMapAttrBase + aoHidd_BitMap_Format)
#define aHidd_BitMap_BytesPerRow   (HiddBitMapAttrBase + aoHidd_BitMap_BytesPerRow)
#define aHidd_BitMap_BytesPerPixel (HiddBitMapAttrBase + aoHidd_BitMap_BytesPerPixel)
#define aHidd_BitMap_BestSize      (HiddBitMapAttrBase + aoHidd_BitMap_BestSize)
#define aHidd_BitMap_LeftEdge      (HiddBitMapAttrBase + aoHidd_BitMap_LeftEdge)
#define aHidd_BitMap_TopEdge       (HiddBitMapAttrBase + aoHidd_BitMap_TopEdge)


/* BitMap formats */
#define HIDDV_BitMap_Format_Planar   0x1
#define HIDDV_BitMap_Format_Chunky   0x2


/* Drawmodes for a graphics context */
#define HIDDV_GC_DrawMode_Copy 0x03 /* Copy src into destination            */
#define HIDDV_GC_DrawMode_XOR  0x06 /* XOR                                  */




/* Messages */
struct hGfx_Point
{
    STACKULONG MethodID;
    STACKWORD  x, y;        /* Position in hidd units */
};


struct hGfx_PixelDirect
{
    STACKULONG MethodID;
    STACKWORD  x, y;        /* Position in hidd units  */
    STACKULONG val;         /* set pixel to this value */
};


struct hGfx_2Coords
{
    STACKULONG MethodID;
    STACKWORD  x1, y1;      /* eg. draw line from (x1, y1) to (x1,y2) */
    STACKWORD  x2, y2;
};


/* Message for HIDDM_Gfx_SpeedTest */
struct hGfx_SpeeTest
{
    STACKULONG  MethodID;
    STACKULONG  val1;
    STACKULONG  val2;
    STACKULONG  val3;
};


struct pHidd_Gfx_CreateGC
{
     MethodID   mID;
     Object     *bitMap;
     UWORD      gcType;
};
struct pHidd_Gfx_DeleteGC
{
    MethodID    mID;
    Object      *gc;
};
enum { GCTYPE_QUICK, GCTYPE_CLIPPING };


/**************
**  GC Defs  **
**************/

#define CLID_Hidd_QuickGC "quickgc.hidd"
#define CLID_Hidd_ClipGC  "clipgc.hidd"

    /* graphics context */
enum
{    
    moHidd_GC_CopyArea,
    moHidd_GC_WritePixelDirect,
    moHidd_GC_WritePixel,
    moHidd_GC_ReadPixel,
    moHidd_GC_DrawLine,
    moHidd_GC_DrawRect,
    moHidd_GC_FillRect,
    moHidd_GC_DrawEllipse,
    moHidd_GC_FillEllipse,
    moHidd_GC_DrawPolygon,
    moHidd_GC_FillPolygon,
    moHidd_GC_DrawText,
    moHidd_GC_FillText,
    moHidd_GC_FillSpan,
    moHidd_GC_Clear,
};

enum
{
    /* Attributes for a graphics context */
    aoHidd_GC_UserData,            /* [.SG] User data                          */
    aoHidd_GC_BitMap,              /* [I.G] Bitmap which this gc uses          */
    aoHidd_GC_Foreground,          /* [.SG] Foreground color                   */
    aoHidd_GC_Background,          /* [.SG] Background color                   */
    aoHidd_GC_DrawMode,            /* [.SG] Draw mode                          */
    aoHidd_GC_Font,                /* [.SG] Current font                       */
    aoHidd_GC_ColorMask,           /* [.SG] Prevents some color bits from      */
                                   /*       changing                           */
    aoHidd_GC_LinePattern,         /* [.SG] Pattern foor line drawing          */
    aoHidd_GC_PlaneMask,           /* [.SG] Shape bitmap                       */
    
    num_Hidd_GC_Attrs
};

#define aHidd_GC_UserData    (HiddGCAttrBase + aoHidd_GC_UserData)
#define aHidd_GC_BitMap      (HiddGCAttrBase + aoHidd_GC_BitMap)
#define aHidd_GC_Foreground  (HiddGCAttrBase + aoHidd_GC_Foreground)
#define aHidd_GC_Background  (HiddGCAttrBase + aoHidd_GC_Background)
#define aHidd_GC_DrawMode    (HiddGCAttrBase + aoHidd_GC_DrawMode)
#define aHidd_GC_Font        (HiddGCAttrBase + aoHidd_GC_Font)
#define aHidd_GC_ColorMask   (HiddGCAttrBase + aoHidd_GC_ColorMask)
#define aHidd_GC_LinePattern (HiddGCAttrBase + aoHidd_GC_LinePattern)
#define aHidd_GC_PlaneMask   (HiddGCAttrBase + aoHidd_GC_PlaneMask)


    /* Messages */
struct pHidd_GC_DrawLine
{
    MethodID    mID;
    WORD        x1 ,y1, x2, y2;
};

struct pHidd_GC_WritePixel
{
    MethodID  mID;
    WORD x, y;
};

struct pHidd_GC_WritePixelDirect
{
    MethodID  mID;
    WORD x, y;
    ULONG val;
};

#endif /* HIDD_GRAPHICS_H */
