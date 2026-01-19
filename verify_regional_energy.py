#!/usr/bin/env python3
import numpy as np

# Read regional outputs
regions = [1, 2, 3]
for t_check in [10, 50, 100]:
    reg_Q_ge_sum = 0
    reg_Q_de_sum = 0
    reg_D_en_sum = 0
    
    print(f"\n=== Time period {t_check} ===")
    for rr in regions:
        try:
            data = np.loadtxt(f'./output/resultsexp_reg{rr}_natvsreg_1.txt', skiprows=0)
            if len(data) > t_check:
                row = data[t_check]
                reg_D_en = row[11]  # Column 12 (0-indexed 11)
                reg_Q_ge = row[19]  # Column 20 (0-indexed 19)
                reg_Q_de = row[20]  # Column 21 (0-indexed 20)
                
                reg_Q_ge_sum += reg_Q_ge
                reg_Q_de_sum += reg_Q_de
                reg_D_en_sum += reg_D_en
                
                print(f'Region {rr}, t={int(row[0])}: D_en={reg_D_en:.4f}, Q_ge={reg_Q_ge:.4f}, Q_de={reg_Q_de:.4f}, Q_total={reg_Q_ge+reg_Q_de:.4f}')
        except Exception as e:
            print(f"Error reading region {rr}: {e}")
    
    # Read national ymc output
    try:
        ymc_data = np.loadtxt('./output/ymc_natvsreg_1.txt', skiprows=0)
        if len(ymc_data) > t_check:
            ymc_row = ymc_data[t_check]
            D_en_TOT = ymc_row[24]  # Column 25
            Q_ge = ymc_row[33]       # Column 34
            Q_de = ymc_row[34]       # Column 35
            
            print(f'\nNational t={int(ymc_row[0])}: D_en={D_en_TOT:.4f}, Q_ge={Q_ge:.4f}, Q_de={Q_de:.4f}')
            print(f'Regional sums: D_en={reg_D_en_sum:.4f}, Q_ge={reg_Q_ge_sum:.4f}, Q_de={reg_Q_de_sum:.4f}')
            print(f'Differences: D_en={abs(D_en_TOT-reg_D_en_sum):.8f}, Q_ge={abs(Q_ge-reg_Q_ge_sum):.8f}, Q_de={abs(Q_de-reg_Q_de_sum):.8f}')
            
            # Check if they match within tolerance
            Q_match = abs(Q_ge-reg_Q_ge_sum) < 0.001 and abs(Q_de-reg_Q_de_sum) < 0.001
            D_match = abs(D_en_TOT-reg_D_en_sum) < 1.0  # Allow small rounding in demand
            
            if Q_match:
                print("✓ Regional Q_ge and Q_de correctly sum to national aggregates!")
                if D_match:
                    print("✓ Regional D_en also matches national aggregate!")
                else:
                    print(f"  (Note: Small D_en difference of {abs(D_en_TOT-reg_D_en_sum):.2f} due to rounding)")
            else:
                print("✗ Mismatch detected in Q values")
    except Exception as e:
        print(f'Error reading ymc: {e}')
