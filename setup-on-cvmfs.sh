# setup ATLAS
export ATLAS_LOCAL_ROOT_BASE=/cvmfs/atlas.cern.ch/repo/ATLASLocalRootBase
alias setupATLAS='source ${ATLAS_LOCAL_ROOT_BASE}/user/atlasLocalSetup.sh'


# setup script for HDF5 dumper
echo -n "setting up ATLAS..."
export ATLAS_LOCAL_ROOT_BASE=/cvmfs/atlas.cern.ch/repo/ATLASLocalRootBase
. ${ATLAS_LOCAL_ROOT_BASE}/user/atlasLocalSetup.sh -q
echo " done"


# -- get HDF5 --
echo -n "setting up HDF5..."
lsetup hdf5
# lsetup "hdf5 1.8.17-linux-centos6-x86_64-gcc447-shared"
# lsetup "hdf5 1.10.0-x86_64-slc6-gcc62-opt"
# lsetup "lcgenv -p LCG_88 x86_64-slc6-gcc49-opt hdf5"
export HDF_PATH=$ALRB_HDF5_PATH
export LD_LIBRARY_PATH=${ALRB_HDF5_PATH}/lib:${LD_LIBRARY_PATH}
export CPLUS_INCLUDE_PATH=${ALRB_HDF5_PATH}/include:$CPLUS_INCLUDE_PATH
echo " done"

# -- get boost --
echo -n "setting up boost..."
lsetup "boost 1.62.0-python2.7-x86_64-slc6-gcc62"
# lsetup boost
# required by makefile
export BOOST_PATH=$ALRB_BOOST_ROOT
export CPLUS_INCLUDE_PATH+=:${ALRB_BOOST_ROOT}/include/boost-1_62/
export LD_LIBRARY_PATH=${ALRB_BOOST_ROOT}/lib:${LD_LIBRARY_PATH}
echo " done"

export EXTRA_LIBS="-L${ALRB_BOOST_ROOT}/lib -L${ALRB_HDF5_PATH}/lib"
# export EXTRA_LIBS+=" -l:libboost_program_options-gcc62-mt-1_62.so"
export EXTRA_LIBS+=" -lboost_program_options-gcc62-mt-1_62"
export EXTRA_HEADER="-I${ALRB_HDF5_PATH}/include"

# *** This is Dan's hardcoded magic ***
# (ROOT_PATH) isn't defined most places
export ROOTSYS=${ROOT_PATH}/bin
# -- get root --
# echo -n "setting up root..."
# localSetupROOT
# lsetup root
# echo " done"




