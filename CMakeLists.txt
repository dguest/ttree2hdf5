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

# Grab HDF5 from AnalysisBaseExternals.  The way we do it is ugly
# until atlas externals is fixed, see here:
#
# https://gitlab.cern.ch/atlas/atlasexternals/merge_requests/209
#
set( H5_LOC $ENV{AnalysisBaseExternals_DIR} )
set( HDF5_INCLUDE_DIRS "${H5_LOC}/include/")
set( HDF5_LIB "${H5_LOC}/lib/")
set( SUFFFIX -shared${CMAKE_SHARED_LIBRARY_SUFFIX})
set( HDF5_LIBRARIES
  ${HDF5_LIB}/libhdf5_cpp${SUFFFIX} ${HDF5_LIB}/libhdf5${SUFFFIX} )

# Add the hdf tuple wrapper
add_library(HdfTuple-static STATIC src/HdfTuple.cxx)
target_link_libraries(HdfTuple-static ${HDF5_LIBRARIES} ${ZLIB_LIBRARIES})
target_include_directories(HdfTuple-static
  PUBLIC include
  PRIVATE ${HDF5_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS} src)

# find root
# list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
find_package(ROOT REQUIRED COMPONENTS RIO Hist Tree Net Core)

# find boost
set(Boost_USE_STATIC_LIBS ON)
find_package( Boost 1.54.0 REQUIRED COMPONENTS program_options)

set( lib_sources
  src/copy_root_tree.cxx
  src/get_tree.cxx
  src/tree_copy_opts.cxx
  src/ttree2hdf5.cxx)

# build main library
add_library( convert2hdf5 STATIC ${lib_sources})
target_include_directories( convert2hdf5
  PRIVATE ${HDF5_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS}
  ${ROOT_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS})
target_link_libraries(convert2hdf5
  HdfTuple-static
  ${Boost_LIBRARIES}
  ${HDF5_LIBRARIES}
  ${ROOT_LIBRARIES})

# build top level executable
add_executable(ttree2hdf5 src/ttree2hdf5.cxx)
target_include_directories( ttree2hdf5 PRIVATE ${ROOT_INCLUDE_DIRS})
target_link_libraries(ttree2hdf5 convert2hdf5)

