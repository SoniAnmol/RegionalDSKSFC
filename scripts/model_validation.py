#%% import libraries
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

#%% set up paths
# Set path for output figures
OUTPUT = Path("/Users/anmolsoni/Nextcloud/projects/Regionalised-SFC-DSK/figures/stylised_facts/")
OUTPUT.mkdir(parents=True, exist_ok=True)

# define paths to experiment runs.
PATHS = {
    'scenario' : Path("/Users/anmolsoni/Nextcloud/projects/Regionalised-SFC-DSK/stylised_facts"),
    }

#%% load simulation result matrices
sim_matrix = {}
for PATH in PATHS.keys():
    sim_matrix[PATH] = load_simulation_results_matrix(PATHS[PATH])

# Extract simulation results
scenario_results = {}
for sim in sim_matrix.keys():
    for scenario_num in sim_matrix[sim].scenario.unique():
        scenario_results[f"{sim}_{scenario_num}"] = sim_matrix[sim].loc[sim_matrix[sim].scenario == scenario_num]

# set colormap for regions and scenarios
region_meta = {
    "macro": {"name": "Macro",   "color": "#4A4A4A"},
    1:       {"name": "North",   "color": "#5665CC"},
    2:       {"name": "Central", "color": "#EC4B37"},
    3:       {"name": "South",   "color": "#03C58E"},
}
SCENARIO_META = {
    "scenario_1": {"name": "No Shock",             "color": "#B0B0B0", "linestyle": ":",  "linewidth": 1.5},
    "scenario_2": {"name": "Climate Shock",         "color": "#4A4A4A", "linestyle": "--", "linewidth": 1.5},
    "scenario_3": {"name": "Recovery",              "color": "#7B2D8B", "linestyle": "-",  "linewidth": 2},
    "scenario_4": {"name": "Protection",            "color": "#FF6F00", "linestyle": "-",  "linewidth": 2},
    "scenario_5": {"name": "Recovery + Protection", "color": "#005F73", "linestyle": "-",  "linewidth": 2},
}
green = "#4dff88"  
red = "#ff4d73"

#%% Compute Macros
def compare_scenarios(df_a, df_b, metric_a, metric_b=None, t_min=300, label_a="DSK", label_b="R-DSK", alpha=0.05, region="macro"):
    """Compare a metric between two scenario DataFrames using Welch's t-test.

    Observations are first averaged per Monte Carlo run (over t > t_min),
    so each run contributes exactly one independent data point to the test.

    Parameters
    ----------
    df_a, df_b : pandas.DataFrame
        Scenario DataFrames containing columns ``t``, ``run``, and ``metric``.
    metric_a : str
        Column name from df_a to compare (e.g. 'Unemployment', 'GDP_r_growth').
    metric_b : str
        Column name from  df_b to compare (e.g. 'Unemployment', 'GDP_r_growth').
    t_min : int, default 300
        Only include observations with t > t_min before averaging.
    label_a, label_b : str
        Display labels for each scenario in printed output.
    alpha : float, default 0.05
        Significance level for the test.
    """
    if metric_b is None:
        metric_b = metric_a
        
    # Filter to macro region if multiple regions present
    if "region" in df_a.columns and df_a["region"].nunique() > 1:
        df_a = df_a[df_a["region"] == region]
    if "region" in df_b.columns and df_b["region"].nunique() > 1:
        df_b = df_b[df_b["region"] == region]

    # Aggregate to run-level means (one independent observation per run)
    a = (
        df_a.loc[df_a["t"] > t_min, ["run", metric_a]]
        .groupby("run")[metric_a].mean()
        .dropna()
    )
    b = (
        df_b.loc[df_b["t"] > t_min, ["run", metric_b]]
        .groupby("run")[metric_b].mean()
        .dropna()
    )

    print(f"Metric : {metric_a} ({label_a}) and {metric_b} ({label_b})  (t > {t_min},  aggregated to run-level means)")
    print(f"  {label_a:10s}  mean={a.mean():.4f}  std={a.std():.4f}  n_runs={len(a)}")
    print(f"  {label_b:10s}  mean={b.mean():.4f}  std={b.std():.4f}  n_runs={len(b)}")

    # Welch's t-test on run-level means
    t_stat, p_value = stats.ttest_ind(a, b, equal_var=True)
    print(f"\n  Welch's t-test  (H0: equal means across runs)")
    print(f"    t-statistic : {t_stat:.4f}")
    print(f"    p-value     : {p_value:.4e}")
    print(f"    Significant : {'Yes' if p_value < alpha else 'No'} (α = {alpha})")

    # ── Print table ─────────────────────────────────────────────────────────
    def _sig_star(p):
        if p < 0.001: return "***"
        if p < 0.01:  return "**"
        if p < 0.05:  return "*"
        return ""

    ratio = b.mean() / a.mean()
    col_width = 22
    metric_display = metric_a if metric_a == metric_b else f"{metric_a}/{metric_b}"
    col_label = f"{label_b} / {label_a}"
    header_parts = [f"{'Metric':>25s}", f"{col_label:^{col_width}s}"]
    sep = "  ".join(["-" * 25, "-" * col_width])
    star = _sig_star(p_value)
    cell = f"{ratio:.4f} (t={t_stat:+.2f}){star}"

    print()
    print("  ".join(header_parts))
    print(sep)
    print("  ".join([f"{metric_display:>25s}", f"{cell:^{col_width}s}"]))
    print(sep)
    print(f"\nRatio = {label_b} / {label_a}  (run-level means, t > {t_min})")
    print(f"Significance: * p<0.05, ** p<0.01, *** p<0.001")
    print()


def compare_crisis_likelihood(df_a, df_b, t_min=300, threshold=-5.0,
                               label_a="DSK", label_b="HRHS", alpha=0.05):
    """Compare per-run crisis likelihood between two scenario DataFrames.

    For each run, crisis likelihood = proportion of quarters (t > t_min)
    where GDP_r_growth < threshold. The resulting per-run likelihoods are
    then compared with Welch's t-test, giving one independent observation
    per Monte Carlo run.

    Parameters
    ----------
    df_a, df_b : pandas.DataFrame
        Scenario DataFrames containing columns ``t``, ``run``,
        and ``GDP_r_growth``.
    t_min : int, default 300
        Only include observations with t > t_min.
    threshold : float, default -5.0
        Crisis threshold in percent (Lamperti et al. 2018: -5.0).
    label_a, label_b : str
        Display labels for each scenario in printed output.
    alpha : float, default 0.05
        Significance level for the test.
    """
    def _run_level_likelihood(df):
        # Filter to macro region if multiple regions present
        if "region" in df.columns and df["region"].nunique() > 1:
            df = df[df["region"] == "macro"]

        work = df.loc[df["t"] > t_min, ["run", "GDP_r_growth"]].copy()
        valid = work["GDP_r_growth"].notna()

        # Binary crisis flag per row
        work["_crisis"] = np.where(valid, (work["GDP_r_growth"] < threshold).astype(int), np.nan)

        # Per-run likelihood = crisis quarters / valid quarters
        obs   = work[valid].groupby("run")["_crisis"].count()
        crises = work[valid].groupby("run")["_crisis"].sum()
        return (crises / obs.replace(0, np.nan)).astype(float).dropna()

    a = _run_level_likelihood(df_a)
    b = _run_level_likelihood(df_b)

    print(f"Crisis likelihood  (GDP_r_growth < {threshold}%,  t > {t_min})")
    print(f"  {label_a:10s}  mean={a.mean():.4f}  std={a.std():.4f}  n_runs={len(a)}")
    print(f"  {label_b:10s}  mean={b.mean():.4f}  std={b.std():.4f}  n_runs={len(b)}")

    t_stat, p_value = stats.ttest_ind(a, b, equal_var=True)
    print(f"\n  Welch's t-test  (H0: equal crisis likelihood across runs)")
    print(f"    t-statistic : {t_stat:.4f}")
    print(f"    p-value     : {p_value:.4e}")
    print(f"    Significant : {'Yes' if p_value < alpha else 'No'} (α = {alpha})")
    print()

def compare_regional_trends(
    metric,
    scenario_results,
    region_meta,
    scenario_num="scenario_1",
    metric_label=None,
    window=None,
    output=None,
    tmin=0,
    tmax=700,
    ci_std=1,
):
    if metric_label is None:
        metric_label = metric

    RATE_METRICS = {"GDP_r_growth"}
    LEVEL_MAP = {"GDP_r_growth": "GDP_r"}

    df = scenario_results[scenario_num]

    if metric in RATE_METRICS and metric in LEVEL_MAP:
        level_col = LEVEL_MAP[metric]

        if level_col not in df.columns:
            raise ValueError(
                f"Level column '{level_col}' needed to recompute "
                f"'{metric}' is missing."
            )

        # compute growth per run so run-level variance is preserved
        df_levels = df.copy().sort_values(["region", "run", "t"])
        df_levels[metric] = df_levels.groupby(["region", "run"])[level_col].transform(
            lambda x: ((x / x.shift(1)) ** 4 - 1) * 100
        )

        # aggregate across runs
        df_metric = (
            df_levels.groupby(["t", "region"])[metric]
            .agg(mean_val="mean", std_val="std")
            .reset_index()
        )

    else:
        # For non-rate metrics: aggregate directly across runs
        df_metric = (
            df.groupby(["t", "region"])[metric]
            .agg(mean_val="mean", std_val="std")
            .reset_index()
        )

    # rolling mean on full series BEFORE time filter
    if window is not None:
        df_metric["mean_val"] = df_metric.groupby("region")["mean_val"].transform(
            lambda x: x.rolling(window=window, min_periods=1).mean()
        )
        df_metric["std_val"] = df_metric.groupby("region")["std_val"].transform(
            lambda x: x.rolling(window=window, min_periods=1).mean()
        )

    # apply time filter after rolling mean
    df_metric = df_metric[(df_metric["t"] >= tmin) & (df_metric["t"] <= tmax)]

    fig, ax = plt.subplots(1, 1, figsize=(6, 4))

    for region in list(region_meta.keys()):
        if region == 'macro':
            continue
        sub = df_metric[df_metric["region"] == region]
        color = region_meta[region]["color"]

        ax.plot(sub["t"], sub["mean_val"], color=color, label=region_meta[region]["name"])
        ax.fill_between(
            sub["t"],
            sub["mean_val"] - ci_std * sub["std_val"],
            sub["mean_val"] + ci_std * sub["std_val"],
            color=color, alpha=0.2,
        )
        # ax.set_title(region_meta[region]["name"])
        ax.set_xlabel("Timestep")
        ax.grid(True, color="lightgray", linestyle="--", linewidth=0.5)
        if tmin <= 300:
            ax.axvline(x=300, color="gray", linestyle="--", linewidth=1)

        ax.set_ylabel(metric_label)
        ax.set_facecolor("#FFFFFF")
        ax.spines[["top", "right", "left", "bottom"]].set_visible(False)
    ax.legend(ncols=3, loc="lower center", bbox_to_anchor=(0.5, -0.35))
    plt.tight_layout()


    if output is not None:
        plt.savefig(
            output / f"{scenario_num}_{metric}.png",
            dpi=300, bbox_inches="tight"
        )

    plt.show()


def plot_regional_metric(
    metric,
    scenario_results,
    region_meta,
    scenario_num="scenario_1",
    metric_label=None,
    window=None,
    share_y_macro=False,
    regional_gap=0.01,
    output=None,
    tmin=0,
    tmax=700,
    ci_std=1,
    ylim_macro=None,
    ylim_regional=None,
):
    if metric_label is None:
        metric_label = metric

    RATE_METRICS = {"GDP_r_growth"}
    LEVEL_MAP = {"GDP_r_growth": "GDP_r"}

    df = scenario_results[scenario_num]

    if metric in RATE_METRICS and metric in LEVEL_MAP:
        level_col = LEVEL_MAP[metric]

        if level_col not in df.columns:
            raise ValueError(
                f"Level column '{level_col}' needed to recompute "
                f"'{metric}' is missing."
            )

        # Step 1: compute growth per run so run-level variance is preserved
        df_levels = df.copy().sort_values(["region", "run", "t"])
        df_levels[metric] = df_levels.groupby(["region", "run"])[level_col].transform(
            lambda x: ((x / x.shift(1)) ** 4 - 1) * 100
        )

        # Step 2: aggregate across runs
        df_metric = (
            df_levels.groupby(["t", "region"])[metric]
            .agg(mean_val="mean", std_val="std")
            .reset_index()
        )

    else:
        # For non-rate metrics: aggregate directly across runs
        df_metric = (
            df.groupby(["t", "region"])[metric]
            .agg(mean_val="mean", std_val="std")
            .reset_index()
        )

    # Step 3: rolling mean on full series BEFORE time filter
    if window is not None:
        df_metric["mean_val"] = df_metric.groupby("region")["mean_val"].transform(
            lambda x: x.rolling(window=window, min_periods=1).mean()
        )
        df_metric["std_val"] = df_metric.groupby("region")["std_val"].transform(
            lambda x: x.rolling(window=window, min_periods=1).mean()
        )

    # Step 4: apply time filter after rolling mean
    df_metric = df_metric[(df_metric["t"] >= tmin) & (df_metric["t"] <= tmax)]

    fig, axs = plt.subplots(1, 4, figsize=(15, 3), sharex=True)

    # sharey must be set BEFORE set_ylim so shared limits are not overridden
    axs[2].sharey(axs[1])
    axs[3].sharey(axs[1])
    if share_y_macro:
        axs[0].sharey(axs[1])

    # axs[0] = macro panel; axs[1-3] = regional panels
    if ylim_macro is not None:
        axs[0].set_ylim(ylim_macro)
    if ylim_regional is not None:
        axs[1].set_ylim(ylim_regional)  # axs[2] and axs[3] follow via sharey

    for i, region in enumerate(region_meta.keys()):
        sub = df_metric[df_metric["region"] == region]
        color = region_meta[region]["color"]

        axs[i].plot(sub["t"], sub["mean_val"], color=color)
        axs[i].fill_between(
            sub["t"],
            sub["mean_val"] - ci_std * sub["std_val"],
            sub["mean_val"] + ci_std * sub["std_val"],
            color=color, alpha=0.2,
        )
        axs[i].set_title(region_meta[region]["name"])
        axs[i].set_xlabel("Timestep")
        axs[i].grid(True, color="lightgray", linestyle="--", linewidth=0.5)
        axs[i].spines[["top", "right", "left", "bottom"]].set_visible(False)

        if tmin == 300:
            axs[i].axvline(x=300, color="gray", linestyle="--", linewidth=1)

        if i == 0:
            axs[i].set_ylabel(metric_label)
            axs[i].set_facecolor("#FFFBF3")
            axs[i].spines[["top", "right", "left", "bottom"]].set_visible(True)
        elif i == 1:
            axs[i].set_ylabel(metric_label)
        else:
            axs[i].tick_params(labelleft=False)

    plt.tight_layout()
    fig.canvas.draw()

    for j in [2, 3]:
        prev = axs[j - 1].get_position()
        cur = axs[j].get_position()
        axs[j].set_position([prev.x1 + regional_gap, cur.y0, cur.width, cur.height])

    fig.canvas.draw()

    b1, b3 = axs[1].get_position(), axs[3].get_position()
    fig.add_artist(FancyBboxPatch(
        (b1.x0 + 0.001, b1.y0),
        b3.x1 - b1.x0,
        b1.y1 - b1.y0,
        boxstyle="round,pad=0.005",
        linewidth=1.2,
        edgecolor="gray",
        facecolor="none",
        transform=fig.transFigure,
        clip_on=False,
    ))

    if output is not None:
        plt.savefig(
            output / f"{scenario_num}_{metric}_panel.png",
            dpi=300, bbox_inches="tight"
        )

    plt.show()

def compare_regional_metric(
    metric,
    scenario_results,
    region_meta,
    scenario_num,
    baseline_scenario_num,
    no_shock_scenario_num=None,
    metric_label=None,
    window=20,
    share_y_macro=False,
    regional_gap=0.01,
    output=None,
    tmin=0,
    tmax=700,
    color_above="#e08214",
    color_below="#8073ac",
    scenario_meta=None,
    ylim_macro=None,
    ylim_regional=None,
):
    from matplotlib.lines import Line2D
    from matplotlib.patches import Patch

    if metric_label is None:
        metric_label = metric

    # Accept explicit metadata, but keep backward-compatible fallbacks.
    if scenario_meta is None:
        scenario_meta = globals().get("SCENARIO_META") or globals().get("SCENARIO_META")
    if scenario_meta is None:
        raise ValueError("scenario_meta is required. Define SCENARIO_META/SCENARIO_META or pass scenario_meta=...")

    # Metrics that must be recomputed from levels after averaging runs
    RATE_METRICS = {"GDP_r_growth", "GDP_r_volatility"}

    # Map rate metric -> the underlying level column needed to recompute it
    LEVEL_MAP = {
        "GDP_r_growth": "GDP_r",
    }

    def _prepare(snum):
        df = scenario_results[snum]

        if metric in RATE_METRICS and metric in LEVEL_MAP:
            level_col = LEVEL_MAP[metric]

            # Step 1: average LEVELS across runs
            df_m = (
                df.groupby(["t", "region"])[level_col]
                .mean()
                .reset_index()
            )

            # Step 2: compute growth from averaged levels
            df_m = df_m.sort_values(["region", "t"])
            df_m[metric] = df_m.groupby("region")[level_col].transform(
                lambda x: ((x / x.shift(1)) ** 4 - 1) * 100
            )

        else:
            # For non-rate metrics average directly as before
            df_m = (
                df.groupby(["t", "region"])[metric]
                .mean()
                .reset_index()
            )

        # Step 3: rolling mean after computing the metric
        if window is not None:
            df_m[metric] = df_m.groupby("region")[metric].transform(
                lambda x: x.rolling(window=window, min_periods=1).mean()
            )

        return df_m[(df_m["t"] >= tmin) & (df_m["t"] <= tmax)]

    df_base = _prepare(baseline_scenario_num)
    df_scen = _prepare(scenario_num)
    if no_shock_scenario_num is not None:
        df_no_shock = _prepare(no_shock_scenario_num)

    fig, axs = plt.subplots(1, 4, figsize=(15, 3), sharex=True)
    axs[2].sharey(axs[1])
    axs[3].sharey(axs[1])
    if share_y_macro:
        axs[0].sharey(axs[1])

    # Set different y-limits for macro vs regional if specified
    if ylim_macro is not None:
        axs[0].set_ylim(ylim_macro)
    if ylim_regional is not None:
        for i in [1, 2, 3]:
            axs[i].set_ylim(ylim_regional)
            
    for i, region in enumerate(region_meta.keys()):
        color = region_meta[region]["color"]

        sub_base = df_base[df_base["region"] == region]
        sub_scen = df_scen[df_scen["region"] == region]
        if no_shock_scenario_num is not None:
            sub_no_shock = df_no_shock[df_no_shock["region"] == region]

        merged = (
            sub_base[["t", metric]].rename(columns={metric: "_base"})
            .merge(sub_scen[["t", metric]].rename(columns={metric: "_scen"}), on="t")
        )
        if no_shock_scenario_num is not None:
            merged = merged.merge(
                sub_no_shock[["t", metric]].rename(columns={metric: "_no_shock"}),
                on="t"
            )

        t_vals = merged["t"].values
        base_vals = merged["_base"].values
        scen_vals = merged["_scen"].values
        if no_shock_scenario_num is not None:
            no_shock_vals = merged["_no_shock"].values

        axs[i].plot(
            t_vals, base_vals, color=color, linewidth=1.5,
            linestyle=scenario_meta[baseline_scenario_num]["linestyle"]
        )
        axs[i].plot(
            t_vals, scen_vals, color=color, linewidth=1.5,
            linestyle=scenario_meta[scenario_num]["linestyle"]
        )
        if no_shock_scenario_num is not None:
            axs[i].plot(
                t_vals, no_shock_vals, color=color, linewidth=1.5,
                linestyle=scenario_meta[no_shock_scenario_num]["linestyle"]
            )

        axs[i].fill_between(
            t_vals, base_vals, scen_vals,
            where=(scen_vals >= base_vals), interpolate=True,
            facecolor=color_above, edgecolor="gray",
            alpha=0.25, hatch="////", linewidth=0.5,
        )
        axs[i].fill_between(
            t_vals, base_vals, scen_vals,
            where=(scen_vals < base_vals), interpolate=True,
            facecolor=color_below, edgecolor="gray",
            alpha=0.25, hatch=r"\\\\", linewidth=0.5,
        )

        axs[i].axvline(x=300, color="black", linestyle="--", linewidth=0.8, alpha=0.5)
        axs[i].set_title(region_meta[region]["name"])
        axs[i].set_xlabel("Timestep")
        axs[i].grid(True, color="lightgray", linestyle="--", linewidth=0.5)
        axs[i].spines[["top", "right", "left", "bottom"]].set_visible(False)

        if i == 0:
            axs[i].set_ylabel(metric_label)
            axs[i].set_facecolor("#FFFBF3")
            axs[i].spines[["top", "right", "left", "bottom"]].set_visible(True)
        elif i == 1:
            axs[i].set_ylabel("")
        else:
            axs[i].tick_params(labelleft=False)

    legend_handles = [
        Line2D([0], [0], color="gray",
               linestyle=scenario_meta[snum]["linestyle"],
               linewidth=1.5,
               label=scenario_meta[snum]["name"])
        for snum in [baseline_scenario_num, scenario_num]
    ]
    if no_shock_scenario_num is not None:
        legend_handles.append(
            Line2D([0], [0], color="gray",
                   linestyle=scenario_meta[no_shock_scenario_num]["linestyle"],
                   linewidth=1.5,
                   label=scenario_meta[no_shock_scenario_num]["name"])
        )
    legend_handles += [
        Patch(facecolor=color_above, alpha=0.25, linewidth=0.1,
              label=f"{scenario_meta[scenario_num]['name']} > {scenario_meta[baseline_scenario_num]['name']}",
              hatch="////"),
        Patch(facecolor=color_below, alpha=0.25, linewidth=0.1,
              label=f"{scenario_meta[scenario_num]['name']} < {scenario_meta[baseline_scenario_num]['name']}",
              hatch=r"\\\\"),
    ]

    fig.legend(
        handles=legend_handles, loc="lower center",
        ncol=len(legend_handles), frameon=False,
        bbox_to_anchor=(0.5, -0.08)
    )

    plt.tight_layout()
    fig.canvas.draw()

    for j in [2, 3]:
        prev = axs[j - 1].get_position()
        cur = axs[j].get_position()
        axs[j].set_position([prev.x1 + regional_gap, cur.y0, cur.width, cur.height])

    fig.canvas.draw()

    b1, b3 = axs[1].get_position(), axs[3].get_position()
    fig.add_artist(FancyBboxPatch(
        (b1.x0 + 0.001, b1.y0),
        b3.x1 - b1.x0,
        b1.y1 - b1.y0,
        boxstyle="round,pad=0.005",
        linewidth=1.2,
        edgecolor="gray",
        facecolor="none",
        transform=fig.transFigure,
        clip_on=False,
    ))

    if output is not None:
        plt.savefig(
            output / f"{baseline_scenario_num}_vs_{scenario_num}_{metric}_panel_{tmin}-{tmax}.png",
            dpi=300, bbox_inches="tight",
        )

    plt.show()

def plot_regional_metric_distribution(
    metric,
    scenario_results,
    region_meta,
    scenario_meta,
    scenario_num,
    baseline_scenario_num,
    no_shock_scenario_num=None,
    metric_label=None,
    share_y_macro=False,
    output=None,
    ylim_macro=None,
    ylim_regional=None,
    summary="run_mean",  # "run_mean", "final", or "pooled_time_run"
    tmin=300,
    tmax=700,
):
    """
    Plot distribution of a metric across scenarios.

    summary:
        "run_mean"        -> one value per run: mean over time
        "final"           -> one value per run: final timestep only
        "pooled_time_run" -> one value per timestep/run; pools time and runs
    """
    import numpy as np
    import pandas as pd
    import seaborn as sns
    import matplotlib.pyplot as plt
    import matplotlib.collections as mcollections
    from matplotlib.lines import Line2D
    from matplotlib.patches import FancyBboxPatch

    if metric_label is None:
        metric_label = metric

    if "macro" not in region_meta:
        raise ValueError("region_meta must contain a 'macro' entry.")

    real_regions = [r for r in region_meta if r != "macro"]

    scenarios_to_plot = [baseline_scenario_num, scenario_num]
    if no_shock_scenario_num is not None:
        scenarios_to_plot = [no_shock_scenario_num, baseline_scenario_num, scenario_num]

    for snum in scenarios_to_plot:
        if snum not in scenario_results:
            raise KeyError(f"{snum!r} is missing from scenario_results.")
        if snum not in scenario_meta:
            raise KeyError(f"{snum!r} is missing from scenario_meta.")

    scenario_order = [scenario_meta[snum]["name"] for snum in scenarios_to_plot]
    scenario_linestyles = [scenario_meta[snum]["linestyle"] for snum in scenarios_to_plot]
    use_split = no_shock_scenario_num is None

    def _prepare(snum, regions):
        df = scenario_results[snum].copy()

        df = df[(df["t"] >= tmin) & (df["t"] <= tmax)]

        required_cols = {"t", "region", "run", metric}
        missing = required_cols - set(df.columns)
        if missing:
            raise ValueError(f"Scenario {snum!r} is missing columns: {missing}")

        df = df[df["region"].isin(regions)]

        if summary == "pooled_time_run":
            df_m = (
                df.groupby(["t", "region", "run"], as_index=False)[metric]
                .mean()
            )

        elif summary == "run_mean":
            df_m = (
                df.groupby(["region", "run"], as_index=False)[metric]
                .mean()
            )

        elif summary == "final":
            final_t = df["t"].max()
            df_final = df[df["t"] == final_t]
            df_m = (
                df_final.groupby(["region", "run"], as_index=False)[metric]
                .mean()
            )

        else:
            raise ValueError(
                "summary must be one of: 'run_mean', 'final', 'pooled_time_run'"
            )

        df_m["Scenario"] = scenario_meta[snum]["name"]
        df_m["ScenarioKey"] = snum
        return df_m

    df_reg = pd.concat(
        [_prepare(snum, real_regions) for snum in scenarios_to_plot],
        ignore_index=True,
    )
    region_order = [region_meta[r]["name"] for r in real_regions]
    df_reg["Region"] = df_reg["region"].map(lambda r: region_meta[r]["name"])

    df_mac = pd.concat(
        [_prepare(snum, ["macro"]) for snum in scenarios_to_plot],
        ignore_index=True,
    )
    df_mac["Region"] = region_meta["macro"]["name"]

    def _color_violins_by_region(ax, ordered_names, split=True):
        poly_cols = [
            c for c in ax.collections
            if isinstance(c, mcollections.PolyCollection)
        ]
        n_regions = len(ordered_names)

        if split:
            for i, name in enumerate(ordered_names):
                region_key = next(k for k, v in region_meta.items() if v["name"] == name)
                color = region_meta[region_key]["color"]

                for j in range(2):
                    idx = i * 2 + j
                    if idx < len(poly_cols):
                        poly_cols[idx].set_facecolor("none")
                        poly_cols[idx].set_edgecolor(color)
                        poly_cols[idx].set_linewidth(1.5)
                        poly_cols[idx].set_linestyle(scenario_linestyles[j])

        else:
            for k, pc in enumerate(poly_cols):
                scenario_idx = k // n_regions
                region_idx = k % n_regions

                if scenario_idx >= len(scenarios_to_plot) or region_idx >= n_regions:
                    continue

                name = ordered_names[region_idx]
                region_key = next(rk for rk, rv in region_meta.items() if rv["name"] == name)
                color = region_meta[region_key]["color"]

                pc.set_facecolor("none")
                pc.set_edgecolor(color)
                pc.set_linewidth(1.5)
                pc.set_linestyle(scenario_linestyles[scenario_idx])

        for line in ax.lines:
            xdata = line.get_xdata()
            if len(xdata) == 0:
                continue

            violin_idx = int(round(float(np.mean(xdata))))
            if 0 <= violin_idx < n_regions:
                name = ordered_names[violin_idx]
                region_key = next(k for k, v in region_meta.items() if v["name"] == name)
                line.set_color(region_meta[region_key]["color"])

    fig, (ax_mac, ax_reg) = plt.subplots(
        1, 2,
        figsize=(13, 4),
        gridspec_kw={"width_ratios": [1, 3]},
        sharey=share_y_macro,
    )
    if ylim_macro is not None:
        ax_mac.set_ylim(ylim_macro)
    if ylim_regional is not None:
        ax_reg.set_ylim(ylim_regional)
    if use_split:
        sns.violinplot(
            data=df_mac,
            x="Region",
            y=metric,
            hue="Scenario",
            hue_order=scenario_order,
            fill=False,
            split=True,
            inner="quartile",
            ax=ax_mac,
        )
        _color_violins_by_region(
            ax_mac,
            [region_meta["macro"]["name"]],
            split=True,
        )

        sns.violinplot(
            data=df_reg,
            x="Region",
            y=metric,
            hue="Scenario",
            hue_order=scenario_order,
            order=region_order,
            fill=False,
            split=True,
            inner="quartile",
            ax=ax_reg,
            legend=False,
        )
        _color_violins_by_region(ax_reg, region_order, split=True)

    else:
        for snum in scenarios_to_plot:
            df_s = df_mac[df_mac["ScenarioKey"] == snum]
            sns.violinplot(
                data=df_s,
                x="Region",
                y=metric,
                fill=False,
                inner="quartile",
                ax=ax_mac,
            )

        _color_violins_by_region(
            ax_mac,
            [region_meta["macro"]["name"]],
            split=False,
        )

        for snum in scenarios_to_plot:
            df_s = df_reg[df_reg["ScenarioKey"] == snum]
            sns.violinplot(
                data=df_s,
                x="Region",
                y=metric,
                order=region_order,
                fill=False,
                inner="quartile",
                ax=ax_reg,
                legend=False,
            )

        _color_violins_by_region(ax_reg, region_order, split=False)

    ax_mac.set_facecolor("#FFFBF3")
    ax_mac.spines[["top", "right", "left", "bottom"]].set_visible(True)
    ax_mac.set_ylabel(metric_label)
    ax_mac.set_xlabel("")
    ax_mac.grid(True, color="lightgray", linestyle="--", linewidth=0.5, axis="y")

    if ax_mac.get_legend() is not None:
        ax_mac.get_legend().remove()

    ax_reg.set_xlabel("")
    ax_reg.set_ylabel("")
    ax_reg.grid(True, color="lightgray", linestyle="--", linewidth=0.5, axis="y")
    ax_reg.spines[["top", "right", "left", "bottom"]].set_visible(False)


    if share_y_macro:
        ax_reg.tick_params(labelleft=False)

    if ax_reg.get_legend() is not None:
        ax_reg.get_legend().remove()

    legend_handles = [
        Line2D(
            [0],
            [0],
            color="gray",
            linestyle=scenario_meta[snum]["linestyle"],
            linewidth=1.5,
            label=scenario_meta[snum]["name"],
        )
        for snum in scenarios_to_plot
    ]

    ax_reg.legend(
        handles=legend_handles,
        frameon=False,
        loc="lower center",
        fontsize=12,
        ncol=len(legend_handles),
        bbox_to_anchor=(0.25, -0.25),
    )

    plt.tight_layout()
    fig.canvas.draw()

    b = ax_reg.get_position()
    fig.add_artist(
        FancyBboxPatch(
            (b.x0 + 0.001, b.y0),
            b.x1 - b.x0,
            b.y1 - b.y0,
            boxstyle="round,pad=0.005",
            linewidth=1.2,
            edgecolor="gray",
            facecolor="none",
            transform=fig.transFigure,
            clip_on=False,
        )
    )

    b_mac = ax_mac.get_position()
    gap = 0.03
    ax_mac.set_position([b.x0 - b_mac.width - gap, b.y0, b_mac.width, b_mac.height])
#
    if output is not None:
        plt.savefig(
            output / f"{baseline_scenario_num}_vs_{scenario_num}_{metric}_distribution.png",
            dpi=300,
            bbox_inches="tight",
        )

    plt.show()

#%% Compute Macros
for scenario in scenario_results.keys():
    #  Compute unemployment
    scenario_results[scenario]['Unemployment'] = (1 - scenario_results[scenario].EmploymentRate) * 100
    # Compute GDP volatility
    scenario_results[scenario]['GDP_r_volatility'] = compute_GDP_r_volatility(scenario_results[scenario])
    # Compute log GDP
    scenario_results[scenario]['GDP_r_log'] = np.log(scenario_results[scenario]['GDP_r'])
    #  Compute Output Growth Rate
    scenario_results[scenario]['GDP_r_growth'] = compute_gdp_r_growth(scenario_results[scenario])
    # Compute GDP Growth Rate per capita
    scenario_results[scenario]['GDP_r_growth_per_capita'] = scenario_results[scenario]['GDP_r_growth'] / scenario_results[scenario]['LS']
    # compute deficit based on Deficit = Deficit / GDP_n
    scenario_results[scenario]['Deficit'] = scenario_results[scenario]['Deficit'] / scenario_results[scenario]['GDP_n'] 
    # Compute I_protection over GDP if I_protection exists
    if 'I_protection' in scenario_results[scenario].columns:
        scenario_results[scenario]['I_protection_over_GDP'] = scenario_results[scenario]['I_protection'] / scenario_results[scenario]['GDP_r'] 
    # compute recovery over GDP if Recovery exists
    if 'Recovery' in scenario_results[scenario].columns:
        scenario_results[scenario]['Recovery_over_GDP'] = scenario_results[scenario]['Recovery'] / scenario_results[scenario]['GDP_r'] 
    # compute recovery over RegGov_TopupGrant
    if 'Recovery' in scenario_results[scenario].columns:
        scenario_results[scenario]['Recovery_over_RegGov_TopupGrant'] = scenario_results[scenario]['Recovery'] / scenario_results[scenario]['RegGov_TopupGrant']
    # Rolling crisis likelihood: fraction of last 4 quarters where GDP growth < -5%
    scenario_results[scenario]['crisis_likelihood'] = (
        scenario_results[scenario]
        .groupby('run')['GDP_r_growth']
        .transform(lambda x: (x < -5).rolling(1, min_periods=1).mean())
    )


# %% Regional unemployment

compare_regional_trends(
    metric="Unemployment",
    scenario_results=scenario_results,
    region_meta=region_meta,
    scenario_num="scenario_1",
    metric_label="Unemployment",
    window=None,
    output=OUTPUT,
    tmin=600,
    tmax=700,
    ci_std=1,
)
# %% Regioanl real GDP
compare_regional_trends(
    metric="GDP_r",
    scenario_results=scenario_results,
    region_meta=region_meta,
    scenario_num="scenario_1",
    metric_label="real GDP",
    window=None,
    output=OUTPUT,
    tmin=600,
    tmax=700,
    ci_std=1,
)

# %% Regioanl wage
compare_regional_trends(
    metric="reg_w",
    scenario_results=scenario_results,
    region_meta=region_meta,
    scenario_num="scenario_1",
    metric_label="wage",
    window=None,
    output=OUTPUT,
    tmin=600,
    tmax=700,
    ci_std=1,
)
# %% Mean productivity across K & C Firms

compare_regional_trends(
    metric="Am",
    scenario_results=scenario_results,
    region_meta=region_meta,
    scenario_num="scenario_1",
    metric_label="Mean Productivity",
    window=None,
    output=OUTPUT,
    tmin=600,
    tmax=700,
    ci_std=1,
)

# %% Population

compare_regional_trends(
    metric="LS",
    scenario_results=scenario_results,
    region_meta=region_meta,
    scenario_num="scenario_1",
    metric_label="Population",
    window=None,
    output=OUTPUT,
    tmin=600,
    tmax=700,
    ci_std=1,
)

# %% K Firm output across regions
# plot_regional_metric(
#     metric="Q1tot",
#     metric_label="K-firm output",
#     scenario_results=scenario_results,
#     region_meta=region_meta,
#     scenario_num='scenario_1',
#     share_y_macro=False,
#     output=OUTPUT,
#     tmin=300,
#     window=1,
# )

compare_regional_trends(
    metric="Q1tot",
    scenario_results=scenario_results,
    region_meta=region_meta,
    scenario_num="scenario_1",
    metric_label="K-firm output",
    window=None,
    output=OUTPUT,
    tmin=0,
    tmax=700,
    ci_std=1,
)

compare_regional_trends(
    metric="N1r",
    scenario_results=scenario_results,
    region_meta=region_meta,
    scenario_num="scenario_1",
    metric_label="Number of K Firms",
    window=None,
    output=OUTPUT,
    tmin=0,
    tmax=700,
    ci_std=1,
)

# %% C Firm output across regions
compare_regional_trends(
    metric="Q2tot",
    scenario_results=scenario_results,
    region_meta=region_meta,
    scenario_num="scenario_1",
    metric_label="C-firm output",
    window=None,
    output=OUTPUT,
    tmin=0,
    tmax=700,
    ci_std=1,
)

compare_regional_trends(
    metric="N2r",
    scenario_results=scenario_results,
    region_meta=region_meta,
    scenario_num="scenario_1",
    metric_label="Number of C Firms",
    window=None,
    output=OUTPUT,
    tmin=0,
    tmax=700,
    ci_std=1,
)
# %%
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

# %% Amount of cross-regional trade, from the bilateral buyer x source columns.
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

# %% Buyer x source flow matrices (window-averaged), per scenario. Off-diagonal = cross-regional.
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
        im = ax.imshow(M, cmap="RdPu", vmin=0, vmax=vmax, aspect="equal")
        ax.set_xticks(range(n_reg)); ax.set_xticklabels([f"R{r}" for r in regions])
        ax.set_yticks(range(n_reg)); ax.set_yticklabels([f"R{r}" for r in regions])
        ax.set_xlabel("source region (seller)")
        if s_i == 0:
            ax.set_ylabel(f"{good_label}\nbuyer region")
        ax.set_title(f"scenario {scen}", fontsize=10)
        for r in range(n_reg):
            for c in range(n_reg):
                ax.text(c, r, f"{M[r, c]:.2g}", ha="center", va="center",
                        color="black" if M[r, c] < 0.6 * vmax else "white", fontsize=7)
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
# Diagnostic: bilateral local share vs reported import share
diag = trade_df[trade_df["scenario"] == 1].copy()
regions_diag = sorted(diag["region"].unique())

cons_cols = [f"cons_buy_from_R{s}" for s in regions_diag]

diag["bilat_total"] = diag[cons_cols].sum(axis=1)
diag["bilat_local"] = np.nan

for r in regions_diag:
    mask = diag["region"] == r
    diag.loc[mask, "bilat_local"] = diag.loc[
        mask, f"cons_buy_from_R{r}"
    ]

diag["local_share_bilat"] = (
    diag["bilat_local"] /
    diag["bilat_total"].replace(0, np.nan)
)

diag["local_share_reported"] = 1.0 - diag["cons_import_share"]

diag["share_gap"] = (
    diag["local_share_bilat"] -
    diag["local_share_reported"]
)

print("\n=== Bilateral accounting consistency ===")
print(
    diag.groupby("region")[
        ["local_share_bilat",
         "local_share_reported",
         "share_gap"]
    ].mean()
)

print(
    "\nMaximum absolute gap:",
    diag["share_gap"].abs().max()
)

local_ts = (
    diag.groupby(["t", "region"])["local_share_bilat"]
    .mean()
    .unstack()
)

print(
    local_ts.loc[
        local_ts.index.isin(
            [1, 2, 5, 10, 25, 50, 100, 200, 300, 500, 700]
        )
    ]
)

local_ts.plot(
    figsize=(8, 4),
    title="Realised local consumption share"
)
plt.axhline(1/3, linestyle="--")
plt.ylabel("local purchase share")
plt.show()

# %%
cols = [
    "region",
    "t",
    "run",
    "local_share_bilat",
    "local_share_reported",
    "share_gap",
    "cons_import_share",
]

print(diag[cols].isna().groupby(diag["region"]).sum())

for r in sorted(diag["region"].unique()):
    sub = diag[diag["region"] == r].copy()

    both = sub[
        sub["local_share_bilat"].notna()
        & sub["local_share_reported"].notna()
    ]

    print(f"\nRegion {r}")
    print("all rows:", len(sub))
    print("rows with both:", len(both))

    print(
        "mean bilateral, same mask:",
        both["local_share_bilat"].mean()
    )

    print(
        "mean reported, same mask:",
        both["local_share_reported"].mean()
    )

    print(
        "mean gap, same mask:",
        (
            both["local_share_bilat"]
            - both["local_share_reported"]
        ).mean()
    )

    print(
        both[
            [
                "t",
                "run",
                "local_share_bilat",
                "local_share_reported",
                "cons_import_share",
            ]
        ].head(10)
    )

r = 1
run = sorted(diag["run"].unique())[0]

one = diag[
    (diag["region"] == r)
    & (diag["run"] == run)
][
    [
        "t",
        "local_share_bilat",
        "local_share_reported",
        "cons_import_share",
        "cons_buy_from_R1",
        "cons_buy_from_R2",
        "cons_buy_from_R3",
    ]
].sort_values("t")

print(one.loc[one["t"].isin([1, 50, 100, 150, 200, 300, 500, 700])])
# %%
test = trade_df[
    (trade_df["scenario"] == 1) &
    (trade_df["run"] == 1)
].copy()

cons_cols = [
    "cons_buy_from_R1",
    "cons_buy_from_R2",
    "cons_buy_from_R3",
]

test["bilat_total"] = test[cons_cols].sum(axis=1)

cols = [
    "t",
    "region",
    "bilat_total",
    "reg_YD",
    "reg_C",
    "LS_region_share",
]

print(
    test[
        (test["t"] >= 70) &
        (test["t"] <= 120)
    ][cols].to_string(index=False)
)

# %% K-firm imitation: within- vs cross-region counts
# Macro rows carry imit_within_count/imit_cross_count; regional rows carry
# within_region_imitation/cross_region_imitation (per-period counts; recorded
# regardless of flag_regional_imitation).
IMIT_MACRO = {"within": "imit_within_count", "cross": "imit_cross_count"}
IMIT_REG = {"within": "within_region_imitation", "cross": "cross_region_imitation"}


def _imit_cols_for_region(region):
    """Return (within_col, cross_col) for a region label ('macro' or int)."""
    return (IMIT_MACRO if region == "macro" else IMIT_REG).values()


def plot_imitation_within_cross(
    scenario_results,
    region_meta,
    scenario_num="scenario_1",
    window=20,
    output=None,
    tmin=0,
    tmax=700,
    ci_std=1,
):
    """Panel of within- (solid) vs cross-region (dashed) K-firm imitation counts.

    One subplot per region (macro + regions). Lines are the mean across Monte
    Carlo runs; shaded bands are +/- ci_std run-level standard deviations.
    """
    df = scenario_results[scenario_num]

    fig, axs = plt.subplots(
        1, len(region_meta), figsize=(3.8 * len(region_meta), 3.2), sharex=True
    )
    axs = np.atleast_1d(axs)

    for i, region in enumerate(region_meta.keys()):
        within_col, cross_col = _imit_cols_for_region(region)
        if within_col not in df.columns or cross_col not in df.columns:
            raise KeyError(
                f"Imitation columns {within_col!r}/{cross_col!r} missing. "
                "Re-run the model with the updated build."
            )

        sub = df[df["region"] == region]
        agg = (
            sub.groupby("t")[[within_col, cross_col]]
            .agg(["mean", "std"])
        )
        if window is not None:
            agg = agg.rolling(window=window, min_periods=1).mean()
        agg = agg[(agg.index >= tmin) & (agg.index <= tmax)]

        color = region_meta[region]["color"]
        t_vals = agg.index

        axs[i].plot(t_vals, agg[(within_col, "mean")], color=color, lw=1.5, label="Within-region")
        axs[i].fill_between(
            t_vals,
            agg[(within_col, "mean")] - ci_std * agg[(within_col, "std")],
            agg[(within_col, "mean")] + ci_std * agg[(within_col, "std")],
            color=color, alpha=0.18,
        )
        axs[i].plot(t_vals, agg[(cross_col, "mean")], color=color, lw=1.5, ls="--", label="Cross-region")
        axs[i].fill_between(
            t_vals,
            agg[(cross_col, "mean")] - ci_std * agg[(cross_col, "std")],
            agg[(cross_col, "mean")] + ci_std * agg[(cross_col, "std")],
            color=color, alpha=0.08,
        )

        axs[i].set_title(region_meta[region]["name"])
        axs[i].set_xlabel("Timestep")
        axs[i].grid(True, color="lightgray", linestyle="--", linewidth=0.5)
        axs[i].spines[["top", "right", "left", "bottom"]].set_visible(False)
        if tmin <= 300 <= tmax:
            axs[i].axvline(x=300, color="gray", linestyle="--", linewidth=1)
        if i == 0:
            axs[i].set_ylabel("Imitation events per period")
            axs[i].set_facecolor("#FFFBF3")
            axs[i].spines[["top", "right", "left", "bottom"]].set_visible(True)

    handles = [
        Line2D([0], [0], color="gray", lw=1.5, ls="-", label="Within-region"),
        Line2D([0], [0], color="gray", lw=1.5, ls="--", label="Cross-region"),
    ]
    fig.legend(handles=handles, loc="lower center", ncol=2, frameon=False, bbox_to_anchor=(0.5, -0.08))
    plt.tight_layout()

    if output is not None:
        plt.savefig(
            output / f"{scenario_num}_imitation_within_cross_panel.png",
            dpi=300, bbox_inches="tight",
        )
    plt.show()


def plot_imitation_cross_share(
    scenario_results,
    region_meta,
    scenario_num="scenario_1",
    window=20,
    output=None,
    tmin=0,
    tmax=700,
    ci_std=1,
):
    """Cross-region share of total K-firm imitation per region (and macro)."""
    df = scenario_results[scenario_num]

    fig, ax = plt.subplots(1, 1, figsize=(6, 4))

    for region in region_meta.keys():
        within_col, cross_col = _imit_cols_for_region(region)
        sub = df[df["region"] == region].copy()
        total = sub[within_col] + sub[cross_col]
        sub["_cross_share"] = np.where(total > 0, sub[cross_col] / total, np.nan)

        agg = sub.groupby("t")["_cross_share"].agg(["mean", "std"])
        if window is not None:
            agg = agg.rolling(window=window, min_periods=1).mean()
        agg = agg[(agg.index >= tmin) & (agg.index <= tmax)]

        color = region_meta[region]["color"]
        ax.plot(agg.index, agg["mean"], color=color, lw=1.5, label=region_meta[region]["name"])
        ax.fill_between(
            agg.index,
            agg["mean"] - ci_std * agg["std"],
            agg["mean"] + ci_std * agg["std"],
            color=color, alpha=0.15,
        )

    ax.set_xlabel("Timestep")
    ax.set_ylabel("Cross-region share of imitation")
    ax.grid(True, color="lightgray", linestyle="--", linewidth=0.5)
    ax.spines[["top", "right", "left", "bottom"]].set_visible(False)
    if tmin <= 300 <= tmax:
        ax.axvline(x=300, color="gray", linestyle="--", linewidth=1)
    ax.legend(ncols=len(region_meta), loc="lower center", bbox_to_anchor=(0.5, -0.3))
    plt.tight_layout()

    if output is not None:
        plt.savefig(
            output / f"{scenario_num}_imitation_cross_share.png",
            dpi=300, bbox_inches="tight",
        )
    plt.show()


from matplotlib.lines import Line2D  # used by plot_imitation_within_cross legend

plot_imitation_within_cross(
    scenario_results=scenario_results,
    region_meta=region_meta,
    scenario_num="scenario_1",
    window=20,
    output=OUTPUT,
    tmin=0,
    tmax=700,
    ci_std=1,
)

# plot_imitation_cross_share(
#     scenario_results=scenario_results,
#     region_meta=region_meta,
#     scenario_num="scenario_1",
#     window=20,
#     output=OUTPUT,
#     tmin=0,
#     tmax=700,
#     ci_std=1,
# )
# %%

# %% Regional stylised-fact statistics
# Complementary Monte Carlo summary statistics for four regional stylised facts
# (productivity dispersion/persistence, unemployment differentiation, business-cycle
# synchronisation, persistent shock effects). Run-level statistics first, then mean +
# 95% CI across runs (DSK/K+S / Reissl et al. convention). Macro rows excluded; time
# observations are never pooled across runs as if independent.
import warnings
import numpy as np
import pandas as pd
from scipy import stats as scipy_stats

# Local copy of the Hamilton filter (kept here to avoid importing
# analyse_stylised_facts, which forces the Matplotlib "Agg" backend at import).
def _hamilton_cycle(series, log_transform=False, h=8, p=4):
    """Hamilton-filter cyclical residual for quarterly data."""
    x = np.asarray(series, dtype=float)
    if not np.all(np.isfinite(x)):
        return np.full(max(0, len(x) - h - p + 1), np.nan)
    if log_transform:
        if np.any(x <= 0):
            return np.full(max(0, len(x) - h - p + 1), np.nan)
        x = np.log(x)
    n = len(x)
    if n <= h + p:
        return np.array([], dtype=float)
    t_idx = np.arange(p - 1, n - h)
    y = x[t_idx + h]
    X = np.column_stack([np.ones(len(t_idx))] + [x[t_idx - lag] for lag in range(p)])
    coef, *_ = np.linalg.lstsq(X, y, rcond=None)
    return y - X @ coef


# One model period = one quarter. Standard quarterly Hamilton-filter specification
# (h = 8 quarters = 2-year horizon, p = 4 quarterly lags) as in the Reissl-style validation.
_HAMILTON_H, _HAMILTON_P = 8, 4

# Metric names that denote regional unemployment (handled as absolute differences).
_UNEMP_METRICS = {"reg_U", "Unemployment", "U"}
# Fallback column search order when a requested metric name is not a literal column.
_METRIC_ALIASES = {
    "reg_U": ["reg_U", "Unemployment"],
    "reg_K": ["reg_K", "reg_CapitalStock", "reg_K_r", "K"],
}


def summarise_across_runs(values, confidence=0.95):
    """Mean, SD, n and Student-t confidence interval across MC run-level values."""
    arr = np.asarray(list(values), dtype=float)
    arr = arr[np.isfinite(arr)]
    n = int(arr.size)
    if n == 0:
        return {"mean": np.nan, "std": np.nan, "n_runs": 0, "ci_low": np.nan, "ci_high": np.nan}
    mean = float(np.mean(arr))
    if n > 1:
        std = float(np.std(arr, ddof=1))
        sem = std / np.sqrt(n)
        tcrit = float(scipy_stats.t.ppf(0.5 * (1.0 + confidence), df=n - 1))
        ci_low, ci_high = mean - tcrit * sem, mean + tcrit * sem
    else:
        std = np.nan
        ci_low = ci_high = np.nan
    return {"mean": mean, "std": std, "n_runs": n, "ci_low": ci_low, "ci_high": ci_high}


def _resolve_metric_column(df, metric):
    """Return an existing column for a requested metric, deriving unemployment if needed."""
    if metric in df.columns:
        return metric
    for cand in _METRIC_ALIASES.get(metric, []):
        if cand in df.columns:
            return cand
    if metric in _UNEMP_METRICS and "EmploymentRate" in df.columns:
        return "Unemployment"  # derived on demand
    return None


def _regional_frame(df, tmin, tmax):
    """Macro-excluded, time-windowed copy with a derived Unemployment column."""
    reg = df[df["region"] != "macro"].copy()
    reg = reg[(reg["t"] >= tmin) & (reg["t"] <= tmax)]
    if "Unemployment" not in reg.columns and "EmploymentRate" in reg.columns:
        reg["Unemployment"] = (1.0 - reg["EmploymentRate"]) * 100.0
    return reg


def _lag_autocorr(series, times, lag):
    """Pearson autocorrelation at `lag` periods using only genuinely lag-separated pairs.

    Sorts by t, applies shift(lag), keeps only pairs where t - t_lag == lag (so missing
    quarters are never bridged into non-consecutive observations), and returns NaN if
    fewer than 3 valid pairs remain. One model period = one quarter (lag=1 quarter,
    lag=4 one year).
    """
    s = pd.DataFrame({"x": np.asarray(series, dtype=float),
                      "t": np.asarray(times, dtype=float)}).sort_values("t")
    s["x_lag"] = s["x"].shift(lag)
    s["t_lag"] = s["t"].shift(lag)
    x = s["x"].to_numpy()
    xl = s["x_lag"].to_numpy()
    dt = s["t"].to_numpy() - s["t_lag"].to_numpy()
    m = np.isfinite(x) & np.isfinite(xl) & (dt == lag)
    if m.sum() < 3 or np.std(x[m]) == 0 or np.std(xl[m]) == 0:
        return np.nan
    return float(np.corrcoef(x[m], xl[m])[0, 1])


def _dispersion_and_persistence(reg, col, cv=False, confidence=0.95,
                                disp_name="dispersion",
                                ar1_name="gap_AR1_quarterly",
                                ar4_name="gap_AR4_annual"):
    """Cross-regional dispersion per run + deviation-gap persistence per run.

    Persistence is reported at two horizons (one model period = one quarter):
    AR(1) = quarter-to-quarter, AR(4) = year-over-year (four quarters).
    """
    # Cross-regional dispersion per (run, t), averaged over time within each run.
    grp = reg.groupby(["run", "t"])[col]
    sd_t = grp.std(ddof=1)
    if cv:
        denom = grp.mean().abs().replace(0.0, np.nan)
        disp_t = sd_t / denom
    else:
        disp_t = sd_t
    disp_per_run = disp_t.groupby("run").mean()

    # Deviation from the contemporaneous cross-regional mean; AR via sorted lagged shift.
    reg = reg.sort_values(["run", "region", "t"]).copy()
    reg["_dev"] = reg[col] - reg.groupby(["run", "t"])[col].transform("mean")

    ar1 = reg.groupby(["run", "region"])[["_dev", "t"]].apply(
        lambda s: _lag_autocorr(s["_dev"].to_numpy(), s["t"].to_numpy(), 1)  # lag=1 quarter
    )
    ar4 = reg.groupby(["run", "region"])[["_dev", "t"]].apply(
        lambda s: _lag_autocorr(s["_dev"].to_numpy(), s["t"].to_numpy(), 4)  # lag=4 = one year
    )
    ar1_per_run = ar1.groupby(level="run").mean()
    ar4_per_run = ar4.groupby(level="run").mean()

    rows = {
        disp_name: summarise_across_runs(disp_per_run.values, confidence),
        ar1_name: summarise_across_runs(ar1_per_run.values, confidence),
        ar4_name: summarise_across_runs(ar4_per_run.values, confidence),
    }
    return pd.DataFrame(rows).T[["mean", "std", "n_runs", "ci_low", "ci_high"]]


def regional_productivity_stats(df, metric="Am", tmin=300, tmax=700, confidence=0.95):
    """Regional productivity dispersion (CV) + quarterly AR(1) and annual AR(4) persistence."""
    reg = _regional_frame(df, tmin, tmax)
    col = _resolve_metric_column(reg, metric)
    if col is None:
        raise KeyError(f"Productivity metric {metric!r} not found in regional columns.")
    return _dispersion_and_persistence(
        reg, col, cv=True, confidence=confidence,
        disp_name="productivity_CV",
        ar1_name="productivity_gap_AR1_quarterly",
        ar4_name="productivity_gap_AR4_annual",
    )


def regional_unemployment_stats(df, metric="reg_U", tmin=300, tmax=700, confidence=0.95):
    """Regional unemployment dispersion (SD) + quarterly AR(1) and annual AR(4) persistence."""
    reg = _regional_frame(df, tmin, tmax)
    col = _resolve_metric_column(reg, metric)
    if col is None:
        raise KeyError(
            f"Unemployment metric {metric!r} not found. Available regional columns "
            f"include: {sorted(reg.columns)}"
        )
    return _dispersion_and_persistence(
        reg, col, cv=False, confidence=confidence,
        disp_name="unemployment_regional_SD",
        ar1_name="unemployment_gap_AR1_quarterly",
        ar4_name="unemployment_gap_AR4_annual",
    )


def _filter_gdp_cycle(sub_df, col, tmin, tmax, h=_HAMILTON_H, p=_HAMILTON_P):
    """Hamilton-filter a FULL (run, region) series, then return the cycle on [tmin, tmax].

    Filtering uses the whole available series so the post-transient window is not
    distorted at its left edge. Returns a pandas Series indexed by t, or empty.
    """
    s = sub_df[["t", col]].dropna().drop_duplicates("t").sort_values("t")
    t_full = s["t"].to_numpy()
    if len(t_full) <= h + p:
        return pd.Series(dtype=float)
    cyc = np.asarray(_hamilton_cycle(s[col].to_numpy(), True, h, p))
    cyc_index = t_full[h + p - 1:]  # residual aligns to the last n-h-p+1 periods
    if len(cyc) != len(cyc_index):
        return pd.Series(dtype=float)
    out = pd.Series(cyc, index=cyc_index)
    out = out[np.isfinite(out.to_numpy())]
    return out[(out.index >= tmin) & (out.index <= tmax)]


def regional_business_cycle_stats(df, metric="GDP_r", tmin=300, tmax=700,
                                  confidence=0.95, min_aligned=3, verbose=True):
    """Zero-lag pairwise correlations of Hamilton-filtered regional GDP cycles across MC runs.

    Per run: exclude macro, filter each region's FULL GDP series, keep cycle obs in
    [tmin, tmax], align regions on common timesteps, compute pairwise Pearson
    correlations and their off-diagonal mean. Returns (summary_df, corr_matrix_df).

    Zero-lag (contemporaneous) correlation is the synchronisation statistic. One model
    period = one quarter, so if lagged cross-correlations are added later their lags
    must be read as quarters: lag -4 = region leads by 1 year, lag 0 = contemporaneous,
    lag +4 = region lags by 1 year. A one-period lag is one quarter, not one year.
    """
    reg = df[df["region"] != "macro"].copy()  # full series: do NOT pre-window
    col = _resolve_metric_column(reg, metric)
    if col is None:
        raise KeyError(f"Business-cycle metric {metric!r} not found in regional columns.")

    regions = sorted(reg["region"].unique())
    pairs = [(a, b) for i, a in enumerate(regions) for b in regions[i + 1:]]
    pair_vals = {f"cycle_corr_R{a}_R{b}": [] for a, b in pairs}
    mean_vals = []
    region_obs = {r: [] for r in regions}
    runs_total = runs_used = runs_dropped = 0

    for _, sub in reg.groupby("run"):
        runs_total += 1
        cycles = {}
        for r in regions:
            s = _filter_gdp_cycle(sub[sub["region"] == r], col, tmin, tmax)
            if len(s) > 0:
                cycles[r] = s
                region_obs[r].append(len(s))
        if len(cycles) < 2:
            runs_dropped += 1
            continue

        cyc_df = pd.DataFrame(cycles)  # outer join on common t index
        run_pair_corrs = []
        used_any = False
        for a, b in pairs:
            name = f"cycle_corr_R{a}_R{b}"
            if a in cyc_df.columns and b in cyc_df.columns:
                pair = cyc_df[[a, b]].dropna()  # align on common timesteps
                if len(pair) >= min_aligned and pair[a].std() > 0 and pair[b].std() > 0:
                    corr = float(np.corrcoef(pair[a].to_numpy(), pair[b].to_numpy())[0, 1])
                    used_any = True
                else:
                    corr = np.nan
            else:
                corr = np.nan
            pair_vals[name].append(corr)
            if np.isfinite(corr):
                run_pair_corrs.append(corr)
        if used_any:
            runs_used += 1
            mean_vals.append(np.nanmean(run_pair_corrs) if run_pair_corrs else np.nan)
        else:
            runs_dropped += 1

    rows = {name: summarise_across_runs(vals, confidence) for name, vals in pair_vals.items()}
    rows["mean_regional_cycle_corr"] = summarise_across_runs(mean_vals, confidence)
    summary = pd.DataFrame(rows).T[["mean", "std", "n_runs", "ci_low", "ci_high"]]

    _assert_correlation_range(pair_vals, mean_vals)
    if verbose:
        _report_cycle_diagnostics("regional", runs_total, runs_used, runs_dropped,
                                  region_obs, min_aligned)

    corr_matrix = pd.DataFrame(np.eye(len(regions)), index=regions, columns=regions, dtype=float)
    for a, b in pairs:
        mc_mean = summary.loc[f"cycle_corr_R{a}_R{b}", "mean"]
        corr_matrix.loc[a, b] = mc_mean
        corr_matrix.loc[b, a] = mc_mean
    return summary, corr_matrix


def _lag_crosscorr(cycle_a, cycle_b, lag, min_aligned=3):
    """Cross-correlation corr(cycle_a[t], cycle_b[t - lag]) aligned on the actual quarterly t.

    cycle_a / cycle_b are pandas Series indexed by integer quarter t. The second series
    is shifted by adding `lag` to its index, so alignment uses real quarter values and
    missing periods never create false lag relationships. Lag convention (quarters):
    lag < 0 => a leads b, lag = 0 => contemporaneous, lag > 0 => a lags b. Returns NaN
    if fewer than `min_aligned` aligned pairs (or zero variance).
    """
    b_shift = pd.Series(cycle_b.to_numpy(), index=cycle_b.index + lag)
    joined = pd.concat([cycle_a.rename("a"), b_shift.rename("b")], axis=1).dropna()
    if len(joined) < min_aligned:
        return np.nan
    xa, xb = joined["a"].to_numpy(), joined["b"].to_numpy()
    if np.std(xa) == 0 or np.std(xb) == 0:
        return np.nan
    return float(np.corrcoef(xa, xb)[0, 1])


def regional_business_cycle_crosscorr_stats(df, metric="GDP_r", tmin=300, tmax=700,
                                            max_lag=4, confidence=0.95,
                                            min_aligned=3, verbose=True):
    """Pairwise lead-lag cross-correlations of Hamilton-filtered regional GDP cycles.

    One model period = one quarter. The Hamilton filter is applied to each region's
    FULL GDP series; the cycle is then restricted to [tmin, tmax] before correlating.
    For each run and region pair (listed A_B) the cross-correlation is computed over
    lags k = -max_lag..+max_lag quarters as corr(cycle_A[t], cycle_B[t - k]).

    Lag convention (quarters): k < 0 => A leads B, k = 0 => contemporaneous,
    k > 0 => A lags B. Example: cycle_corr_R1_R2_lag_-4 is R1 leading R2 by four
    quarters (one year). Per run, the per-pair peak is the LARGEST SIGNED correlation
    (positive synchronisation), with its lag recorded.

    Returns (summary_df, lag_df, by_lag_df):
      summary_df  : max_cycle_corr_<pair> + mean_max_regional_cycle_corr (mean/std/n_runs/ci)
      lag_df      : peak-lag stats per pair (mean/median quarters, mean years, n_runs)
      by_lag_df   : MC mean cross-correlation function per (pair, lag)
    """
    reg = df[df["region"] != "macro"].copy()  # full series: do NOT pre-window; no national GDP
    col = _resolve_metric_column(reg, metric)
    if col is None:
        raise KeyError(f"Business-cycle metric {metric!r} not found in regional columns.")

    regions = sorted(reg["region"].unique())
    pairs = [(a, b) for i, a in enumerate(regions) for b in regions[i + 1:]]
    lags = list(range(-max_lag, max_lag + 1))

    by_lag_vals = {(f"R{a}_R{b}", L): [] for (a, b) in pairs for L in lags}
    maxcorr_vals = {f"R{a}_R{b}": [] for a, b in pairs}
    maxlag_vals = {f"R{a}_R{b}": [] for a, b in pairs}
    mean_max_vals = []
    region_obs = {r: [] for r in regions}
    runs_total = runs_used = runs_dropped = 0

    for _, sub in reg.groupby("run"):
        runs_total += 1
        cyc = {}
        for r in regions:
            s = _filter_gdp_cycle(sub[sub["region"] == r], col, tmin, tmax)
            if len(s) > 0:
                cyc[r] = s
                region_obs[r].append(len(s))
        if len(cyc) < 2:
            runs_dropped += 1
            continue

        run_maxes = []
        used_any = False
        for a, b in pairs:
            pname = f"R{a}_R{b}"
            if a not in cyc or b not in cyc:
                for L in lags:
                    by_lag_vals[(pname, L)].append(np.nan)
                maxcorr_vals[pname].append(np.nan)
                maxlag_vals[pname].append(np.nan)
                continue
            ccf = {}
            for L in lags:
                c = _lag_crosscorr(cyc[a], cyc[b], L, min_aligned)
                by_lag_vals[(pname, L)].append(c)
                ccf[L] = c
            finite = {L: v for L, v in ccf.items() if np.isfinite(v)}
            if finite:
                best_L = max(finite, key=lambda L: finite[L])  # largest SIGNED corr
                maxcorr_vals[pname].append(finite[best_L])
                maxlag_vals[pname].append(best_L)
                run_maxes.append(finite[best_L])
                used_any = True
            else:
                maxcorr_vals[pname].append(np.nan)
                maxlag_vals[pname].append(np.nan)
        if used_any:
            runs_used += 1
            mean_max_vals.append(np.nanmean(run_maxes) if run_maxes else np.nan)
        else:
            runs_dropped += 1

    # Peak-correlation summary (magnitudes) + mean peak across the three pairs.
    summary_rows = {}
    for a, b in pairs:
        pname = f"R{a}_R{b}"
        summary_rows[f"max_cycle_corr_{pname}"] = summarise_across_runs(maxcorr_vals[pname], confidence)
    summary_rows["mean_max_regional_cycle_corr"] = summarise_across_runs(mean_max_vals, confidence)
    summary_df = pd.DataFrame(summary_rows).T[["mean", "std", "n_runs", "ci_low", "ci_high"]]

    # Peak-lag distribution per pair (quarters and years).
    lag_rows = {}
    for a, b in pairs:
        pname = f"R{a}_R{b}"
        arr = np.asarray([v for v in maxlag_vals[pname] if np.isfinite(v)], dtype=float)
        mq = float(np.mean(arr)) if arr.size else np.nan
        medq = float(np.median(arr)) if arr.size else np.nan
        lag_rows[f"max_cycle_corr_lag_{pname}"] = {
            "mean_lag_quarters": mq,
            "median_lag_quarters": medq,
            "mean_lag_years": (mq / 4.0) if np.isfinite(mq) else np.nan,
            "n_runs": int(arr.size),
        }
    lag_df = pd.DataFrame(lag_rows).T[["mean_lag_quarters", "median_lag_quarters",
                                       "mean_lag_years", "n_runs"]]

    # Full MC-mean cross-correlation function for plotting / CSV.
    by_lag_records = []
    for a, b in pairs:
        pname = f"R{a}_R{b}"
        for L in lags:
            s = summarise_across_runs(by_lag_vals[(pname, L)], confidence)
            by_lag_records.append({"pair": pname, "lag_quarters": L, "lag_years": L / 4.0, **s})
    by_lag_df = pd.DataFrame.from_records(
        by_lag_records,
        columns=["pair", "lag_quarters", "lag_years", "mean", "std", "n_runs", "ci_low", "ci_high"],
    )

    _assert_correlation_range(by_lag_vals, mean_max_vals)
    if verbose:
        _report_cycle_diagnostics("regional cross-corr", runs_total, runs_used, runs_dropped,
                                  region_obs, min_aligned)
    return summary_df, lag_df, by_lag_df


def plot_regional_cycle_crosscorrelations(by_lag_df, output=None):
    """Plot the MC-mean cross-correlation function (+/-max_lag quarters) per region pair."""
    pairs = list(dict.fromkeys(by_lag_df["pair"].tolist()))
    lags = sorted(by_lag_df["lag_quarters"].unique())

    _fig, ax = plt.subplots(1, 1, figsize=(7, 4.5))
    for pname in pairs:
        sub = by_lag_df[by_lag_df["pair"] == pname].sort_values("lag_quarters")
        line, = ax.plot(sub["lag_quarters"], sub["mean"], marker="o", lw=1.5, label=pname)
        ax.fill_between(sub["lag_quarters"], sub["ci_low"], sub["ci_high"],
                        color=line.get_color(), alpha=0.15)
    ax.axvline(0, color="gray", lw=1.0, ls="--")
    ax.axhline(0, color="lightgray", lw=0.8, ls="--")
    ax.set_xticks(lags)
    ax.set_xlabel("Lag (quarters)")
    ax.set_ylabel("Correlation")
    ax.set_title("Regional GDP-cycle cross-correlations (MC mean, 95% CI)")
    ax.grid(True, color="lightgray", ls="--", lw=0.5)
    ax.spines[["top", "right"]].set_visible(False)
    ax.legend(title="Region pair")
    plt.tight_layout()
    if output is not None:
        plt.savefig(output / "regional_business_cycle_crosscorrelations.png",
                    dpi=300, bbox_inches="tight")
    plt.show()


def _assert_correlation_range(pair_vals, mean_vals, tol=1e-9):
    """Guard: every reported correlation must lie within [-1, 1]."""
    all_corrs = [v for vals in pair_vals.values() for v in vals if np.isfinite(v)]
    all_corrs += [v for v in mean_vals if np.isfinite(v)]
    assert all(-1.0 - tol <= v <= 1.0 + tol for v in all_corrs), \
        "business-cycle correlation outside [-1, 1]"


def _report_cycle_diagnostics(label, runs_total, runs_used, runs_dropped,
                              region_obs, min_aligned, macro_obs=None):
    """Print run usage and filtered-observation counts; flag heavy run loss."""
    print(f"[{label} cycle diagnostics] runs_total={runs_total} "
          f"runs_used={runs_used} runs_dropped={runs_dropped} "
          f"(min_aligned={min_aligned})")
    for r, counts in region_obs.items():
        if counts:
            arr = np.asarray(counts)
            print(f"  region {r}: valid filtered obs/run "
                  f"min={arr.min()} median={int(np.median(arr))} max={arr.max()} "
                  f"(n_runs_with_obs={len(counts)})")
        else:
            print(f"  region {r}: no valid filtered observations")
    if macro_obs is not None and macro_obs:
        arr = np.asarray(macro_obs)
        print(f"  macro: valid filtered obs/run min={arr.min()} "
              f"median={int(np.median(arr))} max={arr.max()}")
    if runs_total > 0 and runs_dropped > 0.25 * runs_total:
        warnings.warn(
            f"{label} cycle: {runs_dropped}/{runs_total} runs dropped for insufficient "
            f"aligned observations; interpret correlations with caution.", stacklevel=2)


def inspect_business_cycle_filter(df, run, regions=(1, 2, 3), tmin=250, tmax=400,
                                  metric="GDP_r"):
    """Diagnostic: plot the Hamilton-filtered regional GDP cycles for one run (not auto-run)."""
    reg = df[df["region"] != "macro"]
    col = _resolve_metric_column(reg, metric)
    if col is None:
        raise KeyError(f"Business-cycle metric {metric!r} not found in regional columns.")
    sub = reg[reg["run"] == run]
    meta = globals().get("region_meta", {})

    _fig, ax = plt.subplots(1, 1, figsize=(8, 4))
    for r in regions:
        s = _filter_gdp_cycle(sub[sub["region"] == r], col, tmin, tmax)
        if len(s) == 0:
            continue
        color = meta.get(r, {}).get("color")
        label = meta.get(r, {}).get("name", f"R{r}")
        ax.plot(s.index, s.to_numpy(), color=color, label=label)
    ax.axhline(0, color="gray", lw=0.8, ls="--")
    ax.set_xlabel("Timestep")
    ax.set_ylabel(f"Hamilton-filtered {metric} cycle")
    ax.set_title(f"Filtered regional {metric} cycles (run {run})")
    ax.grid(True, color="lightgray", ls="--", lw=0.5)
    ax.spines[["top", "right"]].set_visible(False)
    ax.legend()
    plt.tight_layout()
    plt.show()



def regional_shock_persistence_stats(baseline_df, shock_df, metrics,
                                     t_end_window=100, confidence=0.95):
    """Mean shock-baseline gap over the final 25 years (t_end_window=100 quarters).

    One model period = one quarter, so the default final window of 100 quarters = 25
    years. Relative gap for level variables; absolute difference for unemployment. Runs
    are paired by ('seed', 'run') when seed is available in both scenarios, else by
    'run'. Missing/unavailable metrics raise a clear warning (not a silent skip).
    """
    base = baseline_df[baseline_df["region"] != "macro"].copy()
    shock = shock_df[shock_df["region"] != "macro"].copy()
    for frame in (base, shock):
        if "Unemployment" not in frame.columns and "EmploymentRate" in frame.columns:
            frame["Unemployment"] = (1.0 - frame["EmploymentRate"]) * 100.0

    # Pair by seed as well as run when available; always include run.
    pair_keys = [k for k in ("seed", "run")
                 if k in base.columns and k in shock.columns]
    if "run" not in pair_keys:
        pair_keys.append("run")

    common_regions = sorted(set(base["region"]).intersection(shock["region"]))
    base = base[base["region"].isin(common_regions)]
    shock = shock[shock["region"].isin(common_regions)]

    def _final_means(frame, col):
        # Final 25 years: t_end_window quarters (default 100 quarters = 25 years).
        tmax = frame["t"].max()
        win = frame[frame["t"] > (tmax - t_end_window)]
        return win.groupby(pair_keys + ["region"])[col].mean()

    records = []
    for metric in metrics:
        col_b = _resolve_metric_column(base, metric)
        col_s = _resolve_metric_column(shock, metric)
        if col_b is None or col_s is None:
            msg = (f"regional_shock_persistence_stats: metric {metric!r} has no matching "
                   f"regional column; this stylised fact cannot yet be evaluated.")
            if metric == "reg_K":
                msg += (" Regional capital stock is not written to the ymc_reg output "
                        "(schema/ymc_reg.yaml); add it to the model output/schema first.")
            warnings.warn(msg, stacklevel=2)
            continue

        mb = _final_means(base, col_b).rename("_base")
        ms = _final_means(shock, col_s).rename("_shock")
        merged = pd.concat([mb, ms], axis=1, join="inner").dropna()

        is_unemp = metric in _UNEMP_METRICS or col_b == "Unemployment"
        if is_unemp:
            merged["_gap"] = merged["_shock"] - merged["_base"]
            gap_type = "abs_diff"
        else:
            denom = merged["_base"].abs().replace(0.0, np.nan)
            merged["_gap"] = (merged["_shock"] - merged["_base"]) / denom
            gap_type = "rel_gap"

        merged = merged.reset_index()
        for region, grp in merged.groupby("region"):
            s = summarise_across_runs(grp["_gap"].values, confidence)
            records.append({"metric": metric, "region": region, "gap_type": gap_type, **s})

    cols = ["metric", "region", "gap_type", "mean", "std", "n_runs", "ci_low", "ci_high"]
    return pd.DataFrame.from_records(records, columns=cols)


# ---- Example calls for scenario_1 (first three stylised facts) -----------------
# One model period = one quarter. Validation window t = 300..700 is 401 quarterly
# observations ~ 100.25 years. Statistics stay at quarterly frequency (no annual
# rescaling/aggregation); AR(1)=quarter-to-quarter, AR(4)=year-over-year.
_sf_confidence = 0.95
_sf_tmin, _sf_tmax = 300, 700  # 401 quarterly observations ~ 100.25 years

prod_stats = regional_productivity_stats(
    scenario_results["scenario_1"], metric="Am",
    tmin=_sf_tmin, tmax=_sf_tmax, confidence=_sf_confidence,
)
print("\n=== Regional productivity dispersion & persistence ===")
print(prod_stats.round(4).to_string())
prod_stats.to_csv(OUTPUT / "regional_productivity_stats.csv")

unemp_stats = regional_unemployment_stats(
    scenario_results["scenario_1"], metric="reg_U",
    tmin=_sf_tmin, tmax=_sf_tmax, confidence=_sf_confidence,
)
print("\n=== Regional unemployment differentiation ===")
print(unemp_stats.round(4).to_string())
unemp_stats.to_csv(OUTPUT / "regional_unemployment_stats.csv")

cycle_stats, cycle_corr_matrix = regional_business_cycle_stats(
    scenario_results["scenario_1"], metric="GDP_r",
    tmin=_sf_tmin, tmax=_sf_tmax, confidence=_sf_confidence,
)
print("\n=== Regional business-cycle synchronisation: zero lag ===")
print(cycle_stats.round(4).to_string())
print("\nMC-mean pairwise cycle-correlation matrix:")
print(cycle_corr_matrix.round(4).to_string())
cycle_stats.to_csv(OUTPUT / "regional_business_cycle_stats.csv")
cycle_corr_matrix.to_csv(OUTPUT / "regional_business_cycle_corr_matrix.csv")

# Lead-lag cross-correlations over +/-4 quarters (1 model period = 1 quarter).
cc_stats, cc_lag_stats, cc_by_lag = regional_business_cycle_crosscorr_stats(
    scenario_results["scenario_1"], metric="GDP_r",
    tmin=_sf_tmin, tmax=_sf_tmax, max_lag=4, confidence=_sf_confidence,
)
print("\n=== Regional business-cycle synchronisation: max over +/-4 quarters ===")
print(cc_stats.round(4).to_string())
print("\nPeak-lag distribution per pair (quarters / years):")
print(cc_lag_stats.round(4).to_string())
cc_stats.to_csv(OUTPUT / "regional_business_cycle_crosscorr_stats.csv")
cc_by_lag.to_csv(OUTPUT / "regional_business_cycle_crosscorr_by_lag.csv", index=False)
plot_regional_cycle_crosscorrelations(cc_by_lag, output=OUTPUT)

# Optional diagnostic (manual; not auto-run): visually inspect filtered cycles for one run.
# inspect_business_cycle_filter(scenario_results["scenario_1"], run=1, regions=(1, 2, 3),
#                               tmin=250, tmax=400)

# ---- Persistent regional shock effects (manual; no auto scenario assumption) ---
# Identify the regional-shock scenario explicitly, then call e.g.:
#
# shock_persistence = regional_shock_persistence_stats(
#     scenario_results["scenario_1"],   # baseline (explicit)
#     scenario_results["scenario_2"],   # regional shock (explicit — set to the real one)
#     metrics=["GDP_r", "Am", "reg_U", "N1r", "N2r"],  # reg_K unavailable in ymc_reg output
#     t_end_window=100,
#     confidence=0.95,
# )
# print(shock_persistence.round(4).to_string(index=False))
# shock_persistence.to_csv(OUTPUT / "regional_shock_persistence_stats.csv")
# %%
# %% Additional regional stylised facts
# Tests:
# 1. Regional Okun's law
# 2. Volatility hierarchy: Investment > GDP > Consumption
# 3. Consumption and investment procyclicality
# 4. Regional trade home bias
#
# Model frequency: 1 period = 1 quarter.
# Main validation window: t = 300,...,700 = 401 quarters ≈ 100.25 years.

from scipy import stats as scipy_stats

SF_SCENARIO = "scenario_1"
SF_TMIN = 300
SF_TMAX = 700
SF_CONFIDENCE = 0.95

sf = scenario_results[SF_SCENARIO].copy()

# Regional observations only
sf = sf[sf["region"] != "macro"].copy()
sf["region"] = pd.to_numeric(sf["region"], errors="coerce")
sf = sf.dropna(subset=["region"])
sf["region"] = sf["region"].astype(int)

required_cols = [
    "t",
    "run",
    "region",
    "GDP_r",
    "Consumption_r",
    "Investment_r",
    "EmploymentRate",
    "N1r",
    "N2r",
    "cons_buy_from_R1",
    "cons_buy_from_R2",
    "cons_buy_from_R3",
    "mach_buy_from_R1",
    "mach_buy_from_R2",
    "mach_buy_from_R3",
]

missing = [c for c in required_cols if c not in sf.columns]

if missing:
    raise KeyError(
        "Missing columns required for additional regional "
        f"stylised facts: {missing}"
    )


# -------------------------------------------------------------------
# Helpers
# -------------------------------------------------------------------

def _mc_summary(values, confidence=0.95):
    """Monte Carlo summary from one value per run."""
    x = np.asarray(values, dtype=float)
    x = x[np.isfinite(x)]

    n = len(x)

    if n == 0:
        return {
            "mean": np.nan,
            "std": np.nan,
            "n_runs": 0,
            "ci_low": np.nan,
            "ci_high": np.nan,
        }

    mean = np.mean(x)

    if n == 1:
        return {
            "mean": mean,
            "std": np.nan,
            "n_runs": 1,
            "ci_low": np.nan,
            "ci_high": np.nan,
        }

    std = np.std(x, ddof=1)
    sem = std / np.sqrt(n)

    tcrit = scipy_stats.t.ppf(
        0.5 * (1.0 + confidence),
        df=n - 1,
    )

    return {
        "mean": mean,
        "std": std,
        "n_runs": n,
        "ci_low": mean - tcrit * sem,
        "ci_high": mean + tcrit * sem,
    }


def _summarise_run_region(df, variables):
    """
    Summarise run-region statistics.

    For each variable:
      - report each region separately across MC runs
      - average regions within each run, then summarise across runs
    """
    records = []

    for variable in variables:

        for region, group in df.groupby("region"):
            s = _mc_summary(
                group[variable],
                confidence=SF_CONFIDENCE,
            )

            records.append({
                "statistic": variable,
                "region": f"R{region}",
                **s,
            })

        # One statistic per MC run first
        per_run = (
            df.groupby("run")[variable]
            .mean()
            .dropna()
        )

        s = _mc_summary(
            per_run,
            confidence=SF_CONFIDENCE,
        )

        records.append({
            "statistic": variable,
            "region": "mean_regions",
            **s,
        })

    return pd.DataFrame(records)


def _hamilton_cycle_quarterly(
    series,
    log_transform=True,
    h=8,
    p=4,
):
    """
    Hamilton cyclical residual for quarterly data.

    Quarterly specification:
        h = 8 quarters
        p = 4 lags

    Returns residuals corresponding to the final
    len(series) - h - p + 1 observations.
    """
    x = np.asarray(series, dtype=float)

    if not np.all(np.isfinite(x)):
        return np.array([])

    if log_transform:
        if np.any(x <= 0):
            return np.array([])
        x = np.log(x)

    n = len(x)

    if n <= h + p:
        return np.array([])

    t_idx = np.arange(p - 1, n - h)

    y = x[t_idx + h]

    X = np.column_stack(
        [np.ones(len(t_idx))]
        + [
            x[t_idx - lag]
            for lag in range(p)
        ]
    )

    beta, *_ = np.linalg.lstsq(
        X,
        y,
        rcond=None,
    )

    return y - X @ beta


def _filtered_cycle(group, variable):
    """
    Apply quarterly Hamilton filter to the FULL series.

    The output residuals correspond to the final observations
    because the Hamilton target is y_(t+h).
    """
    work = (
        group[["t", variable]]
        .dropna()
        .sort_values("t")
        .copy()
    )

    if len(work) < 20:
        return pd.DataFrame(
            columns=["t", "cycle"]
        )

    # Require genuinely consecutive quarters
    times = work["t"].to_numpy()

    if not np.all(np.diff(times) == 1):
        return pd.DataFrame(
            columns=["t", "cycle"]
        )

    cycle = _hamilton_cycle_quarterly(
        work[variable].to_numpy(),
        log_transform=True,
        h=8,
        p=4,
    )

    if len(cycle) == 0:
        return pd.DataFrame(
            columns=["t", "cycle"]
        )

    # Hamilton residuals correspond to the last len(cycle)
    # quarterly dates of the original series.
    cycle_times = times[-len(cycle):]

    return pd.DataFrame({
        "t": cycle_times,
        "cycle": cycle,
    })


# ===================================================================
# 1. REGIONAL OKUN'S LAW
# ===================================================================

# EmploymentRate may be stored either as 0-1 or 0-100.
employment = pd.to_numeric(
    sf["EmploymentRate"],
    errors="coerce",
)

if employment.median() <= 1.5:
    sf["_U"] = 100.0 * (1.0 - employment)
else:
    sf["_U"] = 100.0 - employment


okun_records = []

for (run, region), group in sf.groupby(
    ["run", "region"]
):

    g = group.sort_values("t").copy()

    gdp = pd.to_numeric(
        g["GDP_r"],
        errors="coerce",
    )

    g["_logGDP"] = np.where(
        gdp > 0,
        np.log(gdp),
        np.nan,
    )

    # Quarter-to-quarter changes
    g["_gdp_growth_q"] = (
        100.0 * g["_logGDP"].diff(1)
    )

    g["_dU_q"] = g["_U"].diff(1)

    # Ensure lag really is one quarter
    valid_q_lag = (
        g["t"] - g["t"].shift(1)
    ) == 1

    g.loc[
        ~valid_q_lag,
        ["_gdp_growth_q", "_dU_q"]
    ] = np.nan

    # Four-quarter, approximately annual changes
    g["_gdp_growth_y"] = (
        100.0
        * (
            g["_logGDP"]
            - g["_logGDP"].shift(4)
        )
    )

    g["_dU_y"] = (
        g["_U"]
        - g["_U"].shift(4)
    )

    valid_y_lag = (
        g["t"] - g["t"].shift(4)
    ) == 4

    g.loc[
        ~valid_y_lag,
        ["_gdp_growth_y", "_dU_y"]
    ] = np.nan

    # Select post-transient validation window AFTER changes are built
    g = g[
        (g["t"] >= SF_TMIN)
        & (g["t"] <= SF_TMAX)
    ]

    record = {
        "run": run,
        "region": region,
    }

    # Quarterly Okun relationship
    q = g[
        ["_gdp_growth_q", "_dU_q"]
    ].dropna()

    if (
        len(q) >= 20
        and q["_gdp_growth_q"].std() > 0
        and q["_dU_q"].std() > 0
    ):
        res = scipy_stats.linregress(
            q["_gdp_growth_q"],
            q["_dU_q"],
        )

        record["okun_beta_quarterly"] = res.slope
        record["okun_corr_quarterly"] = res.rvalue

    else:
        record["okun_beta_quarterly"] = np.nan
        record["okun_corr_quarterly"] = np.nan

    # Annual, four-quarter Okun relationship
    y = g[
        ["_gdp_growth_y", "_dU_y"]
    ].dropna()

    if (
        len(y) >= 20
        and y["_gdp_growth_y"].std() > 0
        and y["_dU_y"].std() > 0
    ):
        res = scipy_stats.linregress(
            y["_gdp_growth_y"],
            y["_dU_y"],
        )

        record["okun_beta_annual"] = res.slope
        record["okun_corr_annual"] = res.rvalue

    else:
        record["okun_beta_annual"] = np.nan
        record["okun_corr_annual"] = np.nan

    okun_records.append(record)


okun_rr = pd.DataFrame(okun_records)

okun_summary = _summarise_run_region(
    okun_rr,
    [
        "okun_beta_quarterly",
        "okun_corr_quarterly",
        "okun_beta_annual",
        "okun_corr_annual",
    ],
)

# Fraction of regions displaying the expected negative Okun coefficient
okun_negative_per_run = (
    okun_rr.assign(
        negative_beta=(
            okun_rr["okun_beta_annual"] < 0
        ).astype(float)
    )
    .groupby("run")["negative_beta"]
    .mean()
)

okun_negative_summary = _mc_summary(
    okun_negative_per_run,
    confidence=SF_CONFIDENCE,
)


print(
    "\n=== Regional Okun's law ==="
)

print(
    okun_summary.round(4).to_string(index=False)
)

print(
    "\nShare of regions per run with negative annual "
    "Okun coefficient:"
)

print(
    pd.Series(
        okun_negative_summary
    ).round(4).to_string()
)


# ===================================================================
# 2 + 3. REGIONAL VOLATILITY HIERARCHY AND PROCYCLICALITY
# ===================================================================

cycle_records = []
cycle_skipped = 0

for (run, region), group in sf.groupby(
    ["run", "region"]
):

    cycles = {}

    for variable in [
        "GDP_r",
        "Consumption_r",
        "Investment_r",
    ]:

        result = _filtered_cycle(
            group,
            variable,
        )

        if result.empty:
            cycles = {}
            break

        result = result.rename(
            columns={
                "cycle": f"{variable}_cycle"
            }
        )

        cycles[variable] = result

    if len(cycles) != 3:
        cycle_skipped += 1
        continue

    merged = cycles["GDP_r"]

    merged = merged.merge(
        cycles["Consumption_r"],
        on="t",
        how="inner",
    )

    merged = merged.merge(
        cycles["Investment_r"],
        on="t",
        how="inner",
    )

    # Filter only after applying Hamilton filter
    merged = merged[
        (merged["t"] >= SF_TMIN)
        & (merged["t"] <= SF_TMAX)
    ].dropna()

    if len(merged) < 20:
        cycle_skipped += 1
        continue

    y = merged["GDP_r_cycle"]
    c = merged["Consumption_r_cycle"]
    i = merged["Investment_r_cycle"]

    sigma_y = y.std(ddof=1)
    sigma_c = c.std(ddof=1)
    sigma_i = i.std(ddof=1)

    if sigma_y <= 0:
        cycle_skipped += 1
        continue

    cycle_records.append({
        "run": run,
        "region": region,

        # Relative cyclical volatility
        "consumption_volatility_ratio":
            sigma_c / sigma_y,

        "investment_volatility_ratio":
            sigma_i / sigma_y,

        # Expected hierarchy:
        # sigma_I > sigma_Y > sigma_C
        "volatility_hierarchy_ok":
            float(
                sigma_i
                > sigma_y
                > sigma_c
            ),

        # Contemporaneous cyclicality
        "corr_consumption_GDP":
            y.corr(c),

        "corr_investment_GDP":
            y.corr(i),
    })


cycle_rr = pd.DataFrame(cycle_records)

volatility_summary = _summarise_run_region(
    cycle_rr,
    [
        "consumption_volatility_ratio",
        "investment_volatility_ratio",
        "volatility_hierarchy_ok",
    ],
)

procyc_summary = _summarise_run_region(
    cycle_rr,
    [
        "corr_consumption_GDP",
        "corr_investment_GDP",
    ],
)


print(
    "\n=== Regional relative volatility ==="
)

print(
    volatility_summary.round(4).to_string(index=False)
)

print(
    "\nInterpretation target:"
    "\n  consumption_volatility_ratio < 1"
    "\n  investment_volatility_ratio > 1"
    "\n  volatility_hierarchy_ok close to 1"
)

print(
    "\n=== Regional consumption and investment procyclicality ==="
)

print(
    procyc_summary.round(4).to_string(index=False)
)

print(
    "\nInterpretation target:"
    "\n  corr_consumption_GDP > 0"
    "\n  corr_investment_GDP > 0"
)

print(
    f"\nCycle run-region groups skipped because "
    f"filtering was not possible: {cycle_skipped}"
)


# ===================================================================
# 4. REGIONAL CONSUMPTION AND MACHINERY HOME BIAS
# ===================================================================

trade = sf[
    (sf["t"] >= SF_TMIN)
    & (sf["t"] <= SF_TMAX)
].copy()

cons_cols = [
    "cons_buy_from_R1",
    "cons_buy_from_R2",
    "cons_buy_from_R3",
]

mach_cols = [
    "mach_buy_from_R1",
    "mach_buy_from_R2",
    "mach_buy_from_R3",
]

for col in cons_cols + mach_cols + ["N1r", "N2r"]:
    trade[col] = pd.to_numeric(
        trade[col],
        errors="coerce",
    )


# Total purchases by each buyer region
trade["_cons_total"] = (
    trade[cons_cols]
    .sum(axis=1, min_count=1)
)

trade["_mach_total"] = (
    trade[mach_cols]
    .sum(axis=1, min_count=1)
)


# Extract local purchases dynamically
trade["_cons_local"] = np.nan
trade["_mach_local"] = np.nan

for region in sorted(
    trade["region"].unique()
):

    cons_col = f"cons_buy_from_R{region}"
    mach_col = f"mach_buy_from_R{region}"

    mask = trade["region"] == region

    if cons_col in trade.columns:
        trade.loc[
            mask,
            "_cons_local",
        ] = trade.loc[
            mask,
            cons_col,
        ]

    if mach_col in trade.columns:
        trade.loc[
            mask,
            "_mach_local",
        ] = trade.loc[
            mask,
            mach_col,
        ]


# Local purchase shares
trade["cons_local_share"] = np.where(
    trade["_cons_total"] > 0,
    trade["_cons_local"]
    / trade["_cons_total"],
    np.nan,
)

trade["mach_local_share"] = np.where(
    trade["_mach_total"] > 0,
    trade["_mach_local"]
    / trade["_mach_total"],
    np.nan,
)


# ---------------------------------------------------------------
# Size-adjusted home-bias indices
#
# Consumption benchmark:
# local C-firm share = N2r / total N2
#
# Machinery benchmark:
# local K-firm share = N1r / total N1
#
# Index > 1 indicates local purchases exceed the local region's
# share of potential supplier firms.
# ---------------------------------------------------------------

total_N2 = (
    trade.groupby(
        ["run", "t"]
    )["N2r"]
    .transform("sum")
)

total_N1 = (
    trade.groupby(
        ["run", "t"]
    )["N1r"]
    .transform("sum")
)

trade["_local_C_firm_share"] = np.where(
    total_N2 > 0,
    trade["N2r"] / total_N2,
    np.nan,
)

trade["_local_K_firm_share"] = np.where(
    total_N1 > 0,
    trade["N1r"] / total_N1,
    np.nan,
)


trade["cons_home_bias_index"] = np.where(
    trade["_local_C_firm_share"] > 0,
    trade["cons_local_share"]
    / trade["_local_C_firm_share"],
    np.nan,
)

trade["mach_home_bias_index"] = np.where(
    trade["_local_K_firm_share"] > 0,
    trade["mach_local_share"]
    / trade["_local_K_firm_share"],
    np.nan,
)


# Time-average within each run and region first
home_rr = (
    trade.groupby(
        ["run", "region"]
    )[
        [
            "cons_local_share",
            "mach_local_share",
            "cons_home_bias_index",
            "mach_home_bias_index",
        ]
    ]
    .mean()
    .reset_index()
)


home_summary = _summarise_run_region(
    home_rr,
    [
        "cons_local_share",
        "mach_local_share",
        "cons_home_bias_index",
        "mach_home_bias_index",
    ],
)


print(
    "\n=== Regional trade home bias ==="
)

print(
    home_summary.round(4).to_string(index=False)
)

print(
    "\nInterpretation:"
    "\n  local_share gives the fraction purchased locally"
    "\n  home_bias_index > 1 means local purchases exceed "
    "the local supplier-firm share"
)


# Optional consistency check against existing model import-share outputs
if "cons_import_share" in trade.columns:

    cons_check = trade[
        ["cons_import_share", "cons_local_share"]
    ].dropna()

    if not cons_check.empty:
        cons_error = np.max(
            np.abs(
                cons_check["cons_import_share"]
                - (
                    1.0
                    - cons_check["cons_local_share"]
                )
            )
        )

        print(
            "\nConsumption bilateral-flow/import-share "
            f"max discrepancy: {cons_error:.3e}"
        )


if "mach_import_share" in trade.columns:

    mach_check = trade[
        ["mach_import_share", "mach_local_share"]
    ].dropna()

    if not mach_check.empty:
        mach_error = np.max(
            np.abs(
                mach_check["mach_import_share"]
                - (
                    1.0
                    - mach_check["mach_local_share"]
                )
            )
        )

        print(
            "Machinery bilateral-flow/import-share "
            f"max discrepancy: {mach_error:.3e}"
        )


# ===================================================================
# SAVE RESULTS
# ===================================================================

okun_summary.to_csv(
    OUTPUT / "regional_okun_stats.csv",
    index=False,
)

volatility_summary.to_csv(
    OUTPUT / "regional_volatility_stats.csv",
    index=False,
)

procyc_summary.to_csv(
    OUTPUT / "regional_procyclicality_stats.csv",
    index=False,
)

home_summary.to_csv(
    OUTPUT / "regional_home_bias_stats.csv",
    index=False,
)

# Run-region raw statistics are useful for boxplots / robustness checks
okun_rr.to_csv(
    OUTPUT / "regional_okun_run_region.csv",
    index=False,
)

cycle_rr.to_csv(
    OUTPUT / "regional_business_cycle_run_region.csv",
    index=False,
)

home_rr.to_csv(
    OUTPUT / "regional_home_bias_run_region.csv",
    index=False,
)

# %%# %% Diagnose and robustly recompute regional business-cycle facts
#
# Motivation:
# Regional investment can be zero in some quarters because investment is lumpy.
# A log-Hamilton filter therefore drops those run-region series.
#
# This block:
#   1. diagnoses non-positive / missing values
#   2. applies the Hamilton filter in LEVELS to GDP, consumption, investment
#   3. normalises each cycle by its run-region mean level
#   4. recomputes relative volatility and procyclicality
#
# 1 model period = 1 quarter.

ROBUST_TMIN = 300
ROBUST_TMAX = 700
ROBUST_CONFIDENCE = 0.95

df_bc = scenario_results["scenario_1"].copy()

# Regional rows only
df_bc = df_bc[df_bc["region"] != "macro"].copy()

df_bc["region"] = pd.to_numeric(
    df_bc["region"],
    errors="coerce",
)

df_bc = df_bc.dropna(subset=["region"])
df_bc["region"] = df_bc["region"].astype(int)


# ================================================================
# 1. DIAGNOSE WHY LOG FILTERING FAILS
# ================================================================

diagnostic_records = []

for (run, region), g in df_bc.groupby(["run", "region"]):

    g = g.sort_values("t")

    rec = {
        "run": run,
        "region": region,
        "n_obs": len(g),
    }

    for var in [
        "GDP_r",
        "Consumption_r",
        "Investment_r",
    ]:

        x = pd.to_numeric(
            g[var],
            errors="coerce",
        )

        rec[f"{var}_min"] = x.min()
        rec[f"{var}_n_missing"] = x.isna().sum()
        rec[f"{var}_n_zero"] = (x == 0).sum()
        rec[f"{var}_n_negative"] = (x < 0).sum()
        rec[f"{var}_n_nonpositive"] = (x <= 0).sum()

    diagnostic_records.append(rec)


bc_diagnostics = pd.DataFrame(
    diagnostic_records
)


print(
    "\n=== Regional business-cycle filtering diagnostics ==="
)

print(
    "\nRun-region groups with non-positive investment:"
)

print(
    (
        bc_diagnostics[
            "Investment_r_n_nonpositive"
        ] > 0
    ).value_counts()
)


print(
    "\nDistribution of non-positive investment quarters "
    "per run-region:"
)

print(
    bc_diagnostics[
        "Investment_r_n_nonpositive"
    ].describe()
)


print(
    "\nRun-region groups with non-positive GDP:"
)

print(
    (
        bc_diagnostics[
            "GDP_r_n_nonpositive"
        ] > 0
    ).value_counts()
)


print(
    "\nRun-region groups with non-positive consumption:"
)

print(
    (
        bc_diagnostics[
            "Consumption_r_n_nonpositive"
        ] > 0
    ).value_counts()
)


bc_diagnostics.to_csv(
    OUTPUT / "regional_business_cycle_filter_diagnostics.csv",
    index=False,
)


# ================================================================
# 2. LEVEL HAMILTON FILTER WITH RELATIVE NORMALISATION
# ================================================================

def hamilton_relative_cycle(
    group,
    variable,
    h=8,
    p=4,
):
    """
    Hamilton cyclical residual for quarterly data in LEVELS.

    The residual is divided by the mean level of the variable
    within the run-region series:

        relative_cycle_t = residual_t / mean(X)

    This produces a dimensionless cyclical measure and avoids
    requiring X > 0 in every quarter.

    h = 8 quarters
    p = 4 quarterly lags
    """

    work = (
        group[["t", variable]]
        .copy()
        .sort_values("t")
    )

    work[variable] = pd.to_numeric(
        work[variable],
        errors="coerce",
    )

    work = work.dropna()

    if len(work) <= h + p:
        return pd.DataFrame(
            columns=["t", "cycle"]
        )

    times = work["t"].to_numpy()
    x = work[variable].to_numpy(dtype=float)

    # Require genuinely consecutive quarters
    if not np.all(np.diff(times) == 1):
        return pd.DataFrame(
            columns=["t", "cycle"]
        )

    n = len(x)

    t_idx = np.arange(
        p - 1,
        n - h,
    )

    y = x[t_idx + h]

    X = np.column_stack(
        [np.ones(len(t_idx))]
        + [
            x[t_idx - lag]
            for lag in range(p)
        ]
    )

    beta, *_ = np.linalg.lstsq(
        X,
        y,
        rcond=None,
    )

    residual = y - X @ beta

    # Normalise by average level to obtain a dimensionless cycle
    scale = np.mean(np.abs(x))

    if (
        not np.isfinite(scale)
        or scale <= 1e-12
    ):
        return pd.DataFrame(
            columns=["t", "cycle"]
        )

    relative_cycle = residual / scale

    # Hamilton residual corresponds to y = x[t+h]
    cycle_times = times[t_idx + h]

    return pd.DataFrame({
        "t": cycle_times,
        "cycle": relative_cycle,
    })


# ================================================================
# 3. RECOMPUTE VOLATILITY + PROCYCLICALITY
# ================================================================

robust_records = []

groups_total = 0
groups_used = 0
groups_dropped = 0

for (run, region), g in df_bc.groupby(
    ["run", "region"]
):

    groups_total += 1

    cycles = {}

    failed = False

    for variable in [
        "GDP_r",
        "Consumption_r",
        "Investment_r",
    ]:

        c = hamilton_relative_cycle(
            g,
            variable,
        )

        if c.empty:
            failed = True
            break

        c = c.rename(
            columns={
                "cycle":
                    f"{variable}_cycle"
            }
        )

        cycles[variable] = c

    if failed:
        groups_dropped += 1
        continue


    merged = (
        cycles["GDP_r"]
        .merge(
            cycles["Consumption_r"],
            on="t",
            how="inner",
        )
        .merge(
            cycles["Investment_r"],
            on="t",
            how="inner",
        )
    )


    # Select validation window AFTER filtering
    merged = merged[
        (merged["t"] >= ROBUST_TMIN)
        & (merged["t"] <= ROBUST_TMAX)
    ].dropna()


    if len(merged) < 20:
        groups_dropped += 1
        continue


    y = merged["GDP_r_cycle"]
    c = merged["Consumption_r_cycle"]
    i = merged["Investment_r_cycle"]

    sigma_y = y.std(ddof=1)
    sigma_c = c.std(ddof=1)
    sigma_i = i.std(ddof=1)

    if (
        not np.isfinite(sigma_y)
        or sigma_y <= 0
    ):
        groups_dropped += 1
        continue


    robust_records.append({

        "run": run,
        "region": region,
        "n_cycle_obs": len(merged),

        # Relative volatility
        "consumption_to_GDP_vol":
            sigma_c / sigma_y,

        "investment_to_GDP_vol":
            sigma_i / sigma_y,

        # Individual hierarchy tests
        "consumption_less_volatile_than_GDP":
            float(sigma_c < sigma_y),

        "investment_more_volatile_than_GDP":
            float(sigma_i > sigma_y),

        # Full hierarchy
        "volatility_hierarchy_I_gt_Y_gt_C":
            float(
                sigma_i
                > sigma_y
                > sigma_c
            ),

        # Procyclicality
        "corr_consumption_GDP":
            y.corr(c),

        "corr_investment_GDP":
            y.corr(i),
    })

    groups_used += 1


robust_bc = pd.DataFrame(
    robust_records
)


print(
    "\n=== Robust regional business-cycle diagnostics ==="
)

print(
    f"run-region groups total   = {groups_total}"
)

print(
    f"run-region groups used    = {groups_used}"
)

print(
    f"run-region groups dropped = {groups_dropped}"
)


# ================================================================
# 4. MONTE CARLO SUMMARY
# ================================================================

def robust_summary_by_region(
    df,
    metrics,
    confidence=0.95,
):

    records = []

    for metric in metrics:

        # Region-specific results
        for region, g in df.groupby("region"):

            s = _mc_summary(
                g[metric],
                confidence=confidence,
            )

            records.append({
                "statistic": metric,
                "region": f"R{region}",
                **s,
            })


        # First average regions within each run
        per_run = (
            df.groupby("run")[metric]
            .mean()
            .dropna()
        )

        s = _mc_summary(
            per_run,
            confidence=confidence,
        )

        records.append({
            "statistic": metric,
            "region": "mean_regions",
            **s,
        })

    return pd.DataFrame(records)


vol_metrics = [
    "consumption_to_GDP_vol",
    "investment_to_GDP_vol",
    "consumption_less_volatile_than_GDP",
    "investment_more_volatile_than_GDP",
    "volatility_hierarchy_I_gt_Y_gt_C",
]

procyc_metrics = [
    "corr_consumption_GDP",
    "corr_investment_GDP",
]


robust_volatility = robust_summary_by_region(
    robust_bc,
    vol_metrics,
    confidence=ROBUST_CONFIDENCE,
)

robust_procyclicality = robust_summary_by_region(
    robust_bc,
    procyc_metrics,
    confidence=ROBUST_CONFIDENCE,
)


# ================================================================
# 5. PRINT RESULTS
# ================================================================

print(
    "\n=== Regional relative volatility "
    "(level-Hamilton robustness test) ==="
)

print(
    robust_volatility
    .round(4)
    .to_string(index=False)
)


print(
    "\nInterpretation targets:"
)

print(
    "  consumption_to_GDP_vol < 1"
)

print(
    "  investment_to_GDP_vol > 1"
)

print(
    "  volatility_hierarchy_I_gt_Y_gt_C "
    "close to 1"
)


print(
    "\n=== Regional consumption and investment "
    "procyclicality "
    "(level-Hamilton robustness test) ==="
)

print(
    robust_procyclicality
    .round(4)
    .to_string(index=False)
)


print(
    "\nInterpretation targets:"
)

print(
    "  corr_consumption_GDP > 0"
)

print(
    "  corr_investment_GDP > 0"
)


# ================================================================
# 6. SAVE
# ================================================================

robust_bc.to_csv(
    OUTPUT / "regional_business_cycle_run_region_robust.csv",
    index=False,
)

robust_volatility.to_csv(
    OUTPUT / "regional_volatility_stats_robust.csv",
    index=False,
)

robust_procyclicality.to_csv(
    OUTPUT / "regional_procyclicality_stats_robust.csv",
    index=False,
)

# %%