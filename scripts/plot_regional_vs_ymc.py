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

# Regional column names (from the regional output files)
REG_TITLES = {
    2: 'reg_GDP_r',
    3: 'reg_Consumption_r',
    4: 'reg_Investment_r',
    5: '1 - reg_U(1)',
    6: 'reg_Am(1)',
    7: 'reg_Loans_2',
    8: 'reg_Inventories',
    9: 'reg_N.Row(1).sum()',
    10: 'reg_GDP_n',
    11: 'reg_Qge / D_en',
    12: 'reg_D_en_TOT(1)',
    13: 'reg_Emiss_TOT(1)',
    14: 'reg_Cum_emissions',
    15: 'reg_Q1',
    16: 'reg_Q2',
}

# Find latest ymc file by modification time
ymc_files = sorted(OUT_DIR.glob('ymc*.txt'), key=lambda p: p.stat().st_mtime, reverse=True)
if not ymc_files:
    raise SystemExit('No ymc*.txt found in output/. Run the model first.')
ymc_path = ymc_files[0]

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

# Column mapping from regional -> ymc according to provided spec
# Regional columns (1-based): 1=t, 2..14 the 13 variables
REG_TITLES = {
    2: 'Real GDP',
    3: 'Total real consumption',
    4: 'Total real investment',
    5: 'Employment rate',
    6: 'Mean productivity',
    7: 'Loans of C-firms',
    8: "Nominal value of C-firms' inventories",
    9: 'Inventories (real)',
    10: 'Nominal GDP',
    11: 'Quantity of Green Energy Produced / Total Energy Demand',
    12: 'Total energy demand',
    13: 'Total emissions',
    14: 'Cumulative emissions',
    15: 'K-firm production (Q1)',
    16: 'C-firm production (Q2)',
}

YMC_BY_REGION_COL = {
    2: 2,   # GDP_r
    3: 3,   # Consumption_r
    4: 4,   # Investment_r
    5: 5,   # 1 - U(1)
    6: 8,   # Am(1)
    7: 15,  # Loans_2
    8: 20,  # Inventories (nominal)
    9: 21,  # N (real inventories)
    10: 23,  # GDP_n
    11: 24,  # Q_ge / D_en_TOT(1)
    12: 25,  # D_en_TOT(1)
    13: 26,  # Emiss_TOT(1)
    14: 27,  # Cum_emissions
    15: 29,  # Q1tot
    16: 30,  # Q2tot
}

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

print(f'Generated 13 plots in {PLOTS_DIR}')
