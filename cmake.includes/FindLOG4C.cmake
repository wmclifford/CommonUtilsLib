# - Find log4c
#
# Find the LOG4C libraries.
#
# This module defines the following variables:
#
#     LOG4C_FOUND         - True if LOG4C_INCLUDE_DIR and LOG4C_LIBRARY are found
#     LOG4C_LIBRARIES     - Set when LOG4C_LIBRARY is found
#     LOG4C_INCLUDE_DIRS  - Set when LOG4C_INCLUDE_DIR is found
#
#     LOG4C_INCLUDE_DIR   - Where to find log4c.h, etc.
#     LOG4C_LIBRARY       - The log4c library
#
# ========== ========== ========== ========== ========== ========== ========== ==========

find_path ( LOG4C_INCLUDE_DIR NAMES log4c.h DOC "The LOG4C include directory" )
find_library ( LOG4C_LIBRARY NAMES log4c DOC "The LOG4C library" )

include ( FindPackageHandleStandardArgs )
find_package_handle_standard_args ( LOG4C DEFAULT_MSG LOG4C_LIBRARY LOG4C_INCLUDE_DIR )

if ( LOG4C_FOUND )
  set ( LOG4C_LIBRARIES ${LOG4C_LIBRARY} )
  set ( LOG4C_INCLUDE_DIRS ${LOG4C_INCLUDE_DIR} )
endif ()

mark_as_advanced ( LOG4C_INCLUDE_DIR LOG4C_LIBRARY )
