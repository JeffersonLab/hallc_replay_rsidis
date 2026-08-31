#!/bin/bash

# Usage: ./run_prod.sh <run_number> <events> <flag> [spec]
# Example: ./run_prod.sh 12345 10000 COIN_PROD
# Example: ./run_prod.sh 12345 10000 HEEP_PROD shms
# Example: ./run_prod.sh 12345 10000 SHMS_PROD shms
# Example: ./run_prod.sh 12345 10000 HMS_PROD hms

source /etc/profile
module use /group/c-rsidis/modulefiles/
module load hcana

run_number=$1
events=$2
flag=$3
spec=$4

# Normalize flag and spectrometer input
FLAG=$(echo "$flag" | tr '[:lower:]' '[:upper:]')
SPEC=$(echo "$spec" | tr '[:lower:]' '[:upper:]')

# Select analysis macro based on flag
case "$FLAG" in
    COIN_PROD)
        analysis="get_good_coin_ev.C"
        ;;
    HEEP_PROD)
        analysis="get_good_heep_ev.C"
        ;;
    SHMS_PROD|HMS_PROD)
        analysis="get_good_dis_ev.C"
        ;;
    *)
        echo "Error: Invalid flag '$FLAG'"
        echo "Valid options: COIN_PROD, HEEP_PROD, SHMS_PROD, HMS_PROD"
        exit 2
        ;;
esac

echo ""
echo "------------------------------------------------------------------"
echo " Running Analysis: $analysis"
echo " Run Number: $run_number"
echo " Events: $events"
if [[ -n "$SPEC" ]]; then
    echo " Spectrometer: $SPEC"
fi
echo "------------------------------------------------------------------"

# Run the selected analysis with correct arguments
if [[ "$FLAG" == "COIN_PROD" || "$FLAG" == "HEEP_PROD" ]]; then
    hcana -l -b -q "${analysis}(${run_number},${events})"
else
    hcana -l -b -q "${analysis}(${run_number},${events},\"${SPEC}\")"
fi
