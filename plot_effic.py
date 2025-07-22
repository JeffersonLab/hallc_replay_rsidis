import sys
import matplotlib.pyplot as plt

def extract_fid_tracking_efficiencies(filepath):
    results = {"SHMS": [], "HMS": []}
    current_spec = None
    fid_count = 0

    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            if "* SHMS TRACKING EFFICIENCIES *" in line:
                current_spec = "SHMS"
                fid_count = 0
                continue
            elif "*HMS TRACKING EFFICIENCIES *" in line:
                current_spec = "HMS"
                fid_count = 0
                continue

            if "FID TRACK EFFIC" in line and current_spec:
                try:
                    label, value = line.split(":", 1)
                    val, err = value.strip().split("+-")
                    results[current_spec].append((label.strip(), float(val.strip()), float(err.strip())))
                    fid_count += 1
                    if fid_count == 3:
                        current_spec = None
                except ValueError:
                    continue
    return results

if len(sys.argv) != 2:
    print("Usage: python3 plot_effic.py <report_file>")
    sys.exit(1)

report_file = sys.argv[1]
data = extract_fid_tracking_efficiencies(report_file)

fig, ax = plt.subplots(figsize=(8, 5))

metrics = ["SING", "E SING", "HADRON SING"]
x = range(len(metrics))
width = 0.35

hms_vals = [v[1] for v in data["HMS"]] if data.get("HMS") else [0, 0, 0]
hms_errs = [v[2] for v in data["HMS"]] if data.get("HMS") else [0, 0, 0]
shms_vals = [v[1] for v in data["SHMS"]] if data.get("SHMS") else [0, 0, 0]
shms_errs = [v[2] for v in data["SHMS"]] if data.get("SHMS") else [0, 0, 0]

ax.bar([i - width/2 for i in x], shms_vals, width, yerr=shms_errs, label='SHMS', capsize=5, color='tab:blue')
ax.bar([i + width/2 for i in x], hms_vals, width, yerr=hms_errs, label='HMS', capsize=5, color='tab:green')

ax.set_ylabel("Tracking Efficiency")
ax.set_title("Tracking Efficiencies")
ax.set_xticks(x)
ax.set_xticklabels(metrics)
ax.set_ylim(0.7, 1.01)
ax.legend()
ax.grid(True, linestyle='--', alpha=0.5)

for i in range(len(metrics)):
    shms_y = shms_vals[i] + shms_errs[i] + 0.001
    hms_y = hms_vals[i] + hms_errs[i] + 0.001
    plt.text(x[i] - width/2, shms_y, f"{shms_vals[i]:.4f}±{shms_errs[i]:.4f}", ha='center', va='bottom', fontsize=8)
    plt.text(x[i] + width/2, hms_y, f"{hms_vals[i]:.4f}±{hms_errs[i]:.4f}", ha='center', va='bottom', fontsize=8)

plt.tight_layout()
plt.show()
