INCLUDE(cmake/utils.cmake)


SET(BINDIR bin CACHE PATH "bin")
SET(DATADIR share CACHE PATH "share")
SET(LIBDIR lib CACHE PATH "lib")
SET(LOCALEDIR ${DATADIR}/locale CACHE PATH "Where mo files go")


SET(__MOO_DEFINE_H_FILE__ ${CMAKE_BINARY_DIR}/moo-config.h.in)
FILE(WRITE ${__MOO_DEFINE_H_FILE__} "")

MACRO(MOO_DEFINE_H _moo_varname)
  SET(_moo_comment)
  FOREACH(_moo_arg ${ARGN})
    SET(_moo_comment ${_moo_arg})
  ENDFOREACH(_moo_arg)
  FILE(APPEND ${__MOO_DEFINE_H_FILE__} "\n")
  IF(_moo_comment)
    FILE(APPEND ${__MOO_DEFINE_H_FILE__} "/* ${_moo_comment} */\n")
  ENDIF(_moo_comment)
  FILE(APPEND ${__MOO_DEFINE_H_FILE__} "#cmakedefine ${_moo_varname}\n")
ENDMACRO(MOO_DEFINE_H)

MACRO(MOO_CONFIGURE_FILE _moo_in_file _moo_out_file)
  SET(_moo_abs_in_file ${_moo_in_file})
  IF(NOT EXISTS ${_moo_abs_in_file})
    IF(EXISTS ${CMAKE_SOURCE_DIR}/${_moo_abs_in_file})
      SET(_moo_abs_in_file ${CMAKE_SOURCE_DIR}/${_moo_abs_in_file})
    ENDIF(EXISTS ${CMAKE_SOURCE_DIR}/${_moo_abs_in_file})
  ENDIF(NOT EXISTS ${_moo_abs_in_file})
  IF(NOT EXISTS ${_moo_abs_in_file})
    MESSAGE(FATAL_ERROR "File ${_moo_abs_in_file} does not exist")
  ENDIF(NOT EXISTS ${_moo_abs_in_file})

  SET(_moo_abs_out_file ${_moo_out_file})
  IF(NOT IS_ABSOLUTE ${_moo_abs_out_file})
    SET(_moo_abs_out_file ${CMAKE_BINARY_DIR}/${_moo_abs_out_file})
  ENDIF(NOT IS_ABSOLUTE ${_moo_abs_out_file})

  MOO_COPY_FILE(${_moo_abs_in_file} ${_moo_abs_out_file}-in.moo)
  FILE(READ ${__MOO_DEFINE_H_FILE__} _moo_contents)
  FILE(APPEND ${_moo_abs_out_file}-in.moo "\n")
  FILE(APPEND ${_moo_abs_out_file}-in.moo ${_moo_contents})
  CONFIGURE_FILE(${_moo_abs_out_file}-in.moo ${_moo_abs_out_file} ${ARGN})
ENDMACRO(MOO_CONFIGURE_FILE)

INCLUDE(cmake/i18n.cmake)
