SET(CMAKE_CXX_COMPILER "C:/opt/watcom/binnt/wcl386.exe")
SET(CMAKE_CXX_COMPILER_ARG1 "")
SET(CMAKE_CXX_COMPILER_ID "Watcom")
SET(CMAKE_CXX_PLATFORM_ID "Windows")

SET(CMAKE_AR "C:/cyg/bin/ar.exe")
SET(CMAKE_RANLIB "C:/cyg/bin/ranlib.exe")
SET(CMAKE_LINKER "C:/cyg/bin/ld.exe")
SET(CMAKE_COMPILER_IS_GNUCXX )
SET(CMAKE_CXX_COMPILER_LOADED 1)
SET(CMAKE_COMPILER_IS_MINGW )
SET(CMAKE_COMPILER_IS_CYGWIN )
IF(CMAKE_COMPILER_IS_CYGWIN)
  SET(CYGWIN 1)
  SET(UNIX 1)
ENDIF(CMAKE_COMPILER_IS_CYGWIN)

SET(CMAKE_CXX_COMPILER_ENV_VAR "CXX")

IF(CMAKE_COMPILER_IS_MINGW)
  SET(MINGW 1)
ENDIF(CMAKE_COMPILER_IS_MINGW)
SET(CMAKE_CXX_COMPILER_ID_RUN 1)
SET(CMAKE_CXX_IGNORE_EXTENSIONS inl;h;hpp;HPP;H;o;O;obj;OBJ;def;DEF;rc;RC)
SET(CMAKE_CXX_SOURCE_FILE_EXTENSIONS C;M;c++;cc;cpp;cxx;m;mm;CPP)
SET(CMAKE_CXX_LINKER_PREFERENCE 30)
SET(CMAKE_CXX_LINKER_PREFERENCE_PROPAGATES 1)

# Save compiler ABI information.
SET(CMAKE_CXX_SIZEOF_DATA_PTR "4")
SET(CMAKE_CXX_COMPILER_ABI "")

IF(CMAKE_CXX_SIZEOF_DATA_PTR)
  SET(CMAKE_SIZEOF_VOID_P "${CMAKE_CXX_SIZEOF_DATA_PTR}")
ENDIF(CMAKE_CXX_SIZEOF_DATA_PTR)

IF(CMAKE_CXX_COMPILER_ABI)
  SET(CMAKE_INTERNAL_PLATFORM_ABI "${CMAKE_CXX_COMPILER_ABI}")
ENDIF(CMAKE_CXX_COMPILER_ABI)

SET(CMAKE_CXX_HAS_ISYSROOT "")


SET(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "")
SET(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES "")