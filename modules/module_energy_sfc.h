#ifndef MODULE_ENERGY_H
#define MODULE_ENERGY_H

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

//#include <string>
#include <string.h>
#include <sstream>

// Include Newmat Libraries and random number generators
#include "../newmat10/include.h"
#include "../newmat10/newmat.h"
#include "../newmat10/newmatio.h"
#include "../auxiliary/ran1.h"
#include "../auxiliary/betadev.h"
#include "../auxiliary/bnldev.h"

// Include functions from other modules
#include "../dsk_sfc_functions.h"

// -- Functions -- //
void EN_DEM(void);                                  // Aggregates energy demand coming from firms
void ENERGY(void);                                  // Energy production, investment in productive capacity, and energy R&D
void EMISS_IND(void);                               // Calculates aggregate emissions from firms

//-- Flags --//
extern int              flag_energy_exp;
extern int              flag_share_END;
extern int              flag_energyshocks;
extern int              flag_desc;

//-- Pars --//
extern int              N1;
extern int              N2;
extern int              payback_en;
extern double           exp_quota;
extern double           t_CO2_en;
extern double           share_de_0;
extern double           share_RD_en;
extern double           o1_en;
extern long             *p_seed;
extern double           b_a11;
extern double           b_b11;
extern double           uu1_en;
extern double           uu2_en;
extern int              life_plant;
extern int              T;
extern double           exp_quota_param;
extern double           ge_subsidy;

//-- Inits --//
extern double           K_ge0_perc;

// -- Vars -- //
extern RowVector        D1_en;		
extern RowVector        D2_en;			
extern double           D1_en_TOT;              
extern double           D2_en_TOT;              
extern RowVector        D_en_TOT;
extern int              i;
extern int              j;
extern double           tolerance;
extern RowVector        A1p_ef;
extern RowVector        A1p_en;
extern RowVector        Q1;
extern RowVector        Q2;
extern RowVector        A2e_ef;
extern RowVector        A2e_en;
extern RowVector        Emiss1;
extern RowVector        Emiss2;
extern double           Emiss1_TOT;            
extern double           Emiss2_TOT;
extern int              t;
extern int              tt;
extern double           K_ge;                   
extern double           K_de;                  
extern double           Q_ge;                   
extern double           Q_de; 
extern RowVector        C_de;                   
extern RowVector        G_de;  
extern RowVector        G_ge;
extern RowVector        G_ge_n;
extern double           pf;
extern double           mi_en;
extern RowVector        c_en;
extern RowVector        A_de;
extern double           EI_en;
extern double           c_de_min;
extern double           cf_min_ge;
extern RowVector        CF_ge;
extern double           EI_en_de;
extern double           EI_en_ge;
extern double           IC_en;
extern RowVector        IC_en_quota;
extern double           LDexp_en;
extern RowVector        w;
extern double           PC_en;
extern double           Emiss_en;
extern RowVector        EM_de;
extern RowVector        G_de_temp;
extern double           Q_de_temp;
extern int              idmin;
extern double           c_infra;
extern double           share_de;
extern double           Rev_en;
extern double           RD_en_de;
extern double           RD_en_ge;
extern double           LDrd_de;
extern double           LDrd_ge;
extern double           LDentot;
extern double           parber_en_de;
extern double           parber_en_ge;
extern double           parber_en_ge2;
extern double           rnd;
extern double           Inn_en_ge;
extern double           Inn_en_ge2;
extern double           Inn_en_de;
extern double           A_de_inn;
extern double           EM_de_inn;
extern double           CF_ge_inn;
extern RowVector        Emiss_TOT;
extern RowVector        CapitalStock_e;
extern double           Wages_en;
extern double           FuelCost;
extern double           mi_en_preshock;
extern double           pf_preshock;
extern double           mi_en_shock;
extern double           c_en_preshock;
extern double           pf_shock;
extern double           c_infra_t;
extern double           K_gelag;
extern double           K_delag;
extern double           G_de_0; 
extern double           G_ge_0;
extern double           G_ge_n_0;
extern double           Subsidy_Exp;
extern double           subsidy_ge_exp;
extern double           uu1_en_g;
extern double           uu2_en_g;
extern double           exp_quota_0;

#endif