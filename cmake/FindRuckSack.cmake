# Copyright (c) 2014 Andrew Kelley
# This file is MIT licensed.
# See http://opensource.org/licenses/MIT

# RUCKSACK_INCLUDE_DIR
# RUCKSACK_LIBRARY
# RUCKSACK_FOUND
# RUCKSACK_EXECUTABLE

find_path(RUCKSACK_INCLUDE_DIR NAMES rucksack.h)
find_library(RUCKSACK_LIBRARY NAMES rucksack)
find_program(RUCKSACK_EXECUTABLE NAMES rucksack)

if(RUCKSACK_LIBRARY AND RUCKSACK_EXECUTABLE AND RUCKSACK_INCLUDE_DIR)
  set(RUCKSACK_FOUND TRUE)
else()
  set(RUCKSACK_FOUND FALSE)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RUCKSACK DEFAULT_MSG RUCKSACK_LIBRARY RUCKSACK_INCLUDE_DIR)

mark_as_advanced(RUCKSACK_INCLUDE_DIR RUCKSACK_LIBRARY)
