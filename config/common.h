/*
 * ---------------------------------------------------------------------------
 *
 * Lish: Lightweight Interactive SHell
 * Copyright (C) 2005 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: config/common.h
 *
 * Description: Common Definitions
 *
 * ---------------------------------------------------------------------------
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * ---------------------------------------------------------------------------
 */


#ifndef _COMMON_H_
#define _COMMON_H_

/*
 * Headers
 */

/* Project headers */
#include <config.h>


/* Unused parameter */
#ifdef __GNUC__
# define UNUSED   __attribute__ ((__unused__))
# define NORETURN __attribute__ ((__noreturn__))
#else
# define UNUSED
# define NORETURN
#endif

/* Redefinitions of functions related to memory allocation for debugging
   purposes (memory allocation tracing) */
#if defined(DEBUG) && !defined(NDEBUG) && defined(HARDDEBUG)

#include <stdio.h>

#ifdef __GNUC__
# define INLINE __inline__
#else
# define INLINE
#endif

unsigned __mt_alloc_count;

static INLINE void *__mt_malloc(const size_t size, const char *const file,
				const int line)
{
    void *res;

    printf("+ [%02d] malloc(%u) [%s:%d]\n", ++__mt_alloc_count, size, file,
	   line);
    res = malloc(size);
    printf("       = 0x%x\n", (unsigned) res);
    return res;
}

static INLINE void __mt_free(const void *const data, const char *const file,
			     const int line)
{
    printf("- [%02d] free(0x%x) [%s:%d]\n", --__mt_alloc_count,
	   (unsigned) data, file, line);
}

#define malloc(x) __mt_malloc(x, __FILE__, __LINE__)
#define free(x)   __mt_free  (x, __FILE__, __LINE__)

#endif /* DEBUG && !NDEBUG && HARDDEBUG */

#endif /* !_COMMON_H_ */

/* End of file */
