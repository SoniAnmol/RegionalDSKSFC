#include "module_firm_relocation_sfc.h"

#include <algorithm>
#include <cmath>

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
    // ------------------------------------------------------------
    // Regional market opportunity
    // ------------------------------------------------------------
    //
    // C-firms:
    //   buyer-side regional consumption budget / number of C-firms.
    //
    // K-firms:
    //   buyer-side regional nominal machine investment / number of K-firms.
    //
    // The max(N_r, 1) denominator allows currently empty regions to
    // remain potential destinations without generating division by zero.

    const double national_market_C =
        (N2 > 0) ? Cons / static_cast<double>(N2) : 0.0;

    const double national_market_K =
        (N1 > 0) ? Investment_n / static_cast<double>(N1) : 0.0;

    for (int rr = 0; rr < NR; rr++)
    {
        // --------------------------------------------------------
        // C-sector market opportunity
        // --------------------------------------------------------
        double consumption_share =
            ((int)reg_cons_share.size() == NR)
                ? reg_cons_share[rr]
                : 1.0 / static_cast<double>(NR);

        if (consumption_share < 0.0)
        {
            consumption_share = 0.0;
        }

        const double regional_consumption_budget =
            Cons * consumption_share;

        const double number_C =
            std::max(reg_N2[rr], 1.0);

        market_potential_C_reloc[rr] =
            regional_consumption_budget / number_C;

        if (national_market_C > eps)
        {
            market_signal_C_reloc[rr] =
                std::log(
                    (market_potential_C_reloc[rr] + eps) /
                    (national_market_C + eps));
        }
        else
        {
            market_signal_C_reloc[rr] = 0.0;
        }

        // --------------------------------------------------------
        // K-sector market opportunity
        // --------------------------------------------------------
        const double regional_machine_demand =
            std::max(reg_Investment_n[rr], 0.0);

        const double number_K =
            std::max(reg_N1[rr], 1.0);

        market_potential_K_reloc[rr] =
            regional_machine_demand / number_K;

        if (national_market_K > eps)
        {
            market_signal_K_reloc[rr] =
                std::log(
                    (market_potential_K_reloc[rr] + eps) /
                    (national_market_K + eps));
        }
        else
        {
            market_signal_K_reloc[rr] = 0.0;
        }
    }
    // ------------------------------------------------------------
    // Preliminary regional economic attractiveness
    // ------------------------------------------------------------
    //
    // Profitability enters with coefficient 1.
    // Sector-specific market opportunity shifts regional attractiveness.
    //
    // Climate risk is intentionally excluded at this stage and will
    // enter as a separate negative component later.

    for (int rr = 0; rr < NR; rr++)
    {
        attractiveness_econ_K_reloc[rr] =
            rho_K_reloc_exp[rr] +
            beta_market_K_reloc * market_signal_K_reloc[rr];

        attractiveness_econ_C_reloc[rr] =
            rho_C_reloc_exp[rr] +
            beta_market_C_reloc * market_signal_C_reloc[rr];
    }
    // No relocation decision is made in this step.
}