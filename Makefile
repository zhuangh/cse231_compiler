# Makefile for CCIR Gen passes

# Path to top level of LLMV hierarchy
LEVEL = ../../..

# Name of the library to build
LIBRARYNAME = AIR_OPT

# Make the shared library become a loadable module so the tools can
# dlopen/dlsym on the resulting library.
LOADABLE_MODULE = 1

# Incllude the makefile implementation stuff
include $(LEVEL)/Makefile.common

