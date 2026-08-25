#ifndef MODULE_FIRM_RELOCATION_SFC_H
#define MODULE_FIRM_RELOCATION_SFC_H

#include <vector>

// Regional model structure
extern int NR;

// Regional realised sector aggregates
extern std::vector<double> reg_S1;
extern std::vector<double> reg_S2;
extern std::vector<double> reg_Pitot1;
extern std::vector<double> reg_Pitot2;

// Firm-relocation profitability state
extern std::vector<double> rho_K_reloc;
extern std::vector<double> rho_C_reloc;
extern std::vector<double> rho_K_reloc_exp;
extern std::vector<double> rho_C_reloc_exp;

extern bool profit_expectations_initialized_reloc;

// Parameters
extern double lambda_profit_reloc;

// Firm regional relocation
void FIRM_RELOCATION_COMPUTATION(void);

#endif