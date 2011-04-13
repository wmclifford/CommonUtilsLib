# ########## ########## ########## ########## ########## ########## ########## ########## ########## ########## ##########
#
# CommonUtilsLib/src/CMakeLists.txt
#
# ########## ########## ########## ########## ########## ########## ########## ########## ########## ########## ##########

cmake_minimum_required ( VERSION 2.6 )
project ( common_utils )

include ( FindThreads )
include ( ../cmake.includes/FindStandardIncludes.cmake )

configure_file ( "${PROJECT_SOURCE_DIR}/config.h.in" "${PROJECT_BINARY_DIR}/config.h" )
configure_file ( "${PROJECT_SOURCE_DIR}/gccpch.h.in" "${PROJECT_BINARY_DIR}/gccpch.h" )

# ---------- ---------- ---------- ---------- ---------- ---------- ---------- ----------
# Include the project binary (build) directory to have access to
# the "config.h" and "gccpch.h" files.
# ---------- ---------- ---------- ---------- ---------- ---------- ---------- ----------
include_directories ( ${PROJECT_BINARY_DIR} )

# ---------- ---------- ---------- ---------- ---------- ---------- ---------- ----------
# This is a static library that will be included by the main project(s) as
# a dependency. It is a helper library and not meant to be installed anywhere.
# ---------- ---------- ---------- ---------- ---------- ---------- ---------- ----------
add_library (
  ${PROJECT_NAME} STATIC
    child-process.c
    child-process-mgr.c
    circ-link-list.c
    custom-pipes.c
    io-scheduler.c
    logging-svc.c
    mem_pool.c
    process_mgmt.c
    single-link-list.c
    socket-mgr.c
    stack.c
    tcp_service.c
    tcp_socks.c
    unix_socks.c
    udp_socks.c
)

# ########## ########## ########## ########## ########## ########## ########## ########## ########## ########## ##########
