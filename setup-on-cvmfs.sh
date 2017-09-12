# setup ATLAS
export ATLAS_LOCAL_ROOT_BASE=/cvmfs/atlas.cern.ch/repo/ATLASLocalRootBase
alias setupATLAS='source ${ATLAS_LOCAL_ROOT_BASE}/user/atlasLocalSetup.sh'


# setup script for HDF5 dumper
echo -n "setting up ATLAS..."
export ATLAS_LOCAL_ROOT_BASE=/cvmfs/atlas.cern.ch/repo/ATLASLocalRootBase
. ${ATLAS_LOCAL_ROOT_BASE}/user/atlasLocalSetup.sh -q
echo " done"

# -- get boost --
echo -n "setting up boost..."
localSetupBoost boost-1.60.0-python2.7-x86_64-slc6-gcc49 -q
# required by makefile
export BOOST_PATH=$ALRB_BOOST_ROOT
echo " done"

# -- get root --
echo -n "setting up root..."
localSetupROOT -q
echo " done"

# -- get HDF5 --
echo -n "setting up HDF5..."
lsetup hdf5 -q
# lsetup "hdf5 1.10.0-x86_64-slc6-gcc62-opt"
export HDF_PATH=$ALRB_HDF5_PATH
echo " done"



