#
# cmake
# MIPS Linux toolchain
#
set ( CMAKE_SYSTEM_NAME Linux )
set ( CMAKE_SYSTEM_VERSION 1 )

# Cross compiler
set ( CMAKE_C_COMPILER mips-linux-gcc )
set ( CMAKE_CXX_COMPILER mips-linux-g++ )
set ( CMAKE_FIND_ROOT_PATH /opt/uclibc-toolchain/gcc-3.3.x/toolchain_mips /opt/mips-linux )
set ( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )
set ( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
set ( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )

# Additional configuration follows
add_definitions ( -fno-strict-aliasing )
