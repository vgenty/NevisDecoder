#
# This is an example GNUmakefile for my packages
#

# specific names for this package
DICT  = NevisFileIO_FileIODict
SHLIB = libNevisFileIO_FileIO.so

SOURCES = $(filter-out $(DICT).cxx, $(wildcard *.cxx))
FMWK_HEADERS = LinkDef.h $(DICT).h
HEADERS = $(filter-out $(FMWK_HEADERS), $(wildcard *.h))
OBJECTS = $(SOURCES:.cxx=.o)

# include options for this package
INCFLAGS  = -I.                       #Include itself
INCFLAGS  += -I$(LARLITE_COREDIR)     #Include core

# platform-specific options
OSNAME          = $(shell uname -s)
HOST            = $(shell uname -n)
OSNAMEMODE      = $(OSNAME)

LDFLAGS += -L$(LARLITE_LIBDIR) -lLArLite_Base -lLArLite_DataFormat

# call kernel specific compiler setup
include $(LARLITE_BASEDIR)/Makefile/Makefile.${OSNAME}

# call the common GNUmakefile
include $(LARLITE_BASEDIR)/Makefile/GNUmakefile.CORE
