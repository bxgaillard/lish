/*
 * ---------------------------------------------------------------------------
 *
 * Lish: Lightweight Interactive SHell
 * Copyright (C) 2005 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: config/config.h
 *
 * Description: Default Configuration Values
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


#ifndef _CONFIG_H_
#define _CONFIG_H_

/*
 * Constants
 */

/* Version information */
#define LISH_NAME       "Lish"
#define VERSION_NAME    "Ner'zhul"
#define VERSION_MAJOR   1
#define VERSION_MINOR   0
#define VERSION_RELEASE 5

/* Code returned by syntax and internal errors */
#define RET_ERROR 127

/* Buffer sizes */
#define MAX_COMMAND_LENGTH 256 /* Maximum command length                */
#define MAX_COMMANDS       32  /* Maximum number of commands in history */

/* History file */
#define HISTORY_FILE ".history"

/* Shared memory/semaphore keys */
#define MAKE_KEY(a, b, c, d) ((((a) & 0xFF) << 24) | (((b) & 0xFF) << 16) | \
			      (((c) & 0xFF) << 8) | ((d) & 0xFF))
#define SHM_KEY MAKE_KEY('L', 'i', 's', 'h')
#define SEM_KEY (SHM_KEY + 42)

#endif /* !_CONFIG_H_ */
