/*
 * ----------------------------------------------------------------------------
 *
 * Lish: Lightweight Interactive SHell
 * Copyright (C) 2005 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: src/version.c
 *
 * Description: Version Information
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


/* Project headers */
#include <common.h>
#include "version.h"

/* Macros to make a version string */
#define MAKE_STRING(s) #s
#define MAKE_VERSION_STRING(major, minor, release) \
        MAKE_STRING(major) "." MAKE_STRING(minor) "." MAKE_STRING(release)
#define VERSION_STRING MAKE_VERSION_STRING(VERSION_MAJOR, VERSION_MINOR, \
					   VERSION_RELEASE)

/* Name and version strings */
const char lish_name[] = LISH_NAME " " VERSION_NAME;
const char lish_version[] = VERSION_STRING;

/* End of file */
