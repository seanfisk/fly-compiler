# ------------------------------------------------------------------------------
# General

# LLVM needs C++11
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
# Source is for normal includes, binary is for generated (Bison) includes.
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

# Put the executables in the root of the build directory. Using RUNTIME_OUTPUT_DIRECTORY doesn't seem to work anywhere.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
# Put the static libraries in the root of the build directory. Using ARCHIVE_OUTPUT_DIRECTORY doesn't seem to work anywhere.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
# Other executables need to be able to find our libraries (libfly).
link_directories(${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})

# ------------------------------------------------------------------------------
# Dependencies

# Boost - headers and libraries
set(Boost_USE_STATIC_LIBS ON)
## The library list *does not* include header-only libraries.
find_package(Boost 1.60 REQUIRED
  program_options
  filesystem
  )

## Show Boost info
message(STATUS "Boost Information:")
message(" Boost_FOUND: ${Boost_FOUND}")
message(" Boost_VERSION: ${Boost_VERSION}")
message(" Boost_LIB_VERSION: ${Boost_LIB_VERSION}")
message(" Boost_INCLUDE_DIRS (not cached): ${Boost_INCLUDE_DIRS}")
message(" Boost_INCLUDE_DIR (cached): ${Boost_INCLUDE_DIR}")
message(" Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
message(" Boost_LIBRARIES: ${Boost_LIBRARIES}")
if((DEFINED BOOST_ROOT OR DEFINED BOOSTROOT) AND NOT DEFINED Boost_NO_SYSTEM_PATHS)
  message(WARNING "Please consider using `-DBoost_NO_SYSTEM_PATHS=TRUE' on your cmake command line. Not doing so may cause Boost libraries of multiple versions to be linked.")
endif()

include_directories(${Boost_INCLUDE_DIR})

# LLVM - flags, headers, and libraries, only for flyc
## See <http://llvm.org/docs/CMake.html#embedding-llvm-in-your-project>

find_package(LLVM REQUIRED)
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
# We couldn't find the correct way to add these definitions on a per-target basis.
add_definitions(${LLVM_DEFINITIONS})

find_package(FLEX REQUIRED 2.6)
find_package(BISON REQUIRED 3.0)

# ------------------------------------------------------------------------------
# libfly - Fly standard library

add_library(libfly stdlib.cpp)

# We called the target "libfly" to clearly distinguish it from "flyc" later (which was originally called "fly"). However, CMake uses the bare name to determine the library name, so we must reset it.
set_target_properties(libfly PROPERTIES OUTPUT_NAME fly)

# ------------------------------------------------------------------------------
# flyc - Fly compiler
bison_target(parser parser.yy ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp COMPILE_FLAGS "-Wall")
flex_target(scanner lexer.ll ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp)
add_flex_bison_dependency(scanner parser)

add_executable(flyc
  ${BISON_parser_OUTPUTS}
  ${FLEX_scanner_OUTPUTS}
  main.cpp
  cmdline.cpp
  driver.cpp
  utils.cpp
  data_types.cpp
  codegen/plcontext.cpp
  codegen/targets/asm.cpp
  codegen/targets/ir.cpp
  codegen/targets/native.cpp
  codegen/visitors/nblock.cpp
  codegen/visitors/nident.cpp
  codegen/visitors/ninteger.cpp
  codegen/visitors/nlist.cpp
  )

llvm_map_components_to_libnames(REQ_LLVM_LIBRARIES nativecodegen option)

target_link_libraries(flyc
  # We only need a subset of all the Clang support libraries. The
  # specific libraries which are needed were determined through good
  # old trial and error. The order does matter, since we need to have
  # dependents before dependees.
  clangFrontend
  clangEdit
  clangLex
  clangDriver
  clangBasic
  # LLVM libraries (should be after clang libraries, because some
  # clang libraries depend on these.
  ${REQ_LLVM_LIBRARIES}
  # Boost
  ${Boost_LIBRARIES}
  )
