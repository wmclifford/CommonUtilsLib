#
# WMC-Identify-Project-Version.cmake
#
# Pulls the version information for the given target from the file "appversion.mk"
# in the current directory.
#
##

function ( WMC_identify_version )
  if ( ARGC GREATER 0 )
    set ( target_name ${ARGV0} )
  else ( ARGC GREATER 0 )
    set ( target_name ${PROJECT_NAME} )
  endif ( ARGC GREATER 0 )
  message ( STATUS "Target : ${target_name}" )
  file ( STRINGS appversion.mk ver_file_info )
  foreach ( line_contents ${ver_file_info} )
    set ( ver_lvl_name "" )
    set ( ver_lvl_val "" )
    string ( REPLACE "=" ";" ver_lvl_inf ${line_contents} )
    list ( GET ver_lvl_inf 0 ver_lvl_name )
    list ( GET ver_lvl_inf 1 ver_lvl_val )
    set ( ${target_name}_VERSION${ver_lvl_name} ${ver_lvl_val} PARENT_SCOPE )
  endforeach ( line_contents )
  unset ( ver_lvl_name )
  unset ( ver_lvl_val )
endfunction ( WMC_identify_version )
