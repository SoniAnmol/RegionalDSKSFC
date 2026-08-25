import pandas as pd
import numpy as np



# Compute GDP_r growth
def compute_gdp_r_growth(df):
    """Compute annualized GDP growth per row for panel simulation output.

    Growth is computed within each (scenario, run, region) trajectory as:
        ((GDP_r_t / GDP_r_t-1) ** 4 - 1) * 100
    where 4 annualizes quarter-over-quarter growth.

    Parameters
    ----------
    df : pandas.DataFrame
        Input frame containing at least: scenario, run, region, t, and GDP_r.

    Returns
    -------
    pandas.Series
        Annualized growth in percent, aligned to ``df.index``. The first
        timestep in each group is NaN.

    Raises
    ------
    KeyError
        If required grouping columns or a GDP column are missing.
    """
    required_group_cols = ["scenario", "run", "region", "t"]
    missing_group_cols = [col for col in required_group_cols if col not in df.columns]
    if missing_group_cols:
        raise KeyError(f"Missing required columns: {missing_group_cols}")

    gdp_col = "GDP_r" if "GDP_r" in df.columns else None
    if gdp_col is None:
        raise KeyError("Missing GDP column. Expected one of: 'GDP_r'")

    ordered = df[required_group_cols + [gdp_col]].sort_values(required_group_cols).copy()
    prev_gdp = ordered.groupby(["scenario", "run", "region"], sort=False)[gdp_col].shift(1)

    growth = ((ordered[gdp_col] / prev_gdp) ** 4 - 1) * 100
    valid = (prev_gdp > 0) & (ordered[gdp_col] > 0)
    growth = growth.where(valid, np.nan)

    return growth.reindex(df.index)



def compute_GDP_r_volatility(df):
    """Compute annualized real GDP volatility per row for panel simulation output.

    Volatility is the expanding standard deviation of annualized GDP growth
    within each (scenario, run, region) trajectory.

    Parameters
    ----------
    df : pandas.DataFrame
        Input frame containing at least: scenario, run, region, t, and GDP_r.

    Returns
    -------
    pandas.Series
        Expanding standard deviation of annualized GDP growth (percentage points),
        aligned to ``df.index``. The first timestep in each group is NaN.

    Raises
    ------
    KeyError
        If required grouping columns or a GDP column are missing.
    """
    required_group_cols = ["scenario", "run", "region", "t"]
    missing_group_cols = [col for col in required_group_cols if col not in df.columns]
    if missing_group_cols:
        raise KeyError(f"Missing required columns: {missing_group_cols}")

    gdp_col = "GDP_r" if "GDP_r" in df.columns else None
    if gdp_col is None:
        raise KeyError("Missing GDP column. Expected one of: 'GDP_r'")

    ordered = df[required_group_cols + [gdp_col]].sort_values(required_group_cols).copy()
    prev_gdp = ordered.groupby(["scenario", "run", "region"], sort=False)[gdp_col].shift(1)

    growth = ((ordered[gdp_col] / prev_gdp) ** 4 - 1) * 100
    valid = (prev_gdp > 0) & (ordered[gdp_col] > 0)
    ordered["_growth"] = growth.where(valid, np.nan)

    volatility = ordered.groupby(["scenario", "run", "region"], sort=False)["_growth"].transform(
        lambda s: s.expanding(min_periods=2).std()
    )

    return volatility.reindex(df.index)



def compute_crisis_indicator(df, threshold=-5.0):
    """Create a binary crisis indicator from annual GDP growth (%).

    A crisis is defined as GDP_r_growth < threshold. With GDP_r_growth in percent,
    Lamperti et al. (2018) threshold is -5.0.

    Returns a row-aligned Series (1 = crisis, 0 = no crisis, NaN if growth is NaN).
    """
    required_cols = ["GDP_r_growth"]
    missing_cols = [col for col in required_cols if col not in df.columns]
    if missing_cols:
        raise KeyError(f"Missing required columns: {missing_cols}")

    growth = df["GDP_r_growth"]
    out = growth.astype(float).copy()
    out.loc[:] = np.where(growth.notna(), (growth < threshold).astype(int), np.nan)
    return out.rename("crisis")

def compute_crisis_likelihood(
    df,
    group_cols=("run",),
    t_min=None,
    t_max=None,
    crisis_col="crisis",
):
    """Compute crisis likelihood by groups and optional time window.

    Likelihood = sum(crisis) / number_of_non_null_crisis_observations
    where crisis is a binary column (0/1).

    Parameters
    ----------
    df : pandas.DataFrame
        Input data containing group columns, timestep ``t`` (if filtered), and crisis column.
    group_cols : tuple[str, ...], default ("run",)
        Grouping dimensions for likelihood calculation.
    t_min : int | None, default None
        If provided, include observations with t >= t_min.
    t_max : int | None, default None
        If provided, include observations with t <= t_max.
    crisis_col : str, default "crisis"
        Name of crisis indicator column (binary 0/1).

    Returns
    -------
    pandas.Series
        Crisis likelihood by ``group_cols`` as float values in [0, 1].
    """
    required_cols = list(group_cols) + [crisis_col]
    if t_min is not None or t_max is not None:
        required_cols.append("t")

    missing_cols = [col for col in required_cols if col not in df.columns]
    if missing_cols:
        raise KeyError(f"Missing required columns: {missing_cols}")

    work = df.copy()

    # Optional timestep filtering
    if t_min is not None:
        work = work[work["t"] >= t_min]
    if t_max is not None:
        work = work[work["t"] <= t_max]

    # Use only non-null crisis observations
    valid_obs = work[crisis_col].notna()
    obs_count = work[valid_obs].groupby(list(group_cols), sort=False)[crisis_col].count()
    crisis_sum = work[valid_obs].groupby(list(group_cols), sort=False)[crisis_col].sum()

    return (crisis_sum / obs_count.replace(0, np.nan)).astype(float).rename("crisis_likelihood")

