import pandas as pd
import subprocess
import json
from pathlib import Path
from collections import Counter
import numpy as np

def read_run_csv(csv_path):
    """Read CSV with run, start_time, stop_time columns."""
    df = pd.read_csv(csv_path)
    return df[["run_number", "start_time", "stop_time"]]

def run_myData(start_time, stop_time, variables):
    """
    Run myData command for given time range and variables.
    Returns stdout as string.
    """
    cmd = ["myData", "-b", start_time, "-e", stop_time] + variables
    result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    
    if result.returncode != 0:
        raise RuntimeError(f"Command failed: {result.stderr}")
    
    return result.stdout

def parse_myData_output(output):
    """
    Parse myData output into a pandas DataFrame.
    Replaces <undefined> with NaN.
    """
    from io import StringIO
    df = pd.read_csv(StringIO(output), sep=r"\s+")
    df.replace("<undefined>", pd.NA, inplace=True)
    return df

def compute_stats(series):
    series = series.replace("<undefined>", pd.NA)
    series = pd.to_numeric(series, errors="coerce")
    num_undefined = int(series.isna().sum())

    if num_undefined == len(series):
        return {
            "average": -999,
            "current size": int(len(series)),
            "first": -999,
            "maximum": -999,
            "minimum": -999,
            "most_common": -999,
            "num_of_undefined": num_undefined,
            "standard_deviation": -999
        }

    values = series.dropna().tolist()
    max_val = float(max(values))
    min_val = float(min(values))
    avg_val = float((max_val + min_val) / 2)
    first_val = float(values[0])
    most_common_val = float(Counter(values).most_common(1)[0][0])
    std_dev = float(np.std(values))

    return {
        "average": avg_val,
        "current size": int(len(series)),
        "first": first_val,
        "maximum": max_val,
        "minimum": min_val,
        "most_common": most_common_val,
        "num_of_undefined": num_undefined,
        "standard_deviation": std_dev
    }

def process_run(run_number, start_time, stop_time, variables, output_dir, faillog_list):
    """Process a single run and save JSON output."""
    print(f"Processing run {run_number}..")

    output = run_myData(start_time, stop_time, variables)
    df = parse_myData_output(output)
    
    run_json = {"RunNumber": str(run_number)}
    
    for var in variables:
        if var in df.columns:
            stats = compute_stats(df[var])
            run_json[var] = stats
            if stats["average"] == -999:
                faillog_list.append(f"{run_number}, All values undefined for {var}")
        else:
            # Variable missing entirely from output
            stats = {
                "average": -999,
                "current size": 0,
                "first": -999,
                "maximum": -999,
                "minimum": -999,
                "most_common": -999,
                "num_of_undefined": 0,
                "standard_deviation": -999
            }
            run_json[var] = stats
            faillog_list.append(f"{run_number}, Variable {var} missing in output")

    output_path = Path(output_dir) / f"{run_number}.json"
    with open(output_path, "w") as f:
        json.dump(run_json, f, indent=4)
    print(f"✅ Saved {output_path}")

def main(run_csv, variables, output_dir, faillog_path="read_epics_archiver.faillog", min_run=None, max_run=None):  # ADDED
    df_runs = read_run_csv(run_csv)

    # ADDED: Filter run range if provided
    if min_run is not None:
        df_runs = df_runs[df_runs["run_number"] >= min_run]

    if max_run is not None:
        df_runs = df_runs[df_runs["run_number"] <= max_run]

    Path(output_dir).mkdir(parents=True, exist_ok=True)
    
    faillog_list = []

    for _, row in df_runs.iterrows():
        process_run(row["run_number"], row["start_time"], row["stop_time"], variables, output_dir, faillog_list)

    # Save fail log
    if faillog_list:
        with open(faillog_path, "w") as f:
            f.write("\n".join(faillog_list) + "\n")
        print(f"⚠️ Fail log saved to {faillog_path}")

if __name__ == "__main__":
    # Example usage
    run_csv = "output/rsidis_phaseII_run_start_stop_table.csv"  # CSV with run_number,start_time,stop_time
    variables = ["ecDI_B_Set_NMR", "ecDI_B_True_NMR", "ecDI_I_coarse", "ecDI_Set_Current",
                 "ecQ1_I_coarse", "ecQ1_Set_Current", "ecQ2_I_coarse", "ecQ2_Set_Current", "ecQ3_I_coarse", "ecQ3_Set_Current", 
                 "ecSDI_I_coarse", "ecSDI_Set_Current", "ecSHB_I_coarse", "ecSHB_Set_Current", "ecSQ1_I_coarse", "ecSQ1_Set_Current",
                 "ecSQ2_I_coarse", "ecSQ2_Set_Current", "ecSQ3_I_coarse", "ecSQ3_Set_Current",
                 "ecHMS_Angle", "ecSHMS_Angle", "hcBDSPOS", "hcL1_Fan_Freq_R", "hcL2_Fan_Freq_R",
                 "IGL1I00OD16_16"]
    #output_dir = "output/read_epics_archiver"
    output_dir = "."

    # ADDED: choose run range here
    main(run_csv, variables, output_dir, min_run=28266, max_run=28267)
