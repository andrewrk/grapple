# Copyright (c) 2014 Andrew Kelley
# This file is MIT licensed.
# See http://opensource.org/licenses/MIT

# TMXPARSER_INCLUDE_DIR
# TMXPARSER_LIBRARIES
# TMXPARSER_FOUND

if(TMXPARSER_INCLUDE_DIR)
  # Already in cache, be silent
  set(TMXPARSER_FIND_QUIETLY TRUE)
endif(TMXPARSER_INCLUDE_DIR)

find_path(TMXPARSER_INCLUDE_DIR tmxparser/Tmx.h
  PATH_SUFFIXES include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw          # Fink
  /opt/local   # DarwinPorts
  /opt/csw     # Blastwave
  /opt
  ${TMXPARSERDIR}
  $ENV{TMXPARSERDIR})

find_library(TMXPARSER_LIBRARY
  tmxparser
  PATH_SUFFIXES lib64 lib
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw          # Fink
  /opt/local   # DarwinPorts
  /opt/csw     # Blastwave
  /opt
  ${TMXPARSERDIR}
  $ENV{TMXPARSERDIR})

if(TMXPARSER_LIBRARY)
  set(TMXPARSER_FOUND TRUE)
  set(TMXPARSER_LIBRARIES ${TMXPARSER_LIBRARY})
else()
  set(TMXPARSER_FOUND FALSE)
  set(TMXPARSER_LIBRARIES)
endif()

# Handle the QUIETLY and REQUIRED arguments and set SNDFILE_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TMXPARSER DEFAULT_MSG TMXPARSER_LIBRARY TMXPARSER_INCLUDE_DIR)

mark_as_advanced(TMXPARSER_INCLUDE_DIR TMXPARSER_LIBRARY)
