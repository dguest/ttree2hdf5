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

# one option is to build HDF5 here
set( H5_LOC $ENV{AnalysisBaseExternals_DIR} )
set( HDF5_INCLUDE_DIRS "${H5_LOC}/include/")
set( HDF5_LIB "${H5_LOC}/lib/")
set( SUFFFIX -shared${CMAKE_SHARED_LIBRARY_SUFFIX})
# set( HDF5_LIBRARIES
#   ${HDF5_LIB}/libhdf5_cpp${SUFFFIX} ${HDF5_LIB}/libhdf5${SUFFFIX} )
set( HDF5_LIBRARIES
  ${HDF5_LIB}/libhdf5_cpp${SUFFFIX} ${HDF5_LIB}/libhdf5${SUFFFIX} )

# # export to parent scope
# set( HDF5_LIBRARIES ${HDF5_LIBRARIES} PARENT_SCOPE )
# set( HDF5_INCLUDE_DIRS ${HDF5_INCLUDE_DIRS} PARENT_SCOPE )

# Add the hdf tuple wrapper
#
# We build two, one static and one shared
#
option(BUILD_SHARED_HDFTUPLE "Build shared library for hdf5 tuple" OFF)
if( BUILD_SHARED_HDFTUPLE )
  add_library(HdfTuple-shared SHARED src/HdfTuple.cxx)
  target_link_libraries(HdfTuple-shared ${HDF5_LIBRARIES} ${ZLIB_LIBRARIES})
  target_include_directories(HdfTuple-shared
    PRIVATE ${HDF5_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS})
  install(TARGETS HdfTuple-shared LIBRARY DESTINATION lib)
  if( BUILTIN_HDF5 )
    add_dependencies(HdfTuple-shared HDF5)
  endif()
endif()
add_library(HdfTuple-static STATIC src/HdfTuple.cxx)
target_link_libraries(HdfTuple-static ${HDF5_LIBRARIES} ${ZLIB_LIBRARIES})
target_include_directories(HdfTuple-static
  PRIVATE ${HDF5_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS})
install( TARGETS HdfTuple-static ARCHIVE DESTINATION lib )
if( BUILTIN_HDF5 )
  add_dependencies(HdfTuple-static HDF5)
endif()

install( DIRECTORY include/ DESTINATION include)

# set the RPATH so that libraries don't get lost
option( INSTALL_RPATH "Include dependencies in rpath when installing" OFF )
if( INSTALL_RPATH )
  SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

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

  # it's not clear to me where this '.pcm' file is supposed to go, so
  # we put it in a few places and hopefully the user includes one of
  # them.
  set( PCM_FILE ${CMAKE_CURRENT_BINARY_DIR}/libStl_rdict.pcm)
  install(FILES ${PCM_FILE} DESTINATION bin)
  install(FILES ${PCM_FILE} DESTINATION lib)
endif()
