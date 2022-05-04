#!/bin/bash -e

# Constructs a standalone windows MantidWorkbench installer using NSIS.
# The package is created from a pre-packaged version of Mantid from conda
# and removes any excess that is not necessary in a standalone
# package

# Print usage and exit
function usage() {
  local exitcode=$1
  echo "Usage: $0 [options] package_name"
  echo
  echo "Create a standalone installable package out of a mantidworkbench Conda package."
  echo "Requires mamba to be installed in the running environment, and on the path."
  echo "Options:"
  echo "  -c Optional Conda channel overriding the default mantid"
  echo "  -s Optional Add a suffix to the output mantid file, has to be Unstable, or Nightly or not used"
  echo
  echo "Positional Arguments"
  echo "  package_name: The name of the package exe, i.e. the final name will be '${package_name}.exe'"
  exit $exitcode
}

# Optional arguments
CONDA_CHANNEL=mantid
SUFFIX=""
PACKAGE_NAME=${@: -1} # grab last argument
while [ ! $# -eq 0 ]
do
    case "$1" in
        -c)
            CONDA_CHANNEL="$2"
            shift
            ;;
        -s)
            SUFFIX="$2"
            shift
            ;;
        -h)
            usage 0
            ;;
        *)
            if [ ! -z "$2" ]
            then
              usage 1
            fi
            ;;
  esac
  shift
done

# If suffix is not empty and does not contain Unstable or Nightly then fail.
if [ ! -z "$SUFFIX" ]; then
  if [ "$SUFFIX" != "Unstable" ] && [ "$SUFFIX" != "Nightly" ]; then
    echo "Suffix must either not be passed, or be Unstable or Nightly, for release do not pass this argument."
    exit 1
  fi
fi

# Define variables
THIS_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONDA_ENV=_conda_env
CONDA_ENV_PATH=$THIS_SCRIPT_DIR/$CONDA_ENV
COPY_DIR=$THIS_SCRIPT_DIR/_package_build
CONDA_EXE=mamba

# Sanity check arguments. Especially ensure that paths are not empty as we are removing
# items and we don't want to accidentally clean out system paths
test -n "$PACKAGE_NAME" || usage 1

echo "Cleaning up left over old directories"
rm -rf $COPY_DIR
rm -rf $CONDA_ENV_PATH

mkdir $COPY_DIR

echo "Creating conda env from mantidworkbench and jq"
"$CONDA_EXE" create --prefix $CONDA_ENV_PATH mantidworkbench m2w64-jq notebook --copy -c $CONDA_CHANNEL -c conda-forge -y
echo "Conda env created"

# Determine version information
VERSION=$("$CONDA_EXE" list --prefix "$CONDA_ENV_PATH" '^mantid$' --json | $CONDA_ENV_PATH/Library/mingw-w64/bin/jq.exe --raw-output '.[0].version')
echo "Version number: $VERSION"
echo "Removing jq from conda env"
"$CONDA_EXE" remove --prefix $CONDA_ENV_PATH --yes m2w64-jq
echo "jq removed from conda env"

# Pip install quasielasticbayes so it can be packaged alongside workbench on windows
$CONDA_ENV_PATH/python.exe -m pip install quasielasticbayes

echo "Copying root packages of env files (Python, DLLs, Lib, Scripts, ucrt, and msvc files) to package/bin"
cp $CONDA_ENV_PATH/DLLs $COPY_DIR/bin/ -r
cp $CONDA_ENV_PATH/Lib $COPY_DIR/bin/ -r
cp $CONDA_ENV_PATH/Scripts $COPY_DIR/bin/ -r
cp $CONDA_ENV_PATH/tcl $COPY_DIR/bin/ -r
cp $CONDA_ENV_PATH/python*.* $COPY_DIR/bin/
cp $CONDA_ENV_PATH/msvc*.* $COPY_DIR/bin/
cp $CONDA_ENV_PATH/ucrt*.* $COPY_DIR/bin/

echo "Copying mantid python files into bin"
cp $CONDA_ENV_PATH/Lib/site-packages/mantid* $COPY_DIR/bin/ -r
cp $CONDA_ENV_PATH/Lib/site-packages/ $COPY_DIR/bin/ -r
cp $CONDA_ENV_PATH/Lib/site-packages/workbench $COPY_DIR/bin/workbench -r

echo "Copy all DLLs from env/Library/bin to package/bin"
cp $CONDA_ENV_PATH/Library/bin/*.dll $COPY_DIR/bin/

echo "Copy Mantid specific files from env/Library/bin to package/bin"
cp $CONDA_ENV_PATH/Library/bin/Mantid.properties $COPY_DIR/bin/
cp $CONDA_ENV_PATH/Library/bin/MantidNexusParallelLoader.exe $COPY_DIR/bin/
cp $CONDA_ENV_PATH/Library/bin/mantid-scripts.pth $COPY_DIR/bin/
cp $CONDA_ENV_PATH/Library/bin/MantidWorkbench.exe $COPY_DIR/bin/

echo "Copy env/includes to the package/includes"
cp $CONDA_ENV_PATH/Library/include/eigen3 $COPY_DIR/include/ -r

echo "Copy Instrument details to the package"
cp $CONDA_ENV_PATH/Library/instrument $COPY_DIR/ -r

echo "Constructing package/lib/qt5"
mkdir $COPY_DIR/lib
mkdir $COPY_DIR/lib/qt5
mkdir $COPY_DIR/lib/qt5/bin
cp $CONDA_ENV_PATH/Library/bin/QtWebEngineProcess.exe $COPY_DIR/lib/qt5/bin
cp $CONDA_ENV_PATH/Library/bin/qt.conf $COPY_DIR/lib/qt5/bin
cp $CONDA_ENV_PATH/Library/resources $COPY_DIR/lib/qt5/ -r

echo "Copy plugins to the package"
mkdir $COPY_DIR/plugins
mkdir $COPY_DIR/plugins/qt5
cp $CONDA_ENV_PATH/Library/plugins/platforms $COPY_DIR/plugins/qt5/ -r
cp $CONDA_ENV_PATH/Library/plugins/imageformats $COPY_DIR/plugins/qt5/ -r
cp $CONDA_ENV_PATH/Library/plugins/printsupport $COPY_DIR/plugins/qt5/ -r
cp $CONDA_ENV_PATH/Library/plugins/sqldrivers $COPY_DIR/plugins/qt5/ -r
cp $CONDA_ENV_PATH/Library/plugins/styles $COPY_DIR/plugins/qt5/ -r
cp $CONDA_ENV_PATH/Library/plugins/*.dll $COPY_DIR/plugins/
cp $CONDA_ENV_PATH/Library/plugins/python $COPY_DIR/plugins/ -r

echo "Copy scripts into the package"
cp $CONDA_ENV_PATH/Library/scripts $COPY_DIR/ -r

echo "Copy share files (includes mantid docs) to the package"
cp $CONDA_ENV_PATH/Library/share/doc $COPY_DIR/share/ -r
cp $CONDA_ENV_PATH/Library/share/eigen3 $COPY_DIR/share/ -r

echo "Copy executable file and executable script into package"
cp $THIS_SCRIPT_DIR/MantidWorkbench.exe $COPY_DIR/bin/ -f

# Cleanup pdb files and remove them from bin
echo "Performing some cleanup.... deleting files"
rm -rf $COPY_DIR/bin/*.pdb
find $COPY_DIR -name *.pyc -delete
# Delete extra DLLs
rm -rf $COPY_DIR/bin/api-ms-win*.dll
rm -rf $COPY_DIR/bin/libclang.dll

# Now package using NSIS
echo "Packaging package via NSIS"

# Create a conda environment with nsis installed
NSIS_CONDA_ENV=_nsis_conda_env
NSIS_CONDA_ENV_PATH=$THIS_SCRIPT_DIR/$NSIS_CONDA_ENV
# First remove existing environment if it exists
rm -rf $NSIS_CONDA_ENV_PATH
echo "Creating nsis conda env"
"$CONDA_EXE" create --prefix $NSIS_CONDA_ENV_PATH nsis -c conda-forge -y
echo "Conda nsis env created"

NSIS_SCRIPT=$THIS_SCRIPT_DIR/project.nsi

# Make windows-like paths because NSIS is weird
SCRIPT_DRIVE_LETTER="$(echo ${COPY_DIR:1:1} | tr [:lower:] [:upper:])"
COPY_DIR=${COPY_DIR////\\}
COPY_DIR="$SCRIPT_DRIVE_LETTER:${COPY_DIR:2}"

NSIS_SCRIPT=${NSIS_SCRIPT////\\}
NSIS_SCRIPT="$SCRIPT_DRIVE_LETTER:${NSIS_SCRIPT:2}"

NSIS_OUTPUT_LOG=$THIS_SCRIPT_DIR/nsis_log.txt
NSIS_OUTPUT_LOG=${NSIS_OUTPUT_LOG////\\}
NSIS_OUTPUT_LOG="$SCRIPT_DRIVE_LETTER:${NSIS_OUTPUT_LOG:2}"

# Path to makensis from our nsis conda environment
MAKENSIS_COMMAND=$NSIS_CONDA_ENV_PATH/NSIS/makensis
MAKENSIS_COMMAND=${MAKENSIS_COMMAND////\\}
MAKENSIS_COMMAND="$SCRIPT_DRIVE_LETTER:${MAKENSIS_COMMAND:2}"

LOWER_CASE_SUFFIX="$(echo ${SUFFIX} | tr [:upper:] [:lower:])"

MANTID_ICON=$THIS_SCRIPT_DIR/../../../images/mantidplot$LOWER_CASE_SUFFIX.ico
MANTID_ICON=${MANTID_ICON////\\}
MANTID_ICON="$SCRIPT_DRIVE_LETTER:${MANTID_ICON:2}"

WORKBENCH_ICON=$THIS_SCRIPT_DIR/../../../images/mantid_workbench$LOWER_CASE_SUFFIX.ico
WORKBENCH_ICON=${WORKBENCH_ICON////\\}
WORKBENCH_ICON="$SCRIPT_DRIVE_LETTER:${WORKBENCH_ICON:2}"

NOTEBOOK_ICON=$THIS_SCRIPT_DIR/../../../images/mantid_notebook$LOWER_CASE_SUFFIX.ico
NOTEBOOK_ICON=${NOTEBOOK_ICON////\\}
NOTEBOOK_ICON="$SCRIPT_DRIVE_LETTER:${NOTEBOOK_ICON:2}"

LICENSE_PATH=$THIS_SCRIPT_DIR/../../../LICENSE.txt
LICENSE_PATH=${LICENSE_PATH////\\}
LICENSE_PATH="$SCRIPT_DRIVE_LETTER:${LICENSE_PATH:2}"
echo Workebench Icon: $WORKBENCH_ICON

# Generate uninstall commands to make sure to only remove files that are copied by the installer
echo Generating uninstaller helper files
python $THIS_SCRIPT_DIR/create_uninstall_lists.py --package_dir=$COPY_DIR --output_dir=$THIS_SCRIPT_DIR

# Run the makensis command from our nsis Conda environment
echo makensis /V4 /O\"$NSIS_OUTPUT_LOG\" /DVERSION=$VERSION /DPACKAGE_DIR=\"$COPY_DIR\" /DPACKAGE_SUFFIX=$SUFFIX /DOUTFILE_NAME=$PACKAGE_NAME /DMANTID_ICON=$MANTID_ICON /DWORKBENCH_ICON=$WORKBENCH_ICON /DNOTEBOOK_ICON=$NOTEBOOK_ICON /DMUI_PAGE_LICENSE_PATH=$LICENSE_PATH \"$NSIS_SCRIPT\"
cmd.exe /C "START /wait "" $MAKENSIS_COMMAND /V4 /DVERSION=$VERSION /O\"$NSIS_OUTPUT_LOG\" /DPACKAGE_DIR=\"$COPY_DIR\" /DPACKAGE_SUFFIX=$SUFFIX /DOUTFILE_NAME=$PACKAGE_NAME /DMANTID_ICON=$MANTID_ICON /DWORKBENCH_ICON=$WORKBENCH_ICON /DNOTEBOOK_ICON=$NOTEBOOK_ICON /DMUI_PAGE_LICENSE_PATH=$LICENSE_PATH \"$NSIS_SCRIPT\""
echo "Package packaged, find it here: $THIS_SCRIPT_DIR/$PACKAGE_NAME"

echo "Cleaning up left over files"
rm -rf $CONDA_ENV_PATH
rm -rf $COPY_DIR
rm -rf $NSIS_CONDA_ENV_PATH
rm $THIS_SCRIPT_DIR/uninstall_files.nsh
rm $THIS_SCRIPT_DIR/uninstall_dirs.nsh

echo "Done"