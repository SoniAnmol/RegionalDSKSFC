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

# National level columns from resultsexp_<run_name>_<seed>.txt
NATIONAL_COLS = [
    't',
    'GDP_r',
    'Consumption_r',
    'Investment_r',
    'EmploymentRate',
    'cpi_rel',
    'Emiss_TOT',
    'D_en_TOT',
    'LS',
    'K_tot',
    'A1p_en_dead/A1p_en_survive',
    'A2_en_dead/A2_en_survive',
    'Am_en',
    'Am_a',
    'exit_marketshare2',
    'exit_payments2',
    'exit_equity2',
    'exiting_1',
    'Bailout',
    'baddebt_b/GDP',
    'counter_bankfailure',
    'CapitalStock',
    'NW_cb/GDP',
    'NW_h',
    'NW_2',
    'NW_b/GDP',
    'BankProfits',
    'Loans_2',
    'CreditDemand/Supply',
    'NW_gov/GDP',
    'NW_e/GDP',
    'NW_1',
    'bankfailure_dummy',
    'cpi',
    'exp_quota',
    'real_wage',
    'Am2',
    'Am1',
    'GDP_n',
    'Investment_n.dim_mach.a',
    'Consumption',
    't_CO2_en',
    'ReplacementInvestment_r',
    'Emiss1_TOT',
    'Emiss2_TOT',
    'Emiss_en',
    'Tmixed',
    'EnergyPayments',
    'FuelCost/GDP',
    'GreenCapacityShare',
    'Deposits_e/GDP',
    'CapitalStock_e/GDP',
    'Pitot1',
    'Pitot2',
    'ProfitEnergy',
    'real_wages',
    'Dividends_1',
    'Dividends_2',
    'Consumption_nominal',
    'c_en',
    'Deposits_2/GDP',
    'Deposits_1/GDP',
    'Deposits_firms/GDP',
    'exit_total',
    'Pitot_total',
    'Dividends_firms_real',
    'Dividends_e',
    'Dividends_b',
    'NW_firms',
    'GovDebt/GDP',
    'inflation_growth',
    'GDP_r_growth',
    'NW_f/GDP',
    'Labour_income_share',
]

# Regional columns from resultsexp_reg<region_number>_<run_name>_<seed>.txt
REGIONAL_COLS = [
    't',
    'reg_GDP_r',
    'reg_Consumption_r',
    'reg_Investment_r',
    'reg_EmploymentRate',
    'reg_Emiss_TOT',
    'reg_D_en_TOT',
    'reg_LS',
    'reg_K',
    'reg_A1p_en_dead/reg_A1p_en_survive',
    'reg_A2_en_dead/reg_A2_en_survive',
    'reg_Am_en',
    'reg_Am_a',
    'reg_exit_marketshare2',
    'reg_exit_payments2',
    'reg_exit_equity2',
    'reg_exiting_1',
    'reg_CapitalStock',
    'reg_NW_h',
    'reg_NW_2',
    'reg_Loans_2',
    'reg_CreditDemand/Supply',
    'reg_NW_1',
    'reg_Am2',
    'reg_Am1',
    'reg_GDP_n',
    'reg_Investment_n.dim_mach.a',
    'reg_Consumption',
    'reg_ReplacementInvestment_r',
    'reg_Emiss1_TOT',
    'reg_Emiss2_TOT',
    'reg_Emiss_en',
    'reg_EnergyPayments',
    'reg_GreenCapacityShare',
    'reg_Pitot1',
    'reg_Pitot2',
    'reg_real_wages',
    'reg_Dividends_1',
    'reg_Dividends_2',
    'reg_exit_total',
    'reg_Pitot_total',
    'reg_Dividends_firms_real',
    'reg_Dividends_e',
    'reg_Dividends_b',
    'reg_NW_firms',
    'reg_GDP_r_growth',
]

# Mapping from regional column to national column
REG_TO_NAT = {
    'reg_GDP_r': 'GDP_r',
    'reg_Consumption_r': 'Consumption_r',
    'reg_Investment_r': 'Investment_r',
    'reg_EmploymentRate': 'EmploymentRate',
    'reg_Emiss_TOT': 'Emiss_TOT',
    'reg_D_en_TOT': 'D_en_TOT',
    'reg_LS': 'LS',
    'reg_K': 'K_tot',
    'reg_A1p_en_dead/reg_A1p_en_survive': 'A1p_en_dead/A1p_en_survive',
    'reg_A2_en_dead/reg_A2_en_survive': 'A2_en_dead/A2_en_survive',
    'reg_Am_en': 'Am_en',
    'reg_Am_a': 'Am_a',
    'reg_exit_marketshare2': 'exit_marketshare2',
    'reg_exit_payments2': 'exit_payments2',
    'reg_exit_equity2': 'exit_equity2',
    'reg_exiting_1': 'exiting_1',
    'reg_CapitalStock': 'CapitalStock',
    'reg_NW_h': 'NW_h',
    'reg_NW_2': 'NW_2',
    'reg_Loans_2': 'Loans_2',
    'reg_CreditDemand/Supply': 'CreditDemand/Supply',
    'reg_NW_1': 'NW_1',
    'reg_Am2': 'Am2',
    'reg_Am1': 'Am1',
    'reg_GDP_n': 'GDP_n',
    'reg_Investment_n.dim_mach.a': 'Investment_n.dim_mach.a',
    'reg_Consumption': 'Consumption',
    'reg_ReplacementInvestment_r': 'ReplacementInvestment_r',
    'reg_Emiss1_TOT': 'Emiss1_TOT',
    'reg_Emiss2_TOT': 'Emiss2_TOT',
    'reg_Emiss_en': 'Emiss_en',
    'reg_EnergyPayments': 'EnergyPayments',
    'reg_GreenCapacityShare': 'GreenCapacityShare',
    'reg_Pitot1': 'Pitot1',
    'reg_Pitot2': 'Pitot2',
    'reg_real_wages': 'real_wages',
    'reg_Dividends_1': 'Dividends_1',
    'reg_Dividends_2': 'Dividends_2',
    'reg_exit_total': 'exit_total',
    'reg_Pitot_total': 'Pitot_total',
    'reg_Dividends_firms_real': 'Dividends_firms_real',
    'reg_Dividends_e': 'Dividends_e',
    'reg_Dividends_b': 'Dividends_b',
    'reg_NW_firms': 'NW_firms',
    'reg_GDP_r_growth': 'GDP_r_growth',
}

# Labels for plotting (clean names)
REGIONAL_LABELS = {
    't': 'Time',
    'reg_GDP_r': 'Real GDP',
    'reg_Consumption_r': 'Real Consumption',
    'reg_Investment_r': 'Real Investment',
    'reg_EmploymentRate': 'Employment Rate',
    'reg_Emiss_TOT': 'Total Emissions',
    'reg_D_en_TOT': 'Energy Demand',
    'reg_LS': 'Labour Supply',
    'reg_K': 'Capital Stock',
    'reg_A1p_en_dead/reg_A1p_en_survive': 'A1p Energy Dead/Survive Ratio',
    'reg_A2_en_dead/reg_A2_en_survive': 'A2 Energy Dead/Survive Ratio',
    'reg_Am_en': 'Energy Productivity',
    'reg_Am_a': 'Agriculture Productivity',
    'reg_exit_marketshare2': 'Exit Market Share (Type 2)',
    'reg_exit_payments2': 'Exit Payments (Type 2)',
    'reg_exit_equity2': 'Exit Equity (Type 2)',
    'reg_exiting_1': 'Exiting Firms (Type 1)',
    'reg_CapitalStock': 'Capital Stock',
    'reg_NW_h': 'Household Net Worth',
    'reg_NW_2': 'Firms Type 2 Net Worth',
    'reg_Loans_2': 'Loans Type 2',
    'reg_CreditDemand/Supply': 'Credit Demand to Supply Ratio',
    'reg_NW_1': 'Firms Type 1 Net Worth',
    'reg_Am2': 'Productivity Type 2',
    'reg_Am1': 'Productivity Type 1',
    'reg_GDP_n': 'Nominal GDP',
    'reg_Investment_n.dim_mach.a': 'Nominal Investment (Machine)',
    'reg_Consumption': 'Consumption',
    'reg_ReplacementInvestment_r': 'Real Replacement Investment',
    'reg_Emiss1_TOT': 'Emissions Type 1',
    'reg_Emiss2_TOT': 'Emissions Type 2',
    'reg_Emiss_en': 'Energy Emissions',
    'reg_EnergyPayments': 'Energy Payments',
    'reg_GreenCapacityShare': 'Green Capacity Share',
    'reg_Pitot1': 'Profit Type 1',
    'reg_Pitot2': 'Profit Type 2',
    'reg_real_wages': 'Real Wages',
    'reg_Dividends_1': 'Dividends Type 1',
    'reg_Dividends_2': 'Dividends Type 2',
    'reg_exit_total': 'Total Exits',
    'reg_Pitot_total': 'Total Profits',
    'reg_Dividends_firms_real': 'Real Firm Dividends',
    'reg_Dividends_e': 'Energy Dividends',
    'reg_Dividends_b': 'Bank Dividends',
    'reg_NW_firms': 'Firms Net Worth',
    'reg_GDP_r_growth': 'Real GDP Growth',
}

# Find latest national resultsexp file by modification time
national_files = sorted(OUT_DIR.glob('resultsexp_*.txt'), key=lambda p: p.stat().st_mtime, reverse=True)
# Exclude regional files (those with 'reg' in name)
national_files = [f for f in national_files if not re.match(r'resultsexp_reg\d+_', f.name)]

if not national_files:
    raise SystemExit('No resultsexp_*.txt found in output/. Run the model first.')
national_path = national_files[0]

# Extract run name and seed from national filename: resultsexp_<run>_<seed>.txt
nat_match = re.match(r'resultsexp_(.+)_(\d+)\.txt', national_path.name)
if not nat_match:
    raise SystemExit(f'Cannot parse resultsexp filename: {national_path.name}')
run_name = nat_match.group(1)
seed = nat_match.group(2)

# Create plots directory using run name
if run_name and run_name not in ['', 'output', 'results']:
    PLOTS_DIR = OUT_DIR / 'plots' / run_name
else:
    PLOTS_DIR = OUT_DIR / 'plots' / 'default'
PLOTS_DIR.mkdir(parents=True, exist_ok=True)

# Find regional files pattern resultsexp_reg{r}_<run_name>_<seed>.txt
reg_files = {}
for p in OUT_DIR.glob(f'resultsexp_reg*_{run_name}_{seed}.txt'):
    m = re.match(r'resultsexp_reg(\d+)_.*', p.name)
    if not m:
        continue
    r = int(m.group(1))
    reg_files[r] = p

if not reg_files:
    print(f'Warning: No regional result files found (resultsexp_reg*_{run_name}_{seed}.txt).')
    print('Only national-level plots will be generated.')
    regions = []
else:
    regions = sorted(reg_files.keys())

# Read national and regional data with explicit 60-char column widths
national_df = pd.read_fwf(national_path, header=None, names=NATIONAL_COLS, widths=[60] * len(NATIONAL_COLS))
region_dfs = {
    r: pd.read_fwf(path, header=None, names=REGIONAL_COLS, widths=[60] * len(REGIONAL_COLS))
    for r, path in reg_files.items()
}

print(f'\nPlotting resultsexp data from run: {run_name}, seed: {seed}')
print(f'Found {len(regions)} regions')
print(f'Saving plots to: {PLOTS_DIR}')

# Time axis from national
x_nat = national_df['t'].to_numpy()

# Variables to plot as line plots instead of stacked area plots
# These are intensive variables (rates, ratios, means) that don't add up across regions
LINE_PLOT_VARS = [
    'EmploymentRate',
    'GreenCapacityShare',
    'CreditDemand/Supply',
    'A2_en_dead/A2_en_survive',
    'A1p_en_dead/A1p_en_survive',
    'Am1',              # Mean productivity K-firms
    'Am2',              # Mean productivity C-firms
    'GDP_r_growth',     # GDP growth rate (%)
]

# Plot each mapped variable
for reg_col, nat_col in REG_TO_NAT.items():
    if nat_col not in NATIONAL_COLS:
        print(f'Warning: National column {nat_col} not found, skipping {reg_col}')
        continue
    
    plt.figure(figsize=(12, 6))
    
    var_label = REGIONAL_LABELS.get(reg_col, reg_col)
    
    # Colors for regions
    colors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd', '#8c564b', '#e377c2', '#7f7f7f', '#bcbd22', '#17becf']
    
    # Check if this variable should be plotted as lines instead of stacked area
    use_line_plot = nat_col in LINE_PLOT_VARS
    
    # Check if regions exist
    if regions:
        x_reg = None
        y_regions = []
        labels_regions = []
        
        for idx, r in enumerate(regions):
            df = region_dfs[r]
            if x_reg is None:
                x_reg = df['t'].to_numpy()
            if reg_col in df.columns:
                y_reg = df[reg_col].to_numpy()
                y_regions.append(y_reg)
                labels_regions.append(f'Region {r}')
            else:
                print(f'Warning: Column {reg_col} not found in region {r} data')
        
        # Create plot based on variable type
        if x_reg is not None and y_regions:
            if use_line_plot:
                # Line plot for specific variables
                for idx, (y_reg, label) in enumerate(zip(y_regions, labels_regions)):
                    plt.plot(x_reg, y_reg, label=label, 
                            color=colors[idx % len(colors)], linewidth=1.5, alpha=0.8)
            else:
                # Stacked area plot for other variables
                plt.stackplot(x_reg, *y_regions, labels=labels_regions, 
                             colors=colors[:len(y_regions)], alpha=0.7)
    
    # National series as line (on top)
    if nat_col in national_df.columns:
        y_nat = national_df[nat_col].to_numpy()
        n = min(len(x_nat), len(y_nat))
        plt.plot(x_nat[:n], y_nat[:n], label='National (Total)', 
                color='black', linewidth=2.5, zorder=10)
    else:
        print(f'Warning: Column {nat_col} not found in national data')
        continue
    
    if show_plot_title:
        title = f'{nat_col}: Regional Breakdown vs National Total'
        plt.title(title if show_variable_name else reg_col, fontsize=13, fontweight='bold')
    
    plt.xlabel('Time (t)', fontsize=11)
    plt.ylabel(nat_col, fontsize=11)
    plt.legend(fontsize=10, loc='upper left')
    plt.grid(True, alpha=0.3)
    
    # Safe filename - use national column name with / replaced by _over_
    safe_nat_col = nat_col.replace('/', '_over_')
    out_png = PLOTS_DIR / f"{safe_nat_col}.png"
    plt.tight_layout()
    plt.savefig(out_png, dpi=150)
    plt.close()

num_plots = len([col for col in REG_TO_NAT.keys() if REG_TO_NAT[col] in NATIONAL_COLS])
print(f'Generated {num_plots} plots in {PLOTS_DIR}')
