import csv
import re

def parse_ps_values(ps_string):
    ps_values = ps_string.strip().split(',')
    if len(ps_values) != 6:
        raise ValueError(f"Expected 6 PS values, got {len(ps_values)}: {ps_string}")
    return [int(val.strip()) for val in ps_values]

def parse_runlist_file(input_filename, output_filename):
    parsed_rows = []

    with open(input_filename, "r") as infile:
        for line in infile:
            line = line.strip()

            # Skip comment lines
            if line.startswith("#") or line.startswith("!") or not line or set(line.strip()) == {'*'}:
                continue

            # Split using one or more spaces or tabs
            fields = re.split(r'\s+', line, maxsplit=12)

            if len(fields) < 12:
                print(f"Skipping malformed line: {line}")
                continue

            try:
                run_number = int(fields[0])
                date = fields[1]
                start_time = fields[2]
                ebeam = float(fields[3])

                # Allow current to be "max" or a number
                current_raw = fields[4]
                current = current_raw if not current_raw.replace('.', '', 1).isdigit() else float(current_raw)

                target = fields[5]
                hms_p = float(fields[6])
                hms_th = float(fields[7])
                shms_p = float(fields[8])
                shms_th = float(fields[9])
                ps_values = parse_ps_values(fields[10])
                run_type = fields[11]

                # Get comment if it exists
                comment = fields[12].strip() if len(fields) > 12 else ""

                row = {
                    "run": run_number,
                    "date": date,
                    "start_time": start_time,
                    "ebeam": ebeam,
                    "current": current,
                    "target": target,
                    "hms_p": hms_p,
                    "hms_th": hms_th,
                    "shms_p": shms_p,
                    "shms_th": shms_th,
                    "ps1": ps_values[0],
                    "ps2": ps_values[1],
                    "ps3": ps_values[2],
                    "ps4": ps_values[3],
                    "ps5": ps_values[4],
                    "ps6": ps_values[5],
                    "run_type": run_type,
                    "comment": comment
                }

                parsed_rows.append(row)

            except Exception as e:
                print(f"Skipping malformed line: {line}")
                print(f"Reason: {e}")

    # Write to CSV
    with open(output_filename, "w", newline="") as csvfile:
        fieldnames = [
            "run", "date", "start_time", "ebeam", "current", "target", "hms_p", "hms_th",
            "shms_p", "shms_th", "ps1", "ps2", "ps3", "ps4", "ps5", "ps6", "run_type", "comment"
        ]
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()
        for row in parsed_rows:
            writer.writerow(row)

# Run the parser
if __name__ == "__main__":
    #parse_runlist_file("../../rsidis_runlist.dat", "parsed_runlist.csv")
    parse_runlist_file("../../rsidis_runlist_phaseII.dat", "parsed_runlist_phase2.csv")
