FILE(WRITE ${CMAKE_BINARY_DIR}/cmake_uninstall.cmake.in
"
IF(NOT EXISTS \"\@CMAKE_CURRENT_BINARY_DIR\@/install_manifest.txt\")
  MESSAGE(FATAL_ERROR \"Cannot find install manifest: \\\"\@CMAKE_CURRENT_BINARY_DIR\@/install_manifest.txt\\\"\")
ENDIF(NOT EXISTS \"\@CMAKE_CURRENT_BINARY_DIR\@/install_manifest.txt\")

FILE(READ \"\@CMAKE_CURRENT_BINARY_DIR\@/install_manifest.txt\" files)
STRING(REGEX REPLACE \"\\n\" \";\" files \"\${files}\")
FOREACH(file \${files})
  MESSAGE(STATUS \"Uninstalling \\\"\$ENV{DESTDIR}\${file}\\\"\")
  IF(EXISTS \"\$ENV{DESTDIR}\${file}\")
    EXECUTE_PROCESS(
      COMMAND \"\@CMAKE_COMMAND\@\" -E remove \"\$ENV{DESTDIR}\${file}\"
      RESULT_VARIABLE rm_retval
    )
    IF(NOT rm_retval EQUAL 0)
      MESSAGE(FATAL_ERROR \"Problem when removing \\\"\$ENV{DESTDIR}\${file}\\\": \${rm_retval}\")
    ENDIF(NOT rm_retval EQUAL 0)
  ENDIF(EXISTS \"\$ENV{DESTDIR}\${file}\")
ENDFOREACH(file)
")
CONFIGURE_FILE("${CMAKE_BINARY_DIR}/cmake_uninstall.cmake.in" "${CMAKE_BINARY_DIR}/cmake_uninstall.cmake" @ONLY)
ADD_CUSTOM_TARGET(uninstall "${CMAKE_COMMAND}" -P "${CMAKE_BINARY_DIR}/cmake_uninstall.cmake")


#############################################################################
#
# Installation dirs
#

IF(WIN32)
  IF(CMAKE_INSTALL_PREFIX STREQUAL "C:/Program Files/MOO")
    SET(CMAKE_INSTALL_PREFIX "C:/Program Files/medit")
  ENDIF(CMAKE_INSTALL_PREFIX STREQUAL "C:/Program Files/MOO")
ENDIF(WIN32)

SET(MOO_DATA_DIR ${DATADIR}/moo CACHE PATH "Where data files go")
SET(MOO_LIB_DIR ${LIBDIR}/moo CACHE PATH "Where lib files go")
SET(MOO_PLUGINS_DIR ${MOO_LIB_DIR}/plugins CACHE PATH "Where plugins go")
SET(MOO_TEXT_LANG_FILES_DIR ${MOO_DATA_DIR}/language-specs CACHE PATH "Where lang files go")
SET(MOO_DOC_DIR ${DATADIR}/doc/medit CACHE PATH "Where docs go")
SET(MOO_HELP_DIR ${MOO_DOC_DIR}/help CACHE PATH "Where html help files go")
FOREACH(name BINDIR DATADIR LIBDIR MOO_DATA_DIR MOO_LIB_DIR MOO_PLUGINS_DIR
             MOO_TEXT_LANG_FILES_DIR LOCALEDIR MOO_DOC_DIR MOO_HELP_DIR)
  SET(${name}_ABS ${CMAKE_INSTALL_PREFIX}/${${name}})
  MARK_AS_ADVANCED(${name})
ENDFOREACH(name)

IF(WIN32)
  SET(_MEDIT_LIBRARIES_DFLT)
  FOREACH(_moo_gtk_dir ${CMAKE_SOURCE_DIR}/../medit-bin-dist)
    IF(IS_DIRECTORY ${_moo_gtk_dir})
      SET(_MEDIT_LIBRARIES_DFLT ${_moo_gtk_dir})
      BREAK()
    ENDIF(IS_DIRECTORY ${_moo_gtk_dir})
  ENDFOREACH(_moo_gtk_dir)
  SET(MEDIT_LIBRARIES ${_MEDIT_LIBRARIES_DFLT} CACHE PATH "Where Gtk libraries are located")
  IF(NOT MEDIT_LIBRARIES)
    MESSAGE(FATAL_ERROR "MEDIT_LIBRARIES variable not set")
  ENDIF(NOT MEDIT_LIBRARIES)
  INSTALL(DIRECTORY ${MEDIT_LIBRARIES}/ DESTINATION ".")
ENDIF(WIN32)


#############################################################################
#
# CPack
#

SET(CPACK_PACKAGE_NAME "medit")
SET(CPACK_PACKAGE_VENDOR ${MOO_PACKAGE_VENDOR})
STRING(REGEX REPLACE "([0-9]+)[.]([0-9]+)[.]([0-9]+)" "\\1" CPACK_PACKAGE_VERSION_MAJOR ${MOO_VERSION})
STRING(REGEX REPLACE "([0-9]+)[.]([0-9]+)[.]([0-9]+)" "\\2" CPACK_PACKAGE_VERSION_MINOR ${MOO_VERSION})
STRING(REGEX REPLACE "([0-9]+)[.]([0-9]+)[.]([0-9]+)" "\\3" CPACK_PACKAGE_VERSION_PATCH ${MOO_VERSION})
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "medit a text editor")
SET(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${MOO_VERSION})
SET(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/COPYING.GPL)
SET(CPACK_RESOURCE_FILE_README ${CMAKE_SOURCE_DIR}/README)
# SET(CPACK_GENERATOR)
SET(CPACK_PACKAGE_EXECUTABLES medit medit)

SET(CPACK_SOURCE_PACKAGE_FILE_NAME "medit-${MOO_VERSION}")
SET(CPACK_SOURCE_IGNORE_FILES "/build/;/[.]hg;/[.]git")

SET(CPACK_PACKAGE_INSTALL_DIRECTORY "medit")
SET(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "medit")
# SET(CPACK_NSIS_MUI_ICON)
# SET(CPACK_NSIS_MUI_UNIICON)
# SET(CPACK_PACKAGE_ICON ${CMAKE_SOURCE_DIR}/moo/mooutils/pixmaps/medit.ico)
# SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS)
# SET(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS)
# SET(CPACK_NSIS_COMPRESSOR)
SET(CPACK_NSIS_DISPLAY_NAME "medit ${MOO_VERSION}")
SET(CPACK_NSIS_PACKAGE_NAME "medit")
# SET(CPACK_NSIS_INSTALLED_ICON_NAME)
SET(CPACK_NSIS_HELP_LINK "http://mooedit.sourceforge.net/")
SET(CPACK_NSIS_URL_INFO_ABOUT "http://mooedit.sourceforge.net/")
SET(CPACK_NSIS_CONTACT ${MOO_PACKAGE_VENDOR})
# SET(CPACK_NSIS_CREATE_ICONS_EXTRA)
# SET(CPACK_NSIS_DELETE_ICONS_EXTRA)

INCLUDE(CPack)
