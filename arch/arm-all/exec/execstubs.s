/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id: execstubs.s 39924 2011-07-08 15:37:34Z jmcmullan $

    Desc: Stubs to call C functions while preserving all registers, x86-64 version
    Lang: english
*/

#include "aros/arm/asm.h"

/*
 * Up to 4 arguments can be handled in this way.
 * Our stubbed functions use only up to two.
 */
#define STUB(name)	         		  \
	push	{fp, lr}			; \
	add	fp, sp, #4			; \
	push	{r0, r1, r2, r3, r9, r12}	; \
	ldr	r12, 1f				; \
	blx	r12				; \
	pop 	{r0, r1, r2, r3, r9, r12}	; \
	sub	sp, fp, #4			; \
	pop	{fp, lr}			; \
	bx	lr				; \
1:	.word	name

/* To save typing work */
#define STUB0(cname,name)             \
	.globl	cname		    ; \
	_FUNCTION(cname)	    ; \
cname:				    ; \
	STUB(name)

#define STUB1(cname,name)             \
	.globl	cname		    ; \
	_FUNCTION(cname)	    ; \
cname:				    ; \
	STUB(name)

#define STUB2(cname,name)             \
	.globl	cname		    ; \
	_FUNCTION(cname)	    ; \
cname:				    ; \
	STUB(name)

	.text

	/* Call functions and preserve registers */
#ifdef  UseExecstubs
	STUB1(AROS_SLIB_ENTRY(Disable,Exec,20),AROS_CSYMNAME(_Exec_20_Disable))
	STUB1(AROS_SLIB_ENTRY(Enable,Exec,21),AROS_CSYMNAME(_Exec_21_Enable))
	STUB1(AROS_SLIB_ENTRY(Forbid,Exec,22),AROS_CSYMNAME(_Exec_22_Forbid))
	STUB1(AROS_SLIB_ENTRY(Permit,Exec,23),AROS_CSYMNAME(_Exec_23_Permit))

	STUB2(AROS_SLIB_ENTRY(ObtainSemaphore,Exec,94),AROS_CSYMNAME(_Exec_94_ObtainSemaphore))
	STUB2(AROS_SLIB_ENTRY(ReleaseSemaphore,Exec,95),AROS_CSYMNAME(_Exec_95_ReleaseSemaphore))
	STUB2(AROS_SLIB_ENTRY(ObtainSemaphoreShared,Exec,113),AROS_CSYMNAME(_Exec_113_ObtainSemaphoreShared))
#endif

