/*
 * ----------------------------------------------------------------------------
 *
 * Lish: Lightweight Interactive SHell
 * Copyright (C) 2005 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: src/main.h
 *
 * Description: Main Functions (Header)
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


#ifndef _MAIN_H_
#define _MAIN_H_

/* Headers */
#include <common.h>

/* Variables */
extern const char *exe_name; /* Program name, from executable filename      */
extern       char *cwd;      /* Current working directory                   */
extern       int   killed;   /* Number of killed children by signal handler */

/* Prototypes */
int main(int argc, char *argv[]);
void lish_perror(const char *str);
void lish_exit(int error_code) NORETURN;
void lish_abort(void) NORETURN;
void change_cwd(void);
void sig_chld(int sig UNUSED);

#endif /* !_MAIN_H_ */

/* End of file */
