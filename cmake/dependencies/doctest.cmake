# doctest via system
FIND_PACKAGE(doctest REQUIRED)

IF(TARGET doctest::doctest)
  GET_TARGET_PROPERTY(_is_global doctest::doctest IMPORTED_GLOBAL)
  IF(NOT _is_global)
    SET_TARGET_PROPERTIES(
      doctest::doctest
      PROPERTIES IMPORTED_GLOBAL TRUE
    )
  ENDIF()
ELSE()
  ADD_LIBRARY(doctest::doctest INTERFACE IMPORTED GLOBAL)
  SET_PROPERTY(
    TARGET doctest::doctest
    PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${doctest_INCLUDE_DIR}"
  )
ENDIF()
