#!/usr/bin/env python3
import pandas as pd
import pathlib
import re
import matplotlib.pyplot as plt
from datetime import datetime

OUT_DIR = pathlib.Path('output')
PLOTS_DIR = OUT_DIR / 'plots' / 'reg_vs_nat'
PLOTS_DIR.mkdir(parents=True, exist_ok=True)

# Flag to show variable name in legends
show_variable_name = True


def parse_metadata_file(metadata_path):
    """Parse metadata file to extract column definitions.
    
    Returns:
        dict: {col_num: {'variable': str, 'description': str}}
    """
    columns = {}
    with open(metadata_path, 'r') as f:
        for line in f:
            # Look for lines like: "Col  1: t                      Time period"
            match = re.match(r'Col\s+(\d+):\s+(\S+)\s+(.*)', line)
            if match:
                col_num = int(match.group(1))
                variable = match.group(2).strip()
                description = match.group(3).strip()
                columns[col_num] = {
                    'variable': variable,
                    'description': description
                }
    return columns


# Regional column names - will be loaded from metadata
REG_TITLES = {}
YMC_COLUMNS = {}

# Find latest ymc file by modification time
ymc_files = sorted(OUT_DIR.glob('ymc_*.txt'), key=lambda p: p.stat().st_mtime, reverse=True)
# Exclude metadata files
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

# Load metadata files
ymc_metadata_path = OUT_DIR / f'ymc_metadata_{run_name}_{seed}.txt'
regional_metadata_path = OUT_DIR / f'regional_metadata_{run_name}_{seed}.txt'

if not ymc_metadata_path.exists():
    raise SystemExit(f'YMC metadata file not found: {ymc_metadata_path}\nRun the model to generate metadata.')
if not regional_metadata_path.exists():
    raise SystemExit(f'Regional metadata file not found: {regional_metadata_path}\nRun the model to generate metadata.')

print(f'Loading YMC metadata from: {ymc_metadata_path.name}')
YMC_COLUMNS = parse_metadata_file(ymc_metadata_path)

print(f'Loading regional metadata from: {regional_metadata_path.name}')
REG_COLUMNS = parse_metadata_file(regional_metadata_path)

# Build REG_TITLES from metadata (use description for better readability)
REG_TITLES = {col: info['description'] for col, info in REG_COLUMNS.items() if col > 1}

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

# Read ymc (fixed width 60) and regional (fixed width 60)
ymc = pd.read_fwf(ymc_path, header=None)
region_dfs = {r: pd.read_fwf(path, header=None) for r, path in reg_files.items()}

# Build column mapping from regional -> ymc based on variable names
# Map regional columns to corresponding YMC columns
YMC_BY_REGION_COL = {}

# Mapping rules based on variable relationships
regional_to_ymc_mapping = {
    'reg_GDP_r': 'GDP_r(1)',
    'reg_Consumption_r': 'Consumption_r',
    'reg_Investment_r': 'Investment_r',
    'reg_Am': 'Am(1)',
    'reg_Loans_2': 'Loans_2',
    'reg_Inventories': 'Inventories',
    'reg_N': 'N',
    'reg_GDP_n': 'GDP_n',
    'reg_D_en_TOT': 'D_en_TOT(1)',
    'reg_Emiss_TOT': 'Emiss_TOT(1)',
    'reg_Cum_emission': 'Cum_emissions',
    'reg_Q1': 'Q1tot',
    'reg_Q2': 'Q2tot',
    'reg_N1': 'N1r',
    'reg_N2': 'N2r',
    'reg_LS': 'LS',
    'reg_Qge': 'Q_ge',  # Fixed: was reg_Q_ge, should be reg_Qge to match metadata
    'reg_Qde': 'Q_de',  # Fixed: was reg_Q_de, should be reg_Qde to match metadata
    'reg_Emiss1_TOT': 'Emiss1_TOT',
    'reg_Emiss2_TOT': 'Emiss2_TOT',
    'reg_Emiss_en': 'Emiss_en',
}

# Build the mapping automatically
for reg_col, reg_info in REG_COLUMNS.items():
    if reg_col == 1:  # Skip time column
        continue
    
    reg_var = reg_info['variable']
    
    # Try to find matching YMC column
    ymc_var_to_find = regional_to_ymc_mapping.get(reg_var)
    
    if ymc_var_to_find:
        # Find YMC column with this variable name
        for ymc_col, ymc_info in YMC_COLUMNS.items():
            if ymc_info['variable'] == ymc_var_to_find:
                YMC_BY_REGION_COL[reg_col] = ymc_col
                break
    
    # Handle special cases
    if reg_var == '(1-reg_U)/NR':  # Employment rate
        for ymc_col, ymc_info in YMC_COLUMNS.items():
            if ymc_info['variable'] == '1-U(1)':
                YMC_BY_REGION_COL[reg_col] = ymc_col
                break

print(f'\nPlotting {len(YMC_BY_REGION_COL)} variables from run: {run_name}, seed: {seed}')
print(f'Found {len(regions)} regions')

# Time axis from ymc and regional; align by length
x_nat = ymc.iloc[:, 0].to_numpy()

for reg_col, ymc_col in YMC_BY_REGION_COL.items():
    plt.figure(figsize=(9, 5))

    # Get variable name for title
    var_name = REG_TITLES.get(reg_col, f'Column {reg_col}')

    # For Mean productivity (reg_col 6), use line plot; for others, use stacked area
    if reg_col == 6:  # Mean productivity
        # Prepare regional data for line plot
        x_reg = None
        for r in regions:
            df = region_dfs[r]
            if x_reg is None:
                x_reg = df.iloc[:, 0].to_numpy()
            y_reg = df.iloc[:, reg_col - 1].to_numpy()
            plt.plot(x_reg, y_reg, label=f'Region {r}', linewidth=1.5, alpha=0.8)
    else:
        # Prepare regional data for stacked area plot
        x_reg = None
        y_regions = []
        labels_regions = []
        for r in regions:
            df = region_dfs[r]
            if x_reg is None:
                x_reg = df.iloc[:, 0].to_numpy()
            y_reg = df.iloc[:, reg_col - 1].to_numpy()
            y_regions.append(y_reg)
            labels_regions.append(f'Region {r}')

        # Stacked area plot for regions
        if x_reg is not None and y_regions:
            plt.stackplot(x_reg, *y_regions, labels=labels_regions, alpha=0.7)

    # National series as line (on top)
    y_nat = ymc.iloc[:, ymc_col - 1].to_numpy()
    n = min(len(x_nat), len(y_nat))
    plt.plot(x_nat[:n], y_nat[:n], label='National', color='black', linewidth=2.5, zorder=10)

    plt.title(f'{var_name}' if show_variable_name else f'Column {reg_col}')
    plt.xlabel('t')
    plt.ylabel(var_name if show_variable_name else f'Column {reg_col}')
    plt.legend()
    plt.grid(True, alpha=0.3)
    safe = re.sub(r'[^A-Za-z0-9_]+', '_', REG_TITLES[reg_col]).strip('_')
    out_png = PLOTS_DIR / f'{reg_col:02d}_{safe}.png'
    plt.tight_layout()
    plt.savefig(out_png, dpi=150)
    plt.close()

num_plots = len(YMC_BY_REGION_COL)
print(f'Generated {num_plots} plots in {PLOTS_DIR}')
