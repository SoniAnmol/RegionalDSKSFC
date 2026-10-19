# %% Regional stylised-fact validation
"""
Regional DSK-SFC stylised-fact validation.

Baseline scenario only.

Facts tested
------------
1. Persistent regional productivity heterogeneity
2. Persistent regional unemployment differentiation
3. Persistent regional wage differentials
4. Regional wage-productivity association
5. Regional Okun's law
6. Regional business-cycle synchronisation
7. Regional consumption and investment procyclicality
8. Relative volatility of consumption and investment
9. Regional consumption and machinery home bias

Frequency
---------
1 model period = 1 quarter.

Main validation window:
t = 300,...,700
= 401 quarters
≈ 100.25 years

Methodological conventions
---------------------------
- Statistics are computed at run level first.
- Monte Carlo means / SD / 95% CI are then calculated across runs.
- Time observations are not treated as independent MC observations.
- Hamilton filters are applied to the FULL series before selecting t=300..700.
- Regional GDP synchronisation uses a quarterly log-Hamilton filter.
- Regional C/I/Y volatility uses a level-Hamilton residual normalised
  by the series mean because regional investment can be zero.
"""

from scripts.read_batch_runs import load_simulation_results_matrix
import scripts.read_batch_runs as _rbr
from pathlib import Path
import importlib
import sys
import warnings

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from scipy import stats

# %% CONFIGURATION


PROJECT_ROOT = Path.cwd()

if not (PROJECT_ROOT / "scripts").exists():
    PROJECT_ROOT = PROJECT_ROOT.parent

if str(PROJECT_ROOT) not in sys.path:
    sys.path.append(str(PROJECT_ROOT))


importlib.reload(_rbr)


DATA_ROOT = Path("/Users/anmolsoni/Nextcloud/projects/Regionalised-SFC-DSK/stylised_facts")

OUTPUT = Path(
    "/Users/anmolsoni/Nextcloud/projects/" "Regionalised-SFC-DSK/figures/" "stylised_facts/regional_validation"
)

OUTPUT.mkdir(
    parents=True,
    exist_ok=True,
)


SCENARIO = "scenario_1"

TMIN = 300
TMAX = 700

CONFIDENCE = 0.95

HAMILTON_H = 8
HAMILTON_P = 4

MAX_CYCLE_LAG = 4


REGION_META = {
    1: {
        "name": "North",
        "color": "#5665CC",
    },
    2: {
        "name": "Central",
        "color": "#EC4B37",
    },
    3: {
        "name": "South",
        "color": "#03C58E",
    },
}


# %% LOAD DATA


def scenario_key(value):

    value = str(value)

    if value.startswith("scenario_"):
        return value

    try:
        return f"scenario_{int(float(value))}"

    except (TypeError, ValueError):
        return value


sim_matrix = load_simulation_results_matrix(DATA_ROOT)


scenario_results = {}

for scenario_number in sim_matrix["scenario"].dropna().unique():
    key = scenario_key(scenario_number)

    scenario_results[key] = sim_matrix.loc[sim_matrix["scenario"] == scenario_number].copy()


if SCENARIO not in scenario_results:
    raise KeyError(f"{SCENARIO!r} not found. Available scenarios: {sorted(scenario_results)}")


df = scenario_results[SCENARIO].copy()


# Keep regional rows only

df = df[df["region"] != "macro"].copy()


df["region"] = pd.to_numeric(
    df["region"],
    errors="coerce",
)

df = df.dropna(subset=["region"])

df["region"] = df["region"].astype(int)


# Derive unemployment

employment = pd.to_numeric(
    df["EmploymentRate"],
    errors="coerce",
)

if employment.median() <= 1.5:
    df["Unemployment"] = 100.0 * (1.0 - employment)

else:
    df["Unemployment"] = 100.0 - employment


df = df.sort_values(["run", "region", "t"]).reset_index(drop=True)


REGIONS = sorted(df["region"].unique())


# Required output columns

REQUIRED = [
    "GDP_r",
    "Consumption_r",
    "Investment_r",
    "EmploymentRate",
    "Unemployment",
    "Am",
    "reg_w",
    "N1r",
    "N2r",
]

for region in REGIONS:
    REQUIRED += [
        f"cons_buy_from_R{region}",
        f"mach_buy_from_R{region}",
    ]


missing = [col for col in REQUIRED if col not in df.columns]

if missing:
    raise KeyError(f"Missing regional validation columns: {missing}")


# GENERIC HELPERS


def mc_summary(
    values,
    confidence=CONFIDENCE,
):
    """
    Monte Carlo summary.

    Input must contain one statistic per independent run.
    """

    x = np.asarray(
        list(values),
        dtype=float,
    )

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

    std = np.std(
        x,
        ddof=1,
    )

    sem = std / np.sqrt(n)

    tcrit = stats.t.ppf(
        0.5 * (1 + confidence),
        df=n - 1,
    )

    return {
        "mean": mean,
        "std": std,
        "n_runs": n,
        "ci_low": mean - tcrit * sem,
        "ci_high": mean + tcrit * sem,
    }


def summarise_run_region(
    run_region,
    metrics,
):
    """
    Regional and region-average MC summaries.

    mean_regions first averages regions within each run,
    then calculates MC statistics across runs.
    """

    records = []

    for metric in metrics:
        for region, g in run_region.groupby("region"):
            records.append(
                {
                    "statistic": metric,
                    "region": f"R{region}",
                    **mc_summary(g[metric]),
                }
            )

        per_run = run_region.groupby("run")[metric].mean().dropna()

        records.append(
            {
                "statistic": metric,
                "region": "mean_regions",
                **mc_summary(per_run),
            }
        )

    return pd.DataFrame(records)


def savefig(
    fig,
    name,
):

    fig.savefig(
        OUTPUT / f"{name}.png",
        dpi=300,
        bbox_inches="tight",
    )

    fig.savefig(
        OUTPUT / f"{name}.pdf",
        bbox_inches="tight",
    )


def clean_axis(
    ax,
):

    ax.grid(
        True,
        color="lightgray",
        linestyle="--",
        linewidth=0.5,
    )

    ax.spines[
        [
            "top",
            "right",
            "left",
            "bottom",
        ]
    ].set_visible(False)


def lag_autocorr(
    x,
    t,
    lag,
):
    """
    AR(lag) using genuinely lag-separated quarters.

    lag = 1 -> one quarter
    lag = 4 -> one year
    """

    work = pd.DataFrame(
        {
            "x": x,
            "t": t,
        }
    ).sort_values("t")

    work["x_lag"] = work["x"].shift(lag)

    work["t_lag"] = work["t"].shift(lag)

    valid = work["x"].notna() & work["x_lag"].notna() & (work["t"] - work["t_lag"] == lag)

    pair = work.loc[
        valid,
        ["x", "x_lag"],
    ]

    if len(pair) < 3 or pair["x"].std() == 0 or pair["x_lag"].std() == 0:
        return np.nan

    return pair["x"].corr(pair["x_lag"])


# %% PRODUCTIVITY, UNEMPLOYMENT AND WAGE DIFFERENTIATION


def dispersion_persistence(
    data,
    metric,
    prefix,
    dispersion="cv",
):

    work = (
        data[(data["t"] >= TMIN) & (data["t"] <= TMAX)][
            [
                "run",
                "region",
                "t",
                metric,
            ]
        ]
        .dropna()
        .copy()
    )

    # Cross-sectional regional dispersion at each run,t

    grouped = work.groupby(["run", "t"])[metric]

    sd = grouped.std(ddof=1)

    if dispersion == "cv":
        denominator = (
            grouped.mean()
            .abs()
            .replace(
                0,
                np.nan,
            )
        )

        dispersion_t = sd / denominator

        dispersion_name = f"{prefix}_CV"

    elif dispersion == "sd":
        dispersion_t = sd

        dispersion_name = f"{prefix}_regional_SD"

    else:
        raise ValueError("dispersion must be 'cv' or 'sd'")

    dispersion_run = dispersion_t.groupby("run").mean()

    # Regional gaps from contemporaneous cross-region mean

    work["_gap"] = work[metric] - work.groupby(["run", "t"])[metric].transform("mean")

    persistence = []

    for (
        run,
        region,
    ), g in work.groupby(["run", "region"]):
        persistence.append(
            {
                "run": run,
                "region": region,
                "AR1": lag_autocorr(
                    g["_gap"],
                    g["t"],
                    lag=1,
                ),
                "AR4": lag_autocorr(
                    g["_gap"],
                    g["t"],
                    lag=4,
                ),
            }
        )

    persistence = pd.DataFrame(persistence)

    ar1_run = persistence.groupby("run")["AR1"].mean()

    ar4_run = persistence.groupby("run")["AR4"].mean()

    rows = {
        dispersion_name: mc_summary(dispersion_run),
        f"{prefix}_gap_AR1_quarterly": mc_summary(ar1_run),
        f"{prefix}_gap_AR4_annual": mc_summary(ar4_run),
    }

    return pd.DataFrame(rows).T[
        [
            "mean",
            "std",
            "n_runs",
            "ci_low",
            "ci_high",
        ]
    ]


productivity_stats = dispersion_persistence(
    df,
    "Am",
    "productivity",
    dispersion="cv",
)


unemployment_stats = dispersion_persistence(
    df,
    "Unemployment",
    "unemployment",
    dispersion="sd",
)


wage_stats = dispersion_persistence(
    df,
    "reg_w",
    "wage",
    dispersion="cv",
)


# %% WAGE-PRODUCTIVITY ASSOCIATION


def wage_productivity_stats(
    data,
):

    work = (
        data[(data["t"] >= TMIN) & (data["t"] <= TMAX)][
            [
                "run",
                "region",
                "t",
                "reg_w",
                "Am",
            ]
        ]
        .dropna()
        .copy()
    )

    # Remove common national/time trend.
    # We are asking whether relatively productive regions
    # also have relatively high wages.

    work["w_gap"] = work["reg_w"] - work.groupby(["run", "t"])["reg_w"].transform("mean")

    work["A_gap"] = work["Am"] - work.groupby(["run", "t"])["Am"].transform("mean")

    records = []

    for run, g in work.groupby("run"):
        if g["w_gap"].std() > 0 and g["A_gap"].std() > 0:
            corr = g["w_gap"].corr(g["A_gap"])

        else:
            corr = np.nan

        records.append(
            {
                "run": run,
                "wage_productivity_gap_corr": corr,
            }
        )

    runs = pd.DataFrame(records)

    summary = pd.DataFrame({"wage_productivity_gap_corr": mc_summary(runs["wage_productivity_gap_corr"])}).T

    return (
        runs,
        summary,
    )


(
    wage_productivity_runs,
    wage_productivity_summary,
) = wage_productivity_stats(df)


# FIGURE: REGIONAL PRODUCTIVITY / UNEMPLOYMENT / WAGE GAPS


def regional_gap_series(
    data,
    metric,
    relative=False,
):

    work = data[(data["t"] >= TMIN) & (data["t"] <= TMAX)][
        [
            "run",
            "region",
            "t",
            metric,
        ]
    ].copy()

    mean_rt = work.groupby(["run", "t"])[metric].transform("mean")

    if relative:
        work["gap"] = 100 * (work[metric] / mean_rt - 1)

    else:
        work["gap"] = work[metric] - mean_rt

    records = []

    for (
        t,
        region,
    ), g in work.groupby(["t", "region"]):
        s = mc_summary(g["gap"])

        records.append(
            {
                "t": t,
                "region": region,
                **s,
            }
        )

    return pd.DataFrame(records)


prod_gap_plot = regional_gap_series(
    df,
    "Am",
    relative=True,
)


unemp_gap_plot = regional_gap_series(
    df,
    "Unemployment",
    relative=False,
)


wage_gap_plot = regional_gap_series(
    df,
    "reg_w",
    relative=True,
)


fig, axes = plt.subplots(
    1,
    3,
    figsize=(13.5, 3.8),
    sharex=True,
)


plot_specs = [
    (
        prod_gap_plot,
        "Productivity gap (%)",
    ),
    (
        unemp_gap_plot,
        "Unemployment gap (pp)",
    ),
    (
        wage_gap_plot,
        "Wage gap (%)",
    ),
]


for ax, (
    pdata,
    ylabel,
) in zip(
    axes,
    plot_specs,
):
    for region in REGIONS:
        sub = pdata[pdata["region"] == region]

        meta = REGION_META[region]

        ax.plot(
            sub["t"],
            sub["mean"],
            color=meta["color"],
            label=meta["name"],
            linewidth=1.5,
        )

        ax.fill_between(
            sub["t"],
            sub["ci_low"],
            sub["ci_high"],
            color=meta["color"],
            alpha=0.15,
        )

    ax.axhline(
        0,
        color="gray",
        linewidth=0.8,
        linestyle="--",
    )

    ax.set_xlabel("Quarter")

    ax.set_ylabel(ylabel)

    clean_axis(ax)


axes[0].legend(
    frameon=False,
    ncols=3,
)


fig.tight_layout()


savefig(
    fig,
    "regional_persistent_gaps",
)


plt.close(fig)


# %% REGIONAL OKUN'S LAW


def compute_okun(
    data,
):

    records = []

    for (
        run,
        region,
    ), g in data.groupby(["run", "region"]):
        g = g.sort_values("t").copy()

        g["log_GDP"] = np.where(
            g["GDP_r"] > 0,
            np.log(g["GDP_r"]),
            np.nan,
        )

        # Quarter-to-quarter

        g["GDP_growth_q"] = 100 * g["log_GDP"].diff(1)

        g["dU_q"] = g["Unemployment"].diff(1)

        valid_q = g["t"] - g["t"].shift(1) == 1

        g.loc[
            ~valid_q,
            [
                "GDP_growth_q",
                "dU_q",
            ],
        ] = np.nan

        # Four-quarter change

        g["GDP_growth_y"] = 100 * (g["log_GDP"] - g["log_GDP"].shift(4))

        g["dU_y"] = g["Unemployment"] - g["Unemployment"].shift(4)

        valid_y = g["t"] - g["t"].shift(4) == 4

        g.loc[
            ~valid_y,
            [
                "GDP_growth_y",
                "dU_y",
            ],
        ] = np.nan

        g = g[(g["t"] >= TMIN) & (g["t"] <= TMAX)]

        row = {
            "run": run,
            "region": region,
        }

        for (
            name,
            gx,
            ux,
        ) in [
            (
                "quarterly",
                "GDP_growth_q",
                "dU_q",
            ),
            (
                "annual",
                "GDP_growth_y",
                "dU_y",
            ),
        ]:
            pair = g[[gx, ux]].dropna()

            if len(pair) >= 20 and pair[gx].std() > 0 and pair[ux].std() > 0:
                result = stats.linregress(
                    pair[gx],
                    pair[ux],
                )

                row[f"okun_beta_{name}"] = result.slope

                row[f"okun_corr_{name}"] = result.rvalue

            else:
                row[f"okun_beta_{name}"] = np.nan

                row[f"okun_corr_{name}"] = np.nan

        records.append(row)

    run_region = pd.DataFrame(records)

    summary = summarise_run_region(
        run_region,
        [
            "okun_beta_quarterly",
            "okun_corr_quarterly",
            "okun_beta_annual",
            "okun_corr_annual",
        ],
    )

    return (
        run_region,
        summary,
    )


(
    okun_runs,
    okun_summary,
) = compute_okun(df)


# Okun figure

fig, ax = plt.subplots(figsize=(6, 4))


for i, region in enumerate(REGIONS):
    values = okun_runs.loc[
        okun_runs["region"] == region,
        "okun_beta_annual",
    ]

    s = mc_summary(values)

    ax.errorbar(
        i,
        s["mean"],
        yerr=[
            [s["mean"] - s["ci_low"]],
            [s["ci_high"] - s["mean"]],
        ],
        fmt="o",
        capsize=4,
        color=REGION_META[region]["color"],
    )


ax.axhline(
    0,
    color="gray",
    linestyle="--",
)


ax.set_xticks(range(len(REGIONS)))


ax.set_xticklabels([REGION_META[r]["name"] for r in REGIONS])


ax.set_ylabel("Annual Okun coefficient")


clean_axis(ax)


fig.tight_layout()


savefig(
    fig,
    "regional_okun_coefficients",
)


plt.close(fig)


# %% Okun scatter with fitted regression line (annual specification)


def okun_scatter_data(data):
    """
    Pooled (run, region, t) annual GDP growth and annual unemployment
    change, restricted to TMIN..TMAX.
    """

    records = []

    for (
        run,
        region,
    ), g in data.groupby(["run", "region"]):
        g = g.sort_values("t").copy()

        g["log_GDP"] = np.where(
            g["GDP_r"] > 0,
            np.log(g["GDP_r"]),
            np.nan,
        )

        g["GDP_growth_y"] = 100 * (g["log_GDP"] - g["log_GDP"].shift(4))

        g["dU_y"] = g["Unemployment"] - g["Unemployment"].shift(4)

        valid_y = g["t"] - g["t"].shift(4) == 4

        g.loc[
            ~valid_y,
            [
                "GDP_growth_y",
                "dU_y",
            ],
        ] = np.nan

        g = g[(g["t"] >= TMIN) & (g["t"] <= TMAX)]

        pair = g[["GDP_growth_y", "dU_y"]].dropna()

        pair = pair.assign(region=region)

        records.append(pair)

    return pd.concat(records, ignore_index=True)


okun_scatter = okun_scatter_data(df)


fig, axes = plt.subplots(
    1,
    len(REGIONS),
    figsize=(4.5 * len(REGIONS), 4),
    sharex=True,
    sharey=True,
)


for ax, region in zip(axes, REGIONS):
    sub = okun_scatter[okun_scatter["region"] == region]

    x = sub["GDP_growth_y"].to_numpy()

    y = sub["dU_y"].to_numpy()

    meta = REGION_META[region]

    ax.scatter(
        x,
        y,
        s=6,
        alpha=0.12,
        color=meta["color"],
        edgecolors="none",
    )

    if len(sub) >= 20 and x.std() > 0 and y.std() > 0:
        fit = stats.linregress(x, y)

        xline = np.linspace(x.min(), x.max(), 100)

        ax.plot(
            xline,
            fit.intercept + fit.slope * xline,
            color="black",
            linewidth=1.8,
        )

        ax.annotate(
            rf"$\beta={fit.slope:.3f}$" "\n" rf"$r={fit.rvalue:.3f}$" "\n" rf"$R^2={fit.rvalue ** 2:.3f}$",
            xy=(0.4, 0.96),
            xycoords="axes fraction",
            va="top",
            ha="left",
            fontsize=10,
        )

    ax.axhline(0, color="gray", linewidth=0.8, linestyle="--")

    ax.axvline(0, color="gray", linewidth=0.8, linestyle=":")

    ax.set_xlabel(r"Annual GDP growth $100\,\Delta_4\ln Y_{r,t}$")

    ax.set_title(meta["name"])

    clean_axis(ax)


axes[0].set_ylabel(r"Annual change in unemployment $\Delta_4 U_{r,t}$")


fig.tight_layout()


savefig(
    fig,
    "regional_okun_scatter",
)


plt.close(fig)


# QUARTERLY HAMILTON FILTER


def hamilton_cycle(
    series,
    log_transform=False,
):

    x = np.asarray(
        series,
        dtype=float,
    )

    if not np.all(np.isfinite(x)):
        return np.array([])

    if log_transform:
        if np.any(x <= 0):
            return np.array([])

        x = np.log(x)

    n = len(x)

    if n <= HAMILTON_H + HAMILTON_P:
        return np.array([])

    t_idx = np.arange(
        HAMILTON_P - 1,
        n - HAMILTON_H,
    )

    y = x[t_idx + HAMILTON_H]

    X = np.column_stack([np.ones(len(t_idx))] + [x[t_idx - lag] for lag in range(HAMILTON_P)])

    beta, *_ = np.linalg.lstsq(
        X,
        y,
        rcond=None,
    )

    return y - X @ beta


def filtered_log_GDP(
    group,
):

    work = (
        group[
            [
                "t",
                "GDP_r",
            ]
        ]
        .dropna()
        .sort_values("t")
    )

    t = work["t"].to_numpy()

    GDP = work["GDP_r"].to_numpy()

    if len(GDP) <= HAMILTON_H + HAMILTON_P or np.any(GDP <= 0) or not np.all(np.diff(t) == 1):
        return pd.Series(dtype=float)

    cycle = hamilton_cycle(
        GDP,
        log_transform=True,
    )

    cycle_t = t[HAMILTON_H + HAMILTON_P - 1 :]

    out = pd.Series(
        cycle,
        index=cycle_t,
    )

    return out[(out.index >= TMIN) & (out.index <= TMAX)]


# %% REGIONAL BUSINESS-CYCLE SYNCHRONISATION


def crosscorr(
    a,
    b,
    lag,
):

    # Convention:
    # lag < 0 -> first region leads second region
    # lag = 0 -> contemporaneous
    # lag > 0 -> first region lags second region

    b_shift = pd.Series(
        b.values,
        index=b.index + lag,
    )

    pair = pd.concat(
        [
            a.rename("a"),
            b_shift.rename("b"),
        ],
        axis=1,
    ).dropna()

    if len(pair) < 20 or pair["a"].std() == 0 or pair["b"].std() == 0:
        return np.nan

    return pair["a"].corr(pair["b"])


cycle_records = []

lag_records = []


pairs = [
    (
        a,
        b,
    )
    for i, a in enumerate(REGIONS)
    for b in REGIONS[i + 1 :]
]


for run, g_run in df.groupby("run"):
    cycles = {}

    for region in REGIONS:
        cycles[region] = filtered_log_GDP(g_run[g_run["region"] == region])

    row = {
        "run": run,
    }

    zero_corrs = []

    for a, b in pairs:
        name = f"R{a}_R{b}"

        for lag in range(
            -MAX_CYCLE_LAG,
            MAX_CYCLE_LAG + 1,
        ):
            corr = crosscorr(
                cycles[a],
                cycles[b],
                lag,
            )

            lag_records.append(
                {
                    "run": run,
                    "pair": name,
                    "lag_quarters": lag,
                    "lag_years": lag / 4,
                    "correlation": corr,
                }
            )

            if lag == 0:
                row[f"cycle_corr_{name}"] = corr

                zero_corrs.append(corr)

    row["mean_regional_cycle_corr"] = np.nanmean(zero_corrs)

    cycle_records.append(row)


cycle_runs = pd.DataFrame(cycle_records)


lag_runs = pd.DataFrame(lag_records)


cycle_summary = pd.DataFrame(
    {metric: mc_summary(cycle_runs[metric]) for metric in cycle_runs.columns if metric != "run"}
).T


lag_summary_records = []


for (
    pair,
    lag,
), g in lag_runs.groupby(
    [
        "pair",
        "lag_quarters",
    ]
):
    lag_summary_records.append(
        {
            "pair": pair,
            "lag_quarters": lag,
            "lag_years": lag / 4,
            **mc_summary(g["correlation"]),
        }
    )


lag_summary = pd.DataFrame(lag_summary_records)


# Cross-correlation figure

fig, ax = plt.subplots(figsize=(7, 4.2))


for pair in lag_summary["pair"].unique():
    sub = lag_summary[lag_summary["pair"] == pair]

    (line,) = ax.plot(
        sub["lag_quarters"],
        sub["mean"],
        marker="o",
        label=pair,
    )

    ax.fill_between(
        sub["lag_quarters"],
        sub["ci_low"],
        sub["ci_high"],
        color=line.get_color(),
        alpha=0.15,
    )


ax.axhline(
    0,
    color="gray",
    linestyle="--",
)


ax.axvline(
    0,
    color="gray",
    linestyle=":",
)


ax.set_xticks(
    range(
        -MAX_CYCLE_LAG,
        MAX_CYCLE_LAG + 1,
    )
)


ax.set_xlabel("Lag (quarters)")


ax.set_ylabel("Regional GDP-cycle correlation")


ax.legend(
    frameon=False,
)


clean_axis(ax)


fig.tight_layout()


savefig(
    fig,
    "regional_business_cycle_crosscorrelations",
)


plt.close(fig)


# GDP-UNEMPLOYMENT COUNTERCYCLICALITY (LEAD/LAG)


# Cross-correlation of the unemployment cycle with the GDP cycle at
# leads and lags t-4..t+4.
# lag k holds corr(GDP_cycle(t), U_cycle(t+k)).
# A negative contemporaneous value (k=0) indicates that unemployment
# is countercyclical with respect to GDP.

COUNTERCYC_LAGS = range(
    -MAX_CYCLE_LAG,
    MAX_CYCLE_LAG + 1,
)


def filtered_level_series(
    group,
    metric,
):
    """
    Level Hamilton residual indexed by t, restricted to TMIN..TMAX.
    """

    work = (
        group[
            [
                "t",
                metric,
            ]
        ]
        .dropna()
        .sort_values("t")
    )

    t = work["t"].to_numpy()

    x = work[metric].to_numpy()

    if len(x) <= HAMILTON_H + HAMILTON_P or not np.all(np.diff(t) == 1):
        return pd.Series(dtype=float)

    cycle = hamilton_cycle(
        x,
        log_transform=False,
    )

    cycle_t = t[HAMILTON_H + HAMILTON_P - 1 :]

    out = pd.Series(
        cycle,
        index=cycle_t,
    )

    return out[(out.index >= TMIN) & (out.index <= TMAX)]


countercyc_records = []


for (
    run,
    region,
), g in df.groupby(
    [
        "run",
        "region",
    ]
):
    gdp_cycle = filtered_log_GDP(g)

    u_cycle = filtered_level_series(
        g,
        "Unemployment",
    )

    if gdp_cycle.empty or u_cycle.empty:
        continue

    for lag in COUNTERCYC_LAGS:
        # U_shift[t] = U_cycle(t + lag)

        u_shift = pd.Series(
            u_cycle.values,
            index=u_cycle.index - lag,
        )

        pair = pd.concat(
            [
                gdp_cycle.rename("gdp"),
                u_shift.rename("u"),
            ],
            axis=1,
        ).dropna()

        if len(pair) < 20 or pair["gdp"].std() == 0 or pair["u"].std() == 0:
            corr = np.nan

        else:
            corr = pair["gdp"].corr(pair["u"])

        countercyc_records.append(
            {
                "run": run,
                "region": region,
                "lag_quarters": lag,
                "lag_years": lag / 4,
                "gdp_unemployment_corr": corr,
            }
        )


countercyc_runs = pd.DataFrame(countercyc_records)


countercyc_summary_records = []


for (
    region,
    lag,
), grp in countercyc_runs.groupby(
    [
        "region",
        "lag_quarters",
    ]
):
    countercyc_summary_records.append(
        {
            "region": f"R{region}",
            "lag_quarters": lag,
            "lag_years": lag / 4,
            **mc_summary(grp["gdp_unemployment_corr"]),
        }
    )


# Mean across regions: average regions within each run first

per_run_lag = (
    countercyc_runs.groupby(
        [
            "run",
            "lag_quarters",
        ]
    )["gdp_unemployment_corr"]
    .mean()
    .reset_index()
)


for lag, grp in per_run_lag.groupby("lag_quarters"):
    countercyc_summary_records.append(
        {
            "region": "mean_regions",
            "lag_quarters": lag,
            "lag_years": lag / 4,
            **mc_summary(grp["gdp_unemployment_corr"]),
        }
    )


countercyc_summary = pd.DataFrame(countercyc_summary_records)


# Wide table (lag x region) of mean GDP-unemployment correlation

countercyc_table = countercyc_summary.pivot(
    index="lag_quarters",
    columns="region",
    values="mean",
).sort_index()


# Lead/lag figure

fig, ax = plt.subplots(figsize=(7, 4.2))


for region in REGIONS:
    sub = countercyc_summary[countercyc_summary["region"] == f"R{region}"].sort_values("lag_quarters")

    meta = REGION_META[region]

    ax.plot(
        sub["lag_quarters"],
        sub["mean"],
        color=meta["color"],
        label=meta["name"],
        marker="o",
        linewidth=1.5,
    )

    ax.fill_between(
        sub["lag_quarters"],
        sub["ci_low"],
        sub["ci_high"],
        color=meta["color"],
        alpha=0.15,
    )


ax.axhline(
    0,
    color="gray",
    linewidth=0.8,
    linestyle="--",
)


ax.axvline(
    0,
    color="gray",
    linestyle=":",
)


ax.set_xticks(list(COUNTERCYC_LAGS))


ax.set_xlabel("Unemployment relative to GDP (quarters)")


ax.set_ylabel("corr(GDP, unemployment)")


ax.legend(frameon=False, ncols=3, loc="lower center", bbox_to_anchor=(0.5, -0.25))


clean_axis(ax)


fig.tight_layout()


savefig(
    fig,
    "regional_gdp_unemployment_countercyclicality",
)


plt.close(fig)


# REGIONAL VOLATILITY AND PROCYCLICALITY


def relative_level_cycle(
    group,
    metric,
):
    """
    Hamilton residual in levels, normalised by mean absolute level.

    Used for regional GDP / consumption / investment because
    regional investment can equal zero in individual quarters.
    """

    work = (
        group[
            [
                "t",
                metric,
            ]
        ]
        .dropna()
        .sort_values("t")
    )

    t = work["t"].to_numpy()

    x = work[metric].to_numpy()

    if len(x) <= HAMILTON_H + HAMILTON_P or not np.all(np.diff(t) == 1):
        return pd.DataFrame(
            columns=[
                "t",
                "cycle",
            ]
        )

    t_idx = np.arange(
        HAMILTON_P - 1,
        len(x) - HAMILTON_H,
    )

    y = x[t_idx + HAMILTON_H]

    X = np.column_stack([np.ones(len(t_idx))] + [x[t_idx - lag] for lag in range(HAMILTON_P)])

    beta, *_ = np.linalg.lstsq(
        X,
        y,
        rcond=None,
    )

    residual = y - X @ beta

    scale = np.mean(np.abs(x))

    if not np.isfinite(scale) or scale <= 0:
        return pd.DataFrame(
            columns=[
                "t",
                "cycle",
            ]
        )

    return pd.DataFrame(
        {
            "t": t[t_idx + HAMILTON_H],
            "cycle": residual / scale,
        }
    )


internal_records = []


for (
    run,
    region,
), g in df.groupby(
    [
        "run",
        "region",
    ]
):
    cycles = {}

    for metric in [
        "GDP_r",
        "Consumption_r",
        "Investment_r",
    ]:
        result = relative_level_cycle(
            g,
            metric,
        )

        result = result.rename(columns={"cycle": f"{metric}_cycle"})

        cycles[metric] = result

    merged = (
        cycles["GDP_r"]
        .merge(
            cycles["Consumption_r"],
            on="t",
        )
        .merge(
            cycles["Investment_r"],
            on="t",
        )
    )

    merged = merged[(merged["t"] >= TMIN) & (merged["t"] <= TMAX)].dropna()

    if len(merged) < 20:
        continue

    Y = merged["GDP_r_cycle"]

    C = merged["Consumption_r_cycle"]

    I = merged["Investment_r_cycle"]

    sd_Y = Y.std()

    sd_C = C.std()

    sd_I = I.std()

    internal_records.append(
        {
            "run": run,
            "region": region,
            "consumption_to_GDP_vol": sd_C / sd_Y,
            "investment_to_GDP_vol": sd_I / sd_Y,
            "consumption_less_volatile_than_GDP": float(sd_C < sd_Y),
            "investment_more_volatile_than_GDP": float(sd_I > sd_Y),
            "corr_consumption_GDP": C.corr(Y),
            "corr_investment_GDP": I.corr(Y),
        }
    )


internal_runs = pd.DataFrame(internal_records)


volatility_summary = summarise_run_region(
    internal_runs,
    [
        "consumption_to_GDP_vol",
        "investment_to_GDP_vol",
        "consumption_less_volatile_than_GDP",
        "investment_more_volatile_than_GDP",
    ],
)


procyclicality_summary = summarise_run_region(
    internal_runs,
    [
        "corr_consumption_GDP",
        "corr_investment_GDP",
    ],
)


# Volatility / procyclicality figure

fig, axes = plt.subplots(
    1,
    2,
    figsize=(10, 4),
)


x = np.arange(len(REGIONS))


offset = 0.12


for (
    metric,
    label,
    shift,
) in [
    (
        "consumption_to_GDP_vol",
        "Consumption / GDP",
        -offset,
    ),
    (
        "investment_to_GDP_vol",
        "Investment / GDP",
        offset,
    ),
]:
    for i, region in enumerate(REGIONS):
        s = mc_summary(
            internal_runs.loc[
                internal_runs["region"] == region,
                metric,
            ]
        )

        axes[0].errorbar(
            i + shift,
            s["mean"],
            yerr=[
                [s["mean"] - s["ci_low"]],
                [s["ci_high"] - s["mean"]],
            ],
            fmt="o",
            capsize=3,
            label=label if i == 0 else None,
        )


axes[0].axhline(
    1,
    color="gray",
    linestyle="--",
)


axes[0].set_ylabel("Relative cyclical volatility")


axes[0].legend(frameon=False)


for (
    metric,
    label,
    shift,
) in [
    (
        "corr_consumption_GDP",
        "Consumption",
        -offset,
    ),
    (
        "corr_investment_GDP",
        "Investment",
        offset,
    ),
]:
    for i, region in enumerate(REGIONS):
        s = mc_summary(
            internal_runs.loc[
                internal_runs["region"] == region,
                metric,
            ]
        )

        axes[1].errorbar(
            i + shift,
            s["mean"],
            yerr=[
                [s["mean"] - s["ci_low"]],
                [s["ci_high"] - s["mean"]],
            ],
            fmt="o",
            capsize=3,
            label=label if i == 0 else None,
        )


axes[1].axhline(
    0,
    color="gray",
    linestyle="--",
)


axes[1].set_ylabel("Correlation with GDP cycle")


axes[1].legend(frameon=False)


for ax in axes:
    ax.set_xticks(x)

    ax.set_xticklabels([REGION_META[r]["name"] for r in REGIONS])

    clean_axis(ax)


fig.tight_layout()


savefig(
    fig,
    "regional_volatility_procyclicality",
)


plt.close(fig)


# REGIONAL HOME BIAS

trade = df[(df["t"] >= TMIN) & (df["t"] <= TMAX)].copy()


cons_cols = [f"cons_buy_from_R{r}" for r in REGIONS]


mach_cols = [f"mach_buy_from_R{r}" for r in REGIONS]


trade["cons_total"] = trade[cons_cols].sum(axis=1)


trade["mach_total"] = trade[mach_cols].sum(axis=1)


trade["cons_local"] = np.nan

trade["mach_local"] = np.nan


for region in REGIONS:
    mask = trade["region"] == region

    trade.loc[
        mask,
        "cons_local",
    ] = trade.loc[
        mask,
        f"cons_buy_from_R{region}",
    ]

    trade.loc[
        mask,
        "mach_local",
    ] = trade.loc[
        mask,
        f"mach_buy_from_R{region}",
    ]


trade["cons_local_share"] = np.where(
    trade["cons_total"] > 0,
    trade["cons_local"] / trade["cons_total"],
    np.nan,
)


trade["mach_local_share"] = np.where(
    trade["mach_total"] > 0,
    trade["mach_local"] / trade["mach_total"],
    np.nan,
)


total_N2 = trade.groupby(["run", "t"])["N2r"].transform("sum")


total_N1 = trade.groupby(["run", "t"])["N1r"].transform("sum")


trade["C_supplier_share"] = trade["N2r"] / total_N2


trade["K_supplier_share"] = trade["N1r"] / total_N1


trade["cons_home_bias_index"] = trade["cons_local_share"] / trade["C_supplier_share"]


trade["mach_home_bias_index"] = trade["mach_local_share"] / trade["K_supplier_share"]


home_runs = (
    trade.groupby(
        [
            "run",
            "region",
        ]
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


home_summary = summarise_run_region(
    home_runs,
    [
        "cons_local_share",
        "mach_local_share",
        "cons_home_bias_index",
        "mach_home_bias_index",
    ],
)


# Home-bias figure

fig, axes = plt.subplots(
    1,
    2,
    figsize=(9, 3.8),
)


for ax, (
    metric,
    title,
) in zip(
    axes,
    [
        (
            "cons_home_bias_index",
            "Consumption goods",
        ),
        (
            "mach_home_bias_index",
            "Machinery",
        ),
    ],
):
    for i, region in enumerate(REGIONS):
        s = mc_summary(
            home_runs.loc[
                home_runs["region"] == region,
                metric,
            ]
        )

        ax.errorbar(
            i,
            s["mean"],
            yerr=[
                [s["mean"] - s["ci_low"]],
                [s["ci_high"] - s["mean"]],
            ],
            fmt="o",
            capsize=4,
            color=REGION_META[region]["color"],
        )

    ax.axhline(
        1,
        color="gray",
        linestyle="--",
    )

    ax.set_xticks(range(len(REGIONS)))

    ax.set_xticklabels([REGION_META[r]["name"] for r in REGIONS])

    ax.set_ylabel("Home-bias index")

    ax.set_title(title)

    clean_axis(ax)


fig.tight_layout()


savefig(
    fig,
    "regional_home_bias",
)


plt.close(fig)


# CROSS-REGIONAL KNOWLEDGE DIFFUSION AND LABOUR MIGRATION MATRICES


def annotated_heatmap(
    ax,
    matrix,
    row_labels,
    col_labels,
    cmap="viridis",
    vmin=0.0,
    vmax=1.0,
    fmt="{:.2f}",
    text_color=None,
):
    """Imshow heatmap with per-cell value annotations."""

    if vmin is None:
        vmin = np.nanmin(matrix)

    if vmax is None:
        vmax = np.nanmax(matrix)

    im = ax.imshow(
        matrix,
        cmap=cmap,
        vmin=vmin,
        vmax=vmax,
        aspect="auto",
    )

    ax.set_xticks(range(len(col_labels)))

    ax.set_xticklabels(col_labels)

    ax.set_yticks(range(len(row_labels)))

    ax.set_yticklabels(row_labels)

    midpoint = 0.5 * (vmin + vmax)

    for i in range(matrix.shape[0]):
        for j in range(matrix.shape[1]):
            value = matrix[i, j]

            if np.isfinite(value):
                ax.text(
                    j,
                    i,
                    fmt.format(value),
                    ha="center",
                    va="center",
                    color=text_color if text_color is not None else ("black" if value < midpoint else "white"),
                    fontsize=10,
                )

    return im


# Imitation: stay (within-region) vs move (cross-region) likelihood

imit_cols = [
    "within_region_imitation",
    "cross_region_imitation",
]


if all(c in df.columns for c in imit_cols):
    imit = df[(df["t"] >= TMIN) & (df["t"] <= TMAX)][
        [
            "run",
            "region",
            "t",
            *imit_cols,
        ]
    ].copy()

    imit["imit_total"] = imit["within_region_imitation"] + imit["cross_region_imitation"]

    imit["p_stay"] = np.where(
        imit["imit_total"] > 0,
        imit["within_region_imitation"] / imit["imit_total"],
        np.nan,
    )

    imit["p_move"] = np.where(
        imit["imit_total"] > 0,
        imit["cross_region_imitation"] / imit["imit_total"],
        np.nan,
    )

    # Time-average within run, then Monte Carlo across runs

    imit_run = imit.groupby(["run", "region"])[["p_stay", "p_move"]].mean().reset_index()

    imit_summary = summarise_run_region(
        imit_run,
        [
            "p_stay",
            "p_move",
        ],
    )

    imit_matrix = np.full((len(REGIONS), 2), np.nan)

    for i, region in enumerate(REGIONS):
        for j, metric in enumerate(["p_stay", "p_move"]):
            imit_matrix[i, j] = mc_summary(imit_run.loc[imit_run["region"] == region, metric])["mean"]

    fig, ax = plt.subplots(figsize=(4.8, 4))

    im = annotated_heatmap(
        ax,
        imit_matrix,
        row_labels=[REGION_META[r]["name"] for r in REGIONS],
        col_labels=["Stay (within)", "Move (cross)"],
        cmap="Blues",
    )

    ax.set_ylabel("Imitating region")

    ax.set_title("Imitation-target likelihood")

    fig.colorbar(im, ax=ax, fraction=0.046, pad=0.04)

    fig.tight_layout()

    savefig(
        fig,
        "regional_imitation_stay_move",
    )

    plt.close(fig)

    imit_summary.to_csv(
        OUTPUT / "imitation_stay_move_stats.csv",
        index=False,
    )

else:
    imit_summary = None

    warnings.warn("Imitation columns (within/cross) not found; " "skipping imitation stay/move matrix.")


# Labour migration: share of labourers staying vs moving, in percent
# (origin -> destination; diagonal = stayers). Row shares are computed
# within each run, then Monte Carlo averaged across runs. Each row sums
# to 100%.

mig_cols = [f"mig_count_to_R{r}" for r in REGIONS]


if all(c in df.columns for c in mig_cols):
    mig = df[(df["t"] >= TMIN) & (df["t"] <= TMAX)][
        [
            "run",
            "region",
            "t",
            *mig_cols,
        ]
    ].copy()

    mig_run = mig.groupby(["run", "region"])[mig_cols].mean().reset_index()

    row_total = mig_run[mig_cols].sum(axis=1)

    for col in mig_cols:
        mig_run[col] = np.where(
            row_total > 0,
            100.0 * mig_run[col] / row_total,
            np.nan,
        )

    mig_matrix = np.full((len(REGIONS), len(REGIONS)), np.nan)

    for i, origin in enumerate(REGIONS):
        for j, dest in enumerate(REGIONS):
            mig_matrix[i, j] = mc_summary(mig_run.loc[mig_run["region"] == origin, f"mig_count_to_R{dest}"])["mean"]

    fig, ax = plt.subplots(figsize=(5.2, 4.4))
    im = annotated_heatmap(
        ax,
        mig_matrix,
        row_labels=[REGION_META[r]["name"] for r in REGIONS],
        col_labels=[REGION_META[r]["name"] for r in REGIONS],
        cmap="BuPu",
        vmin=0.0,
        vmax=100.0,
        fmt="{:.1f}%",
    )

    ax.set_xlabel("Destination region")

    ax.set_ylabel("Origin region")

    ax.set_title("Labour migration (% of origin pool; diagonal = stayers)")

    fig.colorbar(im, ax=ax, fraction=0.046, pad=0.04)

    fig.tight_layout()

    savefig(
        fig,
        "regional_labour_migration_matrix",
    )

    plt.close(fig)

    mig_matrix_df = pd.DataFrame(
        mig_matrix,
        index=[REGION_META[r]["name"] for r in REGIONS],
        columns=[REGION_META[r]["name"] for r in REGIONS],
    )

    mig_matrix_df.to_csv(OUTPUT / "labour_migration_matrix_pct.csv")

else:
    mig_matrix_df = None

    warnings.warn(
        "Migration columns (mig_count_to_R*) not found; rebuild the model "
        "and rerun to populate the labour migration matrix."
    )


# Inter-regional trade: share of each buyer region's purchases sourced
# from each supplier region, in percent (diagonal = local/home share).
# Row shares are computed within each run, then Monte Carlo averaged.


def trade_share_matrix(prefix):
    buy_cols = [f"{prefix}_buy_from_R{r}" for r in REGIONS]

    if not all(c in df.columns for c in buy_cols):
        return None

    buy = df[(df["t"] >= TMIN) & (df["t"] <= TMAX)][
        [
            "run",
            "region",
            "t",
            *buy_cols,
        ]
    ].copy()

    buy_run = buy.groupby(["run", "region"])[buy_cols].mean().reset_index()

    row_total = buy_run[buy_cols].sum(axis=1)

    for col in buy_cols:
        buy_run[col] = np.where(
            row_total > 0,
            100.0 * buy_run[col] / row_total,
            np.nan,
        )

    matrix = np.full((len(REGIONS), len(REGIONS)), np.nan)

    for i, buyer in enumerate(REGIONS):
        for j, supplier in enumerate(REGIONS):
            matrix[i, j] = mc_summary(buy_run.loc[buy_run["region"] == buyer, f"{prefix}_buy_from_R{supplier}"])["mean"]

    return matrix


trade_specs = [
    ("cons", "Consumption goods", "regional_cons_trade_matrix"),
    ("mach", "Machinery", "regional_mach_trade_matrix"),
]


for prefix, title, fname in trade_specs:
    matrix = trade_share_matrix(prefix)

    if matrix is None:
        warnings.warn(f"Trade columns ({prefix}_buy_from_R*) not found; skipping {title} matrix.")

        continue

    fig, ax = plt.subplots(figsize=(5.2, 4.4))

    im = annotated_heatmap(
        ax,
        matrix,
        row_labels=[REGION_META[r]["name"] for r in REGIONS],
        col_labels=[REGION_META[r]["name"] for r in REGIONS],
        cmap="BuPu",
        vmin=0.0,
        vmax=100.0,
        fmt="{:.1f}%",
    )

    ax.set_xlabel("Supplier region")

    ax.set_ylabel("Buyer region")

    ax.set_title(f"{title} sourcing (% of buyer purchases; diagonal = local)")

    fig.colorbar(im, ax=ax, fraction=0.046, pad=0.04)

    fig.tight_layout()

    savefig(fig, fname)

    plt.close(fig)

    pd.DataFrame(
        matrix,
        index=[REGION_META[r]["name"] for r in REGIONS],
        columns=[REGION_META[r]["name"] for r in REGIONS],
    ).to_csv(OUTPUT / f"{fname}_pct.csv")


# BUILD CLEAN PAPER TABLE


paper_rows = []


def add_indexed_table(
    table,
    fact,
):

    for statistic, row in table.iterrows():
        paper_rows.append(
            {
                "fact": fact,
                "statistic": statistic,
                "region": "MC aggregate",
                "mean": row["mean"],
                "std": row["std"],
                "n_runs": row["n_runs"],
                "ci_low": row["ci_low"],
                "ci_high": row["ci_high"],
            }
        )


def add_mean_region_table(
    table,
    fact,
):

    sub = table[table["region"] == "mean_regions"]

    for _, row in sub.iterrows():
        paper_rows.append(
            {
                "fact": fact,
                "statistic": row["statistic"],
                "region": "Mean across regions",
                "mean": row["mean"],
                "std": row["std"],
                "n_runs": row["n_runs"],
                "ci_low": row["ci_low"],
                "ci_high": row["ci_high"],
            }
        )


add_indexed_table(
    productivity_stats,
    "Productivity heterogeneity",
)


add_indexed_table(
    unemployment_stats,
    "Unemployment differentiation",
)


add_indexed_table(
    wage_stats,
    "Wage differentiation",
)


add_indexed_table(
    wage_productivity_summary,
    "Wage-productivity association",
)


add_indexed_table(
    cycle_summary.loc[["mean_regional_cycle_corr"]],
    "Business-cycle synchronisation",
)


add_mean_region_table(
    okun_summary,
    "Okun's law",
)


add_mean_region_table(
    volatility_summary,
    "Relative volatility",
)


add_mean_region_table(
    procyclicality_summary,
    "Procyclicality",
)


for _, row in countercyc_summary[countercyc_summary["region"] == "mean_regions"].iterrows():
    paper_rows.append(
        {
            "fact": "GDP-unemployment countercyclicality",
            "statistic": f"corr_U(t{int(row['lag_quarters']):+d})_GDP(t)",
            "region": "Mean across regions",
            "mean": row["mean"],
            "std": row["std"],
            "n_runs": row["n_runs"],
            "ci_low": row["ci_low"],
            "ci_high": row["ci_high"],
        }
    )


add_mean_region_table(
    home_summary,
    "Trade home bias",
)


paper_table = pd.DataFrame(paper_rows)


# 12. SAVE TABLES


productivity_stats.to_csv(OUTPUT / "productivity_stats.csv")


unemployment_stats.to_csv(OUTPUT / "unemployment_stats.csv")


wage_stats.to_csv(OUTPUT / "wage_stats.csv")


wage_productivity_summary.to_csv(OUTPUT / "wage_productivity_stats.csv")


okun_summary.to_csv(
    OUTPUT / "okun_stats.csv",
    index=False,
)


cycle_summary.to_csv(OUTPUT / "business_cycle_sync_stats.csv")


lag_summary.to_csv(
    OUTPUT / "business_cycle_crosscorr_by_lag.csv",
    index=False,
)


volatility_summary.to_csv(
    OUTPUT / "relative_volatility_stats.csv",
    index=False,
)


procyclicality_summary.to_csv(
    OUTPUT / "procyclicality_stats.csv",
    index=False,
)


countercyc_summary.to_csv(
    OUTPUT / "gdp_unemployment_countercyclicality_stats.csv",
    index=False,
)


countercyc_table.to_csv(
    OUTPUT / "gdp_unemployment_countercyclicality_table.csv",
)


home_summary.to_csv(
    OUTPUT / "home_bias_stats.csv",
    index=False,
)


paper_table.to_csv(
    OUTPUT / "regional_stylised_facts_summary.csv",
    index=False,
)


# LaTeX table

try:
    paper_table.to_latex(
        OUTPUT / "regional_stylised_facts_summary.tex",
        index=False,
        float_format="%.4f",
    )


except Exception as exc:
    warnings.warn(f"LaTeX table could not be written: {exc}")


# Save run-level data for robustness / later boxplots

okun_runs.to_csv(
    OUTPUT / "run_region_okun.csv",
    index=False,
)


internal_runs.to_csv(
    OUTPUT / "run_region_business_cycle.csv",
    index=False,
)


countercyc_runs.to_csv(
    OUTPUT / "run_region_gdp_unemployment_countercyclicality.csv",
    index=False,
)


home_runs.to_csv(
    OUTPUT / "run_region_home_bias.csv",
    index=False,
)


wage_productivity_runs.to_csv(
    OUTPUT / "run_wage_productivity.csv",
    index=False,
)


# 13. CONSOLE OUTPUT


print("\n" + "=" * 72)

print("REGIONAL STYLISED FACTS")

print("=" * 72)


print(f"\nScenario: {SCENARIO}")

print(f"Window: t={TMIN}..{TMAX} ({TMAX - TMIN + 1} quarters)")


print("\n=== Productivity ===")

print(productivity_stats.round(4).to_string())


print("\n=== Unemployment ===")

print(unemployment_stats.round(4).to_string())


print("\n=== Wages ===")

print(wage_stats.round(4).to_string())


print("\n=== Wage-productivity association ===")

print(wage_productivity_summary.round(4).to_string())


print("\n=== Okun's law ===")

print(okun_summary.round(4).to_string(index=False))


print("\n=== Business-cycle synchronisation ===")

print(cycle_summary.round(4).to_string())


print("\n=== Relative volatility ===")

print(volatility_summary.round(4).to_string(index=False))


print("\n=== Procyclicality ===")

print(procyclicality_summary.round(4).to_string(index=False))


print("\n=== GDP-unemployment countercyclicality (corr of U(t+lag) with GDP(t)) ===")

print(countercyc_table.round(4).to_string())


print("\n=== Home bias ===")

print(home_summary.round(4).to_string(index=False))


if imit_summary is not None:
    print("\n=== Imitation stay (within) vs move (cross) ===")

    print(imit_summary.round(4).to_string(index=False))


if mig_matrix_df is not None:
    print("\n=== Labour flows (% of origin pool; origin -> destination; diagonal = stayers) ===")

    print(mig_matrix_df.round(2).to_string())


print("\nRun-region groups used for C/I/Y business-cycle statistics:")

print(
    len(
        internal_runs[
            [
                "run",
                "region",
            ]
        ].drop_duplicates()
    ),
    "/",
    len(
        df[
            [
                "run",
                "region",
            ]
        ].drop_duplicates()
    ),
)


print("\nOutputs written to:")

print(OUTPUT)

print("=" * 72)
# %%
