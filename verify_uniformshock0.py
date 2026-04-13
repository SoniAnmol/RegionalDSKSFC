#!/usr/bin/env python3
"""Quick verification: compare R-DSK (flag_uniformshocks=0) vs DSK reference.

Uses the same compare_scenarios / compare_crisis_likelihood logic from the
verification_reg_gov notebook.
"""
import sys
from pathlib import Path
import numpy as np
from scipy import stats

# Make pysim-lab scripts importable
pysim_root = Path("/Users/anmolsoni/Documents/pysim-lab")
if str(pysim_root) not in sys.path:
    sys.path.insert(0, str(pysim_root))

from scripts.read_batch_runs import load_simulation_results_matrix
from scripts.compute_macros import compute_GDP_r_volatility, compute_gdp_r_growth

# ── Paths ───────────────────────────────────────────────────────────────
RDSK_PATH = Path("/Users/anmolsoni/Nextcloud/projects/Regionalised-SFC-DSK/verifications/RegGovt_uniformshock0")
DSK_PATH  = Path("/Users/anmolsoni/Nextcloud/projects/Regionalised-SFC-DSK/verifications/dsk_sfc")

ALPHA = 0.05
T_MIN = 200
CRISIS_THRESHOLD = -5.0

# ── Load data ───────────────────────────────────────────────────────────
print("Loading R-DSK results …")
rdsk_matrix = load_simulation_results_matrix(RDSK_PATH)
print("Loading DSK results …")
dsk_matrix = load_simulation_results_matrix(DSK_PATH)

# Split by scenario
def split_scenarios(df):
    return {s: df[df.scenario == s] for s in sorted(df.scenario.unique())}

rdsk_scenarios = split_scenarios(rdsk_matrix)
dsk_scenarios  = split_scenarios(dsk_matrix)

# ── Compute derived columns ─────────────────────────────────────────────
for label, scenarios in [("R-DSK", rdsk_scenarios), ("DSK", dsk_scenarios)]:
    for s, df in scenarios.items():
        df = df.copy()
        df["Unemployment"] = (1 - df["EmploymentRate"]) * 100
        df["GDP_r_volatility"] = compute_GDP_r_volatility(df)
        df["GDP_r_growth"] = compute_gdp_r_growth(df)
        scenarios[s] = df

# ── Comparison functions ─────────────────────────────────────────────────
def _filter_macro(df, region="macro"):
    if "region" in df.columns and df["region"].nunique() > 1:
        return df[df["region"] == region]
    return df


def _run_level_means(df, metric, t_min=T_MIN):
    return df.loc[df["t"] > t_min, ["run", metric]].groupby("run")[metric].mean().dropna()


def _run_level_crisis(df, t_min=T_MIN, threshold=CRISIS_THRESHOLD):
    work = df.loc[df["t"] > t_min, ["run", "GDP_r_growth"]].copy()
    valid = work["GDP_r_growth"].notna()
    work["_crisis"] = np.where(valid, (work["GDP_r_growth"] < threshold).astype(int), np.nan)
    obs = work[valid].groupby("run")["_crisis"].count()
    crises = work[valid].groupby("run")["_crisis"].sum()
    return (crises / obs.replace(0, np.nan)).astype(float).dropna()


def _sig_star(p, alpha=ALPHA):
    if p < 0.001:
        return "***"
    elif p < 0.01:
        return "**"
    elif p < alpha:
        return "*"
    return ""


def compute_comparison(df_dsk, df_rdsk, metric, is_crisis=False):
    df_dsk = _filter_macro(df_dsk)
    df_rdsk = _filter_macro(df_rdsk)
    if is_crisis:
        a = _run_level_crisis(df_dsk)
        b = _run_level_crisis(df_rdsk)
    else:
        a = _run_level_means(df_dsk, metric)
        b = _run_level_means(df_rdsk, metric)
    t_stat, p_value = stats.ttest_ind(a, b, equal_var=True)
    ratio = b.mean() / a.mean() if a.mean() != 0 else np.nan
    return ratio, t_stat, p_value


# ── Run comparisons ─────────────────────────────────────────────────────
metrics = ["Unemployment", "GDP_r_volatility", "GDP_r_growth", "crisis_likelihood"]
scenario_nums = sorted(s for s in dsk_scenarios if s in rdsk_scenarios)

# Collect results: results[metric][scenario] = (ratio, t_stat, p_value)
results = {m: {} for m in metrics}
for s_num in scenario_nums:
    df_dsk = dsk_scenarios[s_num]
    df_rdsk = rdsk_scenarios[s_num]
    for m in metrics:
        results[m][s_num] = compute_comparison(
            df_dsk, df_rdsk, m, is_crisis=(m == "crisis_likelihood")
        )

# ── Print table ─────────────────────────────────────────────────────────
scenario_labels = {1: "Baseline", 2: "Cap. shocks", 3: "Prod. shocks"}
col_width = 22

header_parts = [f"{'Metric':>25s}"]
for s_num in scenario_nums:
    label = scenario_labels.get(s_num, f"Scenario {s_num}")
    header_parts.append(f"{label:^{col_width}s}")
sep = "  ".join(["-" * 25] + ["-" * col_width] * len(scenario_nums))

print()
print("  ".join(header_parts))
print(sep)

for m in metrics:
    row_parts = [f"{m:>25s}"]
    for s_num in scenario_nums:
        ratio, t_stat, p_val = results[m][s_num]
        star = _sig_star(p_val)
        cell = f"{ratio:.4f} (t={t_stat:+.2f}){star}"
        row_parts.append(f"{cell:^{col_width}s}")
    print("  ".join(row_parts))

print(sep)
print(f"\nRatio = R-DSK / DSK  (run-level means, t > {T_MIN})")
print(f"Significance: * p<0.05, ** p<0.01, *** p<0.001")
