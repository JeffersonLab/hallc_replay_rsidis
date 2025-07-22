#!/bin/bash

run_number=$1
events=$2
spec=$3 #valid options (case sensitive): hms or shms

if [ -z "$run_number" ]; then
    echo "[ERROR] Run number is required."
    exit 1
fi

if [ -z "$events" ]; then
    events=50000
fi

SPEC=$(echo "$spec" | tr '[:lower:]' '[:upper:]')

# Paths
script="SCRIPTS/${SPEC}/PRODUCTION/replay_production_${spec}_coin.C"
analysis="get_good_dis_ev.C"
rootFileDir="./ROOTfiles"
reportFileDir="./REPORT_OUTPUT/${SPEC}/PRODUCTION"
batchOutDir="./REPORT_OUTPUT/${SPEC}/PRODUCTION/BATCH"
monPdfDir="./HISTOGRAMS/${SPEC}/PDF"
reportMonDir="./UTIL_OL/REP_MON"

# File names
replayFile="${spec}_coin_replay_production_${run_number}"
rootFile="${replayFile}_${events}.root"

outExpertFile="summaryPlots_${run_number}_${spec}_production_rsidis"

reportFile="${reportFileDir}/replay_${spec}_coin_production_${run_number}_${events}.report"
replayLog="${batchOutDir}/replay_${spec}_coin_production_${run_number}_${events}.report.log"

echo "[INFO] Replay Log: $replayLog"
# Start logging
{
  echo "=================================================================="
  echo "Running HMS singles analysis for run $run_number with $events events"
  echo "Start time: $(date)"
  echo "=================================================================="

  sleep 1
  hcana -q "${script}(${run_number}, ${events})"

  echo ""
  echo "------------------------------------------------------------------"
  echo "Calculating good DIS events"
  echo "------------------------------------------------------------------"

  sleep 1
  hcana -l -b -q "${analysis}(${run_number},${events},\"${SPEC}\")"

  echo ""
  echo "------------------------------------------------------------------"
  echo "Analysis complete for run $run_number"
  echo "End time: $(date)"
  echo "=================================================================="
} 2>&1 | tee "${replayLog}"

