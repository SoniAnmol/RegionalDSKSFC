#include "module_firm_relocation_sfc.h"
#include "../auxiliary/ran1.h"
#include <algorithm>
#include <cmath>
#include <limits>

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
    // ------------------------------------------------------------
    // Origin-region attractiveness gains and relocation inertia
    // ------------------------------------------------------------
    //
    // For each possible origin region, determine the largest
    // attractiveness improvement available elsewhere.
    //
    // This step identifies whether relocation is worth considering.
    // It does not choose a destination and consumes no random draws.

    for (int origin = 0; origin < NR; origin++)
    {
        double best_gain_K = 0.0;
        double best_gain_C = 0.0;

        for (int dest = 0; dest < NR; dest++)
        {
            if (dest == origin)
            {
                continue;
            }

            const double gain_K =
                attractiveness_K_reloc[dest] -
                attractiveness_K_reloc[origin];

            const double gain_C =
                attractiveness_C_reloc[dest] -
                attractiveness_C_reloc[origin];

            best_gain_K = std::max(best_gain_K, gain_K);
            best_gain_C = std::max(best_gain_C, gain_C);
        }

        relocation_gain_K_reloc[origin] = best_gain_K;
        relocation_gain_C_reloc[origin] = best_gain_C;

        relocation_eligible_K_reloc[origin] =
            (best_gain_K > tau_K_reloc) ? 1 : 0;

        relocation_eligible_C_reloc[origin] =
            (best_gain_C > tau_C_reloc) ? 1 : 0;
    }
    // ------------------------------------------------------------
    // Relocation willingness probability
    // ------------------------------------------------------------
    //
    // Conditional on the best available destination exceeding the
    // inertia threshold, relocation willingness rises smoothly with
    // the excess attractiveness gain.
    //
    // This step computes probabilities only. No random draws are made.

    for (int origin = 0; origin < NR; origin++)
    {
        relocation_prob_K_reloc[origin] = 0.0;
        relocation_prob_C_reloc[origin] = 0.0;

        if (relocation_eligible_K_reloc[origin] == 1)
        {
            const double excess_gain_K =
                relocation_gain_K_reloc[origin] - tau_K_reloc;

            relocation_prob_K_reloc[origin] =
                1.0 - std::exp(-gamma_K_reloc * excess_gain_K);
        }

        if (relocation_eligible_C_reloc[origin] == 1)
        {
            const double excess_gain_C =
                relocation_gain_C_reloc[origin] - tau_C_reloc;

            relocation_prob_C_reloc[origin] =
                1.0 - std::exp(-gamma_C_reloc * excess_gain_C);
        }
    }
    // ------------------------------------------------------------
    // Firm-specific production expenses and relocation feasibility
    // ------------------------------------------------------------
    //
    // A firm's current production expense is defined as its realised
    // wage bill plus its realised energy expenditure:
    //
    //     PE = wages + energy payments
    //
    // A firm may consider relocation financially feasible only if
    // current deposits can cover two production-expense equivalents.
    //
    // One production-expense equivalent will later become the realised
    // relocation cost for an actual mover. The second remains as a
    // post-relocation operating-liquidity buffer.
    //
    // This step computes feasibility only. No deposits are changed.

    // ----- K-firms -----
    for (int ii = 0; ii < N1; ii++)
    {
        const int firm = ii + 1;

        const double wages =
            std::max(0.0, Wages_1(firm));

        const double energy =
            std::max(0.0, EnergyPayments_1(firm));

        production_expense_K_reloc[ii] =
            wages + energy;

        const double deposits =
            std::max(0.0, Deposits_1(1, firm));

        // Firms that exited during the current period are not allowed
        // to make a relocation decision.
        //
        // A strictly positive production expense is required so that
        // inactive firms cannot relocate at zero cost.
        relocation_feasible_K_reloc[ii] =
            (exiting_1(firm) == 0.0 &&
             production_expense_K_reloc[ii] > eps &&
             deposits + eps >=
                 2.0 * production_expense_K_reloc[ii])
                ? 1
                : 0;
    }

    // ----- C-firms -----
    for (int jj = 0; jj < N2; jj++)
    {
        const int firm = jj + 1;

        const double wages =
            std::max(0.0, Wages_2(firm));

        const double energy =
            std::max(0.0, EnergyPayments_2(firm));

        production_expense_C_reloc[jj] =
            wages + energy;

        const double deposits =
            std::max(0.0, Deposits_2(1, firm));

        // Firms that exited during the current period are not allowed
        // to make a relocation decision.
        //
        // A strictly positive production expense is required so that
        // inactive firms cannot relocate at zero cost.
        relocation_feasible_C_reloc[jj] =
            (exiting_2(firm) == 0.0 &&
             production_expense_C_reloc[jj] > eps &&
             deposits + eps >=
                 2.0 * production_expense_C_reloc[jj])
                ? 1
                : 0;
    }
    // ------------------------------------------------------------
    // Probabilistic destination choice
    // ------------------------------------------------------------
    //
    // For each origin region, construct a softmax distribution over
    // alternative regions whose attractiveness exceeds the origin by
    // more than the sector-specific relocation threshold.
    //
    // Probabilities are computed only. No random destination draw is made.

    for (int origin = 0; origin < NR; origin++)
    {
        // Reset probability rows.
        for (int dest = 0; dest < NR; dest++)
        {
            destination_prob_K_reloc[origin][dest] = 0.0;
            destination_prob_C_reloc[origin][dest] = 0.0;
        }

        // --------------------------------------------------------
        // K-firm destinations
        // --------------------------------------------------------
        if (relocation_eligible_K_reloc[origin] == 1)
        {
            double max_attractiveness_K =
                -std::numeric_limits<double>::infinity();

            // Find maximum attractiveness among acceptable destinations.
            for (int dest = 0; dest < NR; dest++)
            {
                if (dest == origin)
                {
                    continue;
                }

                const double gain =
                    attractiveness_K_reloc[dest] -
                    attractiveness_K_reloc[origin];

                if (gain > tau_K_reloc)
                {
                    max_attractiveness_K =
                        std::max(
                            max_attractiveness_K,
                            attractiveness_K_reloc[dest]);
                }
            }

            double denom_K = 0.0;

            for (int dest = 0; dest < NR; dest++)
            {
                if (dest == origin)
                {
                    continue;
                }

                const double gain =
                    attractiveness_K_reloc[dest] -
                    attractiveness_K_reloc[origin];

                if (gain > tau_K_reloc)
                {
                    const double weight =
                        std::exp(
                            eta_K_reloc *
                            (attractiveness_K_reloc[dest] -
                             max_attractiveness_K));

                    destination_prob_K_reloc[origin][dest] =
                        weight;

                    denom_K += weight;
                }
            }

            if (denom_K > eps)
            {
                for (int dest = 0; dest < NR; dest++)
                {
                    destination_prob_K_reloc[origin][dest] /=
                        denom_K;
                }
            }
        }

        // --------------------------------------------------------
        // C-firm destinations
        // --------------------------------------------------------
        if (relocation_eligible_C_reloc[origin] == 1)
        {
            double max_attractiveness_C =
                -std::numeric_limits<double>::infinity();

            // Find maximum attractiveness among acceptable destinations.
            for (int dest = 0; dest < NR; dest++)
            {
                if (dest == origin)
                {
                    continue;
                }

                const double gain =
                    attractiveness_C_reloc[dest] -
                    attractiveness_C_reloc[origin];

                if (gain > tau_C_reloc)
                {
                    max_attractiveness_C =
                        std::max(
                            max_attractiveness_C,
                            attractiveness_C_reloc[dest]);
                }
            }

            double denom_C = 0.0;

            for (int dest = 0; dest < NR; dest++)
            {
                if (dest == origin)
                {
                    continue;
                }

                const double gain =
                    attractiveness_C_reloc[dest] -
                    attractiveness_C_reloc[origin];

                if (gain > tau_C_reloc)
                {
                    const double weight =
                        std::exp(
                            eta_C_reloc *
                            (attractiveness_C_reloc[dest] -
                             max_attractiveness_C));

                    destination_prob_C_reloc[origin][dest] =
                        weight;

                    denom_C += weight;
                }
            }

            if (denom_C > eps)
            {
                for (int dest = 0; dest < NR; dest++)
                {
                    destination_prob_C_reloc[origin][dest] /=
                        denom_C;
                }
            }
        }
    }
    // ------------------------------------------------------------
    // Firm-level relocation decisions
    // ------------------------------------------------------------
    //
    // Firms act only if:
    //   1. their origin region is relocation-eligible;
    //   2. the individual firm is financially feasible;
    //   3. a Bernoulli draw is below the origin-specific
    //      relocation willingness probability.
    //
    // Conditional on relocation, the destination is drawn from the
    // origin-specific softmax probabilities computed above.
    //
    // Current locations are not changed here. Successful relocation
    // decisions are written only to the *_next location vectors and
    // therefore take effect at the beginning of the next period.
    //
    // No relocation expenditure is paid in this step.

    // Start from the current-period location configuration.
    // Non-movers therefore automatically retain their current region.
    region_firm_assignment_K_next = region_firm_assignment_K;
    region_firm_assignment_C_next = region_firm_assignment_C;

    // Reset realised-move indicators for this relocation evaluation.
    std::fill(relocated_K_reloc.begin(),
              relocated_K_reloc.end(), 0);

    std::fill(relocated_C_reloc.begin(),
              relocated_C_reloc.end(), 0);

    // ------------------------------------------------------------
    // K-firms
    // ------------------------------------------------------------
    for (int ii = 0; ii < N1; ii++)
    {
        const int origin_region =
            region_firm_assignment_K[ii];

        if (origin_region < 1 || origin_region > NR)
        {
            continue;
        }

        const int origin = origin_region - 1;

        // The firm must face a sufficiently attractive alternative
        // and must be financially able to relocate.
        if (relocation_eligible_K_reloc[origin] != 1 ||
            relocation_feasible_K_reloc[ii] != 1)
        {
            continue;
        }

        const double move_prob =
            relocation_prob_K_reloc[origin];

        if (move_prob <= 0.0)
        {
            continue;
        }

        // Check that a valid destination distribution exists before
        // consuming any random draw.
        double destination_sum = 0.0;

        for (int dest = 0; dest < NR; dest++)
        {
            destination_sum +=
                destination_prob_K_reloc[origin][dest];
        }

        if (destination_sum <= eps)
        {
            continue;
        }

        // Bernoulli relocation decision.
        const double move_draw = ran1(p_seed);

        if (move_draw >= move_prob)
        {
            continue;
        }

        // Conditional destination draw.
        const double destination_draw =
            ran1(p_seed) * destination_sum;

        double cumulative_prob = 0.0;
        int selected_dest = -1;
        int last_positive_dest = -1;

        for (int dest = 0; dest < NR; dest++)
        {
            const double p =
                destination_prob_K_reloc[origin][dest];

            if (p <= 0.0)
            {
                continue;
            }

            last_positive_dest = dest;
            cumulative_prob += p;

            if (destination_draw < cumulative_prob)
            {
                selected_dest = dest;
                break;
            }
        }

        // Numerical fallback in case of tiny floating-point
        // discrepancies in the cumulative distribution.
        if (selected_dest < 0)
        {
            selected_dest = last_positive_dest;
        }

        if (selected_dest >= 0 &&
            selected_dest != origin)
        {
            region_firm_assignment_K_next[ii] =
                selected_dest + 1;

            relocated_K_reloc[ii] = 1;
        }
    }

    // ------------------------------------------------------------
    // C-firms
    // ------------------------------------------------------------
    for (int jj = 0; jj < N2; jj++)
    {
        const int origin_region =
            region_firm_assignment_C[jj];

        if (origin_region < 1 || origin_region > NR)
        {
            continue;
        }

        const int origin = origin_region - 1;

        // The firm must face a sufficiently attractive alternative
        // and must be financially able to relocate.
        if (relocation_eligible_C_reloc[origin] != 1 ||
            relocation_feasible_C_reloc[jj] != 1)
        {
            continue;
        }

        const double move_prob =
            relocation_prob_C_reloc[origin];

        if (move_prob <= 0.0)
        {
            continue;
        }

        // Check that a valid destination distribution exists before
        // consuming any random draw.
        double destination_sum = 0.0;

        for (int dest = 0; dest < NR; dest++)
        {
            destination_sum +=
                destination_prob_C_reloc[origin][dest];
        }

        if (destination_sum <= eps)
        {
            continue;
        }

        // Bernoulli relocation decision.
        const double move_draw = ran1(p_seed);

        if (move_draw >= move_prob)
        {
            continue;
        }

        // Conditional destination draw.
        const double destination_draw =
            ran1(p_seed) * destination_sum;

        double cumulative_prob = 0.0;
        int selected_dest = -1;
        int last_positive_dest = -1;

        for (int dest = 0; dest < NR; dest++)
        {
            const double p =
                destination_prob_C_reloc[origin][dest];

            if (p <= 0.0)
            {
                continue;
            }

            last_positive_dest = dest;
            cumulative_prob += p;

            if (destination_draw < cumulative_prob)
            {
                selected_dest = dest;
                break;
            }
        }

        // Numerical fallback in case of tiny floating-point
        // discrepancies in the cumulative distribution.
        if (selected_dest < 0)
        {
            selected_dest = last_positive_dest;
        }

        if (selected_dest >= 0 &&
            selected_dest != origin)
        {
            region_firm_assignment_C_next[jj] =
                selected_dest + 1;

            relocated_C_reloc[jj] = 1;
        }
    }
}