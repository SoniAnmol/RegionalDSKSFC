#!/usr/bin/env python3
"""
Lamperti-style Comparison Table Generator

This script reads simulation output files from a multi-scenario Monte Carlo experiment
and constructs comparison tables exactly as in Lamperti et al. (2018).

Key specifications:
- Simulation length: 600 quarters total
- Climate shocks start at t = 200
- Analysis window: ONLY t = 201 to t = 600 (400 quarters = 100 years)
- Crisis threshold: annual GDP growth < -0.05
- 50 Monte Carlo runs per scenario
"""

import os
import pandas as pd
import numpy as np
from scipy import stats
from pathlib import Path


# ==============================================================================
# CONFIGURATION
# ==============================================================================

OUTPUT_DIR = "./output/verifications/"
MAPPER_FILE = "./mapper.xlsx"

# Simulation timing parameters (DO NOT CHANGE)
TOTAL_QUARTERS = 600
SHOCK_START = 200  # Shocks start at t=200
ANALYSIS_START = 201  # Analysis starts at t=201 (index 200)
ANALYSIS_END = 600  # Analysis ends at t=600 (index 599)

# Number of quarters per year (DO NOT CHANGE)
QUARTERS_PER_YEAR = 4

# Crisis threshold (DO NOT CHANGE)
CRISIS_THRESHOLD = -0.05

# Scenario mapping (exact order for output table)
SCENARIO_NAMES = {
    1: "No shocks",
    2: "Labour productivity (LP)",
    3: "Energy efficiency (EF)",
    4: "Capital stock (CS)",
    5: "Inventories (INV)",
    6: "LP & EF",
    7: "LP & CS",
    8: "CS & EF",
    9: "CS & INV"
}

# Number of runs per scenario
NUM_RUNS = 50

# Number of regions in the model
NUM_REGIONS = 3


# ==============================================================================
# HELPER FUNCTIONS
# ==============================================================================

def load_mapper(mapper_file):
    """Load the column mapper from Excel file."""
    try:
        mapper = pd.read_excel(mapper_file)
        print(f"✓ Loaded mapper with {len(mapper)} columns")
        return mapper
    except Exception as e:
        print(f"✗ Error loading mapper: {e}")
        return None


def read_simulation_file(filepath, n_cols=None):
    """
    Read a simulation output file.

    Parameters:
    -----------
    filepath : str or Path
        Path to the simulation file
    n_cols : int, optional
        Expected number of columns (for validation)

    Returns:
    --------
    pandas.DataFrame or None
    """
    try:
        # Read whitespace-separated file without headers
        df = pd.read_csv(filepath, sep=r'\s+', header=None)

        if n_cols is not None and len(df.columns) != n_cols:
            print(f"  Warning: Expected {n_cols} columns, got {len(df.columns)} in {filepath}")

        return df
    except Exception as e:
        print(f"  ✗ Error reading {filepath}: {e}")
        return None


def get_column_index(mapper, column_name):
    """Get the column index for a given variable name."""
    row = mapper[mapper['column_name'] == column_name]
    if len(row) == 0:
        return None
    return int(row['column_index'].iloc[0])


def get_regional_column_mappers():
    """
    Create column mappers for regional files.

    Returns two DataFrames:
    1. Baseline regional mapper (ymc_#_*.txt files, 24 columns)
    2. Shock scenario regional mapper (resultsexp_reg#_*.txt files, 46 columns)
    """
    # Baseline regional file structure (24 columns)
    baseline_regional_cols = {
        'column_index': [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23],
        'column_name': [
            't', 'reg_GDP_r', 'reg_Consumption_r', 'reg_Investment_r', 'reg_EmploymentRate',
            'reg_Am', 'reg_Loans_2', 'reg_Inventories', 'reg_N', 'reg_GDP_n',
            'reg_Q_ge_over_D_en', 'reg_D_en_TOT', 'reg_Emiss_TOT', 'reg_Cum_emissions',
            'reg_Q1', 'reg_Q2', 'reg_N1', 'reg_N2', 'reg_LS', 'reg_Q_ge', 'reg_Q_de',
            'reg_Emiss1_TOT', 'reg_Emiss2_TOT', 'reg_Emiss_en'
        ]
    }

    # Shock scenario regional file structure (46 columns)
    shock_regional_cols = {
        'column_index': list(range(46)),
        'column_name': [
            't', 'reg_GDP_r', 'reg_Consumption_r', 'reg_Investment_r', 'reg_EmploymentRate',
            'reg_Emiss_TOT', 'reg_D_en_TOT', 'reg_LS', 'reg_K',
            'reg_A1p_en_dead/reg_A1p_en_survive', 'reg_A2_en_dead/reg_A2_en_survive',
            'reg_Am_en', 'reg_Am_a', 'reg_exit_marketshare2', 'reg_exit_payments2',
            'reg_exit_equity2', 'reg_exiting_1', 'reg_CapitalStock/GDP', 'reg_NW_h/GDP',
            'reg_NW_2/GDP', 'reg_Loans_2/GDP', 'reg_CreditDemand/Supply', 'reg_NW_1/GDP',
            'reg_Am2', 'reg_Am1', 'reg_GDP_n', 'reg_Investment_n.dim_mach.a',
            'reg_Consumption', 'reg_ReplacementInvestment_r', 'reg_Emiss1_TOT',
            'reg_Emiss2_TOT', 'reg_Emiss_en', 'reg_EnergyPayments', 'reg_GreenCapacityShare',
            'reg_Pitot1', 'reg_Pitot2', 'reg_real_wages', 'reg_Dividends_1',
            'reg_Dividends_2', 'reg_exit_total', 'reg_Pitot_total', 'reg_Dividends_firms_real',
            'reg_Dividends_e', 'reg_Dividends_b', 'reg_NW_firms/GDP', 'reg_GDP_r_growth'
        ]
    }

    baseline_mapper = pd.DataFrame(baseline_regional_cols)
    shock_mapper = pd.DataFrame(shock_regional_cols)

    return baseline_mapper, shock_mapper


def annualize_gdp(gdp_quarterly):
    """
    Convert quarterly GDP to annual GDP by summing 4 consecutive quarters.

    Parameters:
    -----------
    gdp_quarterly : array-like
        Quarterly GDP values for the analysis window (t=201-600)

    Returns:
    --------
    numpy.array
        Annual GDP values (100 years)
    """
    # Should have exactly 400 quarters
    assert len(gdp_quarterly) == 400, f"Expected 400 quarters, got {len(gdp_quarterly)}"

    # Reshape into (100 years, 4 quarters) and sum across quarters
    gdp_quarterly = np.array(gdp_quarterly)
    annual_gdp = gdp_quarterly.reshape(100, 4).sum(axis=1)

    return annual_gdp


def compute_growth_rate(annual_gdp):
    """
    Compute annual GDP growth rate.

    Parameters:
    -----------
    annual_gdp : array-like
        Annual GDP values (100 years)

    Returns:
    --------
    numpy.array
        Growth rates for years 2-100 (99 values)
    """
    annual_gdp = np.array(annual_gdp)

    # g_y = (GDP_y - GDP_{y-1}) / GDP_{y-1}
    growth_rates = np.diff(annual_gdp) / annual_gdp[:-1]

    return growth_rates


def compute_long_run_growth(annual_gdp):
    """
    Compute Compound Annual Growth Rate (CAGR) over the full period.

    This measures long-run growth trajectory as in Lamperti et al. (2018),
    not the arithmetic mean of period-by-period growth rates.

    CAGR = (GDP_final / GDP_initial)^(1/(n-1)) - 1

    Parameters:
    -----------
    annual_gdp : array-like
        Annual GDP values (100 years)

    Returns:
    --------
    float
        Compound annual growth rate over the period
    """
    annual_gdp = np.array(annual_gdp)
    n = len(annual_gdp)
    return (annual_gdp[-1] / annual_gdp[0]) ** (1 / (n - 1)) - 1


def compute_crisis_indicator(growth_rates):
    """
    Compute crisis indicator based on annual growth rates.

    A crisis occurs if annual GDP growth < -0.05

    Parameters:
    -----------
    growth_rates : array-like
        Annual GDP growth rates

    Returns:
    --------
    float
        Mean crisis indicator (likelihood of crisis)
    """
    crisis_indicator = (np.array(growth_rates) < CRISIS_THRESHOLD).astype(int)
    return crisis_indicator.mean()


def process_run(run_folder, scenario_num, run_num, mapper, is_baseline=False):
    """
    Process a single run and compute all metrics.

    Parameters:
    -----------
    run_folder : Path
        Path to the run folder
    scenario_num : int
        Scenario number
    run_num : int
        Run number
    mapper : pandas.DataFrame
        Column mapper
    is_baseline : bool
        Whether this is the baseline scenario

    Returns:
    --------
    dict or None
        Dictionary with computed metrics
    """
    # Determine filename pattern
    if is_baseline:
        # Baseline: ymc_scenario01_repXXX_YYYY.txt
        pattern = f"ymc_scenario{scenario_num:02d}_rep{run_num:03d}_*.txt"
    else:
        # Shock scenarios: resultsexp_scenario##_rep###_####.txt
        pattern = f"resultsexp_scenario{scenario_num:02d}_rep{run_num:03d}_*.txt"

    # Find the file
    files = list(run_folder.glob(pattern))
    if len(files) == 0:
        print(f"  ✗ No file found for scenario {scenario_num}, run {run_num} in {run_folder}")
        return None
    if len(files) > 1:
        print(f"  Warning: Multiple files found for scenario {scenario_num}, run {run_num}, using first")

    filepath = files[0]

    # Read the file
    df = read_simulation_file(filepath)
    if df is None or len(df) != TOTAL_QUARTERS:
        print(
            f"  ✗ Invalid data length for {filepath}: expected {TOTAL_QUARTERS}, got {len(df) if df is not None else 0}")
        return None

    # Get column indices from mapper
    gdp_col = get_column_index(mapper, 'GDP_r_1')
    unemp_col = get_column_index(mapper, 'EmploymentRate_1')

    if gdp_col is None:
        print(f"  ✗ GDP column not found in mapper")
        return None

    # Extract analysis window (t=201-600, indices 200-599)
    analysis_data = df.iloc[ANALYSIS_START-1:ANALYSIS_END]

    # Extract quarterly GDP for analysis window
    gdp_quarterly = analysis_data.iloc[:, gdp_col].values

    # 1. Annualize GDP
    annual_gdp = annualize_gdp(gdp_quarterly)

    # 2. Compute long-run growth rate (CAGR over the century)
    # This is the Lamperti et al. (2018) approach: compound growth, not mean of annual rates
    mean_growth_rate = compute_long_run_growth(annual_gdp)

    # 3. Compute crisis likelihood (still need annual growth rates for this)
    growth_rates = compute_growth_rate(annual_gdp)
    crisis_likelihood = compute_crisis_indicator(growth_rates)

    # 4. Compute mean unemployment (quarterly, t=201-600)
    # NOTE: EmploymentRate_1 is EMPLOYMENT rate, so we invert it to get UNEMPLOYMENT
    if unemp_col is not None:
        employment_rate = analysis_data.iloc[:, unemp_col].values
        mean_unemployment = 1.0 - employment_rate.mean()  # Invert to get unemployment
    else:
        mean_unemployment = np.nan

    # 5. End-of-century GDP (year 100)
    gdp_end = annual_gdp[-1]

    return {
        'scenario': scenario_num,
        'run': run_num,
        'mean_growth_rate': mean_growth_rate,
        'crisis_likelihood': crisis_likelihood,
        'mean_unemployment': mean_unemployment,
        'gdp_end': gdp_end
    }


def process_regional_run(run_folder, scenario_num, run_num, region_id, regional_mapper, is_baseline=False):
    """
    Process a single run for a specific region using regional column mapper.

    Parameters:
    -----------
    run_folder : Path
        Root folder containing simulation outputs
    scenario_num : int
        Scenario number (1=baseline, 2-9=shock scenarios)
    run_num : int
        Monte Carlo run number
    region_id : str
        Region identifier (e.g., '1', '2', '3' or 'reg1', 'reg2', 'reg3')
    regional_mapper : pd.DataFrame
        Regional column mapper (baseline or shock structure)
    is_baseline : bool
        Whether this is a baseline scenario file

    Returns:
    --------
    dict or None
        Dictionary with computed metrics or None if file not found
    """
    # Determine filename pattern
    # Note: Baseline uses numeric IDs (ymc_1_), shock scenarios use reg prefix (resultsexp_reg1_)
    if is_baseline:
        # For baseline, use numeric region ID
        if not region_id.startswith('reg'):
            pattern = f"ymc_{region_id}_scenario{scenario_num:02d}_rep{run_num:03d}_*.txt"
        else:
            # Try without reg prefix
            num_id = region_id.replace('reg', '')
            pattern = f"ymc_{num_id}_scenario{scenario_num:02d}_rep{run_num:03d}_*.txt"
    else:
        # For shock scenarios, need reg prefix
        if not region_id.startswith('reg'):
            # Add reg prefix
            pattern = f"resultsexp_reg{region_id}_scenario{scenario_num:02d}_rep{run_num:03d}_*.txt"
        else:
            pattern = f"resultsexp_{region_id}_scenario{scenario_num:02d}_rep{run_num:03d}_*.txt"

    # Find the file
    files = list(run_folder.glob(pattern))
    if len(files) == 0:
        return None
    if len(files) > 1:
        # Use the first match
        pass

    filepath = files[0]

    # Read the file
    df = read_simulation_file(filepath)
    if df is None or len(df) != TOTAL_QUARTERS:
        return None

    # Get column indices from regional mapper
    gdp_col = get_column_index(regional_mapper, 'reg_GDP_r')
    unemp_col = get_column_index(regional_mapper, 'reg_EmploymentRate')

    if gdp_col is None:
        print(f"  ✗ Regional GDP column not found in mapper")
        return None

    # Extract analysis window (t=201-600, indices 200-599)
    analysis_data = df.iloc[ANALYSIS_START-1:ANALYSIS_END]

    # Extract quarterly GDP for analysis window
    gdp_quarterly = analysis_data.iloc[:, gdp_col].values

    # 1. Annualize GDP
    annual_gdp = annualize_gdp(gdp_quarterly)

    # 2. Compute long-run growth rate (CAGR over the century)
    mean_growth_rate = compute_long_run_growth(annual_gdp)

    # 3. Compute crisis likelihood (using annual growth rates)
    growth_rates = compute_growth_rate(annual_gdp)
    crisis_likelihood = compute_crisis_indicator(growth_rates)

    # 4. Compute mean unemployment (quarterly, t=201-600)
    # NOTE: Column interpretation differs between baseline and shock scenarios:
    # - Baseline (ymc_#_*.txt): reg_EmploymentRate stores (1-unemployment)/NR = employment/NR
    #   C++ code: target << (1.0 - reg_U[region - 1]) / NR
    # - Shock scenarios (resultsexp_reg#_*.txt): reg_EmploymentRate stores 1-unemployment = employment
    #   C++ code: target << 1 - reg_U[region - 1]
    if unemp_col is not None and unemp_col < len(df.columns):
        col_values = analysis_data.iloc[:, unemp_col].values
        col_mean = col_values.mean()

        if is_baseline:
            # Baseline: column is (employment_rate / NR), so multiply by NR then invert
            # col_mean ≈ 0.317 = 0.95 / 3, so: unemployment = 1 - (0.317 * 3) = 1 - 0.95 = 0.05
            mean_unemployment = 1.0 - (col_mean * NUM_REGIONS)
        else:
            # Shock scenarios: column is employment rate, invert to get unemployment
            mean_unemployment = 1.0 - col_mean
    else:
        mean_unemployment = np.nan

    # 5. End-of-century GDP (year 100)
    gdp_end = annual_gdp[-1]

    return {
        'scenario': scenario_num,
        'region': region_id,
        'run': run_num,
        'mean_growth_rate': mean_growth_rate,
        'crisis_likelihood': crisis_likelihood,
        'mean_unemployment': mean_unemployment,
        'gdp_end': gdp_end
    }


def compute_ratios(scenario_metrics, baseline_metrics):
    """
    Compute ratios relative to baseline for each run.

    Parameters:
    -----------
    scenario_metrics : list of dict
        Metrics for the scenario (one dict per run)
    baseline_metrics : list of dict
        Metrics for baseline (one dict per run)

    Returns:
    --------
    dict
        Dictionary with arrays of ratios for each metric
    """
    n_runs = len(scenario_metrics)

    # Extract baseline means (average across all baseline runs)
    baseline_growth = np.mean([m['mean_growth_rate'] for m in baseline_metrics])
    baseline_crisis = np.mean([m['crisis_likelihood'] for m in baseline_metrics])
    baseline_unemp = np.mean([m['mean_unemployment'] for m in baseline_metrics if not np.isnan(m['mean_unemployment'])])
    baseline_gdp_end = np.mean([m['gdp_end'] for m in baseline_metrics])

    # Compute run-level ratios
    growth_ratios = []
    crisis_ratios = []
    unemp_ratios = []
    gdp_end_ratios = []

    for metrics in scenario_metrics:
        growth_ratios.append(metrics['mean_growth_rate'] / baseline_growth)

        # For crisis likelihood, handle zero baseline
        if baseline_crisis > 0:
            crisis_ratios.append(metrics['crisis_likelihood'] / baseline_crisis)
        else:
            # If baseline has no crises, ratio is not meaningful
            crisis_ratios.append(np.inf if metrics['crisis_likelihood'] > 0 else 1.0)

        if not np.isnan(metrics['mean_unemployment']) and baseline_unemp > 0:
            unemp_ratios.append(metrics['mean_unemployment'] / baseline_unemp)
        else:
            unemp_ratios.append(np.nan)

        gdp_end_ratios.append(metrics['gdp_end'] / baseline_gdp_end)

    return {
        'growth_rate': np.array(growth_ratios),
        'crisis_likelihood': np.array(crisis_ratios),
        'unemployment': np.array(unemp_ratios),
        'gdp_end': np.array(gdp_end_ratios)
    }


def perform_t_test(ratios):
    """
    Perform one-sample t-test: H0: mean ratio = 1

    Parameters:
    -----------
    ratios : array-like
        Array of ratio values

    Returns:
    --------
    tuple
        (mean_ratio, t_statistic, p_value)
    """
    ratios = np.array(ratios)

    # Remove NaN and infinite values
    valid_ratios = ratios[np.isfinite(ratios)]

    if len(valid_ratios) < 2:
        return np.nan, np.nan, np.nan

    mean_ratio = valid_ratios.mean()

    # One-sample t-test against mu=1.0
    t_stat, p_value = stats.ttest_1samp(valid_ratios, 1.0)

    return mean_ratio, abs(t_stat), p_value


def format_cell_table5(values, metric_type='growth'):
    """
    Format a table cell for Table 5: absolute levels with Monte Carlo standard deviation.

    Format:
    mean%
    (std)

    Parameters:
    -----------
    values : array-like
        Array of run-level values for this metric
    metric_type : str
        'growth', 'crisis', or 'unemployment' for formatting

    Returns:
    --------
    str
        Formatted cell string
    """
    values = np.array(values)
    valid_values = values[np.isfinite(values)]

    if len(valid_values) < 2:
        return "N/A"

    mean = valid_values.mean()
    std = valid_values.std(ddof=1)  # Sample standard deviation

    if metric_type == 'growth':
        # Growth rate: report as percentage with 2 decimals
        return f"{mean*100:.2f}%\n({std*100:.3f})"
    elif metric_type == 'crisis':
        # Crisis likelihood: report as percentage with 1 decimal
        return f"{mean*100:.1f}%\n({std*100:.3f})"
    elif metric_type == 'unemployment':
        # Unemployment: report as percentage with 1 decimal
        return f"{mean*100:.1f}%\n({std*100:.3f})"
    elif metric_type == 'gdp_end':
        # GDP end: report absolute value
        return f"{mean:.2f}\n({std:.2f})"
    else:
        return "N/A"


def compute_beta_moments(alpha, beta):
    """
    Compute mean, variance, std, and CV of Beta distribution.

    Parameters:
    -----------
    alpha : float
        Beta distribution alpha parameter (X_a)
    beta : float
        Beta distribution beta parameter (X_b)

    Returns:
    --------
    dict
        Dictionary with mean, var, std, cv
    """
    if alpha <= 0 or beta <= 0:
        return {'mean': np.nan, 'var': np.nan, 'std': np.nan, 'cv': np.nan}

    mean = alpha / (alpha + beta)
    var = (alpha * beta) / ((alpha + beta)**2 * (alpha + beta + 1))
    std = np.sqrt(var)
    cv = std / mean if mean > 0 else np.nan

    return {'mean': mean, 'var': var, 'std': std, 'cv': cv}


def compute_aggregate_shock_moments(output_dir, scenario_num, num_runs, nshocks=9):
    """
    Compute first and second moments of climate shock size over time from aggregate shockpars files.
    Uses Beta distribution parameters X_a and X_b.

    CORRECT METHODOLOGY:
    1. For each shock type i: compute Beta moments (mean_i, std_i, cv_i)
    2. Average moments across shock types
    3. Average across 4 quarters of target year
    4. Average across Monte Carlo runs

    This is mathematically correct because:
    E[mean of Beta(α_i, β_i)] ≠ E[Beta(mean(α), mean(β))]

    Parameters:
    -----------
    output_dir : str
        Path to output directory
    scenario_num : int
        Scenario number to analyze
    num_runs : int
        Number of Monte Carlo runs
    nshocks : int
        Number of shock types (default: 9)

    Returns:
    --------
    dict
        Dictionary with year -> {mean, std, cv, n_runs} statistics
    """
    # Target years and their corresponding quarter indices (0-indexed)
    target_years = {
        2000: (200, 203),   # t=201-204
        2025: (300, 303),   # t=301-304
        2050: (400, 403),   # t=401-404
        2075: (500, 503),   # t=501-504
        2100: (596, 599),   # t=597-600
    }

    scenario_folder = Path(output_dir) / f"scenario_{scenario_num:02d}"

    # Storage for run-level yearly averages
    # Structure: {year: {'means': [], 'stds': [], 'cvs': []}}
    year_moments = {year: {'means': [], 'stds': [], 'cvs': []} for year in target_years.keys()}

    # Process each run
    for run_num in range(1, num_runs + 1):
        run_folder = scenario_folder / f"run_{run_num:03d}"

        if not run_folder.exists():
            continue

        # Find shockpars file
        pattern = f"shockpars_scenario{scenario_num:02d}_rep{run_num:03d}_*.txt"
        files = list(run_folder.glob(pattern))
        if len(files) == 0:
            continue

        filepath = files[0]

        try:
            # Read shockpars file
            # Structure: col 0=t, cols 1-9=X_a(1..9), cols 10-18=X_b(1..9), then regional
            df = pd.read_csv(filepath, sep=r'\s+', header=None)

            if len(df) != TOTAL_QUARTERS:
                continue

            # Extract shock moments at target years
            for year, (start_idx, end_idx) in target_years.items():
                # Storage for this year's 4 quarters
                quarter_means_list = []
                quarter_stds_list = []
                quarter_cvs_list = []

                # Process each quarter in the target year
                for quarter_idx in range(start_idx, end_idx + 1):
                    if quarter_idx >= len(df):
                        continue

                    # Extract X_a and X_b for all shock types
                    xa_values = df.iloc[quarter_idx, 1:1+nshocks].values  # cols 1-9
                    xb_values = df.iloc[quarter_idx, 1+nshocks:1+2*nshocks].values  # cols 10-18

                    # Compute Beta moments for EACH shock type individually
                    shock_means = []
                    shock_stds = []
                    shock_cvs = []

                    for i in range(nshocks):
                        alpha = xa_values[i]
                        beta = xb_values[i]

                        if alpha > 0 and beta > 0:
                            # Compute Beta distribution moments for this shock type
                            moments = compute_beta_moments(alpha, beta)

                            if not np.isnan(moments['mean']):
                                shock_means.append(moments['mean'])
                                shock_stds.append(moments['std'])
                                shock_cvs.append(moments['cv'])

                    # Average across shock types for this quarter
                    if len(shock_means) > 0:
                        quarter_means_list.append(np.mean(shock_means))
                        quarter_stds_list.append(np.mean(shock_stds))
                        quarter_cvs_list.append(np.mean(shock_cvs))

                # Average over the 4 quarters for this run and year
                if len(quarter_means_list) > 0:
                    year_moments[year]['means'].append(np.mean(quarter_means_list))
                    year_moments[year]['stds'].append(np.mean(quarter_stds_list))
                    year_moments[year]['cvs'].append(np.mean(quarter_cvs_list))

        except Exception as e:
            continue

    # Compute statistics across Monte Carlo runs
    results = {}
    for year in sorted(target_years.keys()):
        means = np.array(year_moments[year]['means'])
        stds = np.array(year_moments[year]['stds'])
        cvs = np.array(year_moments[year]['cvs'])

        if len(means) >= 2:
            results[year] = {
                'mean': means.mean(),
                'std': stds.mean(),
                'cv': cvs.mean(),
                'n_runs': len(means)
            }
        else:
            results[year] = {
                'mean': np.nan,
                'std': np.nan,
                'cv': np.nan,
                'n_runs': len(means)
            }

    return results


def compute_regional_shock_moments(output_dir, scenario_num, num_runs, nshocks=9):
    """
    Compute shock moments from regional shock parameters in shockpars files.
    Uses Beta distribution parameters X_a_reg and X_b_reg.

    CORRECT METHODOLOGY (same as aggregate):
    1. For each shock type i: compute Beta moments (mean_i, std_i, cv_i)
    2. Average moments across shock types
    3. Average across 4 quarters of target year
    4. Average across Monte Carlo runs

    Parameters:
    -----------
    output_dir : str
        Path to output directory
    scenario_num : int
        Scenario number
    num_runs : int
        Number of Monte Carlo runs
    nshocks : int
        Number of shock types (default: 9)

    Returns:
    --------
    dict or None
        Dictionary with region_id -> year -> {mean, std, cv, n_runs} statistics
        Returns None if no regional data found
    """
    target_years = {
        2000: (200, 203),
        2025: (300, 303),
        2050: (400, 403),
        2075: (500, 503),
        2100: (596, 599),
    }

    scenario_folder = Path(output_dir) / f"scenario_{scenario_num:02d}"

    # First, detect number of regions by checking a sample shockpars file
    sample_run = scenario_folder / "run_001"
    if not sample_run.exists():
        return None

    pattern = f"shockpars_scenario{scenario_num:02d}_rep001_*.txt"
    files = list(sample_run.glob(pattern))
    if len(files) == 0:
        return None

    # Read first file to detect structure
    try:
        df_sample = pd.read_csv(files[0], sep=r'\s+', header=None)
        n_cols = df_sample.shape[1]

        # Structure: col 0=t, cols 1-9=X_a, cols 10-18=X_b, then regional
        # Regional: for each region rr, cols for X_a_reg(1..nshocks, rr), then X_b_reg(1..nshocks, rr)
        # So: 1 + 9 + 9 = 19 aggregate columns
        # Then: NR * (nshocks + nshocks) = NR * 18 regional columns

        if n_cols <= 19:
            # No regional data
            return None

        regional_cols = n_cols - 19
        if regional_cols % (2 * nshocks) != 0:
            print(f"  ⚠ Warning: Unexpected shockpars structure (cols={n_cols})")
            return None

        NR = regional_cols // (2 * nshocks)
        print(f"  Detected {NR} regions in shockpars files")

    except Exception as e:
        print(f"  ✗ Error detecting regional structure: {e}")
        return None

    # Process each region
    regional_results = {}

    for region_num in range(1, NR + 1):
        region_id = str(region_num)

        # Calculate column indices for this region
        # Aggregate: cols 0-18 (t, X_a(1-9), X_b(1-9))
        # Region r: X_a_reg starts at col 19 + (r-1)*18, X_b_reg starts at col 19 + (r-1)*18 + 9
        xa_start = 19 + (region_num - 1) * 2 * nshocks
        xa_end = xa_start + nshocks
        xb_start = xa_end
        xb_end = xb_start + nshocks

        # Storage for shock moments
        year_moments = {year: {'means': [], 'stds': [], 'cvs': []} for year in target_years.keys()}

        # Process each run
        for run_num in range(1, num_runs + 1):
            run_folder = scenario_folder / f"run_{run_num:03d}"

            if not run_folder.exists():
                continue

            # Find shockpars file
            pattern = f"shockpars_scenario{scenario_num:02d}_rep{run_num:03d}_*.txt"
            files = list(run_folder.glob(pattern))
            if len(files) == 0:
                continue

            filepath = files[0]

            try:
                df = pd.read_csv(filepath, sep=r'\s+', header=None)

                if len(df) != TOTAL_QUARTERS:
                    continue

                # Extract shock moments at target years
                for year, (start_idx, end_idx) in target_years.items():
                    # Storage for this year's 4 quarters
                    quarter_means_list = []
                    quarter_stds_list = []
                    quarter_cvs_list = []

                    # Process each quarter in the target year
                    for quarter_idx in range(start_idx, end_idx + 1):
                        if quarter_idx >= len(df):
                            continue

                        # Extract regional X_a and X_b for all shock types
                        xa_values = df.iloc[quarter_idx, xa_start:xa_end].values
                        xb_values = df.iloc[quarter_idx, xb_start:xb_end].values

                        # Compute Beta moments for EACH shock type individually
                        shock_means = []
                        shock_stds = []
                        shock_cvs = []

                        for i in range(nshocks):
                            alpha = xa_values[i]
                            beta = xb_values[i]

                            if alpha > 0 and beta > 0:
                                # Compute Beta distribution moments for this shock type
                                moments = compute_beta_moments(alpha, beta)

                                if not np.isnan(moments['mean']):
                                    shock_means.append(moments['mean'])
                                    shock_stds.append(moments['std'])
                                    shock_cvs.append(moments['cv'])

                        # Average across shock types for this quarter
                        if len(shock_means) > 0:
                            quarter_means_list.append(np.mean(shock_means))
                            quarter_stds_list.append(np.mean(shock_stds))
                            quarter_cvs_list.append(np.mean(shock_cvs))

                    # Average over the 4 quarters for this run and year
                    if len(quarter_means_list) > 0:
                        year_moments[year]['means'].append(np.mean(quarter_means_list))
                        year_moments[year]['stds'].append(np.mean(quarter_stds_list))
                        year_moments[year]['cvs'].append(np.mean(quarter_cvs_list))

            except Exception as e:
                continue

        # Compute statistics across MC runs
        region_results = {}
        for year in sorted(target_years.keys()):
            means = np.array(year_moments[year]['means'])
            stds = np.array(year_moments[year]['stds'])
            cvs = np.array(year_moments[year]['cvs'])

            if len(means) >= 2:
                region_results[year] = {
                    'mean': means.mean(),
                    'std': stds.mean(),
                    'cv': cvs.mean(),
                    'n_runs': len(means)
                }
            else:
                region_results[year] = {
                    'mean': np.nan,
                    'std': np.nan,
                    'cv': np.nan,
                    'n_runs': len(means)
                }

        regional_results[region_id] = region_results

    return regional_results


def compute_national_shock_summary(output_dir, scenario_num, num_runs, nshocks=9):
    """
    Compute national shock summary with CV calculated at run-year aggregated level.

    Methodology per user specification:
    1) For each run, each quarter, each shock type: compute Beta moments
    2) For each quarter: average means and stds across shock types
    3) For each year: average quarterly means and stds, then compute CV = avg_std / avg_mean
    4) Across runs: average the run-level means, stds, and CVs

    This differs from compute_aggregate_shock_moments() which averages individual CVs.

    Returns:
    --------
    dict: {year: {'mean': float, 'std': float, 'cv': float, 'n_runs': int}}
    """
    target_years = {
        2000: (200, 203),
        2025: (300, 303),
        2050: (400, 403),
        2075: (500, 503),
        2100: (596, 599),
    }

    scenario_folder = Path(output_dir) / f"scenario_{scenario_num:02d}"

    # Storage: {year: {'means': [], 'stds': [], 'cvs': []}}
    year_data = {year: {'means': [], 'stds': [], 'cvs': []} for year in target_years.keys()}

    # Process each run
    for run_num in range(1, num_runs + 1):
        run_folder = scenario_folder / f"run_{run_num:03d}"
        if not run_folder.exists():
            continue

        # Find shockpars file
        pattern = f"shockpars_scenario{scenario_num:02d}_rep{run_num:03d}_*.txt"
        files = list(run_folder.glob(pattern))
        if len(files) == 0:
            continue

        try:
            df = pd.read_csv(files[0], sep=r'\s+', header=None)
            if len(df) != TOTAL_QUARTERS:
                continue

            # Process each target year
            for year, (start_idx, end_idx) in target_years.items():
                quarter_means = []  # avg_mean_q for each quarter
                quarter_stds = []   # avg_std_q for each quarter

                # For each quarter in this year
                for quarter_idx in range(start_idx, end_idx + 1):
                    if quarter_idx >= len(df):
                        continue

                    # Extract shock parameters (cols 1-9: X_a, cols 10-18: X_b)
                    xa = df.iloc[quarter_idx, 1:1+nshocks].values
                    xb = df.iloc[quarter_idx, 1+nshocks:1+2*nshocks].values

                    # Compute Beta moments for each shock type
                    shock_means = []
                    shock_stds = []

                    for i in range(nshocks):
                        if xa[i] > 0 and xb[i] > 0:
                            moments = compute_beta_moments(xa[i], xb[i])
                            if not np.isnan(moments['mean']):
                                shock_means.append(moments['mean'])
                                shock_stds.append(moments['std'])

                    # Average across shock types for this quarter
                    if len(shock_means) > 0:
                        quarter_means.append(np.mean(shock_means))
                        quarter_stds.append(np.mean(shock_stds))

                # Compute run-year aggregates
                if len(quarter_means) > 0:
                    avg_mean_run_year = np.mean(quarter_means)
                    avg_std_run_year = np.mean(quarter_stds)

                    # CV at run-year level
                    if avg_mean_run_year > 0:
                        cv_run_year = avg_std_run_year / avg_mean_run_year

                        year_data[year]['means'].append(avg_mean_run_year)
                        year_data[year]['stds'].append(avg_std_run_year)
                        year_data[year]['cvs'].append(cv_run_year)

        except Exception:
            continue

    # Aggregate across Monte Carlo runs
    results = {}
    for year in sorted(target_years.keys()):
        means = np.array(year_data[year]['means'])
        stds = np.array(year_data[year]['stds'])
        cvs = np.array(year_data[year]['cvs'])

        if len(means) >= 2:
            results[year] = {
                'mean': means.mean(),
                'std': stds.mean(),
                'cv': cvs.mean(),
                'n_runs': len(means)
            }
        else:
            results[year] = {
                'mean': np.nan,
                'std': np.nan,
                'cv': np.nan,
                'n_runs': len(means)
            }

    return results


def compute_regional_shock_summary(output_dir, scenario_num, num_runs, nshocks=9):
    """
    Compute regional shock summary with CV calculated at run-year aggregated level.

    Same methodology as compute_national_shock_summary() but for regional parameters.

    Returns:
    --------
    dict: {region_id: {year: {'mean': float, 'std': float, 'cv': float, 'n_runs': int}}}
    """
    target_years = {
        2000: (200, 203),
        2025: (300, 303),
        2050: (400, 403),
        2075: (500, 503),
        2100: (596, 599),
    }

    scenario_folder = Path(output_dir) / f"scenario_{scenario_num:02d}"

    # Detect number of regions from first file
    num_regions = None
    for run_num in range(1, num_runs + 1):
        run_folder = scenario_folder / f"run_{run_num:03d}"
        if not run_folder.exists():
            continue

        pattern = f"shockpars_scenario{scenario_num:02d}_rep{run_num:03d}_*.txt"
        files = list(run_folder.glob(pattern))
        if len(files) > 0:
            try:
                df = pd.read_csv(files[0], sep=r'\s+', header=None)
                total_cols = len(df.columns)
                regional_cols = total_cols - 1 - 2*nshocks  # Subtract time col and aggregate params
                if regional_cols > 0 and regional_cols % (2*nshocks) == 0:
                    num_regions = regional_cols // (2*nshocks)
                    break
            except Exception:
                continue

    if num_regions is None or num_regions <= 0:
        return None

    # Storage: {region_id: {year: {'means': [], 'stds': [], 'cvs': []}}}
    regional_data = {}
    for region_id in range(1, num_regions + 1):
        regional_data[region_id] = {
            year: {'means': [], 'stds': [], 'cvs': []}
            for year in target_years.keys()
        }

    # Process each run
    for run_num in range(1, num_runs + 1):
        run_folder = scenario_folder / f"run_{run_num:03d}"
        if not run_folder.exists():
            continue

        pattern = f"shockpars_scenario{scenario_num:02d}_rep{run_num:03d}_*.txt"
        files = list(run_folder.glob(pattern))
        if len(files) == 0:
            continue

        try:
            df = pd.read_csv(files[0], sep=r'\s+', header=None)
            if len(df) != TOTAL_QUARTERS:
                continue

            # Process each region
            for region_id in range(1, num_regions + 1):
                # Column indices for this region
                reg_start = 1 + 2*nshocks + (region_id - 1) * 2 * nshocks
                xa_cols = range(reg_start, reg_start + nshocks)
                xb_cols = range(reg_start + nshocks, reg_start + 2*nshocks)

                # Process each target year
                for year, (start_idx, end_idx) in target_years.items():
                    quarter_means = []
                    quarter_stds = []

                    for quarter_idx in range(start_idx, end_idx + 1):
                        if quarter_idx >= len(df):
                            continue

                        # Extract regional shock parameters
                        xa = df.iloc[quarter_idx, list(xa_cols)].values
                        xb = df.iloc[quarter_idx, list(xb_cols)].values

                        # Compute Beta moments for each shock type
                        shock_means = []
                        shock_stds = []

                        for i in range(nshocks):
                            if xa[i] > 0 and xb[i] > 0:
                                moments = compute_beta_moments(xa[i], xb[i])
                                if not np.isnan(moments['mean']):
                                    shock_means.append(moments['mean'])
                                    shock_stds.append(moments['std'])

                        if len(shock_means) > 0:
                            quarter_means.append(np.mean(shock_means))
                            quarter_stds.append(np.mean(shock_stds))

                    # Compute run-year aggregates
                    if len(quarter_means) > 0:
                        avg_mean_run_year = np.mean(quarter_means)
                        avg_std_run_year = np.mean(quarter_stds)

                        if avg_mean_run_year > 0:
                            cv_run_year = avg_std_run_year / avg_mean_run_year

                            regional_data[region_id][year]['means'].append(avg_mean_run_year)
                            regional_data[region_id][year]['stds'].append(avg_std_run_year)
                            regional_data[region_id][year]['cvs'].append(cv_run_year)

        except Exception:
            continue

    # Aggregate across Monte Carlo runs
    results = {}
    for region_id in range(1, num_regions + 1):
        region_results = {}
        for year in sorted(target_years.keys()):
            means = np.array(regional_data[region_id][year]['means'])
            stds = np.array(regional_data[region_id][year]['stds'])
            cvs = np.array(regional_data[region_id][year]['cvs'])

            if len(means) >= 2:
                region_results[year] = {
                    'mean': means.mean(),
                    'std': stds.mean(),
                    'cv': cvs.mean(),
                    'n_runs': len(means)
                }
            else:
                region_results[year] = {
                    'mean': np.nan,
                    'std': np.nan,
                    'cv': np.nan,
                    'n_runs': len(means)
                }

        results[region_id] = region_results

    return results


def main():
    """Main analysis function."""
    # ===========================================================================
    # SETUP
    # ===========================================================================
    print("=" * 80)
    print("TABLE GENERATION")
    print("=" * 80)
    print()

    # Load column mapper
    mapper_path = Path('mapper.xlsx')
    if not mapper_path.exists():
        print(f"Mapper file not found: {mapper_path}")
        return

    mapper = pd.read_excel(mapper_path)
    print(f"Loaded aggregate mapper with {len(mapper)} columns")

    # Create regional column mappers
    regional_mapper_baseline, regional_mapper_shock = get_regional_column_mappers()
    print(
        f"Created regional mappers (baseline: {len(regional_mapper_baseline)} cols, shock: {len(regional_mapper_shock)} cols)")
    print()

    # ===========================================================================
    # PROCESS ALL SCENARIOS
    # ===========================================================================
    print("=" * 80)
    print("PROCESSING SCENARIOS")
    print("=" * 80)
    print()

    all_metrics = {}

    for scenario_num in SCENARIO_NAMES.keys():
        print(f"Processing Scenario {scenario_num}: {SCENARIO_NAMES[scenario_num]}")

        scenario_folder = Path(OUTPUT_DIR) / f"scenario_{scenario_num:02d}"

        if not scenario_folder.exists():
            print(f"  Scenario folder not found: {scenario_folder}")
            continue

        is_baseline = (scenario_num == 1)
        scenario_metrics = []

        # Process each run
        for run_num in range(1, NUM_RUNS + 1):
            run_folder = scenario_folder / f"run_{run_num:03d}"

            if not run_folder.exists():
                print(f"  Run folder not found: {run_folder}")
                continue

            metrics = process_run(run_folder, scenario_num, run_num, mapper, is_baseline)

            if metrics is not None:
                scenario_metrics.append(metrics)

        print(f" Processed {len(scenario_metrics)} runs")

        if len(scenario_metrics) > 0:
            all_metrics[scenario_num] = scenario_metrics

        print()

    # Check if we have baseline
    if 1 not in all_metrics:
        print("Baseline scenario (Scenario 1) not found. Cannot compute ratios.")
        return

    baseline_metrics = all_metrics[1]
    print(f"Baseline scenario has {len(baseline_metrics)} runs")
    print()

    # Save run-level metrics
    print("Saving run-level metrics...")
    all_runs_data = []
    for scenario_num, metrics_list in all_metrics.items():
        for metrics in metrics_list:
            all_runs_data.append(metrics)

    run_level_df = pd.DataFrame(all_runs_data)
    run_level_df.to_csv('./output/verifications/metrics_run_level.csv', index=False)
    print()

    # ===========================================================================
    # SANITY CHECKS
    # ===========================================================================
    print("=" * 80)
    print("SANITY CHECKS")
    print("=" * 80)
    print()

    # Check 1: Baseline statistics
    print("1. BASELINE SCENARIO STATISTICS (Scenario 1)")
    print("-" * 80)
    baseline_df = run_level_df[run_level_df['scenario'] == 1]
    print(f"Number of baseline runs: {len(baseline_df)}")
    print()

    for metric in ['mean_growth_rate', 'crisis_likelihood', 'mean_unemployment', 'gdp_end']:
        values = baseline_df[metric].values
        print(f"{metric}:")
        print(f"  Mean:   {values.mean():.6f}")
        print(f"  Std:    {values.std():.6f}")
        print(f"  Min:    {values.min():.6f}")
        print(f"  Max:    {values.max():.6f}")
        print(f"  Median: {np.median(values):.6f}")
        print()

    # Check 2: Sample values from first few runs
    print("2. SAMPLE RAW VALUES (First 3 runs of Scenario 1 and Scenario 2)")
    print("-" * 80)
    for scenario_num in [1, 2]:
        if scenario_num in all_metrics:
            print(f"\nScenario {scenario_num} ({SCENARIO_NAMES.get(scenario_num, 'Unknown')}):")
            for i in range(min(3, len(all_metrics[scenario_num]))):
                m = all_metrics[scenario_num][i]
                print(f"  Run {m['run']}:")
                print(f"    Growth rate: {m['mean_growth_rate']:.6f}")
                print(f"    Crisis likelihood: {m['crisis_likelihood']:.6f}")
                print(f"    Unemployment: {m['mean_unemployment']:.6f}")
                print(f"    GDP end: {m['gdp_end']:.2f}")
    print()

    # Check 3: Data range validation
    print("3. DATA RANGE VALIDATION")
    print("-" * 80)
    issues_found = False

    # Check growth rates (should typically be between -0.5 and 0.5)
    growth_values = run_level_df['mean_growth_rate'].values
    suspicious_growth = (growth_values < -0.5) | (growth_values > 0.5)
    if suspicious_growth.any():
        print(f" WARNING: {suspicious_growth.sum()} runs have unusual growth rates (outside -50% to +50%)")
        print(f" Range: {growth_values.min():.4f} to {growth_values.max():.4f}")
        issues_found = True
    else:
        print(f"Growth rates in reasonable range: {growth_values.min():.4f} to {growth_values.max():.4f}")

    # Check crisis likelihood (should be between 0 and 1)
    crisis_values = run_level_df['crisis_likelihood'].values
    if (crisis_values < 0).any() or (crisis_values > 1).any():
        print(f" ERROR: Crisis likelihood outside [0,1] range!")
        print(f" Range: {crisis_values.min():.4f} to {crisis_values.max():.4f}")
        issues_found = True
    else:
        print(f" Crisis likelihood in valid range [0,1]: {crisis_values.min():.4f} to {crisis_values.max():.4f}")

    # Check unemployment (should be between 0 and 1, and now properly inverted from employment)
    unemp_values = run_level_df['mean_unemployment'].dropna().values
    if len(unemp_values) > 0:
        if unemp_values.max() > 1.0:
            print(f"WARNING: Unemployment values exceed 1.0, which suggests inversion problem")
            print(f"  Range: {unemp_values.min():.4f} to {unemp_values.max():.4f}")
            issues_found = True
        else:
            print(f" Unemployment in valid range [0,1]: {unemp_values.min():.4f} to {unemp_values.max():.4f}")

            # Should now be unemployment rates (low values ~0.05)
            if unemp_values.mean() < 0.2:
                print(f"  ℹ Unemployment rate mean: {unemp_values.mean():.4f} (properly inverted from employment)")
                print(f"     → Higher values = MORE unemployment = WORSE")
            else:
                print(f"  WARNING: Unemployment mean is {unemp_values.mean():.4f}, seems high")
                print(f"  Check if employment rate was properly inverted")
                issues_found = True

    # Check GDP end values (should be positive and reasonable)
    gdp_values = run_level_df['gdp_end'].values
    if (gdp_values <= 0).any():
        print(f"✗ ERROR: Some GDP end values are non-positive!")
        issues_found = True
    else:
        print(f"✓ GDP end values are positive: {gdp_values.min():.2f} to {gdp_values.max():.2f}")

    print()

    # Check 4: Verify column extraction with sample data
    print("4. COLUMN EXTRACTION VERIFICATION")
    print("-" * 80)
    # Read one file and show what columns we're using
    first_scenario_folder = Path(OUTPUT_DIR) / "scenario_01" / "run_001"
    sample_file = list(first_scenario_folder.glob("ymc_scenario01_*.txt"))[0]
    sample_df = read_simulation_file(sample_file)

    gdp_col = get_column_index(mapper, 'GDP_r_1')
    unemp_col = get_column_index(mapper, 'EmploymentRate_1')

    print(f"Mapper indicates:")
    print(f"  GDP column index: {gdp_col}")
    print(f"  Unemployment column index: {unemp_col}")
    print(f"  Mapper name: 'EmploymentRate_1' (we invert this to get unemployment)")
    print(f"\nSample file: {sample_file.name}")
    print(f"  Total columns: {len(sample_df.columns)}")
    print(f"  Total rows: {len(sample_df)}")
    print(f"\nFirst 5 values from t=201-205 (analysis window start):")
    print(f"  GDP (col {gdp_col}): {sample_df.iloc[200:205, gdp_col].values}")
    print(f"  EmploymentRate (col {unemp_col}): {sample_df.iloc[200:205, unemp_col].values}")
    emp_sample = sample_df.iloc[200:205, unemp_col].values
    unemp_sample = 1.0 - emp_sample
    print(f"  Unemployment (1 - employment): {unemp_sample}")
    print(f"\n  Note: Employment ~0.95 → Unemployment ~0.05 (5%)")
    print(f"        After inversion, ratio > 1.0 means MORE unemployment = WORSE")
    print()

    # Check 5: Annualization check
    print("5. ANNUALIZATION VERIFICATION")
    print("-" * 80)
    # Take first baseline run and show annualization
    test_gdp_quarterly = sample_df.iloc[200:600, gdp_col].values
    test_annual = annualize_gdp(test_gdp_quarterly)
    print(f"Quarterly GDP sample (t=201-204): {test_gdp_quarterly[:4]}")
    print(
        f"Annual GDP Year 1: {test_annual[0]:.2f} (should equal sum of 4 quarters: {test_gdp_quarterly[:4].sum():.2f})")
    print(f"Match: {'✓' if abs(test_annual[0] - test_gdp_quarterly[:4].sum()) < 0.01 else '✗'}")
    print()

    # Check 6: Growth rate calculation
    print("6. GROWTH RATE CALCULATION VERIFICATION")
    print("-" * 80)

    # Check CAGR (long-run growth)
    test_cagr = compute_long_run_growth(test_annual)
    manual_cagr = (test_annual[-1] / test_annual[0]) ** (1 / (len(test_annual) - 1)) - 1
    print(f"Long-run growth (CAGR):")
    print(f"  First year GDP: {test_annual[0]:.2f}")
    print(f"  Last year GDP: {test_annual[-1]:.2f}")
    print(f"  Computed CAGR: {test_cagr:.6f}")
    print(f"  Manual CAGR: {manual_cagr:.6f}")
    print(f"  Match: {'✓' if abs(test_cagr - manual_cagr) < 0.000001 else '✗'}")
    print(f"\nNote: This CAGR is what appears in the Lamperti table,")
    print(f"      NOT the mean of annual growth rates.")

    # Also show annual growth rates (for crisis detection)
    test_growth = compute_growth_rate(test_annual)
    print(f"\nAnnual growth rates (for crisis detection):")
    print(f"  First 4 values: {test_growth[:4]}")
    print(f"  Mean: {test_growth.mean():.6f} (NOT used in table)")
    print()

    # Check 7: Crisis detection
    print("7. CRISIS DETECTION VERIFICATION")
    print("-" * 80)
    test_crisis = compute_crisis_indicator(test_growth)
    crisis_years = (test_growth < CRISIS_THRESHOLD).sum()
    print(f"Crisis threshold: {CRISIS_THRESHOLD}")
    print(f"Number of growth rates below threshold: {crisis_years}")
    print(f"Crisis likelihood (proportion): {test_crisis:.4f}")
    print(f"Calculation: {crisis_years}/99 = {crisis_years/99:.4f}")
    print(f"Match: {'✓' if abs(test_crisis - crisis_years/99) < 0.000001 else '✗'}")
    print()

    if issues_found:
        print("=" * 80)
        print("WARNING: Some data issues were detected above.")
        print("Please review the sanity checks before proceeding with results.")
        print("=" * 80)
        print()
    else:
        print("=" * 80)
        print(" All sanity checks passed!")
        print("=" * 80)
        print()

    # Compute Table 5: Absolute levels with Monte Carlo standard deviations
    print("Computing Table 5: Main economic performances (absolute levels)...")
    print()

    # Show baseline summary
    print("BASELINE SCENARIO SUMMARY:")
    print("-" * 80)
    baseline_growth_mean = np.mean([m['mean_growth_rate'] for m in baseline_metrics])
    baseline_crisis_mean = np.mean([m['crisis_likelihood'] for m in baseline_metrics])
    baseline_unemp_mean = np.mean([m['mean_unemployment']
                                  for m in baseline_metrics if not np.isnan(m['mean_unemployment'])])

    print(f"Mean long-run growth (CAGR): {baseline_growth_mean*100:.2f}%")
    print(f"Mean crisis likelihood: {baseline_crisis_mean*100:.1f}%")
    print(f"Mean unemployment rate: {baseline_unemp_mean*100:.1f}%")
    print(f"\nNote: Table 5 reports ABSOLUTE LEVELS for all scenarios, not ratios.")
    print()

    table_data = []

    for scenario_num in sorted(SCENARIO_NAMES.keys()):
        if scenario_num not in all_metrics:
            continue

        scenario_name = SCENARIO_NAMES[scenario_num]
        scenario_metrics = all_metrics[scenario_num]

        # Extract run-level values
        growth_values = np.array([m['mean_growth_rate'] for m in scenario_metrics])
        crisis_values = np.array([m['crisis_likelihood'] for m in scenario_metrics])
        unemp_values = np.array([m['mean_unemployment']
                                for m in scenario_metrics if not np.isnan(m['mean_unemployment'])])

        # Format cells with mean and MC standard deviation
        table_data.append({
            'Scenario': scenario_name,
            'Output growth rate': format_cell_table5(growth_values, 'growth'),
            'Likelihood of crises': format_cell_table5(crisis_values, 'crisis'),
            'Unemployment': format_cell_table5(unemp_values, 'unemployment')
        })

    # Create DataFrame
    lamperti_df = pd.DataFrame(table_data)

    # Save CSV
    lamperti_df.to_csv('./output/verifications/table5_levels.csv', index=False)
    print("Saved table5_levels.csv")

    # Save Markdown
    with open('./output/verifications/table5_levels.md', 'w') as f:
        f.write("# Table 5: Main economic performances under heterogeneous climate damages and shock scenarios\n\n")
        f.write("Monte Carlo standard deviations in parentheses.\n\n")
        f.write(lamperti_df.to_markdown(index=False))
        f.write("\n\n")
        f.write(f"**Note**: All values refer to a Monte Carlo of size {NUM_RUNS}.\n\n")
        f.write("**Definitions**:\n")
        f.write("- Output growth rate: Compound Annual Growth Rate (CAGR) over 100-year post-shock period\n")
        f.write("- Likelihood of crises: Share of years with annual GDP growth < -5%\n")
        f.write("- Unemployment: Mean unemployment rate (inverted from EmploymentRate_1)\n")

    print("Saved lamperti_table5_levels.md")
    print()

    # Display table
    print("=" * 80)
    print("TABLE 5: MAIN ECONOMIC PERFORMANCES (ABSOLUTE LEVELS)")
    print("=" * 80)
    print(lamperti_df.to_string(index=False))
    print()

    # Process regional data if available
    print("Checking for regional data...")

    # Initialize regional_results
    regional_results = {}

    # Try to find regional files in first scenario
    first_scenario_folder = Path(OUTPUT_DIR) / "scenario_01" / "run_001"

    # Look for both naming patterns:
    # Pattern 1: ymc_1_*, ymc_2_*, ymc_3_* (regional by number)
    # Pattern 2: resultsexp_reg1_*, resultsexp_reg2_* (regional with 'reg' prefix)
    regional_files_nums = list(first_scenario_folder.glob("ymc_[0-9]*_*.txt"))
    regional_files_regs = list(first_scenario_folder.glob("*_reg[0-9]*_*.txt"))

    regional_files = regional_files_nums + regional_files_regs

    if len(regional_files) > 0:
        print(f" Found regional data files")

        # Extract region IDs
        region_ids = set()
        for f in regional_files:
            parts = f.stem.split('_')
            # Check for numeric regions (ymc_1_, ymc_2_)
            if len(parts) > 1 and parts[0] in ['ymc'] and parts[1].isdigit():
                region_ids.add(parts[1])
            # Check for reg-prefixed regions (resultsexp_reg1_)
            for i, part in enumerate(parts):
                if part.startswith('reg') and len(part) > 3:
                    region_ids.add(part)

        region_ids = sorted(region_ids)
        print(f"  Regions found: {', '.join(region_ids)}")

        # Process each region
        regional_results = {}

        for region_id in region_ids:
            print(f"\nProcessing region: {region_id}")

            region_metrics = {}

            for scenario_num in SCENARIO_NAMES.keys():
                scenario_folder = Path(OUTPUT_DIR) / f"scenario_{scenario_num:02d}"

                if not scenario_folder.exists():
                    continue

                is_baseline = (scenario_num == 1)
                scenario_regional_metrics = []

                # Select appropriate regional mapper based on scenario type
                regional_mapper = regional_mapper_baseline if is_baseline else regional_mapper_shock

                for run_num in range(1, NUM_RUNS + 1):
                    run_folder = scenario_folder / f"run_{run_num:03d}"

                    if not run_folder.exists():
                        continue

                    metrics = process_regional_run(run_folder, scenario_num, run_num,
                                                   region_id, regional_mapper, is_baseline)

                    if metrics is not None:
                        scenario_regional_metrics.append(metrics)

                if len(scenario_regional_metrics) > 0:
                    region_metrics[scenario_num] = scenario_regional_metrics

            if 1 in region_metrics:
                regional_results[region_id] = region_metrics
                print(f"  Processed {len(region_metrics)} scenarios for {region_id}")

        # Create regional tables
        if len(regional_results) > 0:
            print("\nCreating regional comparison tables...")

            all_regional_data = []

            for region_id, region_metrics in regional_results.items():

                for scenario_num in sorted(SCENARIO_NAMES.keys()):
                    if scenario_num not in region_metrics:
                        continue

                    scenario_name = SCENARIO_NAMES[scenario_num]
                    scenario_metrics = region_metrics[scenario_num]

                    # Extract run-level values for this region
                    growth_values = np.array([m['mean_growth_rate'] for m in scenario_metrics])
                    crisis_values = np.array([m['crisis_likelihood'] for m in scenario_metrics])
                    unemp_values = np.array([m['mean_unemployment']
                                            for m in scenario_metrics if not np.isnan(m['mean_unemployment'])])

                    # Format cells with absolute levels and MC standard deviations
                    all_regional_data.append({
                        'Region': region_id,
                        'Scenario': scenario_name,
                        'Output growth rate': format_cell_table5(growth_values, 'growth'),
                        'Likelihood of crises': format_cell_table5(crisis_values, 'crisis'),
                        'Unemployment': format_cell_table5(unemp_values, 'unemployment')
                    })

            regional_df = pd.DataFrame(all_regional_data)
            regional_df.to_csv('./output/verifications/table5_regions.csv', index=False)

            # Save Markdown version
            with open('./output/verifications/table5_regions.md', 'w') as f:
                f.write("# Table 5: Regional Economic Performances (Absolute Levels)\n\n")
                f.write(f"Reported values are averages over a Monte Carlo of size {NUM_RUNS}.\n\n")
                f.write("**Format**: Each cell shows:\n")
                f.write("- First line: Mean value (percentage)\n")
                f.write("- Second line: (Monte Carlo standard deviation)\n\n")
                f.write("**Metrics**:\n")
                f.write("- Output growth rate: Compound Annual Growth Rate (CAGR)\n")
                f.write("- Likelihood of crises: Fraction of years with negative growth\n")
                f.write("- Unemployment: Mean unemployment rate over 100 years (inverted from employment)\n\n")
                f.write("**Note**: Analysis window is 100 years post-shock (t=201-600, quarters 51-150 annually)\n\n")
                f.write(regional_df.to_markdown(index=False))
                f.write("\n")

            print("Saved lamperti_table5_regions.csv")
            print("Saved lamperti_table5_regions.md")
            print()
    else:
        print("  No regional data files found")
        print()

    # ===========================================================================
    # TABLE 4: SHOCK MOMENTS OVER TIME (BETA DISTRIBUTION PARAMETERS)
    # ===========================================================================
    print("=" * 80)
    print("COMPUTING TABLE 4: SHOCK STATISTICS OVER TIME (CORRECTED)")
    print("=" * 80)
    print()

    print("Using Beta distribution parameters X_a and X_b from shockpars files.")
    print()
    print("CORRECT METHODOLOGY:")
    print("  1. For each shock type i: compute Beta(α_i, β_i) moments")
    print("  2. Average moments across shock types")
    print("  3. Average across 4 quarters")
    print("  4. Average across Monte Carlo runs")
    print()
    print("This ensures: E[mean(Beta_i)] ≠ mean(E[Beta(mean(α), mean(β))])")
    print()

    # Analyze all shock scenarios (excluding scenario 1 "No shocks")
    shock_scenarios = [s for s in range(2, 10) if s in SCENARIO_NAMES]

    # ===========================================================================
    # AGGREGATE SHOCK TABLE
    # ===========================================================================
    print("=" * 80)
    print("AGGREGATE SHOCK MOMENTS")
    print("=" * 80)

    table4_aggregate_data = []

    for scenario_num in shock_scenarios:
        if scenario_num not in SCENARIO_NAMES:
            continue

        scenario_name = SCENARIO_NAMES[scenario_num]
        print(f"Processing scenario {scenario_num}: {scenario_name}")

        shock_moments = compute_aggregate_shock_moments(OUTPUT_DIR, scenario_num, NUM_RUNS)

        if shock_moments is None:
            print(f" Failed to compute shock moments")
            continue

        # Sanity check: shock intensity should increase over time
        means = [shock_moments[year]['mean'] for year in sorted(shock_moments.keys())
                 if not np.isnan(shock_moments[year]['mean'])]
        if len(means) >= 2:
            if means[-1] <= means[0] * 1.01:  # Less than 1% increase
                print(f"WARNING: Shock intensity does not increase over time")
                print(f"            Year 2000: {means[0]:.4f}, Year 2100: {means[-1]:.4f}")
                print(f"            Verify climate–damage parameterisation.")

        # Add mean row (Average value of shocks)
        mean_row = {'Scenario': scenario_name, 'Statistic': 'Average value of shocks'}
        for year in sorted(shock_moments.keys()):
            mean_val = shock_moments[year]['mean']
            mean_row[str(year)] = f"{mean_val*100:.3f}%" if not np.isnan(mean_val) else "N/A"
        table4_aggregate_data.append(mean_row)

        # Add std row (Standard deviation of shocks)
        std_row = {'Scenario': scenario_name, 'Statistic': 'Standard deviation of shocks'}
        for year in sorted(shock_moments.keys()):
            std_val = shock_moments[year]['std']
            std_row[str(year)] = f"{std_val*100:.3f}%" if not np.isnan(std_val) else "N/A"
        table4_aggregate_data.append(std_row)

        # Add CV row (Coefficient of variation)
        cv_row = {'Scenario': scenario_name, 'Statistic': 'Coefficient of variation'}
        for year in sorted(shock_moments.keys()):
            cv_val = shock_moments[year]['cv']
            cv_row[str(year)] = f"{cv_val:.3f}" if not np.isnan(cv_val) else "N/A"
        table4_aggregate_data.append(cv_row)

        print(f"  Computed aggregate shock moments ({shock_moments[2000]['n_runs']} runs)")

    if len(table4_aggregate_data) > 0:
        table4_agg_df = pd.DataFrame(table4_aggregate_data)

        # Save CSV
        table4_agg_df.to_csv('./output/verifications/table4.csv', index=False)

        # Save Markdown
        with open('./output/verifications/table4.md', 'w') as f:
            f.write("# Table 4: First and second moment of climate shock size over time (Corrected)\n\n")
            f.write(f"Reported values are averages over a Monte Carlo of size {NUM_RUNS}.\n\n")
            f.write("**CORRECTED METHODOLOGY**:\n\n")
            f.write("This table uses the mathematically correct computation:\n\n")
            f.write("1. For each shock type i (1 to 9):\n")
            f.write("   - Compute Beta(α_i, β_i) moments: mean_i, std_i, cv_i\n")
            f.write("2. Average moments across shock types: mean(mean_i), mean(std_i), mean(cv_i)\n")
            f.write("3. Average across 4 quarters of target year\n")
            f.write("4. Average across Monte Carlo runs\n\n")
            f.write("**Why this matters**: E[mean(Beta_i)] ≠ E[Beta(mean(α), mean(β))]\n\n")
            f.write("**Beta distribution moments**:\n")
            f.write("- Mean = α / (α + β)\n")
            f.write("- Variance = (α × β) / ((α + β)² × (α + β + 1))\n")
            f.write("- Std Dev = √Variance\n")
            f.write("- CV = Std Dev / Mean\n\n")
            f.write(table4_agg_df.to_markdown(index=False))
            f.write("\n\n**Notes**:\n")
            f.write("- Values computed for 9 shock types (X_a(1-9), X_b(1-9))\n")
            f.write("- Each year represents 4-quarter average:\n")
            f.write("  - 2000 (t=201-204), 2025 (t=301-304), 2050 (t=401-404), ")
            f.write("2075 (t=501-504), 2100 (t=597-600)\n")
            f.write("- Then averaged across Monte Carlo runs\n")
            f.write("- Mean and Std Dev reported as percentages\n")
            f.write("- CV reported as unit value (dimensionless)\n")

        print("Saved lamperti_table4_correct.md")

        # Display table
        print()
        print("=" * 80)
        print("TABLE 4: AGGREGATE SHOCK STATISTICS (CORRECTED)")
        print("=" * 80)
        print(table4_agg_df.to_string(index=False))
        print()
    else:
        print("No aggregate shock statistics computed")

    print()

    # ===========================================================================
    # REGIONAL SHOCK TABLES
    # ===========================================================================
    print("=" * 80)
    print("REGIONAL SHOCK MOMENTS")
    print("=" * 80)

    regional_shock_results = {}

    for scenario_num in shock_scenarios:
        if scenario_num not in SCENARIO_NAMES:
            continue

        scenario_name = SCENARIO_NAMES[scenario_num]
        print(f"Processing scenario {scenario_num}: {scenario_name}")

        regional_moments = compute_regional_shock_moments(OUTPUT_DIR, scenario_num, NUM_RUNS)

        if regional_moments is None:
            print(f"  No regional shock data found")
            continue

        regional_shock_results[scenario_num] = regional_moments

        for region_id, moments in regional_moments.items():
            n_runs = moments[2000]['n_runs']
            print(f"  Region {region_id}: {n_runs} runs processed")

    if len(regional_shock_results) > 0:
        # Create separate table for each region
        # First, determine all regions
        all_regions = set()
        for scenario_moments in regional_shock_results.values():
            all_regions.update(scenario_moments.keys())

        all_regions = sorted(all_regions, key=lambda x: int(x))

        for region_id in all_regions:
            table4_regional_data = []

            for scenario_num in sorted(regional_shock_results.keys()):
                if region_id not in regional_shock_results[scenario_num]:
                    continue

                scenario_name = SCENARIO_NAMES[scenario_num]
                shock_moments = regional_shock_results[scenario_num][region_id]

                # Add mean row (Average value of shocks)
                mean_row = {'Scenario': scenario_name, 'Statistic': 'Average value of shocks'}
                for year in sorted(shock_moments.keys()):
                    mean_val = shock_moments[year]['mean']
                    mean_row[str(year)] = f"{mean_val*100:.3f}%" if not np.isnan(mean_val) else "N/A"
                table4_regional_data.append(mean_row)

                # Add std row (Standard deviation of shocks)
                std_row = {'Scenario': scenario_name, 'Statistic': 'Standard deviation of shocks'}
                for year in sorted(shock_moments.keys()):
                    std_val = shock_moments[year]['std']
                    std_row[str(year)] = f"{std_val*100:.3f}%" if not np.isnan(std_val) else "N/A"
                table4_regional_data.append(std_row)

                # Add CV row (Coefficient of variation)
                cv_row = {'Scenario': scenario_name, 'Statistic': 'Coefficient of variation'}
                for year in sorted(shock_moments.keys()):
                    cv_val = shock_moments[year]['cv']
                    cv_row[str(year)] = f"{cv_val:.4f}" if not np.isnan(cv_val) else "N/A"
                table4_regional_data.append(cv_row)

            # Create DataFrame for regional table
            if table4_regional_data:
                table4_reg_df = pd.DataFrame(table4_regional_data)

                # Save CSV
                csv_filename = f'./output/verifications/table4_region_{region_id}_correct.csv'
                table4_reg_df.to_csv(csv_filename, index=False)

                # Save Markdown
                md_filename = f'./output/verifications/table4_region_{region_id}_correct.md'
                with open(md_filename, 'w') as f:
                    f.write(
                        f"# Table 4: First and second moment of climate shock size over time - Region {region_id} (CORRECTED)\n\n")
                    f.write(f"Reported values are averages over a Monte Carlo of size {NUM_RUNS}.\n\n")
                    f.write("**CORRECTED METHODOLOGY**:\n\n")
                    f.write("1. For each shock type i: compute Beta(α_i, β_i) moments\n")
                    f.write("2. Average moments across shock types\n")
                    f.write("3. Average across 4 quarters\n")
                    f.write("4. Average across Monte Carlo runs\n\n")
                    f.write("**Why this matters**: E[mean(Beta_i)] ≠ E[Beta(mean(α), mean(β))]\n\n")
                    f.write("**Beta distribution moments**:\n")
                    f.write("- Mean = α / (α + β)\n")
                    f.write("- Variance = (α × β) / ((α + β)² × (α + β + 1))\n")
                    f.write("- Std Dev = √Variance\n")
                    f.write("- CV = Std Dev / Mean\n\n")
                    f.write(table4_reg_df.to_markdown(index=False))
                    f.write("\n\n**Notes**:\n")
                    f.write("- Values computed for 9 shock types per region\n")
                    f.write("- Each year represents 4-quarter average:\n")
                    f.write("  - 2000 (t=201-204), 2025 (t=301-304), 2050 (t=401-404), ")
                    f.write("2075 (t=501-504), 2100 (t=597-600)\n")
                    f.write("- Then averaged across Monte Carlo runs\n")
                    f.write("- Mean and Std Dev reported as percentages\n")
                    f.write("- CV reported as unit value (dimensionless)\n")

                print(f" Saved {csv_filename} and {md_filename}")

        print()

    else:
        print("No regional shock data found in any scenario")

    print()

    # ===========================================================================
    # TABLES 4a & 4b: SHOCK SUMMARIES WITH AGGREGATED CV CALCULATION
    # ===========================================================================
    print("=" * 80)
    print("GENERATING TABLES 4a & 4b: SHOCK SUMMARIES (AGGREGATED CV)")
    print("=" * 80)
    print()
    print("Methodology: CV computed at run-year aggregated level")
    print("  CV_run_year = (mean of stds) / (mean of means)")
    print("  Then averaged across Monte Carlo runs")
    print()

    # ===========================================================================
    # TABLE 4a: NATIONAL SHOCK SUMMARY
    # ===========================================================================
    print("=" * 80)
    print("TABLE 4a: NATIONAL SHOCK SUMMARY")
    print("=" * 80)

    table4a_data = []

    for scenario_num in shock_scenarios:
        if scenario_num not in SCENARIO_NAMES:
            continue

        scenario_name = SCENARIO_NAMES[scenario_num]
        print(f"Processing scenario {scenario_num}: {scenario_name}")

        summary = compute_national_shock_summary(OUTPUT_DIR, scenario_num, NUM_RUNS)

        if summary is None or all(np.isnan(summary[year]['mean']) for year in summary.keys()):
            print(f"  → No national shock data")

            # Add rows with N/A
            mean_row = {'Scenario': scenario_name, 'Statistic': 'Average value of shocks'}
            std_row = {'Scenario': scenario_name, 'Statistic': 'Standard deviation of shocks'}
            cv_row = {'Scenario': scenario_name, 'Statistic': 'Coefficient of variation'}

            for year in [2000, 2025, 2050, 2075, 2100]:
                mean_row[str(year)] = "N/A"
                std_row[str(year)] = "N/A"
                cv_row[str(year)] = "N/A"

            table4a_data.extend([mean_row, std_row, cv_row])
            continue

        # Add rows with data
        mean_row = {'Scenario': scenario_name, 'Statistic': 'Average value of shocks'}
        std_row = {'Scenario': scenario_name, 'Statistic': 'Standard deviation of shocks'}
        cv_row = {'Scenario': scenario_name, 'Statistic': 'Coefficient of variation'}

        for year in sorted(summary.keys()):
            mean_val = summary[year]['mean']
            std_val = summary[year]['std']
            cv_val = summary[year]['cv']
            n_runs = summary[year]['n_runs']

            mean_row[str(year)] = f"{mean_val*100:.3f}%" if not np.isnan(mean_val) else "N/A"
            std_row[str(year)] = f"{std_val*100:.3f}%" if not np.isnan(std_val) else "N/A"
            cv_row[str(year)] = f"{cv_val:.3f}" if not np.isnan(cv_val) else "N/A"

        table4a_data.extend([mean_row, std_row, cv_row])

        # Report number of runs used
        first_year = list(summary.keys())[0]
        print(f"  ✓ Computed from {summary[first_year]['n_runs']} runs")

    # Save TABLE 4a
    if len(table4a_data) > 0:
        table4a_df = pd.DataFrame(table4a_data)

        # Save CSV
        table4a_df.to_csv('./output/verifications/national_shock_summary.csv', index=False)

        # Save Markdown
        with open('./output/verifications/national_shock_summary.md', 'w') as f:
            f.write("# Table 4a: National Shock Summary\n\n")
            f.write(f"Reported values are averages over Monte Carlo of size {NUM_RUNS}.\n\n")
            f.write("**Methodology**:\n\n")
            f.write("1. For each shock type i: compute Beta(α_i, β_i) moments\n")
            f.write("2. For each quarter: average means and stds across shock types\n")
            f.write("3. For each year: average quarterly values\n")
            f.write("4. Compute CV at run-year level: CV = (avg_std) / (avg_mean)\n")
            f.write("5. Average across Monte Carlo runs\n\n")
            f.write("**Beta distribution moments**:\n")
            f.write("- Mean = α / (α + β)\n")
            f.write("- Std Dev = √[α×β / ((α+β)²×(α+β+1))]\n\n")
            f.write(table4a_df.to_markdown(index=False))
            f.write("\n\n**Notes**:\n")
            f.write("- Mean and Std Dev reported as percentages (3 decimals)\n")
            f.write("- CV reported as dimensionless value (3 decimals)\n")
            f.write("- Target years: 2000 (t=201-204), 2025 (t=301-304), 2050 (t=401-404), ")
            f.write("2075 (t=501-504), 2100 (t=597-600)\n")

        print()

        # Display table
        print()
        print("=" * 80)
        print("TABLE 4a: NATIONAL SHOCK SUMMARY")
        print("=" * 80)
        print(table4a_df.to_string(index=False))
        print()
    else:
        print("No national shock data found")

    print()

    # ===========================================================================
    # TABLE 4b: REGIONAL SHOCK SUMMARIES
    # ===========================================================================
    print("=" * 80)
    print("TABLE 4b: REGIONAL SHOCK SUMMARIES")
    print("=" * 80)

    table4b_generated = False

    for scenario_num in shock_scenarios:
        if scenario_num not in SCENARIO_NAMES:
            continue

        scenario_name = SCENARIO_NAMES[scenario_num]
        print(f"Processing scenario {scenario_num}: {scenario_name}")

        regional_summary = compute_regional_shock_summary(OUTPUT_DIR, scenario_num, NUM_RUNS)

        if regional_summary is None:
            print(f"No regional shock data")
            continue

        # On first scenario with regional data, initialize storage
        if not table4b_generated:
            all_region_ids = sorted(regional_summary.keys())
            table4b_data = {region_id: [] for region_id in all_region_ids}
            table4b_generated = True

        # Add data for each region
        for region_id, region_results in regional_summary.items():
            print(f"  ✓ Region {region_id}: {region_results[2000]['n_runs']} runs")

            mean_row = {'Scenario': scenario_name, 'Statistic': 'Average value of shocks'}
            std_row = {'Scenario': scenario_name, 'Statistic': 'Standard deviation of shocks'}
            cv_row = {'Scenario': scenario_name, 'Statistic': 'Coefficient of variation'}

            for year in sorted(region_results.keys()):
                mean_val = region_results[year]['mean']
                std_val = region_results[year]['std']
                cv_val = region_results[year]['cv']

                mean_row[str(year)] = f"{mean_val*100:.3f}%" if not np.isnan(mean_val) else "N/A"
                std_row[str(year)] = f"{std_val*100:.3f}%" if not np.isnan(std_val) else "N/A"
                cv_row[str(year)] = f"{cv_val:.3f}" if not np.isnan(cv_val) else "N/A"

            table4b_data[region_id].extend([mean_row, std_row, cv_row])

    # Save TABLE 4b files (one per region)
    if table4b_generated:
        for region_id in all_region_ids:
            if len(table4b_data[region_id]) > 0:
                table4b_df = pd.DataFrame(table4b_data[region_id])

                # Save CSV
                csv_filename = f'./output/verifications/regional_shock_summary_region_{region_id}.csv'
                table4b_df.to_csv(csv_filename, index=False)

                # Save Markdown
                md_filename = f'./output/verifications/regional_shock_summary_region_{region_id}.md'
                with open(md_filename, 'w') as f:
                    f.write(f"# Table 4b: Regional Shock Summary - Region {region_id}\n\n")
                    f.write(f"Reported values are averages over Monte Carlo of size {NUM_RUNS}.\n\n")
                    f.write("**Methodology**:\n\n")
                    f.write("1. For each shock type i: compute Beta(α_i, β_i) moments\n")
                    f.write("2. For each quarter: average means and stds across shock types\n")
                    f.write("3. For each year: average quarterly values\n")
                    f.write("4. Compute CV at run-year level: CV = (avg_std) / (avg_mean)\n")
                    f.write("5. Average across Monte Carlo runs\n\n")
                    f.write("**Beta distribution moments**:\n")
                    f.write("- Mean = α / (α + β)\n")
                    f.write("- Std Dev = √[α×β / ((α+β)²×(α+β+1))]\n\n")
                    f.write(table4b_df.to_markdown(index=False))
                    f.write("\n\n**Notes**:\n")
                    f.write("- Mean and Std Dev reported as percentages (3 decimals)\n")
                    f.write("- CV reported as dimensionless value (3 decimals)\n")
                    f.write("- Target years: 2000 (t=201-204), 2025 (t=301-304), 2050 (t=401-404), ")
                    f.write("2075 (t=501-504), 2100 (t=597-600)\n")

                print(f"\n✓ Saved {csv_filename}")
                print(f"✓ Saved {md_filename}")

        print()
        print(f"✓ Generated Table 4b for {len(all_region_ids)} regions")
    else:
        print("  → No regional shock data found in any scenario")

    print()

    # FINAL SUMMARY

    print("=" * 80)
    print("ANALYSIS COMPLETE")
    print("=" * 80)
    print()
    print("Output files created:")
    print("  1. metrics_run_level.csv - Run-level metrics for all scenarios")
    print("  2. table5_levels.csv/.md - Table 5: Absolute performance levels")
    print("  3. table5_regions.csv/.md - Regional Table 5: Absolute levels")
    print("  4. table4_correct.csv - Table 4: Aggregate shock statistics")
    print("  5. table4_correct.md - Table 4: Aggregate shock statistics")
    if len(regional_shock_results) > 0:
        for region_id in all_regions:
            print(
                f"  6. table4_region_{region_id}_correct.csv/.md - Region {region_id} shock statistics")
    print("  7. national_shock_summary.csv/.md - Table 4a: National shock summary (aggregated CV)")
    if table4b_generated:
        for region_id in all_region_ids:
            print(
                f"  8. regional_shock_summary_region_{region_id}.csv/.md - Table 4b: Region {region_id} shock summary (aggregated CV)")
    print()
    print("Note: Tables 4a/4b use CV computed at run-year aggregated level")
    print()


if __name__ == "__main__":
    main()
