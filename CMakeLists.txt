# Set the minimum CMake version required to build the project.
cmake_minimum_required( VERSION 3.1 )

# Silence some warnings on macOS with new CMake versions.
if( NOT ${CMAKE_VERSION} VERSION_LESS 3.9 )
   cmake_policy( SET CMP0068 NEW )
endif()

# Set the project's name and version.
project( ttree2hdf5 VERSION 1.0 )

# Set the warning flag(s) to use.
set( CMAKE_CXX_STANDARD 14)
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic" )

include_directories(include src)

# find zlib
find_package( ZLIB REQUIRED )
get_filename_component(ZLIB_LIB_DIR ZLIB_LIBRARIES DIRECTORY)

# find HDF5
#
# Ideally we would like to use find_package, but in ATLAS code we're
# waiting on a fix for this here:
#
# https://sft.its.cern.ch/jira/browse/SPI-984
#
set(H5_LOC "FIND" CACHE STRING "Directory where HDF5 lives (or BUILTIN)")
if(H5_LOC STREQUAL "FIND")
  find_package( HDF5 1.8.12 REQUIRED COMPONENTS CXX HL)
else()
  if(H5_LOC STREQUAL "BUILTIN")
    # one option is to build HDF5 here
    include(ProcessorCount)
    ProcessorCount(NProcs)
    message(STATUS "Building HDF5 with ${NProcs} cores")
    if(NProcs EQUAL 0)
      message(" *** Processor count failed, using 1 core *** ")
      set(NProcs 1)
    endif()
    include( ExternalProject )
    ExternalProject_Add( HDF5
      PREFIX ${CMAKE_BINARY_DIR}/externals/hdf5
      URL "https://support.hdfgroup.org/ftp/HDF5/current18/src/hdf5-1.8.19.tar.gz"
      URL_MD5 "7f568e2464d4ab0a74d16b23956d900b"
      BUILD_IN_SOURCE 1
      CONFIGURE_COMMAND ./configure --prefix=<INSTALL_DIR> --enable-cxx --with-zlib=${ZLIB_INCLUDE_DIRS},${ZLIB_LIB_DIR}
      BUILD_COMMAND make -j${NProcs}
      INSTALL_COMMAND make install)
    set(H5_LOC ${CMAKE_BINARY_DIR}/externals/hdf5)
    set(BUILTIN_HDF5 TRUE)
  elseif(NOT IS_DIRECTORY ${H5_LOC})
    message(FATAL_ERROR "HDF5 path ${H5_LOC} does not point to a directory")
  endif()
  # if not a custom build, take the path to be user defined
  set( HDF5_INCLUDE_DIRS "${H5_LOC}/include/")
  set( HDF5_LIB "${H5_LOC}/lib/")
  if(BUILTIN_HDF5)
    set( SUFFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX})
  else()
    set( SUFFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
  endif()
  set( HDF5_LIBRARIES
    ${HDF5_LIB}/libhdf5_cpp${SUFFFIX} ${HDF5_LIB}/libhdf5${SUFFFIX} )
endif()


# Add the hdf tuple wrapper
#
# We build two, one static and one shared
#
add_library(HdfTuple SHARED src/HdfTuple.cxx)
target_link_libraries(HdfTuple ${HDF5_LIBRARIES} ${ZLIB_LIBRARIES})
target_include_directories(HdfTuple
  PRIVATE ${HDF5_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS})

add_library(HdfTuple-static STATIC src/HdfTuple.cxx)
target_link_libraries(HdfTuple-static ${HDF5_LIBRARIES} ${ZLIB_LIBRARIES})
target_include_directories(HdfTuple
  PRIVATE ${HDF5_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS})

if(BUILTIN_HDF5)
  add_dependencies( HdfTuple HDF5 )
  add_dependencies( HdfTuple-static HDF5 )
endif()
install(TARGETS HdfTuple HdfTuple-static
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)

# _______________________________________________________________________
# ROOT stuff
#
# this can be disabled if we just want the library to write out HDF5,
# or for CI tests
#
option( ROOT_DEPENDANTS "Enable converter from ROOT (requires ROOT)" ON )
if( NOT ROOT_DEPENDANTS )
  message(STATUS "Disabled root dependants")
else()
  # find root
  list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
  find_package(ROOT REQUIRED COMPONENTS RIO Hist Tree Net Core)
  # make root dicts
  include(${ROOT_USE_FILE})
  ROOT_GENERATE_DICTIONARY(G__Stl src/Stl.h LINKDEF src/LinkDef.h)

  # find boost
  set(Boost_USE_STATIC_LIBS ON)
  find_package( Boost 1.54.0 REQUIRED COMPONENTS program_options)

  set( lib_sources
    src/copy_root_tree.cxx
    src/get_tree.cxx
    src/tree_copy_opts.cxx
    src/ttree2hdf5.cxx
    src/unshittify.cxx)

  # build main library
  add_library( convert2hdf5 STATIC ${lib_sources})
  target_link_libraries(convert2hdf5
    HdfTuple-static
    ${Boost_LIBRARIES}
    ${ROOT_LIBRARIES})
  target_include_directories( convert2hdf5
    PRIVATE ${HDF5_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS}
    ${ROOT_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS})
  if(BUILTIN_HDF5)
    add_dependencies( convert2hdf5 HDF5 )
  endif()

  # build top level executable
  add_executable(ttree2hdf5 src/ttree2hdf5.cxx G__Stl.cxx)
  target_link_libraries(ttree2hdf5 convert2hdf5)

  # install binary
  install(TARGETS ttree2hdf5 convert2hdf5
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libStl_rdict.pcm DESTINATION bin)
endif()
