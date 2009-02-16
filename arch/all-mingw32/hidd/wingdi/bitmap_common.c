/*
    Copyright  1995-2008, The AROS Development Team. All rights reserved.
    $Id: bitmap_common.c 26918 2007-10-02 02:55:49Z rob $
*/

#include <exec/alerts.h>
#include <aros/macros.h>

/****************************************************************************************/

BOOL MNAME(Hidd_BitMap__SetColors)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SetColors *msg)
{
    struct bitmap_data  *data = OOP_INST_DATA(cl, o);
    HIDDT_PixelFormat 	*pf;    
    ULONG   	    	 xc_i, col_i;

    pf = BM_PIXFMT(o);

    if (vHidd_ColorModel_StaticPalette == HIDD_PF_COLMODEL(pf) ||
    	vHidd_ColorModel_TrueColor == HIDD_PF_COLMODEL(pf) )
    {	 
	 /* Superclass takes care of this case */

	 return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }

    /* Ve have a vHidd_GT_Palette bitmap */    

    if (!OOP_DoSuperMethod(cl, o, (OOP_Msg)msg)) return FALSE;
/*    
    if (data->flags & BMDF_COLORMAP_ALLOCED)
    {
    	LOCK_GDI	

	for ( xc_i = msg->firstColor, col_i = 0;
    	      col_i < msg->numColors; 
	      xc_i ++, col_i ++ )
	{
            XColor xcol;

	    xcol.red   = msg->colors[col_i].red;
	    xcol.green = msg->colors[col_i].green;
	    xcol.blue  = msg->colors[col_i].blue;
	    xcol.pad   = 0;
	    xcol.pixel = xc_i;
	    xcol.flags = DoRed | DoGreen | DoBlue;
	    
	    XCALL(XStoreColor, data->display, data->colmap, &xcol);

	}
	
    	UNLOCK_GDI	
	
    }*/ /* if (data->flags & BMDF_COLORMAP_ALLOCED) */

    return TRUE;
}

/****************************************************************************************/

VOID MNAME(Hidd_BitMap__PutPixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    APTR dc;
     
    LOCK_GDI
    dc = GET_DC(data);
    if (dc) {
    	GDICALL(SetPixel, dc, msg->x, msg->y, msg->pixel);
    	FREE_DC(data, dc);
    }
    UNLOCK_GDI
}

/****************************************************************************************/

HIDDT_Pixel MNAME(Hidd_BitMap__GetPixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    APTR dc;
    HIDDT_Pixel     	pixel;

    LOCK_GDI
    dc = GET_DC(data);
    if (dc) {
    	pixel = GDICALL(GetPixel, dc, msg->x, msg->y);
    	FREE_DC(data, dc);
    }
    UNLOCK_GDI

    return pixel;
}

/****************************************************************************************/

VOID MNAME(Hidd_BitMap__FillRect)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    APTR dc, br;
    ULONG col, mode;
        
    EnterFunc(bug("GDIGfx.BitMap::FillRect(%d,%d,%d,%d)\n",
    	          msg->minX, msg->minY, msg->maxX, msg->maxY));
    
    switch (GC_DRMD(msg->gc)) {
    case vHidd_GC_DrawMode_Clear:
        col = GC_BG(msg->gc);
    	mode = BLACKNESS;
    	break;
    case vHidd_GC_DrawMode_Copy:
        col = GC_FG(msg->gc);
    	mode = PATCOPY;
    	break;
    case vHidd_GC_DrawMode_NoOp:
        return;
    case vHidd_GC_DrawMode_Xor:
        col = GC_FG(msg->gc);
    	mode = PATINVERT;
    	break;
    case vHidd_GC_DrawMode_Equiv:
        col = !GC_FG(msg->gc);
    	mode = PATINVERT;
    	break;
    case vHidd_GC_DrawMode_Invert:
        col = 0xFFFFFFFF;
    	mode = PATINVERT;
    	break;
    case vHidd_GC_DrawMode_CopyInverted:
        col = !GC_FG(msg->gc);
    	mode = PATCOPY;
    	break;
    case vHidd_GC_DrawMode_Set:
        mode = WHITENESS;
    	break;
    default:
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    LOCK_GDI
    dc = GET_DC(data);
    if (dc) {
    	br = GDICALL(CreateSolidBrush, col);
    	if (br) {
    	    GDICALL(PatBlt, dc, msg->minX, msg->minY, msg->maxX - msg->minX + 1, msg->maxY - msg->minY + 1, mode);
    	    GDICALL(DeleteObject, br);
    	}
    	FREE_DC(data, dc);
    }
    UNLOCK_GDI    
    
    ReturnVoid("GDIGfx.BitMap::FillRect");    
}

/****************************************************************************************/

VOID MNAME(Hidd_BitMap__GetImage)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImage *msg)
{    
    ASSERT(msg->width > 0 && msg->height > 0);
    
/*  getimage_xlib(cl, o, msg->x, msg->y, msg->width, msg->height,
	    	       msg->pixels, (APTR (*)())ximage_to_buf, msg);*/
}

/****************************************************************************************/

VOID MNAME(Hidd_BitMap__GetImageLUT)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImageLUT *msg)
{
    ASSERT(msg->width != 0 && msg->height != 0);
/*  getimage_xlib(cl, o, msg->x, msg->y, msg->width, msg->height,
	    	      msg->pixels, (APTR (*)())ximage_to_buf_lut, msg);*/
}

/****************************************************************************************/

VOID MNAME(Hidd_BitMap__PutImage)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImage *msg)
{
    EnterFunc(bug("GDIGfx.BitMap::PutImage(pa=%p, x=%d, y=%d, w=%d, h=%d)\n",
    	    	  msg->pixels, msg->x, msg->y, msg->width, msg->height));
	
/*  putimage_xlib(cl, o, msg->gc, msg->x, msg->y,
	    	      msg->width, msg->height, msg->pixels,
		      (APTR (*)()) buf_to_ximage, msg);*/
    ReturnVoid("GDIGfx.BitMap::PutImage");
}

/****************************************************************************************/

VOID MNAME(Hidd_BitMap__PutImageLUT)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImageLUT *msg)
{
    EnterFunc(bug("GDIGfx.BitMap::PutImage(pa=%p, x=%d, y=%d, w=%d, h=%d)\n",
    	    	  msg->pixels, msg->x, msg->y, msg->width, msg->height));
	
/*  putimage_xlib(cl, o, msg->gc, msg->x, msg->y,
	    	      msg->width, msg->height, msg->pixels,
		      (APTR (*)())buf_to_ximage_lut, msg);*/
    ReturnVoid("GDIGfx.BitMap::PutImageLUT");
}

/****************************************************************************************/

VOID MNAME(Hidd_BitMap__BlitColorExpansion)(OOP_Class *cl, OOP_Object *o,
					    struct pHidd_BitMap_BlitColorExpansion *msg
)
{
    struct bitmap_data  *data = OOP_INST_DATA(cl, o);
/*  XImage  	    	*dest_im;*/
    HIDDT_Pixel     	 fg, bg;
    ULONG   	    	 cemd;
    LONG    	    	 x, y;    
/*  Drawable 	    	 d = 0;*/
    
    EnterFunc(bug("GDIGfx.BitMap::BlitColorExpansion(%p, %d, %d, %d, %d, %d, %d)\n",
    	    	  msg->srcBitMap, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height));
    
#ifdef NOT_YET
    OOP_GetAttr(msg->srcBitMap, aHidd_GDIBitMap_Drawable, (IPTR *)&d);
    
    if (0 == d)
    {
    	/* We know nothing about the source bitmap. Let the superclass handle this */
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	return;
    }
    
    fg = GC_FG(msg->gc);
    bg = GC_BG(msg->gc);
    cemd = GC_COLEXP(msg->gc);

    if (0 != d)
    {
    	LOCK_GDI    

	XCALL(XSetForeground, data->display, data->gc, fg);
		
    	if (cemd & vHidd_GC_ColExp_Opaque)  
	{
	    XCALL(XSetBackground, data->display, data->gc, bg);
	    XCALL(XSetFunction, data->display, data->gc, GXcopy);
	    
	    XCALL(XCopyPlane, data->display, d, DRAWABLE(data), data->gc,
	    	       msg->srcX, msg->srcY, msg->width, msg->height,
		       msg->destX, msg->destY, 0x01);
	}
	else
	{
	    /* Do transparent blit */
	    
	    XGCValues val;
	    
	    val.stipple		= d;
	    val.ts_x_origin	= msg->destX - msg->srcX;
	    val.ts_y_origin	= msg->destY - msg->srcY;
	    val.fill_style	= FillStippled;

	    XCALL(XSetFunction, data->display, data->gc, GC_DRMD(msg->gc));
	    
	    XCALL(XChangeGC, data->display, data->gc,
	    	      GCStipple|GCTileStipXOrigin|GCTileStipYOrigin|GCFillStyle,
		      &val);
		      
	    XCALL(XFillRectangle, data->display, DRAWABLE(data), data->gc,
	    	    	   msg->destX, msg->destY, msg->width, msg->height);
	    
	    XCALL(XSetFillStyle, data->display, data->gc, FillSolid);

	}
	
    	UNLOCK_GDI	

    }
    else
    {
    	/* We know nothing about the format of the source bitmap
	   an must get single pixels
	*/

    	LOCK_GDI    
	dest_im = XCALL(XGetImage, data->display, DRAWABLE(data),
	    	    	    msg->destX, msg->destY, msg->width, msg->height,
			    AllPlanes, ZPixmap);    	
    	UNLOCK_GDI   
	 
	if (!dest_im)
    	    ReturnVoid("GDIGfx.BitMap::BlitColorExpansion()");

	D(bug("Src bm: %p\n", msg->srcBitMap));
	
	for (y = 0; y < msg->height; y ++)
	{
	    for (x = 0; x < msg->width; x ++)
	    {
		ULONG is_set;
	    
	    	is_set = HIDD_BM_GetPixel(msg->srcBitMap, x + msg->srcX, y + msg->srcY);
	    
	    	if (is_set)
	    	{
	    	    XPutPixel(dest_im, x, y, fg);
		
	    	}
	    	else
	    	{
		    if (cemd & vHidd_GC_ColExp_Opaque)
		    {
			XPutPixel(dest_im, x, y, bg);
		    }
		}
		
	    } /* for (each x) */
	    
    	} /* for (each y) */
    
	/* Put image back into display */

    	LOCK_GDI    
	XCALL(XSetFunction, data->display, data->gc, GC_DRMD(msg->gc));
	XCALL(XPutImage, data->display, DRAWABLE(data), data->gc, dest_im,
	    	  0, 0, msg->destX, msg->destY, msg->width, msg->height);
    	XDestroyImage(dest_im);
    	UNLOCK_GDI
    }

    LOCK_GDI
    XFLUSH(data->display);
    UNLOCK_GDI 
#endif
    ReturnVoid("GDIGfx.BitMap::BlitColorExpansion");
}

/****************************************************************************************/

VOID MNAME(Root__Get)(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    ULONG   	    	idx;
    
    if (IS_GDIBM_ATTR(msg->attrID, idx))
    {
	switch (idx)
	{
	    case aoHidd_GDIBitMap_Drawable:
	    	*msg->storage = (IPTR)data->drawable;
		break;

	    case aoHidd_GDIBitMap_MasterWindow:
	        *msg->storage = (IPTR)data->drawable;
		break;
		
	    default:
	    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
		break;
	}
    }
    else
    {
    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    
}

/****************************************************************************************/

VOID MNAME(Hidd_BitMap__DrawLine)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawLine *msg)
{
/*  struct bitmap_data  *data = OOP_INST_DATA(cl, o);
    OOP_Object      	*gc = msg->gc;
    
    if (GC_LINEPAT(gc) != (UWORD)~0)
    {*/
    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	
	return;
/*  }
    
    LOCK_GDI
    
    if (GC_DOCLIP(gc))
    {
    	XRectangle cr;
	
	cr.x = GC_CLIPX1(gc);
	cr.y = GC_CLIPY1(gc);
	cr.width  = GC_CLIPX2(gc) - cr.x + 1;
	cr.height = GC_CLIPY2(gc) - cr.y + 1;
    
    	XCALL(XSetClipRectangles, data->display, data->gc,
	    	    	   0, 0, &cr, 1, Unsorted);
    }
    
    XCALL(XSetForeground, data->display, data->gc, GC_FG(gc));
    XCALL(XSetFunction, data->display, data->gc, GC_DRMD(gc));
    
    XCALL(XDrawLine, data->display, DRAWABLE(data), data->gc,
    	      msg->x1, msg->y1, msg->x2, msg->y2);
	
    if (GC_DOCLIP(gc))
    {
    	XCALL(XSetClipMask, data->display, data->gc, None);
    }	
    
    XFLUSH(data->display);
    
    UNLOCK_GDI*/
}

/****************************************************************************************/

VOID MNAME(Hidd_BitMap__DrawEllipse)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawEllipse *msg)
{
/*  struct bitmap_data  *data = OOP_INST_DATA(cl, o);
    OOP_Object      	*gc = msg->gc;
    
    LOCK_GDI
    
    if (GC_DOCLIP(gc))
    {
    	XRectangle cr;
	
    	D(kprintf("GDI::Drawllipse: clip %d %d %d %d\n"
	    	    , GC_CLIPX1(gc), GC_CLIPY1(gc), GC_CLIPX2(gc), GC_CLIPY2(gc)));
		
	cr.x = GC_CLIPX1(gc);
	cr.y = GC_CLIPY1(gc);
	cr.width  = GC_CLIPX2(gc) - cr.x + 1;
	cr.height = GC_CLIPY2(gc) - cr.y + 1;
    
    	XCALL(XSetClipRectangles, data->display, data->gc,
	    	    	   0, 0, &cr, 1, Unsorted);
    }
    
    XCALL(XSetForeground, data->display, data->gc, GC_FG(gc));
    XCALL(XSetFunction, data->display, data->gc, GC_DRMD(gc));
    
    D(kprintf("GDI::Drawllipse: coord %d %d %d %d\n"
	    	, msg->x, msg->y, msg->rx, msg->ry));
    
    XCALL(XDrawArc, data->display, DRAWABLE(data), data->gc,
    	     msg->x - msg->rx, msg->y - msg->ry,
	     msg->rx * 2, msg->ry * 2, 0, 360 * 64);
	
    if (GC_DOCLIP(gc))
    {
    	XCALL(XSetClipMask, data->display, data->gc, None);
    }	
    
    XFLUSH(data->display);
    
    UNLOCK_GDI*/
}

/****************************************************************************************/
