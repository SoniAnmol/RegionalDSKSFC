#include "module_firm_relocation_sfc.h"

void FIRM_RELOCATION_COMPUTATION(void)
{
    if (NR <= 0)
    {
        return;
    }

    const double eps = 1e-12;

    // ------------------------------------------------------------
    // National sector profit margins
    // ------------------------------------------------------------
    // Used only to initialise expectations so that a region with
    // no current firms/sales is not mechanically assigned zero
    // expected profitability.

    double national_profit_K = 0.0;
    double national_sales_K = 0.0;
    double national_profit_C = 0.0;
    double national_sales_C = 0.0;

    for (int rr = 0; rr < NR; rr++)
    {
        national_profit_K += reg_Pitot1[rr];
        national_sales_K += reg_S1[rr];

        national_profit_C += reg_Pitot2[rr];
        national_sales_C += reg_S2[rr];
    }

    const double national_margin_K =
        (national_sales_K > eps)
            ? national_profit_K / national_sales_K
            : 0.0;

    const double national_margin_C =
        (national_sales_C > eps)
            ? national_profit_C / national_sales_C
            : 0.0;

    // ------------------------------------------------------------
    // Initialise regional expected profitability
    // ------------------------------------------------------------
    if (!profit_expectations_initialized_reloc)
    {
        for (int rr = 0; rr < NR; rr++)
        {
            rho_K_reloc_exp[rr] = national_margin_K;
            rho_C_reloc_exp[rr] = national_margin_C;
        }

        profit_expectations_initialized_reloc = true;
    }

    // ------------------------------------------------------------
    // Regional realised margins and adaptive expectations
    // ------------------------------------------------------------
    for (int rr = 0; rr < NR; rr++)
    {
        // ----- K-firms -----
        if (reg_S1[rr] > eps)
        {
            rho_K_reloc[rr] = reg_Pitot1[rr] / reg_S1[rr];

            rho_K_reloc_exp[rr] =
                (1.0 - lambda_profit_reloc) * rho_K_reloc_exp[rr] +
                lambda_profit_reloc * rho_K_reloc[rr];
        }
        else
        {
            // No informative regional observation this period.
            // Preserve the previous expected profitability.
            rho_K_reloc[rr] = rho_K_reloc_exp[rr];
        }

        // ----- C-firms -----
        if (reg_S2[rr] > eps)
        {
            rho_C_reloc[rr] = reg_Pitot2[rr] / reg_S2[rr];

            rho_C_reloc_exp[rr] =
                (1.0 - lambda_profit_reloc) * rho_C_reloc_exp[rr] +
                lambda_profit_reloc * rho_C_reloc[rr];
        }
        else
        {
            // No informative regional observation this period.
            // Preserve the previous expected profitability.
            rho_C_reloc[rr] = rho_C_reloc_exp[rr];
        }
    }

    // No relocation decision is made in this step.
}