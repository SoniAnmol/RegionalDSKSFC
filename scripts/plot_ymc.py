#!/usr/bin/env python3
import pandas as pd
import pathlib
import re
import matplotlib.pyplot as plt
from datetime import datetime

OUT_DIR = pathlib.Path('output')

# Flag to show variable name in legends
show_variable_name = True

# Flag to show plot titles
show_plot_title = False

# Static column definitions (indices are 0-based in pandas)
YMC_COLS = [
    't',
    'GDP_r_1',
    'Consumption_r',
    'Investment_r',
    'EmploymentRate_1',
    'cpi_1',
    'cpi_1_over_cpi_2',
    'Am_1',
    'Deficit',
    'GB_1',
    'w_1',
    'w_1_over_w_2',
    'r',
    'r_bonds',
    'Loans_2_Row1_Sum',
    'Deposits_Row1_Sum',
    'baddebt_b_Sum',
    'CreditSupply_all',
    'CreditDemand_all',
    'Inventories_Row1_Sum',
    'N_Row1_Sum',
    'Bailout',
    'GDP_n_1',
    'Q_ge_over_D_en_TOT_1',
    'D_en_TOT_1',
    'Emiss_TOT_1',
    'Cum_emissions',
    'Tmixed_1',
    'Q1tot',
    'Q2tot',
    'N1r',
    'N2r',
    'LS',
    'Q_ge',
    'Q_de',
    'Emiss1_TOT',
    'Emiss2_TOT',
    'Emiss_en',
]

YMC_LABELS = {
    't': 'time',
    'GDP_r_1': 'GDP_real',
    'Consumption_r': 'Consumption',
    'Investment_r': 'Investment',
    'EmploymentRate_1': 'EmploymentRate',
    'cpi_1': 'CPI',
    'cpi_1_over_cpi_2': 'CPI_rel',
    'Am_1': 'Productivity_A',
    'Deficit': 'PublicDeficit',
    'GB_1': 'GovDebt',
    'w_1': 'Wage',
    'w_1_over_w_2': 'Wage_rel',
    'r': 'PolicyRate',
    'r_bonds': 'BondRate',
    'Loans_2_Row1_Sum': 'TotalLoans',
    'Deposits_Row1_Sum': 'TotalDeposits',
    'baddebt_b_Sum': 'BankBadDebt',
    'CreditSupply_all': 'CreditSupply',
    'CreditDemand_all': 'CreditDemand',
    'Inventories_Row1_Sum': 'Inventories',
    'N_Row1_Sum': 'Employment',
    'Bailout': 'Bailouts',
    'GDP_n_1': 'GDP_nominal',
    'Q_ge_over_D_en_TOT_1': 'GreenEnergyShare',
    'D_en_TOT_1': 'EnergyDemand',
    'Emiss_TOT_1': 'TotalEmissions',
    'Cum_emissions': 'CumulativeEmissions',
    'Tmixed_1': 'Temperature',
    'Q1tot': 'Q_KGoods',
    'Q2tot': 'Q_CGoods',
    'N1r': 'Num_KFirms',
    'N2r': 'Num_CFirms',
    'LS': 'LabourSupply',
    'Q_ge': 'GreenEnergy',
    'Q_de': 'DirtyEnergy',
    'Emiss1_TOT': 'Emissions_KFirms',
    'Emiss2_TOT': 'Emissions_CFirms',
    'Emiss_en': 'Emissions_Energy',
}

REG_COLS = [
    't',
    'reg_GDP_r',
    'reg_Consumption_r',
    'reg_Investment_r',
    'reg_EmploymentRate',
    'reg_Am',
    'reg_Loans_2',
    'reg_Inventories',
    'reg_N',
    'reg_GDP_n',
    'reg_Q_ge_over_D_en',
    'reg_D_en_TOT',
    'reg_Emiss_TOT',
    'reg_Cum_emissions',
    'reg_Q1tot',
    'reg_Q2tot',
    'reg_N1',
    'reg_N2',
    'reg_LS',
    'reg_Q_ge',
    'reg_Q_de',
    'reg_Emiss1_TOT',
    'reg_Emiss2_TOT',
    'reg_Emiss_en',
]

REG_LABELS = {
    't': 'time',
    'reg_GDP_r': 'GDP_real_reg',
    'reg_Consumption_r': 'Consumption_real_reg',
    'reg_Investment_r': 'Investment_real_reg',
    'reg_EmploymentRate': 'EmploymentRate_reg',
    'reg_Am': 'Productivity_reg',
    'reg_Loans_2': 'Loans_reg',
    'reg_Inventories': 'Inventories_nominal_reg',
    'reg_N': 'Inventories_real_reg',
    'reg_GDP_n': 'GDP_nominal_reg',
    'reg_Q_ge_over_D_en': 'GreenEnergyShare_reg',
    'reg_D_en_TOT': 'EnergyDemand_reg',
    'reg_Emiss_TOT': 'TotalEmissions_reg',
    'reg_Cum_emissions': 'CumulativeEmissions_reg',
    'reg_Q1tot': 'Output_KFirms_reg',
    'reg_Q2tot': 'Output_CFirms_reg',
    'reg_N1': 'Num_KFirms_reg',
    'reg_N2': 'Num_CFirms_reg',
    'reg_LS': 'LabourSupply_reg',
    'reg_Q_ge': 'GreenEnergy_reg',
    'reg_Q_de': 'DirtyEnergy_reg',
    'reg_Emiss1_TOT': 'Emissions_KFirms_reg',
    'reg_Emiss2_TOT': 'Emissions_CFirms_reg',
    'reg_Emiss_en': 'Emissions_Energy_reg',
}

REG_TO_YMC = {
    'reg_GDP_r': 'GDP_r_1',
    'reg_Consumption_r': 'Consumption_r',
    'reg_Investment_r': 'Investment_r',
    'reg_EmploymentRate': 'EmploymentRate_1',
    'reg_Am': 'Am_1',
    'reg_Loans_2': 'Loans_2_Row1_Sum',
    'reg_Inventories': 'Inventories_Row1_Sum',
    'reg_N': 'N_Row1_Sum',
    'reg_GDP_n': 'GDP_n_1',
    'reg_Q_ge_over_D_en': 'Q_ge_over_D_en_TOT_1',
    'reg_D_en_TOT': 'D_en_TOT_1',
    'reg_Emiss_TOT': 'Emiss_TOT_1',
    'reg_Cum_emissions': 'Cum_emissions',
    'reg_Q1tot': 'Q1tot',
    'reg_Q2tot': 'Q2tot',
    'reg_N1': 'N1r',
    'reg_N2': 'N2r',
    'reg_LS': 'LS',
    'reg_Q_ge': 'Q_ge',
    'reg_Q_de': 'Q_de',
    'reg_Emiss1_TOT': 'Emiss1_TOT',
    'reg_Emiss2_TOT': 'Emiss2_TOT',
    'reg_Emiss_en': 'Emiss_en',
}

# Find latest ymc file by modification time
ymc_files = sorted(OUT_DIR.glob('ymc_*.txt'), key=lambda p: p.stat().st_mtime, reverse=True)
# Exclude metadata files (legacy)
ymc_files = [f for f in ymc_files if 'metadata' not in f.name]
if not ymc_files:
    raise SystemExit('No ymc_*.txt found in output/. Run the model first.')
ymc_path = ymc_files[0]

# Extract run name and seed from ymc filename: ymc_<run>_<seed>.txt
ymc_match = re.match(r'ymc_(.+)_(\d+)\.txt', ymc_path.name)
if not ymc_match:
    raise SystemExit(f'Cannot parse ymc filename: {ymc_path.name}')
run_name = ymc_match.group(1)
seed = ymc_match.group(2)

# Create plots directory using run name (default to 'figures' if run name is empty or generic)
if run_name and run_name not in ['', 'output', 'results']:
    PLOTS_DIR = OUT_DIR / 'plots' / run_name
else:
    PLOTS_DIR = OUT_DIR / 'plots' / 'figures'
PLOTS_DIR.mkdir(parents=True, exist_ok=True)

# Find regional files pattern resultsexp_reg{r}_*.txt (pick most recent per region)
reg_files = {}
for p in OUT_DIR.glob('resultsexp_reg*_*.txt'):
    m = re.match(r'resultsexp_reg(\d+)_.*', p.name)
    if not m:
        continue
    r = int(m.group(1))
    prev = reg_files.get(r)
    if prev is None or p.stat().st_mtime > prev.stat().st_mtime:
        reg_files[r] = p

if not reg_files:
    raise SystemExit('No regional result files found (resultsexp_reg*_*.txt). Run regional scenario.')

regions = sorted(reg_files.keys())

# Read ymc/regional with explicit 60-char column widths to avoid inference drift
ymc = pd.read_fwf(ymc_path, header=None, names=YMC_COLS, widths=[60] * len(YMC_COLS))
region_dfs = {
    r: pd.read_fwf(path, header=None, names=REG_COLS, widths=[60] * len(REG_COLS))
    for r, path in reg_files.items()
}

# Build mapping from regional column name -> national column name (both strings)
YMC_BY_REGION_COL = {reg_col: REG_TO_YMC[reg_col] for reg_col in REG_TO_YMC if reg_col in REG_COLS}

print(f'\nPlotting {len(YMC_BY_REGION_COL)} variables from run: {run_name}, seed: {seed}')
print(f'Found {len(regions)} regions')
print(f'Saving plots to: {PLOTS_DIR}')

# Time axis from ymc and regional; align by length
x_nat = ymc['t'].to_numpy()

for reg_col, ymc_col in YMC_BY_REGION_COL.items():
    plt.figure(figsize=(12, 6))

    var_label = REG_LABELS.get(reg_col, reg_col)

    # Special handling for Q1tot and Q2tot: enhanced stacked area with better colors
    if reg_col in ['reg_Q1tot', 'reg_Q2tot']:
        x_reg = None
        y_regions = []
        labels_regions = []
        colors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd', '#8c564b']
        
        for idx, r in enumerate(regions):
            df = region_dfs[r]
            if x_reg is None:
                x_reg = df['t'].to_numpy()
            y_reg = df[reg_col].to_numpy()
            y_regions.append(y_reg)
            labels_regions.append(f'Region {r}')

        if x_reg is not None and y_regions:
            plt.stackplot(x_reg, *y_regions, labels=labels_regions, 
                         colors=colors[:len(regions)], alpha=0.7)

        # National series as line (on top)
        y_nat = ymc[ymc_col].to_numpy()
        n = min(len(x_nat), len(y_nat))
        plt.plot(x_nat[:n], y_nat[:n], label='National (Total)', 
                color='black', linewidth=2.5, zorder=10)

        title = f'{var_label}: Regional Breakdown vs National Total'
        
    # For mean productivity, use line plot; others stacked area
    elif reg_col == 'reg_Am':
        x_reg = None
        for r in regions:
            df = region_dfs[r]
            if x_reg is None:
                x_reg = df['t'].to_numpy()
            y_reg = df[reg_col].to_numpy()
            plt.plot(x_reg, y_reg, label=f'Region {r}', linewidth=1.5, alpha=0.8)
        
        # National series as line (on top)
        y_nat = ymc[ymc_col].to_numpy()
        n = min(len(x_nat), len(y_nat))
        plt.plot(x_nat[:n], y_nat[:n], label='National', color='black', linewidth=2.5, zorder=10)
        
        title = var_label
        
    else:
        x_reg = None
        y_regions = []
        labels_regions = []
        for r in regions:
            df = region_dfs[r]
            if x_reg is None:
                x_reg = df['t'].to_numpy()
            y_reg = df[reg_col].to_numpy()
            y_regions.append(y_reg)
            labels_regions.append(f'Region {r}')

        if x_reg is not None and y_regions:
            plt.stackplot(x_reg, *y_regions, labels=labels_regions, alpha=0.7)

        # National series as line (on top)
        y_nat = ymc[ymc_col].to_numpy()
        n = min(len(x_nat), len(y_nat))
        plt.plot(x_nat[:n], y_nat[:n], label='National', color='black', linewidth=2.5, zorder=10)

        title = var_label

    if show_plot_title:
        plt.title(title if show_variable_name else reg_col, fontsize=13, fontweight='bold')
    plt.xlabel('Time (t)', fontsize=11)
    plt.ylabel(var_label if show_variable_name else reg_col, fontsize=11)
    plt.legend(fontsize=10, loc='upper left')
    plt.grid(True, alpha=0.3)
    safe = re.sub(r'[^A-Za-z0-9_]+', '_', var_label).strip('_')
    out_png = PLOTS_DIR / f"{reg_col}_{safe}.png"
    plt.tight_layout()
    plt.savefig(out_png, dpi=150)
    plt.close()

num_plots = len(YMC_BY_REGION_COL)
print(f'Generated {num_plots} plots in {PLOTS_DIR}')
