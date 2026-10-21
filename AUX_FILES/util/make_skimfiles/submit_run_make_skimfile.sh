#!/bin/bash

# ------------------------------------------------------------------------- #
# This script is a wrapper for the run_make_skimfile.sh script that makes   # 
# skim files out of R-SIDIS hcana ROOT files.                               #
# ---------                                                                 #
# P. Datta <pdbforce@jlab.org> CREATED 10-15-2025                           #
# ---------                                                                 #
# ** Do not tamper with this sticker! Log any updates to the script above.  #
# ------------------------------------------------------------------------- #

# -------------------------------------------------------------------------- #
# ----------------- VERY IMPORTANT TO SET UP CORRECTLY !!! ----------------- #
# -------------------------------------------------------------------------- #
# Path to make_skimfiles directory. It MUST include all the following scripts
# 1. submit_run_make_skimfile.sh
# 2. run_make_skimfile.sh
# 3. make_skimmed_rootfile.C
# ---
SCRIPT_DIR=/u/group/c-rsidis/pdbforce/pass1_prep/statscounter/hallc_replay_rsidis/AUX_FILES/util/make_skimfiles

runlist=$1        # run list (single column txt file w/ run numbers to analyze)
runtype=$2        # run type : SIDIS / HMSHEEP / SHMSHEEP / HMSDIS / SHMSDIS
indir=$3          # input directory (directory with R-SIDIS hcana ROOT files)
run_on_ifarm=$4   # want to run on ifarm instead of batch farm? 1 => yes
reportdir=${5:-$indir} # report directory; defaults to input ROOT directory

workflowname="rsidis-pass1-${runtype}-skimmed"
outdirpath="/lustre24/expphy/volatile/hallc/c-rsidis/pdbforce/replay/pass1_skimmed"     # output directory (destination for the generated skim files)

# Job specifications
jram='2200MB'    # per-job requested RAM
jtime='2h'       # per-job requested walltime
jdisk='5GB'      # per-job requested disk space (very important to specify)

# Sanity check 1: Validating the number of arguments provided
if [[ "$#" -lt 4 || "$#" -gt 5 ]]; then
    echo -e "\n--!--\n Illegal number of arguments!!"
    echo -e " This script expects: <runlist> <runtype> <indir> <run_on_ifarm> [reportdir]\n"
    exit;
else 
    echo -e '\n------'
    echo -e ' Check the following variable(s):'
    if [[ $run_on_ifarm -ne 1 ]]; then
	echo -e ' "workflowname" : '$workflowname''
    fi
    echo -e ' "outdirpath"   : '$outdirpath' \n------'
    while true; do
	read -p "Do they look good? [y/n] " yn
	echo -e ""
	case $yn in
	    [Yy]*) 
		break; ;;
	    [Nn]*) 
		if [[ $run_on_ifarm -ne 1 ]]; then
		    read -p "Enter desired workflowname : " temp1
		    workflowname=$temp1
		fi
		read -p "Enter desired outdirpath   : " temp2
		outdirpath=$temp2		
		break; ;;
	esac
    done
fi

# Creating the workflow
if [[ $run_on_ifarm -ne 1 ]]; then
    swif2 create -workflow ${workflowname}
else
    echo -e "\nRunning all jobs on ifarm!\n"
fi

while read run; do
    # Define the base script path
    script_path=$SCRIPT_DIR'/run_make_skimfile.sh'
    # Define the arguments
    args="${run} ${runtype} ${indir} ${outdirpath} ${run_on_ifarm} ${SCRIPT_DIR} ${reportdir}"
    
    if [[ $run_on_ifarm -ne 1 ]]; then
        swif2 add-job \
              -workflow ${workflowname} \
              -name rsidis_skim_${runtype}_${run} \
              -partition production \
              -cores 1 \
              -ram $jram \
              -time $jtime \
	      -disk $jdisk \
              /bin/bash $script_path $args
    else
        /bin/bash $script_path $args
    fi
done < "$runlist"

# run the workflow and then print status
if [[ $run_on_ifarm -ne 1 ]]; then
    swif2 run -workflow ${workflowname}
    echo -e "\n Getting workflow status.. [may take a few minutes!] \n"
    swif2 status -workflow ${workflowname}
fi

# Example execution
# Use HMSHEEP when the electron is in HMS, or SHMSHEEP when it is in SHMS.
#./submit_run_make_skimfile.sh runlists/pass1/heep_pass1_final.txt HMSHEEP /cache/hallc/c-rsidis/analysis/replays/pass1 0
#./submit_run_make_skimfile.sh runlists/pass1/heep_pass1_final.txt SHMSHEEP /cache/hallc/c-rsidis/analysis/replays/pass1 0
#./submit_run_make_skimfile.sh runlists/pass1/hms_pass1_final.txt HMSDIS /cache/hallc/c-rsidis/analysis/replays/pass1 0
#./submit_run_make_skimfile.sh runlists/pass1/shms_pass1_final.txt SHMSDIS /cache/hallc/c-rsidis/analysis/replays/pass1 0
#./submit_run_make_skimfile.sh runlists/pass1/coin_pass1_final.txt SIDIS /cache/hallc/c-rsidis/analysis/replays/pass1 0
