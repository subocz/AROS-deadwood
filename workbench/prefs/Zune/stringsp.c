/*
    Copyright (C) 2003-2006, The AROS Development Team. All rights reserved.
*/

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <mui/Rawimage_mcc.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#ifdef __AROS__
#include <proto/alib.h>
#endif

#include "zunestuff.h"
#include <string.h>

/*  #define DEBUG 1 */
/*  #include <aros/debug.h> */

extern struct Library *MUIMasterBase;

struct MUI_StringsPData
{
    Object *string_popframe;
    Object *popup_popimage;
    Object *popfile_popimage;
    Object *popdrawer_popimage;
    Object *inactive_bg_popimage;
    Object *inactive_text_poppen;
    Object *active_bg_popimage;
    Object *active_text_poppen;
    Object *cursor_poppen;
    Object *marked_bg_poppen;
    Object *marked_text_poppen;
};

Object *MakePopupPopimage(CONST_STRPTR title)
{
    return MUI_NewObject
	(
	    MUIC_Popimage,
	    MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_Image,
	    MUIA_CycleChain, 1,
	    MUIA_FixWidth, 28,
	    MUIA_MaxHeight, 28,
	    MUIA_Imagedisplay_FreeHoriz, FALSE,
	    MUIA_Imagedisplay_FreeVert, FALSE,
	    MUIA_Window_Title, (IPTR)title,
	    TAG_DONE
	    );
}

static IPTR StringsP_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_StringsPData *data;
    struct MUI_StringsPData d;
    
    obj = (Object *) DoSuperNewTags
    (
        cl, obj, NULL,
	MUIA_Group_Horiz, FALSE,
	Child, (IPTR) HGroup,
	Child, (IPTR) VGroup,
	Child, (IPTR) HGroup,
	GroupFrameT(_(MSG_STRING_FRAME)),
	Child, (IPTR) (d.string_popframe = MakePopframe()),
	End, // String Frame
	Child, HGroup,
	GroupFrameT(_(MSG_SPECIAL_POPUP_BUTTONS)),
        MUIA_Group_SameWidth, TRUE,
	Child, (IPTR) VGroup,
                MUIA_Group_VertSpacing, 1,
                Child, (IPTR) (d.popup_popimage = MakePopupPopimage(_(MSG_POPUP))),
                Child, (IPTR) CLabel(_(MSG_DEFAULT)),
            End,
            Child, (IPTR) VGroup,
                MUIA_Group_VertSpacing, 1,
                Child, (IPTR) (d.popfile_popimage = MakePopupPopimage(_(MSG_POPUP_FILE))),
                Child, (IPTR) CLabel(_(MSG_FILE)),
            End,
            Child, (IPTR) VGroup,
                MUIA_Group_VertSpacing, 1,
                Child, (IPTR) (d.popdrawer_popimage = MakePopupPopimage(_(MSG_POPUP_DRAWER))),
                Child, (IPTR) CLabel(_(MSG_DRAWER)),
            End,
	End, // Special Popup Buttons
	End, // VGroup Left
	Child, VGroup,
	Child, HGroup,
	GroupFrameT(_(MSG_INACTIVE_STRING_COLORS)),
        MUIA_Group_SameWidth, TRUE,
	Child, (IPTR) VGroup,
                MUIA_Group_VertSpacing, 1,
                Child, (IPTR) (d.inactive_bg_popimage = MakeBackgroundPopimage()),
                Child, (IPTR) CLabel(_(MSG_BACKGROUND)),
            End,
            Child, (IPTR) VGroup,
                MUIA_Group_VertSpacing, 1,
                Child, (IPTR) (d.inactive_text_poppen = MakePoppen()),
                Child, (IPTR) CLabel(_(MSG_TEXT)),
            End,
	End, // Inactive String Colors
	Child, HGroup,
	GroupFrameT(_(MSG_ACTIVE_STRING_COLORS)),
        MUIA_Group_SameWidth, TRUE,
	Child, (IPTR) VGroup,
                MUIA_Group_VertSpacing, 1,
                Child, (IPTR) (d.active_bg_popimage = MakeBackgroundPopimage()),
                Child, (IPTR) CLabel(_(MSG_BACKGROUND)),
            End,
            Child, (IPTR) VGroup,
                MUIA_Group_VertSpacing, 1,
                Child, (IPTR) (d.active_text_poppen = MakePoppen()),
                Child, (IPTR) CLabel(_(MSG_TEXT)),
            End,
	End, // Inactive String Colors
	Child, HGroup,
	    Child, HGroup,
	    GroupFrameT(_(MSG_CURSOR)),
	    Child, (IPTR) (d.cursor_poppen = MakePoppen()),
	    End, // Cursor
	    Child, HGroup,
	    GroupFrameT(_(MSG_MARKED_STRING_COLORS)),
	    MUIA_Group_SameWidth, TRUE,
	    Child, (IPTR) VGroup,
	        MUIA_Group_VertSpacing, 1,
	        Child, (IPTR) (d.marked_bg_poppen = MakePoppen()),
	        Child, (IPTR) CLabel(_(MSG_BACKGROUND)),
	        End,
	    Child, (IPTR) VGroup,
	        MUIA_Group_VertSpacing, 1,
	        Child, (IPTR) (d.marked_text_poppen = MakePoppen()),
	        Child, (IPTR) CLabel(_(MSG_TEXT)),
	        End,
	    End, // Marked String Colors
	End, // Cursor + Marked

	End,
        End,
	Child, (IPTR) StringObject,
	StringFrame,
	MUIA_CycleChain, 1,
	MUIA_String_Contents, _(MSG_EXAMPLE_STRING_GADGET),
	MUIA_String_Format, MUIV_String_Format_Center,
	End,
    	TAG_MORE, (IPTR) msg->ops_AttrList);
	
    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    *data = d;

    return (IPTR)obj;
}


/*
 * MUIM_Settingsgroup_ConfigToGadgets
 */
static IPTR StringsP_ConfigToGadgets(struct IClass *cl, Object *obj,
				    struct MUIP_Settingsgroup_ConfigToGadgets *msg)
{
    struct MUI_StringsPData *data = INST_DATA(cl, obj);
    STRPTR spec;

/* Frames */
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Frame_String);
    set(data->string_popframe, MUIA_Framedisplay_Spec, (IPTR)spec);

/* Looks */
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Image_PopUp);
    set(data->popup_popimage, MUIA_Imagedisplay_Spec, (IPTR)spec);
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Image_PopFile);
    set(data->popfile_popimage, MUIA_Imagedisplay_Spec, (IPTR)spec);
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Image_PopDrawer);
    set(data->popdrawer_popimage, MUIA_Imagedisplay_Spec, (IPTR)spec);

/* pens & images */
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_String_Background);
    set(data->inactive_bg_popimage, MUIA_Imagedisplay_Spec, (IPTR)spec);
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_String_Text);
    set(data->inactive_text_poppen, MUIA_Pendisplay_Spec, (IPTR)spec);

    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_String_ActiveBackground);
    set(data->active_bg_popimage, MUIA_Imagedisplay_Spec, (IPTR)spec);
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_String_ActiveText);
    set(data->active_text_poppen, MUIA_Pendisplay_Spec, (IPTR)spec);

    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_String_Cursor);
    set(data->cursor_poppen, MUIA_Pendisplay_Spec, (IPTR)spec);

    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_String_MarkedText);
    set(data->marked_text_poppen, MUIA_Pendisplay_Spec, (IPTR)spec);

    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_String_MarkedBackground);
    set(data->marked_bg_poppen, MUIA_Pendisplay_Spec, (IPTR)spec);
    return 1;    
}


/*
 * MUIM_Settingsgroup_ConfigToGadgets
 */
static IPTR StringsP_GadgetsToConfig(struct IClass *cl, Object *obj,
				    struct MUIP_Settingsgroup_GadgetsToConfig *msg)
{
    struct MUI_StringsPData *data = INST_DATA(cl, obj);
    STRPTR str;

/* Frames */
    str = (STRPTR)XGET(data->string_popframe, MUIA_Framedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetFramespec, MUICFG_Frame_String,
	     (IPTR)str);

/* Looks */
    str = (STRPTR)XGET(data->popup_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Image_PopUp,
	     (IPTR)str);
    str = (STRPTR)XGET(data->popfile_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Image_PopFile,
	     (IPTR)str);
    str = (STRPTR)XGET(data->popdrawer_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Image_PopDrawer,
	     (IPTR)str);

/* pens & images */
    str = (STRPTR)XGET(data->inactive_bg_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetPenspec,
	     MUICFG_String_Background, (IPTR)str);
    str = (STRPTR)XGET(data->inactive_text_poppen, MUIA_Pendisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetPenspec,
	     MUICFG_String_Text, (IPTR)str);

    str = (STRPTR)XGET(data->active_bg_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetPenspec,
	     MUICFG_String_ActiveBackground, (IPTR)str);
    str = (STRPTR)XGET(data->active_text_poppen, MUIA_Pendisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetPenspec,
	     MUICFG_String_ActiveText, (IPTR)str);

    str = (STRPTR)XGET(data->cursor_poppen, MUIA_Pendisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetPenspec,
	     MUICFG_String_Cursor, (IPTR)str);

    str = (STRPTR)XGET(data->marked_text_poppen, MUIA_Pendisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetPenspec,
	     MUICFG_String_MarkedText, (IPTR)str);

    str = (STRPTR)XGET(data->marked_bg_poppen, MUIA_Pendisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetPenspec,
	     MUICFG_String_MarkedBackground, (IPTR)str);

    return TRUE;
}


BOOPSI_DISPATCHER(IPTR, StringsP_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return StringsP_New(cl, obj, (struct opSet *)msg);
	case MUIM_Settingsgroup_ConfigToGadgets: return StringsP_ConfigToGadgets(cl,obj,(APTR)msg);break;
	case MUIM_Settingsgroup_GadgetsToConfig: return StringsP_GadgetsToConfig(cl,obj,(APTR)msg);break;
    }
    
    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUIP_Strings_desc = { 
    "Strings",
    MUIC_Group,
    sizeof(struct MUI_StringsPData),
    (void*)StringsP_Dispatcher 
};


static const UBYTE icon32[] =
{
    0x00, 0x00, 0x00, 0x18,  // width
    0x00, 0x00, 0x00, 0x13,  // height
    'B', 'Z', '2', '\0',
    0x00, 0x00, 0x00, 0x78,  // number of bytes

    0x42, 0x5a, 0x68, 0x39, 0x31, 0x41, 0x59, 0x26, 0x53, 0x59, 0xe5, 0x44, 
    0x0a, 0x8c, 0x00, 0x02, 0x69, 0xf5, 0x22, 0xa2, 0x22, 0x22, 0x00, 0x00, 
    0x20, 0x02, 0x00, 0x20, 0x00, 0x00, 0x40, 0x42, 0x40, 0x00, 0x00, 0xa0, 
    0x00, 0x90, 0x20, 0x00, 0x00, 0x88, 0x94, 0xd4, 0xcd, 0x46, 0xd4, 0xe2, 
    0xaa, 0x6c, 0x15, 0x67, 0x9d, 0xe8, 0xf9, 0x9b, 0xd6, 0x52, 0xa0, 0x3d, 
    0x77, 0x69, 0x68, 0xf4, 0x40, 0x86, 0x79, 0x99, 0x85, 0xe9, 0x81, 0x03, 
    0x67, 0xd1, 0x10, 0x7d, 0x14, 0x43, 0x82, 0x88, 0x69, 0x51, 0x4e, 0x73, 
    0x50, 0x10, 0x80, 0xc8, 0x25, 0xb6, 0x89, 0x14, 0x21, 0x04, 0x08, 0x01, 
    0x54, 0x92, 0xd5, 0x45, 0xb8, 0x20, 0x19, 0x48, 0x84, 0x20, 0x92, 0x42, 
    0x11, 0xfe, 0x2e, 0xe4, 0x8a, 0x70, 0xa1, 0x21, 0xca, 0x88, 0x15, 0x18, 
};


Object *stringsclass_get_icon(void)
{
    return RawimageObject,
        MUIA_Rawimage_Data, icon32,
    End;
}
