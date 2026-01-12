#ifndef FLAGS_H
#define FLAGS_H

int flag_portfolioallocation;           // determines how banks' demand for government bonds is governed
                                        // 0 = Bond demand governed by banks' share in total profit
                                        // 1 = Bond demand given by percentage of banks' loan portfolio

int flag_shockexperiment;               // Setting = 1 makes model save data needed for climate damage & energy price shock experiments

int flag_exogenousshocks;               // Setting = 1 activates experiment with exogenous climate shocks

int flag_clim_tech;                     // Activates key DSK (as opposed to KS) features including climate box, shocks, energy tech., etc.
                                        // 0 = KS
                                        // 1 = [BASELINE] DSK

int flag_cum_emissions;                 // Switches between C-Roads climate box and simple cumulative emissions one
                                        // = 0 [BASELINE] C-ROADS
                                        // = 1  simple cumulative emission linear relation with temp

int flag_tax_CO2;                       // Activates C02 tax 
                                        // = 0 [BASELINE] off
                                        // = 1 on and increasing with inflation
                                        // = 2 on and increasing linearly with time
                                        // = 3 on and increasing exponentially with time + inflation correction
                                        // = 4 on and increasing with nominal GDP

int flag_encapshocks;                   // Shocks to energy sector's productive capacity
                                        // = 0 no shock
                                        // = 1 Energy sector loses some percentage of both brown and green capacity

int flag_popshocks;                     // Shocks to the population/labour force
                                        // = 0 no shock
                                        // = 1 reduce labour force by some percentage

int flag_demandshocks;                  // Shocks to aggregate consumption demand
                                        // = 0 no shock
                                        // = 1 aggregate consumption demand reduced by x%

int flag_capshocks;                     // Shocks to C-firms' capital stocks
                                        // = 0 no shock
                                        // = 1 x% shocks to capital stocks of all firms 
                                        // = 2 x% shock to aggregate capital stock, affecting firms with uniform probability
                                        // = 3 x% shock to aggregate capital stock, with some firms having higher prob. of being affected

int flag_outputshocks;                  // Shocks to current output of C- and K-firms
                                        // = 0 no shock
                                        // = 1 x% shocks to current output of all C- and K-firms 
                                        // = 2 x% shock to aggregate output of both cons. and cap. goods, affecting firms with uniform probability
                                        // = 3 x% shock to aggregate output of both cons. and cap. goods, with some firms having higher prob. of being affected

int flag_inventshocks;                  // Shocks to inventories of C-firms
                                        // = 0 no shock
                                        // = 1 x% shocks to inventories of all C-firms
                                        // = 2 x% shock to aggregate inventories of C-firms, affecting firms with uniform probability
                                        // = 3 x% shock to aggregate inventories of C-firms, with some firms having higher prob. of being affected

int flag_RDshocks;                      // Shocks to the R&D process of K-firms
                                        // = 0 no shocks
                                        // = 1 on shape of beta distribution (alpha)
                                        // = 2 on support of beta distribution
                                        // = 3 on Bernoulli determining whether firm innovates/imitates at all
                                        // = 4 on amount of resources devoted to R&D

int flag_prodshocks1;                   // Shocks to productivity affecting the characteristics of capital vintages (roughly similar to TFP shocks in conventional models)
                                        // = 0 no shock
                                        // = 1 On labour productivity of current vintages
                                        // = 2 On energy efficiency of current vintages
                                        // = 3 Both labour producitivity and energy efficiency of current vintages
                                        // = 4 On labour productivity of all existing vintages
                                        // = 5 On energy efficiency of all existing vintages
                                        // = 6 Both labour producitivity and energy efficiency of all existing vintages

int flag_prodshocks2;                   // Shocks to productivity (not affecting characteristics of capital vintages)
                                        // = 0 no shock
                                        // = 1 On labour productivity of C-firms and K-firms
                                        // = 2 On energy efficiency of C-firms and K-firms
                                        // = 3 Both labour producitivity and energy efficiency

int flag_energyshocks;                  // Shock to energy price
                                        // = 0 no shock
                                        // = 1 transitory shock to energy price through mark-up
                                        // = 2 permanent shock to energy price through mark-up
                                        // = 3 transitory shock to energy price through fossil fuel price
                                        // = 4 permanent shock to energy price through fossil fuel price

int flag_share_END;                     // Switches on endogenous share of R&D expenditures in dirty vs green energy
                                        // = 0 exogenous, given by share_de_0
                                        // = 1 endogenous

int flag_energy_exp;                    //Determines whether maximum expansion of green energy capacity per period is constrained
                                        // = 0 not constrained
                                        // = 1 [BASELINE] constrained
                                        // = 2 green energy capacity is expanded in order to keep the green share equal to the initial one

int flag_pop_growth;                    // Switches on population growth at exogenous rate from period 101
                                        // = 0 [BASELINE] off
                                        // = 1 on

int flagbailout;                        // Switches between bailout rules for banks
                                        // = 0 [BASELINE] Banks are always bailed out by government
                                        // = 1 Failing banks are purchased by largest bank; government steps in as last resort
                                
int flagFRONT;			                // Determines whether technological frontier evolves endogenously or exogenously (only relevant when flag_clim_tech=0)
								        // = 0 [BASELINE] Endogenous							        
                                        // = 1 Exogenous

int flagTCGEN;			                // Determines whether technological innovation is exogenous or endogenous (only relevant when flag_clim_tech=0)
								        // = 0 [BASELINE] Endogenous
                                        // = 1 Exogenous

int flag_entry;                         // Determines what happens if households cannot finance K-firm and/or C-firm entry
                                        // = 0 The remaining entry costs are paid by government 
                                        // = 1 The remaining entry costs are booked as a loss for the banks

int flag_nonCO2_force;                  // Determines whether or not to consider non-CO2 radiative forcing in the C-Roads climate box
                                        // = 0 Non-CO2 forcing not included
                                        // = 1 [BASELINE] Non-CO2 forcing included

int flag_validation;                    // Determines whether output files needed to construct validation graphs and statistics should be generated
                                        // = 0 No
                                        // = 1 Yes (need to also have flag_shockexperiment=0)

int flag_inventories;                   // Switches C-firm inventories on or off
                                        // = 0 Off
                                        // = 1 On

int flag_scrap_age;                     // Determines whether old machines are always replaced 
                                        // = 0 Yes (old version)
                                        // = 1 No, only if not replacing them would take firm below desired capital stock (new version)

int flag_uniformshocks;                 // Determines whether one-off micro-level climate shocks should be uniform or heterogeneous across agents
                                        // = 0 heterogeneous shocks
                                        // = 1 uniform shocks (i.e. same size shock for all agents)

int flag_desc;

#endif