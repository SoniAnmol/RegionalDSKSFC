#include "module_climate_sfc.h"

void CLIMATE_POLICY(void)
{
    if (flag_tax_CO2 == 1)
    {
        // Constant tax increasing with inflation
        t_CO2 = cpi(2) / cpi_init * t_CO2_0;
        t_CO2_en = cpi(2) / cpi_init * t_CO2_en_0;
    }

    if (flag_tax_CO2 == 2)
    {
        // linear increase as function of time
        t_CO2 = t_CO2_0 + (t - t_start_climbox) / tc1;
        t_CO2_en = t_CO2_en_0 + (t - t_start_climbox) / tc1;
    }

    if (flag_tax_CO2 == 3)
    {
        // Exponential increase + inflation correction
        t_CO2 = cpi(2) / cpi_init * (t_CO2_0 * pow(tc2, (t - t_start_climbox)));
        t_CO2_en = cpi(2) / cpi_init * (t_CO2_en_0 * pow(tc2, (t - t_start_climbox)));
    }

    if (flag_tax_CO2 == 4)
    {
        // Increasing with nominal GDP
        t_CO2 = GDP_n(2) / GDP_init * t_CO2_0;
        if (flag_desc == 1 && t >= 200)
        {
            if (t == 200)
            {
                GDP_init = GDP_n(2);
                t_CO2_en_0 = t_CO2_en * 2;
                t_CO2_en = GDP_n(2) / GDP_init * t_CO2_en_0;
            }
            else if (t > 200 && t <= 212)
            {
                t_CO2_en_0 = t_CO2_en_0 * 2;
                t_CO2_en = GDP_n(2) / GDP_init * t_CO2_en_0;
            }
            else
            {
                t_CO2_en = GDP_n(2) / GDP_init * t_CO2_en_0;
            }
        }
        else if (flag_desc == 4 && t >= 200)
        {
            if (t == 200)
            {
                GDP_init = GDP_n(2);
                t_CO2_en_0 = t_CO2_en * 1.5;
                t_CO2_en = GDP_n(2) / GDP_init * t_CO2_en_0;
            }
            else if (t > 200 && t <= 212)
            {
                t_CO2_en_0 = t_CO2_en_0 * 1.5;
                t_CO2_en = GDP_n(2) / GDP_init * t_CO2_en_0;
            }
            else
            {
                t_CO2_en = GDP_n(2) / GDP_init * t_CO2_en_0;
            }
        }
        else
        {
            t_CO2_en = GDP_n(2) / GDP_init * t_CO2_en_0;
        }
    }

    t_CO2 = max(t_CO2, 0.0);
    t_CO2_en = max(t_CO2_en, 0.0);
}

void CLIMATEBOX(void)
{
    // C-Roads climate box

    // Fix the equilibrium pre-industrial carbon concentrations in each ocean layer
    for (j = 1; j <= ndep; j++)
    {
        Con0(j) = Con00 * laydep(j);
    }

    // Get emissions
    Emiss_yearly(1) = 0;
    Emiss_yearly(2) = 0;
    for (tt = 1; tt <= freqclim; tt++)
    {
        Emiss_yearly(1) += Emiss_TOT(tt);
        Emiss_yearly(2) += Emiss_TOT(tt + freqclim);
    }

    // During first run of climate box, initialise calib. emissions
    if (t > t_start_climbox && t < (t_start_climbox + freqclim + 1))
    {
        Emiss_yearly_calib(2) = Emiss_yearly_0;
        Emiss_yearly_calib(1) = Emiss_yearly_0;
        Emiss_gauge = Emiss_yearly(1);
        Emiss_global = Emiss_gauge / emiss_share - Emiss_gauge;
        Emiss_gauge += Emiss_global;
    }

    Emiss_global = Emiss_global * g_emiss_global;
    Emiss_yearly(1) += Emiss_global;

    // Calculate new annual industrial emissions
    g_rate_em_y = Emiss_yearly(1) / Emiss_gauge;
    Emiss_yearly_calib(1) = Emiss_yearly_0 * g_rate_em_y;

    // Atmosphere-Biosphere carbon exchange (+emissions)
    // Atmospheric carbon taken up by plants
    NPP = NPP0 * (1 + fertil * log(Cat(2) / Cat0)) * (1 + heatstress * Tmixed(2));
    // Carbon released into atmosphere by decaying humus
    humrelease = hum(2) / humtime;
    // Carbon released into atmosphere by rotting plants
    biorelease = biom(2) / biotime * (1 - humfrac);

    // Exclude ocean uptake which is computed below
    dCat1 = Emiss_yearly_calib(1) + humrelease + biorelease - NPP;
    // Initial guess for Cat (excluding ocean uptake)
    Cat1 = Cat(2) + dCat1;

    // Update biomass carbon
    biom(1) = biom(2) + NPP - biom(2) / biotime;
    // Update humus carbon
    hum(1) = hum(2) + biom(2) / biotime * humfrac - hum(2) / humtime;

    // Carbon mixing between ocean layers
    // Carbon flux between layers through diffusion
    for (j = 1; j <= ndep - 1; j++)
    {
        fluxC(j) = eddydif * (Con(2, j) / laydep(j) - Con(2, j + 1) / laydep(j + 1)) / ((laydep(j + 1) + laydep(j)) / 2);
    }

    // Update carbon content in layers
    // Preliminary carbon content of mixed layer (before exchange with atmosphere)
    Con1 = Con(2, 1) - fluxC(1);

    for (j = 2; j <= ndep - 1; j++)
    {
        Con(1, j) = Con(2, j) + fluxC(j - 1) - fluxC(j);
    }

    Con(1, ndep) = Con(2, ndep) + fluxC(ndep - 1);

    // Ocean-atmposphere carbon exchange
    // Total carbon in atmosphere and upper ocean layer
    Ctot1 = Con1 + Cat1;

    // Re-distribute carbon between upper ocean layer and atmosphere iteratively until an equilibrium is reached.
    // Start from an initial guess for Cat which implies a value for Cay.
    // The slope of the curve (Caa) is estimated by using two additional nearby x and y values (Caxx and Caay).

    // Initial guess for Cat
    Cax(1) = Cat1;
    Cay(1) = Ctot1 - Cax(1) - Conref * (1 - ConrefT * Tmixed(2)) * pow((Cax(1) / Cat0), 1 / (rev0 + revC * log(Cax(1) / Cat0)));
    // Nearby value for Cat
    Caxx(1) = Cax(1) + 1.5 * Cay(1);
    Cayy(1) = Ctot1 - Caxx(1) - Conref * (1 - ConrefT * Tmixed(2)) * pow((Caxx(1) / Cat0), 1 / (rev0 + revC * log(Caxx(1) / Cat0)));
    // Initial guess for gradient
    Caa(1) = (Cayy(1) - Cay(1)) / (Caxx(1) - Cax(1));

    i = 1;
    do
    {
        Cax(i + 1) = Cax(i) - Cay(i) / Caa(i);
        Cay(i + 1) = Ctot1 - Cax(i + 1) - Conref * (1 - ConrefT * Tmixed(2)) * pow((Cax(i + 1) / Cat0), 1 / (rev0 + revC * log(Cax(i + 1) / Cat0)));
        if (abs(Cay(i + 1)) < 1e-10)
        {
            Cax(niterclim) = Cax(i + 1);
            break;
        }
        Caxx(i + 1) = Cax(i + 1) - 2 * Cay(i) / Caa(i);
        Cayy(i + 1) = Ctot1 - Caxx(i + 1) - Conref * (1 - ConrefT * Tmixed(2)) * pow((Caxx(i + 1) / Cat0), 1 / (rev0 + revC * log(Caxx(i + 1) / Cat0)));
        Caa(i + 1) = (Cayy(i + 1) - Cay(i + 1)) / (Caxx(i + 1) - Cax(i + 1));
        i++;
    } while (i <= niterclim - 1);

    // Final value for atmospheric carbon
    Cat(1) = Cax(niterclim);
    // Final value for carbon in upper ocean layer
    Con(1, 1) = Ctot1 - Cat(1);

    // Radiation and temperature change

    // Compute radiative forcing
    FCO2 = forCO2 * log(Cat(1) / Cat0);

    // No non-CO2 forcing
    if (flag_nonCO2_force == 0)
    {
        Fin = FCO2;
    }

    // Non-CO2 forcing taken into account in a rough way
    if (flag_nonCO2_force == 1)
    {
        Fin = FCO2 * otherforcefac;
    }

    // Extra outradiated energy due to global warming
    Fout = outrad * Tmixed(2);

    // Ocean mixing
    // Carbon flux between layers
    for (j = 1; j <= ndep - 1; j++)
    {
        fluxH(j) = eddydif * (Hon(2, j) / laydep(j) - Hon(2, j + 1) / laydep(j + 1)) / ((laydep(j + 1) + laydep(j)) / 2);
    }

    // Update heat content and temperature in layers (including radiative contribution to top layer)
    // Top layer heat content
    Hon(1, 1) = Hon(2, 1) - fluxH(1) + (Fin - Fout) * secyr / seasurf;
    // Top layer temperature w.r.t pre-industrial value
    Ton(1, 1) = Hon(1, 1) / laydep(1) / heatcap;
    //"Surface" temperature is that of the mixed layer
    Tmixed(1) = Ton(1, 1);

    for (j = 2; j <= ndep - 1; j++)
    {
        // Middle layer heat content
        Hon(1, j) = Hon(2, j) + fluxH(j - 1) - fluxH(j);
        // Middle layer temperature w.r.t pre-industrial value
        Ton(1, j) = Hon(1, j) / laydep(j) / heatcap;
    }

    // Bottom layer heat content
    Hon(1, ndep) = Hon(2, ndep) + fluxH(ndep - 1);
    // Middle layer temperature w.r.t pre-industrial value
    Ton(1, ndep) = Hon(1, ndep) / laydep(ndep) / heatcap;
}

void CLIMATEBOX_CUM_EMISS(void)
{
    // Simple climate box based on cumulative emissions

    Emiss_yearly(1) = 0;
    Emiss_yearly(2) = 0;
    for (tt = 1; tt <= freqclim; tt++)
    {
        Emiss_yearly(1) += Emiss_TOT(tt);
        Emiss_yearly(2) += Emiss_TOT(tt + freqclim);
    }

    if (t > t_start_climbox && t < (t_start_climbox + freqclim + 1))
    {
        Cum_emissions = Cum_emissions_0;
        Emiss_yearly_calib(2) = Emiss_yearly_0;
        Emiss_global = Emiss_yearly(1) / emiss_share - Emiss_yearly(1);
    }

    Emiss_yearly(2) += Emiss_global;
    Emiss_global = Emiss_global * g_emiss_global;
    Emiss_yearly(1) += Emiss_global;

    g_rate_em_y = (Emiss_yearly(1) - Emiss_yearly(2)) / Emiss_yearly(2);

    Emiss_yearly_calib(1) = Emiss_yearly_calib(2) * (1 + g_rate_em_y);

    Cum_emissions += Emiss_yearly_calib(1);

    Tmixed(1) = intercept_temp + slope_temp * Cum_emissions;
}

void SINGLESHOCK(void)
{
    // This is used for the experiment in which there is only a single fully exogenous climate shock (in period 300)
    // Set shock size
    if (t == 300)
    {
        if (flag_prodshocks1 > 0)
        {
            if (flag_uniformshocks == 0)
            {
                X_a(1) = (shocks_kfirms(1) * (b_0(1) - 2 / 3) + 1 / 3) / (1 - shocks_kfirms(1));
                for (i = 1; i <= N1; i++)
                {
                    rnd = betadev(X_a(1), b_0(1), p_seed);
                    shocks_machprod(i) = rnd;
                    rnd = betadev(X_a(1), b_0(1), p_seed);
                    shocks_techprod(i) = rnd;
                }
            }
            else
            {
                shocks_machprod = shocks_kfirms;
                shocks_techprod = shocks_kfirms;
            }
        }

        if (flag_prodshocks2 == 1)
        {
            // Shock to labour productivity of both K-firms and C-firms --> this affects overall productivity of C-firms, not just recent vintage of machines
            if (flag_uniformshocks == 0)
            {
                X_a(2) = (shocks_cfirms(1) * (b_0(2) - 2 / 3) + 1 / 3) / (1 - shocks_cfirms(1));
                for (j = 1; j <= N2; j++)
                {
                    rnd = betadev(X_a(2), b_0(2), p_seed);
                    shocks_labprod2(j) = rnd;
                }
                X_a(2) = (shocks_kfirms(1) * (b_0(2) - 2 / 3) + 1 / 3) / (1 - shocks_kfirms(1));
                for (i = 1; i <= N1; i++)
                {
                    rnd = betadev(X_a(2), b_0(2), p_seed);
                    shocks_labprod1(i) = rnd;
                }
            }
            else
            {
                shocks_labprod2 = shocks_cfirms;
                shocks_labprod1 = shocks_kfirms;
            }
        }

        if (flag_prodshocks2 == 2)
        {
            // Shock to energy efficiency of both K-firms and C-firms --> this affects overall efficiency of C-firms, not just recent vintage of machines
            if (flag_uniformshocks == 0)
            {
                X_a(2) = (shocks_cfirms(1) * (b_0(2) - 2 / 3) + 1 / 3) / (1 - shocks_cfirms(1));
                for (j = 1; j <= N2; j++)
                {
                    rnd = betadev(X_a(2), b_0(2), p_seed);
                    shocks_eneff2(j) = rnd;
                }
                X_a(2) = (shocks_kfirms(1) * (b_0(2) - 2 / 3) + 1 / 3) / (1 - shocks_kfirms(1));
                for (i = 1; i <= N1; i++)
                {
                    rnd = betadev(X_a(2), b_0(2), p_seed);
                    shocks_eneff1(i) = rnd;
                }
            }
            else
            {
                shocks_eneff2 = shocks_cfirms;
                shocks_eneff1 = shocks_kfirms;
            }
        }

        if (flag_prodshocks2 == 3)
        {
            // Shock to labour productivity and energy efficiency of both K-firms and C-firms --> as above
            if (flag_uniformshocks == 0)
            {
                X_a(2) = (shocks_cfirms(1) * (b_0(2) - 2 / 3) + 1 / 3) / (1 - shocks_cfirms(1));
                for (j = 1; j <= N2; j++)
                {
                    rnd = betadev(X_a(2), b_0(2), p_seed);
                    shocks_labprod2(j) = rnd;
                    rnd = betadev(X_a(2), b_0(2), p_seed);
                    shocks_eneff2(j) = rnd;
                }
                X_a(2) = (shocks_kfirms(1) * (b_0(2) - 2 / 3) + 1 / 3) / (1 - shocks_kfirms(1));
                for (i = 1; i <= N1; i++)
                {
                    rnd = betadev(X_a(2), b_0(2), p_seed);
                    shocks_labprod1(i) = rnd;
                    rnd = betadev(X_a(2), b_0(2), p_seed);
                    shocks_eneff1(i) = rnd;
                }
            }
            else
            {
                shocks_labprod2 = shocks_cfirms;
                shocks_labprod1 = shocks_kfirms;
                shocks_eneff2 = shocks_cfirms;
                shocks_eneff1 = shocks_kfirms;
            }
        }

        if (flag_encapshocks > 0)
        {
            shocks_encapstock_de = shock_scalar;
            shocks_encapstock_ge = shock_scalar;
        }

        if (flag_popshocks > 0)
        {
            shock_pop = shock_scalar;
        }

        if (flag_demandshocks > 0)
        {
            shock_cons = shock_scalar;
        }

        if (flag_capshocks > 0)
        {
            // Shock to C-firms' capital stocks
            if (flag_uniformshocks == 0)
            {
                X_a(6) = (shocks_cfirms(1) * (b_0(6) - 2 / 3) + 1 / 3) / (1 - shocks_cfirms(1));
                for (j = 1; j <= N2; j++)
                {
                    rnd = betadev(X_a(6), b_0(6), p_seed);
                    shocks_capstock(j) = rnd;
                }
            }
            else
            {
                shocks_capstock = shocks_cfirms;
            }

            if (flag_capshocks == 3)
            {
                for (j = 1; j <= N2; j++)
                {
                    risk_c(j) = gasdev(p_seed);
                }
                risk_c = risk_c - risk_c.Minimum() + 0.01;
                risk_c = risk_c / risk_c.Maximum();
            }
        }

        if (flag_outputshocks > 0)
        {
            // Shock to current output
            if (flag_uniformshocks == 0)
            {
                X_a(7) = (shocks_cfirms(1) * (b_0(7) - 2 / 3) + 1 / 3) / (1 - shocks_cfirms(1));
                for (j = 1; j <= N2; j++)
                {
                    rnd = betadev(X_a(7), b_0(7), p_seed);
                    shocks_output2(j) = rnd;
                }
                X_a(7) = (shocks_kfirms(1) * (b_0(7) - 2 / 3) + 1 / 3) / (1 - shocks_kfirms(1));
                for (i = 1; i <= N1; i++)
                {
                    rnd = betadev(X_a(7), b_0(7), p_seed);
                    shocks_output1(i) = rnd;
                }
            }
            else
            {
                shocks_output2 = shocks_cfirms;
                shocks_output1 = shocks_kfirms;
            }

            if (flag_outputshocks == 3)
            {
                for (j = 1; j <= N2; j++)
                {
                    risk_c(j) = gasdev(p_seed);
                }
                risk_c = risk_c - risk_c.Minimum() + 0.01;
                risk_c = risk_c / risk_c.Maximum();
                for (i = 1; i <= N1; i++)
                {
                    risk_k(i) = gasdev(p_seed);
                }
                risk_k = risk_k - risk_k.Minimum() + 0.01;
                risk_k = risk_k / risk_k.Maximum();
            }
        }

        if (flag_inventshocks > 0)
        {
            if (flag_uniformshocks == 0)
            {
                X_a(8) = (shocks_cfirms(1) * (b_0(8) - 2 / 3) + 1 / 3) / (1 - shocks_cfirms(1));
                for (j = 1; j <= N2; j++)
                {
                    rnd = betadev(X_a(8), b_0(8), p_seed);
                    shocks_invent(j) = rnd;
                }
            }
            else
            {
                shocks_invent = shocks_cfirms;
            }

            if (flag_inventshocks == 3 && risk_c.Sum() == 0)
            {
                for (j = 1; j <= N2; j++)
                {
                    risk_c(j) = gasdev(p_seed);
                }
                risk_c = risk_c - risk_c.Minimum() + 0.01;
                risk_c = risk_c / risk_c.Maximum();
            }
        }

        if (flag_RDshocks > 0)
        {
            // Shock to R&D
            if (flag_uniformshocks == 0)
            {
                X_a(9) = (shocks_kfirms(1) * (b_0(9) - 2 / 3) + 1 / 3) / (1 - shocks_kfirms(1));
                for (i = 1; i <= N1; i++)
                {
                    rnd = betadev(X_a(9), b_0(9), p_seed);
                    shocks_rd(i) = rnd;
                }
            }
            else
            {
                shocks_rd = shocks_kfirms;
            }
        }
    }
    else if (t > 300)
    {
        shocks_rd = persistence * shocks_rd;
        shocks_labprod2 = persistence * shocks_labprod2;
        shocks_labprod1 = persistence * shocks_labprod1;
        shocks_eneff2 = persistence * shocks_eneff2;
        shocks_eneff1 = persistence * shocks_eneff1;
        shocks_output2 = persistence * shocks_output2;
        shocks_output1 = persistence * shocks_output1;
        shocks_capstock = persistence * shocks_capstock;
        shocks_encapstock_de = persistence * shocks_encapstock_de;
        shocks_encapstock_ge = persistence * shocks_encapstock_ge;
        shocks_invent = persistence * shocks_invent;
        shocks_machprod = persistence * shocks_machprod;
        shocks_techprod = persistence * shocks_techprod;
        shock_pop = persistence * shock_pop;
        shock_cons = persistence * shock_cons;
    }

    if (t >= 300)
    {
        if (flag_prodshocks1 == 1)
        {
            for (i = 1; i <= N1; i++)
            {
                A1(i) = max(A0, A1(i) * (1 - shocks_machprod(i)));

                A1p(i) = max(A0 * pm, A1p(i) * (1 - shocks_techprod(i)));
            }
        }

        if (flag_prodshocks1 == 2)
        {
            for (i = 1; i <= N1; i++)
            {
                A1_en(i) = max(A0_en, A1_en(i) * (1 - shocks_machprod(i)));

                A1p_en(i) = max(A0_en, A1p_en(i) * (1 - shocks_techprod(i)));
            }
        }

        if (flag_prodshocks1 == 3)
        {
            for (i = 1; i <= N1; i++)
            {
                A1(i) = max(A0, A1(i) * (1 - shocks_machprod(i)));
                A1_en(i) = max(A0_en, A1_en(i) * (1 - shocks_machprod(i)));

                A1p(i) = max(A0 * pm, A1p(i) * (1 - shocks_techprod(i)));
                A1p_en(i) = max(A0_en, A1p_en(i) * (1 - shocks_techprod(i)));
            }
        }

        if (flag_prodshocks1 == 4)
        {
            for (i = 1; i <= N1; i++)
            {
                A1(i) = max(A0, A1(i) * (1 - shocks_machprod(i)));
                for (tt = t0; tt <= t; tt++)
                {
                    A(tt, i) = max(A0, A(tt, i) * (1 - shocks_machprod(i)));
                }

                A1p(i) = max(A0 * pm, A1p(i) * (1 - shocks_techprod(i)));
            }
        }

        if (flag_prodshocks1 == 5)
        {
            for (i = 1; i <= N1; i++)
            {
                A1_en(i) = max(A0_en, A1_en(i) * (1 - shocks_machprod(i)));
                for (tt = t0; tt <= t; tt++)
                {
                    A_en(tt, i) = max(A0_en, A_en(tt, i) * (1 - shocks_machprod(i)));
                }

                A1p_en(i) = max(A0_en, A1p_en(i) * (1 - shocks_techprod(i)));
            }
        }

        if (flag_prodshocks1 == 6)
        {
            for (i = 1; i <= N1; i++)
            {
                A1(i) = max(A0, A1(i) * (1 - shocks_machprod(i)));
                A1_en(i) = max(A0_en, A1_en(i) * (1 - shocks_machprod(i)));
                for (tt = t0; tt <= t; tt++)
                {
                    A(tt, i) = max(A0, A(tt, i) * (1 - shocks_machprod(i)));
                    A_en(tt, i) = max(A0_en, A_en(tt, i) * (1 - shocks_machprod(i)));
                }

                A1p(i) = max(A0 * pm, A1p(i) * (1 - shocks_techprod(i)));
                A1p_en(i) = max(A0_en, A1p_en(i) * (1 - shocks_techprod(i)));
            }
        }

        if (flag_encapshocks > 0)
        {
            // Shock to energy sector capital stock
            for (tt = 1; tt <= t; tt++)
            {
                if (G_de(tt) > 0)
                {
                    G_de(tt) = ROUND(G_de(tt) * (1 - shocks_encapstock_de(t)));
                }

                if (G_ge(tt) > 0)
                {
                    G_ge(tt) = ROUND(G_ge(tt) * (1 - shocks_encapstock_ge(t)));
                    G_ge_n(tt) = ROUND(G_ge_n(tt) * (1 - shocks_encapstock_ge(t)));
                }
            }
            CapitalStock_e(1) = G_ge_n.Sum();
        }

        if (flag_popshocks > 0)
        {
            // Shock to population (labour supply)
            LS = LS * (1 - shock_pop);
        }
    }
}

void SHOCKS(void)
{
    /*
     * SHOCKS(): Regional climate shock implementation
     *
     * If NR > 0 (regional model):
     *   - Uses same global temperature Tmixed(2) for all regions (global hazard source)
     *   - Computes regional disaster parameters X_a_reg, X_b_reg using region-specific and shock-specific
     *     a_0_regional[shock][region], b_0_regional[shock][region], 
     *     shockexponent1_regional[shock][region], shockexponent2_regional[shock][region]
     *   - Regional exponents OVERRIDE scalar values from climshockparams (no multiplication)
     *   - Scalar exponents are IGNORED in regional mode (NR > 0)
     *   - Preserves shock-type differentiation at regional level (e.g., shock 1 vs shock 2 can differ)
     *   - Draws ONE shock value per region per shock channel and broadcasts to all firms in that region
     *     → Implies perfect within-region shock correlation
     *     → Economically justified: unified climate hazard affecting spatially proximate agents in same region
     *   - All 16 shock channels (productivity, energy, labor, capital, output, inventory, R&D) regionalized
     *   - Preserves SFC integrity: all shocks scale real variables only; balance sheets/flows unchanged
     *   - Risk-based targeting (risk_c, risk_k) disabled in regional mode to avoid double-counting heterogeneity
     *
     * If NR == 0 (non-regional/global model):
     *   - Uses existing scalar X_a, X_b with firm-level or uniform shocks per flag_uniformshocks
     *   - Backward compatible with original model
     */
    // This function generates repeated endogenous climate shocks

    // Update parameter for disaster generating function
    // Compute regional disaster parameters if regional model
    // Using same global temperature Tmixed(2) for all regions; differentiation via regional shock parameters
    // Regional exponents override scalar values - no multiplication or fallback
    if (NR > 0)
    {
        for (int rr = 1; rr <= NR; rr++)
        {
            for (int i = 1; i <= nshocks; i++)
            {
                // Use regional exponents indexed [shock-1][region-1] (0-based arrays)
                // These completely override scalar shockexponent1(i) and shockexponent2(i)
                double exp1 = shockexponent1_regional[i-1][rr-1];
                double exp2 = shockexponent2_regional[i-1][rr-1];
                double a0 = a_0_regional[i-1][rr-1];
                double b0 = b_0_regional[i-1][rr-1];
                
                X_a_reg(i, rr) = a0 * pow(1 + log((Tmixed(2) + T_pre) / (T_pre + Tmixedinit1)), exp1);
                X_b_reg(i, rr) = b0 * pow((T_pre + Tmixedinit1) / (Tmixed(2) + T_pre), exp2);
            }
        }
    }
    else
    {
        // Fallback: use global scalar parameters (non-regional mode)
        for (i = 1; i <= nshocks; i++)
        {
            X_a(i) = a_0(i) * pow(1 + log((Tmixed(2) + T_pre) / (T_pre + Tmixedinit1)), shockexponent1(i));
            X_b(i) = b_0(i) * pow((T_pre + Tmixedinit1) / (Tmixed(2) + T_pre), shockexponent2(i));
        }
    }

    if (t > (t_start_climbox + 4))
    {
        //=== SHOCK CHANNEL 1: K-firm Machine Productivity (flag_prodshocks1==1) ===
        if (flag_prodshocks1 == 1)
        {
            if (NR > 0)
            {
                // Regional mode: draw one shock per region, broadcast to all firms in that region
                for (int rr = 1; rr <= NR; rr++)
                {
                    rnd = betadev(X_a_reg(1, rr), X_b_reg(1, rr), p_seed);
                    double shock_machprod_reg = rnd;
                    rnd = betadev(X_a_reg(1, rr), X_b_reg(1, rr), p_seed);
                    double shock_techprod_reg = rnd;

                    // Broadcast to all K-firms in this region
                    for (i = 1; i <= N1; i++)
                    {
                        if (region_firm_assignment_K[i - 1] == rr)
                        {
                            shocks_machprod(i) = shock_machprod_reg;
                            A1(i) = max(A0, A1(i) * (1 - shock_machprod_reg));

                            shocks_techprod(i) = shock_techprod_reg;
                            A1p(i) = max(A0 * pm, A1p(i) * (1 - shock_techprod_reg));
                        }
                    }
                }
            }
            else
            {
                // Global mode: each firm gets its own shock
                for (i = 1; i <= N1; i++)
                {
                    rnd = betadev(X_a(1), X_b(1), p_seed);
                    shocks_machprod(i) = rnd;
                    A1(i) = max(A0, A1(i) * (1 - shocks_machprod(i)));

                    rnd = betadev(X_a(1), X_b(1), p_seed);
                    shocks_techprod(i) = rnd;
                    A1p(i) = max(A0 * pm, A1p(i) * (1 - shocks_techprod(i)));
                }
            }
        }

        //=== SHOCK CHANNEL 2: K-firm Energy Productivity (flag_prodshocks1==2) ===
        if (flag_prodshocks1 == 2)
        {
            if (NR > 0)
            {
                // Regional mode: draw one shock per region, broadcast to all firms in that region
                for (int rr = 1; rr <= NR; rr++)
                {
                    rnd = betadev(X_a_reg(1, rr), X_b_reg(1, rr), p_seed);
                    double shock_machprod_reg = rnd;
                    rnd = betadev(X_a_reg(1, rr), X_b_reg(1, rr), p_seed);
                    double shock_techprod_reg = rnd;

                    // Broadcast to all K-firms in this region
                    for (i = 1; i <= N1; i++)
                    {
                        if (region_firm_assignment_K[i - 1] == rr)
                        {
                            shocks_machprod(i) = shock_machprod_reg;
                            A1_en(i) = max(A0_en, A1_en(i) * (1 - shock_machprod_reg));

                            shocks_techprod(i) = shock_techprod_reg;
                            A1p_en(i) = max(A0_en, A1p_en(i) * (1 - shock_techprod_reg));
                        }
                    }
                }
            }
            else
            {
                // Global mode: each firm gets its own shock
                for (i = 1; i <= N1; i++)
                {
                    rnd = betadev(X_a(1), X_b(1), p_seed);
                    shocks_machprod(i) = rnd;
                    A1_en(i) = max(A0_en, A1_en(i) * (1 - shocks_machprod(i)));

                    rnd = betadev(X_a(1), X_b(1), p_seed);
                    shocks_techprod(i) = rnd;
                    A1p_en(i) = max(A0_en, A1p_en(i) * (1 - shocks_techprod(i)));
                }
            }
        }

        //=== SHOCK CHANNEL 3: K-firm Both Productivities (flag_prodshocks1==3) ===
        if (flag_prodshocks1 == 3)
        {
            if (NR > 0)
            {
                // Regional mode: draw one shock per region, broadcast to all firms in that region
                for (int rr = 1; rr <= NR; rr++)
                {
                    rnd = betadev(X_a_reg(1, rr), X_b_reg(1, rr), p_seed);
                    double shock_machprod_reg = rnd;
                    rnd = betadev(X_a_reg(1, rr), X_b_reg(1, rr), p_seed);
                    double shock_techprod_reg = rnd;

                    // Broadcast to all K-firms in this region
                    for (i = 1; i <= N1; i++)
                    {
                        if (region_firm_assignment_K[i - 1] == rr)
                        {
                            shocks_machprod(i) = shock_machprod_reg;
                            A1(i) = max(A0, A1(i) * (1 - shock_machprod_reg));
                            A1_en(i) = max(A0_en, A1_en(i) * (1 - shock_machprod_reg));

                            shocks_techprod(i) = shock_techprod_reg;
                            A1p(i) = max(A0 * pm, A1p(i) * (1 - shock_techprod_reg));
                            A1p_en(i) = max(A0_en, A1p_en(i) * (1 - shock_techprod_reg));
                        }
                    }
                }
            }
            else
            {
                // Global mode: each firm gets its own shock
                for (i = 1; i <= N1; i++)
                {
                    rnd = betadev(X_a(1), X_b(1), p_seed);
                    shocks_machprod(i) = rnd;
                    A1(i) = max(A0, A1(i) * (1 - shocks_machprod(i)));
                    A1_en(i) = max(A0_en, A1_en(i) * (1 - shocks_machprod(i)));

                    rnd = betadev(X_a(1), X_b(1), p_seed);
                    shocks_techprod(i) = rnd;
                    A1p(i) = max(A0 * pm, A1p(i) * (1 - shocks_techprod(i)));
                    A1p_en(i) = max(A0_en, A1p_en(i) * (1 - shocks_techprod(i)));
                }
            }
        }

        //=== SHOCK CHANNEL 4: K-firm Machine with Historical Updates (flag_prodshocks1==4) ===
        if (flag_prodshocks1 == 4)
        {
            if (NR > 0)
            {
                // Regional mode: draw one shock per region, broadcast to all firms in that region
                for (int rr = 1; rr <= NR; rr++)
                {
                    rnd = betadev(X_a_reg(1, rr), X_b_reg(1, rr), p_seed);
                    double shock_machprod_reg = rnd;
                    rnd = betadev(X_a_reg(1, rr), X_b_reg(1, rr), p_seed);
                    double shock_techprod_reg = rnd;

                    // Broadcast to all K-firms in this region
                    for (i = 1; i <= N1; i++)
                    {
                        if (region_firm_assignment_K[i - 1] == rr)
                        {
                            shocks_machprod(i) = shock_machprod_reg;
                            A1(i) = max(A0, A1(i) * (1 - shock_machprod_reg));
                            for (tt = t0; tt <= t; tt++)
                            {
                                A(tt, i) = max(A0, A(tt, i) * (1 - shock_machprod_reg));
                            }

                            shocks_techprod(i) = shock_techprod_reg;
                            A1p(i) = max(A0 * pm, A1p(i) * (1 - shock_techprod_reg));
                        }
                    }
                }
            }
            else
            {
                // Global mode: each firm gets its own shock
                for (i = 1; i <= N1; i++)
                {
                    rnd = betadev(X_a(1), X_b(1), p_seed);
                    shocks_machprod(i) = rnd;
                    A1(i) = max(A0, A1(i) * (1 - shocks_machprod(i)));
                    for (tt = t0; tt <= t; tt++)
                    {
                        A(tt, i) = max(A0, A(tt, i) * (1 - shocks_machprod(i)));
                    }

                    rnd = betadev(X_a(1), X_b(1), p_seed);
                    shocks_techprod(i) = rnd;
                    A1p(i) = max(A0 * pm, A1p(i) * (1 - shocks_techprod(i)));
                }
            }
        }

        //=== SHOCK CHANNEL 5: K-firm Energy with Historical Updates (flag_prodshocks1==5) ===
        if (flag_prodshocks1 == 5)
        {
            if (NR > 0)
            {
                // Regional mode: draw one shock per region, broadcast to all firms in that region
                for (int rr = 1; rr <= NR; rr++)
                {
                    rnd = betadev(X_a_reg(1, rr), X_b_reg(1, rr), p_seed);
                    double shock_machprod_reg = rnd;
                    rnd = betadev(X_a_reg(1, rr), X_b_reg(1, rr), p_seed);
                    double shock_techprod_reg = rnd;

                    // Broadcast to all K-firms in this region
                    for (i = 1; i <= N1; i++)
                    {
                        if (region_firm_assignment_K[i - 1] == rr)
                        {
                            shocks_machprod(i) = shock_machprod_reg;
                            A1_en(i) = max(A0_en, A1_en(i) * (1 - shock_machprod_reg));
                            for (tt = t0; tt <= t; tt++)
                            {
                                A_en(tt, i) = max(A0_en, A_en(tt, i) * (1 - shock_machprod_reg));
                            }

                            shocks_techprod(i) = shock_techprod_reg;
                            A1p_en(i) = max(A0_en, A1p_en(i) * (1 - shock_techprod_reg));
                        }
                    }
                }
            }
            else
            {
                // Global mode: each firm gets its own shock
                for (i = 1; i <= N1; i++)
                {
                    rnd = betadev(X_a(1), X_b(1), p_seed);
                    shocks_machprod(i) = rnd;
                    A1_en(i) = max(A0_en, A1_en(i) * (1 - shocks_machprod(i)));
                    for (tt = t0; tt <= t; tt++)
                    {
                        A_en(tt, i) = max(A0_en, A_en(tt, i) * (1 - shocks_machprod(i)));
                    }

                    rnd = betadev(X_a(1), X_b(1), p_seed);
                    shocks_techprod(i) = rnd;
                    A1p_en(i) = max(A0_en, A1p_en(i) * (1 - shocks_techprod(i)));
                }
            }
        }

        //=== SHOCK CHANNEL 6: K-firm Both with Historical (flag_prodshocks1==6) ===
        if (flag_prodshocks1 == 6)
        {
            if (NR > 0)
            {
                // Regional mode: draw one shock per region, broadcast to all firms in that region
                for (int rr = 1; rr <= NR; rr++)
                {
                    rnd = betadev(X_a_reg(1, rr), X_b_reg(1, rr), p_seed);
                    double shock_machprod_reg = rnd;
                    rnd = betadev(X_a_reg(1, rr), X_b_reg(1, rr), p_seed);
                    double shock_techprod_reg = rnd;

                    // Broadcast to all K-firms in this region
                    for (i = 1; i <= N1; i++)
                    {
                        if (region_firm_assignment_K[i - 1] == rr)
                        {
                            shocks_machprod(i) = shock_machprod_reg;
                            A1(i) = max(A0, A1(i) * (1 - shock_machprod_reg));
                            A1_en(i) = max(A0_en, A1_en(i) * (1 - shock_machprod_reg));
                            for (tt = t0; tt <= t; tt++)
                            {
                                A(tt, i) = max(A0, A(tt, i) * (1 - shock_machprod_reg));
                                A_en(tt, i) = max(A0_en, A_en(tt, i) * (1 - shock_machprod_reg));
                            }

                            shocks_techprod(i) = shock_techprod_reg;
                            A1p(i) = max(A0 * pm, A1p(i) * (1 - shock_techprod_reg));
                            A1p_en(i) = max(A0_en, A1p_en(i) * (1 - shock_techprod_reg));
                        }
                    }
                }
            }
            else
            {
                // Global mode: each firm gets its own shock
                for (i = 1; i <= N1; i++)
                {
                    rnd = betadev(X_a(1), X_b(1), p_seed);
                    shocks_machprod(i) = rnd;
                    A1(i) = max(A0, A1(i) * (1 - shocks_machprod(i)));
                    A1_en(i) = max(A0_en, A1_en(i) * (1 - shocks_machprod(i)));
                    for (tt = t0; tt <= t; tt++)
                    {
                        A(tt, i) = max(A0, A(tt, i) * (1 - shocks_machprod(i)));
                        A_en(tt, i) = max(A0_en, A_en(tt, i) * (1 - shocks_machprod(i)));
                    }

                    rnd = betadev(X_a(1), X_b(1), p_seed);
                    shocks_techprod(i) = rnd;
                    A1p(i) = max(A0 * pm, A1p(i) * (1 - shocks_techprod(i)));
                    A1p_en(i) = max(A0_en, A1p_en(i) * (1 - shocks_techprod(i)));
                }
            }
        }

        //=== SHOCK CHANNEL 7: Labor Productivity (flag_prodshocks2==1) ===
        if (flag_prodshocks2 == 1)
        {
            if (NR > 0)
            {
                // Regional mode: draw one shock per region, broadcast to all firms in that region
                for (int rr = 1; rr <= NR; rr++)
                {
                    rnd = betadev(X_a_reg(2, rr), X_b_reg(2, rr), p_seed);
                    double shock_labprod_reg = rnd;

                    // Broadcast to all C-firms in this region
                    for (j = 1; j <= N2; j++)
                    {
                        if (region_firm_assignment_C[j - 1] == rr)
                        {
                            shocks_labprod2(j) = shock_labprod_reg;
                        }
                    }

                    // Broadcast to all K-firms in this region
                    for (i = 1; i <= N1; i++)
                    {
                        if (region_firm_assignment_K[i - 1] == rr)
                        {
                            shocks_labprod1(i) = shock_labprod_reg;
                        }
                    }
                }
            }
            else
            {
                // Global mode: each firm gets its own shock
                for (j = 1; j <= N2; j++)
                {
                    rnd = betadev(X_a(2), X_b(2), p_seed);
                    shocks_labprod2(j) = rnd;
                }

                for (i = 1; i <= N1; i++)
                {
                    rnd = betadev(X_a(2), X_b(2), p_seed);
                    shocks_labprod1(i) = rnd;
                }
            }
        }

        //=== SHOCK CHANNEL 8: Energy Efficiency (flag_prodshocks2==2) ===
        if (flag_prodshocks2 == 2)
        {
            if (NR > 0)
            {
                // Regional mode: draw one shock per region, broadcast to all firms in that region
                for (int rr = 1; rr <= NR; rr++)
                {
                    rnd = betadev(X_a_reg(2, rr), X_b_reg(2, rr), p_seed);
                    double shock_eneff_reg = rnd;

                    // Broadcast to all C-firms in this region
                    for (j = 1; j <= N2; j++)
                    {
                        if (region_firm_assignment_C[j - 1] == rr)
                        {
                            shocks_eneff2(j) = shock_eneff_reg;
                        }
                    }

                    // Broadcast to all K-firms in this region
                    for (i = 1; i <= N1; i++)
                    {
                        if (region_firm_assignment_K[i - 1] == rr)
                        {
                            shocks_eneff1(i) = shock_eneff_reg;
                        }
                    }
                }
            }
            else
            {
                // Global mode: each firm gets its own shock
                for (j = 1; j <= N2; j++)
                {
                    rnd = betadev(X_a(2), X_b(2), p_seed);
                    shocks_eneff2(j) = rnd;
                }

                for (i = 1; i <= N1; i++)
                {
                    rnd = betadev(X_a(2), X_b(2), p_seed);
                    shocks_eneff1(i) = rnd;
                }
            }
        }

        //=== SHOCK CHANNEL 9: Both Labor and Energy (flag_prodshocks2==3) ===
        if (flag_prodshocks2 == 3)
        {
            if (NR > 0)
            {
                // Regional mode: draw one shock per region, broadcast to all firms in that region
                for (int rr = 1; rr <= NR; rr++)
                {
                    rnd = betadev(X_a_reg(2, rr), X_b_reg(2, rr), p_seed);
                    double shock_labprod_reg = rnd;
                    rnd = betadev(X_a_reg(2, rr), X_b_reg(2, rr), p_seed);
                    double shock_eneff_reg = rnd;

                    // Broadcast to all C-firms in this region
                    for (j = 1; j <= N2; j++)
                    {
                        if (region_firm_assignment_C[j - 1] == rr)
                        {
                            shocks_labprod2(j) = shock_labprod_reg;
                            shocks_eneff2(j) = shock_eneff_reg;
                        }
                    }

                    // Broadcast to all K-firms in this region
                    for (i = 1; i <= N1; i++)
                    {
                        if (region_firm_assignment_K[i - 1] == rr)
                        {
                            shocks_labprod1(i) = shock_labprod_reg;
                            shocks_eneff1(i) = shock_eneff_reg;
                        }
                    }
                }
            }
            else
            {
                // Global mode: each firm gets its own shock
                for (j = 1; j <= N2; j++)
                {
                    rnd = betadev(X_a(2), X_b(2), p_seed);
                    shocks_labprod2(j) = rnd;
                    rnd = betadev(X_a(2), X_b(2), p_seed);
                    shocks_eneff2(j) = rnd;
                }

                for (i = 1; i <= N1; i++)
                {
                    rnd = betadev(X_a(2), X_b(2), p_seed);
                    shocks_labprod1(i) = rnd;
                    rnd = betadev(X_a(2), X_b(2), p_seed);
                    shocks_eneff1(i) = rnd;
                }
            }
        }

        //=== SHOCK CHANNEL 10: Energy Capacity Shock (flag_encapshocks) ===
        if (flag_encapshocks > 0)
        {
            if (NR > 0)
            {
                // Regional mode: draw one shock scalar per region, apply nationally
                for (int rr = 1; rr <= NR; rr++)
                {
                    regional_shock_value[rr - 1] = X_a_reg(3, rr) / (X_a_reg(3, rr) + X_b_reg(3, rr));
                }

                // Apply shocks to energy infrastructure (averaged across regions)
                double shock_encap_avg = 0.0;
                for (int rr = 0; rr < NR; rr++)
                {
                    shock_encap_avg += regional_shock_value[rr];
                }
                shock_encap_avg /= NR;

                for (tt = 1; tt <= t; tt++)
                {
                    shocks_encapstock_de(t) = shock_encap_avg;
                    if (G_de(tt) > 0)
                    {
                        G_de(tt) = ROUND(G_de(tt) * (1 - shock_encap_avg));
                    }

                    shocks_encapstock_ge(t) = shock_encap_avg;
                    if (G_ge(tt) > 0)
                    {
                        G_ge(tt) = ROUND(G_ge(tt) * (1 - shock_encap_avg));
                        G_ge_n(tt) = ROUND(G_ge_n(tt) * (1 - shock_encap_avg));
                    }
                }
                CapitalStock_e(1) = G_ge_n.Sum();
            }
            else
            {
                // Global mode: single scalar shock
                for (tt = 1; tt <= t; tt++)
                {
                    shocks_encapstock_de(t) = X_a(3) / (X_a(3) + X_b(3));
                    if (G_de(tt) > 0)
                    {
                        G_de(tt) = ROUND(G_de(tt) * (1 - shocks_encapstock_de(t)));
                    }

                    shocks_encapstock_ge(t) = X_a(3) / (X_a(3) + X_b(3));
                    if (G_ge(tt) > 0)
                    {
                        G_ge(tt) = ROUND(G_ge(tt) * (1 - shocks_encapstock_ge(t)));
                        G_ge_n(tt) = ROUND(G_ge_n(tt) * (1 - shocks_encapstock_ge(t)));
                    }
                }
                CapitalStock_e(1) = G_ge_n.Sum();
            }
        }

        //=== SHOCK CHANNEL 11: Labor Supply Shock (flag_popshocks) ===
        if (flag_popshocks > 0)
        {
            if (NR > 0)
            {
                // Regional mode: draw one shock scalar per region, apply globally
                for (int rr = 1; rr <= NR; rr++)
                {
                    regional_shock_value[rr - 1] = X_a_reg(4, rr) / (X_a_reg(4, rr) + X_b_reg(4, rr));
                }

                // Average shock across regions and apply to global labor supply
                double shock_pop_avg = 0.0;
                for (int rr = 0; rr < NR; rr++)
                {
                    shock_pop_avg += regional_shock_value[rr];
                }
                shock_pop_avg /= NR;

                shock_pop = shock_pop_avg;
                LS = LS * (1 - shock_pop_avg);
            }
            else
            {
                // Global mode: single scalar shock
                shock_pop = X_a(4) / (X_a(4) + X_b(4));
                LS = LS * (1 - shock_pop);
            }
        }

        //=== SHOCK CHANNEL 12: Demand Shock (flag_demandshocks) ===
        if (flag_demandshocks > 0)
        {
            if (NR > 0)
            {
                // Regional mode: draw one shock scalar per region, apply globally
                for (int rr = 1; rr <= NR; rr++)
                {
                    regional_shock_value[rr - 1] = X_a_reg(5, rr) / (X_a_reg(5, rr) + X_b_reg(5, rr));
                }

                // Average shock across regions and apply to global demand
                double shock_cons_avg = 0.0;
                for (int rr = 0; rr < NR; rr++)
                {
                    shock_cons_avg += regional_shock_value[rr];
                }
                shock_cons_avg /= NR;

                shock_cons = shock_cons_avg;
            }
            else
            {
                // Global mode: single scalar shock
                shock_cons = X_a(5) / (X_a(5) + X_b(5));
            }
        }

        //=== SHOCK CHANNEL 13: Capital Destruction Shock (flag_capshocks) ===
        if (flag_capshocks > 0)
        {
            if (NR > 0)
            {
                // Regional mode: draw one shock per region, broadcast to all firms in that region
                if (flag_capshocks == 1)
                {
                    for (int rr = 1; rr <= NR; rr++)
                    {
                        rnd = betadev(X_a_reg(6, rr), X_b_reg(6, rr), p_seed);
                        double shock_capstock_reg = rnd;

                        for (j = 1; j <= N2; j++)
                        {
                            if (region_firm_assignment_C[j - 1] == rr)
                            {
                                shocks_capstock(j) = shock_capstock_reg;
                            }
                        }
                    }
                }
                else
                {
                    // For risk-based shocks (flag_capshocks==3), apply averaged regional shock
                    double shock_capstock_avg = 0.0;
                    for (int rr = 1; rr <= NR; rr++)
                    {
                        double shock_reg = X_a_reg(6, rr) / (X_a_reg(6, rr) + X_b_reg(6, rr));
                        shock_capstock_avg += shock_reg;
                    }
                    shock_capstock_avg /= NR;
                    shocks_capstock = shock_capstock_avg;
                }

                if (flag_capshocks == 3 && risk_c.Sum() == 0)
                {
                    for (j = 1; j <= N2; j++)
                    {
                        risk_c(j) = gasdev(p_seed);
                    }
                    risk_c = risk_c - risk_c.Minimum() + 0.01;
                    risk_c = risk_c / risk_c.Maximum();
                }
            }
            else
            {
                // Global mode: only allow risk-based (flag_capshocks==3 or scalar)
                if (flag_capshocks == 1)
                {
                    for (j = 1; j <= N2; j++)
                    {
                        rnd = betadev(X_a(6), X_b(6), p_seed);
                        shocks_capstock(j) = rnd;
                    }
                }
                else
                {
                    shocks_capstock = X_a(6) / (X_a(6) + X_b(6));
                }

                if (flag_capshocks == 3 && risk_c.Sum() == 0)
                {
                    for (j = 1; j <= N2; j++)
                    {
                        risk_c(j) = gasdev(p_seed);
                    }
                    risk_c = risk_c - risk_c.Minimum() + 0.01;
                    risk_c = risk_c / risk_c.Maximum();
                }
            }
        }

        //=== SHOCK CHANNEL 14: Output Shock (flag_outputshocks) ===
        if (flag_outputshocks > 0)
        {
            if (NR > 0)
            {
                // Regional mode: draw one shock per region, broadcast to all firms in that region
                if (flag_outputshocks == 1)
                {
                    for (int rr = 1; rr <= NR; rr++)
                    {
                        rnd = betadev(X_a_reg(7, rr), X_b_reg(7, rr), p_seed);
                        double shock_output_reg = rnd;

                        // Apply to C-firms
                        for (j = 1; j <= N2; j++)
                        {
                            if (region_firm_assignment_C[j - 1] == rr)
                            {
                                shocks_output2(j) = shock_output_reg;
                            }
                        }

                        // Apply to K-firms
                        for (i = 1; i <= N1; i++)
                        {
                            if (region_firm_assignment_K[i - 1] == rr)
                            {
                                shocks_output1(i) = shock_output_reg;
                            }
                        }
                    }
                }
                else
                {
                    // For risk-based shocks, apply averaged regional shock
                    double shock_output_avg = 0.0;
                    for (int rr = 1; rr <= NR; rr++)
                    {
                        double shock_reg = X_a_reg(7, rr) / (X_a_reg(7, rr) + X_b_reg(7, rr));
                        shock_output_avg += shock_reg;
                    }
                    shock_output_avg /= NR;
                    shocks_output2 = shock_output_avg;
                    shocks_output1 = shock_output_avg;
                }

                if (flag_outputshocks == 3 && risk_c.Sum() == 0)
                {
                    for (j = 1; j <= N2; j++)
                    {
                        risk_c(j) = gasdev(p_seed);
                    }
                    risk_c = risk_c - risk_c.Minimum() + 0.01;
                    risk_c = risk_c / risk_c.Maximum();
                }

                if (flag_outputshocks == 3 && risk_k.Sum() == 0)
                {
                    for (i = 1; i <= N1; i++)
                    {
                        risk_k(i) = gasdev(p_seed);
                    }
                    risk_k = risk_k - risk_k.Minimum() + 0.01;
                    risk_k = risk_k / risk_k.Maximum();
                }
            }
            else
            {
                // Global mode: only allow risk-based (flag_outputshocks==3 or scalar)
                if (flag_outputshocks == 1)
                {
                    for (j = 1; j <= N2; j++)
                    {
                        rnd = betadev(X_a(7), X_b(7), p_seed);
                        shocks_output2(j) = rnd;
                    }

                    for (i = 1; i <= N1; i++)
                    {
                        rnd = betadev(X_a(7), X_b(7), p_seed);
                        shocks_output1(i) = rnd;
                    }
                }
                else
                {
                    shocks_output2 = X_a(7) / (X_a(7) + X_b(7));
                    shocks_output1 = X_a(7) / (X_a(7) + X_b(7));
                }

                if (flag_outputshocks == 3 && risk_c.Sum() == 0)
                {
                    for (j = 1; j <= N2; j++)
                    {
                        risk_c(j) = gasdev(p_seed);
                    }
                    risk_c = risk_c - risk_c.Minimum() + 0.01;
                    risk_c = risk_c / risk_c.Maximum();
                }

                if (flag_outputshocks == 3 && risk_k.Sum() == 0)
                {
                    for (i = 1; i <= N1; i++)
                    {
                        risk_k(i) = gasdev(p_seed);
                    }
                    risk_k = risk_k - risk_k.Minimum() + 0.01;
                    risk_k = risk_k / risk_k.Maximum();
                }
            }
        }

        //=== SHOCK CHANNEL 15: Inventory Shock (flag_inventshocks) ===
        if (flag_inventshocks > 0)
        {
            if (NR > 0)
            {
                // Regional mode: draw one shock per region, broadcast to all firms in that region
                if (flag_inventshocks == 1)
                {
                    for (int rr = 1; rr <= NR; rr++)
                    {
                        rnd = betadev(X_a_reg(8, rr), X_b_reg(8, rr), p_seed);
                        double shock_invent_reg = rnd;

                        for (j = 1; j <= N2; j++)
                        {
                            if (region_firm_assignment_C[j - 1] == rr)
                            {
                                shocks_invent(j) = shock_invent_reg;
                            }
                        }
                    }
                }
                else
                {
                    // For risk-based shocks, apply averaged regional shock
                    double shock_invent_avg = 0.0;
                    for (int rr = 1; rr <= NR; rr++)
                    {
                        double shock_reg = X_a_reg(8, rr) / (X_a_reg(8, rr) + X_b_reg(8, rr));
                        shock_invent_avg += shock_reg;
                    }
                    shock_invent_avg /= NR;
                    shocks_invent = shock_invent_avg;
                }

                if (flag_inventshocks == 3 && risk_c.Sum() == 0)
                {
                    for (j = 1; j <= N2; j++)
                    {
                        risk_c(j) = gasdev(p_seed);
                    }
                    risk_c = risk_c - risk_c.Minimum() + 0.01;
                    risk_c = risk_c / risk_c.Maximum();
                }
            }
            else
            {
                // Global mode: only allow risk-based (flag_inventshocks==3 or scalar)
                if (flag_inventshocks == 1)
                {
                    for (j = 1; j <= N2; j++)
                    {
                        rnd = betadev(X_a(8), X_b(8), p_seed);
                        shocks_invent(j) = rnd;
                    }
                }
                else
                {
                    shocks_invent = X_a(8) / (X_a(8) + X_b(8));
                }

                if (flag_inventshocks == 3 && risk_c.Sum() == 0)
                {
                    for (j = 1; j <= N2; j++)
                    {
                        risk_c(j) = gasdev(p_seed);
                    }
                    risk_c = risk_c - risk_c.Minimum() + 0.01;
                    risk_c = risk_c / risk_c.Maximum();
                }
            }
        }

        //=== SHOCK CHANNEL 16: R&D Effectiveness Shock (flag_RDshocks) ===
        if (flag_RDshocks > 0)
        {
            if (NR > 0)
            {
                // Regional mode: draw one shock per region, broadcast to all firms in that region
                for (int rr = 1; rr <= NR; rr++)
                {
                    rnd = betadev(X_a_reg(9, rr), X_b_reg(9, rr), p_seed);
                    double shock_rd_reg = rnd;

                    for (i = 1; i <= N1; i++)
                    {
                        if (region_firm_assignment_K[i - 1] == rr)
                        {
                            shocks_rd(i) = shock_rd_reg;
                        }
                    }
                }
            }
            else
            {
                // Global mode: each firm gets its own shock
                for (i = 1; i <= N1; i++)
                {
                    rnd = betadev(X_a(9), X_b(9), p_seed);
                    shocks_rd(i) = rnd;
                }
            }
        }
    }
}

void UPDATECLIMATE(void)
{
    // Update variables entering the climate box for the next period
    if (t % freqclim == 0)
    {
        Tmixed(2) = Tmixed(1);
        Emiss_yearly_calib(2) = Emiss_yearly_calib(1);
        Emiss_TOT(2) = Emiss_TOT(1);

        Cat(2) = Cat(1);
        biom(2) = biom(1);
        hum(2) = hum(1);
        for (j = 1; j <= ndep; j++)
        {
            Con(2, j) = Con(1, j);
            Hon(2, j) = Hon(1, j);
            Ton(2, j) = Ton(1, j);
        }
    }

    for (j = 1; j <= freqclim * 2 - 1; j++)
    {
        Emiss_TOT(freqclim * 2 - j + 1) = Emiss_TOT(freqclim * 2 - j);
    }
}