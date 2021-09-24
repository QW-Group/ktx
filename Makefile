#   QWProgs-DM
#   Copyright (C) 2004  [MAD]ApxuTekTop
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#
#   $Id$

RM=/bin/rm


SRCDIR=src
INCDIR=include
PREFIX=/usr/local

MAKEDL=$(MAKE) -f Makefile.dl
MAKEDL32=$(MAKE) -f Makefile.dl32
MAKEQVM=$(MAKE) -f Makefile.vm

default:    dl

all:        build

dl:         check build-dlbots

dl32:       check build-dl32bots

vm:         check build-vmbots

build:      check build-dlbots build-dl32bots build-vmbots

build-dl:
			+cd $(SRCDIR); $(MAKEDL) build

build-dlbots:
			+cd $(SRCDIR); $(MAKEDL) build BOT_SUPPORT=1

build-dl32:
			+cd $(SRCDIR); $(MAKEDL32) build

build-dl32bots:
			+cd $(SRCDIR); $(MAKEDL32) build BOT_SUPPORT=1

build-vm:
			+cd $(SRCDIR); $(MAKEQVM) build
build-vmbots:
			+cd $(SRCDIR); $(MAKEQVM) build BOT_SUPPORT=1


install:	install-dl install-dl32 install-vm

install-dl:
			+cd $(SRCDIR); $(MAKEDL) PREFIX=$(PREFIX) install

install-dl32:
			+cd $(SRCDIR); $(MAKEDL32) PREFIX=$(PREFIX) install

install-vm:
			+cd $(SRCDIR); $(MAKEQVM) PREFIX=$(PREFIX) install


clean:		clean-local clean-include clean-dl clean-dl32 clean-vm

clean-local:
			$(RM) -f *~
			$(RM) -rf autom4te.cache

clean-include:
			+cd $(INCDIR); $(RM) -f *~

clean-dl:
			+cd $(SRCDIR); $(MAKEDL) clean

clean-dl32:
			+cd $(SRCDIR); $(MAKEDL32) clean

clean-vm:
			+cd $(SRCDIR); $(MAKEQVM) clean


distclean:	distclean-local distclean-include distclean-dl distclean-dl32 distclean-vm

distclean-local:
			$(RM) -f *~ *.orig *.rej *.tmp
			$(RM) -f Makefile config.log config.status
			$(RM) -rf autom4te.cache

distclean-include:
			+cd $(INCDIR); $(RM) -f *~ *.orig *.rej *.tmp

distclean-dl:
			+cd $(SRCDIR); $(MAKEDL) distclean

distclean-dl32:
			+cd $(SRCDIR); $(MAKEDL32) distclean

distclean-vm:
			+cd $(SRCDIR); $(MAKEQVM) distclean

check:
ifeq (,$(wildcard $(SRCDIR)/Makefile.dl))
			+./configure
endif
