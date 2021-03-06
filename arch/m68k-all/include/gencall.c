/*
    Copyright (C) 1995-2019, The AROS Development Team. All rights reserved.
*/
/* This program generates the libcall.h macroset for m68k-elf
 *
 * WARNING: The header generated by this program has only been
 *          tested to work with GCC 4.5.1 to GCC 8.3.0 m68k-elf ONLY.
 *
 * If it breaks, you get to keep both pieces.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define GENCALL_MAX	(13 + 1)	/* Max number of arguments */

/* NOTE: For all 'call' macros, ie AROS_LC5(), the
 *       'bt' parameter is frequently garbled by
 *       callers who use '#define's like:
 *
 * #define UtilityBase mydata->utilitybase
 *
 *       this makes a 'bt' parameter of 'struct UtilityBase *'
 *       turn into 'struct mydata->utilitybase *'. 
 *
 *       Unhappiness ensues. So, we must use 'void *'
 *       for all 'bt' arguments in the call macros.
 */

#define FLAG_BN		(1 << 0)
#define FLAG_DOUBLE	(1 << 1)
#define FLAG_NR    	(1 << 2)

void aros_ufp(int id, int is_static)
{
    int i;

    printf("#define AROS_UFP%d%s(t,n", id, is_static ? "S" : "");
    for (i = 0; i < id; i++)
        printf(",a%d", i + 1);
    printf(") \\\n");
    printf("\t%st n (void)\n", is_static ? "static " : "");
}

void aros_ufh(int id, int is_static)
{
    int i;

    printf("#define AROS_UFH%d%s(t,n", id, is_static ? "S" : "");
    for (i = 0; i < id; i++)
        printf(",a%d", i + 1);
    printf(") \\\n");
    printf("\t%st n (void) {%s\n", is_static ? "static " : "", (i==0) ? "" : " \\");
    for (i = 0; i < id; i++)
        printf(" \\\n\t__AROS_UFPA(a%d) __attribute__((unused)) __AROS_UFCA(a%d) = __AROS_ISREG(a%d,__AROS_FP_REG) ? (__AROS_UFPA(a%d))(ULONG)__builtin_frame_address(1) : ({register ULONG __r asm(__AROS_UFSA(a%d));asm volatile(\"\":\"=r\"(__r):\"0\"(__r));(__AROS_UFPA(a%d))__r;});", i+1, i+1, i+1, i+1, i+1, i+1);
    printf("\n");
}

static void asm_regs_init(int id, int flags, const char *type, const char *jmp, const char *addr)
{
    int i;
    int has_bn = (flags & FLAG_BN);
    const char *output = "\"=r\"(_ret0), \"=r\"(_ret1), \"=r\"(_mem0), \"=r\"(_mem1)";

    printf("\tregister volatile ULONG _ret0 asm(\"%%d0\"); \\\n");
    printf("\tregister volatile ULONG _ret1 asm(\"%%d1\"); \\\n");
    printf("\tregister volatile ULONG _mem0 asm(\"%%a0\"); \\\n");
    printf("\tregister volatile ULONG _mem1 asm(\"%%a1\"); \\\n");
    printf("\t{ \\\n");

    /* Input values */
    for (i = 0; i < id; i++)
        printf("\t   ULONG _arg%d = (ULONG)__AROS_%sCA(a%d); \\\n",
               i + 1, type, i + 1
        );
    if (has_bn)
        printf("\t   ULONG _bn_arg = (ULONG)bn; \\\n");

    /* Define registers */
    for (i = 0; i < id; i++)
        printf("\t   register volatile ULONG  __AROS_%sTA(a%d) asm(__AROS_%sSA(a%d)); \\\n",
               type, i + 1, type, i + 1
        );
    if (has_bn)
        printf("\t   register volatile ULONG _bn asm(\"%%a6\"); \\\n");

    /* Set registers (non FP) */
    for (i = 1; i <= id; i++)
        printf("\t   if (! __AROS_ISREG(a%d,__AROS_FP_REG)) { \\\n"
               "\t      __AROS_%sTA(a%d) = _arg%d; } \\\n",
               i, type, i, i
        );
    if (has_bn)
        printf("\t   if (! __AROS_ISREG(bt,bn,A6,__AROS_FP_REG)) { \\\n"
               "\t      _bn = _bn_arg; } \\\n"
        );

    /* Set FP register */
    for (i = 1; i <= id; i++)
    {
        printf("\t   if ( __AROS_ISREG(a%d,__AROS_FP_REG)) { \\\n"
               "\t      asm volatile (\"move.l %%%%\" __AROS_FP_SREG \",%%%%sp@-\\nmove.l %%4,%%%%\" __AROS_FP_SREG \"\\n%s\\nmove.l %%%%sp@+,%%%%\" __AROS_FP_SREG \"\\n\" : %s : \"r\" (_arg%d), %s \\\n",
               i, jmp, output, i, addr
        );
        int j;
        for (j = 0; j < id; j++)
            printf("\t\t, \"r\" (__AROS_%sTA(a%d)) \\\n", type, j + 1);
        printf ("\t\t: \"cc\", \"memory\" \\\n");
        printf("\t       ); }\\\n");
    }
    if (has_bn)
    {
        printf("\t   if ( __AROS_ISREG(bt,bn,A6,__AROS_FP_REG)) { \\\n"
               "\t      asm volatile (\"move.l %%%%\" __AROS_FP_SREG \",%%%%sp@-\\nmove.l %%4,%%%%\" __AROS_FP_SREG \"\\n%s\\nmove.l %%%%sp@+,%%%%\" __AROS_FP_SREG \"\\n\" : %s : \"r\" (_bn_arg), %s \\\n", jmp, output, addr
        );
        int j;
        for (j = 0; j < id; j++)
            printf("\t\t, \"r\" (__AROS_%sTA(a%d)) \\\n", type, j + 1);
        printf ("\t\t: \"cc\", \"memory\" \\\n");
        printf("\t       ); }\\\n");
    }
    if (has_bn || id > 0)
    {
        printf("\t   if (!(0");
        if (has_bn)
            printf(" || __AROS_ISREG(bt,bn,A6,__AROS_FP_REG)");
        for (i = 0; i < id; i++)
            printf(" || __AROS_ISREG(a%d,__AROS_FP_REG)", i+1);
        printf(")) {\\\n"
               "\t      asm volatile (\"%s\\n\" : %s : \"i\" (0), %s \\\n", jmp, output, addr
        );
        int j;
        for (j = 0; j < id; j++)
            printf("\t\t, \"r\" (__AROS_%sTA(a%d)) \\\n", type, j + 1);
        printf ("\t\t: \"cc\", \"memory\" \\\n");
        printf("\t       ); }\\\n");
    }

    printf("\t}\\\n");

    if (flags & FLAG_NR) {
        return;
    }

    /* Get the return code
     *
     * We rely upon the compiler to optimize this to either
     * as single %d0 for BYTE..LONG, or %d0/%d1 for QUAD/double
     *
     * Struct returns are not supported.
     */
    printf("\t(void)_mem0; (void)_mem1; \\\n");
    printf("\t(sizeof(t) < sizeof(QUAD)) ? (t)(_ret0) :\\\n");
    printf("\t   ({struct { ULONG r0,r1; } rv;\\\n");
    printf("\t     t *t_ptr = (t *)&rv.r0;\\\n");
    printf("\t     rv.r0 = _ret0; rv.r1 = _ret1; *t_ptr; });\\\n");
}

static inline const char *nr(int flags)
{
    if (flags & FLAG_NR)
        return "NR";
    else
        return "";
}

static void aros_ufc(int id, int flags)
{
    int i;
    char jmp[256];

    printf("#define __AROS_UFC%d%s(t,n", id, nr(flags));
    for (i = 0; i < id; i++)
        printf(",a%d", i + 1);
    printf(") \\\n");
    printf("\t({ APTR _n = (n);\\\n");
    snprintf(jmp, sizeof(jmp),
             "\tpea.l 6(%%%%pc)\\n"
             "\tmove.l %%5, -(%%%%sp)\\n"
             "\trts\\n"
    );
    jmp[sizeof(jmp)-1]=0;

    asm_regs_init(i, flags, "UF", jmp, "\"r\" (_n), \"i\" (__LINE__)");
    printf("\t  })\n\n");
    printf("#define AROS_UFC%d%s __AROS_UFC%d%s\n", id, nr(flags), id, nr(flags));
}

void aros_lc(int id, int flags)
{
    int i;
    int is_double = (flags & FLAG_DOUBLE);
    flags |= FLAG_BN;

    printf("#define __AROS_LC%d%s%s(t,n,", id, is_double ? "D" : "", nr(flags));
    for (i = 0; i < id; i++)
        printf("a%d,", i + 1);
    printf("bt,bn,o,s) \\\n");
    printf("\t({ \\\n");
    asm_regs_init(id, flags, "L", "jsr %c5(%%a6)", "\"i\" (-1 * (o) * LIB_VECTSIZE), \"r\" (_bn)");
    printf("\t  })\n\n");
    printf("#define AROS_LC%d%s%s __AROS_LC%d%s%s\n", id, is_double ? "D" : "", nr(flags), id, is_double ? "D" : "", nr(flags));
}

void aros_lp(int id, int is_ignored)
{
    int i;

    printf("#define __AROS_LP%d%s(t,n,", id, is_ignored ? "I" : "");
    for (i = 0; i < id; i++)
        printf("a%d,", i + 1);
    printf("bt,bn,o,s) \\\n");
    printf("\tt n ( ");
    for (i = 0; i < id; i++)
        printf("__AROS_LHA(a%d)%s", 
               i + 1,
               ((i + 1) == id) ? "" : ", \\\n\t"
        );
    if (id == 0)
        printf("void ");
    printf(")\n");
    printf("#define AROS_LP%d%s __AROS_LP%d%s\n", id, is_ignored ? "I" : "", id, is_ignored ? "I" : "");
}

void aros_lh(int id, int is_ignored)
{
    int i;

    printf("#define __AROS_LH%d%s(t,n,", id, is_ignored ? "I" : "");
    for (i = 0; i < id; i++)
        printf("a%d,", i + 1);
    printf("bt,bn,o,s) \\\n");
    printf("\tt AROS_SLIB_ENTRY(n,s,o) (void) {");
    for (i = 0; i < id; i++)
        printf(" \\\n\t__AROS_LPA(a%d) __attribute__((unused)) __AROS_LCA(a%d) = __AROS_ISREG(a%d,__AROS_FP_REG) ? (__AROS_LPA(a%d))(ULONG)__builtin_frame_address(1) : ({register ULONG __r asm(__AROS_LSA(a%d));asm volatile(\"\":\"=r\"(__r):\"0\"(__r));(__AROS_LPA(a%d))__r;});", i+1, i+1, i+1, i+1, i+1, i+1);
    if (!is_ignored)
        printf(" \\\n\tregister bt __attribute__((unused)) bn = __AROS_ISREG(bn,bt,A6,__AROS_FP_REG) ? (bt)(ULONG)__builtin_frame_address(1) : ({register ULONG __r asm(\"%%a6\");asm volatile(\"\":\"=r\"(__r):\"0\"(__r));(bt)__r;});");
    printf("\n");
    printf("#define AROS_LH%d%s __AROS_LH%d%s\n", id, is_ignored ? "I" : "", id, is_ignored ? "I" : "");
}

static void aros_call(int id, int flags)
{
    int i;
    printf("#define __AROS_CALL%d%s(t,n,", id, nr(flags));
    for (i = 0; i < id; i++)
        printf("a%d,", i + 1);
    printf("bt,bn) \\\n");
    printf("\tAROS_UFC%d%s(t,n", id + 1, nr(flags));
    for (i = 0; i < id; i++)
    {
        printf(",AROS_UFCA(a%d)", i + 1);
    }
    printf(",AROS_UFCA(bt,bn,A6))\n");
    printf("#define AROS_CALL%d%s __AROS_CALL%d%s\n", id, nr(flags), id, nr(flags));
}

static void aros_lvo_call(int id, int flags)
{
    int i;
    printf("#define __AROS_LVO_CALL%d%s(t,", id, nr(flags));
    for (i = 0; i < id; i++)
        printf("a%d,", i + 1);
    printf("bt,bn,o,s) \\\n");
    printf("\t__AROS_CALL%d%s(t,__AROS_GETVECADDR(bn,o), \\\n", id, nr(flags));
    for (i = 0; i < id; i++)
        printf("\t\tAROS_LCA(a%d), \\\n", i + 1);
    printf("\t\tbt,bn)\n");
    printf("#define AROS_LVO_CALL%d%s __AROS_LVO_CALL%d%s\n", id, nr(flags), id, nr(flags));
}

static void aros_ld(int id, int is_ignored)
{
    int i;

    printf("#define __AROS_LD%d%s(t,n,", id, is_ignored ? "I" : "");
    for (i = 0; i < id; i++)
        printf("a%d,", i + 1);
    printf("bt,bn,o,s) \\\n");
    printf("\t__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) (void)\n");
    printf("#define AROS_LD%d%s __AROS_LD%d%s\n", id, is_ignored ? "I" : "", id, is_ignored ? "I" : "");
}

static const char asmextra[] =
"/* Get the register from a triplet */\n"
"#define __AROS_UFRA(type,name,reg)            reg\n"
"\n"
"/* Temporary variables */\n"
"#define __AROS_UFTA(type,name,reg)            reg##_tmp\n"
"\n"
"/* Get the register as a string from the triplet */\n"
"#define __AROS_UFSA(type,name,reg)            \"%\"#reg\n"
;

static const char libextra[] =
"/* Get the register from a triplet */\n"
"#define __AROS_LRA(type,name,reg)             reg\n"
"#define __AROS_LRAQUAD1(type,name,reg1,reg2)  reg1\n"
"#define __AROS_LRAQUAD2(type,name,reg1,reg2)  reg2\n"
"\n"
"/* Temporary variables */\n"
"#define __AROS_LTA(type,name,reg)             reg##_tmp\n"
"#define __AROS_LTAQUAD(type,name,reg1,reg2)   reg1##_##reg2##_tmp\n"
"#define __AROS_LTAQUAD1(type,name,reg1,reg2)  reg1##_tmp\n"
"#define __AROS_LTAQUAD2(type,name,reg1,reg2)  reg2##_tmp\n"
"\n"
"/* Get the register as a string from the triplet */\n"
"#define __AROS_LSA(type,name,reg)             \"%\"#reg\n"
"#define __AROS_LSAQUAD1(type,name,reg1,reg2)  \"%\"#reg1\n"
"#define __AROS_LSAQUAD2(type,name,reg1,reg2)  \"%\"#reg2\n"
"\n"
"#define __AROS_LHQUAD1(t,n,q1,bt,bn,o,s) \\\n"
"        __AROS_LH2(t,n, \\\n"
"                AROS_LHA(ULONG, __AROS_LTAQUAD1(q1), __AROS_LRAQUAD1(q1)), \\\n"
"                AROS_LHA(ULONG, __AROS_LTAQUAD2(q1), __AROS_LRAQUAD2(q1)), \\\n"
"                bt, bn, o, s) \\\n"
"                union { \\\n"
"                        __AROS_LPAQUAD(q1) val; \\\n"
"                        ULONG reg[2]; \\\n"
"                } __AROS_LTAQUAD(q1); \\\n"
"                __AROS_LTAQUAD(q1).reg[0] = __AROS_LTAQUAD1(q1); \\\n"
"                __AROS_LTAQUAD(q1).reg[1] = __AROS_LTAQUAD2(q1); \\\n"
"                __AROS_LPAQUAD(q1) __attribute__((unused)) __AROS_LCAQUAD(q1) = __AROS_LTAQUAD(q1).val;\n"
"\n"
"#define AROS_LHQUAD1 __AROS_LHQUAD1\n"
"\n"
"#define __AROS_LHQUAD2(t,n,q1,q2,bt,bn,o,s) \\\n"
"        __AROS_LH4(t,n, \\\n"
"                AROS_LHA(ULONG, __AROS_LTAQUAD1(q1), __AROS_LRAQUAD1(q1)), \\\n"
"                AROS_LHA(ULONG, __AROS_LTAQUAD2(q1), __AROS_LRAQUAD2(q1)), \\\n"
"                AROS_LHA(ULONG, __AROS_LTAQUAD1(q2), __AROS_LRAQUAD1(q2)), \\\n"
"                AROS_LHA(ULONG, __AROS_LTAQUAD2(q2), __AROS_LRAQUAD2(q2)), \\\n"
"                bt, bn, o, s) \\\n"
"                union { \\\n"
"                        __AROS_LPAQUAD(q1) val; \\\n"
"                        ULONG reg[2]; \\\n"
"                } __AROS_LTAQUAD(q1); \\\n"
"                union { \\\n"
"                        __AROS_LPAQUAD(q2) val; \\\n"
"                        ULONG reg[2]; \\\n"
"                } __AROS_LTAQUAD(q2); \\\n"
"                __AROS_LTAQUAD(q1).reg[0] = __AROS_LTAQUAD1(q1); \\\n"
"                __AROS_LTAQUAD(q1).reg[1] = __AROS_LTAQUAD2(q1); \\\n"
"                __AROS_LPAQUAD(q1) __attribute__((unused)) __AROS_LCAQUAD(q1) = __AROS_LTAQUAD(q1).val; \\\n"
"                __AROS_LTAQUAD(q2).reg[0] = __AROS_LTAQUAD1(q2); \\\n"
"                __AROS_LTAQUAD(q2).reg[1] = __AROS_LTAQUAD2(q2); \\\n"
"                __AROS_LPAQUAD(q2) __attribute__((unused)) __AROS_LCAQUAD(q2) = __AROS_LTAQUAD(q2).val;\n"
"\n"
"#define AROS_LHQUAD2 __AROS_LHQUAD2\n"
"\n"
"#define __AROS_LH1QUAD2(t,n,a1,q1,q2,bt,bn,o,s) \\\n"
"        __AROS_LH5(t,n, \\\n"
"                AROS_LHA(a1), \\\n"
"                AROS_LHA(ULONG, __AROS_LTAQUAD1(q1), __AROS_LRAQUAD1(q1)), \\\n"
"                AROS_LHA(ULONG, __AROS_LTAQUAD2(q1), __AROS_LRAQUAD2(q1)), \\\n"
"                AROS_LHA(ULONG, __AROS_LTAQUAD1(q2), __AROS_LRAQUAD1(q2)), \\\n"
"                AROS_LHA(ULONG, __AROS_LTAQUAD2(q2), __AROS_LRAQUAD2(q2)), \\\n"
"                bt, bn, o, s) \\\n"
"                union { \\\n"
"                        __AROS_LPAQUAD(q1) val; \\\n"
"                        ULONG reg[2]; \\\n"
"                } __AROS_LTAQUAD(q1); \\\n"
"                union { \\\n"
"                        __AROS_LPAQUAD(q2) val; \\\n"
"                        ULONG reg[2]; \\\n"
"                } __AROS_LTAQUAD(q2); \\\n"
"                __AROS_LTAQUAD(q1).reg[0] = __AROS_LTAQUAD1(q1); \\\n"
"                __AROS_LTAQUAD(q1).reg[1] = __AROS_LTAQUAD2(q1); \\\n"
"                __AROS_LPAQUAD(q1) __attribute__((unused)) __AROS_LCAQUAD(q1) = __AROS_LTAQUAD(q1).val; \\\n"
"                __AROS_LTAQUAD(q2).reg[0] = __AROS_LTAQUAD1(q2); \\\n"
"                __AROS_LTAQUAD(q2).reg[1] = __AROS_LTAQUAD2(q2); \\\n"
"                __AROS_LPAQUAD(q2) __attribute__((unused)) __AROS_LCAQUAD(q2) = __AROS_LTAQUAD(q2).val;\n"
"\n"
"#define AROS_LH1QUAD2 __AROS_LH1QUAD2\n"
"\n"
"#define __AROS_LH1QUAD1(t,n,a1,q1,bt,bn,o,s) \\\n"
"        __AROS_LH3(t,n, \\\n"
"                AROS_LHA(a1), \\\n"
"                AROS_LHA(ULONG, __AROS_LTAQUAD1(q1), __AROS_LRAQUAD1(q1)), \\\n"
"                AROS_LHA(ULONG, __AROS_LTAQUAD2(q1), __AROS_LRAQUAD2(q1)), \\\n"
"                bt, bn, o, s) \\\n"
"                union { \\\n"
"                        __AROS_LPAQUAD(q1) val; \\\n"
"                        ULONG reg[2]; \\\n"
"                } __AROS_LTAQUAD(q1); \\\n"
"                __AROS_LTAQUAD(q1).reg[0] = __AROS_LTAQUAD1(q1); \\\n"
"                __AROS_LTAQUAD(q1).reg[1] = __AROS_LTAQUAD2(q1); \\\n"
"                __AROS_LPAQUAD(q1) __attribute__((unused)) __AROS_LCAQUAD(q1) = __AROS_LTAQUAD(q1).val;\n"
"\n"
"#define AROS_LH1QUAD1 __AROS_LH1QUAD1\n"
"\n"
"#define __AROS_LH2QUAD1(t,n,a1,a2,q1,bt,bn,o,s) \\\n"
"        __AROS_LH4(t,n, \\\n"
"                AROS_LHA(a1), \\\n"
"                AROS_LHA(a2), \\\n"
"                AROS_LHA(ULONG, __AROS_LTAQUAD1(q1), __AROS_LRAQUAD1(q1)), \\\n"
"                AROS_LHA(ULONG, __AROS_LTAQUAD2(q1), __AROS_LRAQUAD2(q1)), \\\n"
"                bt, bn, o, s) \\\n"
"                union { \\\n"
"                        __AROS_LPAQUAD(q1) val; \\\n"
"                        ULONG reg[2]; \\\n"
"                } __AROS_LTAQUAD(q1); \\\n"
"                __AROS_LTAQUAD(q1).reg[0] = __AROS_LTAQUAD1(q1); \\\n"
"                __AROS_LTAQUAD(q1).reg[1] = __AROS_LTAQUAD2(q1); \\\n"
"                __AROS_LPAQUAD(q1) __attribute__((unused)) __AROS_LCAQUAD(q1) = __AROS_LTAQUAD(q1).val;\n"
"\n"
"#define AROS_LH2QUAD1 __AROS_LH2QUAD1\n"
"\n"
"#define __AROS_LH3QUAD1(t,n,a1,a2,a3,q1,bt,bn,o,s) \\\n"
"        __AROS_LH5(t,n, \\\n"
"                AROS_LHA(a1), \\\n"
"                AROS_LHA(a2), \\\n"
"                AROS_LHA(a3), \\\n"
"                AROS_LHA(ULONG, __AROS_LTAQUAD1(q1), __AROS_LRAQUAD1(q1)), \\\n"
"                AROS_LHA(ULONG, __AROS_LTAQUAD2(q1), __AROS_LRAQUAD2(q1)), \\\n"
"                bt, bn, o, s) \\\n"
"                union { \\\n"
"                        __AROS_LPAQUAD(q1) val; \\\n"
"                        ULONG reg[2]; \\\n"
"                } __AROS_LTAQUAD(q1); \\\n"
"                __AROS_LTAQUAD(q1).reg[0] = __AROS_LTAQUAD1(q1); \\\n"
"                __AROS_LTAQUAD(q1).reg[1] = __AROS_LTAQUAD2(q1); \\\n"
"                __AROS_LPAQUAD(q1) __attribute__((unused)) __AROS_LCAQUAD(q1) = __AROS_LTAQUAD(q1).val;\n"
"\n"
"#define AROS_LH3QUAD1 __AROS_LH3QUAD1\n"
"\n"
"#define __AROS_LCQUAD1(t,n,q1,bt,bn,o,s) \\\n"
"        ({ \\\n"
"                union { \\\n"
"                        __AROS_LPAQUAD(q1) val; \\\n"
"                        ULONG reg[2]; \\\n"
"                } _q1 = { .val = __AROS_LCAQUAD(q1) }; \\\n"
"                __AROS_LC2##t(t, n,  \\\n"
"                        AROS_LCA(ULONG, _q1.reg[0], __AROS_LRAQUAD1(q1)), \\\n"
"                        AROS_LCA(ULONG, _q1.reg[1], __AROS_LRAQUAD2(q1)), \\\n"
"                        bt, bn, o, s); \\\n"
"         })\n"
"\n"
"#define AROS_LCQUAD1 __AROS_LCQUAD1\n"
"\n"
"#define __AROS_LCQUAD2(t,n,q1,q2,bt,bn,o,s) \\\n"
"        ({ \\\n"
"                union { \\\n"
"                        __AROS_LPAQUAD(q1) val; \\\n"
"                        ULONG reg[2]; \\\n"
"                } _q1 = { .val = __AROS_LCAQUAD(q1) }; \\\n"
"                union { \\\n"
"                        __AROS_LPAQUAD(q2) val; \\\n"
"                        ULONG reg[2]; \\\n"
"                } _q2 = { .val = __AROS_LCAQUAD(q2) }; \\\n"
"                __AROS_LC4##t(t, n,  \\\n"
"                        AROS_LCA(ULONG, _q1.reg[0], __AROS_LRAQUAD1(q1)), \\\n"
"                        AROS_LCA(ULONG, _q1.reg[1], __AROS_LRAQUAD2(q1)), \\\n"
"                        AROS_LCA(ULONG, _q2.reg[0], __AROS_LRAQUAD1(q2)), \\\n"
"                        AROS_LCA(ULONG, _q2.reg[1], __AROS_LRAQUAD2(q2)), \\\n"
"                        bt, bn, o, s); \\\n"
"         })\n"
"\n"
"#define AROS_LCQUAD2 __AROS_LCQUAD2\n"
"\n"
"#define __AROS_LC1QUAD1(t,n,a1,q1,bt,bn,o,s) \\\n"
"        ({ \\\n"
"                union { \\\n"
"                        __AROS_LPAQUAD(q1) val; \\\n"
"                        ULONG reg[2]; \\\n"
"                } _q1 = { .val = __AROS_LCAQUAD(q1) }; \\\n"
"                __AROS_LC3##t(t, n,  \\\n"
"                        AROS_LCA(a1), \\\n"
"                        AROS_LCA(ULONG, _q1.reg[0], __AROS_LRAQUAD1(q1)), \\\n"
"                        AROS_LCA(ULONG, _q1.reg[1], __AROS_LRAQUAD2(q1)), \\\n"
"                        bt, bn, o, s); \\\n"
"         })\n"
"\n"
"#define AROS_LC1QUAD1 __AROS_LC1QUAD1\n"
"\n"
"#define __AROS_LC2QUAD1(t,n,a1,a2,q1,bt,bn,o,s) \\\n"
"        ({ \\\n"
"                union { \\\n"
"                        __AROS_LPAQUAD(q1) val; \\\n"
"                        ULONG reg[2]; \\\n"
"                } _q1 = { .val = __AROS_LCAQUAD(q1) }; \\\n"
"                __AROS_LC4##t(t, n,  \\\n"
"                        AROS_LCA(a1), \\\n"
"                        AROS_LCA(a2), \\\n"
"                        AROS_LCA(ULONG, _q1.reg[0], __AROS_LRAQUAD1(q1)), \\\n"
"                        AROS_LCA(ULONG, _q1.reg[1], __AROS_LRAQUAD2(q1)), \\\n"
"                        bt, bn, o, s); \\\n"
"         })\n"
"\n"
"#define AROS_LC2QUAD1 __AROS_LC2QUAD1\n"
"\n"
"#define __AROS_LC3QUAD1(t,n,a1,a2,a3,q1,bt,bn,o,s) \\\n"
"        ({ \\\n"
"                union { \\\n"
"                        __AROS_LPAQUAD(q1) val; \\\n"
"                        ULONG reg[2]; \\\n"
"                } _q1 = { .val = __AROS_LCAQUAD(q1) }; \\\n"
"                __AROS_LC5##t(t, n,  \\\n"
"                        AROS_LCA(a1), \\\n"
"                        AROS_LCA(a2), \\\n"
"                        AROS_LCA(a3), \\\n"
"                        AROS_LCA(ULONG, _q1.reg[0], __AROS_LRAQUAD1(q1)), \\\n"
"                        AROS_LCA(ULONG, _q1.reg[1], __AROS_LRAQUAD2(q1)), \\\n"
"                        bt, bn, o, s); \\\n"
"         })\n"
"\n"
"#define AROS_LC3QUAD1 __AROS_LC3QUAD1\n"
"\n"
"#define __AROS_LC2double __AROS_LC2D\n"
"#define __AROS_LC3double __AROS_LC3D\n"
"#define __AROS_LC4double __AROS_LC4D\n"
"#define __AROS_LC2LONG   __AROS_LC2\n"
"#define __AROS_LC3LONG   __AROS_LC3\n"
"#define __AROS_LC4LONG   __AROS_LC4\n"
"#define __AROS_LC5LONG   __AROS_LC5\n"
"\n"
"#   define AROS_LDQUAD1(t,n,q1,bt,bn,o,s) \\\n"
"        __AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \\\n"
"        __AROS_LDAQUAD(q1), __AROS_LD_BASE(bt,bn))\n"
"#   define AROS_LDQUAD2(t,n,q1,q2,bt,bn,o,s) \\\n"
"        __AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \\\n"
"        __AROS_LDAQUAD(q1), \\\n"
"        __AROS_LDAQUAD(q2),__AROS_LD_BASE(bt,bn))\n"
"\n"
"#define AROS_LPQUAD1(t,n,q1,bt,bn,o,s) \\\n"
"                t n (__AROS_LPAQUAD(q1))\n"
"#define AROS_LPQUAD2(t,n,q1,q2,bt,bn,o,s) \\\n"
"                t n (__AROS_LPAQUAD(q1), __AROS_LPAQUAD(q2))\n"
;

int main(int argc, char **argv)
{
    int i;

    if (argc != 2)
    {
        fprintf(stderr,
                "Error in arguments\n"
                "Usage: %s (asmcall|libcall)\n",
                argv[0]
        );
        exit(20);
    }

    if (strcmp(argv[1], "asmcall") == 0)
    {
        printf("/*\n  Copyright \xa9 2019, The AROS Development Team. All rights reserved.\n  This file is AUTOGENERATED from arch/m68k-all/include/gencall.c\n*/\n");
        printf("\n");
        printf("#ifndef AROS_M68K_ASMCALL_H\n");
        printf("#define AROS_M68K_ASMCALL_H\n");
        printf("\n");

        printf("#define __AROS_CPU_SPECIFIC_ASMCALLS\n\n");

        for (i = 0; i < GENCALL_MAX; i++)
            aros_ufp(i, 0);

        for (i = 0; i < GENCALL_MAX; i++)
            aros_ufp(i, 1);

        for (i = 0; i < GENCALL_MAX; i++)
            aros_ufh(i, 0);

        for (i = 0; i < GENCALL_MAX; i++)
            aros_ufh(i, 1);

        for (i = 0; i < GENCALL_MAX; i++)
            aros_ufc(i, 0);

        for (i = 0; i < GENCALL_MAX; i++)
            aros_ufc(i, FLAG_NR);

        printf("\n%s\n", asmextra);

        printf("#endif /* AROS_M68K_ASMCALL_H */\n");
    }
    else if (strcmp(argv[1], "libcall") == 0)
    {
        printf("/*\n  Copyright \xa9 2019, The AROS Development Team. All rights reserved.\n  This file is AUTOGENERATED from arch/m68k-all/include/gencall.c\n*/\n");
        printf("\n");
        printf("#ifndef AROS_M68K_LIBCALL_H\n");
        printf("#define AROS_M68K_LIBCALL_H\n");
        printf("\n");
        printf("/* Call a libary function which requires the libbase */\n");
        printf("\n");

        printf("#define __AROS_CPU_SPECIFIC_LP\n\n");
	
        for (i = 0; i < GENCALL_MAX; i++)
            aros_lp(i, 0);

        for (i = 0; i < GENCALL_MAX; i++)
            aros_lp(i, 1);

        printf("\n");
        printf("#define __AROS_CPU_SPECIFIC_LH\n\n");
	
        for (i = 0; i < GENCALL_MAX; i++)
            aros_lh(i, 0);

        for (i = 0; i < GENCALL_MAX; i++)
            aros_lh(i, 1);

        printf("\n");
        printf("#define __AROS_CPU_SPECIFIC_LC\n\n");
	
        for (i = 0; i < GENCALL_MAX; i++) {
            aros_lc(i, 0);
            aros_lc(i, FLAG_NR);
        }

        /* For double return AROS_LC2D..AROS_LC4D */
        aros_lc(2, FLAG_DOUBLE);
        aros_lc(3, FLAG_DOUBLE);
        aros_lc(4, FLAG_DOUBLE);

        for (i = 0; i < GENCALL_MAX; i++) {
            aros_call(i, 0);
            aros_call(i, FLAG_NR);
        }

        for (i = 0; i < GENCALL_MAX; i++)
            aros_lvo_call(i, 0);

        for (i = 0; i < GENCALL_MAX; i++)
            aros_lvo_call(i, FLAG_NR);

        printf("\n");
        printf("#define __AROS_CPU_SPECIFIC_LD\n\n");
	
        for (i = 0; i < GENCALL_MAX; i++)
            aros_ld(i, 0);

        for (i = 0; i < GENCALL_MAX; i++)
            aros_ld(i, 1);

        printf("\n%s\n", libextra);

        printf("#endif /* AROS_M68K_LIBCALL_H */\n");
    }
    else
    {
        fprintf(stderr,
                "Error in arguments\n"
                "Usage: %s (asmcall|libcall)\n",
                argv[0]
        );
        exit(20);
    }

    return 0;
}
