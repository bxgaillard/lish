/*
 * ----------------------------------------------------------------------------
 *
 * Lish: Lightweight Interactive SHell
 * Copyright (C) 2005 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: src/command.h
 *
 * Description: Custom French Style to English C Style Translations
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


#ifndef _COMMAND_H_
#define _COMMAND_H_

/*
 * Include this header before the defines to avoid possible conflicts
 */
#include <stdio.h>

/*
 * Just define the French names by their English equivalents
 */
#define mots           words
#define mot            word
#define suiv           next
#define Mots           words_t
#define commande       command
#define Simple         simple_t
#define FICHIER        RFILE
#define DESCRIPTEUR    DESCRIPTOR
#define redirfichier   redir_file
#define redirdesc      redir_desc
#define Redirection    redirection_t
#define fichier        file
#define RedirFichier   redir_file_t
#define RedirDesc      redir_desc_t
#define redirigee      redirected
#define Redirigee      redirected_t
#define Pipeline       pipeline_t
#define condop         cond_op
#define Condop         cond_op_t
#define conditionnelle conditional
#define Conditionnelle conditional_t
#define seqop          seq_op
#define Seqop          seq_op_t
#define Sequence       sequence_t
#define Commande       command_t

/*
 * Include the concerned header
 */
#include <commande.h>

/*
 * Defines not needed anymore
 */
#undef mots
#undef mot
#undef suiv
#undef Mots
#undef commande
#undef Simple
#undef FICHIER
#undef DESCRIPTEUR
#undef redirfichier
#undef redirdesc
#undef Redirection
#undef fichier
#undef RedirFichier
#undef RedirDesc
#undef redirigee
#undef Redirigee
#undef Pipeline
#undef condop
#undef Condop
#undef conditionnelle
#undef Conditionnelle
#undef seqop
#undef Seqop
#undef Sequence
#undef Commande

/*
 * Also re-define function names
 */
#define parse_command parse_commande
#define dump_command  dump_commande
#define free_command  free_commande

#endif /* !_COMMAND_H_ */

/* End of file */
