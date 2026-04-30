# nanobind via python environment

EXECUTE_PROCESS(
  COMMAND python3 -m nanobind --cmake_dir
  OUTPUT_VARIABLE nanobind_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

FIND_PACKAGE(
  Python
  COMPONENTS Interpreter Development
  REQUIRED
)
FIND_PACKAGE(nanobind CONFIG REQUIRED PATHS ${nanobind_DIR})
