## Process this file with automake to produce Makefile.in

## Created by Anjuta

AM_CPPFLAGS = \
	$(WX_CXXFLAGS) \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" 

AM_CFLAGS =\
	 -Wall \
	 -g \
	 $(WX_CPPFLAGS)

bin_PROGRAMS = wxterminal

wxterminal_SOURCES = \
	main.cc     \
	terminal-ctrl.hpp     \
	terminal-ctrl.cpp

wxterminal_LDFLAGS = 

wxterminal_LDADD = \
	 \
	$(WX_LIBS)

