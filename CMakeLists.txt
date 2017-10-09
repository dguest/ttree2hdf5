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
# TODO: figure out how to use this in the hdf5 builder. We should pass
# it an option like `--with-zlib=${ZLIB_INCLUDE_DIRS},${ZLIB_LIB_DIR}`
# but this screws up on some systems because the strings from FindZLIB
# are generators

# find HDF5
#
# Ideally we would like to use find_package, but in ATLAS code we're
# waiting on a fix for this here:
#
# https://sft.its.cern.ch/jira/browse/SPI-984
#
option(BUILTIN_HDF5 "Build hdf5 internally" NO)
if( BUILTIN_HDF5 )
  # one option is to build HDF5 here
  set( H5_LOC ${CMAKE_BINARY_DIR}/externals/hdf5 )
  set( HDF5_INCLUDE_DIRS "${H5_LOC}/include/")
  set( HDF5_LIB "${H5_LOC}/lib/")
  set( SUFFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX})
  set( HDF5_LIBRARIES
    ${HDF5_LIB}/libhdf5_cpp${SUFFFIX} ${HDF5_LIB}/libhdf5${SUFFFIX} )

  # count processors
  include(ProcessorCount)
  ProcessorCount(NProcs)
  message(STATUS "Building HDF5 with ${NProcs} cores")
  if(NProcs EQUAL 0)
    message(" *** Processor count failed, using 1 core *** ")
    set(NProcs 1)
  endif()
  include( ExternalProject )

  # add the external
  ExternalProject_Add( HDF5
    PREFIX ${H5_LOC}
    URL "https://support.hdfgroup.org/ftp/HDF5/current18/src/hdf5-1.8.19.tar.gz"
    URL_MD5 "7f568e2464d4ab0a74d16b23956d900b"
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
    --enable-cxx
    BUILD_COMMAND make -j${NProcs}
    INSTALL_COMMAND make install
    BUILD_BYPRODUCTS ${HDF5_LIBRARIES} )
else()
  find_package( HDF5 1.8.12 REQUIRED COMPONENTS CXX HL)
endif()


# Add the hdf tuple wrapper
#
# We build two, one static and one shared
#
option(BUILD_SHARED_HDFTUPLE "Build shared library for hdf5 tuple" ON)
if( BUILD_SHARED_HDFTUPLE )
  add_library(HdfTuple-shared SHARED src/HdfTuple.cxx)
  target_link_libraries(HdfTuple-shared ${HDF5_LIBRARIES} ${ZLIB_LIBRARIES})
  target_include_directories(HdfTuple-shared
    PRIVATE ${HDF5_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS})
  install(TARGETS HdfTuple-shared LIBRARY DESTINATION lib)
endif()
add_library(HdfTuple-static STATIC src/HdfTuple.cxx)
target_link_libraries(HdfTuple-static ${HDF5_LIBRARIES} ${ZLIB_LIBRARIES})
target_include_directories(HdfTuple-static
  PRIVATE ${HDF5_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS})
install( TARGETS HdfTuple-static ARCHIVE DESTINATION lib )

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
