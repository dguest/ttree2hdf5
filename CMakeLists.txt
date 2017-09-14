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

find_package( Boost 1.54.0 REQUIRED COMPONENTS program_options)
#find_package( HDF5 COMPONENTS CXX HL)

# AHHHHH
set( ALRB_HDF5_PATH "/cvmfs/atlas.cern.ch/repo/ATLASLocalRootBase/x86_64/hdf5/1.10.0-x86_64-slc6-gcc62-opt/hdf5-1.10-install")

file(GLOB hdf5
"${ALRB_HDF5_PATH}/include/*.h"
)

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

add_library( Ttree2hdf5 SHARED ${lib_headers} ${lib_sources} ${hdf5} )
target_include_directories( Ttree2hdf5
  PUBLIC ${Boost_INCLUDE_DIRS} ${HDF5_INCLUDE_DIR} ${ROOT_INCLUDE_DIRS}
)
add_executable(ttree2hdf5 src/ttree2hdf5.cxx)
#target_link_libraries(ttree2hdf5 Ttree2hdf5 ${Boost_LIBRARIES} ${ROOT_LIBRARIES} ${HDF5_CXX_LIBRARIES} ${hdf4lib}/libhdf5_hl.so ${hdf4lib}/libhdf5_hl_cpp.so)
target_link_libraries(ttree2hdf5 Ttree2hdf5 ${Boost_LIBRARIES} ${ROOT_LIBRARIES} ${ALRB_HDF5_PATH}/lib/libhdf5_cpp.so ${ALRB_HDF5_PATH}/lib//libhdf5.so)


