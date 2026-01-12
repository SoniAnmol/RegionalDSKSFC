# %%
import pandas as pd
import matplotlib.pyplot as plt
import glob
import os
import re
from pathlib import Path
from typing import Dict, List, Tuple

import matplotlib
matplotlib.use("Agg")  # headless

# %% Config
# Set a target run name to force plotting that run. If None, the script will
# search for the latest run that has matching regional files.
TARGET_RUN = "regtest"

# Include variable names in legend labels for easier variable matching
INCLUDE_VARIABLE_NAME_IN_LEGEND = True

# %% Column names for national resultsexp (74 columns, 1-based in model comments)
NATIONAL_COLS: List[str] = [
    "t",                       # 1
    "GDP_r",                   # 2
    "Consumption_r",           # 3
    "Investment_r",            # 4
    "employment_rate",         # 5 (1 - U)
    "cpi_ratio",               # 6 (cpi(1)/cpi(2))
    "Emiss_total",             # 7 (Emiss1+Emiss2+Emiss_en)
    "D_en",                    # 8 (D_en_TOT)
    "LS",                      # 9
    "K_sum",                   # 10 (K.Sum)
    "A1_en_ratio",             # 11 (dead/survive)
    "A2_en_ratio",             # 12 (dead/survive)
    "Am_en",                   # 13
    "Am_a",                    # 14
    "exit_marketshare2_sum",   # 15
    "exit_payments2_sum",      # 16
    "exit_equity2_sum",        # 17
    "exiting_1_count",         # 18
    "Bailout",                 # 19
    "baddebt_b_gdp",           # 20
    "counter_bankfailure",     # 21
    "CapitalStock_gdp",        # 22
    "NW_cb_gdp",               # 23
    "NW_h_gdp",                # 24
    "NW_2_gdp",                # 25
    "NW_b_gdp",                # 26
    "BankProfits",             # 27
    "Loans_2_gdp",             # 28
    "CreditDemand_to_Supply",  # 29
    "NW_gov_gdp",              # 30
    "NW_e_gdp",                # 31
    "NW_1_gdp",                # 32
    "counter_bankfailure_cap",  # 33 (min(1,counter))
    "cpi",                     # 34
    "exp_quota",               # 35
    "real_wage",               # 36 (w/cpi)
    "Am2",                     # 37
    "Am1",                     # 38
    "GDP_n",                   # 39
    "Investment_n",            # 40 (scaled)
    "Consumption_n",           # 41
    "t_CO2_en",                # 42
    "ReplacementInvestment_r",  # 43
    "Emiss1_TOT",              # 44
    "Emiss2_TOT",              # 45
    "Emiss_en",                # 46
    "Tmixed",                  # 47
    "EnergyPayments",          # 48
    "FuelCost_GDP",            # 49
    "K_green_share",           # 50 (K_gelag/(K_gelag+K_delag))
    "Deposits_e_gdp",          # 51
    "CapitalStock_e_gdp",      # 52
    "Pitot1",                  # 53
    "Pitot2",                  # 54
    "ProfitEnergy",            # 55
    "Real_Wages",              # 56 (Wages/cpi)
    "Dividends_1",             # 57
    "Dividends_2",             # 58
    "Consumption_dup",         # 59 (duplicate in source)
    "c_en",                    # 60
    "Deposits_2_gdp",          # 61
    "Deposits_1_gdp",          # 62
    "Deposits_sum_gdp",        # 63
    "exit_payments_equity_sum",  # 64
    "Pitot_sum",               # 65
    "Dividends_total_real",    # 66
    "Dividends_e_real",        # 67
    "Dividends_b_real",        # 68
    "NW_firms_gdp",            # 69 (NW_1+NW_2)
    "GB_gdp",                  # 70
    "cpi_5_ratio",             # 71
    "GDP_r_growth4",           # 72 (pow(GDP_r(1)/GDP_r(2),4)-1)
    "NW_f_gdp",                # 73
    "wage_income_share",       # 74
]

# Column names for regional resultsexp rows (34 columns)
REGION_COLS: List[str] = [
    "t",                  # 1
    "reg_GDP_r",          # 2
    "reg_S1",             # 3
    "reg_S2",             # 4
    "reg_Investment",     # 5
    "reg_K",              # 6
    "reg_employment_rate",  # 7
    "reg_LS_used",        # 8
    "reg_Emiss_total",    # 9
    "reg_Emiss1",         # 10
    "reg_Emiss2",         # 11
    "reg_Emiss_en",       # 12
    "reg_D_en",           # 13
    "reg_dirty_cap",      # 14
    "reg_green_cap",      # 15
    "reg_green_share",    # 16
    "reg_Am1",            # 17
    "reg_Am2",            # 18
    "reg_A1en_ratio",     # 19
    "reg_A2en_ratio",     # 20
    "reg_exit1_count",    # 21
    "reg_exit2_count",    # 22
    "reg_exit_ms2",       # 23
    "reg_exit_pay2",      # 24
    "reg_exit_eq2",       # 25
    "reg_Pi1",            # 26
    "reg_Pi2",            # 27
    "reg_NW1",            # 28
    "reg_NW2",            # 29
    "reg_NW_e",           # 30
    "reg_Deposits1",      # 31
    "reg_Deposits2",      # 32
    "reg_CapStock1",      # 33
    "reg_CapStock2",      # 34
]

# Map regional variables to national counterparts for overlay
REG_TO_NAT: Dict[str, str] = {
    "reg_GDP_r": "GDP_r",
    "reg_S1": "Pitot1",
    "reg_S2": "Pitot2",
    "reg_Investment": "Investment_r",
    "reg_K": "K_sum",
    "reg_employment_rate": "employment_rate",
    "reg_LS_used": "LS",
    "reg_Emiss_total": "Emiss_total",
    "reg_Emiss1": "Emiss1_TOT",
    "reg_Emiss2": "Emiss2_TOT",
    "reg_Emiss_en": "Emiss_en",
    "reg_D_en": "D_en",
    "reg_dirty_cap": "K_dirty_share",  # Not in national cols, but this is the best available
    "reg_green_cap": "K_green_share",  # Not in national cols, but this is the best available
    "reg_Am1": "Am1",
    "reg_Am2": "Am2",
    "reg_A1en_ratio": "A1_en_ratio",
    "reg_A2en_ratio": "A2_en_ratio",
    "reg_exit1_count": "exiting_1_count",
    "reg_exit2_count": "exit_equity2_sum",  # Approximation: total C-firm exits
    "reg_exit_ms2": "exit_marketshare2_sum",
    "reg_exit_pay2": "exit_payments2_sum",
    "reg_exit_eq2": "exit_equity2_sum",
    "reg_Pi1": "Pitot1",  # K-firm profits → K-firm total income
    "reg_Pi2": "Pitot2",  # C-firm profits → C-firm total income
    "reg_NW1": "NW_1_gdp",
    "reg_NW2": "NW_2_gdp",
    "reg_NW_e": "NW_e_gdp",
    "reg_Deposits1": "Deposits_1_gdp",
    "reg_Deposits2": "Deposits_2_gdp",
    "reg_CapStock1": "CapitalStock_gdp",  # Approximation
    "reg_CapStock2": "CapitalStock_gdp",  # Approximation
}

# Variable naming dictionary for better plot titles and labels
VAR_LABELS: Dict[str, str] = {
    "reg_GDP_r": "Regional Real GDP",
    "reg_S1": "Regional K-firm Nominal Sales",
    "reg_S2": "Regional C-firm Nominal Sales",
    "reg_Investment": "Regional Investment",
    "reg_K": "Regional Capital Stock (C-firms)",
    "reg_employment_rate": "Regional Employment Rate",
    "reg_LS_used": "Regional Labor Demand",
    "reg_Emiss_total": "Regional Total Emissions",
    "reg_Emiss1": "Regional K-firm Emissions",
    "reg_Emiss2": "Regional C-firm Emissions",
    "reg_Emiss_en": "Regional Energy Sector Emissions",
    "reg_D_en": "Regional Energy Demand",
    "reg_dirty_cap": "Regional Dirty Energy Capacity",
    "reg_green_cap": "Regional Green Energy Capacity",
    "reg_green_share": "Regional Green Capacity Share",
    "reg_Am1": "Regional Avg K-firm Productivity",
    "reg_Am2": "Regional Avg C-firm Productivity",
    "reg_A1en_ratio": "Regional K-firm Energy Productivity Ratio",
    "reg_A2en_ratio": "Regional C-firm Energy Productivity Ratio",
    "reg_exit1_count": "Regional K-firm Exits",
    "reg_exit2_count": "Regional C-firm Exits",
    "reg_exit_ms2": "Regional C-firm Exit Market Share",
    "reg_exit_pay2": "Regional C-firm Exit Payments",
    "reg_exit_eq2": "Regional C-firm Exit Equity",
    "reg_Pi1": "Regional K-firm Profits",
    "reg_Pi2": "Regional C-firm Profits",
    "reg_NW1": "Regional K-firm Net Worth",
    "reg_NW2": "Regional C-firm Net Worth",
    "reg_NW_e": "Regional Energy Sector Net Worth",
    "reg_Deposits1": "Regional K-firm Deposits",
    "reg_Deposits2": "Regional C-firm Deposits",
    "reg_CapStock1": "Regional K-firm Capital Stock Value",
    "reg_CapStock2": "Regional C-firm Capital Stock Value",
}


def find_run_seed(run_hint: str) -> Tuple[Path, str, str]:
    """Return national resultsexp file for the given run that has regional files."""
    root = Path(__file__).resolve().parent.parent
    pattern = str(root / f"output/resultsexp_{run_hint}_*.txt")
    candidates = [Path(p) for p in glob.glob(pattern)]

    if not candidates:
        raise FileNotFoundError(f"No resultsexp_{run_hint}_*.txt files found in output/")

    candidates_sorted = sorted(candidates, key=lambda p: p.stat().st_mtime, reverse=True)
    for cand in candidates_sorted:
        m = re.match(rf"resultsexp_{re.escape(run_hint)}_([0-9]+)\.txt", cand.name)
        if not m:
            continue
        seed = m.group(1)
        region_pattern = str(root / f"output/resultsexp_reg*_{run_hint}_{seed}.txt")
        if glob.glob(region_pattern):
            return cand, run_hint, seed

    raise FileNotFoundError(f"No resultsexp_{run_hint}_*.txt file has matching regional files")


def find_region_files(run: str, seed: str) -> List[Tuple[int, Path]]:
    root = Path(__file__).resolve().parent.parent
    pattern = str(root / f"output/resultsexp_reg*_{run}_{seed}.txt")
    files = []
    for path_str in glob.glob(pattern):
        name = os.path.basename(path_str)
        m = re.match(r"resultsexp_reg([0-9]+)_" + re.escape(run) + r"_" + re.escape(seed) + r"\.txt", name)
        if m:
            files.append((int(m.group(1)), Path(path_str)))
    if not files:
        raise FileNotFoundError(f"No regional files matching {pattern}")
    return sorted(files, key=lambda x: x[0])


def read_fwf(path: Path, cols: List[str]) -> pd.DataFrame:
    df = pd.read_fwf(path, header=None, names=cols)
    return df


def align_on_time(base_t: pd.Series, df: pd.DataFrame, col: str) -> pd.Series:
    if "t" not in df.columns:
        raise ValueError("Dataframe missing time column 't'")
    merged = pd.merge(pd.DataFrame({"t": base_t}), df[["t", col]], on="t", how="left")
    return merged[col]


def make_plots():
    # 1. Find the run/seed for TARGET_RUN
    if TARGET_RUN is None:
        raise ValueError("TARGET_RUN must be set to a run name (e.g., 'regtest')")

    national_path, run, seed = find_run_seed(TARGET_RUN)
    region_files = find_region_files(run, seed)

    national_df = read_fwf(national_path, NATIONAL_COLS)
    region_dfs = []
    for idx, path in region_files:
        df = read_fwf(path, REGION_COLS)
        df["region"] = idx
        region_dfs.append(df)

    # Use first region timeline as base
    base_t = region_dfs[0]["t"]

    plot_root = Path("output/plots") / run
    plot_root.mkdir(parents=True, exist_ok=True)

    # Generate region labels based on INCLUDE_VARIABLE_NAME_IN_LEGEND flag
    # (will be updated for each variable in the loop below)
    for var in REGION_COLS[1:]:  # skip time
        # Stack values per region
        region_var_df = pd.DataFrame({"t": base_t})
        for (idx, _), df in zip(region_files, region_dfs):
            region_var_df[f"region_{idx}"] = align_on_time(base_t, df, var)

        y_arrays = [region_var_df[col].fillna(0).values for col in region_var_df.columns if col != "t"]

        # Create region labels with or without variable name
        if INCLUDE_VARIABLE_NAME_IN_LEGEND:
            region_labels = [f"region {idx} {var}" for idx, _ in region_files]
        else:
            region_labels = [f"region {idx}" for idx, _ in region_files]

        fig, ax = plt.subplots(figsize=(10, 6))
        ax.stackplot(region_var_df["t"], y_arrays, labels=region_labels, alpha=0.7)

        # Overlay national if mapped
        if var in REG_TO_NAT:
            nat_var = REG_TO_NAT[var]
            if nat_var in national_df.columns:
                nat_series = align_on_time(base_t, national_df, nat_var)
                # Add variable name to national label if flag is set
                nat_label = f"national {nat_var}" if INCLUDE_VARIABLE_NAME_IN_LEGEND else "national"
                ax.plot(base_t, nat_series, color="black", linewidth=2, label=nat_label)

        # Use VAR_LABELS for better titles and labels
        var_label = VAR_LABELS.get(var, var)
        ax.set_title(f"{var_label} (stacked regions with national overlay)", fontsize=12, fontweight='bold')
        ax.set_xlabel("Time Period (t)", fontsize=10)
        ax.set_ylabel(var_label, fontsize=10)
        ax.legend(loc="best", fontsize=8)
        ax.grid(True, alpha=0.3)
        outfile = plot_root / f"{var}.png"
        fig.tight_layout()
        fig.savefig(outfile, dpi=150)
        plt.close(fig)

    print(f"Plots written to {plot_root}")


if __name__ == "__main__":
    make_plots()
# %%
