// Define global variables
#ifndef GLOBAL_VAR_H
#define GLOBAL_VAR_H

#include "dsk_sfc_parameters.h"
#include "dsk_sfc_inits.h"

int fulloutput;          // Dummy indicating whether full output is saved
long int seed;           // Seed for random number generation
long int *p_seed;        // Pointer to seed
RowVector shocks_kfirms; // Vector of climate shocks to K-firms
RowVector shocks_cfirms; // Vector of climate shocks to C-firms
double shock_scalar;     // Scalar climate shock
ofstream Errors;         // File for saving error messages
int cerr_enabled;        // Determines whether error messages to console are printed to console
int verbose;             // Determines whether simulation progress updates are printed to console

int i;                              // Index
int ii;                             // Index
int iii;                            // Index
int j;                              // Index
int jjj;                            // Index
int t;                              // Current simulation period
int tt;                             // Index
int rni;                            // Random number
int t0;                             // Index
int t00;                            // Index
int n;                              // Counter
double pareto_rv;                   // Pareto random number
double tolerance;                   // Tolerance level for deviations from accounting consistency
double regionalaccountingtolerance; // Tolerance for regional accounting validation (log-only)
double deviation;                   // Deviation from accounting consistency
double parber;                      // Input for draw from Bernoulli
double rnd;                         // Random number
double NB;                          // Number of banks
long int NB_long;                   // Number of banks as int
double N1r;                         // Number of K-firms as double
double N2r;                         // Number of C-firms as double
int step;                           // Counter
int stepbis;                        // Counter
int cont;                           // Counter
double age0;                        // Age of initial machines
double Amax;                        // Maximum labour productivity C-firms
double A1pmax;                      // Maximum labour productivity K-firms
double A1_en_max;                   // Maximum energy efficiency C-firms
double A1_ef_max;                   // Maximum environmental friendliness C-firms
double A1p_en_max;                  // Maximum energy efficiency K-firms
double A1p_ef_max;                  // Maximum environmental friendliness K-firms
double D20;                         // Initial demand for consumption goods
int DS2_min_index;                  // Index of minimum debt service to sales ratio
int newbroch;                       // Brochures sent to potential customers
int indforn;                        // C-firm's machine supplier
int flag;                           // Flag
double payback;                     // Payback period
int jmax;                           // Index
int tmax;                           // Index
int imax;                           // Index
double nmachprod;                   // Number of machines used
double nmp_temp;                    // Number of machines used (temporary value)
double cmin;                        // Index
int imin;                           // Index
int jmin;                           // Index
int tmin;                           // Index
double InternalFunds;               // Internal funds available to C-firm
double prestmax;                    // Maximum loans demanded by C-firm
double p1prova;                     // Temporary storage for K-firm price
int rated_firm_2;                   // Index
double Qpast;                       // Temporary storage for K-firm output
double Ipast;                       // Temporary storage for investment
double scrapmax;                    // Maximum number of machines to be scrapped
double cmax;                        // Maximum production cost
int ind_i;                          // Index
int ind_tt;                         // Index
double scrap_n;                     // Nominal value of scrapped machines
int sendingBank;                    // Index
int receivingBank;                  // Index
double c_de_min;                    // Minimum cost of dirty energy plant
double cf_min_ge;                   // Minimum cost of green energy capacity expansion
RowVector G_de_temp;                // Temporary storage for dirty energy capacity
double Q_de_temp;                   // Temporary storage for dirty energy produced
int idmin;                          // Index
double parber_en_de;                // Input for draw from Bernoulli
double parber_en_ge;                // Input for draw from Bernoulli
double parber_en_ge2;               // Input for draw from Bernoulli
double l2m;                         // Mean unsatisfied demand
double p2m;                         // Mean C-firm price
double Cres;                        // Residual consumption
double Cresbis;                     // Temporary storage for residual consumption
RowVector Q2temp;                   // Temporary storage for C-firm output
RowVector f_temp2;                  // Temporary storage for C-firm market shares
RowVector D_temp2;                  // Temporary storage for consumption demand
double cpi_temp;                    // Temporary cpi
RowVector Bond_share;               // Share of gov. bonds owned by each bank
int maxbank;                        // Index
double max_equity;                  // Maximum bank equity
double multip_bailout;              // Multiplier for bailout
double min_equity;                  // Minimum bank equity
int ns1;                            // Number of active K-firms
int ns2;                            // Number of active C-firms
double mD1;                         // Mean deposits K-firms
double mD2;                         // Mean deposits C-firms
double multip_entry;                // Multiplier for firm entry
double injection;                   // Liquidity injection for entering firms
double injection2;                  // Alternative storage for liquidity injection for entering firms
double n_mach_exit;                 // Number of machines of exiting firms
double n_mach_exit2;                // Temporary storage for n_mach_exit
double n_mach_needed;               // Number of machines needed for entering firms
double n_mach_resid;                // Number of remaining machines for entering firms
double n_mach_resid2;               // Temporary storage for remaining machines for entering firms
double n_exit2;                     // Number of exiting C-firms
RowVector k_entry;                  // Share of second-hand capital allocated to each firm
double cpi_init;                    // Initial value of cpi needed for climate policy
double GDP_init;                    // Initial value of nominal GDP needed for climate policy
double baddebt_2_temp;              // Temporary storage for C-firms' bad debt
double markdownCapital;             // Markdown applied to capital goods sold on second hand market
RowVector prior;                    // Used to determine deviations in bank balance sheets
double post;                        // Used to determine deviations in bank balance sheets
double prior_cb;                    // Used to determine deviations in CB balance sheet
double post_cb;                     // Used to determine deviations in CB balance sheet
double DepositsCheck_1;             // Used to detect errors in distribution of firm deposits
double DepositsCheck_2;             // Used to detect errors in distribution of firm deposits
double p2_entry;                    // Price of newly entering C-firms
double f2_exit;                     // Sum of market shares of exiting C-firms
double CurrentDemand;               // Sum of demand experienced by C-firms in current period
RowVector EntryShare;               // Share of available market captured by entering C-firms
RowVector CompEntry;                // Pseudo-competitiveness of entering C-firms
double CompEntry_m;                 // Mean pseudo-competitiveness of entering C-firms
double K_gap;                       // Gap between desired and actual capital
RowVector K_temp;                   // Temporary storage for capital stock
double K_top;                       // Upper limit for expansion investment
int loss;                           // Temporary storage for capital stock lost due to climate shocks
int lossj;                          // Temporary storage for capital stock lost at firm level
RowVector K_loss;                   // Temporary storage for capital stock lost at firm level
int rani;                           // Random integer
int rant;                           // Random integer
int ranj;                           // Random integer
double reduction;                   // Temporary storage for reduction in production due to insufficient labour supply
RowVector marker_age;               // Indicates whether a firm has only 1 unit of capital left which is also older than agemax
double K_temp_sum;                  // Temporary storage for overall capital stock
double mi_en_preshock;              // Temporary storage for pre-shock energy markup
double pf_preshock;                 // Temporary storage for pre-shock fossil fuel price
double mi_en_shock;                 // Temporary storage for shocked energy markup
double c_en_preshock;               // Temporary storage for pre-shock energy price
double pf_shock;                    // Temporary storage for shocked fossil fuel price
double c_infra_t;                   // Target inframarginal energy cost to achieve desired energy price shock
RowVector risk_c;                   // Measure of individual C-firms' exposure to extreme events
RowVector risk_k;                   // Measure of individual K-firms' exposure to extreme events
double ptemp;                       // Temporary storage for C-firm price
RowVector pass_1;                   // Indicator for K-firm passthrough of energy price shock
RowVector pass_2;                   // Indicator for C-firm passthrough of energy price shock
double Ldtemp;                      // Temporary storage for C-Firm labour

// Balance sheet items
RowVector Deposits_h(2);       // Household deposits
RowVector Deposits_e(2);       // Energy Sector deposits
Matrix Deposits_1;             // K-firm deposits
Matrix Deposits_2;             // C-firm deposits
Matrix Deposits;               // Deposits from banks' side
Matrix Deposits_hb;            // Household deposits from banks' side
Matrix Deposits_eb;            // Energy sector deposits from banks' side
Matrix GB_b;                   // Government bonds held by banks
RowVector GB_cb(2);            // Government bonds held by CB
RowVector GB(2);               // Government bonds
RowVector Deposits_fuel(2);    // Deposits of fossil fuel sector with CB
RowVector Deposits_fuel_cb(2); // Deposits of fossil fuel sector from CB side
Matrix Loans_2;                // Loans of C-firms
Matrix Loans_b;                // Loans to C-firms from banks' side
Matrix Advances_b;             // CB advances to banks
RowVector Advances(2);         // CB advances
Matrix Reserves_b;             // CB reserves held by banks
RowVector Reserves(2);         // CB Reserves
Matrix CapitalStock;           // Nominal value of C-firms' capital stock (machines)
Matrix deltaCapitalStock;      // Change in nominal value of C-firms' capital stock
RowVector CapitalStock_e(2);   // Nominal value of Energy sectors' capital stock (plants)
Matrix Inventories;            // Nominal value of C-firms' inventories
RowVector NW_h(2);             // Net worth of households
Matrix NW_1;                   // Net worth of K-firms
Matrix NW_2;                   // Net worth of C-firms
Matrix NW_b;                   // Net worth of banks
RowVector NW_gov(2);           // Net worth of Government
RowVector NW_cb(2);            // Net worth of CB
RowVector NW_e(2);             // Net worth of Energy sector
RowVector NW_f(2);             // Net worth of fossil fuel sector

double NW_h_c;     // Net worth of households (control; for SFC-check)
RowVector NW_1_c;  // Net worth of K-firms (control; for SFC-check)
RowVector NW_2_c;  // Net worth of C-firms (control; for SFC-check)
RowVector NW_b_c;  // Net worth of banks (control; for SFC-check)
double NW_gov_c;   // Net worth of Government (control; for SFC-check)
double NW_cb_c;    // Net worth of CB (control; for SFC-check)
double NW_e_c;     // Net worth of energy sector (control; for SFC-check)
double NW_f_c;     // Net worth of fossil fuel sector (control; for SFC-check)
double NWSum;      // Sum of net worths
double RealAssets; // Nominal value of all real assets in the economy

// Additional TFM Items
RowVector Wages_2;            // Wages paid by C-firms
RowVector Wages_1;            // Wages paid by K-firms
double Wages_en;              // Wages paid by Energy sector
double Wages;                 // Wages received by households
RowVector Dividends(2);       // Dividend payments received by households
double Benefits;              // Unemployment benefits received by households
RowVector Investment_2;       // Investiment expenditures of C-firms
RowVector EnergyPayments_1;   // Payments of K-firms for energy
RowVector EnergyPayments_2;   // Payments of C-firms for energy
double EnergyPayments;        // Energy payments received by energy sector
RowVector Dividends_1;        // Dividends paid by K-firms
RowVector Dividends_2;        // Dividends paid by C-firms
RowVector Dividends_b;        // Dividends paid by banks
double Dividends_e;           // Dividends paid by energy sector
RowVector Taxes_1;            // Taxes on profits paid by K-firms
RowVector Taxes_2;            // Taxes on profits  paid by C-firms
double Taxes_h;               // Taxes on income paid by households
RowVector Taxes_b;            // Taxes on profits paid by banks
RowVector Taxes_CO2_1;        // Taxes on CO2 paid by K-firms
RowVector Taxes_CO2_2;        // Taxes on CO2 paid by C-firms
double Taxes_CO2_e;           // Taxes on CO2 paid by energy sector
double Taxes_CO2;             // Total CO2 taxes collected by the government
RowVector InterestDeposits_1; // Deposit interest paid to K-firms
RowVector InterestDeposits_2; // Deposit interest paid to C-firms
RowVector InterestDeposits;   // Deposit interest paid by banks
double InterestDeposits_h;    // Deposit interest paid to households
double InterestDeposits_e;    // Deposit interest paid to energy sector
double InterestBonds;         // Bond interest paid by government
double InterestBonds_cb;      // Bond interest paid to CB
RowVector InterestBonds_b;    // Bond interest paid to banks
double BondRepayments_cb;     // Bond repayments made to CB
RowVector BondRepayments_b;   // Bond repayments made to banks
double Taxes;                 // Taxes collected by government
double Consumption;           // Consumption expenditure of households
RowVector LoanInterest;       // Loan interest paid to banks
RowVector LoanInterest_2;     // Loan interest paid by C-firms
double FirmTransfers;         // Transfer payments for firm entry made by households
double FirmTransfers_1;       // Transfer payments received by entering K-firms
double FirmTransfers_2;       // Transfer payments received by entering C-firms
RowVector Injection_1;        // Total injection of net worth for entering K-firms
RowVector Injection_2;        // Total injection of net worth for entering C-firms
double InterestReserves;      // Interest on reserves paid by CB
double InterestAdvances;      // Interest on advances received by CB
RowVector InterestReserves_b; // Interest on reserves received by banks
RowVector InterestAdvances_b; // INterest on advances paid by banks
double TransferCB;            // Transfers of CB profits to government
double FuelCost;              // Cost of fossil fuels for Energy sector
double TransferFuel;          // Transfer from fossil fuel sector to households
double Taxes_e_shock;         // Excess profit tax on energy sector
double Taxes_f_shock;         // Excess profit tax on fossil fuel sector
double Transfer_shock;        // Transfer to households to combat energy price shock

double Balance_h;     // Sectoral balance households
double Balance_1;     // Sectoral balance K-firms
double Balance_2;     // Sectoral balance C-firms
double Balance_e;     // Sectoral balance energy sector
double Balance_b;     // Sectoral balance banks
double Balance_g;     // Sectoral balance government
double Balance_cb;    // Sectoral balance CB
double Balance_f;     // Sectoral balance fossil fuels
double BalanceSum;    // Sum of sectoral balances
RowVector Balances_1; // Individual balances K-firms

// Households
RowVector w(2);              // Wage rate
double LS;                   // Labour supply
RowVector U(2);              // Unemployment rate
double Divtot_1;             // Total K-firm dividends paid to households
double Divtot_2;             // Total C-firm dividends paid to households
double Cons;                 // Households' consumption demand
double Deposits_recovered_1; // Liquidity recovered from failing K-firms
double Deposits_recovered_2; // Liquidity recovered from failing C-firms

// C-Firms
RowVector p2;                       // C-firms' prices
RowVector BankingSupplier_2;        // C-firms' suppliers of banking services
Matrix BankMatch_2;                 // Matrix matching C-firms to banks
RowVector A2;                       // Labour productivity C-firms
RowVector A2_en;                    // Energy efficiency C-firms
RowVector A2_ef;                    // Environmental friendliness C-firms
RowVector c2;                       // Production cost C-firms
RowVector c2p;                      // Production cost C-firms use to set prices (for energy price shock experiment)
RowVector c1p;                      // Production cost K-firms use to set prices (for energy price shock experiment)
RowVector l2;                       // Unsatisfied consumption demand
RowVector DebtRemittances2;         // Loan repayments C-firms
RowVector baddebt_2;                // Bad debt of exiting C-firms
Matrix S2;                          // C-firm revenues
RowVector Sales2;                   // Temporary storage for C-firm revenues
RowVector K;                        // C-firms' productive capacity
RowVector K_cur;                    // Needed for shocks to capital stock
Matrix f2;                          // C-firms' market share
RowVector E2;                       // C-firms' competitiveness
RowVector CreditDemand;             // C-firms' credit demand
RowVector I;                        // Total investment of C-firms in terms of productive capacity
Matrix EI;                          // Expansion investment of C-firms in terms of productive capacity
RowVector EI_n;                     // Nominal value of C-firms' expansion investment
RowVector SI;                       // Substitution investment of C-firms in terms of productive capacity
RowVector SI_n;                     // Nominal value of C-firms' substitution investment
RowVector Q2;                       // Quantity produced by C-firms
Matrix mu2;                         // Mark-up of C-firms
RowVector fornit;                   // C-firms' supplier of machine tools
RowVector n_mach;                   // C-firms' number of machines
Matrix D2;                          // Demand for consumption goods
RowVector De;                       // Expected demand
Matrix N;                           // Inventories (real)
RowVector Ne;                       // Desired inventories
RowVector DebtServiceToSales2;      // C-firms' debt service to sales ratio
Matrix DebtService_2;               // C-firms' debt service
Real DS2_min;                       // Minimum debt service to sales ratio
RowVector DebtServiceToSales2_temp; // Temporary storage for C-firms' debt service to sales ratio
RowVector k;                        // Ranking of C-firms' debt service to sales ratio
RowVector r_deb_h;                  // Borrowing rate charged to individual C-firms
RowVector EId;                      // Desired expansion investment
RowVector SId;                      // Desired replacement investment
RowVector SId_age;                  // Desired replacement investment due to excessive age
RowVector SId_cost;                 // Desired replacement investment due to high cost
RowVector EIp;                      // Expansion investment post determination of maximum acceptable borrowing
RowVector SIp;                      // Replacement investment post determination of maximum acceptable borrowing
RowVector SIp_age;                  // Replacement investment due to excessive age post determination of maximum acceptable borrowing
RowVector SIp_cost;                 // Replacement investment due to high cost post determination of maximum acceptable borrowing
RowVector Ip;                       // Overall investment post determination of maximum acceptable borrowing
RowVector Cmach;                    // Cost of overall investment
RowVector CmachEI;                  // Cost of expansion investment
RowVector CmachSI;                  // Cost of replacement investment
RowVector CmachSI_age;              // Cost of replacement investment due to excessive age
RowVector CmachSI_cost;             // Cost of replacement investment due to high cost
RowVector Qd;                       // Quantity demanded from C-firms
RowVector Kd;                       // Desired capital stock of C-firms
RowVector Ktrig;                    // Current capital stock determining expansion investment
RowVector A2e;                      // Effective labour productivity
RowVector c2e;                      // Effective unit cost
RowVector A2e_en;                   // Effective energy efficiency
RowVector A2e_ef;                   // Effective environmental friendliness
RowVector A2e2;                     // Needed for capital stock shocks
RowVector A2e_en2;                  // Needed for capital stock shocks
RowVector A2e_ef2;                  // Needed for capital stock shocks
RowVector Ld2;                      // C-firms' labour demand
RowVector Ld2_control;              // Needed for capital stock shocks
RowVector mol;                      // C-firms' net revenue
RowVector exiting_2;                // Indicating whether firm is exiting
RowVector exit_payments2;           // Indicating whether firm is exiting due to inability to make a payment
RowVector exit_equity2;             // Indicating whether firm is exiting due to negative equity
RowVector exit_marketshare2;        // Indicating whether firm is exiting due to loss of market share
RowVector D2_en;                    // C-firms' energy demand
RowVector Emiss2;                   // C-firms' emissions
RowVector dN;                       // Change in inventories
RowVector dNm;                      // Change in nominal value of inventories
RowVector Pi2;                      // C-firms' profit
RowVector n_mach_entry;             // Number of machines of entering firms
RowVector scrap_age;                // Number of machines scrapped due to age

std::vector<std::vector<std::vector<int>>> age;        // Age of exiting machines
std::vector<std::vector<std::vector<double>>> g_c;     // Frequency of machines for cost calculation
std::vector<std::vector<std::vector<double>>> g_c2;    // Needed for shocks to capital stock
std::vector<std::vector<std::vector<double>>> g_c3;    // Needed for shocks to capital stock
std::vector<std::vector<std::vector<double>>> g;       // Frequency of machines
std::vector<std::vector<std::vector<double>>> g_price; // Array containing original purchase prices of machines
std::vector<std::vector<std::vector<double>>> gtemp;   // Temporary storage for frequency of machines
std::vector<std::vector<std::vector<double>>> C_pb;    // Array containing production costs of machines to be scrapped
std::vector<std::vector<std::vector<double>>> g_pb;    // Array containing machines to be scrapped
std::vector<std::vector<double>> g_secondhand;         // Machines to be sold on second-hand market
std::vector<std::vector<double>> g_secondhand_p;       // Prices of machines to be sold on second-hand market
std::vector<std::vector<int>> age_secondhand;          // Age of machines to be sold on second-hand market

// K-firms
RowVector p1;                // Prices of K-firms
RowVector BankingSupplier_1; // K-firms' suppliers of banking services
Matrix BankMatch_1;          // Matrix matching K-firms to banks
RowVector baddebt_1;         // Bad debt of exiting K-firms
RowVector A1;                // Productivity of machines produced by K-firm
RowVector A1p;               // Productivity of K-firm production technique
RowVector A1_en;             // Energy Efficiency of machines produced by K-firm
RowVector A1_ef;             // Environmental Friendliness of machines produced by K-firm
RowVector A1p_en;            // Energy Efficiency of K-firm production technique
RowVector A1p_ef;            // Environmental Friendliness of K-firm production technique
Matrix A_en;                 // Matrix containing energy efficiencies of existing machine tools
Matrix A_ef;                 // Matrix containing environmental friendliness of existing machine tools
Matrix A;                    // Matrix containing productivities of existing machine tools
Matrix C;                    // Matrix containing cost of existing machine tools
Matrix C_secondhand;         // Matrix containing cost of machine tools available on secondhand market
RowVector c1;                // Production cost of K-firms
Matrix f1;                   // Market share of K-firms
RowVector Q1;                // Quantity produced by K-firms
RowVector Td;                // Technological distance
RowVector Tdw;               // Technological distance to foreign firms
Matrix Match;                // Matrix matching K-firms to customers
RowVector S1;                // Revenues of K-firms
RowVector Sales1;            // Temporary storage for revenues of K-firms
double A1top;                // Maximum productivity of machines produced
double A1ptop;               // Maximum productivity of K-firm production process
RowVector A1f;               // Productivity of machines produced by foreign firms
RowVector A1pf;              // Productivity of production process used by foreign firms
double A1_en_top;            // Maximum energy efficiency of machines produced
double A1p_en_top;           // Maximum energy efficiency of K-firm production process
double A1_ef_top;            // Maximum environmental friendliness of machines produced
double A1p_ef_top;           // Maximum environmental friendliness of K-firm production process
Matrix RD;                   // K-firms' R&D expenditures
RowVector Ld1rd;             // Labour demand for R&D
RowVector Inn;               // Indicates whether K-firm innovates
RowVector Imm;               // Indicates whether K-firm imitates
RowVector A1inn;             // Productivity of innovated machine
RowVector A1pinn;            // Productivity of innovated production process
RowVector A1imm;             // Productivity of imitated machine
RowVector A1pimm;            // Productivity of imitated production process
RowVector EE_inn;            // Energy efficiency of innovated machine
RowVector EEp_inn;           // Energy efficiency of innovated production process
RowVector EE_imm;            // Energy efficiency of imitated machine
RowVector EEp_imm;           // Energy efficiency of imitated production process
RowVector EF_inn;            // Environmental friendliness of innovated machine
RowVector EFp_inn;           // Environmental friendliness of innovated production process
RowVector EF_imm;            // Environmental friendliness of imitated machine
RowVector EFp_imm;           // Environmental friendliness of imitated production process
RowVector nclient;           // K-firms' number of clients
RowVector RDin;              // Part of R&D expenditure devoted to innovation
RowVector RDim;              // Part of R&D expenditure devoted to imitation
RowVector A1w;               // Productivity of machines produced in rest of the world
RowVector A1pw;              // Productivity of production process used in rest of the world
RowVector exiting_1;         // Indicates whether K-firm is exiting
RowVector D1_en;             // Energy demand of K-firms
RowVector D1;                // Amount of machines demanded from K-firms
RowVector Ld1;               // Labour demand for production of K-firms
RowVector Emiss1;            // Emissions of K-firms
RowVector Pi1;               // Profit of K-firms
RowVector Anew;              // Productivity of new machines after exogenous technological change
RowVector ee1;               // K-firm from which newly entering one will copy

// Banks
RowVector NbClient_1;            // Banks' number of K-firm clients
RowVector NbClient_2;            // Banks' number of C-firm clients
RowVector NL_1;                  // Banks' initial number of K-firm clients
RowVector NL_2;                  // Banks' initial number of C-firm clients
Matrix fB;                       // Market Share of banks
double r_depo;                   // Interest rate on bank deposits
RowVector r_deb;                 // Base loan rate of banks
RowVector Fragility;             // Bank fragility measure = Bad debt/Net worth of bank
RowVector Bank_active;           // Indicates whether bank is active or has been bought
RowVector bankmarkup;            // Mark-up of banks over monetary policy rate
RowVector BankProfits;           // Profit of banks
RowVector BankProfits_temp;      // Temporary storage for banks' profit
RowVector BankCredit;            // Remaining credit supply
RowVector BaselBankCredit;       // Credit supply given by regulatory ratio
RowVector buffer;                // Used for calculating maximum credit according to regulatory ratio
RowVector bonds_dem;             // Banks' demand for government bonds
double bonds_dem_tot;            // Total demand for government bonds from banks
Matrix DebtServiceToSales2_bank; // Matrix storing debt service to sales ratio for clients of each bank
Matrix DS2_rating;               // Matrix ranking clients of each bank by debt service to sales ratio
RowVector Outflows;              // Transactions implying outflows of reserves
RowVector Inflows;               // Transactions implying inflows of reserves
RowVector DepositShare_e;        // Banks' share in deposit "market"
RowVector DepositShare_h;        // Banks' share in deposit "market"
RowVector baddebt_b;             // Bad debt held by banks
RowVector bonds_purchased;       // Gov. bonds purchases in the current period
RowVector BankEquity_temp;       // Temporary storage for bank net worth
RowVector Bailout_b;             // Government bailouts received by banks
RowVector LossAbsorbed;          // Losses absorbed when banks buy failing banks
RowVector capitalRecovered;      // Nominal value of second-hand capital recovered from failing firms
RowVector capitalRecovered2;     // Temporary storage for nominal value of second-hand capital recovered from failing firms
RowVector capitalRecoveredShare; // Share of nominal value of second-hand capital recovered from failing firms
RowVector ReserveBalance;        // Balance between inflows and outflows of reserves
RowVector LossEntry_b;           // Losses from firm entry taken by banks
RowVector ShareBonds;            // Share of gov. bonds held by each bank
RowVector ShareReserves;         // Share of reserves held by each bank
RowVector ShareAdvances;         // Share of advances held by each bank
RowVector Adjustment;            // Adjustment term to preserve stock-flow consistency

// Government
double r_bonds;      // Interest rate on gov. bonds
double Bailout;      // Cost of bank bailouts
double G;            // Government spending on unemployment benefits
double Deficit;      // Public deficit
double PSBR;         // Public sector borrowing requirement (includes bond repayments to be financed)
double NewBonds;     // Newly issued bonds
double EntryCosts;   // Firm entry costs paid by government
double BankTransfer; // Transfers from gov. to banks for firm entry

// Central Bank
double r_cbreserves;   // Interest rate on CB reserves
double r_a;            // Annual policy rate (used in quarterly calibration)
double r;              // Monetary policy rate
RowVector ProfitCB(2); // Profits of the central bank
double Adjustment_cb;  // Adjustment term to preserve stock-flow consistency
double d_cpi_target_a; // Annualised inflation target
double inflation_a;    // Annualised inflation rate

// Energy
RowVector A_de;        // Thermal efficiency of dirty energy plants
RowVector EM_de;       // Emissions per unit of energy produced of dirty plants
double pf;             // Price of fossil fuel
double mi_en;          // Mark-up of energy producer
RowVector G_de;        // Number of dirty energy plants of different vintages
RowVector G_ge;        // number of green energy plants of different vintages
RowVector G_ge_n;      // Book value of green energy plants of different vintages
RowVector CF_ge;       // Cost of expanding green energy capacity
RowVector c_en(2);     // Price of energy
double D1_en_TOT;      // Total energy demand from K-firms
double D2_en_TOT;      // Total energy demand from C-firms
RowVector D_en_TOT(2); // Total energy demand
double K_ge;           // Total capacity of green energy
double K_de;           // Total capacity of dirty energy
double K_gelag;        // Lagged value of green capacity
double K_delag;        // Lagged value of dirty capacity
double Q_ge;           // Quantity of green energy produced
double Q_de;           // Quantity of dirty energy produced
RowVector C_de;        // Unit production cost of dirty energy plants
double EI_en;          // Investment in productive capacity of energy sector
double EI_en_de;       // Investment in dirty energy capacity
double EI_en_ge;       // Investment in green energy capacity
double IC_en;          // Total cost of green energy investment
RowVector IC_en_quota; // Investment cost for green energy paid in each period (ammortisation)
double LDexp_en;       // Labour demand for green energy investment
double PC_en;          // Production cost of energy sector
double c_infra;        // Unit cost of the inframarginal dirty plant used
double share_de;       // Share of dirty energy in total energy capacity
double Rev_en;         // Revenue of energy sector
double RD_en_de;       // R&D expenditure on dirty technology
double RD_en_ge;       // R&D expenditure on green technology
double LDrd_de;        // Labour demand for R&D in dirty technology
double LDrd_ge;        // Labour demand for R&D in green technology
double Inn_en_ge;      // Indicates whether innovation has taken place in green energy
double Inn_en_ge2;     // Indicates whether innovation has taken place in government-supported green energy R&D
double Inn_en_de;      // Indicates whether innovation has taken place in dirty energy
double A_de_inn;       // Thermal efficiency of new dirty technology
double EM_de_inn;      // Emission intensity of new dirty technology
double CF_ge_inn;      // Installation cost of new green technology
double ProfitEnergy;   // Profit of the energy sector
double G_de_0;         // Initial capacity of dirty energy
double G_ge_0;         // Initial capacity of green energy
double G_ge_n_0;       // Initial nominal value of green capacity
double Subsidy_Exp;    // Subsidy for expansion of green energy capacity
double subsidy_ge_exp; // Subsidy rate for expansion of green energy capacity
double uu1_en_g;       // Boost to R&D effectiveness from public support
double uu2_en_g;       // Boost to R&D effectiveness from public support

// Regions
int NR;                                               // Number of regions
RowVector region_K_shares;                            // Shares of K-firm initial locations by region
RowVector region_C_shares;                            // Shares of C-firm initial locations by region
RowVector region_energy_dirty_shares;                 // Shares of initial dirty capacity by region
RowVector region_energy_green_shares;                 // Shares of initial green capacity by region
RowVector relocation_probability_K;                   // Relocation probabilities for entering K-firms
RowVector relocation_probability_C;                   // Relocation probabilities for entering C-firms
RowVector ge_growth_probability;                      // Probabilities for allocating new green capacity by region
RowVector de_growth_probability;                      // Probabilities for allocating new dirty capacity by region
std::vector<int> region_firm_assignment_K;            // Region index for each K-firm (1-based regions)
std::vector<int> region_firm_assignment_C;            // Region index for each C-firm (1-based regions)
std::vector<double> region_labor_supply;              // Regional labour supply levels
std::vector<double> region_unemployment;              // Regional unemployment levels (deprecated: use reg_U)
std::vector<std::string> region_resultsexp_names;     // Filenames for regional resultsexp outputs
std::vector<std::ofstream> region_resultsexp_streams; // Persistent streams for regional resultsexp outputs
std::vector<double> region_dirty_capacity;            // Regional dirty capacity stocks (derivative tracking)
std::vector<double> region_green_capacity;            // Regional green capacity stocks (derivative tracking)
std::vector<double> region_dirty_capacity_lag;        // Lagged regional dirty capacity stocks
std::vector<double> region_green_capacity_lag;        // Lagged regional green capacity stocks

// Regional Accounting Variables
std::vector<double> reg_GDP_r;         // Regional real GDP
std::vector<double> reg_Consumption_r; // Regional total real consumption
std::vector<double> reg_Investment_r;  // Regional total real investment
std::vector<double> reg_U;             // Regional unemployment rate
std::vector<double> reg_Am;            // Regional mean productivity (weighted avg of K and C-firm productivity)
std::vector<double> reg_Loans_2;       // Regional loans of C-firms
std::vector<double> reg_Inventories;   // Regional nominal value of C-firms' inventories
std::vector<double> reg_N;             // Regional real inventories
std::vector<double> reg_GDP_n;         // Regional nominal GDP
std::vector<double> reg_Qge;           // Regional quantity of green energy produced
std::vector<double> reg_D_en_TOT;      // Regional total energy demand
std::vector<double> reg_Emiss_TOT;     // Regional total emissions
std::vector<double> reg_Cum_emissions; // Regional cumulative emissions

// Climate
RowVector Tmixed(2);             // Temperature in the mixed layer
RowVector Emiss_yearly_calib(2); // Emissions calibrated w.r.t. year 2010
double g_rate_em_y;              // Annual growth rate of emissions
RowVector Emiss_TOT;             // Total emissions
RowVector Emiss_yearly(2);       // Annual emissions
double Emiss1_TOT;               // Total emissions K-firms
double Emiss2_TOT;               // Total emissions C-firms
double Emiss_en;                 // Total emissions Energy sector
double NPP;                      // Net primary production
RowVector Tforvar(5);            // Used for computing variance of temperature
double Cum_emissions;            // Cumulative emissions
RowVector shocks_machprod;       // Shocks to productivity of machines
RowVector shocks_techprod;       // Shocks to productivity of K-firm production processes
RowVector shocks_encapstock_de;  // Shocks to capacity of dirty energy
RowVector shocks_encapstock_ge;  // Shocks to capacity of green energy
RowVector shocks_capstock;       // Shocks to stock of machines
RowVector shocks_invent;         // Shocks to inventories
RowVector shocks_rd;             // Shocks to R&D effectiveness
RowVector shocks_labprod1;       // Shocks to K-firm labour productivity
RowVector shocks_labprod2;       // Shocks to C-firm labour productivity
RowVector shocks_eneff1;         // Shocks to K-firm energy efficiency
RowVector shocks_eneff2;         // Shocks to C-firm energy efficiency
RowVector shocks_output1;        // Shocks to K-firm output
RowVector shocks_output2;        // Shocks to C-firm output
double shock_pop;                // Shock to population of households
double shock_cons;               // Shock to aggregate consumption demand
RowVector X_a;                   // Location parameters of beta distribution for climate shocks
RowVector X_b;                   // Scale parameters of beta distribution for climate shocks
RowVector Loss_Capital;          // Nominal value of capital stock destroyed by climate shock
RowVector Loss_Inventories;      // Nominal value of inventories destroyed by climate shock
double t_CO2;                    // Carbon tax on firms
double t_CO2_en;                 // Carbon tax on energy sector
RowVector Con0;                  // Pre-industrial carbon content of ocean layers
double Emiss_gauge;              // Reference value for emissions
RowVector Cat(2);                // Atmospheric carbon in GtC
double humrelease;               // Carbon released from decaying humus
RowVector hum(2);                // Carbon stored in humus (GtC)
double biorelease;               // Carbon released from decaying biomass
RowVector biom(2);               // Carbon stored in biomass (GtC)
double Cat1;                     // Temporary guess for Cat after biosphere and emissions
double dCat1;                    // Carbon added to atmosphere and ocean
RowVector fluxC;                 // Carbon flux between ocean layers
Matrix Con;                      // Carbon concentration in the ndep ocean layers in GtC
Matrix Hon;                      // Heat content of the ocean layers  (Joule per m^2 of ocean surface)
Matrix Ton;                      // Temperature of the ocean layers
double Con1;                     // Temporary guess for carbon in upper ocean layer (GtC)
double Ctot1;                    // Temporary guess for carbon in upper ocean layer + atmosphere (GtC)
RowVector Cax;                   // Cat used during iteration for equlibrating atmospheric and mixed-layer carbon
RowVector Caxx;                  // Cat used during iteration for equlibrating atmospheric and mixed-layer carbon
RowVector Cay;                   // Residual used during iteration
RowVector Cayy;                  // Residual used during iteration
RowVector Caa;                   // Estimated slope duing the iteration
double FCO2;                     // Radiative forcing from CO2
double Fin;                      // Radiative forcing input due to all greenhouse gases
double Fout;                     // Additional out-radiation due to global warming
RowVector fluxH;                 // Heat flux between ocean layers
double Emiss_global;             // Global GHG emissions (used for EU calibration)

// Others; Aggregate; Reporting etc.
RowVector Am(2);                // Mean productivity across K and C-firms
double Am_a;                    // Mean productivity across K and C-firms (alternative definition)
double Am2;                     // Mean productivity across C-firms
double Am1;                     // Mean productivity across K-firms
RowVector ftot(3);              // Sum of C-firm market shares
RowVector Em2(2);               // Mean competitiveness of C-firms
RowVector cpi(5);               // Consumer price index
double kpi;                     // Price index of capital goods
RowVector Am_en(2);             // Mean energy efficiency across K and C-firms
double LD1rdtot;                // Total labour demand for R&D from K-firms
double LDentot;                 // Total labour demand of energy sector
double Tdtot;                   // Sum of technological distances
double LD1tot;                  // Total labour demand for production from K-firms
double LD2tot;                  // Total labour demand for production from C-firms
double LSe;                     // Remaining labour supply
double LD;                      // Total labour demand
double LD2;                     // Total labour demand for production from firms
double Pitot1;                  // Total profit of K-firms
double Pitot2;                  // Total profit of C-firms
double dNtot;                   // Total change in real value of inventories
double dNmtot;                  // Total change in nominal value of inventories
double ExpansionInvestment_r;   // Total real expansion investment
double ExpansionInvestment_n;   // Total nominal expansion investment
double ReplacementInvestment_r; // Total real replacement investment
double ReplacementInvestment_n; // Total nominal replacement investment
double Investment_r;            // Total real investment
double Investment_n;            // Total nominal investment
double Consumption_r;           // Total real consumption
Real CreditDemand_all;          // Total demand for bank credit
Real CreditSupply_all;          // Total supply of bank credit
double Q2tot;                   // Total output of C-firms
double Q1tot;                   // Total output of K-firms
double Q2dtot;                  // Total desired output of C-firms
double D2tot;                   // Total demand for consumption goods
double A_mi;                    // Mean of log C-firm productivity
double A1_mi;                   // Mean of log K-firm productivity
double A2_en_mi;                // Mean of log C-firm energy efficiency
double A2_ef_mi;                // Mean of log C-firm environmental friendliness
double A1_en_mi;                // Mean of log log K-firm energy efficiency
double A1_ef_mi;                // Mean of log K-firm environmental friendliness
double A_sd;                    // Standard deviation of C-firm productivity
double H1;                      // Herfindahl index K-firms
double H2;                      // Herfindahl index C-firms
double HB;                      // Herfindahl index banks
double GDP_rg;                  // Growth rate real GDP
double GDP_ng;                  // Growth rate nominal GDP
RowVector GDP_r(2);             // Real GDP
RowVector GDP_n(2);             // Nominal GDP
double d_U;                     // Change in unemployment rate
double d_cpi;                   // Change in consumer price index
double d_Am;                    // Change in mean productivity
double d_cpi2;                  // Change in consumer price index
double dw;                      // Change in wage
double dw2;                     // Change in wage
double A2scr;                   // Log deviation of C-firm productivity from mean
double A1scr;                   // Log deviation of K-firm productivity from mean
Matrix S1_temp;                 // Temporary storage for K-firm sales
Matrix S2_temp;                 // Temporary storage for C-firm sales
double Utilisation;             // C-firms' aggregate capacity utilisation
double counter_bankfailure;     // Number of failing banks
double A1p_en_dead;             // Energy efficiency of failing K-Firms
double A1p_en_survive;          // Energy efficiency of surviving K-Firms
double A2_en_dead;              // Energy efficiency of failing C-Firms
double A2_en_survive;           // Energy efficiency of surviving C-Firms
double exp_quota_0;             // Initial value of maximum expansion of green energy

// Filenames
char nomefile1[64];     // File "out" (inv_output1)
char nomefile2[64];     // File "ymc" (inv_ymc)
char nomefile3[64];     // File "resultsexp" (inv_res)
char nomefile4[64];     // File "A1" (inv_prod1)
char nomefile5[64];     // File "A2" (inv_prod2)
char nomefile6[64];     // File "A1_all" (inv_prodall1)
char nomefile7[64];     // File "A2_all" (inv_prodall2)
char nomefile8[64];     // File "A1all_en" (inv_prodall1_en)
char nomefile9[64];     // File "A2all_en" (inv_prodall2_en)
char nomefile10[64];    // File "A1all_ef" (inv_prodall1_ef)
char nomefile11[64];    // File "A2all_ef" (inv_prodall2_ef)
char nomefile12[64];    // File "NW1all" (inv_nwall1)
char nomefile13[64];    // File "NW2all" (inv_nwall2)
char nomefile14[64];    // File "NWBall" (inv_nwall3)
char nomefile15[64];    // File "Deb2all" (inv_deball2)
char nomefile16[64];    // File "validation1" (inv_val1)
char nomefile17[64];    // File "validation2" (inv_val2)
char nomefile18[64];    // File "validation3" (inv_val3)
char nomefile19[64];    // File "validation4" (inv_val4)
char nomefile20[64];    // File "validation5" (inv_val5)
char nomefile21[64];    // File "validation6" (inv_val6)
char nomefile22[64];    // File "validation7" (inv_val7)
char nomefile23[64];    // File "validation8" (inv_val8)
char nomefile24[64];    // File "validation9" (inv_val9)
char nomefile25[64];    // File "validation10" (inv_val10)
char nomefile26[64];    // File "validation10" (inv_val10)
char nomefile27[64];    // File "validation10" (inv_val10)
char nomefile28[64];    // File "shockpars" (inv_shockpars)
char errorfilename[64]; // Name of error file

#endif