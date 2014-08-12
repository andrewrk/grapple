# Copyright (c) 2014 Andrew Kelley
# This file is MIT licensed.
# See http://opensource.org/licenses/MIT

# CHIPMUNK_INCLUDE_DIR
# CHIPMUNK_LIBRARY
# CHIPMUNK_FOUND

find_path(CHIPMUNK_INCLUDE_DIR NAMES chipmunk/chipmunk.h)
find_library(CHIPMUNK_LIBRARY NAMES chipmunk)

if(CHIPMUNK_LIBRARY AND CHIPMUNK_INCLUDE_DIR)
  set(CHIPMUNK_FOUND TRUE)
else()
  set(CHIPMUNK_FOUND FALSE)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CHIPMUNK DEFAULT_MSG CHIPMUNK_LIBRARY CHIPMUNK_INCLUDE_DIR)

mark_as_advanced(CHIPMUNK_INCLUDE_DIR CHIPMUNK_LIBRARY)
