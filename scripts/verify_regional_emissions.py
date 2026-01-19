#!/usr/bin/env python3
import numpy as np

# Read regional outputs
regions = [1, 2, 3]
for t_check in [10, 50, 100]:
    print(f"\n=== Time period t={t_check+1} ===")
    
    reg_Emiss1_sum = 0
    reg_Emiss2_sum = 0
    reg_Emiss_en_sum = 0
    
    for rr in regions:
        try:
            data = np.loadtxt(f'./output/resultsexp_reg{rr}_natvsreg_1.txt', skiprows=0)
            if len(data) > t_check:
                row = data[t_check]
                reg_Emiss1 = row[21]  # Column 22 (0-indexed 21)
                reg_Emiss2 = row[22]  # Column 23 (0-indexed 22)
                reg_Emiss_en = row[23]  # Column 24 (0-indexed 23)
                
                reg_Emiss1_sum += reg_Emiss1
                reg_Emiss2_sum += reg_Emiss2
                reg_Emiss_en_sum += reg_Emiss_en
                
                print(f'Region {rr}: Emiss1={reg_Emiss1:.4f}, Emiss2={reg_Emiss2:.4f}, Emiss_en={reg_Emiss_en:.4f}')
        except Exception as e:
            print(f"Error reading region {rr}: {e}")
    
    # Read national ymc output
    try:
        ymc_data = np.loadtxt('./output/ymc_natvsreg_1.txt', skiprows=0)
        if len(ymc_data) > t_check:
            ymc_row = ymc_data[t_check]
            Emiss1_TOT = ymc_row[35]  # Column 36
            Emiss2_TOT = ymc_row[36]  # Column 37
            Emiss_en = ymc_row[37]    # Column 38
            
            print(f'\nNational: Emiss1_TOT={Emiss1_TOT:.4f}, Emiss2_TOT={Emiss2_TOT:.4f}, Emiss_en={Emiss_en:.4f}')
            print(f'Regional sums: Emiss1={reg_Emiss1_sum:.4f}, Emiss2={reg_Emiss2_sum:.4f}, Emiss_en={reg_Emiss_en_sum:.4f}')
            print(f'Differences: Emiss1={abs(Emiss1_TOT-reg_Emiss1_sum):.8f}, Emiss2={abs(Emiss2_TOT-reg_Emiss2_sum):.8f}, Emiss_en={abs(Emiss_en-reg_Emiss_en_sum):.8f}')
            
            # Check if they match within tolerance
            emiss_match = (abs(Emiss1_TOT-reg_Emiss1_sum) < 0.001 and 
                          abs(Emiss2_TOT-reg_Emiss2_sum) < 0.001 and 
                          abs(Emiss_en-reg_Emiss_en_sum) < 0.001)
            
            if emiss_match:
                print("✓ Regional emissions correctly sum to national aggregates!")
            else:
                print("✗ Mismatch detected in emissions")
    except Exception as e:
        print(f'Error reading ymc: {e}')
