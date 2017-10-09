
# setup script for HDF5 dumper
echo -n "setting up ATLAS..."
export ATLAS_LOCAL_ROOT_BASE=/cvmfs/atlas.cern.ch/repo/ATLASLocalRootBase
. ${ATLAS_LOCAL_ROOT_BASE}/user/atlasLocalSetup.sh -q
echo " done"

echo "============ setting up AnalysisBaseExternals =========="
asetup AnalysisBaseExternals,21.2.4
echo "====================== Done ============================"
