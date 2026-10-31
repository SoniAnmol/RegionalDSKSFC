#!/usr/bin/env python3
"""
Compare the DSK-SFC qualitative stylised facts reported in Reissl et al.
(Table A1) across two simulation scenarios.
The first 200 quarterly periods are discarded. Snapshot validation files are
written every 50 periods, so retained rows correspond to t=250,...,600.
"""

from pathlib import Path
import csv
import math
import re
from collections import defaultdict

import numpy as np

try:
    from scipy.stats import kurtosis, t as student_t, gennorm, norm, laplace
except ImportError as exc:
    raise SystemExit(
        "This script requires scipy. Install with:\n"
        "    python3 -m pip install numpy scipy matplotlib"
    ) from exc

try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    HAVE_MATPLOTLIB = True
except ImportError:
    HAVE_MATPLOTLIB = False



ROOT_DIR = Path(
    "/Users/anmolsoni/Nextcloud/projects/Regionalised-SFC-DSK/validation/"
)

BURN_IN = 200
SNAPSHOT_FREQUENCY = 50

# Hamilton quarterly filter used by Reissl et al.
HAMILTON_H = 8
HAMILTON_P = 4

# Cross-correlation lags in quarters.
# Convention: corr(x_t, y_{t+lag}); positive lag means y follows x.
MAX_CCF_LAG = 8

# Confidence intervals across Monte Carlo runs.
ALPHA = 0.05

# Operational criteria used only to turn qualitative diagnostics into a
# convenient checklist. These are NOT thresholds stated by Reissl et al.
#
# For SF02 and SF10, fat tails are assessed with the generalized-normal
# (Subbotin/exponential-power) shape parameter beta:
#     beta = 2  -> Gaussian
#     beta = 1  -> Laplace
#     beta < 2  -> more peaked/heavy-tailed than Gaussian
#
# Excess kurtosis is retained as corroborating evidence. The old >2-sigma
# tail ratio is still reported descriptively, but is NOT used for pass/fail,
# because the submitted Reissl executable itself can have a ratio below 1
# while clearly exhibiting beta < 2 and large positive excess kurtosis.
FAT_TAIL_MAX_BETA = 2.0
FAT_TAIL_MIN_EXCESS_KURTOSIS = 0.0

# "Lumpy investment" is assessed using validation10, which records C-firm
# investment I(j), not I/K. These thresholds are therefore a practical proxy.
LUMPY_MIN_ZERO_SHARE = 0.10
LUMPY_MIN_TOP10_SHARE = 0.30

# Persistent heterogeneity: cross-sectional dispersion should not collapse.
HETEROGENEITY_EPS = 1e-6
HETEROGENEITY_MIN_FINAL_TO_INITIAL = 0.50

OUTPUT_DIR = ROOT_DIR / "stylised_facts_analysis"
PLOT_DIR = OUTPUT_DIR / "plots"


# Table A1 stylised facts


STYLISED_FACTS = [
    ("SF01", "Endogenous growth with persistent fluctuations"),
    ("SF02", "Fat-tailed GDP growth-rate distribution"),
    ("SF03", "Relative volatility of main macroeconomic aggregates"),
    ("SF04", "Cross-correlations of main macroeconomic aggregates"),
    ("SF05", "Pro-cyclical private debt"),
    ("SF06", "Pro-cyclical R&D investment"),
    ("SF07", "Pro-cyclical energy demand"),
    ("SF08", "Pro-cyclical emissions"),
    ("SF09", "Cross-correlation between private debt and loan losses"),
    ("SF10", "Fat-tailed firm growth-rate distribution"),
    ("SF11", "Lumpy investment rates at firm level"),
    ("SF12", "Persistent productivity heterogeneity across firms"),
    ("SF13", "Persistent energy efficiency heterogeneity across firms"),
    ("SF14", "Persistent emission intensity heterogeneity across firms"),
]

# Generic helpers

def finite_array(values):
    x = np.asarray(values, dtype=float)
    return x[np.isfinite(x)]


def mean_ci(values, confidence=0.95):
    x = finite_array(values)
    n = len(x)
    if n == 0:
        return {"n": 0, "mean": math.nan, "sd": math.nan, "se": math.nan,
                "lower95": math.nan, "upper95": math.nan}
    mean = float(np.mean(x))
    if n == 1:
        return {"n": 1, "mean": mean, "sd": math.nan, "se": math.nan,
                "lower95": math.nan, "upper95": math.nan}
    sd = float(np.std(x, ddof=1))
    se = sd / math.sqrt(n)
    q = float(student_t.ppf((1.0 + confidence) / 2.0, df=n - 1))
    return {"n": n, "mean": mean, "sd": sd, "se": se,
            "lower95": mean - q * se, "upper95": mean + q * se}


def safe_corr(x, y):
    x = np.asarray(x, dtype=float)
    y = np.asarray(y, dtype=float)
    good = np.isfinite(x) & np.isfinite(y)
    x = x[good]
    y = y[good]
    if len(x) < 3 or np.std(x) == 0 or np.std(y) == 0:
        return math.nan
    return float(np.corrcoef(x, y)[0, 1])


def autocorr(x, lag=1):
    x = np.asarray(x, dtype=float)
    if lag <= 0 or len(x) <= lag:
        return math.nan
    return safe_corr(x[:-lag], x[lag:])


def cross_correlation(x, y, lags):
    """corr(x_t, y_{t+lag}); positive lag means y follows x."""
    x = np.asarray(x, dtype=float)
    y = np.asarray(y, dtype=float)
    if len(x) != len(y):
        raise ValueError("Cross-correlation series must have equal length.")
    out = []
    for lag in lags:
        if lag > 0:
            xx, yy = x[:-lag], y[lag:]
        elif lag < 0:
            xx, yy = x[-lag:], y[:lag]
        else:
            xx, yy = x, y
        out.append(safe_corr(xx, yy))
    return np.asarray(out, dtype=float)


def hamilton_cycle(series, log_transform=False, h=8, p=4):
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
    X = np.column_stack([np.ones(len(t_idx))] +
                        [x[t_idx - lag] for lag in range(p)])
    beta, *_ = np.linalg.lstsq(X, y, rcond=None)
    return y - X @ beta


def annualised_log_growth(series):
    x = np.asarray(series, dtype=float)
    if len(x) < 2 or not np.all(np.isfinite(x)) or np.any(x <= 0):
        return math.nan
    return float(4.0 * np.mean(np.diff(np.log(x))))


def standardised_values(x):
    x = finite_array(x)
    if len(x) < 2:
        return np.array([], dtype=float)
    sd = np.std(x, ddof=1)
    if sd <= 0:
        return np.array([], dtype=float)
    return (x - np.mean(x)) / sd


def fat_tail_metrics(values):
    """
    Distributional diagnostics for SF02/SF10.

    The primary shape diagnostic is the generalized-normal shape parameter beta:
        beta = 2 : Gaussian
        beta = 1 : Laplace
        beta < 2 : more peaked / heavier-tailed than Gaussian

    Also reports excess kurtosis, the legacy >2-sigma tail ratio (descriptive
    only), and AIC for Gaussian, Laplace and generalized-normal fits.
    Fits are performed on standardized observations.
    """
    x = finite_array(values)

    empty = {
        "n": len(x),
        "excess_kurtosis": math.nan,
        "tail_ratio_2sd": math.nan,
        "gennorm_beta": math.nan,
        "aic_gaussian": math.nan,
        "aic_laplace": math.nan,
        "aic_gennorm": math.nan,
        "delta_aic_gaussian_minus_gennorm": math.nan,
        "delta_aic_gaussian_minus_laplace": math.nan,
    }

    if len(x) < 8:
        return empty

    z = standardised_values(x)
    if len(z) < 8:
        return empty

    excess = float(kurtosis(z, fisher=True, bias=False))

    # Legacy diagnostic, retained only for descriptive comparison.
    normal_tail_prob = 0.04550026389635842  # P(|Z|>2), Z~N(0,1)
    observed_tail_prob = float(np.mean(np.abs(z) > 2.0))
    tail_ratio = observed_tail_prob / normal_tail_prob

    try:
        # Gaussian: 2 estimated parameters (loc, scale)
        n_loc, n_scale = norm.fit(z)
        ll_norm = float(np.sum(norm.logpdf(z, loc=n_loc, scale=n_scale)))
        aic_norm = 2 * 2 - 2 * ll_norm

        # Laplace: 2 estimated parameters (loc, scale)
        l_loc, l_scale = laplace.fit(z)
        ll_laplace = float(np.sum(laplace.logpdf(z, loc=l_loc, scale=l_scale)))
        aic_laplace = 2 * 2 - 2 * ll_laplace

        # Generalized normal: 3 estimated parameters (beta, loc, scale)
        beta_hat, g_loc, g_scale = gennorm.fit(z)
        ll_gennorm = float(
            np.sum(gennorm.logpdf(z, beta_hat, loc=g_loc, scale=g_scale))
        )
        aic_gennorm = 2 * 3 - 2 * ll_gennorm

        return {
            "n": len(x),
            "excess_kurtosis": excess,
            "tail_ratio_2sd": tail_ratio,
            "gennorm_beta": float(beta_hat),
            "aic_gaussian": float(aic_norm),
            "aic_laplace": float(aic_laplace),
            "aic_gennorm": float(aic_gennorm),
            "delta_aic_gaussian_minus_gennorm": float(aic_norm - aic_gennorm),
            "delta_aic_gaussian_minus_laplace": float(aic_norm - aic_laplace),
        }
    except Exception:
        empty["excess_kurtosis"] = excess
        empty["tail_ratio_2sd"] = tail_ratio
        return empty

def top_share(values, q=0.10):
    x = finite_array(values)
    x = x[x >= 0]
    if len(x) == 0 or np.sum(x) <= 0:
        return math.nan
    n_top = max(1, int(math.ceil(q * len(x))))
    xs = np.sort(x)
    return float(np.sum(xs[-n_top:]) / np.sum(xs))


def slope_against_time(times, values):
    times = np.asarray(times, dtype=float)
    values = np.asarray(values, dtype=float)
    good = np.isfinite(times) & np.isfinite(values)
    times, values = times[good], values[good]
    if len(values) < 2:
        return math.nan
    X = np.column_stack([np.ones(len(times)), times])
    beta, *_ = np.linalg.lstsq(X, values, rcond=None)
    return float(beta[1])


def fmt(x, digits=4):
    if isinstance(x, (bool, np.bool_)):
        return "YES" if bool(x) else "NO"
    if x is None:
        return "NA"
    try:
        if not math.isfinite(float(x)):
            return "NA"
    except (TypeError, ValueError):
        return str(x)
    return f"{float(x):.{digits}f}"


# File discovery and reading


def get_seed(path):
    m = re.fullmatch(r"validation1_(\d+)\.txt", path.name)
    return int(m.group(1)) if m else None


def get_scenario(path, root):
    rel = path.resolve().relative_to(root.resolve())
    if len(rel.parts) < 2:
        raise ValueError(
            f"Cannot infer scenario from {path}. The first directory below "
            "ROOT_DIR must be the scenario name."
        )
    return rel.parts[0]


def related_validation_path(validation1_path, number, seed):
    return validation1_path.parent / f"validation{number}_{seed}.txt"


def load_matrix(path):
    data = np.genfromtxt(path, dtype=float, missing_values="NA",
                         filling_values=np.nan, invalid_raise=False)
    if data.size == 0:
        return np.empty((0, 0), dtype=float)
    if data.ndim == 1:
        data = data.reshape(1, -1)
    return data


def snapshot_post_burn(data):
    if data.size == 0:
        return np.array([], dtype=float), data
    times = np.arange(SNAPSHOT_FREQUENCY,
                      SNAPSHOT_FREQUENCY * (data.shape[0] + 1),
                      SNAPSHOT_FREQUENCY, dtype=float)
    keep = times > BURN_IN
    return times[keep], data[keep, :]


# One-run macro diagnostics

CCF_RELATIONS = {
    "gdp_consumption": ("GDP vs Consumption", "gdp", "consumption"),
    "gdp_investment": ("GDP vs Investment", "gdp", "investment"),
    "gdp_employment": ("GDP vs Employment", "gdp", "employment"),
    "gdp_inflation": ("GDP vs Inflation", "gdp", "inflation"),
    "gdp_private_debt": ("GDP vs Private debt", "gdp", "loans"),
    "gdp_rd": ("GDP vs R&D", "gdp", "rd"),
    "gdp_energy": ("GDP vs Energy demand", "gdp", "energy"),
    "gdp_emissions": ("GDP vs Emissions", "gdp", "emissions"),
    "debt_bad_debt": ("Private debt vs Bad debt", "loans", "bad_debt"),
}

LAGS = np.arange(-MAX_CCF_LAG, MAX_CCF_LAG + 1, dtype=int)


def read_macro_run(path):
    data = np.loadtxt(path)
    if data.ndim == 1:
        data = data.reshape(1, -1)
    if data.shape[1] < 40:
        raise ValueError(f"{path} has {data.shape[1]} columns; expected >=40.")

    t = data[:, 0]
    post = data[t > BURN_IN]
    if len(post) == 0:
        raise ValueError(f"No observations remain after burn-in in {path}")

    gdp = post[:, 1]
    consumption = post[:, 2]
    investment = post[:, 3]
    employment = post[:, 4]
    inflation = 4.0 * (post[:, 5] - 1.0)
    process_emissions = post[:, 6]
    energy = post[:, 7]
    rd = post[:, 8]
    loans = post[:, 9]
    bad_debt = post[:, 10]
    energy_emissions = post[:, 39]
    emissions = process_emissions + energy_emissions

    gdp_growth = np.diff(np.log(gdp)) if np.all(gdp > 0) else np.array([])

    cycles = {
        "gdp": hamilton_cycle(gdp, True, HAMILTON_H, HAMILTON_P),
        "consumption": hamilton_cycle(consumption, True, HAMILTON_H, HAMILTON_P),
        "investment": hamilton_cycle(investment, True, HAMILTON_H, HAMILTON_P),
        "employment": hamilton_cycle(employment, False, HAMILTON_H, HAMILTON_P),
        "inflation": hamilton_cycle(inflation, False, HAMILTON_H, HAMILTON_P),
        "loans": hamilton_cycle(loans, bool(np.all(loans > 0)),
                                 HAMILTON_H, HAMILTON_P),
        "rd": hamilton_cycle(rd, False, HAMILTON_H, HAMILTON_P),
        "bad_debt": hamilton_cycle(bad_debt, False, HAMILTON_H, HAMILTON_P),
        "energy": hamilton_cycle(energy, bool(np.all(energy > 0)),
                                  HAMILTON_H, HAMILTON_P),
        "emissions": hamilton_cycle(emissions, bool(np.all(emissions > 0)),
                                     HAMILTON_H, HAMILTON_P),
    }

    ft = fat_tail_metrics(gdp_growth)
    metrics = {
        "n_post_burnin": len(post),
        "sf01_annual_gdp_growth": annualised_log_growth(gdp),
        "sf01_gdp_cycle_sd": float(np.nanstd(cycles["gdp"], ddof=1)),
        "sf01_gdp_cycle_acf1": autocorr(cycles["gdp"], 1),
        "sf02_gdp_growth_excess_kurtosis": ft["excess_kurtosis"],
        "sf02_gdp_growth_tail_ratio_2sd": ft["tail_ratio_2sd"],
        "sf02_gdp_growth_gennorm_beta": ft["gennorm_beta"],
        "sf02_gdp_growth_aic_gaussian": ft["aic_gaussian"],
        "sf02_gdp_growth_aic_laplace": ft["aic_laplace"],
        "sf02_gdp_growth_aic_gennorm": ft["aic_gennorm"],
        "sf02_gdp_growth_delta_aic_gaussian_minus_gennorm":
            ft["delta_aic_gaussian_minus_gennorm"],
        "sf02_gdp_growth_delta_aic_gaussian_minus_laplace":
            ft["delta_aic_gaussian_minus_laplace"],
        "sd_gdp": float(np.nanstd(cycles["gdp"], ddof=1)),
        "sd_consumption": float(np.nanstd(cycles["consumption"], ddof=1)),
        "sd_investment": float(np.nanstd(cycles["investment"], ddof=1)),
        "sd_employment": float(np.nanstd(cycles["employment"], ddof=1)),
        "sd_inflation": float(np.nanstd(cycles["inflation"], ddof=1)),
    }

    gdp_sd = metrics["sd_gdp"]
    metrics["sf03_consumption_to_gdp_vol"] = (
        metrics["sd_consumption"] / gdp_sd if gdp_sd > 0 else math.nan)
    metrics["sf03_investment_to_gdp_vol"] = (
        metrics["sd_investment"] / gdp_sd if gdp_sd > 0 else math.nan)
    metrics["sf03_employment_to_gdp_vol"] = (
        metrics["sd_employment"] / gdp_sd if gdp_sd > 0 else math.nan)
    metrics["sf03_inflation_to_gdp_vol"] = (
        metrics["sd_inflation"] / gdp_sd if gdp_sd > 0 else math.nan)

    ccfs = {}
    lag0_idx = int(np.where(LAGS == 0)[0][0])

    for key, (_, xname, yname) in CCF_RELATIONS.items():
        curve = cross_correlation(cycles[xname], cycles[yname], LAGS)
        ccfs[key] = curve
        metrics[f"ccf_{key}_lag0"] = float(curve[lag0_idx])
        finite = np.isfinite(curve)
        if np.any(finite):
            finite_idx = np.where(finite)[0]
            local_idx = finite_idx[np.nanargmax(curve[finite])]
            metrics[f"ccf_{key}_max"] = float(curve[local_idx])
            metrics[f"ccf_{key}_max_lag"] = int(LAGS[local_idx])
        else:
            metrics[f"ccf_{key}_max"] = math.nan
            metrics[f"ccf_{key}_max_lag"] = math.nan

    # SF09: bad debt following private debt at positive lags.
    debt_bad = ccfs["debt_bad_debt"]
    mask = (LAGS >= 1) & (LAGS <= MAX_CCF_LAG)
    vals, lags = debt_bad[mask], LAGS[mask]
    if np.any(np.isfinite(vals)):
        idx = np.nanargmax(vals)
        metrics["sf09_debt_leads_bad_debt_max_corr"] = float(vals[idx])
        metrics["sf09_debt_leads_bad_debt_lag"] = int(lags[idx])
    else:
        metrics["sf09_debt_leads_bad_debt_max_corr"] = math.nan
        metrics["sf09_debt_leads_bad_debt_lag"] = math.nan

    plot_series = {"t": post[:, 0], "gdp": gdp, "gdp_growth": gdp_growth}
    return metrics, ccfs, plot_series


# Snapshot/micro diagnostics

def heterogeneity_metrics(path):
    data = load_matrix(path)
    times, post = snapshot_post_burn(data)
    if post.size == 0:
        empty = {"mean_sd": math.nan, "min_sd": math.nan,
                 "initial_sd": math.nan, "final_sd": math.nan,
                 "final_to_initial": math.nan, "sd_slope": math.nan}
        return empty, [], []

    sds, valid_times = [], []
    for tt, row in zip(times, post):
        x = finite_array(row)
        if len(x) >= 2:
            sds.append(float(np.std(x, ddof=1)))
            valid_times.append(float(tt))

    if not sds:
        empty = {"mean_sd": math.nan, "min_sd": math.nan,
                 "initial_sd": math.nan, "final_sd": math.nan,
                 "final_to_initial": math.nan, "sd_slope": math.nan}
        return empty, [], []

    initial, final = sds[0], sds[-1]
    metrics = {
        "mean_sd": float(np.mean(sds)),
        "min_sd": float(np.min(sds)),
        "initial_sd": float(initial),
        "final_sd": float(final),
        "final_to_initial": float(final / initial) if initial > 0 else math.nan,
        "sd_slope": slope_against_time(valid_times, sds),
    }
    return metrics, valid_times, sds


def firm_growth_metrics(path):
    data = load_matrix(path)
    _, post = snapshot_post_burn(data)

    empty = {
        "excess_kurtosis": math.nan,
        "tail_ratio_2sd": math.nan,
        "gennorm_beta": math.nan,
        "aic_gaussian": math.nan,
        "aic_laplace": math.nan,
        "aic_gennorm": math.nan,
        "delta_aic_gaussian_minus_gennorm": math.nan,
        "delta_aic_gaussian_minus_laplace": math.nan,
    }

    if post.size == 0:
        return empty, np.array([])

    x = finite_array(post.ravel())
    ft = fat_tail_metrics(x)

    return {
        "excess_kurtosis": ft["excess_kurtosis"],
        "tail_ratio_2sd": ft["tail_ratio_2sd"],
        "gennorm_beta": ft["gennorm_beta"],
        "aic_gaussian": ft["aic_gaussian"],
        "aic_laplace": ft["aic_laplace"],
        "aic_gennorm": ft["aic_gennorm"],
        "delta_aic_gaussian_minus_gennorm":
            ft["delta_aic_gaussian_minus_gennorm"],
        "delta_aic_gaussian_minus_laplace":
            ft["delta_aic_gaussian_minus_laplace"],
    }, x

def investment_lumpiness_metrics(path):
    data = load_matrix(path)
    _, post = snapshot_post_burn(data)
    if post.size == 0:
        empty = {"zero_share": math.nan, "positive_cv": math.nan,
                 "top10_share": math.nan, "p95_to_positive_median": math.nan}
        return empty, np.array([])

    x = finite_array(post.ravel())
    x = x[x >= 0]
    if len(x) == 0:
        empty = {"zero_share": math.nan, "positive_cv": math.nan,
                 "top10_share": math.nan, "p95_to_positive_median": math.nan}
        return empty, x

    zero_share = float(np.mean(x == 0))
    positive = x[x > 0]
    if len(positive) >= 2 and np.mean(positive) > 0:
        positive_cv = float(np.std(positive, ddof=1) / np.mean(positive))
        med = float(np.median(positive))
        p95_ratio = float(np.percentile(positive, 95) / med) if med > 0 else math.nan
    else:
        positive_cv = math.nan
        p95_ratio = math.nan

    return {"zero_share": zero_share, "positive_cv": positive_cv,
            "top10_share": top_share(x, 0.10),
            "p95_to_positive_median": p95_ratio}, x


# Operational Table A1 checklist

def mean_metric(summary_lookup, scenario, metric):
    row = summary_lookup.get((scenario, metric))
    return row["mean"] if row else math.nan


def support_fat_tail(kurt_value, beta_value):
    """
    Operational support rule for SF02/SF10.

    Reissl et al. state these as qualitative fat-tail facts and do not specify
    a numerical acceptance threshold. We therefore require:
      1. positive excess kurtosis; and
      2. fitted generalized-normal beta < 2, where beta=2 is Gaussian.

    The >2-sigma tail ratio is excluded from pass/fail.
    """
    return (
        math.isfinite(kurt_value)
        and math.isfinite(beta_value)
        and kurt_value > FAT_TAIL_MIN_EXCESS_KURTOSIS
        and beta_value < FAT_TAIL_MAX_BETA
    )

def support_heterogeneity(summary_lookup, scenario, prefix):
    c_min = mean_metric(summary_lookup, scenario, f"{prefix}_c_min_sd")
    k_min = mean_metric(summary_lookup, scenario, f"{prefix}_k_min_sd")
    c_ratio = mean_metric(summary_lookup, scenario, f"{prefix}_c_final_to_initial")
    k_ratio = mean_metric(summary_lookup, scenario, f"{prefix}_k_final_to_initial")
    return (math.isfinite(c_min) and math.isfinite(k_min)
            and math.isfinite(c_ratio) and math.isfinite(k_ratio)
            and c_min > HETEROGENEITY_EPS and k_min > HETEROGENEITY_EPS
            and c_ratio >= HETEROGENEITY_MIN_FINAL_TO_INITIAL
            and k_ratio >= HETEROGENEITY_MIN_FINAL_TO_INITIAL)


def build_checklist(summary_lookup, scenarios):
    rows = []

    def evidence_for(scenario, sf):
        if sf == "SF01":
            growth = mean_metric(summary_lookup, scenario, "sf01_annual_gdp_growth")
            sd = mean_metric(summary_lookup, scenario, "sf01_gdp_cycle_sd")
            acf = mean_metric(summary_lookup, scenario, "sf01_gdp_cycle_acf1")
            supported = (math.isfinite(growth) and growth > 0 and
                         math.isfinite(sd) and sd > 0 and
                         math.isfinite(acf) and acf > 0)
            evidence = (f"annual GDP growth={fmt(growth)}; cycle SD={fmt(sd)}; "
                        f"ACF(1)={fmt(acf)}")
            criterion = "positive long-run growth; non-zero and persistent GDP cycle"

        elif sf == "SF02":
            k = mean_metric(
                summary_lookup, scenario, "sf02_gdp_growth_excess_kurtosis"
            )
            beta = mean_metric(
                summary_lookup, scenario, "sf02_gdp_growth_gennorm_beta"
            )
            tr = mean_metric(
                summary_lookup, scenario, "sf02_gdp_growth_tail_ratio_2sd"
            )
            daic = mean_metric(
                summary_lookup, scenario,
                "sf02_gdp_growth_delta_aic_gaussian_minus_gennorm"
            )
            supported = support_fat_tail(k, beta)
            evidence = (
                f"excess kurtosis={fmt(k)}; generalized-normal beta={fmt(beta)}; "
                f"Gaussian AIC - GenNorm AIC={fmt(daic)}; "
                f">2sd tail ratio={fmt(tr)} (descriptive only)"
            )
            criterion = (
                "positive excess kurtosis and generalized-normal beta < 2; "
                ">2sd tail ratio is descriptive only"
            )

        elif sf == "SF03":
            c = mean_metric(summary_lookup, scenario, "sf03_consumption_to_gdp_vol")
            inv = mean_metric(summary_lookup, scenario, "sf03_investment_to_gdp_vol")
            emp = mean_metric(summary_lookup, scenario, "sf03_employment_to_gdp_vol")
            inf = mean_metric(summary_lookup, scenario, "sf03_inflation_to_gdp_vol")
            supported = (math.isfinite(c) and c < 1 and
                         math.isfinite(inv) and inv > 1)
            evidence = (f"C/GDP={fmt(c)}; I/GDP={fmt(inv)}; "
                        f"Emp/GDP={fmt(emp)}; Inf/GDP={fmt(inf)}")
            criterion = "consumption less volatile and investment more volatile than GDP"

        elif sf == "SF04":
            c = mean_metric(summary_lookup, scenario, "ccf_gdp_consumption_lag0")
            inv = mean_metric(summary_lookup, scenario, "ccf_gdp_investment_lag0")
            emp = mean_metric(summary_lookup, scenario, "ccf_gdp_employment_lag0")
            inf = mean_metric(summary_lookup, scenario, "ccf_gdp_inflation_lag0")
            supported = (math.isfinite(c) and c > 0 and
                         math.isfinite(inv) and inv > 0 and
                         math.isfinite(emp) and emp > 0)
            evidence = (f"lag-0 corr: C={fmt(c)}, I={fmt(inv)}, "
                        f"Emp={fmt(emp)}, Inf={fmt(inf)}")
            criterion = ("core macro co-movement: consumption, investment and "
                         "employment pro-cyclical; inflation reported descriptively")

        elif sf == "SF05":
            c = mean_metric(summary_lookup, scenario, "ccf_gdp_private_debt_lag0")
            supported = math.isfinite(c) and c > 0
            evidence = f"GDP-private debt lag-0 corr={fmt(c)}"
            criterion = "positive filtered GDP-private-debt correlation"

        elif sf == "SF06":
            c = mean_metric(summary_lookup, scenario, "ccf_gdp_rd_lag0")
            supported = math.isfinite(c) and c > 0
            evidence = f"GDP-R&D lag-0 corr={fmt(c)}"
            criterion = "positive filtered GDP-R&D correlation"

        elif sf == "SF07":
            c = mean_metric(summary_lookup, scenario, "ccf_gdp_energy_lag0")
            mx = mean_metric(summary_lookup, scenario, "ccf_gdp_energy_max")
            lag = mean_metric(summary_lookup, scenario, "ccf_gdp_energy_max_lag")
            supported = math.isfinite(mx) and mx > 0
            evidence = f"lag-0 corr={fmt(c)}; max corr={fmt(mx)} at lag {fmt(lag,1)}"
            criterion = "positive GDP-energy-demand business-cycle correlation"

        elif sf == "SF08":
            c = mean_metric(summary_lookup, scenario, "ccf_gdp_emissions_lag0")
            mx = mean_metric(summary_lookup, scenario, "ccf_gdp_emissions_max")
            lag = mean_metric(summary_lookup, scenario, "ccf_gdp_emissions_max_lag")
            supported = math.isfinite(mx) and mx > 0
            evidence = f"lag-0 corr={fmt(c)}; max corr={fmt(mx)} at lag {fmt(lag,1)}"
            criterion = "positive GDP-emissions business-cycle correlation"

        elif sf == "SF09":
            mx = mean_metric(summary_lookup, scenario, "sf09_debt_leads_bad_debt_max_corr")
            lag = mean_metric(summary_lookup, scenario, "sf09_debt_leads_bad_debt_lag")
            supported = math.isfinite(mx) and mx > 0
            evidence = f"max corr with bad debt following loans={fmt(mx)}; lag={fmt(lag,1)}"
            criterion = "positive correlation when bad debt follows private debt"

        elif sf == "SF10":
            ck = mean_metric(
                summary_lookup, scenario, "sf10_c_growth_excess_kurtosis"
            )
            cb = mean_metric(
                summary_lookup, scenario, "sf10_c_growth_gennorm_beta"
            )
            ct = mean_metric(
                summary_lookup, scenario, "sf10_c_growth_tail_ratio_2sd"
            )
            cdaic = mean_metric(
                summary_lookup, scenario,
                "sf10_c_growth_delta_aic_gaussian_minus_gennorm"
            )

            kk = mean_metric(
                summary_lookup, scenario, "sf10_k_growth_excess_kurtosis"
            )
            kb = mean_metric(
                summary_lookup, scenario, "sf10_k_growth_gennorm_beta"
            )
            kt = mean_metric(
                summary_lookup, scenario, "sf10_k_growth_tail_ratio_2sd"
            )
            kdaic = mean_metric(
                summary_lookup, scenario,
                "sf10_k_growth_delta_aic_gaussian_minus_gennorm"
            )

            supported = (
                support_fat_tail(ck, cb)
                and support_fat_tail(kk, kb)
            )
            evidence = (
                f"C-firms: kurt={fmt(ck)}, beta={fmt(cb)}, "
                f"dAIC={fmt(cdaic)}, >2sd={fmt(ct)}; "
                f"K-firms: kurt={fmt(kk)}, beta={fmt(kb)}, "
                f"dAIC={fmt(kdaic)}, >2sd={fmt(kt)}"
            )
            criterion = (
                "positive excess kurtosis and generalized-normal beta < 2 "
                "for both C- and K-firm growth; >2sd ratios descriptive only"
            )

        elif sf == "SF11":
            z = mean_metric(summary_lookup, scenario, "sf11_zero_share")
            top = mean_metric(summary_lookup, scenario, "sf11_top10_share")
            cv = mean_metric(summary_lookup, scenario, "sf11_positive_cv")
            supported = (math.isfinite(z) and z >= LUMPY_MIN_ZERO_SHARE and
                         math.isfinite(top) and top >= LUMPY_MIN_TOP10_SHARE)
            evidence = (f"zero-investment share={fmt(z)}; top-10% investment "
                        f"share={fmt(top)}; positive CV={fmt(cv)}")
            criterion = ("operational proxy: many zero observations and "
                         "investment concentrated in large spikes")

        elif sf == "SF12":
            supported = support_heterogeneity(summary_lookup, scenario,
                                               "sf12_productivity")
            c = mean_metric(summary_lookup, scenario, "sf12_productivity_c_mean_sd")
            k = mean_metric(summary_lookup, scenario, "sf12_productivity_k_mean_sd")
            cr = mean_metric(summary_lookup, scenario,
                             "sf12_productivity_c_final_to_initial")
            kr = mean_metric(summary_lookup, scenario,
                             "sf12_productivity_k_final_to_initial")
            evidence = (f"mean cross-sectional SD C={fmt(c)}, K={fmt(k)}; "
                        f"final/initial dispersion C={fmt(cr)}, K={fmt(kr)}")
            criterion = "cross-firm productivity dispersion remains non-zero"

        elif sf == "SF13":
            supported = support_heterogeneity(summary_lookup, scenario,
                                               "sf13_energy_eff")
            c = mean_metric(summary_lookup, scenario, "sf13_energy_eff_c_mean_sd")
            k = mean_metric(summary_lookup, scenario, "sf13_energy_eff_k_mean_sd")
            cr = mean_metric(summary_lookup, scenario,
                             "sf13_energy_eff_c_final_to_initial")
            kr = mean_metric(summary_lookup, scenario,
                             "sf13_energy_eff_k_final_to_initial")
            evidence = (f"mean cross-sectional SD C={fmt(c)}, K={fmt(k)}; "
                        f"final/initial dispersion C={fmt(cr)}, K={fmt(kr)}")
            criterion = "cross-firm energy-efficiency dispersion remains non-zero"

        elif sf == "SF14":
            supported = support_heterogeneity(summary_lookup, scenario,
                                               "sf14_emiss_int")
            c = mean_metric(summary_lookup, scenario, "sf14_emiss_int_c_mean_sd")
            k = mean_metric(summary_lookup, scenario, "sf14_emiss_int_k_mean_sd")
            cr = mean_metric(summary_lookup, scenario,
                             "sf14_emiss_int_c_final_to_initial")
            kr = mean_metric(summary_lookup, scenario,
                             "sf14_emiss_int_k_final_to_initial")
            evidence = (f"mean cross-sectional SD C={fmt(c)}, K={fmt(k)}; "
                        f"final/initial dispersion C={fmt(cr)}, K={fmt(kr)}")
            criterion = "cross-firm emission-intensity dispersion remains non-zero"

        else:
            raise KeyError(sf)

        return bool(supported), evidence, criterion

    for sf, label in STYLISED_FACTS:
        row = {"stylised_fact_id": sf, "stylised_fact": label}
        supports = {}
        for scenario in scenarios:
            supported, evidence, criterion = evidence_for(scenario, sf)
            supports[scenario] = supported
            row[f"{scenario}_supported"] = supported
            row[f"{scenario}_evidence"] = evidence
            row["operational_criterion"] = criterion

        if len(scenarios) == 2:
            a, b = scenarios
            row["preserved_in_scenario2"] = (
                bool(supports[b]) if supports[a]
                else "INCONCLUSIVE_BASELINE_FAIL"
            )
        else:
            row["preserved_in_scenario2"] = "NA"
        rows.append(row)

    return rows


# Plotting

def save_line_plot(x, series_dict, title, xlabel, ylabel, path,
                   horizontal_zero=False):
    if not HAVE_MATPLOTLIB:
        return
    plt.figure(figsize=(8, 5))
    for label, y in series_dict.items():
        plt.plot(x, y, label=label)
    if horizontal_zero:
        plt.axhline(0.0, linewidth=0.8)
    plt.title(title)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.legend()
    plt.tight_layout()
    plt.savefig(path, dpi=180)
    plt.close()


def save_histogram(series_dict, title, xlabel, path, bins=60):
    if not HAVE_MATPLOTLIB:
        return
    plt.figure(figsize=(8, 5))
    for label, values in series_dict.items():
        z = standardised_values(values)
        if len(z):
            plt.hist(z, bins=bins, density=True, histtype="step", label=label)
    plt.title(title)
    plt.xlabel(xlabel)
    plt.ylabel("Density")
    plt.legend()
    plt.tight_layout()
    plt.savefig(path, dpi=180)
    plt.close()


def save_distribution_fit_plot(series_dict, title, xlabel, path, bins=60):
    """Empirical standardized density with Gaussian and generalized-normal fits."""
    if not HAVE_MATPLOTLIB:
        return

    plt.figure(figsize=(8, 5))
    grid = np.linspace(-6.0, 6.0, 600)

    plt.plot(grid, norm.pdf(grid), linestyle="--", label="Gaussian reference")

    for label, values in series_dict.items():
        z = standardised_values(values)
        if len(z) < 8:
            continue

        plt.hist(
            z, bins=bins, density=True, histtype="step",
            label=f"{label} empirical"
        )

        try:
            beta_hat, loc_hat, scale_hat = gennorm.fit(z)
            plt.plot(
                grid,
                gennorm.pdf(
                    grid, beta_hat, loc=loc_hat, scale=scale_hat
                ),
                label=f"{label} GenNorm (beta={beta_hat:.2f})",
            )
        except Exception:
            pass

    plt.xlim(-6, 6)
    plt.title(title)
    plt.xlabel(xlabel)
    plt.ylabel("Density")
    plt.legend()
    plt.tight_layout()
    plt.savefig(path, dpi=180)
    plt.close()


def save_bar_plot(labels, scenario_values, title, ylabel, path):
    if not HAVE_MATPLOTLIB:
        return
    x = np.arange(len(labels), dtype=float)
    n = len(scenario_values)
    width = 0.8 / max(1, n)
    plt.figure(figsize=(8, 5))
    for idx, (scenario, values) in enumerate(scenario_values.items()):
        offset = (idx - (n - 1) / 2.0) * width
        plt.bar(x + offset, values, width=width, label=scenario)
    plt.xticks(x, labels)
    plt.title(title)
    plt.ylabel(ylabel)
    plt.legend()
    plt.tight_layout()
    plt.savefig(path, dpi=180)
    plt.close()


# Main

def main():
    if not ROOT_DIR.exists():
        raise SystemExit(f"ROOT_DIR does not exist: {ROOT_DIR}")

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    PLOT_DIR.mkdir(parents=True, exist_ok=True)

    validation1_files = sorted(ROOT_DIR.rglob("validation1_*.txt"))
    validation1_files = [p for p in validation1_files
                         if re.fullmatch(r"validation1_\d+\.txt", p.name)]
    if not validation1_files:
        raise SystemExit(f"No validation1_<seed>.txt files found below {ROOT_DIR}")

    run_rows = []
    ccf_store = defaultdict(lambda: defaultdict(list))
    macro_plot_store = defaultdict(list)
    pooled_gdp_growth = defaultdict(list)
    pooled_c_growth = defaultdict(list)
    pooled_k_growth = defaultdict(list)
    pooled_investment = defaultdict(list)
    heterogeneity_snapshot_rows = []
    seen = set()

    for idx, path in enumerate(validation1_files, start=1):
        scenario = get_scenario(path, ROOT_DIR)
        seed = get_seed(path)
        key = (scenario, seed)
        if key in seen:
            raise SystemExit(f"Duplicate scenario/seed found: {scenario}, seed {seed}")
        seen.add(key)

        try:
            macro_metrics, ccfs, macro_series = read_macro_run(path)
        except Exception as exc:
            print(f"WARNING: skipping macro file {path}: {exc}")
            continue

        row = {"scenario": scenario, "seed": seed,
               "validation1_file": str(path), **macro_metrics}

        for relation, curve in ccfs.items():
            ccf_store[scenario][relation].append(curve)

        macro_plot_store[scenario].append((macro_series["t"], macro_series["gdp"]))
        pooled_gdp_growth[scenario].extend(macro_series["gdp_growth"].tolist())

        # SF10: firm growth distributions.
        for validation_no, sector in ((11, "c"), (12, "k")):
            p = related_validation_path(path, validation_no, seed)
            if p.exists():
                m, values = firm_growth_metrics(p)
                row[f"sf10_{sector}_growth_excess_kurtosis"] = m["excess_kurtosis"]
                row[f"sf10_{sector}_growth_tail_ratio_2sd"] = m["tail_ratio_2sd"]
                row[f"sf10_{sector}_growth_gennorm_beta"] = m["gennorm_beta"]
                row[f"sf10_{sector}_growth_aic_gaussian"] = m["aic_gaussian"]
                row[f"sf10_{sector}_growth_aic_laplace"] = m["aic_laplace"]
                row[f"sf10_{sector}_growth_aic_gennorm"] = m["aic_gennorm"]
                row[
                    f"sf10_{sector}_growth_delta_aic_gaussian_minus_gennorm"
                ] = m["delta_aic_gaussian_minus_gennorm"]
                row[
                    f"sf10_{sector}_growth_delta_aic_gaussian_minus_laplace"
                ] = m["delta_aic_gaussian_minus_laplace"]
                if sector == "c":
                    pooled_c_growth[scenario].extend(values.tolist())
                else:
                    pooled_k_growth[scenario].extend(values.tolist())
            else:
                print(f"WARNING: missing {p}")
                row[f"sf10_{sector}_growth_excess_kurtosis"] = math.nan
                row[f"sf10_{sector}_growth_tail_ratio_2sd"] = math.nan
                row[f"sf10_{sector}_growth_gennorm_beta"] = math.nan
                row[f"sf10_{sector}_growth_aic_gaussian"] = math.nan
                row[f"sf10_{sector}_growth_aic_laplace"] = math.nan
                row[f"sf10_{sector}_growth_aic_gennorm"] = math.nan
                row[
                    f"sf10_{sector}_growth_delta_aic_gaussian_minus_gennorm"
                ] = math.nan
                row[
                    f"sf10_{sector}_growth_delta_aic_gaussian_minus_laplace"
                ] = math.nan

        # SF11: lumpy C-firm investment.
        p10 = related_validation_path(path, 10, seed)
        if p10.exists():
            m, values = investment_lumpiness_metrics(p10)
            row["sf11_zero_share"] = m["zero_share"]
            row["sf11_positive_cv"] = m["positive_cv"]
            row["sf11_top10_share"] = m["top10_share"]
            row["sf11_p95_to_positive_median"] = m["p95_to_positive_median"]
            pooled_investment[scenario].extend(values.tolist())
        else:
            print(f"WARNING: missing {p10}")
            row["sf11_zero_share"] = math.nan
            row["sf11_positive_cv"] = math.nan
            row["sf11_top10_share"] = math.nan
            row["sf11_p95_to_positive_median"] = math.nan

        # SF12-SF14: persistent cross-firm heterogeneity.
        heterogeneity_files = {
            "sf12_productivity_c": 4,
            "sf12_productivity_k": 5,
            "sf13_energy_eff_c": 6,
            "sf14_emiss_int_c": 7,
            "sf13_energy_eff_k": 8,
            "sf14_emiss_int_k": 9,
        }
        for prefix, validation_no in heterogeneity_files.items():
            p = related_validation_path(path, validation_no, seed)
            if p.exists():
                hm, times, sds = heterogeneity_metrics(p)
                for k, value in hm.items():
                    row[f"{prefix}_{k}"] = value
                for tt, sd in zip(times, sds):
                    heterogeneity_snapshot_rows.append({
                        "scenario": scenario, "seed": seed,
                        "series": prefix, "time": tt,
                        "cross_sectional_sd": sd,
                    })
            else:
                print(f"WARNING: missing {p}")
                for k in ("mean_sd", "min_sd", "initial_sd", "final_sd",
                          "final_to_initial", "sd_slope"):
                    row[f"{prefix}_{k}"] = math.nan

        run_rows.append(row)
        if idx % 25 == 0 or idx == len(validation1_files):
            print(f"Processed {idx}/{len(validation1_files)} runs")

    if not run_rows:
        raise SystemExit("No usable runs were found.")

    scenarios = sorted({r["scenario"] for r in run_rows})
    print("\nRuns found by scenario:")
    for scenario in scenarios:
        n = sum(r["scenario"] == scenario for r in run_rows)
        print(f"  {scenario}: {n}")
    if len(scenarios) != 2:
        print("\nWARNING: comparison is designed for exactly two scenarios.")

    # Run-level metrics.
    run_metric_names = sorted({k for row in run_rows for k in row
                               if k not in {"scenario", "seed", "validation1_file"}})
    run_csv = OUTPUT_DIR / "stylised_facts_run_metrics.csv"
    with run_csv.open("w", newline="") as f:
        fields = ["scenario", "seed", "validation1_file", *run_metric_names]
        writer = csv.DictWriter(f, fieldnames=fields)
        writer.writeheader()
        writer.writerows(run_rows)

    # Scenario-level summaries.
    metric_summary_rows = []
    for scenario in scenarios:
        subset = [r for r in run_rows if r["scenario"] == scenario]
        for metric in run_metric_names:
            vals = []
            for r in subset:
                value = r.get(metric, math.nan)
                if isinstance(value, (int, float, np.number)):
                    vals.append(value)
            sm = mean_ci(vals)
            metric_summary_rows.append({"scenario": scenario, "metric": metric, **sm})

    metric_summary_csv = OUTPUT_DIR / "stylised_facts_metric_summary.csv"
    with metric_summary_csv.open("w", newline="") as f:
        fields = ["scenario", "metric", "n", "mean", "sd", "se",
                  "lower95", "upper95"]
        writer = csv.DictWriter(f, fieldnames=fields)
        writer.writeheader()
        writer.writerows(metric_summary_rows)

    summary_lookup = {(r["scenario"], r["metric"]): r
                      for r in metric_summary_rows}

    # Cross-correlation curves.
    ccf_rows = []
    for scenario in scenarios:
        for relation, (description, _, _) in CCF_RELATIONS.items():
            curves = ccf_store[scenario][relation]
            if not curves:
                continue
            arr = np.asarray(curves, dtype=float)
            for lag_idx, lag in enumerate(LAGS):
                sm = mean_ci(arr[:, lag_idx])
                ccf_rows.append({"scenario": scenario, "relation": relation,
                                 "description": description, "lag": int(lag), **sm})

    ccf_csv = OUTPUT_DIR / "cross_correlations.csv"
    with ccf_csv.open("w", newline="") as f:
        fields = ["scenario", "relation", "description", "lag", "n", "mean",
                  "sd", "se", "lower95", "upper95"]
        writer = csv.DictWriter(f, fieldnames=fields)
        writer.writeheader()
        writer.writerows(ccf_rows)

    # Heterogeneity by snapshot.
    grouped = defaultdict(list)
    for r in heterogeneity_snapshot_rows:
        grouped[(r["scenario"], r["series"], r["time"])].append(
            r["cross_sectional_sd"])

    heterogeneity_summary_rows = []
    for (scenario, series, time), values in sorted(grouped.items()):
        sm = mean_ci(values)
        heterogeneity_summary_rows.append({"scenario": scenario, "series": series,
                                           "time": time, **sm})

    hetero_csv = OUTPUT_DIR / "heterogeneity_by_snapshot.csv"
    with hetero_csv.open("w", newline="") as f:
        fields = ["scenario", "series", "time", "n", "mean", "sd", "se",
                  "lower95", "upper95"]
        writer = csv.DictWriter(f, fieldnames=fields)
        writer.writeheader()
        writer.writerows(heterogeneity_summary_rows)

    # Pooled SF02/SF10 distribution fits.
    fat_tail_fit_rows = []

    pooled_series = {
        "SF02_GDP_growth": pooled_gdp_growth,
        "SF10_C_firm_growth": pooled_c_growth,
        "SF10_K_firm_growth": pooled_k_growth,
    }

    for series_name, scenario_dict in pooled_series.items():
        for scenario in scenarios:
            values = scenario_dict.get(scenario, [])
            m = fat_tail_metrics(values)
            fat_tail_fit_rows.append({
                "scenario": scenario,
                "series": series_name,
                "n": m["n"],
                "excess_kurtosis": m["excess_kurtosis"],
                "tail_ratio_2sd_descriptive": m["tail_ratio_2sd"],
                "gennorm_beta": m["gennorm_beta"],
                "aic_gaussian": m["aic_gaussian"],
                "aic_laplace": m["aic_laplace"],
                "aic_gennorm": m["aic_gennorm"],
                "delta_aic_gaussian_minus_gennorm":
                    m["delta_aic_gaussian_minus_gennorm"],
                "delta_aic_gaussian_minus_laplace":
                    m["delta_aic_gaussian_minus_laplace"],
                "operational_fat_tail_support": support_fat_tail(
                    m["excess_kurtosis"], m["gennorm_beta"]
                ),
            })

    fat_tail_fit_csv = OUTPUT_DIR / "fat_tail_distribution_fits.csv"
    with fat_tail_fit_csv.open("w", newline="") as f:
        fields = [
            "scenario", "series", "n", "excess_kurtosis",
            "tail_ratio_2sd_descriptive", "gennorm_beta",
            "aic_gaussian", "aic_laplace", "aic_gennorm",
            "delta_aic_gaussian_minus_gennorm",
            "delta_aic_gaussian_minus_laplace",
            "operational_fat_tail_support",
        ]
        writer = csv.DictWriter(f, fieldnames=fields)
        writer.writeheader()
        writer.writerows(fat_tail_fit_rows)

    # Table A1 checklist.
    checklist_rows = build_checklist(summary_lookup, scenarios)
    checklist_csv = OUTPUT_DIR / "stylised_facts_checklist.csv"
    checklist_fields = ["stylised_fact_id", "stylised_fact", "operational_criterion"]
    for scenario in scenarios:
        checklist_fields += [f"{scenario}_supported", f"{scenario}_evidence"]
    checklist_fields += ["preserved_in_scenario2"]
    with checklist_csv.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=checklist_fields)
        writer.writeheader()
        writer.writerows(checklist_rows)

    # Plots.
    if HAVE_MATPLOTLIB:
        gdp_series, common_t = {}, None
        for scenario in scenarios:
            paths = macro_plot_store[scenario]
            if not paths:
                continue
            common_t = paths[0][0]
            normalised = []
            for t, gdp in paths:
                if len(t) == len(common_t) and gdp[0] > 0:
                    normalised.append(gdp / gdp[0])
            if normalised:
                gdp_series[scenario] = np.mean(normalised, axis=0)
        if common_t is not None and gdp_series:
            save_line_plot(common_t, gdp_series,
                           "SF01: Mean real GDP path after transient", "Quarter",
                           "Real GDP, normalised to 1 at first retained quarter",
                           PLOT_DIR / "sf01_gdp_paths.png")

        save_distribution_fit_plot(
            pooled_gdp_growth,
            "SF02: Standardised quarterly GDP growth",
            "Standardised quarterly log GDP growth",
            PLOT_DIR / "sf02_gdp_growth_distribution_fit.png",
        )

        rel_vol_values = {}
        for scenario in scenarios:
            rel_vol_values[scenario] = [
                mean_metric(summary_lookup, scenario, "sf03_consumption_to_gdp_vol"),
                mean_metric(summary_lookup, scenario, "sf03_investment_to_gdp_vol"),
                mean_metric(summary_lookup, scenario, "sf03_employment_to_gdp_vol"),
                mean_metric(summary_lookup, scenario, "sf03_inflation_to_gdp_vol"),
            ]
        save_bar_plot(["Consumption", "Investment", "Employment", "Inflation"],
                      rel_vol_values, "SF03: Volatility relative to GDP",
                      "SD(variable cycle) / SD(GDP cycle)",
                      PLOT_DIR / "sf03_relative_volatility.png")

        ccf_mean_lookup = defaultdict(dict)
        for r in ccf_rows:
            ccf_mean_lookup[(r["scenario"], r["relation"])][r["lag"]] = r["mean"]
        for relation, (description, _, _) in CCF_RELATIONS.items():
            series_dict = {}
            for scenario in scenarios:
                series_dict[scenario] = [
                    ccf_mean_lookup[(scenario, relation)].get(int(lag), math.nan)
                    for lag in LAGS]
            save_line_plot(LAGS, series_dict, description,
                           "Lag in quarters; positive lag = second variable follows first",
                           "Correlation", PLOT_DIR / f"{relation}_ccf.png",
                           horizontal_zero=True)

        save_distribution_fit_plot(
            pooled_c_growth,
            "SF10: Standardised C-firm growth distribution",
            "Standardised C-firm growth",
            PLOT_DIR / "sf10_cfirm_growth_distribution_fit.png",
        )
        save_distribution_fit_plot(
            pooled_k_growth,
            "SF10: Standardised K-firm growth distribution",
            "Standardised K-firm growth",
            PLOT_DIR / "sf10_kfirm_growth_distribution_fit.png",
        )

        positive_inv = {}
        for scenario, values in pooled_investment.items():
            x = finite_array(values)
            positive_inv[scenario] = x[x > 0]
        save_histogram(positive_inv,
                       "SF11: Distribution of positive C-firm investment observations",
                       "Standardised positive investment",
                       PLOT_DIR / "sf11_positive_investment_distribution.png")

        series_names = sorted({r["series"] for r in heterogeneity_summary_rows})
        hetero_lookup = defaultdict(dict)
        for r in heterogeneity_summary_rows:
            hetero_lookup[(r["scenario"], r["series"])][r["time"]] = r["mean"]
        for series in series_names:
            times = sorted({r["time"] for r in heterogeneity_summary_rows
                            if r["series"] == series})
            series_dict = {}
            for scenario in scenarios:
                series_dict[scenario] = [
                    hetero_lookup[(scenario, series)].get(tt, math.nan)
                    for tt in times]
            save_line_plot(times, series_dict, series.replace("_", " "),
                           "Quarter", "Cross-sectional SD of log deviation",
                           PLOT_DIR / f"{series}_dispersion.png")
    else:
        print("\nWARNING: matplotlib not installed; CSVs created, plots skipped.")

    # Console summary.
    print("\nTable A1 stylised-fact comparison")
    print("=" * 120)
    if len(scenarios) == 2:
        a, b = scenarios
        print(f"{'ID':5s} {'Stylised fact':58s} {a:>12s} {b:>12s} {'Preserved':>12s}")
        print("-" * 120)
        for row in checklist_rows:
            pa = row[f"{a}_supported"]
            pb = row[f"{b}_supported"]
            preserved = row["preserved_in_scenario2"]
            print(f"{row['stylised_fact_id']:5s} {row['stylised_fact'][:58]:58s} "
                  f"{fmt(pa):>12s} {fmt(pb):>12s} {str(preserved):>12s}")
    else:
        print("Exactly two scenarios are recommended for comparison.")

    print("\nInterpretation notes")
    print("--------------------")
    print("1. These are operational diagnostics for qualitative stylised facts, not formal tests from Reissl et al.")
    print("2. CCF convention: corr(x_t, y_{t+lag}); positive lag means the second variable follows the first.")
    print("3. SF11 uses C-firm investment I(j) from validation10 as a proxy. The validation output lacks firm capital, so exact I/K cannot be constructed.")
    print("4. SF12-SF14 use the cross-sectional log deviations already written to validation4-validation9.")
    print("5. SF02/SF10: generalized-normal beta < 2 is the primary fat-tail shape diagnostic; beta=2 is Gaussian and beta=1 is Laplace.")
    print("6. The >2-sigma tail ratio is reported only descriptively and is not used for pass/fail.")
    print("7. Positive Gaussian AIC - GenNorm AIC means the generalized-normal fit is preferred after the AIC complexity penalty.")

    print("\nOutput written to:")
    print(f"  {OUTPUT_DIR}")
    print("\nMain files:")
    print("  stylised_facts_checklist.csv")
    print("  stylised_facts_metric_summary.csv")
    print("  stylised_facts_run_metrics.csv")
    print("  cross_correlations.csv")
    print("  heterogeneity_by_snapshot.csv")
    print("  fat_tail_distribution_fits.csv")
    if HAVE_MATPLOTLIB:
        print("  plots/")


if __name__ == "__main__":
    main()
