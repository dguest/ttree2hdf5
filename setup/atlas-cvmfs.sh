
# setup script for HDF5 dumper
echo -n "setting up ATLAS..."
export ATLAS_LOCAL_ROOT_BASE=/cvmfs/atlas.cern.ch/repo/ATLASLocalRootBase
. ${ATLAS_LOCAL_ROOT_BASE}/user/atlasLocalSetup.sh -q
echo " done"

echo "============ setting up AnalysisBaseExternals =========="
asetup AnalysisBaseExternals,21.2.4
echo "====================== Done ============================"

cat <<EOF
WARNING: This code is a hack. We're using a hardcoded path to HDF5 which
lives on AFS only! This should be fixed as soon as we resolve this issue:

https://sft.its.cern.ch/jira/browse/SPI-984

until then this will only work on AFS
EOF
