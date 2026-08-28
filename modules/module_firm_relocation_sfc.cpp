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
    // ------------------------------------------------------------
    // Raw regional climate hazard
    // ------------------------------------------------------------
    //
    // The expected intensity of a Beta(a,b) shock is:
    //
    //     E[x] = a / (a + b)
    //
    // Only shock channels that generate region-specific firm damage
    // enter the relocation hazard index.
    //
    // Equal weights are used across active sector-relevant channels
    // in this first implementation.

    for (int rr = 0; rr < NR; rr++)
    {
        hazard_K_reloc[rr] = 0.0;
        hazard_C_reloc[rr] = 0.0;
    }

    if (flag_exogenousshocks == 0)
    {
        auto expected_beta_shock = [&](int channel, int rr) -> double
        {
            const double a = X_a_reg(channel, rr + 1);
            const double b = X_b_reg(channel, rr + 1);
            const double denom = a + b;

            if (a < 0.0 || b < 0.0 || denom <= eps)
            {
                return 0.0;
            }

            return a / denom;
        };

        for (int rr = 0; rr < NR; rr++)
        {
            double hazard_sum_K = 0.0;
            double hazard_sum_C = 0.0;

            int active_channels_K = 0;
            int active_channels_C = 0;

            // --------------------------------------------------------
            // K-firm regional climate exposure
            // --------------------------------------------------------

            // Channel 1: machine/process productivity
            if (flag_prodshocks1 > 0)
            {
                hazard_sum_K += expected_beta_shock(1, rr);
                active_channels_K++;
            }

            // Channel 2: labour productivity / energy efficiency
            if (flag_prodshocks2 > 0)
            {
                const double h = expected_beta_shock(2, rr);

                hazard_sum_K += h;
                hazard_sum_C += h;

                active_channels_K++;
                active_channels_C++;
            }

            // Channel 7: output shock.
            // Only flag==1 applies the hazard region-specifically.
            if (flag_outputshocks == 1)
            {
                const double h = expected_beta_shock(7, rr);

                hazard_sum_K += h;
                hazard_sum_C += h;

                active_channels_K++;
                active_channels_C++;
            }

            // Channel 9: K-firm R&D effectiveness
            if (flag_RDshocks > 0)
            {
                hazard_sum_K += expected_beta_shock(9, rr);
                active_channels_K++;
            }

            // --------------------------------------------------------
            // C-firm regional climate exposure
            // --------------------------------------------------------

            // Channel 6: physical capital destruction.
            // Only flag==1 applies the hazard region-specifically.
            if (flag_capshocks == 1)
            {
                hazard_sum_C += expected_beta_shock(6, rr);
                active_channels_C++;
            }

            // Channel 8: inventory destruction.
            // Only flag==1 applies the hazard region-specifically.
            if (flag_inventshocks == 1)
            {
                hazard_sum_C += expected_beta_shock(8, rr);
                active_channels_C++;
            }

            // Equal-weight mean across active relevant channels.
            hazard_K_reloc[rr] =
                (active_channels_K > 0)
                    ? hazard_sum_K / static_cast<double>(active_channels_K)
                    : 0.0;

            hazard_C_reloc[rr] =
                (active_channels_C > 0)
                    ? hazard_sum_C / static_cast<double>(active_channels_C)
                    : 0.0;
        }
    }
    // ------------------------------------------------------------
    // Residual regional climate risk after adaptation
    // ------------------------------------------------------------
    //
    // Firms assess regional climate risk as expected physical hazard
    // conditional on the protection already present in the region:
    //
    //     climate risk = hazard * adaptation damage multiplier
    //
    // Omega_adapt_rg = 1 implies no protection.
    // Lower Omega_adapt_rg implies stronger regional protection.

    for (int rr = 0; rr < NR; rr++)
    {
        double omega_adapt = 1.0;

        if ((int)Omega_adapt_rg.size() == NR)
        {
            omega_adapt =
                std::min(1.0, std::max(0.0, Omega_adapt_rg[rr]));
        }

        climate_risk_K_reloc[rr] =
            hazard_K_reloc[rr] * omega_adapt;

        climate_risk_C_reloc[rr] =
            hazard_C_reloc[rr] * omega_adapt;
    }
    // ------------------------------------------------------------
    // Final regional attractiveness including climate risk
    // ------------------------------------------------------------
    //
    // Regional attractiveness combines:
    //   1. expected profitability,
    //   2. sector-specific market opportunity,
    //   3. adaptation-adjusted climate risk.
    //
    // Higher residual climate risk lowers attractiveness.

    for (int rr = 0; rr < NR; rr++)
    {
        attractiveness_K_reloc[rr] =
            attractiveness_econ_K_reloc[rr] -
            beta_risk_K_reloc * climate_risk_K_reloc[rr];

        attractiveness_C_reloc[rr] =
            attractiveness_econ_C_reloc[rr] -
            beta_risk_C_reloc * climate_risk_C_reloc[rr];
    }
    // No relocation decision is made in this step.
}