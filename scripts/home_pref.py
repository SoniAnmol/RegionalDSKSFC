#%% load libraries

from pathlib import Path
import geopandas as gpd
import sys
from scipy.stats import beta
import requests
from matplotlib.colors import ListedColormap
import importlib
from scipy.stats import beta as beta_dist
import seaborn as sns
import json
import plotly.express as px
import plotly.graph_objects as go
from matplotlib.colors import to_hex
from matplotlib.patches import FancyBboxPatch

# Load reusable reader from scripts/read_batch_runs.py
project_root = Path.cwd()
if not (project_root / "scripts").exists():
    project_root = project_root.parent
if str(project_root) not in sys.path:
    sys.path.append(str(project_root))

import scripts.read_batch_runs as _rbr
importlib.reload(_rbr)

from scripts.read_batch_runs import (
    load_simulation_results_matrix,
    check_regional_consistency,
)
from scripts.visualise_shocks import *
from scripts.compute_macros import *
from scripts.lineplots import *

#%% Set up paths
# Set path for output figures
OUTPUT = Path("/Users/anmolsoni/Nextcloud/projects/Regionalised-SFC-DSK/figures/home_pref/")
OUTPUT.mkdir(parents=True, exist_ok=True)

# define paths to experiment runs.
PATHS = {
    'scenario' : Path("/Users/anmolsoni/Nextcloud/projects/Regionalised-SFC-DSK/home_pref"),
    }

#%% Load simulation results
# load simulation result matrices
sim_matrix = {}
for PATH in PATHS.keys():
    sim_matrix[PATH] = load_simulation_results_matrix(PATHS[PATH])

# Extract simulation results
scenario_results = {}
for sim in sim_matrix.keys():
    for scenario_num in sim_matrix[sim].scenario.unique():
        scenario_results[f"{sim}_{scenario_num}"] = sim_matrix[sim].loc[sim_matrix[sim].scenario == scenario_num]
        
# %%
for scenario in  scenario_results.keys():
    results = scenario_results[scenario]
# %%
# Compare inter-regional trade shares (per region) across scenario1 vs scenario2
import numpy as np
import matplotlib.pyplot as plt

TRADE_VARS = {
    "mach_import_share": "Machines bought from other regions",
    "mach_export_share": "Machines sold to other regions",
    "cons_import_share": "C-goods bought from other regions",
    "cons_export_share": "C-goods sold to other regions",
}

# Use the home_pref matrix (contains all scenarios) and keep regional rows only
_df = sim_matrix["scenario"].copy()
_missing = [c for c in TRADE_VARS if c not in _df.columns]
if _missing:
    raise KeyError(f"Trade-share columns missing from results: {_missing}. Re-run the model with the updated build.")

trade_df = _df[_df["region"] != "macro"].copy()
trade_df["region"] = trade_df["region"].astype(int)
trade_df["scenario"] = trade_df["scenario"].astype(int)

# Mean across Monte Carlo replications for each (scenario, region, t)
trade_mean = (
    trade_df.groupby(["scenario", "region", "t"], as_index=False)[list(TRADE_VARS)]
    .mean()
    .sort_values(["scenario", "region", "t"])
)

regions = sorted(trade_mean["region"].unique())
scenarios = sorted(trade_mean["scenario"].unique())
scen_colors = dict(zip(scenarios, plt.cm.tab10.colors))

# %%
# Time-series grid: rows = trade variables, cols = regions; scenario1 vs scenario2 overlaid
fig, axes = plt.subplots(
    len(TRADE_VARS), len(regions),
    figsize=(4.2 * len(regions), 2.8 * len(TRADE_VARS)),
    sharex=True, sharey="row", squeeze=False,
)
for r_i, (var, var_label) in enumerate(TRADE_VARS.items()):
    for c_i, region in enumerate(regions):
        ax = axes[r_i][c_i]
        for scen in scenarios:
            sub = trade_mean[(trade_mean["scenario"] == scen) & (trade_mean["region"] == region)]
            ax.plot(sub["t"], sub[var], label=f"scenario {scen}", color=scen_colors[scen], lw=1.2)
        if r_i == 0:
            ax.set_title(f"Region {region}")
        if c_i == 0:
            ax.set_ylabel(var, fontsize=9)
        ax.set_ylim(-0.02, 1.02)
        ax.grid(alpha=0.3)
axes[0][0].legend(fontsize=8, loc="upper right")
for ax in axes[-1]:
    ax.set_xlabel("t")
fig.suptitle("Inter-regional trade shares by region: scenario 1 vs scenario 2", y=1.001)
fig.tight_layout()
fig.savefig(OUTPUT / "trade_shares_timeseries.png", dpi=150, bbox_inches="tight")
plt.show()

# %%
# Time-averaged comparison over a stable window (default: last 100 periods)
WINDOW = 100
tmax = trade_mean["t"].max()
window_df = trade_mean[trade_mean["t"] > (tmax - WINDOW)]
trade_summary = (
    window_df.groupby(["scenario", "region"], as_index=False)[list(TRADE_VARS)]
    .mean()
)
print(f"Mean trade shares over last {WINDOW} periods (t > {tmax - WINDOW}):")
print(trade_summary.round(4).to_string(index=False))

# Grouped bars: one panel per variable, x = region, bars = scenarios
fig, axes = plt.subplots(1, len(TRADE_VARS), figsize=(4.5 * len(TRADE_VARS), 3.6), squeeze=False)
x = np.arange(len(regions))
bar_w = 0.8 / max(len(scenarios), 1)
for a_i, (var, var_label) in enumerate(TRADE_VARS.items()):
    ax = axes[0][a_i]
    for s_i, scen in enumerate(scenarios):
        vals = [
            trade_summary.loc[
                (trade_summary["scenario"] == scen) & (trade_summary["region"] == reg), var
            ].mean()
            for reg in regions
        ]
        ax.bar(x + s_i * bar_w, vals, width=bar_w, label=f"scenario {scen}", color=scen_colors[scen])
    ax.set_title(var, fontsize=10)
    ax.set_xticks(x + bar_w * (len(scenarios) - 1) / 2)
    ax.set_xticklabels([f"R{reg}" for reg in regions])
    ax.set_ylim(0, 1.02)
    ax.grid(alpha=0.3, axis="y")
axes[0][0].set_ylabel(f"mean share (last {WINDOW}t)")
axes[0][0].legend(fontsize=8)
fig.suptitle("Inter-regional trade shares by region: scenario 1 vs scenario 2", y=1.02)
fig.tight_layout()
fig.savefig(OUTPUT / "trade_shares_summary_bars.png", dpi=150, bbox_inches="tight")
plt.show()

# %%
# Amount of cross-regional trade, from the bilateral buyer x source columns.
# Each regional row (region = buyer r) carries mach_buy_from_R{s} / cons_buy_from_R{s}
# = nominal value that region r bought from source region s. Off-diagonal (s != r) is cross-regional.
mach_src_cols = {s: f"mach_buy_from_R{s}" for s in regions}
cons_src_cols = {s: f"cons_buy_from_R{s}" for s in regions}
_bil_missing = [c for c in list(mach_src_cols.values()) + list(cons_src_cols.values()) if c not in trade_df.columns]
if _bil_missing:
    raise KeyError(f"Bilateral trade columns missing: {_bil_missing}. Re-run the model with the updated build.")

_bil_cols = list(mach_src_cols.values()) + list(cons_src_cols.values())
bil_mean = (
    trade_df.groupby(["scenario", "region", "t"], as_index=False)[_bil_cols]
    .mean()
    .sort_values(["scenario", "region", "t"])
)


def _cross_sum(row, cols_map):
    # Sum purchases from every source region other than the buyer's own region
    return sum(row[cols_map[s]] for s in cols_map if s != row["region"])


bil_mean["mach_cross"] = bil_mean.apply(lambda r: _cross_sum(r, mach_src_cols), axis=1)
bil_mean["cons_cross"] = bil_mean.apply(lambda r: _cross_sum(r, cons_src_cols), axis=1)

# Total cross-regional trade over time (summed across buyer regions), per scenario
cross_ts = (
    bil_mean.groupby(["scenario", "t"], as_index=False)[["mach_cross", "cons_cross"]].sum()
)

fig, axes = plt.subplots(1, 2, figsize=(11, 3.8), squeeze=False)
for a_i, (col, title) in enumerate(
    [("mach_cross", "Cross-regional machine purchases"),
     ("cons_cross", "Cross-regional C-goods purchases")]
):
    ax = axes[0][a_i]
    for scen in scenarios:
        sub = cross_ts[cross_ts["scenario"] == scen]
        ax.plot(sub["t"], sub[col], label=f"scenario {scen}", color=scen_colors[scen], lw=1.2)
    ax.set_title(title, fontsize=10)
    ax.set_xlabel("t")
    ax.set_ylabel("nominal value")
    ax.grid(alpha=0.3)
axes[0][0].legend(fontsize=8)
fig.suptitle("Total cross-regional trade (national) over time", y=1.02)
fig.tight_layout()
fig.savefig(OUTPUT / "cross_regional_trade_timeseries.png", dpi=150, bbox_inches="tight")
plt.show()

# %%
# Buyer x source flow matrices (window-averaged), per scenario. Off-diagonal = cross-regional.
flow = bil_mean[bil_mean["t"] > (tmax - WINDOW)].groupby(
    ["scenario", "region"], as_index=False
)[_bil_cols].mean()

n_reg = len(regions)
reg_idx = {reg: i for i, reg in enumerate(regions)}
goods = [("machines", mach_src_cols), ("C-goods", cons_src_cols)]

fig, axes = plt.subplots(
    len(goods), len(scenarios),
    figsize=(3.6 * len(scenarios), 3.4 * len(goods)), squeeze=False,
)
for g_i, (good_label, cols_map) in enumerate(goods):
    # Shared colour scale per good-type across scenarios for comparability
    mats = {}
    for scen in scenarios:
        M = np.zeros((n_reg, n_reg))
        for _, row in flow[flow["scenario"] == scen].iterrows():
            r = reg_idx[int(row["region"])]
            for s in regions:
                M[r, reg_idx[s]] = row[cols_map[s]]
        mats[scen] = M
    vmax = max((m.max() for m in mats.values()), default=1.0) or 1.0
    for s_i, scen in enumerate(scenarios):
        ax = axes[g_i][s_i]
        M = mats[scen]
        im = ax.imshow(M, cmap="viridis", vmin=0, vmax=vmax, aspect="equal")
        ax.set_xticks(range(n_reg)); ax.set_xticklabels([f"R{r}" for r in regions])
        ax.set_yticks(range(n_reg)); ax.set_yticklabels([f"R{r}" for r in regions])
        ax.set_xlabel("source region (seller)")
        if s_i == 0:
            ax.set_ylabel(f"{good_label}\nbuyer region")
        ax.set_title(f"scenario {scen}", fontsize=10)
        for r in range(n_reg):
            for c in range(n_reg):
                ax.text(c, r, f"{M[r, c]:.2g}", ha="center", va="center",
                        color="white" if M[r, c] < 0.6 * vmax else "black", fontsize=7)
        fig.colorbar(im, ax=ax, fraction=0.046, pad=0.04)
fig.suptitle(f"Buyer x source trade flows (mean over last {WINDOW}t)", y=1.01)
fig.tight_layout()
fig.savefig(OUTPUT / "cross_regional_flow_matrix.png", dpi=150, bbox_inches="tight")
plt.show()

# Print local vs cross-regional totals per scenario (window mean)
cross_summary = cross_ts[cross_ts["t"] > (tmax - WINDOW)].groupby("scenario")[["mach_cross", "cons_cross"]].mean()
print(f"Mean cross-regional trade over last {WINDOW} periods:")
print(cross_summary.round(2).to_string())

# %%
