#
# cmake
# i386 Linux toolchain (for controller and control head)
#
set ( CMAKE_SYSTEM_NAME Linux )
set ( CMAKE_SYSTEM_VERSION 1 )

# Cross compiler
set ( CMAKE_C_COMPILER i386-linux-gcc )
set ( CMAKE_CXX_COMPILER i386-linux-g++ )

set ( CMAKE_FIND_ROOT_PATH /opt/epic-toolchain/crosstool-0.43/result/gcc-3.3.4-glibc-2.3.2/i386-pc-linux-gnu /opt/i386-linux )
set ( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )
set ( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
set ( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )

# Additional configuration follows
