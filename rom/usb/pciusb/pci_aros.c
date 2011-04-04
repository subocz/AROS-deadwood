/* pci_aros.c - pci access abstraction for AROS by Chris Hodges
*/

#include "uhwcmd.h"

#include <inttypes.h>

#include <aros/symbolsets.h>

#include <exec/types.h>
#include <oop/oop.h>

#include <devices/timer.h>

#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <hidd/irq.h>

#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/exec.h>

// FIXME due to the data structures defined in uhcichip.h, ohcichip.h and ehcichip.h,
// and their alignments, the use of 64 bit pointers will break these alignments.
// Moreover, to correctly support 64 bit, the PCI card needs to be 64 bit capable.
// otherwise the data needs to be copied from and to 32 bit space. Such mechanisms
// are currently not implemented in pciusb.device at all.
#warning "pciusb.device currently will NOT work under 64 bit! Don't try this!"

#define NewList NEWLIST

#undef HiddPCIDeviceAttrBase
//#undef HiddUSBDeviceAttrBase
//#undef HiddUSBHubAttrBase
//#undef HiddUSBDrvAttrBase
#undef HiddAttrBase

#define HiddPCIDeviceAttrBase (hd->hd_HiddPCIDeviceAB)
//#define HiddUSBDeviceAttrBase (hd->hd_HiddUSBDeviceAB)
//#define HiddUSBHubAttrBase (hd->hd_HiddUSBHubAB)
//#define HiddUSBDrvAttrBase (hd->hd_HiddUSBDrvAB)
#define HiddAttrBase (hd->hd_HiddAB)

AROS_UFH3(void, pciEnumerator,
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(OOP_Object *, pciDevice, A2),
          AROS_UFHA(APTR, message, A1))
{
    AROS_USERFUNC_INIT

    struct PCIDevice *hd = (struct PCIDevice *) hook->h_Data;
    struct PCIController *hc;
    IPTR hcitype;
    IPTR dev;
    IPTR bus;
    IPTR intline;
    ULONG devid;

    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Interface, &hcitype);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Bus, &bus);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Dev, &dev);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_INTLine, &intline);

    devid = (bus<<16)|dev;

    if(intline == 255)
    {
        // we can't work without the correct interrupt line
        // BIOS needs plug & play os option disabled. Alternatively AROS must support APIC reconfiguration
        KPRINTF(200, ("ERROR: PCI card has no interrupt line assigned by BIOS, disable Plug & Play OS!\n"));
    }

#if defined(__powerpc__)
    else if((hcitype == HCITYPE_OHCI))
#elif defined(USB3) 
    else if((hcitype == HCITYPE_UHCI) || (hcitype == HCITYPE_OHCI) || (hcitype == HCITYPE_EHCI) || (hcitype == HCITYPE_XHCI))
#else
    else if((hcitype == HCITYPE_UHCI) || (hcitype == HCITYPE_OHCI) || (hcitype == HCITYPE_EHCI))
#endif
    {
        KPRINTF(10, ("Found PCI device 0x%lx of type %ld, Intline=%ld\n", devid, hcitype, intline));

        hc = AllocPooled(hd->hd_MemPool, sizeof(struct PCIController));
        if(hc)
        {
            hc->hc_Device = hd;
            hc->hc_DevID = devid;
            hc->hc_HCIType = hcitype;
            hc->hc_PCIDeviceObject = pciDevice;
            hc->hc_PCIIntLine = intline;

            OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (IPTR *) &hc->hc_PCIDriverObject);

            NewList(&hc->hc_CtrlXFerQueue);
            NewList(&hc->hc_IntXFerQueue);
            NewList(&hc->hc_IsoXFerQueue);
            NewList(&hc->hc_BulkXFerQueue);
            NewList(&hc->hc_TDQueue);
            NewList(&hc->hc_PeriodicTDQueue);
            NewList(&hc->hc_OhciRetireQueue);
            AddTail(&hd->hd_TempHCIList, &hc->hc_Node);
        }
    }

    AROS_USERFUNC_EXIT
}

/* /// "UhciResetHandler()" */
static AROS_UFH3(void, UhciResetHandler,
                 AROS_UFHA(struct PCIController *, hc, A1),
                 AROS_UFHA(APTR, unused, A5),
                 AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    // stop controller and disable all interrupts
    WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, 0);
    WRITEIO16_LE(hc->hc_RegBase, UHCI_USBINTEN, 0);

    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "pciInit()" */
BOOL pciInit(struct PCIDevice *hd)
{
    struct PCIController *hc;
    struct PCIController *nexthc;
    struct PCIUnit *hu;
    ULONG unitno = 0;
    UWORD ohcicnt;
    UWORD uhcicnt;

    KPRINTF(10, ("*** pciInit(%08lx) ***\n", hd));
    if(sizeof(IPTR) > 4)
    {
        KPRINTF(200, ("I said the pciusb.device is not 64bit compatible right now. Go away!\n"));
        return FALSE;
    }

    NewList(&hd->hd_TempHCIList);

    if(!(hd->hd_IRQHidd = OOP_NewObject(NULL, (STRPTR) CLID_Hidd_IRQ, NULL)))
    {
        KPRINTF(20, ("Unable to create IRQHidd object!\n"));
        return FALSE;
    }

    if((hd->hd_PCIHidd = OOP_NewObject(NULL, (STRPTR) CLID_Hidd_PCI, NULL)))
    {
        struct TagItem tags[] =
        {
            { tHidd_PCI_Class,      (PCI_CLASS_SERIAL_USB>>8) & 0xff },
            { tHidd_PCI_SubClass,   (PCI_CLASS_SERIAL_USB & 0xff) },
            { TAG_DONE, 0UL }
        };

        struct OOP_ABDescr attrbases[] =
        {
            { (STRPTR) IID_Hidd,            &hd->hd_HiddAB },
            { (STRPTR) IID_Hidd_PCIDevice,  &hd->hd_HiddPCIDeviceAB },
//            { (STRPTR) IID_Hidd_USBDevice,  &hd->hd_HiddUSBDeviceAB },
//            { (STRPTR) IID_Hidd_USBHub,     &hd->hd_HiddUSBHubAB },
//            { (STRPTR) IID_Hidd_USBDrv,     &hd->hd_HiddUSBDrvAB },
            { NULL, NULL }
        };

        struct Hook findHook =
        {
             h_Entry:        (IPTR (*)()) pciEnumerator,
             h_Data:         hd,
        };

        OOP_ObtainAttrBases(attrbases);

#if defined(__powerpc__)
        KPRINTF(20, ("Searching for OHCI devices...\n"));
#elif defined(USB3)
        KPRINTF(20, ("Searching for (U/O/E/X)HCI devices...\n"));
#else
        KPRINTF(20, ("Searching for (U/O/E)HCI devices...\n"));
#endif

        HIDD_PCI_EnumDevices(hd->hd_PCIHidd, &findHook, (struct TagItem *) &tags);
    } else {
        KPRINTF(20, ("Unable to create PCIHidd object!\n"));
        OOP_DisposeObject(hd->hd_IRQHidd);
        return FALSE;
    }

    while(hd->hd_TempHCIList.lh_Head->ln_Succ)
    {
        hu = AllocPooled(hd->hd_MemPool, sizeof(struct PCIUnit));
        if(!hu)
        {
            // actually, we should get rid of the allocated memory first, but I don't care as DeletePool() will take care of this eventually
            return FALSE;
        }
        hu->hu_Device = hd;
        hu->hu_UnitNo = unitno;
        hu->hu_DevID = ((struct PCIController *) hd->hd_TempHCIList.lh_Head)->hc_DevID;

        NewList(&hu->hu_Controllers);
        NewList(&hu->hu_RHIOQueue);
        ohcicnt = 0;
        uhcicnt = 0;
        // find all belonging host controllers
        hc = (struct PCIController *) hd->hd_TempHCIList.lh_Head;
        while((nexthc = (struct PCIController *) hc->hc_Node.ln_Succ))
        {
            if(hc->hc_DevID == hu->hu_DevID)
            {
                Remove(&hc->hc_Node);
                hc->hc_Unit = hu;
                if(hc->hc_HCIType == HCITYPE_UHCI)
                {
                    hc->hc_FunctionNum = uhcicnt++;
                }
                else if(hc->hc_HCIType == HCITYPE_OHCI)
                {
                    hc->hc_FunctionNum = ohcicnt++;
                } else {
                    hc->hc_FunctionNum = 0;
                }
                AddTail(&hu->hu_Controllers, &hc->hc_Node);
            }
            hc = nexthc;
        }
        AddTail(&hd->hd_Units, (struct Node *) hu);
        unitno++;
    }
    return TRUE;
}
/* \\\ */

/* /// "PCIXReadConfigByte()" */
UBYTE PCIXReadConfigByte(struct PCIController *hc, UBYTE offset)
{
    struct pHidd_PCIDevice_ReadConfigByte msg;

    msg.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
    msg.reg = offset;

    return OOP_DoMethod(hc->hc_PCIDeviceObject, (OOP_Msg) &msg);
}
/* \\\ */

/* /// "PCIXReadConfigWord()" */
UWORD PCIXReadConfigWord(struct PCIController *hc, UBYTE offset)
{
    struct pHidd_PCIDevice_ReadConfigWord msg;

    msg.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigWord);
    msg.reg = offset;

    return OOP_DoMethod(hc->hc_PCIDeviceObject, (OOP_Msg) &msg);
}
/* \\\ */

/* /// "PCIXReadConfigLong()" */
ULONG PCIXReadConfigLong(struct PCIController *hc, UBYTE offset)
{
    struct pHidd_PCIDevice_ReadConfigLong msg;

    msg.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigLong);
    msg.reg = offset;

    return OOP_DoMethod(hc->hc_PCIDeviceObject, (OOP_Msg) &msg);
}
/* \\\ */

/* /// "PCIXWriteConfigByte()" */
void PCIXWriteConfigByte(struct PCIController *hc, ULONG offset, UBYTE value)
{
    struct pHidd_PCIDevice_WriteConfigByte msg;

    msg.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
    msg.reg = offset;
    msg.val = value;

    OOP_DoMethod(hc->hc_PCIDeviceObject, (OOP_Msg) &msg);
}
/* \\\ */

/* /// "PCIXWriteConfigWord()" */
void PCIXWriteConfigWord(struct PCIController *hc, ULONG offset, UWORD value)
{
    struct pHidd_PCIDevice_WriteConfigWord msg;

    msg.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigWord);
    msg.reg = offset;
    msg.val = value;

    OOP_DoMethod(hc->hc_PCIDeviceObject, (OOP_Msg) &msg);
}
/* \\\ */

/* /// "PCIXWriteConfigLong()" */
void PCIXWriteConfigLong(struct PCIController *hc, ULONG offset, ULONG value)
{
    struct pHidd_PCIDevice_WriteConfigLong msg;

    msg.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigLong);
    msg.reg = offset;
    msg.val = value;

    OOP_DoMethod(hc->hc_PCIDeviceObject, (OOP_Msg) &msg);
}
/* \\\ */

/* /// "pciStrcat()" */
void pciStrcat(STRPTR d, STRPTR s)
{
    while(*d) d++;
    while((*d++ = *s++));
}
/* \\\ */

/* /// "pciAllocUnit()" */
BOOL pciAllocUnit(struct PCIUnit *hu)
{
    struct PCIDevice *hd = hu->hu_Device;
    struct PCIController *hc;

    BOOL allocgood = TRUE;
    ULONG usb11ports;
    ULONG usb20ports;
#if defined(USB3)
    ULONG usb30ports;
#endif
    ULONG cnt;

    ULONG ohcicnt = 0;
    ULONG uhcicnt = 0;
    ULONG ehcicnt = 0;
#if defined(USB3)
    ULONG xhcicnt = 0;
#endif
    STRPTR prodname;

    struct TagItem pciActivateMem[] =
    {
            { aHidd_PCIDevice_isMEM,    TRUE },
            { TAG_DONE, 0UL },
    };

    struct TagItem pciActivateIO[] =
    {
            { aHidd_PCIDevice_isIO,     TRUE },
            { TAG_DONE, 0UL },
    };

    struct TagItem pciDeactivateBusmaster[] =
    {
            { aHidd_PCIDevice_isMaster, FALSE },
            { TAG_DONE, 0UL },
    };

    struct TagItem pciActivateBusmaster[] =
    {
            { aHidd_PCIDevice_isMaster, TRUE },
            { TAG_DONE, 0UL },
    };

    KPRINTF(10, ("*** pciAllocUnit(%08lx) ***\n", hu));
    hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ)
    {
#if 0 // FIXME this needs to be replaced by something AROS supports
        PCIXObtainBoard(hc->hc_BoardObject);
        hc->hc_BoardAllocated = PCIXSetBoardAttr(hc->hc_BoardObject, PCIXTAG_OWNER, (ULONG) hd->hd_Library.lib_Node.ln_Name);
        allocgood &= hc->hc_BoardAllocated;
        if(!hc->hc_BoardAllocated)
        {
            KPRINTF(20, ("Couldn't allocate board, already allocated by %s\n", PCIXGetBoardAttr(hc->hc_BoardObject, PCIXTAG_OWNER)));
        }
        PCIXReleaseBoard(hc->hc_BoardObject);
#endif

        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }

    if(allocgood)
    {
        // allocate necessary memory
        hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
        while(hc->hc_Node.ln_Succ)
        {
            switch(hc->hc_HCIType)
            {
                case HCITYPE_UHCI:
                {
                    struct UhciQH *uqh;
                    struct UhciQH *preduqh;
                    struct UhciTD *utd;
                    ULONG *tabptr;
                    UBYTE *memptr;
                    ULONG bitcnt;

                    hc->hc_NumPorts = 2; // UHCI always uses 2 ports per controller
                    KPRINTF(20, ("Found UHCI Controller %08lx FuncNum=%ld with %ld ports\n", hc->hc_PCIDeviceObject, hc->hc_FunctionNum, hc->hc_NumPorts));
                    hc->hc_CompleteInt.is_Node.ln_Type = NT_INTERRUPT;
                    hc->hc_CompleteInt.is_Node.ln_Name = "UHCI CompleteInt";
                    hc->hc_CompleteInt.is_Node.ln_Pri  = 0;
                    hc->hc_CompleteInt.is_Data = hc;
                    hc->hc_CompleteInt.is_Code = (void (*)(void)) &uhciCompleteInt;

                    hc->hc_PCIMemSize = sizeof(ULONG) * UHCI_FRAMELIST_SIZE + UHCI_FRAMELIST_ALIGNMENT + 1;
                    hc->hc_PCIMemSize += sizeof(struct UhciQH) * UHCI_QH_POOLSIZE;
                    hc->hc_PCIMemSize += sizeof(struct UhciTD) * UHCI_TD_POOLSIZE;
                    memptr = HIDD_PCIDriver_AllocPCIMem(hc->hc_PCIDriverObject, hc->hc_PCIMemSize);
                    if(!memptr)
                    {
                        allocgood = FALSE;
                        break;
                    }
                    hc->hc_PCIMem = (APTR) memptr;
                    // PhysicalAddress - VirtualAdjust = VirtualAddress
                    // VirtualAddress  + VirtualAdjust = PhysicalAddress
                    hc->hc_PCIVirtualAdjust = ((ULONG) pciGetPhysical(hc, memptr)) - ((ULONG) memptr);
                    KPRINTF(10, ("VirtualAdjust 0x%08lx\n", hc->hc_PCIVirtualAdjust));

                    // align memory
                    memptr = (UBYTE *) ((((ULONG) hc->hc_PCIMem) + UHCI_FRAMELIST_ALIGNMENT) & (~UHCI_FRAMELIST_ALIGNMENT));
                    hc->hc_UhciFrameList = (ULONG *) memptr;
                    KPRINTF(10, ("FrameListBase 0x%08lx\n", hc->hc_UhciFrameList));
                    memptr += sizeof(APTR) * UHCI_FRAMELIST_SIZE;

                    // build up QH pool
                    uqh = (struct UhciQH *) memptr;
                    hc->hc_UhciQHPool = uqh;
                    cnt = UHCI_QH_POOLSIZE - 1;
                    do
                    {
                        // minimal initalization
                        uqh->uqh_Succ = (struct UhciXX *) (uqh + 1);
                        WRITEMEM32_LE(&uqh->uqh_Self, (ULONG) (&uqh->uqh_Link) + hc->hc_PCIVirtualAdjust + UHCI_QHSELECT);
                        uqh++;
                    } while(--cnt);
                    uqh->uqh_Succ = NULL;
                    WRITEMEM32_LE(&uqh->uqh_Self, (ULONG) (&uqh->uqh_Link) + hc->hc_PCIVirtualAdjust + UHCI_QHSELECT);
                    memptr += sizeof(struct UhciQH) * UHCI_QH_POOLSIZE;

                    // build up TD pool
                    utd = (struct UhciTD *) memptr;
                    hc->hc_UhciTDPool = utd;
                    cnt = UHCI_TD_POOLSIZE - 1;
                    do
                    {
                        utd->utd_Succ = (struct UhciXX *) (utd + 1);
                        WRITEMEM32_LE(&utd->utd_Self, (ULONG) (&utd->utd_Link) + hc->hc_PCIVirtualAdjust + UHCI_TDSELECT);
                        utd++;
                    } while(--cnt);
                    utd->utd_Succ = NULL;
                    WRITEMEM32_LE(&utd->utd_Self, (ULONG) (&utd->utd_Link) + hc->hc_PCIVirtualAdjust + UHCI_TDSELECT);
                    memptr += sizeof(struct UhciTD) * UHCI_TD_POOLSIZE;

                    // terminating QH
                    hc->hc_UhciTermQH = preduqh = uqh = uhciAllocQH(hc);
                    uqh->uqh_Succ = NULL;
                    CONSTWRITEMEM32_LE(&uqh->uqh_Link, UHCI_TERMINATE);
                    CONSTWRITEMEM32_LE(&uqh->uqh_Element, UHCI_TERMINATE);

                    // dummy Bulk QH
                    hc->hc_UhciBulkQH = uqh = uhciAllocQH(hc);
                    uqh->uqh_Succ = (struct UhciXX *) preduqh;
                    preduqh->uqh_Pred = (struct UhciXX *) uqh;
                    uqh->uqh_Link = preduqh->uqh_Self; // link to terminating QH
                    CONSTWRITEMEM32_LE(&uqh->uqh_Element, UHCI_TERMINATE);
                    preduqh = uqh;

                    // dummy Ctrl QH
                    hc->hc_UhciCtrlQH = uqh = uhciAllocQH(hc);
                    uqh->uqh_Succ = (struct UhciXX *) preduqh;
                    preduqh->uqh_Pred = (struct UhciXX *) uqh;
                    uqh->uqh_Link = preduqh->uqh_Self; // link to Bulk QH
                    CONSTWRITEMEM32_LE(&uqh->uqh_Element, UHCI_TERMINATE);

                    // dummy ISO TD
                    hc->hc_UhciIsoTD = utd = uhciAllocTD(hc);
                    utd->utd_Succ = (struct UhciXX *) uqh;
                    //utd->utd_Pred = NULL; // no certain linkage above this level
                    uqh->uqh_Pred = (struct UhciXX *) utd;
                    utd->utd_Link = uqh->uqh_Self; // link to Ctrl QH

                    CONSTWRITEMEM32_LE(&utd->utd_CtrlStatus, 0);

                    // 1 ms INT QH
                    hc->hc_UhciIntQH[0] = uqh = uhciAllocQH(hc);
                    uqh->uqh_Succ = (struct UhciXX *) utd;
                    uqh->uqh_Pred = NULL; // who knows...
                    //uqh->uqh_Link = utd->utd_Self; // link to ISO
                    CONSTWRITEMEM32_LE(&uqh->uqh_Element, UHCI_TERMINATE);
                    preduqh = uqh;

                    // make 9 levels of QH interrupts
                    for(cnt = 1; cnt < 9; cnt++)
                    {
                        hc->hc_UhciIntQH[cnt] = uqh = uhciAllocQH(hc);
                        uqh->uqh_Succ = (struct UhciXX *) preduqh;
                        uqh->uqh_Pred = NULL; // who knows...
                        //uqh->uqh_Link = preduqh->uqh_Self; // link to previous int level
                        CONSTWRITEMEM32_LE(&uqh->uqh_Element, UHCI_TERMINATE);
                        preduqh = uqh;
                    }

                    uhciUpdateIntTree(hc);

                    // fill in framelist with IntQH entry points based on interval
                    tabptr = hc->hc_UhciFrameList;
                    for(cnt = 0; cnt < UHCI_FRAMELIST_SIZE; cnt++)
                    {
                        uqh = hc->hc_UhciIntQH[8];
                        bitcnt = 0;
                        do
                        {
                            if(cnt & (1UL<<bitcnt))
                            {
                                uqh = hc->hc_UhciIntQH[bitcnt];
                                break;
                            }
                        } while(++bitcnt < 9);
                        *tabptr++ = uqh->uqh_Self;
                    }

                    // this will cause more PCI memory access, but faster USB transfers as well
                    //WRITEMEM32_LE(&hc->hc_UhciTermQH->uqh_Link, AROS_LONG2LE(hc->hc_UhciBulkQH->uqh_Self));

                    // time to initialize hardware...
                    OOP_GetAttr(hc->hc_PCIDeviceObject, aHidd_PCIDevice_Base4, (IPTR *) &hc->hc_RegBase);
                    hc->hc_RegBase = (APTR) (((IPTR) hc->hc_RegBase) & (~0xf));
                    KPRINTF(10, ("RegBase = 0x%08lx\n", hc->hc_RegBase));
                    OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciActivateIO);

                    // disable BIOS legacy support
                    KPRINTF(10, ("Turning off BIOS legacy support (old value=%04lx)\n", PCIXReadConfigWord(hc, UHCI_USBLEGSUP)));
                    PCIXWriteConfigWord(hc, UHCI_USBLEGSUP, 0x8f00);

                    KPRINTF(10, ("Resetting UHCI HC\n"));
                    WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_GLOBALRESET);
                    uhwDelayMS(15, hu);

                    OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciDeactivateBusmaster); // no busmaster yet

                    WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_HCRESET);
                    cnt = 100;
                    do
                    {
                        uhwDelayMS(10, hu);
                        if(!(READIO16_LE(hc->hc_RegBase, UHCI_USBCMD) & UHCF_HCRESET))
                        {
                            break;
                        }
                    } while(--cnt);

                    if(cnt == 0)
                    {
                        KPRINTF(20, ("Reset Timeout!\n"));
                        WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_HCRESET);
                        uhwDelayMS(15, hu);
                    } else {
                        KPRINTF(20, ("Reset finished after %ld ticks\n", 100-cnt));
                    }

                    // stop controller and disable all interrupts first
                    KPRINTF(10, ("Stopping controller and enabling busmaster\n"));
                    WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, 0);
                    WRITEIO16_LE(hc->hc_RegBase, UHCI_USBINTEN, 0);

                    OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciActivateBusmaster); // enable busmaster

                    // Fix for VIA Babble problem
                    cnt = PCIXReadConfigByte(hc, 0x40);
                    if(!(cnt & 0x40))
                    {
                        KPRINTF(20, ("Applying VIA Babble workaround\n"));
                        PCIXWriteConfigByte(hc, 0x40, cnt|0x40);
                    }

                    KPRINTF(10, ("Configuring UHCI HC\n"));
                    WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_MAXPACKET64|UHCF_CONFIGURE);

                    WRITEIO16_LE(hc->hc_RegBase, UHCI_FRAMECOUNT, 0);

                    WRITEIO32_LE(hc->hc_RegBase, UHCI_FRAMELISTADDR, (ULONG) pciGetPhysical(hc, hc->hc_UhciFrameList));

                    WRITEIO16_LE(hc->hc_RegBase, UHCI_USBSTATUS, UHIF_TIMEOUTCRC|UHIF_INTONCOMPLETE|UHIF_SHORTPACKET);

                    // install reset handler
                    hc->hc_ResetInt.is_Code = UhciResetHandler;
                    hc->hc_ResetInt.is_Data = hc;
                    AddResetCallback(&hc->hc_ResetInt);

                    // add interrupt
                    hc->hc_PCIIntHandler.h_Node.ln_Name = "UHCI PCI (pciusb.device)";
                    hc->hc_PCIIntHandler.h_Node.ln_Pri = 5;
                    hc->hc_PCIIntHandler.h_Code = uhciIntCode;
                    hc->hc_PCIIntHandler.h_Data = hc;
                    HIDD_IRQ_AddHandler(hd->hd_IRQHidd, &hc->hc_PCIIntHandler, hc->hc_PCIIntLine);

                    WRITEIO16_LE(hc->hc_RegBase, UHCI_USBINTEN, UHIF_TIMEOUTCRC|UHIF_INTONCOMPLETE|UHIF_SHORTPACKET);

                    // clear all port bits (both ports)
                    WRITEIO32_LE(hc->hc_RegBase, UHCI_PORT1STSCTRL, 0);

                    // enable PIRQ
                    KPRINTF(10, ("Enabling PIRQ (old value=%04lx)\n", PCIXReadConfigWord(hc, UHCI_USBLEGSUP)));
                    PCIXWriteConfigWord(hc, UHCI_USBLEGSUP, 0x2000);

                    WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_MAXPACKET64|UHCF_CONFIGURE|UHCF_RUNSTOP);
                    SYNC;

                    KPRINTF(20, ("HW Init done\n"));

                    KPRINTF(10, ("HW Regs USBCMD=%04lx\n", READIO16_LE(hc->hc_RegBase, UHCI_USBCMD)));
                    KPRINTF(10, ("HW Regs USBSTS=%04lx\n", READIO16_LE(hc->hc_RegBase, UHCI_USBSTATUS)));
                    KPRINTF(10, ("HW Regs FRAMECOUNT=%04lx\n", READIO16_LE(hc->hc_RegBase, UHCI_FRAMECOUNT)));
                    break;
                }

                case HCITYPE_OHCI:
                {
                    allocgood = ohciInit(hc,hu);
                    break;
                }

                case HCITYPE_EHCI:
                {
                    allocgood = ehciInit(hc,hu);
                    break;
                }
#if defined(USB3)
                case HCITYPE_XHCI:
                {
                    allocgood = xhciInit(hc,hu);
                    break;
                }
#endif
            }
            hc = (struct PCIController *) hc->hc_Node.ln_Succ;
        }
    }

    if(!allocgood)
    {
        // free previously allocated boards
        hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
        while(hc->hc_Node.ln_Succ)
        {
#if 0
            PCIXObtainBoard(hc->hc_BoardObject);
            if(hc->hc_BoardAllocated)
            {
                hc->hc_BoardAllocated = FALSE;
                PCIXSetBoardAttr(hc->hc_BoardObject, PCIXTAG_OWNER, 0);
            }
            PCIXReleaseBoard(hc->hc_BoardObject);
#endif
            hc = (struct PCIController *) hc->hc_Node.ln_Succ;
        }
        return FALSE;
    }

    // find all belonging host controllers
    usb11ports = 0;
    usb20ports = 0;
#if defined(USB3)
    usb30ports = 0;
#endif

    hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ)
    {
#if defined(USB3)
        if(hc->hc_HCIType == HCITYPE_XHCI)
        {
            xhcicnt++;
            if(usb30ports)
            {
                KPRINTF(200, ("WARNING: Two XHCI controllers per Board?!?\n"));
            }
            usb30ports = hc->hc_NumPorts;
        }
        else if(hc->hc_HCIType == HCITYPE_EHCI)
#else
        if(hc->hc_HCIType == HCITYPE_EHCI)
#endif
        {
            ehcicnt++;
            if(usb20ports)
            {
                KPRINTF(200, ("WARNING: Two EHCI controllers per Board?!?\n"));
            }
            usb20ports = hc->hc_NumPorts;
            for(cnt = 0; cnt < usb20ports; cnt++)
            {
                hu->hu_PortMap20[cnt] = hc;
                hc->hc_PortNum20[cnt] = cnt;
            }
        }
        else if(hc->hc_HCIType == HCITYPE_UHCI)
        {
            uhcicnt++;
        }
        else if(hc->hc_HCIType == HCITYPE_OHCI)
        {
            ohcicnt++;
        }
        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }

    hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ)
    {
        if((hc->hc_HCIType == HCITYPE_UHCI) || (hc->hc_HCIType == HCITYPE_OHCI))
        {
            if(hc->hc_complexrouting)
            {
                ULONG locport = 0;
                for(cnt = 0; cnt < usb20ports; cnt++)
                {
                    if(((hc->hc_portroute >> (cnt<<2)) & 0xf) == hc->hc_FunctionNum)
                    {
                        KPRINTF(10, ("CHC %ld Port %ld assigned to global Port %ld\n", hc->hc_FunctionNum, locport, cnt));
                        hu->hu_PortMap11[cnt] = hc;
                        hu->hu_PortNum11[cnt] = locport;
                        hc->hc_PortNum20[locport] = cnt;
                        locport++;
                    }
                }
            } else {
                for(cnt = usb11ports; cnt < usb11ports + hc->hc_NumPorts; cnt++)
                {
                    hu->hu_PortMap11[cnt] = hc;
                    hu->hu_PortNum11[cnt] = cnt - usb11ports;
                    hc->hc_PortNum20[cnt - usb11ports] = cnt;
                }
            }
            usb11ports += hc->hc_NumPorts;
        }
        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }
    if((usb11ports != usb20ports) && usb20ports)
    {
        KPRINTF(20, ("Warning! #EHCI Ports (%ld) does not match USB 1.1 Ports (%ld)!\n", usb20ports, usb11ports));
    }

    hu->hu_RootHub11Ports = usb11ports;
    hu->hu_RootHub20Ports = usb20ports;
#if defined(USB3)
    hu->hu_RootHub30Ports = usb30ports;
    hu->hu_RootHubPorts = (usb11ports > usb20ports) ? ((usb11ports > usb30ports) ? usb11ports : usb30ports) : ((usb30ports > usb20ports) ? usb30ports : usb20ports);
#else
    hu->hu_RootHubPorts = (usb11ports > usb20ports) ? usb11ports : usb20ports;
#endif
    for(cnt = 0; cnt < hu->hu_RootHubPorts; cnt++)
    {
        hu->hu_EhciOwned[cnt] = hu->hu_PortMap20[cnt] ? TRUE : FALSE;
    }

#if defined(USB3)
    KPRINTF(1000, ("Unit %ld: USB Board %08lx has %ld USB1.1, %ld USB2.0 and %ld USB3.0 ports!\n", hu->hu_UnitNo, hu->hu_DevID, hu->hu_RootHub11Ports, hu->hu_RootHub20Ports, hu->hu_RootHub30Ports));
#else
    KPRINTF(10, ("Unit %ld: USB Board %08lx has %ld USB1.1 and %ld USB2.0 ports!\n", hu->hu_UnitNo, hu->hu_DevID, hu->hu_RootHub11Ports, hu->hu_RootHub20Ports));
#endif

    hu->hu_FrameCounter = 1;
    hu->hu_RootHubAddr = 0;

    // put em online
    hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ)
    {
        hc->hc_Online = TRUE;
        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }

    // create product name of device
    prodname = hu->hu_ProductName;
    *prodname = 0;
    pciStrcat(prodname, "PCI ");
    if(ohcicnt + uhcicnt)
    {
        if(ohcicnt + uhcicnt >1)
        {
            prodname[4] = ohcicnt + uhcicnt + '0';
            prodname[5] = 'x';
            prodname[6] = 0;
        }
        pciStrcat(prodname, ohcicnt ? "OHCI" : "UHCI");
        if(ehcicnt)
        {
            pciStrcat(prodname, " +");
        } else{
            pciStrcat(prodname, " USB 1.1");
        }
    }
    if(ehcicnt)
    {
        pciStrcat(prodname, " EHCI USB 2.0");
    }
#if defined(USB3)
    if(xhcicnt)
    {
        if(xhcicnt >1)
        {
            prodname[4] = xhcicnt + '0';
            prodname[5] = 'x';
            prodname[6] = 0;
        }
        pciStrcat(prodname, " XHCI USB 3.0");
    }
#endif
#if 0 // user can use pcitool to check what the chipset is and not guess it from this
    pciStrcat(prodname, " Host Controller (");
    if(ohcicnt + uhcicnt)
    {
        pciStrcat(prodname, ohcicnt ? "NEC)" : "VIA, Intel, ALI, etc.)");
    } else {
		pciStrcat(prodname, "Emulated?)");
	}
#else
    pciStrcat(prodname, " Host Controller");
#endif
    KPRINTF(10, ("Unit allocated!\n", hd));

    return TRUE;
}
/* \\\ */

/* /// "pciFreeUnit()" */
void pciFreeUnit(struct PCIUnit *hu)
{
    struct PCIDevice *hd = hu->hu_Device;
    struct PCIController *hc;

    struct TagItem pciDeactivate[] =
    {
            { aHidd_PCIDevice_isIO,     FALSE },
            { aHidd_PCIDevice_isMEM,    FALSE },
            { aHidd_PCIDevice_isMaster, FALSE },
            { TAG_DONE, 0UL },
    };

    KPRINTF(10, ("*** pciFreeUnit(%08lx) ***\n", hu));

    // put em offline
    hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ)
    {
        hc->hc_Online = FALSE;
        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }

#if defined(USB3)
    xhciFree(hc, hu);
#endif
    // doing this in three steps to avoid these damn host errors
    ehciFree(hc, hu);
    ohciFree(hc, hu);

    hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ)
    {
        switch(hc->hc_HCIType)
        {
            case HCITYPE_UHCI:
                KPRINTF(20, ("Shutting down UHCI %08lx\n", hc));
                WRITEIO16_LE(hc->hc_RegBase, UHCI_USBINTEN, 0);
                // disable PIRQ
                PCIXWriteConfigWord(hc, UHCI_USBLEGSUP, 0);
                // disable all ports
                WRITEIO32_LE(hc->hc_RegBase, UHCI_PORT1STSCTRL, 0);
                uhwDelayMS(50, hu);
                //WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_MAXPACKET64|UHCF_CONFIGURE);
                //uhwDelayMS(50, hu);
                KPRINTF(20, ("Stopping UHCI %08lx\n", hc));
                WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, 0);
                SYNC;

                //KPRINTF(20, ("Reset done UHCI %08lx\n", hc));
                uhwDelayMS(10, hu);

                KPRINTF(20, ("Resetting UHCI %08lx\n", hc));
                WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_HCRESET);
                SYNC;

                uhwDelayMS(50, hu);
                WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, 0);
                SYNC;

                KPRINTF(20, ("Shutting down UHCI done.\n"));
                break;
        }

        if(hc->hc_PCIMem)
        {
            HIDD_PCIDriver_FreePCIMem(hc->hc_PCIDriverObject, hc->hc_PCIMem);
            hc->hc_PCIMem = NULL;
        }
        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }

    // disable and free board
    hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ)
    {
        OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciDeactivate); // deactivate busmaster and IO/Mem
        if(hc->hc_PCIIntHandler.h_Node.ln_Name)
        {
            HIDD_IRQ_RemHandler(hd->hd_IRQHidd, &hc->hc_PCIIntHandler);
            hc->hc_PCIIntHandler.h_Node.ln_Name = NULL;
        }
#if 0

        PCIXObtainBoard(hc->hc_BoardObject);
        hc->hc_BoardAllocated = FALSE;
        PCIXSetBoardAttr(hc->hc_BoardObject, PCIXTAG_OWNER, 0);
        PCIXReleaseBoard(hc->hc_BoardObject);
#endif
        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }
}
/* \\\ */

/* /// "pciExpunge()" */
void pciExpunge(struct PCIDevice *hd)
{
    struct PCIController *hc;
    struct PCIUnit *hu;

    KPRINTF(10, ("*** pciExpunge(%08lx) ***\n", hd));

    hu = (struct PCIUnit *) hd->hd_Units.lh_Head;
    while(((struct Node *) hu)->ln_Succ)
    {
        Remove((struct Node *) hu);
        hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
        while(hc->hc_Node.ln_Succ)
        {
            Remove(&hc->hc_Node);
            FreePooled(hd->hd_MemPool, hc, sizeof(struct PCIController));
            hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
        }
        FreePooled(hd->hd_MemPool, hu, sizeof(struct PCIUnit));
        hu = (struct PCIUnit *) hd->hd_Units.lh_Head;
    }
    if(hd->hd_PCIHidd)
    {
        struct OOP_ABDescr attrbases[] =
        {
            { (STRPTR) IID_Hidd,            &hd->hd_HiddAB },
            { (STRPTR) IID_Hidd_PCIDevice,  &hd->hd_HiddPCIDeviceAB },
//            { (STRPTR) IID_Hidd_USBDevice,  &hd->hd_HiddUSBDeviceAB },
//            { (STRPTR) IID_Hidd_USBHub,     &hd->hd_HiddUSBHubAB },
//            { (STRPTR) IID_Hidd_USBDrv,     &hd->hd_HiddUSBDrvAB },
            { NULL, NULL }
        };

        OOP_ReleaseAttrBases(attrbases);

        OOP_DisposeObject(hd->hd_PCIHidd);
    }
    if(hd->hd_IRQHidd)
    {
        OOP_DisposeObject(hd->hd_IRQHidd);
    }
}
/* \\\ */

/* /// "pciGetPhysical()" */
APTR pciGetPhysical(struct PCIController *hc, APTR virtaddr)
{
    //struct PCIDevice *hd = hc->hc_Device;
    return(HIDD_PCIDriver_CPUtoPCI(hc->hc_PCIDriverObject, virtaddr));
}
/* \\\ */
