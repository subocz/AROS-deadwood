/*
    Copyright (C) 2017, The AROS Development Team. All rights reserved.
*/

#ifndef RESOURCES_EXECLOCK_H
#define RESOURCES_EXECLOCK_H

#define LOCKB_DISABLE   0
#define LOCKB_FORBID    1
#define LOCKF_DISABLE   (1 << LOCKB_DISABLE)
#define LOCKF_FORBID    (1 << LOCKB_FORBID)

#define LOCK_MODE_READ  0
#define LOCK_MODE_WRITE 1

#endif /* !RESOURCES_EXECLOCK_H */
