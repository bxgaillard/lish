# ----------------------------------------------------------------------------
#
# Lish: Lightweight Interactive SHell
# Copyright (C) 2005 Benjamin Gaillard
#
# ----------------------------------------------------------------------------
#
#        File: config/flags.mk
#
# Description: Programs and Flags
#
#     Comment: These are generic programs/flags.  You cas customize them
#              according to your system.
#
# ----------------------------------------------------------------------------
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc., 59
# Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# ----------------------------------------------------------------------------


##############################################################################
#
# Editable configuration parameters
#
#

# Default programs
CC       ?= gcc
YACC      = bison # Only Bison seems to work
LEX      ?= flex
MKDEP    ?= makedepend
AR       ?= ar
RANLIB   ?= ranlib
RM       ?= rm -f
MV       ?= mv -f
LN       ?= ln -sf

# Default flags
CFLAGS    ?= -O2 -fomit-frame-pointer -pipe
DEBUG     ?= -O0 -g -Werror -pipe
WARN      ?= -Wall -W -ansi -pedantic
CPPFLAGS  ?=
LDFLAGS   ?= -s
YACCFLAGS ?=
LEXFLAGS  ?=

# End of file
