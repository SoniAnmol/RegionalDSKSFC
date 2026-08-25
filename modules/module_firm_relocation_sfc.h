#ifndef MODULE_FIRM_RELOCATION_SFC_H
#define MODULE_FIRM_RELOCATION_SFC_H

#include <vector>
#include "../newmat10/include.h"
#include "../newmat10/newmat.h"

// Regional climate-hazard distributions
extern Matrix X_a_reg;
extern Matrix X_b_reg;

// Regional model structure
extern int NR;
extern int N1;
extern int N2;

// National market aggregates
extern double Cons;
extern double Investment_n;

// Regional market-opportunity inputs
extern std::vector<double> reg_cons_share;
extern std::vector<double> reg_Investment_n;
extern std::vector<double> reg_N1;
extern std::vector<double> reg_N2;

extern std::vector<double> market_potential_K_reloc;
extern std::vector<double> market_potential_C_reloc;
extern std::vector<double> market_signal_K_reloc;
extern std::vector<double> market_signal_C_reloc;

// Preliminary regional economic attractiveness
extern std::vector<double> attractiveness_econ_K_reloc;
extern std::vector<double> attractiveness_econ_C_reloc;

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
extern double beta_market_K_reloc;
extern double beta_market_C_reloc;

// Climate-shock activation flags
extern int flag_exogenousshocks;
extern int flag_prodshocks1;
extern int flag_prodshocks2;
extern int flag_capshocks;
extern int flag_outputshocks;
extern int flag_inventshocks;
extern int flag_RDshocks;

// Raw firm-relocation climate hazard
extern std::vector<double> hazard_K_reloc;
extern std::vector<double> hazard_C_reloc;

// Firm regional relocation
void FIRM_RELOCATION_COMPUTATION(void);

#endif