#ifndef MODULE_MACRO_H
#define MODULE_MACRO_H

#include <iostream>
#include <algorithm>
#include <iomanip>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <fstream>
#include <cmath>
#include <fenv.h>

// #include <string>
#include <string.h>
#include <sstream>
#include <vector>

// Include Newmat Libraries and random number generators
#include "../newmat10/include.h"
#include "../newmat10/newmat.h"
#include "../newmat10/newmatio.h"
#include "../auxiliary/ran1.h"

// Include functions from other modules
#include "../dsk_sfc_functions.h"

// Forward declare regional global variables used in REGIONAL_UPDATE
extern std::vector<double> reg_S1, reg_S2, reg_K, reg_Investment, reg_EI, reg_SI, reg_Wages, reg_Dividends_1, reg_Dividends_2;
extern std::vector<double> reg_Ld1, reg_Ld2, reg_Emiss1, reg_Emiss2;
extern std::vector<double> reg_Pi1, reg_Pi2, reg_Pitot1, reg_Pitot2, reg_Dividends_1, reg_Dividends_2, reg_NW_1, reg_NW2;
extern std::vector<double> reg_Deposits1, reg_Deposits2, reg_CapitalStock1, reg_CapitalStock2, reg_CapitalStock, reg_NW_h;
extern std::vector<double> reg_CreditDemand_all, reg_CreditSupply_all;
extern std::vector<double> reg_Am1, reg_Am2, reg_Am_a, reg_Am_en, reg_GDP_r, reg_Investment_r, reg_ReplacementInvestment_r, reg_Investment_n, reg_EnergyPayments, reg_LS, reg_Cum_emissions, reg_Consumption;
extern std::vector<double> reg_Emiss1_TOT, reg_Emiss2_TOT, reg_Emiss_en;
extern double Emiss1_TOT, Emiss2_TOT, Emiss_en, Cum_emissions, LS, LD;
// Forward declare firm-level matrices
extern Matrix NW_1, NW_2, Deposits_1, Deposits_2, CapitalStock, Loans_2, Inventories, EI, N, S2;
extern RowVector S1, Ld1, Ld2, K, I, SI, A1p, A2, nclient, Pi1, Pi2, Emiss2, CreditDemand;
extern RowVector NW_h;

// -- Functions -- //
void LABOR(void);           // Allocates labour supply; scales down production of firms if labour supply is insufficient
void MACRO(void);           // Calculates macroeconomic aggregates
void REGIONAL_UPDATE(void); // Recalculates regional aggregates post-ENTRYEXIT to match national timing
void WAGE(void);            // Determines change in wage rate
void GOV_BUDGET(void);      // Determines unemployment benefits & government deficit; implements bond market
void TAYLOR(void);          // Determines change in monetary policy rate

extern int flag_desc;

//-- Pars --//
extern int N1;
extern int N2;
extern double N1r;
extern double N2r;
extern double NB;
extern double dim_mach;
extern double wu;
extern double bonds_share;
extern double aliqb;
extern double taylor1;
extern double taylor2;
extern double bankmarkdown;
extern double centralbankmarkdown;
extern double bondsmarkdown;
extern double ustar;
extern double d_cpi_target;
extern double mdw;
extern double psi1;
extern double psi2;
extern double psi3;
extern double w_min;
extern double u_low;
extern int fulloutput;
extern double kappa;
extern double taylor;
extern double varphi;
extern double a;
extern double g_ls;
extern long *p_seed;

// -- Vars -- //
extern int i;
extern int j;
extern int t;
extern double LD1tot;
extern double LD2tot;
extern double LD1rdtot;
extern double LDentot;
extern RowVector Ld1;
extern RowVector Ld2;
extern double LSe;
extern double LS;
extern double LD;
extern double LD2;
extern RowVector Q2;
extern RowVector Q1;
extern RowVector EnergyPayments_1;
extern RowVector EnergyPayments_2;
extern double Qpast;
extern Matrix Match;
extern RowVector I;
extern Matrix EI;
extern RowVector SI;
extern double G;
extern RowVector w;
extern RowVector GB;
extern RowVector GB_cb;
extern double Deficit;
extern double Taxes;
extern double Bailout;
extern double r_bonds;
extern double r_cbreserves;
extern double PSBR;
extern double NewBonds;
extern Matrix GB_b;
extern RowVector Bond_share;
extern RowVector Inflows;
extern double InterestBonds;
extern double InterestBonds_cb;
extern RowVector InterestBonds_b;
extern double BondRepayments_cb;
extern RowVector BondRepayments_b;
extern RowVector bonds_dem;
extern double bonds_dem_tot;
extern RowVector BankProfits;
extern RowVector BankProfits_temp;
extern RowVector Deposits_h;
extern RowVector DepositShare_h;
extern Matrix Deposits;
extern Matrix Deposits_hb;
extern double r;
extern double r_depo;
extern RowVector r_deb;
extern double r_base;
extern RowVector bankmarkup;
extern RowVector U;
extern double d_cpi;
extern RowVector Outflows;
extern RowVector bonds_purchased;
extern RowVector Am;
extern double Am_a;
extern double Am1;
extern double Am2;
extern double ExpansionInvestment_r;
extern double ExpansionInvestment_n;
extern double ReplacementInvestment_r;
extern double ReplacementInvestment_n;
extern double Investment_r;
extern double Investment_n;
extern double Consumption_r;
extern Real CreditDemand_all;
extern Real CreditSupply_all;
extern double Q2tot;
extern double Q1tot;
extern double Q2dtot;
extern double D2tot;
extern double A_mi;
extern double A1_mi;
extern double A2_en_mi;
extern double A2_ef_mi;
extern double A1_en_mi;
extern double A1_ef_mi;
extern double A_sd;
extern double H1;
extern double H2;
extern RowVector Qd;
extern Matrix D2;
extern double GDP_rg;
extern double GDP_ng;
extern RowVector EI_n;
extern RowVector SI_n;
extern RowVector A2e;
extern Matrix f2;
extern Matrix f1;
extern RowVector A2e_en;
extern RowVector A2e_ef;
extern double D2_en_TOT;
extern RowVector Emiss2;
extern double Emiss2_TOT;
extern double D1_en_TOT;
extern RowVector Emiss1;
extern double Emiss1_TOT;
extern RowVector A2;
extern RowVector A2_en;
extern RowVector A2_ef;
extern RowVector A1p;
extern RowVector A1p_en;
extern RowVector A1p_ef;
extern RowVector BaselBankCredit;
extern RowVector CreditDemand;
extern double HB;
extern Matrix fB;
extern RowVector GDP_r;
extern RowVector GDP_n;
extern double Consumption;
extern double Pitot1;
extern double Pitot2;
extern double d_U;
extern RowVector cpi;
extern double d_Am;
extern double dw;
extern RowVector ProfitCB;
extern double TransferCB;
extern double Benefits;
extern double EntryCosts;
extern double Taxes_CO2;
extern double Subsidy_Exp;
extern RowVector p1;
extern RowVector p2;
extern double d_cpi_target_a;
extern double inflation_a;
extern RowVector D2_en;
extern RowVector D1_en;
extern RowVector Am_en;
extern Matrix Loans_b;
extern int ranj;
extern RowVector Dividends_1;
extern RowVector Dividends_2;
extern RowVector Wages_1;
extern RowVector Wages_2;
extern RowVector shocks_labprod1;
extern double reduction;
extern double Ipast;
extern RowVector K;
extern RowVector nclient;
extern RowVector S1;
extern Matrix S2;
extern double r_a;
extern double Taxes_e_shock;
extern double Taxes_f_shock;
extern double Transfer_shock;
extern Matrix Loans_2;
extern Matrix Inventories;
extern Matrix N;

// Regional variables
extern int NR;
extern std::vector<int> region_firm_assignment_K;
extern std::vector<int> region_firm_assignment_C;
extern std::vector<double> reg_N1;
extern std::vector<double> reg_N2;
extern std::vector<double> reg_Q1;
extern std::vector<double> reg_Q2;
extern std::vector<double> reg_U;
extern std::vector<double> reg_Q1tot;
extern std::vector<double> reg_Q2tot;
extern std::vector<double> reg_Am;
extern std::vector<double> reg_D1_en;
extern std::vector<double> reg_D2_en;
extern std::vector<double> reg_D_en_TOT;
extern std::vector<double> reg_GDP_n;
extern std::vector<double> reg_Loans_2;
extern std::vector<double> reg_Inventories;
extern std::vector<double> reg_N;
extern std::vector<double> reg_Am_a;

// Cached national aggregates
extern double cached_Loans_2_sum;
extern double cached_Inventories_sum;
extern double cached_N_sum;
extern double cached_Consumption_r;
extern double cached_Q1tot;
extern double cached_Q2tot;
extern double cached_Emiss1_TOT;
extern double cached_Emiss2_TOT;
extern double cached_Emiss_TOT;

#endif