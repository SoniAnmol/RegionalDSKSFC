// Define Functions

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "rapidjson/document.h"

// Initialisation
void SETPARAMS(const rapidjson::Document& inputs);                      // Sets parameters, flags and initial values using JSON input
void RESIZE(void);                                                      // Re-sizes all arrays and matrices based on supplied number of agents & periods
void INITIALIZE(int Exseed);                                            // Initialises the model
void ALLOCATEBANKCUSTOMERS(void);                                       // Sets the number of customers of each bank 
double bpareto(double, double, double);                                 // Generates draws from pareto distribution

// Model functions
void SETVARS(void);                                                     // Resets all variables for which this is necessary at the beginning of each period
void DEPOSITINTEREST(void);                                             // Banks pay deposit interest to firms, households and energy sector
void MACH(void);                                                        // New machines & technologies are delivered; prices are calculated
void BROCHURE(void);                                                    // K-firms send brochures to new potential customers
void INVEST(void);                                                      // C-firms calculate expected demand, desired expansion investment & inventories
void SCRAPPING(void);				                                    // Old machines to be scrapped & corresponding substitution investment are determined
void COSTPROD(void);                                                    // C-firms calculate expected production cost
void ORD(void);                                                         // C-firms determine maximum acceptable debt & provisional orders of machines
void ALLOCATECREDIT(void);                                              // Bank credit is allocated. Rationed firms scale down investment and if necessary production. Firms which cannot produce exit
void PRODMACH(void);                                                    // K-firms produce machines; total labour and energy demand are determined; 
void ADJUSTEMISSENLAB(void);                                               // Adjust emissions and energy demand in case of shocks to capital stock
void PAY_LAB_INV(void);                                                 // Wages and investment are paid
void CANCMACH(void);                                                    // Old machines are scrapped and replaced by new ones
void COMPET2(void);                                                     // C-firms' market shares are determined
void PROFIT(void);                                                      // Profits for firms and energy sector are determined; firms which cannot pay taxes or interest exit
void ALLOC(void);                                                       // Household consumption demand is allocated to C-firms
void ENTRYEXIT(void);                                                   // Firms exit and are replaced
void TECHANGEND(void);                                                  // Endogenous technological change
void SFC_CHECK(void);                                                   // Checks for stock-flow consistency
void SAVE(void);                                                        // Current period values are written to output files
void UPDATE(void);                                                      // Lagged variables are updated
void OVERBOOST(void);                                                   // Speeds up simulation by shortening iterations through technology arrays

// Auxiliary
double ROUND(double);
void DEPOSITCHECK(void);                                                // Used to check that bank deposits are being tracked correctly in all sectors
void NEGATIVITYCHECK(void);                                             // Used to check that variables which should not become negative are indeed not negative
void CHECKSUMS(void);                                                   // Used to check that micro-level stocks correctly add up to their respective aggregates
void ADJUSTSTOCKS(void);                                                // Used to adjust for accounting deviations arising from rounding issues
void catchAlarm(int sig);

//  Creation of output files
void FOLDERS(char *path);
void INTFILE(void);	
void GENFILEOUTPUT1(char *path, const char *s1, char runname[], char const* seednumber);
void GENFILEYMC(char *path, const char *s2,char runname[], char const* seednumber);
void GENFILESHOCKEXP(char *path, const char *s3,char runname[], char const* seednumber);
void GENFILEPROD1(char *path, const char *s4, char runname[], char const* seednumber);
void GENFILEPROD2(char *path, const char *s5, char runname[], char const* seednumber);
void GENFILEPRODALL1(char *path, const char *s6, char runname[], char const* seednumber);
void GENFILEPRODALL2(char *path, const char *s7, char runname[], char const* seednumber);
void GENFILEPRODALL1_en(char *path, const char *s8, char runname[], char const* seednumber);
void GENFILEPRODALL2_en(char *path, const char *s9, char runname[], char const* seednumber);
void GENFILEPRODALL1_ef(char *path, const char *s10, char runname[], char const* seednumber);
void GENFILEPRODALL2_ef(char *path, const char *s11, char runname[], char const* seednumber);
void GENFILENWALL1(char *path, const char *s12, char runname[], char const* seednumber);
void GENFILENWALL2(char *path, const char *s13, char runname[], char const* seednumber);
void GENFILENWALL3(char *path, const char *s14, char runname[], char const* seednumber);
void GENFILEDEBALL2(char *path, const char *s15, char runname[], char const* seednumber);
void GENFILEVALIDATION1(char *path, const char *s16, char const* seednumber);
void GENFILEVALIDATION2(char *path, const char *s17, char const* seednumber);
void GENFILEVALIDATION3(char *path, const char *s18, char const* seednumber);
void GENFILEVALIDATION4(char *path, const char *s19, char const* seednumber);
void GENFILEVALIDATION5(char *path, const char *s20, char const* seednumber);
void GENFILEVALIDATION6(char *path, const char *s21, char const* seednumber);
void GENFILEVALIDATION7(char *path, const char *s22, char const* seednumber);
void GENFILEVALIDATION8(char *path, const char *s23, char const* seednumber);
void GENFILEVALIDATION9(char *path, const char *s24, char const* seednumber);
void GENFILEVALIDATION10(char *path, const char *s25, char const* seednumber);
void GENFILEVALIDATION11(char *path, const char *s26, char const* seednumber);
void GENFILEVALIDATION12(char *path, const char *s27, char const* seednumber);
void GENFILESHOCKPARS(char *path, const char *s28,char runname[], char const* seednumber);

//  Writing output
void WRITEPROD(void);
void WRITEDEB(void);
void WRITENW(void);

#endif