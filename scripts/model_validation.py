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
# %%
