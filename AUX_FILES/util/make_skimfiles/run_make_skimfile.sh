#!/bin/bash

# ------------------------------------------------------------------------- #
# This script skim files out of R-SIDIS hcana ROOT files.                   #
# ---------                                                                 #
# P. Datta <pdbforce@jlab.org> CREATED 10-15-2025                           #
# ---------                                                                 #
# ** Do not tamper with this sticker! Log any updates to the script above.  #
# ------------------------------------------------------------------------- #

# SLURM directives (can be overridden by sbatch or SWIF2)
#SBATCH --job-name=skim_${run}
#SBATCH --output=skim_${run}.out
#SBATCH --error=skim_${run}.err
#SBATCH --time=01:00:00
#SBATCH --partition=production
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --mem=8GB

# list of arguments
run=$1            # run number
runtype=$2        # run type : SIDIS / HMSHEEP / SHMSHEEP / HMSDIS / SHMSDIS
indir=$3          # input directory
outdir=$4         # output directory
run_on_ifarm=$5   # want to run on ifarm instead of batch farm? 1 => yes
SCRIPT_DIR=$6     # Full path to directory that includes make_skimmed_rootfile.C
reportdir=${7:-$indir} # report directory; defaults to input ROOT directory

# Basic validation
if [[ -z "$run" || -z "$runtype" || -z "$indir" || -z "$outdir" ]]; then
    echo "Usage: $0 <run_number> <runtype> <input_dir> <output_dir> <run_on_ifarm> <script_dir> [report_dir]"
    exit 1
fi

ifarmworkdir=${PWD}
if [[ $run_on_ifarm == 1 ]]; then
    SWIF_JOB_WORK_DIR=$ifarmworkdir
    echo -e "Running all jobs on ifarm!"
fi
echo -e 'Work directory = '$SWIF_JOB_WORK_DIR

# Load hallc analyzer environment
MODULES=/etc/profile.d/modules.sh
if [[ $(type -t module) != function && -r ${MODULES} ]]; then 
    source ${MODULES}
fi
module use /group/halla/modulefiles
module load analyzer/1.7.12

# Run analyzer for this single run
analyzer -b -q "$SCRIPT_DIR/make_skimmed_rootfile.C(${run}, \"${runtype}\", \"${indir}\", \"${SWIF_JOB_WORK_DIR}\", \"${reportdir}\")"

# move the output file to out directory
mv skimmed*.root $outdir
