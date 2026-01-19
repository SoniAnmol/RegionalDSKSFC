#include "module_macro_sfc.h"

void LABOR(void)
{
	// Calculate total labour demand
	for (i = 1; i <= N1; i++)
	{
		LD1tot += Ld1(i);
	}

	for (j = 1; j <= N2; j++)
	{
		LD2tot += Ld2(j);
	}

	if (t > 200)
	{
		LS *= g_ls;
	}

	LSe = LS;
	LSe -= (LD1rdtot + LDentot);

	// If total labour demand exceeds supply, production is scaled back
	if (LD2tot + LD1tot <= LSe)
	{
		LSe = LSe - LD1tot - LD2tot;
	}
	else
	{

		for (j = 1; j <= N2; j++)
		{
			Ld2(j) = Ld2(j) * LSe / (LD1tot + LD2tot);
			Q2(j) = Ld2(j) * A2e(j);
		}

		for (i = 1; i <= N1; i++)
		{
			Qpast = Q1(i);

			if (Qpast > 0)
			{
				Ld1(i) = Ld1(i) * LSe / (LD1tot + LD2tot);
				Q1(i) = floor(Ld1(i) * ((1 - shocks_labprod1(i)) * A1p(i) * a));
				reduction = Qpast - Q1(i);
				while (reduction > 0)
				{
					ranj = int(ran1(p_seed) * N1 * N2) % N2 + 1;
					if (Match(ranj, i) == 1 && I(ranj) > 0)
					{
						Ipast = I(ranj);
						I(ranj) = floor((I(ranj) / dim_mach) * Q1(i) / Qpast) * dim_mach;
						if (I(ranj) < EI(1, ranj))
						{
							EI(1, ranj) = I(ranj);
						}
						SI(ranj) = I(ranj) - EI(1, ranj);
						reduction -= (Ipast - I(ranj)) / dim_mach;
					}
				}
			}
		}

		LD2tot = 0;
		LD1tot = 0;

		for (i = 1; i <= N1; i++)
		{
			LD1tot += Ld1(i);
		}

		for (j = 1; j <= N2; j++)
		{
			LD2tot += Ld2(j);
		}
	}

	LD = LD1tot + LD2tot + LD1rdtot + LDentot;
	LD2 = LD1tot + LD2tot;

	if (flag_desc == 2 && t >= 200)
	{
		wu = 0.45;
	}

	if (flag_desc == 3 && t >= 200)
	{
		wu = 0.6;
	}

	// Determine unemployment benefit payments
	if (LS > LD)
	{
		G = (LS - LD) * (w(2) * wu);
	}
	else
	{
		G = 0;
	}

	G = G + Transfer_shock;

	Benefits = G;
}

void MACRO(void)
{
	// Calculate macroeconomic aggregates, mean values etc
	ExpansionInvestment_r = EI.Row(1).Sum();
	ExpansionInvestment_n = EI_n.Sum();
	ReplacementInvestment_r = SI.Sum();
	ReplacementInvestment_n = SI_n.Sum();
	Investment_r = ExpansionInvestment_r + ReplacementInvestment_r;
	Investment_n = ExpansionInvestment_n + ReplacementInvestment_n;
	Q2tot = Q2.Sum();
	Q2dtot = Qd.Sum();
	D2tot = D2.Row(1).Sum();
	Q1tot = Q1.Sum();

	for (j = 1; j <= N2; j++)
	{
		if (LD2 > 0)
		{
			Am_a += Ld2(j) / LD2 * A2e(j);
			Am2 += Ld2(j) / LD2tot * A2e(j);
		}
		if ((D2_en_TOT + D1_en_TOT) > 0)
		{
			Am_en(1) += D2_en(j) / (D2_en_TOT + D1_en_TOT) * A2e_en(j);
		}
		Am(1) += A2(j);
		A_mi += log(A2(j));
		A2_en_mi += log(A2_en(j));
		A2_ef_mi += log(A2_ef(j));
		H2 += f2(1, j) * f2(1, j);
	}

	A_mi /= N2r;
	A2_en_mi /= N2r;
	A2_ef_mi /= N2r;
	H2 = (H2 - 1 / N2r) / (1 - 1 / N2r);

	for (j = 1; j <= N2; j++)
	{
		A_sd += (log(A2(j)) - A_mi) * (log(A2(j)) - A_mi);
	}

	A_sd = sqrt(A_sd / N2);

	for (i = 1; i <= N1; i++)
	{
		if (Q1tot > 0)
		{
			f1(1, i) = Q1(i) / Q1tot;
		}
		else
		{
			f1(1, i) = f1(2, i);
		}

		H1 += f1(1, i) * f1(1, i);
		A1_mi += log(A1p(i));
		A1_en_mi += log(A1p_en(i));
		A1_ef_mi += log(A1p_ef(i));

		if (LD2 > 0)
		{
			Am_a += Ld1(i) / LD2 * A1p(i) * a;
			Am1 += Ld1(i) / LD1tot * A1p(i);
		}
		Am(1) += A1p(i) * a;
		if ((D2_en_TOT + D1_en_TOT) > 0)
		{
			Am_en(1) += D1_en(i) / (D2_en_TOT + D1_en_TOT) * A1p_en(i);
		}
	}

	Am(1) /= (N1r + N2r);
	A1_mi /= N1r;
	A1_en_mi /= N1r;
	A1_ef_mi /= N1r;
	H1 = (H1 - 1 / N1r) / (1 - 1 / N1r);

	CreditSupply_all = BaselBankCredit.Sum();
	CreditDemand_all = CreditDemand.Sum();

	for (i = 1; i <= NB; i++)
	{
		if (CreditSupply_all > 0)
		{
			fB(1, i) = (BaselBankCredit(i) / CreditSupply_all);
		}
		else
		{
			fB(1, i) = fB(2, i);
		}

		HB += fB(1, i) * fB(1, i);
	}

	// If fulloutput==1, save individual productivity and debt values
	if (fulloutput == 1)
	{
		WRITEPROD();
		WRITEDEB();
	}

	// GDP
	GDP_r(1) = Q1tot * dim_mach + Q2tot;
	GDP_n(1) = 0;
	for (i = 1; i <= N1; i++)
	{
		GDP_n(1) += Q1(i) * dim_mach * p1(i) * a;
	}
	for (i = 1; i <= N2; i++)
	{
		GDP_n(1) += Q2(i) * p2(i);
	}

	if (t > 1)
	{
		GDP_rg = log(GDP_r(1)) - log(GDP_r(2));
		GDP_ng = log(GDP_n(1)) - log(GDP_n(2));
	}

	// Determine unemployment rate
	U(1) = (LS - LD) / LS;

	// Calculate regional unemployment rates if regions are defined
	if (NR > 0)
	{
		for (int rr = 1; rr <= NR; ++rr)
		{
			double reg_LS_used = 0;
			double reg_LD = 0;

			// Aggregate labor demand from K-firms in this region
			for (int ii = 1; ii <= N1; ++ii)
			{
				if (region_firm_assignment_K[ii - 1] == rr)
				{
					reg_LS_used += Ld1(ii);
				}
			}

			// Aggregate labor demand from C-firms in this region
			for (int jj = 1; jj <= N2; ++jj)
			{
				if (region_firm_assignment_C[jj - 1] == rr)
				{
					reg_LS_used += Ld2(jj);
				}
			}

			// Calculate regional labor demand including R&D and energy sector proportionally
			double total_LD_firms = LD1tot + LD2tot; // Total Ld1 + Ld2 across all firms
			double reg_LD_rd = 0;
			double reg_LD_en = 0;
			if (total_LD_firms > 0)
			{
				reg_LD_rd = LD1rdtot * (reg_LS_used / total_LD_firms);
				reg_LD_en = LDentot * (reg_LS_used / total_LD_firms);
			}

			// Total regional labor demand
			reg_LD = reg_LS_used + reg_LD_rd + reg_LD_en;

			// Allocate regional labor supply proportionally to total labor demand
			double reg_LS = (LD > 0 && LS > 0) ? LS * (reg_LD / LD) : 0;

			// Regional unemployment rate = (reg_LS - reg_LD) / reg_LS
			reg_U[rr - 1] = (reg_LS > 0) ? (reg_LS - reg_LD) / reg_LS : 0;
		}
	}

	// Update wage rate
	WAGE();
}

void WAGE(void)
{
	if (u_low == 0.05)
	{
		if (U(2) < u_low)
		{
			U(2) = u_low;
		}
		d_U = (U(1) - U(2)) / U(2);
	}
	else
	{
		d_U = (U(1) - U(2));
	}

	d_cpi = (cpi(1) - cpi(2)) / cpi(2);

	d_Am = kappa * d_Am + (1 - kappa) * ((Am(1) - Am(2)) / Am(2));

	dw = d_cpi_target + psi1 * (d_cpi - d_cpi_target) + psi2 * d_Am - psi3 * d_U;

	if (dw > mdw)
	{
		dw = mdw;
	}
	if (dw < (-mdw))
	{
		dw = (-mdw);
	}

	w(1) = w(2) * (1 + dw);

	if (w(1) < w_min - 0.001)
	{
		w(1) = w_min;
	}
}

void GOV_BUDGET(void)
{
	// If outstanding government debt is greater than 0, need to take bond repayments & interest into account when calculating borrowing requirement
	if (GB(2) > 0)
	{
		TransferCB = ProfitCB(2);
		InterestBonds = r_bonds * GB(2);
		InterestBonds_cb = r_bonds * GB_cb(2);
		Deficit = G + r_bonds * GB(2) + Bailout + EntryCosts - Taxes - TransferCB - Taxes_CO2 - Taxes_e_shock - Taxes_f_shock + Subsidy_Exp;
		if ((-Deficit) > GB(2))
		{
			PSBR = Deficit;
			BondRepayments_cb = 0;
			for (i = 1; i <= NB; i++)
			{
				InterestBonds_b(i) = r_bonds * GB_b(2, i);
				BondRepayments_b(i) = 0;
			}
		}
		else if ((-Deficit) > GB_b.Row(2).Sum())
		{
			PSBR = Deficit + bonds_share * GB_cb(2);
			BondRepayments_cb = bonds_share * GB_cb(2);
			for (i = 1; i <= NB; i++)
			{
				InterestBonds_b(i) = r_bonds * GB_b(2, i);
				BondRepayments_b(i) = 0;
			}
		}
		else
		{
			PSBR = Deficit + bonds_share * GB(2);
			BondRepayments_cb = bonds_share * GB_cb(2);
			for (i = 1; i <= NB; i++)
			{
				InterestBonds_b(i) = r_bonds * GB_b(2, i);
				BondRepayments_b(i) = bonds_share * GB_b(2, i);
			}
		}
	}
	else
	{
		// If gov. debt is negative, government earns reserve rate on deposits with CB
		InterestBonds = -r_cbreserves * GB(2);
		InterestBonds_cb = -r_cbreserves * GB_cb(2);
		TransferCB = ProfitCB(2);
		BondRepayments_cb = 0;
		for (i = 1; i <= NB; i++)
		{
			InterestBonds_b(i) = 0;
			BondRepayments_b(i) = 0;
		}
		Deficit = G - r_cbreserves * GB(2) + Bailout + EntryCosts - Taxes - TransferCB - Taxes_CO2 + Subsidy_Exp;
		PSBR = Deficit;
	}

	// If government debt is smaller than 0 it is treated as a government deposit at the CB. This is first run down before new borrowing happens
	if (PSBR > 0 && GB(2) < 0)
	{
		if ((-GB(2)) >= PSBR)
		{
			GB(1) += PSBR;
			GB_cb(1) += PSBR;
			PSBR = 0;
		}
		else
		{
			PSBR = PSBR + GB(2);
			GB_cb(1) = 0;
			GB(1) = 0;
		}
	}

	// Government needs to borrow
	if (PSBR >= 0)
	{
		// Determine supply of new bonds and possibly banks' demand for bonds
		NewBonds = PSBR;
		for (i = 1; i <= NB; i++)
		{
			if (BankProfits(i) > 0)
			{
				BankProfits_temp(i) = (1 - aliqb) * BankProfits(i);
			}
			else
			{
				BankProfits_temp(i) = 0;
			}
		}

		for (i = 1; i <= NB; i++)
		{
			bonds_dem(i) = max(0.0, varphi * Loans_b(1, i) - GB_b(1, i));
		}

		bonds_dem_tot = bonds_dem.Sum();

		for (i = 1; i <= NB; i++)
		{
			// If there is excess demand for bonds, banks buy minimum between their demand and a share determined by their relative profits
			if (bonds_dem_tot >= PSBR & bonds_dem(i) >= (BankProfits_temp(i) / BankProfits_temp.Sum()) * PSBR)
			{
				bonds_purchased(i) = (BankProfits_temp(i) / BankProfits_temp.Sum()) * PSBR;
				GB_b(1, i) += bonds_purchased(i);
				GB(1) += bonds_purchased(i);
				Outflows(i) += bonds_purchased(i);
				NewBonds -= bonds_purchased(i);
			}
			else if (bonds_dem_tot >= PSBR & bonds_dem(i) < (BankProfits_temp(i) / BankProfits_temp.Sum()) * PSBR)
			{
				bonds_purchased(i) = bonds_dem(i);
				GB_b(1, i) += bonds_purchased(i);
				GB(1) += bonds_purchased(i);
				Outflows(i) += bonds_purchased(i);
				NewBonds -= bonds_purchased(i);
			}
			// If there is excess supply of bonds, demand is fully satisfied
			else if (bonds_dem_tot < PSBR)
			{
				bonds_purchased(i) = bonds_dem(i);
				GB_b(1, i) += bonds_purchased(i);
				GB(1) += bonds_purchased(i);
				Outflows(i) += bonds_purchased(i);
				NewBonds -= bonds_purchased(i);
			}
		}
		// Central bank buys remaining bonds
		GB_cb(1) += max(0.0, NewBonds);
		GB(1) += max(0.0, NewBonds);
	}
	// Government is running a surplus
	else
	{
		// If surplus is sufficient to repay all outstanding bonds held by banks, repay them and then repay the CB (possibly making GB_cb negative)
		if ((-PSBR) >= GB_b.Row(2).Sum())
		{
			for (i = 1; i <= NB; i++)
			{
				Inflows(i) += GB_b(2, i);
				GB(1) -= GB_b(2, i);
				PSBR += GB_b(2, i);
				GB_b(1, i) = 0;
			}
			GB_cb(1) += PSBR;
			GB(1) += PSBR;
		}
		// Otherwise repay on bonds held by banks
		else
		{
			Bond_share = GB_b.Row(2) / GB_b.Row(2).Sum();
			for (i = 1; i <= NB; i++)
			{
				Inflows(i) -= (PSBR * Bond_share(i));
				GB_b(1, i) += (PSBR * Bond_share(i));
				GB(1) += (PSBR * Bond_share(i));
			}
		}
	}

	// Make interest and principal payments on bonds
	for (i = 1; i <= NB; i++)
	{
		if (GB_b(1, i) > 0)
		{
			Inflows(i) += InterestBonds_b(i) + BondRepayments_b(i);
			GB_b(1, i) -= BondRepayments_b(i);
			GB(1) -= BondRepayments_b(i);
		}
		else
		{
			Inflows(i) += InterestBonds_b(i);
		}
	}

	if (GB_cb(1) > 0)
	{
		GB_cb(1) -= BondRepayments_cb;
		GB(1) -= BondRepayments_cb;
	}
}

void TAYLOR(void)
{
	// Update monetary policy rate & all other rates linked to it
	if (d_cpi_target < 0.02)
	{
		inflation_a = cpi(1) / cpi(5) - 1;
		r_a = (r_base + taylor1 * (inflation_a - d_cpi_target_a) + taylor2 * (ustar - U(1)));
		r = taylor * r + (1 - taylor) * (pow((1 + r_a), 0.25) - 1);
	}
	else
	{
		inflation_a = (cpi(1) / cpi(2)) - 1;
		r_a = (r_base + taylor1 * (inflation_a - d_cpi_target_a) + taylor2 * (ustar - U(1)));
		r = taylor * r + (1 - taylor) * r_a;
	}

	if (r <= 0)
	{
		r = 0.000001;
	}

	r_depo = r * (1 - bankmarkdown);
	r_cbreserves = r * (1 - centralbankmarkdown);

	r_bonds = r * (1 - bondsmarkdown);
	r_deb = r * (1 + bankmarkup);
}
