/*
 * ----------------------------------------------------------------------------
 *
 * Lish: Lightweight Interactive SHell
 * Copyright (C) 2005 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: src/history.h
 *
 * Description: History Functions (Header)
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


#ifndef _HISTORY_H_
#define _HISTORY_H_

/* Extern variables */
extern int was_old_command;

/* Prototypes */
void  history_init(void);
void  history_exit(void);
char *history_last(void);
char *history_number(int i);
char *history_string(const char *str);
void  history_add(const char *cmd);
void  history_list(void);
void  history_clear(void);

#endif /* !_HISTORY_H_ */

/* End of file */
