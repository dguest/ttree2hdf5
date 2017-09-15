# Set the minimum CMake version required to build the project.
cmake_minimum_required( VERSION 3.1 )

# Silence some warnings on macOS with new CMake versions.
if( NOT ${CMAKE_VERSION} VERSION_LESS 3.9 )
   cmake_policy( SET CMP0068 NEW )
endif()

# Set the project's name and version.
project( ttree2hdf5 VERSION 1.0 )

# Set the warning flag(s) to use.
set( CMAKE_CXX_FLAGS "-Wall -pedantic" )

include_directories(include)

list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
#---Locate the ROOT package and defines a number of variables (e.g. ROOT_INCLUDE_DIRS)
find_package(ROOT REQUIRED COMPONENTS MathCore RIO Hist Tree Net Core Matrix Physics)

include(${ROOT_USE_FILE})
ROOT_GENERATE_DICTIONARY(G__Stl include/Stl.h LINKDEF include/LinkDef.h)

find_package( Boost 1.54.0 REQUIRED COMPONENTS program_options)
#Ideally we would like to use this, but can't
#find_package( HDF5 COMPONENTS CXX HL)

# We're waiting on a fix for this here:
# https://sft.its.cern.ch/jira/browse/SPI-984

# So instead we hack it
set( HDF5_DIR "/afs/cern.ch/user/d/dguest/afswork/public/hdf5/hdf5-1.8.19/install/")
set( HDF5_INCLUDE "${HDF5_DIR}/include/")
set( HDF5_LIB "${HDF5_DIR}/lib/")

set( lib_headers
  include/HdfTuple.hh
  include/LinkDef.h
  include/Stl.h
  include/copy_root_tree.hh
  include/get_tree.hh
  include/tree_copy_opts.hh
  include/unshittify.hh
)

set( lib_sources
  src/HdfTuple.cxx
  src/copy_root_tree.cxx
  src/get_tree.cxx
  src/tree_copy_opts.cxx
  src/ttree2hdf5.cxx
  src/unshittify.cxx
)

add_library( Ttree2hdf5 SHARED ${lib_headers} ${lib_sources} G__Stl)
include_directories(BEFORE SYSTEM ${HDF5_INCLUDE})
target_include_directories( Ttree2hdf5
  PRIVATE ${HDF5_INCLUDE}
  PUBLIC ${Boost_INCLUDE_DIRS} ${ROOT_INCLUDE_DIRS}
)
add_executable(ttree2hdf5 src/ttree2hdf5.cxx)
target_link_libraries(ttree2hdf5 Ttree2hdf5
  ${Boost_LIBRARIES}
  ${ROOT_LIBRARIES}
  ${HDF5_LIB}/libhdf5_cpp.so
  ${HDF5_LIB}/libhdf5.so
)


