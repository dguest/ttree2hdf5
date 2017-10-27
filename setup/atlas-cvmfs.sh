
# setup script for HDF5 dumper
echo -n "setting up ATLAS..."
export ATLAS_LOCAL_ROOT_BASE=/cvmfs/atlas.cern.ch/repo/ATLASLocalRootBase
. ${ATLAS_LOCAL_ROOT_BASE}/user/atlasLocalSetup.sh -q
echo " done"

echo "================ setting up CMake ======================"
lsetup cmake
echo "============ setting up AnalysisBaseExternals =========="
asetup AnalysisBaseExternals,21.2.4
echo "====================== Done ============================"

HDF5_ROOT=/afs/cern.ch/user/d/dguest/afswork/public/hdf5/hdf5-1.8.19/install/
echo "set HDF5_ROOT to ${HDF5_ROOT}"
