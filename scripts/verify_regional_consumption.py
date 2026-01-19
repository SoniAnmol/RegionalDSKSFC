#!/usr/bin/env python3
"""
Verify that regional consumption values sum to national consumption.
"""

import pandas as pd
import numpy as np

def verify_regional_consumption():
    """Check that sum of regional consumption equals national consumption."""
    
    # Read YMC (national) data - Consumption_r is column 3 (0-indexed)
    ymc_file = "output/ymc_fixedtest_1.txt"
    ymc_data = pd.read_csv(ymc_file, sep=r'\s+', header=None)
    
    # Column 3 is Consumption_r (national)
    national_consumption = ymc_data.iloc[:, 3].values
    
    # Read regional data files
    regional_files = [
        "output/resultsexp_reg1_fixedtest_1.txt",
        "output/resultsexp_reg2_fixedtest_1.txt",
        "output/resultsexp_reg3_fixedtest_1.txt"
    ]
    
    regional_consumption = []
    for reg_file in regional_files:
        reg_data = pd.read_csv(reg_file, sep=r'\s+', header=None)
        # Regional consumption is column 2 (0-indexed) - written as 3rd value (after t and GDP_r)
        regional_consumption.append(reg_data.iloc[:, 2].values)
    
    # Sum regional consumption
    regional_consumption_sum = sum(regional_consumption)
    
    # Calculate differences
    diff = national_consumption - regional_consumption_sum
    
    # Print verification results
    print("=" * 60)
    print("REGIONAL CONSUMPTION VERIFICATION")
    print("=" * 60)
    print(f"\nData points checked: {len(national_consumption)}")
    print(f"\nNational Consumption_r statistics:")
    print(f"  Mean:    {np.mean(national_consumption):.6f}")
    print(f"  Min:     {np.min(national_consumption):.6f}")
    print(f"  Max:     {np.max(national_consumption):.6f}")
    
    print(f"\nSum of Regional Consumption statistics:")
    print(f"  Mean:    {np.mean(regional_consumption_sum):.6f}")
    print(f"  Min:     {np.min(regional_consumption_sum):.6f}")
    print(f"  Max:     {np.max(regional_consumption_sum):.6f}")
    
    print(f"\nDifference (National - Sum of Regional):")
    print(f"  Mean:    {np.mean(diff):.10f}")
    print(f"  Max:     {np.max(np.abs(diff)):.10f}")
    print(f"  Std:     {np.std(diff):.10f}")
    
    # Check if differences are within acceptable tolerance
    tolerance = 1e-6
    if np.max(np.abs(diff)) < tolerance:
        print(f"\n✓ VERIFICATION PASSED: All differences < {tolerance}")
    else:
        print(f"\n✗ VERIFICATION FAILED: Some differences >= {tolerance}")
        # Show periods with largest differences
        large_diff_idx = np.where(np.abs(diff) >= tolerance)[0]
        print(f"\nPeriods with differences >= {tolerance}:")
        for idx in large_diff_idx[:10]:  # Show first 10
            print(f"  Period {idx+1}: diff = {diff[idx]:.10f}")
    
    print("\nRegional breakdown (first 5 periods):")
    print(f"{'Period':<10} {'Reg1':<12} {'Reg2':<12} {'Reg3':<12} {'Sum':<12} {'National':<12} {'Diff':<12}")
    print("-" * 90)
    for i in range(min(5, len(national_consumption))):
        print(f"{i+1:<10} {regional_consumption[0][i]:<12.6f} {regional_consumption[1][i]:<12.6f} "
              f"{regional_consumption[2][i]:<12.6f} {regional_consumption_sum[i]:<12.6f} "
              f"{national_consumption[i]:<12.6f} {diff[i]:<12.10f}")
    
    print("=" * 60)

if __name__ == "__main__":
    verify_regional_consumption()
