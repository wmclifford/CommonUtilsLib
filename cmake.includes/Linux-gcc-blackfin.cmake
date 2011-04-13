#
# cmake
# Blackfin uClibc Linux toolchain
#
set ( CMAKE_SYSTEM_NAME Linux )
set ( CMAKE_SYSTEM_VERSION 1 )

# Cross compiler
set ( CMAKE_C_COMPILER bfin-linux-uclibc-gcc )
set ( CMAKE_CXX_COMPILER bfin-linux-uclibc-g++ )

set ( CMAKE_FIND_ROOT_PATH /var/blackfin/uclinux/staging/usr /opt/uClinux/bfin-linux-uclibc /opt/bfin-uclibc )
set ( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )
set ( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
set ( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )

# -isystem /var/blackfin/uclinux/staging/usr/include
add_definitions ( -pipe )
add_definitions ( -isystem /var/blackfin/uclinux/staging/usr/include )
