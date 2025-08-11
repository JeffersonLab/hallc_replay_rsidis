#!/bin/bash

currentRun=$1
if [ -z "$currentRun" ]; then
  echo "Usage: $0 <run_number>"
  exit 1
fi

base_out=/home/cdaq/rsidis-2025/hydra_in/

declare -A dir_map=(
  [HMS]="HMS"
  [SHMS]="SHMS"
  [HMS_RSIDIS]="HMS_RSIDIS"
  [SHMS_RSIDIS]="SHMS_RSIDIS"
  [COIN]="COIN"
)

declare -A plots_by_dir

plots_by_dir[HMS]="
HMS_Time_Shift_Monitoring
HMS_Hodoscope_ADC_Occ_Mult
HMS_Hodoscope_TDC_Occ_Mult
HMS_Hodoscope_Pedestals
HMS_Hodoscope_Pedestal_Monitoring
HMS_Drift_Chamber_Wire_Maps_All_Hits
HMS_Drift_Chamber_Drift_Distance
HMS_Drift_Chamber_Drift_Time
HMS_Focal_Plane
HMS_Target_Quantities
HMS_Cherenkov_Occ_Mult_Ped
HMS_Cherenkov_Pedestal_Monitoring
HMS_Cherenkov_NPE
HMS_Calorimeter_Occupancy
HMS_Calorimeter_Multiplicity
HMS_Calorimeter_Pedestals
HMS_Calorimeter_Pedestal_Monitoring
HMS_Drift_Chamber_Reference_Times
HMS_Trigger_Reference_Times
HMS_Raw_Fast_Raster_No_Track
HMS_Raw_Fast_Raster_Track_Cut
HMS_EPICS_BPM
HMS_PID
HMS_Trigger_Pedestal_Tracking
"

plots_by_dir[HMS_RSIDIS]="
HMS_Kinematics
HMS_Good_DIS_Counts
"

plots_by_dir[SHMS]="
SHMS_Time_Shift_Monitoring
SHMS_Hodoscope_ADC_Occ_Mult
SHMS_Hodoscope_TDC_Occ_Mult
SHMS_Hodoscope_Pedestals
SHMS_Hodoscope_Pedestal_Monitoring
SHMS_Drift_Chamber_Wire_Maps_All_Hits
SHMS_Drift_Chamber_Drift_Distance
SHMS_Drift_Chamber_Drift_Time
SHMS_Focal_Plane
SHMS_Target_Quantites
SHMS_Cherenkov_Occ_Mult
SHMS_Cherenkov_Pedestals
SHMS_Cherenkov_Pedestal_Monitoring
SHMS_Cherenkov_NPE
SHMS_Calorimeter_Occ_Mult
SHMS_Calorimeter_Pedestals
SHMS_Pre_Shower_Pedestal_Monitoring
SHMS_Calorimeter_Pedestal_Monitoring
SHMS_Drift_Chamber_Reference_Times
SHMS_Trigger_Reference_Times
SHMS_Raw_Fast_Raster_No_Track
SHMS_Raw_Fast_Raster_Track_Cut
SHMS_EPICS_BPM
SHMS_PID
SHMS_Trigger_Pedestal_Tracking
"

plots_by_dir[SHMS_RSIDIS]="
SHMS_Kinematics
SHMS_GOOD_DIS_Counts
"

plots_by_dir[COIN]="
COIN_Cointime
COIN_Kinematics_1
COIN_Kinematics_2
COIN_Pt_Coverage
COIN_Beta_vs_Cointime
COIN_Summary
"

# Process each directory
for dir in "${!plots_by_dir[@]}"; do
  input_dir="../HISTOGRAMS/RSIDIS/${currentRun}/${dir}"
  output_dir="${base_out}/${currentRun}"

  if [ ! -d "$output_dir" ]; then
    echo "Creating $output_dir"
    mkdir -p "$output_dir"
    # The output_dir has g+ws permission so hydra can clean dir.
    chmod 2775 "$output_dir"
  fi

  echo "Copying plots from $input_dir to $output_dir"

  i=1
  while IFS= read -r plot_name; do
    [[ -z "$plot_name" ]] && continue  # skip empty lines

    index=$(printf "%02d" $i)
    src="${input_dir}/tmp_${index}.png"
    dst="${output_dir}/${plot_name}.png"

    if [ -f "$src" ]; then
      cp "$src" "$dst"
      echo "Copied $src -> $dst"
    else
      echo "Warning: Missing file $src"
    fi

    i=$((i + 1))
  done <<< "${plots_by_dir[$dir]}"
done
