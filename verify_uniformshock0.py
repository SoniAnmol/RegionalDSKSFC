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
def compare_metric(df_a, df_b, metric, label_a="DSK", label_b="R-DSK",
                   t_min=T_MIN, alpha=ALPHA, region="macro"):
    if "region" in df_a.columns and df_a["region"].nunique() > 1:
        df_a = df_a[df_a["region"] == region]
    if "region" in df_b.columns and df_b["region"].nunique() > 1:
        df_b = df_b[df_b["region"] == region]

    a = df_a.loc[df_a["t"] > t_min, ["run", metric]].groupby("run")[metric].mean().dropna()
    b = df_b.loc[df_b["t"] > t_min, ["run", metric]].groupby("run")[metric].mean().dropna()

    t_stat, p_value = stats.ttest_ind(a, b, equal_var=True)
    passed = p_value >= alpha
    tag = "PASS" if passed else "FAIL"
    print(f"  [{tag}] {metric:25s}  p={p_value:.4e}  "
          f"(DSK: {a.mean():.4f}±{a.std():.4f}, R-DSK: {b.mean():.4f}±{b.std():.4f})")
    return passed


def compare_crisis(df_a, df_b, label_a="DSK", label_b="R-DSK",
                   t_min=T_MIN, threshold=CRISIS_THRESHOLD, alpha=ALPHA):
    def _run_level(df):
        if "region" in df.columns and df["region"].nunique() > 1:
            df = df[df["region"] == "macro"]
        work = df.loc[df["t"] > t_min, ["run", "GDP_r_growth"]].copy()
        valid = work["GDP_r_growth"].notna()
        work["_crisis"] = np.where(valid, (work["GDP_r_growth"] < threshold).astype(int), np.nan)
        obs = work[valid].groupby("run")["_crisis"].count()
        crises = work[valid].groupby("run")["_crisis"].sum()
        return (crises / obs.replace(0, np.nan)).astype(float).dropna()

    a = _run_level(df_a)
    b = _run_level(df_b)
    t_stat, p_value = stats.ttest_ind(a, b, equal_var=True)
    passed = p_value >= alpha
    tag = "PASS" if passed else "FAIL"
    print(f"  [{tag}] {'crisis_likelihood':25s}  p={p_value:.4e}  "
          f"(DSK: {a.mean():.4f}±{a.std():.4f}, R-DSK: {b.mean():.4f}±{b.std():.4f})")
    return passed


# ── Run comparisons ─────────────────────────────────────────────────────
metrics = ["Unemployment", "GDP_r_volatility", "GDP_r_growth"]
all_pass = True
total = 0
failures = 0

for s_num in sorted(dsk_scenarios.keys()):
    if s_num not in rdsk_scenarios:
        print(f"\nScenario {s_num}: SKIPPED (not found in R-DSK output)")
        continue
    print(f"\n{'='*60}")
    print(f" Scenario {s_num}")
    print(f"{'='*60}")
    df_dsk = dsk_scenarios[s_num]
    df_rdsk = rdsk_scenarios[s_num]

    for m in metrics:
        total += 1
        if not compare_metric(df_dsk, df_rdsk, m):
            failures += 1
            all_pass = False

    # crisis likelihood
    total += 1
    if not compare_crisis(df_dsk, df_rdsk):
        failures += 1
        all_pass = False

print(f"\n{'='*60}")
print(f" SUMMARY:  {total - failures}/{total} passed   ({failures} failures)")
if all_pass:
    print(" ALL TESTS PASSED")
else:
    print(f" {failures} TESTS FAILED")
print(f"{'='*60}")
