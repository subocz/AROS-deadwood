#ifndef _INLINE_AROS_H
#define _INLINE_AROS_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef AROS_BASE_NAME
#define AROS_BASE_NAME ArosBase
#endif

#define ArosInquireA(taglist) \
	LP1(0x1e, ULONG, ArosInquireA, struct TagItem *, taglist, a0, \
	, AROS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define ArosInquire(tags...) \
	({ULONG _tags[] = { tags }; ArosInquireA((struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#endif /* _INLINE_AROS_H */
