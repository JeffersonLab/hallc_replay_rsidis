import csv
import os
import re

def hms_dir(run_number):
    return f"/net/cdaq/cdaql3data/cdaq/hallc-online-rsidis2025/REPORT_OUTPUT/HMS/PRODUCTION/replay_hms_coin_production_{run_number}_-1.report"

def shms_dir(run_number):
    return f"/net/cdaq/cdaql3data/cdaq/hallc-online-rsidis2025/REPORT_OUTPUT/SHMS/PRODUCTION/replay_shms_coin_production_{run_number}_-1.report"

def coin_dir(run_number):
    return f"/net/cdaq/cdaql3data/cdaq/hallc-online-rsidis2025/REPORT_OUTPUT/COIN/PRODUCTION/replay_coin_production_{run_number}_-1.report"

def find_variable(line_number, pattern):
    return line_number, pattern

# Mapping: variable -> (line_index, char_start, char_end)
HMS_MAP = {
    "BCM1_Q": find_variable(46,"BCM1  Beam Cut Charge: "),
    "BCM1_I": find_variable(39,"BCM1 Beam Cut Current: "),
    "BCM2_Q": find_variable(47,"BCM2  Beam Cut Charge: "),
    "BCM2_I": find_variable(40,"BCM2 Beam Cut Current: "),
    "BCM4A_Q": find_variable(48,"BCM4A Beam Cut Charge: "),
    "BCM4A_I": find_variable(41,"BCM4A Beam Cut Current: "),
    "BCM4B_Q": find_variable(49,"BCM4B Beam Cut Charge: "),
    "BCM4B_I": find_variable(42,"BCM4B Beam Cut Current: "),
    "BCM4C_Q": find_variable(50,"BCM4C Beam Cut Charge: "),
    "BCM4C_I": find_variable(43,"BCM4C Beam Cut Current: "),
    "h_esing_Eff": find_variable(351,"E SING FID TRACK EFFIC         :"),
    "h_hadron_Eff": find_variable(352,"HADRON SING FID TRACK EFFIC    :"),
    "ps1" : find_variable(63,"Ps1_factor ="),
    "ps2" : find_variable(64,"Ps2_factor ="),
    "ps3" : find_variable(65,"Ps3_factor ="),
    "ps4" : find_variable(66,"Ps4_factor ="),
    "ps5" : find_variable(67,"Ps5_factor ="),
    "ps6" : find_variable(68,"Ps6_factor ="),
    "pTRIG3" : find_variable(126,"pTRIG3 :"),
    "pTRIG4" : find_variable(127,"pTRIG4 :"),
    "phys_triggers": find_variable(91,"Physics Triggers (current cut) :"),
    "hEL_REAL": find_variable(101,"hEL_REAL  :"),
    "pEL_REAL:": find_variable(120, "pEL_REAL  :"),
    "electr_deadtime": find_variable(175,"OG 6 GeV Electronic Live Time (100, 150) :"),
    "h_EL_CLEAN": find_variable(102,"hEL_CLEAN :"),
    "p_EL_CLEAN": find_variable(121,"pEL_CLEAN :"),
}

SHMS_MAP = {
    "BCM1_Q": find_variable(46,"BCM1  Beam Cut Charge: "),
    "BCM1_I": find_variable(39,"BCM1 Beam Cut Current: "),
    "BCM2_Q": find_variable(47,"BCM2  Beam Cut Charge: "),
    "BCM2_I": find_variable(40,"BCM2 Beam Cut Current: "),
    "BCM4A_Q": find_variable(48,"BCM4A Beam Cut Charge: "),
    "BCM4A_I": find_variable(41,"BCM4A Beam Cut Current: "),
    "BCM4B_Q": find_variable(49,"BCM4B Beam Cut Charge: "),
    "BCM4B_I": find_variable(42,"BCM4B Beam Cut Current: "),
    "BCM4C_Q": find_variable(50,"BCM4B Beam Cut Charge: "),
    "BCM4C_I": find_variable(43,"BCM4C Beam Cut Charge: "),
    "p_esing_Eff": find_variable(377,"E SING FID TRACK EFFIC         :"),
    "p_hadron_Eff": find_variable(378,"HADRON SING FID TRACK EFFIC    :"),
    "ps1" : find_variable(57,"Ps1_factor ="),
    "ps2" : find_variable(58,"Ps2_factor ="),
    "ps3" : find_variable(59,"Ps3_factor ="),
    "ps4" : find_variable(60,"Ps4_factor ="),
    "ps5" : find_variable(61,"Ps5_factor ="),
    "ps6" : find_variable(62,"Ps6_factor ="),
    "pTRIG1" : find_variable(116,"pTRIG1 :"),
    "pTRIG2" : find_variable(117,"pTRIG4 :"),
    "phys_triggers": find_variable(85,"Physics Triggers (current cut) :"),
    "hEL_REAL": find_variable(112,"hEL_REAL  :"),
    "electr_deadtime": find_variable(167,"OG 6 GeV Electronic Live Time (100, 150) :"),
    "h_EL_CLEAN": find_variable(102,"hEL_CLEAN :"),
    "p_EL_CLEAN": find_variable(121,"pEL_CLEAN :"),
}

COIN_MAP = {
    "BCM1_Q": find_variable(54,"HMS BCM1  Beam Cut Charge:"),
    "BCM1_I": find_variable(47,"HMS BCM1 Beam Cut Current:"),
    "BCM2_Q": find_variable(55,"HMS BCM2  Beam Cut Charge:"),
    "BCM2_I": find_variable(48,"HMS BCM2 Beam Cut Current:"),
    "BCM4A_Q": find_variable(56,"HMS BCM4A Beam Cut Charge:"),
    "BCM4A_I": find_variable(49,"HMS BCM4A Beam Cut Current:"),
    "BCM4B_Q": find_variable(57,"HMS BCM4B Beam Cut Charge:"),
    "BCM4B_I": find_variable(50,"HMS BCM4B Beam Cut Current:"),
    "BCM4C_Q": find_variable(58,"HMS BCM4C Beam Cut Charge:"),
    "BCM4C_I": find_variable(51,"HMS BCM4C Beam Cut Current:"),
    "h_esing_Eff": find_variable(648,"E SING FID TRACK EFFIC         :"),
    "h_hadron_Eff": find_variable(649,"HADRON SING FID TRACK EFFIC    :"),
    "p_esing_Eff": find_variable(507,"E SING FID TRACK EFFIC         :"),
    "p_hadron_Eff": find_variable(508,"HADRON SING FID TRACK EFFIC    :"),
    "ps1" : find_variable(105,"Ps1_factor ="),
    "ps2" : find_variable(106,"Ps2_factor ="),
    "ps3" : find_variable(107,"Ps3_factor ="),
    "ps4" : find_variable(108,"Ps4_factor ="),
    "ps5" : find_variable(109,"Ps5_factor ="),
    "ps6" : find_variable(110,"Ps6_factor ="),
    "phys_triggers": find_variable(146,"HMS Accepted Physics Triggers       :"),
    "hEL_REAL": find_variable(205,"HMS_hEL_REAL  :"),
    "electr_deadtime": find_variable(279,"ROC2 OG 6 GeV Electronic Dead Time (100, 150) (no BCM cut) :"),
    "helicity_C": find_variable(1248,"BCM2  Helicity Gated Charge:"),
    "helicity_A": find_variable(1258,"BCM2  Helicity Gated Charge Asymmetry:"),
    "h_EL_CLEAN": find_variable(206,"HMS_hEL_CLEAN :"),
    "p_EL_CLEAN": find_variable(180,"SHMS_pEL_CLEAN :"),
    "ps5_comp_livetime": find_variable(254,"ROC2 Pre-Scaled Ps5 ROC2 Computer Live Time (no BCM cut) :"),
    "ps6_comp_livetime": find_variable(257,"ROC2 Pre-Scaled Ps6 ROC2 Computer Live Time (no BCM cut) :"),
}

run_type_map = {
    "HMS": HMS_MAP,
    "SHMS": SHMS_MAP,
    "COIN": COIN_MAP,
}

def parse_report_file(report_path, mapping):
    props = {}

    if not os.path.exists(report_path):
        return props

    with open(report_path, "r") as f:
        lines = f.readlines()

    for var, (line_number, pattern) in mapping.items():
        try:
            line = lines[line_number]

            # Make sure we're looking at the expected line
            if pattern not in line:
                raise ValueError(
                    f"Expected '{pattern}' on line {line_number}, "
                    f"but found:\n{line.strip()}"
                )

            # Everything after the identifying pattern
            value_string = line.split(pattern, 1)[1]

            # Extract the first numerical value
            match = re.search(
                r"[-+]?\d*\.?\d+(?:[Ee][-+]?\d+)?",
                value_string
            )

            if match:
                props[var] = float(match.group())
            else:
                raise ValueError(
                    f"No numerical value found after '{pattern}' "
                    f"on line {line_number}"
                )

        except (IndexError, ValueError):
            props[var] = None

    return props


def find_special_report_file(run_number):
    if os.path.exists(coin_dir(run_number)):
        return coin_dir(run_number), run_type_map["COIN"]
    elif os.path.exists(hms_dir(run_number)):
        return hms_dir(run_number), run_type_map["HMS"]
    else:
        return shms_dir(run_number), run_type_map["SHMS"]


def load_extra_info(run_number, run_type, issues=None):
    keep_cols = ["coin", "randoms", "ransubcoin", "normyield", "normyield_err", "ctmean", "ctsigma"]

    hms_run_types = ("HMSDIS", "HEE", "HMSHEE")
    shms_run_types = ("SHMSDIS" "SHMSHEE")

    if run_type in hms_run_types:
        extra_path = f"/net/cdaq/cdaql3data/cdaq/hallc-online-rsidis2025/REPORT_OUTPUT/HMS/PRODUCTION/output_get_good_dis_ev_{run_number}_-1.csv"
        issue_text = "missing get_good_dis_ev file"
    elif run_type in shms_run_types:
        extra_path = f"/net/cdaq/cdaql3data/cdaq/hallc-online-rsidis2025/REPORT_OUTPUT/SHMS/PRODUCTION/output_get_good_dis_ev_{run_number}_-1.csv"
        issue_text = "missing get_good_dis_ev file"
    else:
        extra_path = f"/net/cdaq/cdaql3data/cdaq/hallc-online-rsidis2025/REPORT_OUTPUT/COIN/PRODUCTION/output_get_good_coin_ev_{run_number}_-1.csv"
        issue_text = "missing get_good_coin_ev file"

    if not os.path.exists(extra_path):
        print(f"file not found for run {run_number} ({run_type}): {extra_path}")
        if issues is not None:
            issues.append({
                "run": run_number,
                "run_type": run_type,
                "issue": issue_text
            })
        return {col: -999 for col in keep_cols}

    with open(extra_path, newline="") as f:
        reader = csv.DictReader(f)
        try:
            row = next(reader)
            return {col: float(row[col]) for col in keep_cols}
        except (StopIteration, KeyError, ValueError):
            return {col: -999 for col in keep_cols}


def load_fan_data(run_number, fan_csv_path):
    
#    Reads fan_freq.csv and returns mean and stdev for the given run_number.

    import csv
    if not os.path.exists(fan_csv_path):
        return {"fan_mean": -999, "fan_stdev": -999}

    with open(fan_csv_path, newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            if str(row["run"]) == str(run_number):
                try:
                    return {
                        "fan_mean": float(row.get("mean", -999)),
                        "fan_stdev": float(row.get("stdev", -999))
                    }
                except ValueError:
                    return {"fan_mean": -999, "fan_stdev": -999}

    # If run not found
    return {"fan_mean": -999, "fan_stdev": -999}

def load_ihwp_table(ihwp_csv_path):
    ihwp_map = {}
    if not os.path.exists(ihwp_csv_path):
        print(f"⚠️ IHWP file not found: {ihwp_csv_path}")
        return ihwp_map

    with open(ihwp_csv_path, newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            run = row.get("run_number")
            if run:
                ihwp_map[str(run)] = {
                    "IHWP": row.get("IHWP", ""),
                    "start_time": row.get("start_time", ""),
                    "stop_time": row.get("stop_time", "")
                }
    return ihwp_map

def load_coin_block_ratios(coin_block_ratios_csv_path):
    coin_block_ratios_map = {}
    if not os.path.exists(coin_block_ratios_csv_path):
        print(f"⚠️ Coin block ratios file not found: {coin_block_ratios_csv_path}")
        return coin_block_ratios_map

    with open(coin_block_ratios_csv_path, newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            run = row.get("run")
            if run:
                coin_block_ratios_map[str(run)] = {
                    "coinblock_ratio": row.get("ratio", "")
                }
    return coin_block_ratios_map


# === New Kinematic Conversion Table ===
KINEMATIC_TABLE = [
    {"ebeam": 8.5831, "x": 0.25, "Q2": 3.3, "z": 0.9, "thpq": 2.0,   "hms_p": 1.531, "hms_th": 29.045, "shms_p": 6.538, "shms_th": 7.865},
    {"ebeam": 8.5831, "x": 0.25, "Q2": 3.3, "z": 0.67, "thpq": 2.0,   "hms_p": 1.531, "hms_th": 29.045, "shms_p": 4.868, "shms_th": 7.865},
    {"ebeam": 8.5831, "x": 0.25, "Q2": 3.3, "z": 0.67, "thpq": 5.2, "hms_p": 1.531, "hms_th": 29.045, "shms_p": 4.868, "shms_th": 11.075},
    {"ebeam": 8.5831, "x": 0.25, "Q2": 3.3, "z": 0.67, "thpq": 8.5, "hms_p": 1.531, "hms_th": 29.045, "shms_p": 4.868, "shms_th": 14.375},
    {"ebeam": 8.5831, "x": 0.25, "Q2": 3.3, "z": 0.5,  "thpq": 2.0,   "hms_p": 1.531, "hms_th": 29.045, "shms_p": 3.632, "shms_th": 7.865},
    {"ebeam": 8.5831, "x": 0.25, "Q2": 3.3, "z": 0.5,  "thpq": 5.2, "hms_p": 1.531, "hms_th": 29.045, "shms_p": 3.632, "shms_th": 11.075},
    {"ebeam": 8.5831, "x": 0.25, "Q2": 3.3, "z": 0.5,  "thpq": 8.5, "hms_p": 1.531, "hms_th": 29.045, "shms_p": 3.632, "shms_th": 14.375},
    {"ebeam": 8.5831, "x": 0.25, "Q2": 3.3, "z": 0.36, "thpq": 2.0,   "hms_p": 1.531, "hms_th": 29.045, "shms_p": 2.615, "shms_th": 7.865},
    {"ebeam": 10.6716, "x": 0.25, "Q2": 3.3, "z": 0.9,  "thpq": -0.8, "hms_p": 3.642, "hms_th": 16.75, "shms_p": 6.538, "shms_th": 7.51},
    {"ebeam": 10.6716, "x": 0.25, "Q2": 3.3, "z": 0.9,  "thpq": 2.0,    "hms_p": 3.642, "hms_th": 16.75, "shms_p": 6.538, "shms_th": 10.305},
    {"ebeam": 10.6716, "x": 0.25, "Q2": 3.3, "z": 0.67, "thpq": 2.0,    "hms_p": 3.642, "hms_th": 16.75, "shms_p": 4.868, "shms_th": 10.305},
    {"ebeam": 10.6716, "x": 0.25, "Q2": 3.3, "z": 0.67, "thpq": -0.8, "hms_p": 3.642, "hms_th": 16.75, "shms_p": 4.868, "shms_th": 7.51},
    {"ebeam": 10.6716, "x": 0.25, "Q2": 3.3, "z": 0.5,  "thpq": -0.8, "hms_p": 3.642, "hms_th": 16.75, "shms_p": 3.632, "shms_th": 7.51},
    {"ebeam": 10.6716, "x": 0.25, "Q2": 3.3, "z": 0.5,  "thpq": 2.0,    "hms_p": 3.642, "hms_th": 16.75, "shms_p": 3.632, "shms_th": 10.305},
    {"ebeam": 10.6716, "x": 0.25, "Q2": 3.3, "z": 0.5,  "thpq": 5.2,  "hms_p": 3.642, "hms_th": 16.75, "shms_p": 3.632, "shms_th": 13.505},
    {"ebeam": 10.6716, "x": 0.25, "Q2": 3.3, "z": 0.5,  "thpq": 8.5,  "hms_p": 3.642, "hms_th": 16.75, "shms_p": 3.632, "shms_th": 16.81},
    {"ebeam": 10.6716, "x": 0.25, "Q2": 3.3, "z": 0.36, "thpq": 2.0,    "hms_p": 3.642, "hms_th": 16.75, "shms_p": 2.615, "shms_th": 10.305},
    {"ebeam": 10.6716, "x": 0.25, "Q2": 3.3, "z": 0.36, "thpq": -0.2,    "hms_p": 3.642, "hms_th": 16.75, "shms_p": 3.632, "shms_th": 8.11},
    {"ebeam": 6.449, "x": 0.22, "Q2": 2.2, "z": 0.5, "thpq": 2.0,    "hms_p": 1.165, "hms_th": 31.278, "shms_p": 2.766, "shms_th": 8.275},
    {"ebeam": 6.449, "x": 0.22, "Q2": 2.2, "z": 0.9, "thpq": 2.0,    "hms_p": 1.165, "hms_th": 31.278, "shms_p": 4.978, "shms_th": 8.275},
    {"ebeam": 6.449, "x": 0.44, "Q2": 4.4, "z": 0.9, "thpq": 2.0,    "hms_p": 1.165, "hms_th": 44.830, "shms_p": 5.154, "shms_th": 10.240},
    {"ebeam": 6.449, "x": 0.44, "Q2": 4.4, "z": 0.67, "thpq": 2.0,   "hms_p": 1.165, "hms_th": 44.830, "shms_p": 3.837, "shms_th": 10.240},
    {"ebeam": 6.449, "x": 0.44, "Q2": 4.4, "z": 0.52, "thpq": 2.0,   "hms_p": 1.165, "hms_th": 44.830, "shms_p": 2.978, "shms_th": 10.240},
    {"ebeam": 6.449, "x": 0.44, "Q2": 4.4, "z": 0.67, "thpq": 2.0,   "hms_p": 1.165, "hms_th": 44.830, "shms_p": 3.837, "shms_th": 10.240},
    {"ebeam": 6.449, "x": 0.44, "Q2": 4.4, "z": 0.52, "thpq": 2.0,   "hms_p": 1.165, "hms_th": 44.830, "shms_p": 2.978, "shms_th": 10.240},
    {"ebeam": 10.6716, "x": 0.44, "Q2": 4.4, "z": 0.52, "thpq": -2.0,   "hms_p": 5.343, "hms_th": 15.97, "shms_p": 2.978, "shms_th": 12.87},
    {"ebeam": 10.6716, "x": 0.44, "Q2": 4.4, "z": 0.52, "thpq": 0.0,   "hms_p": 5.343, "hms_th": 15.97, "shms_p": 2.978, "shms_th": 14.87},
    {"ebeam": 10.6716, "x": 0.44, "Q2": 4.4, "z": 0.52, "thpq": 2.0,   "hms_p": 5.343, "hms_th": 15.97, "shms_p": 2.978, "shms_th": 16.87}
]


def find_kinematics(ebeam, hms_p, hms_th, shms_p, shms_th, tol=0.01):
    
    # Returns matching (x, Q2, z, thpq) for the given kinematic settings.
    
    for row in KINEMATIC_TABLE:
        if (
            abs(row["ebeam"] - abs(float(ebeam))) < tol and
            abs(row["hms_p"] - abs(float(hms_p))) < tol and
            abs(row["hms_th"] - abs(float(hms_th))) < tol and
            abs(row["shms_p"] - abs(float(shms_p))) < tol and
            abs(row["shms_th"] - abs(float(shms_th))) < tol
        ):
            return {
                "x": row["x"],
                "Q2": row["Q2"],
                "z": row["z"],
                "thpq": row["thpq"]
            }
    return {"x": -999, "Q2": -999, "z": -999, "thpq": -999}

def compute_corr_coeff(f, I):
    # Fit coefficients:
    alpha2, alpha1, alpha0 = -4.77805843e-06, 1.47503555e-04,-3.17321158e-04
    beta2, beta1, beta0 = 4.53147451e-04, -1.26593244e-02, 2.89365318e-02
    gamma2, gamma1, gamma0 = -1.05667262e-02, 2.17611407e-01, 1.41933284e+02

    if any(v in (-999, None) for v in [f, I]):
        return -999

    try:
        Y_fI = ((alpha2*I**2 + alpha1*I + alpha0)*f**2 +
                (beta2*I**2 + beta1*I + beta0)*f +
                (gamma2*I**2 + gamma1*I + gamma0))
        Y_f0 = (alpha0*f**2 + beta0*f + gamma0)
        if Y_fI == 0:
            return -999
        return round(Y_f0 / Y_fI, 6)
    except Exception:
        return -999

# Helicity based charge:

def helicity_charge_hp(C,A):
    if any(v in (-999,None) for v in [C,A]):
        return -999
    else:
        BCM2_Q_hp = (C/2)*(1+A)
    return round(BCM2_Q_hp,5)

def helicity_charge_hm(C,A):
    if any(v in (-999,None) for v in [C,A]):
        return -999
    else:
        BCM2_Q_hm = (C/2)*(1-A)
    return round(BCM2_Q_hm,5)



def collect_run_info(input_csv, output_csv, run_type_map):
    keep_columns = ["run", "ebeam", "target", "hms_p", "hms_th", "shms_p", "shms_th", "run_type"] 
    results = []
    issues = []

    # ihwp_map = load_ihwp_table("updated_merged_run_start_stop_log_100625.csv")
    coin_block_ratios_map = load_coin_block_ratios("/home/cdaq/users/jgilguti/coin_block/coin_block_ratios.csv")

    with open(input_csv, newline="") as f:
        reader = csv.DictReader(f)

        for row in reader:
            run_number = row["run"]
            run_type = row["run_type"]

            # Figure out report path depending on run type
            if run_type in ("PI-SIDIS", "PI+SIDIS", "HOLE", "HEEP"):
                report_path = coin_dir(run_number)
                mapping = run_type_map["COIN"]
            elif run_type in ("HMSDIS", "HEE", "HMSHEE"):
                report_path = hms_dir(run_number)
                mapping = run_type_map["HMS"]
            elif run_type in ("SHMSDIS", "SHMSHEE"):
                report_path = shms_dir(run_number)
                mapping = run_type_map["SHMS"]
            else:
                report_path, mapping = find_special_report_file(run_number)

            # Extract variables
            props = {}
            if report_path and os.path.exists(report_path):
                props = parse_report_file(report_path, mapping)

                if mapping is run_type_map["COIN"]:
                    ps1, ps2, ps3, ps4, ps5, ps6 = props.get("ps1"), props.get("ps2"), props.get("ps3"), props.get("ps4"), props.get("ps5"), props.get("ps6")
                    ps5_comp_livetime = props.get("ps5_comp_livetime")
                    ps6_comp_livetime = props.get("ps6_comp_livetime")

                    props["comp_livetime"] = -999

                    if ps5 not in (None, -999) and ps5 > 0:
                        if ps5_comp_livetime not in (None, -999):
                            props["comp_livetime"] = round(ps5_comp_livetime / 100, 5)

                    elif ps6 not in (None, -999) and ps6 > 0:
                        if ps6_comp_livetime not in (None, -999):
                            props["comp_livetime"] = round(ps6_comp_livetime / 100, 5)
                    
#                    props["comp_livetime"] = 1.0
# Uncomment when find out line number for helicity_A and helicity_C:
                    props["BCM2_Q_hp"] = helicity_charge_hp(props["helicity_C"], props["helicity_A"])
                    props["BCM2_Q_hm"] = helicity_charge_hm(props["helicity_C"], props["helicity_A"])
                    
                else:
                    phys_triggers = props.get("phys_triggers")
                    ps1, ps2, ps3, ps4, ps5, ps6 = props.get("ps1"), props.get("ps2"), props.get("ps3"), props.get("ps4"), props.get("ps5"), props.get("ps6")
                    ps_values = [props.get(f"ps{i}", 1) for i in range(1, 7)]
                    pTRIG1 = props.get("pTRIG1")
                    pTRIG2 = props.get("pTRIG2")
                    pTRIG3 = props.get("pTRIG3")
                    pTRIG4 = props.get("pTRIG4")

                    props["comp_livetime"] = -999

                    if phys_triggers not in (None, -999):

                        ps_product = 1
                        for ps in ps_values:
                            if ps in (None,-999):
                                ps = 1
                            ps_product *= ps

                        # Determine livetime based on spectrometer type                          
                        if mapping is run_type_map["HMS"]:
                            if pTRIG3 and ps3 > 0:
                                props["comp_livetime"] = round((-1 * ps_product * phys_triggers) / pTRIG3, 5)
                            elif pTRIG4 and ps4 > 0:
                                props["comp_livetime"] = round((-1 * ps_product * phys_triggers) / pTRIG4, 5)

                        elif mapping is run_type_map["SHMS"]:
                            if pTRIG1 and ps1 > 0:
                                props["comp_livetime"] = round((-1 * ps_product * phys_triggers) / pTRIG1, 5)
                            elif pTRIG2 and ps2 > 0:
                                props["comp_livetime"] = round((-1 * ps_product * phys_triggers) / pTRIG2, 5)

                        if props["comp_livetime"] > 1:
                            props["comp_livetime"] = 1.0


                if mapping is run_type_map["HMS"]:
                    props["pEff"] = -999

                if mapping is run_type_map["SHMS"]:
                    props["hEff"] = -999

            else:
                if report_path:  # file path expected but missing
                    print(f"⚠️ Report file not found: {report_path}")
                    issues.append({
                        "run": run_number,
                        "run_type": run_type,
                        "issue": "missing report file"
                    })
                props = {var: -999 for var in mapping.keys()}

            # Include helicity based charge information:
#            props["BCM2_Q_hp"], props["BCM2_Q_hm"] = helicity_charge(props["helicity_C"],props["helicity_A"])

            # Load extra info from output_get_good_coin_ev
            extra_props = load_extra_info(run_number, run_type, issues)
            props.update(extra_props)

            # Load fan speed table
            fan_props = load_fan_data(run_number, "fan_freq_pass0.csv") 
            props.update(fan_props)

            # Merge input row with extracted props
            merged = {col: row[col] for col in keep_columns if col in row}
            merged.update(props)

            # Include IHWP value
            #merged["IHWP"] = ihwp_map.get(str(run_number), "")

            # ihwp_info = ihwp_map.get(str(run_number),{})
            # merged["IHWP"]=ihwp_info.get("IHWP",-999)
            # merged["start_time"] = ihwp_info.get("start_time", -999)
            # merged["stop_time"] = ihwp_info.get("stop_time", -999)

            merged["IHWP"]= -999
            merged["start_time"] = -999
            merged["stop_time"] = -999

            coin_block_ratio_info = coin_block_ratios_map.get(str(run_number), {})
            merged["coinblock_ratio"] = coin_block_ratio_info.get("coinblock_ratio", -999)

            kin = find_kinematics(
                float(row["ebeam"]),
                float(row["hms_p"]),
                float(row["hms_th"]),
                float(row["shms_p"]),
                float(row["shms_th"]),
            )
            merged.update(kin)

            # Include fan speed and boiling corrections for LH2 and only boiling correction for LD2
            f = merged.get("fan_mean", -999)
            I = merged.get("BCM2_I", -999)
            target = merged.get("target", "")
            
            # if target == "LH2":
            #    merged["boil_corr"] = compute_corr_coeff(f,I)
            # elif target == "LD2":
            #     merged["boil_corr"] = round(1 + 0.03493 * (I / 100), 6)
            # else:
            merged["boil_corr"] = 1.0
   
            results.append(merged)

            

    # Write results to CSV
    fieldnames = keep_columns + ["x","Q2","z","thpq","BCM1_Q","BCM1_I","BCM2_Q","BCM2_I","BCM4A_Q","BCM4A_I","BCM4B_Q","BCM4B_I","BCM4C_Q","BCM4C_I","h_esing_Eff","h_hadron_Eff","p_esing_Eff","p_hadron_Eff","ps1","ps2","ps3","ps4","ps5","ps6","comp_livetime","electr_deadtime",
#get_good_coin_events variables:
"coin", "randoms", "ransubcoin", "normyield", "normyield_err", "ctmean","ctsigma",
#fan speed variables:
# "fan_mean", "fan_stdev",
"boil_corr",
#start and stop times
#"start_time", "stop_time",
"IHWP",
#coin block ratio
"coinblock_ratio",
#helicity based charge                                 
"BCM2_Q_hp", "BCM2_Q_hm",
"h_EL_CLEAN", "p_EL_CLEAN"]

    for row in results:
        for key in fieldnames:
            if key not in row or row[key] in ("", None):
                row[key] = -999

                    
    with open(output_csv, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(results)

    issue_csv = output_csv.replace(".csv", "_missing_files.csv")

    with open(issue_csv, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=["run", "run_type", "issue"])
        writer.writeheader()
        writer.writerows(issues)





# ========= MAIN =========
if __name__ == "__main__":
    collect_run_info("../parse_runlist/parsed_runlist_phase2.csv", "../../rsidis_bigtable_phaseII.csv", run_type_map)
