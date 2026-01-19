#include "module_energy_sfc.h"

void EN_DEM(void)
{
    // Sum up energy demands coming from C-firms and K-firms
    D1_en_TOT = D1_en.Sum();
    D2_en_TOT = D2_en.Sum();

    D_en_TOT(1) = ROUND(D1_en_TOT + D2_en_TOT);

    // Compute regional energy demands
    if (NR > 0)
    {
        // Reset regional energy demands
        for (int rr = 0; rr < NR; ++rr)
        {
            reg_D1_en[rr] = 0;
            reg_D2_en[rr] = 0;
            reg_D_en_TOT[rr] = 0;
        }

        // Aggregate K-firm energy demand by region
        for (int ii = 1; ii <= N1; ++ii)
        {
            int rr = region_firm_assignment_K[ii - 1];
            if (rr >= 1 && rr <= NR)
            {
                reg_D1_en[rr - 1] += D1_en(ii);
            }
        }

        // Aggregate C-firm energy demand by region
        for (int jj = 1; jj <= N2; ++jj)
        {
            int rr = region_firm_assignment_C[jj - 1];
            if (rr >= 1 && rr <= NR)
            {
                reg_D2_en[rr - 1] += D2_en(jj);
            }
        }

        // Calculate total regional energy demand
        for (int rr = 0; rr < NR; ++rr)
        {
            reg_D_en_TOT[rr] = reg_D1_en[rr] + reg_D2_en[rr];
        }
    }
}

void ENERGY(void)
{
    // Aggregate energy demands first
    EN_DEM();

    double ge_before = G_ge(t);
    double de_before = G_de(t);

    // Initialise energy sector
    if (t == 1)
    {
        Q_ge = ROUND(K_ge0_perc * D_en_TOT(1));
        G_ge(t) = Q_ge;
        G_ge_n(t) = CF_ge(1) * G_ge(t);
        G_de(t) = D_en_TOT(1) - G_ge(t);
        C_de(t) = pf / A_de(t) + t_CO2_en * EM_de(t);
        K_gelag = G_ge(t);
        K_delag = G_de(t);
        G_de_0 = G_de(t);
        G_ge_0 = G_ge(t);
        G_ge_n_0 = G_ge_n(t);

        if (NR > 0)
        {
            for (int rr = 1; rr <= NR; ++rr)
            {
                const double dirty_share = region_energy_dirty_shares(rr);
                const double green_share = region_energy_green_shares(rr);
                region_dirty_capacity[rr - 1] = dirty_share * G_de(t);
                region_green_capacity[rr - 1] = green_share * G_ge(t);
            }

            // Compute regional Q_ge and Q_de (initialization)
            // Distribute national Q proportionally to regional capacities
            double total_green_cap = 0;
            double total_dirty_cap = 0;
            for (int rr = 0; rr < NR; ++rr)
            {
                total_green_cap += region_green_capacity[rr];
                total_dirty_cap += region_dirty_capacity[rr];
            }

            for (int rr = 0; rr < NR; ++rr)
            {
                if (total_green_cap > 0)
                    reg_Q_ge[rr] = Q_ge * (region_green_capacity[rr] / total_green_cap);
                else
                    reg_Q_ge[rr] = 0;

                if (total_dirty_cap > 0)
                    reg_Q_de[rr] = (D_en_TOT(1) - Q_ge) * (region_dirty_capacity[rr] / total_dirty_cap);
                else
                    reg_Q_de[rr] = 0;
            }
        }

        ge_before = G_ge(t);
        de_before = G_de(t);
    }

    // Determine existing productive capacity for energy
    K_ge = G_ge.Sum();
    K_de = G_de.Sum();

    // If existing capacity is insufficient to satisfy demand, expansion investment takes place
    if ((K_ge + K_de) <= D_en_TOT(1))
    {
        EI_en = D_en_TOT(1) - K_ge - K_de;

        c_de_min = C_de(1) * 100000;
        cf_min_ge = CF_ge(1) * 100000;

        // Determine best dirty & green technology
        for (tt = 1; tt <= t; tt++)
        {
            C_de(tt) = pf / A_de(tt) + t_CO2_en * EM_de(tt);
            if (C_de(tt) < c_de_min)
            {
                c_de_min = C_de(tt);
            }

            if (CF_ge(tt) < cf_min_ge)
            {
                cf_min_ge = CF_ge(tt);
            }
        }

        // If green investment is not constrained, make all investment green if green is superior
        if (flag_energy_exp == 0)
        {
            if (c_de_min * payback_en < cf_min_ge)
            {
                EI_en_de = EI_en;
                G_de(t) += EI_en_de;
            }
            else
            {
                EI_en_ge = EI_en;
                G_ge(t) += EI_en_ge;
                IC_en_quota(t) = EI_en * cf_min_ge / payback_en;
                G_ge_n(t) += EI_en_ge * cf_min_ge;
            }
        }
        // Otherwise, may be forced to also invest in dirty even when green is better
        else if (flag_energy_exp == 1)
        {
            if (c_de_min * payback_en > cf_min_ge)
            {
                if (EI_en < exp_quota * (exp_quota * K_gelag + (K_gelag - K_ge)))
                {
                    EI_en_ge = EI_en;
                    G_ge(t) += EI_en_ge;
                    IC_en_quota(t) = EI_en * cf_min_ge / payback_en;
                    G_ge_n(t) += EI_en_ge * cf_min_ge;
                }
                else
                {
                    EI_en_ge = exp_quota * (exp_quota * K_gelag + (K_gelag - K_ge));
                    G_ge(t) += EI_en_ge;
                    IC_en_quota(t) = EI_en_ge * cf_min_ge / payback_en;
                    G_ge_n(t) += EI_en_ge * cf_min_ge;
                    EI_en_de = EI_en - EI_en_ge;
                    G_de(t) += EI_en_de;
                }
            }
            else
            {
                EI_en_de = EI_en;
                G_de(t) += EI_en_de;
            }
        }
        // Case in which share of investment in green is fixed exogenously
        else if (flag_energy_exp == 2)
        {
            EI_en_ge = K_ge0_perc * EI_en;
            G_ge(t) += EI_en_ge;
            IC_en_quota(t) = EI_en_ge * cf_min_ge / payback_en;
            G_ge_n(t) += EI_en_ge * cf_min_ge;
            EI_en_de = EI_en - EI_en_ge;
            G_de(t) += EI_en_de;
        }
        else
        {
            if (flag_desc > 3 && t >= 200)
            {
                subsidy_ge_exp = ge_subsidy * cf_min_ge;
            }
            else
            {
                subsidy_ge_exp = 0;
            }

            if (flag_energy_exp == 4)
            {
                exp_quota = max(exp_quota_0, tanh(((c_de_min * payback_en - (cf_min_ge - subsidy_ge_exp)) / (cf_min_ge - subsidy_ge_exp)) / exp_quota_param));
            }

            if (c_de_min * payback_en > (cf_min_ge - subsidy_ge_exp))
            {
                if (EI_en < (exp_quota * K_gelag + (K_gelag - K_ge)) || (K_delag / (K_gelag + K_delag)) <= (exp_quota))
                {
                    EI_en_ge = EI_en;
                    G_ge(t) += EI_en_ge;
                    IC_en_quota(t) = EI_en * cf_min_ge / payback_en;
                    Subsidy_Exp = EI_en * subsidy_ge_exp;
                    G_ge_n(t) += EI_en_ge * cf_min_ge;
                }
                else
                {
                    EI_en_ge = exp_quota * K_gelag + (K_gelag - K_ge);
                    G_ge(t) += EI_en_ge;
                    IC_en_quota(t) = EI_en_ge * cf_min_ge / payback_en;
                    Subsidy_Exp = EI_en_ge * subsidy_ge_exp;
                    G_ge_n(t) += EI_en_ge * cf_min_ge;
                    EI_en_de = EI_en - EI_en_ge;
                    G_de(t) += EI_en_de;
                }
            }
            else
            {
                EI_en_ge = K_ge0_perc * EI_en;
                G_ge(t) += EI_en_ge;
                IC_en_quota(t) = EI_en_ge * cf_min_ge / payback_en;
                G_ge_n(t) += EI_en_ge * cf_min_ge;
                EI_en_de = EI_en - EI_en_ge;
                G_de(t) += EI_en_de;
            }
        }

        const double ge_added = G_ge(t) - ge_before;
        const double de_added = G_de(t) - de_before;
        if (NR > 0)
        {
            if (ge_added > 0)
            {
                for (int rr = 1; rr <= NR; ++rr)
                {
                    region_green_capacity[rr - 1] += ge_added * ge_growth_probability(rr);
                }
            }
            if (de_added > 0)
            {
                for (int rr = 1; rr <= NR; ++rr)
                {
                    region_dirty_capacity[rr - 1] += de_added * de_growth_probability(rr);
                }
            }
        }

        // New capacity
        K_ge = G_ge.Sum();
        K_de = G_de.Sum();
    }

    // Calculate ammortised investment cost from expansion of green energy
    for (tt = 1; tt <= t; tt++)
    {
        if (t - tt <= payback_en && G_ge(tt) > 0)
        {
            IC_en += IC_en_quota(tt);
        }
    }

    // Calculate associated labour demand
    LDexp_en = IC_en / w(2);

    // Quantity of dirty energy is the residual
    Q_de = D_en_TOT(1) - K_ge;

    // Only produce dirty energy if green is insufficient
    if (Q_de <= 0)
    {
        Q_de = 0;
        Q_ge = D_en_TOT(1);

        // Compute regional Q_ge and Q_de (green sufficient case)
        // Distribute national Q proportionally to regional capacities
        if (NR > 0)
        {
            double total_green_cap = 0;
            for (int rr = 0; rr < NR; ++rr)
                total_green_cap += region_green_capacity[rr];

            for (int rr = 0; rr < NR; ++rr)
            {
                if (total_green_cap > 0)
                    reg_Q_ge[rr] = Q_ge * (region_green_capacity[rr] / total_green_cap);
                else
                    reg_Q_ge[rr] = 0;
                reg_Q_de[rr] = 0; // No dirty energy needed
            }
        }

        // constant marginal cost for green energy
        c_infra = 0;
        // Energy mark-up shock
        if ((flag_energyshocks == 1 && t == 245) || (flag_energyshocks == 2 && t == 245))
        {
            mi_en_preshock = mi_en;
            mi_en = 2 * mi_en;
            mi_en_shock = mi_en;
        }
        if ((flag_energyshocks == 5 && t == 245))
        {
            mi_en_preshock = mi_en;
            mi_en_shock = 2 * mi_en;
            mi_en += 0.2 * (mi_en_shock - mi_en_preshock);
        }
        if ((flag_energyshocks == 5 && t > 245 && t < 250))
        {
            mi_en += 0.2 * (mi_en_shock - mi_en_preshock);
        }

        c_en(1) = mi_en;
        Emiss_en = 0;
    }
    else
    {
        Q_ge = K_ge;

        // Compute regional Q_ge and Q_de (dirty needed case)
        // Distribute national Q proportionally to regional capacities
        if (NR > 0)
        {
            double total_green_cap = 0;
            double total_dirty_cap = 0;
            for (int rr = 0; rr < NR; ++rr)
            {
                total_green_cap += region_green_capacity[rr];
                total_dirty_cap += region_dirty_capacity[rr];
            }

            for (int rr = 0; rr < NR; ++rr)
            {
                if (total_green_cap > 0)
                    reg_Q_ge[rr] = Q_ge * (region_green_capacity[rr] / total_green_cap);
                else
                    reg_Q_ge[rr] = 0;

                if (total_dirty_cap > 0)
                    reg_Q_de[rr] = Q_de * (region_dirty_capacity[rr] / total_dirty_cap);
                else
                    reg_Q_de[rr] = 0;
            }
        }

        for (tt = 1; tt <= t; tt++)
        {
            C_de(tt) = pf / A_de(tt) + t_CO2_en * EM_de(tt);
        }
        G_de_temp = G_de;
        Q_de_temp = Q_de;

        // If dirty energy is needed, successively activate dirty plants, starting with the most efficient
        while (Q_de_temp > 0)
        {
            c_de_min = C_de(1) * 10;
            idmin = 1;
            for (tt = 1; tt <= t; tt++)
            {
                if (G_de_temp(tt) > 0)
                {
                    if (C_de(tt) <= c_de_min)
                    {
                        idmin = tt;
                        c_de_min = C_de(idmin);
                    }
                }
            }

            if (Q_de_temp > G_de_temp(idmin))
            {
                PC_en += G_de(idmin) * C_de(idmin);
                Emiss_en += G_de(idmin) * EM_de(idmin);
                Q_de_temp -= G_de_temp(idmin);
                FuelCost += G_de(idmin) * pf / A_de(idmin);
            }
            else
            {
                PC_en += Q_de_temp * C_de(idmin);
                Emiss_en += Q_de_temp * EM_de(idmin);
                FuelCost += Q_de_temp * pf / A_de(idmin);
                Q_de_temp = 0;
            }
            G_de_temp(idmin) = 0;
            if (Q_de_temp < tolerance)
            {
                Q_de_temp = 0;
            }
        }

        // Increasing marginal cost in dirty energy
        c_infra = c_de_min;
        // Fossil fuel price shock
        if ((flag_energyshocks == 3 && t == 245) || (flag_energyshocks == 4 && t == 245))
        {
            c_en_preshock = c_infra + mi_en;
            pf_preshock = pf;
            c_infra_t = 2 * c_en(2) - mi_en;
            i = 0;
            while (c_infra < c_infra_t)
            {
                i = i + 1;
                pf = pf_preshock * pow((1.01), i);
                for (tt = 1; tt <= t; tt++)
                {
                    C_de(tt) = pf / A_de(tt) + t_CO2_en * EM_de(tt);
                }
                G_de_temp = G_de;
                Q_de_temp = Q_de;
                PC_en = 0;
                Emiss_en = 0;
                FuelCost = 0;

                while (Q_de_temp > 0)
                {
                    c_de_min = C_de(1) * 10;
                    idmin = 1;
                    for (tt = 1; tt <= t; tt++)
                    {
                        if (G_de_temp(tt) > 0)
                        {
                            if (C_de(tt) <= c_de_min)
                            {
                                idmin = tt;
                                c_de_min = C_de(idmin);
                            }
                        }
                    }

                    if (Q_de_temp > G_de_temp(idmin))
                    {
                        PC_en += G_de(idmin) * C_de(idmin);
                        Emiss_en += G_de(idmin) * EM_de(idmin);
                        Q_de_temp -= G_de_temp(idmin);
                        FuelCost += G_de(idmin) * pf / A_de(idmin);
                    }
                    else
                    {
                        PC_en += Q_de_temp * C_de(idmin);
                        Emiss_en += Q_de_temp * EM_de(idmin);
                        FuelCost += Q_de_temp * pf / A_de(idmin);
                        Q_de_temp = 0;
                    }
                    G_de_temp(idmin) = 0;
                    if (Q_de_temp < tolerance)
                    {
                        Q_de_temp = 0;
                    }
                }
                c_infra = c_de_min;
            }
            pf_shock = pf;
        }

        // Energy mark-up shock
        if ((flag_energyshocks == 1 && t == 245) || (flag_energyshocks == 2 && t == 245))
        {
            c_en_preshock = c_infra + mi_en;
            mi_en_preshock = mi_en;
            mi_en = 2 * (mi_en + c_infra) - c_infra;
            mi_en_shock = mi_en;
        }

        if ((flag_energyshocks == 5 && t == 245))
        {
            mi_en_preshock = mi_en;
            mi_en_shock = 1.1657689 * (mi_en_preshock + c_infra) - c_infra;
            mi_en = mi_en_shock;
        }
        if ((flag_energyshocks == 5 && t == 246))
        {
            mi_en_shock = 1.3315378 * (mi_en_preshock + c_infra) - c_infra;
            mi_en = mi_en_shock;
        }
        if ((flag_energyshocks == 5 && t == 247))
        {
            mi_en_shock = 1.4973331 * (mi_en_preshock + c_infra) - c_infra;
            mi_en = mi_en_shock;
        }
        if ((flag_energyshocks == 5 && t == 248))
        {
            mi_en_shock = 1.6631020 * (mi_en_preshock + c_infra) - c_infra;
            mi_en = mi_en_shock;
        }
        if ((flag_energyshocks == 5 && t == 249))
        {
            mi_en_shock = 1.8288973 * (mi_en_preshock + c_infra) - c_infra;
            mi_en = mi_en_shock;
        }
        if ((flag_energyshocks == 5 && t == 250))
        {
            mi_en_shock = 1.7731569 * (mi_en_preshock + c_infra) - c_infra;
            mi_en = mi_en_shock;
        }
        if ((flag_energyshocks == 5 && t == 251))
        {
            mi_en_shock = 1.7174166 * (mi_en_preshock + c_infra) - c_infra;
            mi_en = mi_en_shock;
        }
        if ((flag_energyshocks == 5 && t == 252))
        {
            mi_en_shock = 1.6616762 * (mi_en_preshock + c_infra) - c_infra;
            mi_en = mi_en_shock;
        }
        if ((flag_energyshocks == 5 && t == 253))
        {
            mi_en_shock = 1.6059358 * (mi_en_preshock + c_infra) - c_infra;
            mi_en = mi_en_shock;
        }
        if ((flag_energyshocks == 5 && t == 254))
        {
            mi_en_shock = 1.5502218 * (mi_en_preshock + c_infra) - c_infra;
            mi_en = mi_en_shock;
        }
        if ((flag_energyshocks == 5 && t == 255))
        {
            mi_en_shock = 1.4532108 * (mi_en_preshock + c_infra) - c_infra;
            mi_en = mi_en_shock;
        }
        if ((flag_energyshocks == 5 && t == 256))
        {
            mi_en_shock = 1.3562262 * (mi_en_preshock + c_infra) - c_infra;
            mi_en = mi_en_shock;
        }
        if ((flag_energyshocks == 5 && t == 257))
        {
            mi_en_shock = 1.2592417 * (mi_en_preshock + c_infra) - c_infra;
            mi_en = mi_en_shock;
        }
        if ((flag_energyshocks == 5 && t == 258))
        {
            mi_en_shock = 1.1622571 * (mi_en_preshock + c_infra) - c_infra;
            mi_en = mi_en_shock;
        }
        if ((flag_energyshocks == 5 && t == 259))
        {
            mi_en_shock = 1.0652725 * (mi_en_preshock + c_infra) - c_infra;
            mi_en = mi_en_shock;
        }
        if ((flag_energyshocks == 5 && t == 260))
        {
            mi_en_shock = 1 * (mi_en_preshock + c_infra) - c_infra;
            mi_en = mi_en_shock;
        }

        c_en(1) = c_infra + mi_en;
    }

    if (flag_share_END == 1)
    {
        share_de = K_de / (K_de + K_ge);
    }
    else if (flag_share_END == 2)
    {
        share_de = 1 - Q_ge / D_en_TOT(1);
    }
    else
    {
        share_de = share_de_0;
    }

    // Determine energy sector revenue & R&D expenditures
    Rev_en = c_en(1) * D_en_TOT(1);

    // Divide between clean and dirty
    if (Rev_en * share_RD_en < Rev_en - IC_en - PC_en)
    {
        RD_en_de = share_RD_en * share_de * Rev_en;
        RD_en_ge = share_RD_en * (1 - share_de) * Rev_en;
    }
    else
    {
        RD_en_de = share_de * (Rev_en - PC_en - IC_en);
        RD_en_ge = (1 - share_de) * (Rev_en - PC_en - IC_en);
    }

    // Compute labour demand for R&D
    LDrd_de = RD_en_de / w(2);
    LDrd_ge = RD_en_ge / w(2);

    parber_en_de = 1 - exp(-o1_en * RD_en_de);
    Inn_en_de = bnldev(parber_en_de, 1, p_seed);

    if (flag_desc == 5 && t >= 200)
    {
        Subsidy_Exp += (RD_en_ge + RD_en_de);
        LDrd_ge = (2 * RD_en_ge + RD_en_de) / w(2);
        parber_en_ge = 1 - exp(-o1_en * (RD_en_ge));
        Inn_en_ge = bnldev(parber_en_ge, 1, p_seed);
        parber_en_ge2 = 1 - exp(-o1_en * (RD_en_ge + RD_en_de));
        Inn_en_ge2 = bnldev(parber_en_ge, 1, p_seed);
        if (Inn_en_ge2 == 1)
        {
            uu1_en_g = uu1_en_g + 0.0025;
            uu2_en_g = uu1_en_g + 0.0025;
        }
    }
    else
    {
        parber_en_ge = 1 - exp(-o1_en * RD_en_ge);
        Inn_en_ge = bnldev(parber_en_ge, 1, p_seed);
        uu1_en_g = 0;
        uu2_en_g = 0;
    }

    // If innovation takes place, update technologies
    if (Inn_en_de == 1)
    {
        rnd = betadev(b_a11, b_b11, p_seed);
        rnd = uu1_en + rnd * (uu2_en - uu1_en);
        A_de_inn = A_de(t) * (1 + rnd);

        if (A_de_inn > 1)
        {
            A_de_inn = 1;
        }

        rnd = betadev(b_a11, b_b11, p_seed);
        rnd = uu1_en + rnd * (uu2_en - uu1_en);
        EM_de_inn = EM_de(t) * (1 - rnd);

        if (EM_de_inn < 0)
        {
            EM_de_inn = 0;
        }
    }
    else
    {
        A_de_inn = A_de(t);
        EM_de_inn = EM_de(t);
    }

    if (Inn_en_ge == 1 && CF_ge(t) > 0)
    {
        rnd = betadev(b_a11, b_b11, p_seed);
        rnd = (uu1_en + uu1_en_g) + rnd * ((uu2_en + uu2_en_g) - (uu1_en + uu1_en_g));
        CF_ge_inn = CF_ge(t) * (1 - rnd);

        if (CF_ge_inn < 0)
        {
            CF_ge_inn = 0;
        }
    }
    else
    {
        CF_ge_inn = CF_ge(t);
    }

    K_gelag = max(K_ge, (K_ge + K_de) * K_ge0_perc);
    K_delag = K_de;

    if (t <= life_plant)
    {
        G_de(1) -= G_de_0 / life_plant;
        G_ge(1) -= G_ge_0 / life_plant;
        G_ge_n(1) -= G_ge_n_0 / life_plant;
    }

    // Update technology vectors
    if (t < T)
    {
        if (pf / A_de_inn + t_CO2_en * EM_de_inn < pf / A_de(t) + t_CO2_en * EM_de(t))
        {
            A_de(t + 1) = A_de_inn;
            EM_de(t + 1) = EM_de_inn;
            C_de(t + 1) = pf / A_de_inn + t_CO2_en * EM_de_inn;
        }
        else
        {
            A_de(t + 1) = A_de(t);
            EM_de(t + 1) = EM_de(t);
            C_de(t + 1) = pf / A_de(t) + t_CO2_en * EM_de(t);
        }

        if (CF_ge_inn < CF_ge(t))
        {
            CF_ge(t + 1) = CF_ge_inn;
        }
        else
        {
            CF_ge(t + 1) = CF_ge(t);
        }

        for (tt = 1; tt <= t; tt++)
        {
            if (t - tt > life_plant)
            {
                G_de(tt) = 0;
                G_ge(tt) = 0;
                G_ge_n(tt) = 0;
            }
        }
    }

    // Update nominal value of Energy capital stock; Energy capital valued at production cost --> 0 for dirty
    CapitalStock_e(1) = G_ge_n.Sum();

    // Calculate total emissions
    Emiss_TOT(1) = Emiss_en + Emiss2_TOT + Emiss1_TOT;

    // Calculate total labour demand & wages to be paid by energy sector (paid in next period)
    LDentot = LDexp_en + LDrd_de + LDrd_ge;
    Wages_en = w(2) * LDentot;
}

void EMISS_IND(void)
{
    // Calculate total emissions from C-firms and K-firms
    for (i = 1; i <= N1; i++)
    {
        Emiss1(i) = A1p_ef(i) / A1p_en(i) * Q1(i);
    }

    for (j = 1; j <= N2; j++)
    {
        Emiss2(j) = A2e_ef(j) / A2e_en(j) * Q2(j);
    }

    Emiss1_TOT = Emiss1.Sum();
    Emiss2_TOT = Emiss2.Sum();
}