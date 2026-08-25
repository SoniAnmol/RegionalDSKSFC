#include "module_macro_sfc.h"

static void UPDATE_UNEMPLOYMENT_RATES(void)
{
	if (LS > 0)
	{
		U(1) = (LS - LD) / LS;
	}
	else
	{
		U(1) = 0;
	}

	if (NR <= 0)
	{
		return;
	}

	std::vector<double> reg_LD_totals(NR, 0.0);
	double mapped_reg_LD_total = 0;
	double total_LD_firms = LD1tot + LD2tot;

	for (int rr = 1; rr <= NR; ++rr)
	{
		double reg_LD_firms = 0;

		for (int ii = 1; ii <= N1; ++ii)
		{
			if (region_firm_assignment_K[ii - 1] == rr)
			{
				reg_LD_firms += Ld1(ii);
			}
		}

		for (int jj = 1; jj <= N2; ++jj)
		{
			if (region_firm_assignment_C[jj - 1] == rr)
			{
				reg_LD_firms += Ld2(jj);
			}
		}

		double reg_LD_rd = 0;
		double reg_LD_en = 0;
		if (total_LD_firms > 0)
		{
			reg_LD_rd = LD1rdtot * (reg_LD_firms / total_LD_firms);
			reg_LD_en = LDentot * (reg_LD_firms / total_LD_firms);
		}

		double reg_LD_total = reg_LD_firms + reg_LD_rd + reg_LD_en;
		reg_LD_totals[rr - 1] = reg_LD_total;
		mapped_reg_LD_total += reg_LD_total;
	}

	for (int rr = 1; rr <= NR; ++rr)
	{
		double reg_LD_total = reg_LD_totals[rr - 1];

		if (flag_regional_labor == 1)
		{
			// Regional labour supply is a STATE driven by exogenous shares sigma_r:
			//   LS_r = LS * LS_region_share[r].
			// Do NOT allocate supply by regional labour demand here.
			double sigma_r = ((int)LS_region_share.size() == NR) ? LS_region_share[rr - 1] : 1.0 / NR;
			reg_LS[rr - 1] = (LS > 0) ? LS * sigma_r : 0;

			// Employment and unemployment with safe handling of LS_r <= 0.
			double L_r = std::min(reg_LD_total, reg_LS[rr - 1]);
			if (L_r < 0)
				L_r = 0;
			reg_U[rr - 1] = (reg_LS[rr - 1] > 0) ? (reg_LS[rr - 1] - L_r) / reg_LS[rr - 1] : 0;
		}
		else
		{
			// Legacy (flag off): demand-proportional regional labour supply (back-compat).
			if (LS > 0 && mapped_reg_LD_total > 0)
			{
				reg_LS[rr - 1] = LS * (reg_LD_total / mapped_reg_LD_total);
			}
			else if (LS > 0)
			{
				reg_LS[rr - 1] = LS / NR;
			}
			else
			{
				reg_LS[rr - 1] = 0;
			}

			reg_U[rr - 1] = (reg_LS[rr - 1] > 0) ? (reg_LS[rr - 1] - reg_LD_total) / reg_LS[rr - 1] : 0;
		}
	}
}

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
	if (flag_regional_labor == 1 && NR > 0)
	{
		// ===== Per-region labour rationing (phi_r) =====
		// Region-r firms can only use region-r labour. If regional production
		// labour demand exceeds the labour available for production in region r,
		// scale that region's firm labour (and hence output) by phi_r.
		double total_prod_LD = LD1tot + LD2tot;

		for (int rr = 1; rr <= NR; ++rr)
		{
			// Production labour demand for region r
			double prod_LD_r = 0.0;
			for (int ii = 1; ii <= N1; ++ii)
				if (region_firm_assignment_K[ii - 1] == rr)
					prod_LD_r += Ld1(ii);
			for (int jj = 1; jj <= N2; ++jj)
				if (region_firm_assignment_C[jj - 1] == rr)
					prod_LD_r += Ld2(jj);

			// Apportion national R&D + energy labour to region r by production-labour share
			double share_r = (total_prod_LD > 0) ? prod_LD_r / total_prod_LD : 1.0 / NR;
			double reg_LD_rd_r = LD1rdtot * share_r;
			double reg_LD_en_r = LDentot * share_r;

			// Regional labour supply (state) and labour available for production:
			//   LSe_r = LS_r - LD^rd_r - LD^en_r,  LS_r = LS * sigma_r
			double sigma_r = ((int)LS_region_share.size() == NR) ? LS_region_share[rr - 1] : 1.0 / NR;
			double LS_r = LS * sigma_r;
			double LSe_r = LS_r - reg_LD_rd_r - reg_LD_en_r;
			if (LSe_r < 0)
				LSe_r = 0;

			// Ration only if regional production labour demand exceeds available labour
			if (prod_LD_r > LSe_r && prod_LD_r > 0)
			{
				double phi_r = LSe_r / prod_LD_r;

				// Scale C-firms in region r
				for (int jj = 1; jj <= N2; ++jj)
				{
					if (region_firm_assignment_C[jj - 1] == rr)
					{
						Ld2(jj) = Ld2(jj) * phi_r;
						Q2(jj) = Ld2(jj) * A2e(jj);
					}
				}

				// Scale K-firms in region r and their machine orders (mirror national logic)
				for (int ii = 1; ii <= N1; ++ii)
				{
					if (region_firm_assignment_K[ii - 1] != rr)
						continue;
					Qpast = Q1(ii);
					if (Qpast > 0)
					{
						Ld1(ii) = Ld1(ii) * phi_r;
						Q1(ii) = floor(Ld1(ii) * ((1 - shocks_labprod1(ii)) * A1p(ii) * a));
						reduction = Qpast - Q1(ii);
						while (reduction > 0)
						{
							ranj = int(ran1(p_seed) * N1 * N2) % N2 + 1;
							if (Match(ranj, ii) == 1 && I(ranj) > 0)
							{
								Ipast = I(ranj);
								I(ranj) = floor((I(ranj) / dim_mach) * Q1(ii) / Qpast) * dim_mach;
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
			}
		}

		// Recompute national totals after regional rationing
		LD1tot = 0;
		LD2tot = 0;
		for (i = 1; i <= N1; i++)
		{
			LD1tot += Ld1(i);
		}
		for (j = 1; j <= N2; j++)
		{
			LD2tot += Ld2(j);
		}
		LSe = LS - (LD1rdtot + LDentot) - LD1tot - LD2tot;
		if (LSe < 0)
			LSe = 0;
	}
	else if (LD2tot + LD1tot <= LSe)
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
	// Reset regional accounting variables at the start of each period
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

	// Note: reg_Q1tot and reg_Q2tot are computed in REGIONAL_UPDATE() after ENTRYEXIT
	// to ensure they reflect the same firm population as other regional aggregates

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

		// Accumulate regional Am_a and Am_en for C-firms
		if (NR > 0)
		{
			int rr = region_firm_assignment_C[j - 1];
			if (rr >= 1 && rr <= NR && LD2 > 0)
			{
				reg_Am_a[rr - 1] += Ld2(j) / LD2 * A2e(j);
			}
			if (rr >= 1 && rr <= NR && (D2_en_TOT + D1_en_TOT) > 0)
			{
				reg_Am_en[rr - 1] += D2_en(j) / (D2_en_TOT + D1_en_TOT) * A2e_en(j);
			}
		}
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

		// Accumulate regional Am_a and Am_en for K-firms
		if (NR > 0)
		{
			int rr = region_firm_assignment_K[i - 1];
			if (rr >= 1 && rr <= NR && LD2 > 0)
			{
				reg_Am_a[rr - 1] += Ld1(i) / LD2 * A1p(i) * a;
			}
			if (rr >= 1 && rr <= NR && (D2_en_TOT + D1_en_TOT) > 0)
			{
				reg_Am_en[rr - 1] += D1_en(i) / (D2_en_TOT + D1_en_TOT) * A1p_en(i);
			}
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

	// Determine national and regional unemployment rates
	UPDATE_UNEMPLOYMENT_RATES();

	// Update wage rate
	WAGE();
}

void REGIONAL_UPDATE(void)
{
	// Recalculate regional aggregates post-ENTRYEXIT to match national timing
	// This ensures regional aggregates reflect the same state as national aggregates
	// ENTRYEXIT modifies: prices (p1, p2), Loans_2, Inventories, N, and firm region assignments
	if (NR > 0)
	{
		// Reset all regional accumulators
		for (int rr = 0; rr < NR; ++rr)
		{
			reg_GDP_n[rr] = 0;
			reg_Q1[rr] = 0;
			reg_Q2[rr] = 0;
			reg_Q1tot[rr] = 0;
			reg_Q2tot[rr] = 0;
			reg_Loans_2[rr] = 0;
			reg_CreditDemand_all[rr] = 0;
			reg_CreditSupply_all[rr] = 0;
			reg_Inventories[rr] = 0;
			reg_N[rr] = 0;
			reg_N1[rr] = 0;
			reg_N2[rr] = 0;
			reg_S1[rr] = 0;
			reg_S2[rr] = 0;
			reg_K[rr] = 0;
			reg_Investment[rr] = 0;
			reg_Investment_n[rr] = 0;
			reg_ReplacementInvestment_r[rr] = 0;
			reg_EnergyPayments[rr] = 0;
			reg_Wages[rr] = 0;
			reg_EI[rr] = 0;
			reg_SI[rr] = 0;
			reg_Ld1[rr] = 0;
			reg_Ld2[rr] = 0;
			reg_Emiss1[rr] = 0;
			reg_Emiss2[rr] = 0;
			reg_Emiss1_TOT[rr] = 0;
			reg_Emiss2_TOT[rr] = 0;
			reg_Pi1[rr] = 0;
			reg_Pi2[rr] = 0;
			reg_Pitot1[rr] = 0;
			reg_Pitot2[rr] = 0;
			reg_Dividends_1[rr] = 0; // Initialize dividends for K-firms
			reg_Dividends_2[rr] = 0;
			reg_NW_1[rr] = 0;
			reg_NW2[rr] = 0;
			reg_Deposits1[rr] = 0;
			reg_Deposits2[rr] = 0;
			reg_CapitalStock1[rr] = 0;
			reg_CapitalStock2[rr] = 0;
			reg_CapitalStock[rr] = 0;
			reg_H1[rr] = 0;
			reg_H2[rr] = 0;
		}

		// Recalculate regional GDP_n (depends on prices which change in ENTRYEXIT)
		// and firm counts (firms can relocate during ENTRYEXIT)
		// Also aggregate K-firm variables
		for (int ii = 1; ii <= N1; ++ii)
		{
			int rr = region_firm_assignment_K[ii - 1];
			if (rr >= 1 && rr <= NR)
			{
				reg_GDP_n[rr - 1] += Q1(ii) * dim_mach * p1(ii) * a;
				reg_Q1[rr - 1] += Q1(ii);
				reg_Q1tot[rr - 1] += Q1(ii);
				reg_N1[rr - 1]++;
				reg_S1[rr - 1] += S1(ii);
				reg_Ld1[rr - 1] += Ld1(ii);
				reg_Emiss1_TOT[rr - 1] += Emiss1(ii);
				reg_EnergyPayments[rr - 1] += EnergyPayments_1(ii);
				reg_Wages[rr - 1] += Wages_1(ii);
				reg_Pi1[rr - 1] += Pi1(ii);
				reg_Pitot1[rr - 1] += Pi1(ii);
				reg_Dividends_1[rr - 1] += Dividends_1(ii); // Aggregate dividends for K-firms
				// Use Deposits_1 directly: NW_1 = Deposits_1 (as SFC_CHECK computes),
				// but NW_1(1,ii) is not updated until SFC_CHECK runs, so it holds
				// the previous period's value here. Deposits_1(1,ii) is current.
				reg_NW_1[rr - 1] += Deposits_1(1, ii);
				reg_Deposits1[rr - 1] += Deposits_1(1, ii);
				// Note: K-firms don't have capital stock (they produce machines, don't hold them)
			}
		}

		// Recalculate regional C-firm aggregates
		for (int jj = 1; jj <= N2; ++jj)
		{
			int rr = region_firm_assignment_C[jj - 1];
			if (rr >= 1 && rr <= NR)
			{
				reg_GDP_n[rr - 1] += Q2(jj) * p2(jj);
				reg_Q2[rr - 1] += Q2(jj);
				reg_Q2tot[rr - 1] += Q2(jj);
				reg_Loans_2[rr - 1] += Loans_2(1, jj);
				reg_Inventories[rr - 1] += Inventories(1, jj);
				reg_N[rr - 1] += N(1, jj);
				reg_N2[rr - 1]++;
				reg_S2[rr - 1] += S2(1, jj);
				reg_K[rr - 1] += K(jj);
				reg_Investment[rr - 1] += I(jj);
				reg_Investment_n[rr - 1] += EI_n(jj) + SI_n(jj);
				reg_EI[rr - 1] += EI(1, jj);
				reg_ReplacementInvestment_r[rr - 1] += SI(jj);
				reg_SI[rr - 1] += SI(jj);
				reg_EnergyPayments[rr - 1] += EnergyPayments_2(jj);
				reg_Wages[rr - 1] += Wages_2(jj);
				reg_Ld2[rr - 1] += Ld2(jj);
				reg_Emiss2[rr - 1] += Emiss2(jj);
				reg_Emiss2_TOT[rr - 1] += Emiss2(jj);
				reg_Pi2[rr - 1] += Pi2(jj);
				reg_Pitot2[rr - 1] += Pi2(jj);
				// Compute NW_2 from components directly, matching SFC_CHECK's formula:
				// NW_2 = CapitalStock + deltaCapitalStock + Inventories + Deposits_2 - Loans_2.
				// NW_2(1,jj) is only updated in ENTRYEXIT and not refreshed after
				// BANKING/BAILOUT/SETTLEMENT modify the balance-sheet components.
				reg_NW2[rr - 1] += CapitalStock(1, jj) + deltaCapitalStock(1, jj) + Inventories(1, jj) + Deposits_2(1, jj) - Loans_2(1, jj);
				reg_Deposits2[rr - 1] += Deposits_2(1, jj);
				reg_CapitalStock2[rr - 1] += CapitalStock(1, jj);
				reg_Dividends_2[rr - 1] += Dividends_2(jj); // Aggregate dividends for C-firms
				reg_CreditDemand_all[rr - 1] += CreditDemand(jj);
			}
		}

		// Calculate total regional capital stock (C-firms only, matching national CapitalStock)
		for (int rr = 1; rr <= NR; ++rr)
		{
			reg_CapitalStock[rr - 1] = reg_CapitalStock2[rr - 1];
		}

		// Allocate credit supply proportionally to regional demand so regional totals sum to national
		double total_reg_credit_demand = 0;
		for (int rr = 1; rr <= NR; ++rr)
		{
			total_reg_credit_demand += reg_CreditDemand_all[rr - 1];
		}
		if (total_reg_credit_demand > 0)
		{
			for (int rr = 1; rr <= NR; ++rr)
			{
				reg_CreditSupply_all[rr - 1] = CreditSupply_all * (reg_CreditDemand_all[rr - 1] / total_reg_credit_demand);
			}
		}
		else
		{
			for (int rr = 1; rr <= NR; ++rr)
			{
				reg_CreditSupply_all[rr - 1] = 0;
			}
		}

		// Compute total regional GDP_n for allocation ratios (household NW, etc.)
		// Do NOT overwrite national GDP_n(1) — it was correctly computed by MACRO()
		// before ENTRYEXIT. Overwriting with post-ENTRYEXIT prices injects
		// entry/exit noise into the nominal GDP growth series.
		double total_regional_GDP = 0;
		for (int rr = 1; rr <= NR; ++rr)
		{
			total_regional_GDP += reg_GDP_n[rr - 1];
		}

		// Allocate household net worth to regions based on GDP share.
		// Use Deposits_h(1) as the NW_h base: NW_h(1) = Deposits_h(1) by
		// definition (SFC_CHECK), but NW_h(1) is only assigned there and holds
		// the previous period's value here. Deposits_h(1) is live and current.
		if (total_regional_GDP > 0)
		{
			for (int rr = 1; rr <= NR; ++rr)
			{
				reg_NW_h[rr - 1] = Deposits_h(1) * (reg_GDP_n[rr - 1] / total_regional_GDP);
			}
		}

		// Calculate regional average productivity and derived variables
		std::vector<double> reg_LD_totals(NR, 0.0);
		double mapped_reg_LD_total = 0;

		// Compute regional normalised Herfindahl indices for K-firms and C-firms.
		// Use the same formula as the national index: H = (sum(s_i^2) - 1/n) / (1 - 1/n)
		for (int ii = 1; ii <= N1; ++ii)
		{
			int rr = region_firm_assignment_K[ii - 1];
			if (rr >= 1 && rr <= NR && reg_Q1tot[rr - 1] > 0)
			{
				double s = Q1(ii) / reg_Q1tot[rr - 1];
				reg_H1[rr - 1] += s * s;
			}
		}
		for (int jj = 1; jj <= N2; ++jj)
		{
			int rr = region_firm_assignment_C[jj - 1];
			if (rr >= 1 && rr <= NR && reg_Q2tot[rr - 1] > 0)
			{
				double s = Q2(jj) / reg_Q2tot[rr - 1];
				reg_H2[rr - 1] += s * s;
			}
		}
		for (int rr = 1; rr <= NR; ++rr)
		{
			double n1 = reg_N1[rr - 1];
			double n2 = reg_N2[rr - 1];
			reg_H1[rr - 1] = (n1 > 1) ? (reg_H1[rr - 1] - 1.0 / n1) / (1.0 - 1.0 / n1) : 0.0;
			reg_H2[rr - 1] = (n2 > 1) ? (reg_H2[rr - 1] - 1.0 / n2) / (1.0 - 1.0 / n2) : 0.0;
		}

		for (int rr = 1; rr <= NR; ++rr)
		{
			double reg_A1_sum = 0;
			double reg_A2_sum = 0;
			double reg_A1_weight = 0;
			double reg_A2_weight = 0;

			// Aggregate productivity from K-firms in this region (weighted by sales)
			for (int ii = 1; ii <= N1; ++ii)
			{
				if (region_firm_assignment_K[ii - 1] == rr && nclient(ii) >= 1)
				{
					reg_A1_sum += A1p(ii) * S1(ii);
					reg_A1_weight += S1(ii);
				}
			}

			// Aggregate productivity from C-firms in this region (weighted by sales)
			for (int jj = 1; jj <= N2; ++jj)
			{
				if (region_firm_assignment_C[jj - 1] == rr)
				{
					reg_A2_sum += A2(jj) * S2(1, jj);
					reg_A2_weight += S2(1, jj);
				}
			}

			// Calculate regional average productivities
			reg_Am1[rr - 1] = (reg_A1_weight > 0) ? reg_A1_sum / reg_A1_weight : 0;
			reg_Am2[rr - 1] = (reg_A2_weight > 0) ? reg_A2_sum / reg_A2_weight : 0;
			reg_Am[rr - 1] = (reg_A1_weight + reg_A2_weight > 0)
								 ? (reg_A1_sum + reg_A2_sum) / (reg_A1_weight + reg_A2_weight)
								 : 0;

			// Calculate regional real GDP
			reg_GDP_r[rr - 1] = reg_Q1[rr - 1] * dim_mach + reg_Q2[rr - 1];

			// Calculate regional real investment
			reg_Investment_r[rr - 1] = reg_EI[rr - 1] + reg_SI[rr - 1];

			// Aggregate regional labour DEMAND (firms + R&D + energy apportioned by firm-labour share).
			// Regional labour SUPPLY is set separately below from sigma_r (state), not from demand.
			double reg_LD_firms = reg_Ld1[rr - 1] + reg_Ld2[rr - 1];
			double total_LD_firms = LD1tot + LD2tot;
			double reg_LD_rd = 0;
			double reg_LD_en = 0;
			if (total_LD_firms > 0)
			{
				reg_LD_rd = LD1rdtot * (reg_LD_firms / total_LD_firms);
				reg_LD_en = LDentot * (reg_LD_firms / total_LD_firms);
			}
			double reg_LD_total = reg_LD_firms + reg_LD_rd + reg_LD_en;
			reg_LD_totals[rr - 1] = reg_LD_total;
			mapped_reg_LD_total += reg_LD_total;

			// Calculate regional cumulative emissions
			double national_emiss_tot = Emiss1_TOT + Emiss2_TOT + Emiss_en;
			if (national_emiss_tot > 0 && Cum_emissions > 0)
			{
				double reg_Emiss_total = reg_Emiss1_TOT[rr - 1] + reg_Emiss2_TOT[rr - 1] + reg_Emiss_en[rr - 1];
				double share = reg_Emiss_total / national_emiss_tot;
				reg_Cum_emissions[rr - 1] = Cum_emissions * share;
			}
			else
			{
				reg_Cum_emissions[rr - 1] = 0;
			}
		}

		for (int rr = 1; rr <= NR; ++rr)
		{
			if (flag_regional_labor == 1)
			{
				// Regional labour supply is a STATE driven by exogenous shares sigma_r:
				//   LS_r = LS * LS_region_share[r]. Demand is NOT used to allocate supply.
				double sigma_r = ((int)LS_region_share.size() == NR) ? LS_region_share[rr - 1] : 1.0 / NR;
				reg_LS[rr - 1] = (LS > 0) ? LS * sigma_r : 0;

				double L_r = std::min(reg_LD_totals[rr - 1], reg_LS[rr - 1]);
				if (L_r < 0)
					L_r = 0;
				reg_U[rr - 1] = (reg_LS[rr - 1] > 0) ? (reg_LS[rr - 1] - L_r) / reg_LS[rr - 1] : 0;

				// Explicit rate and headcount (disambiguate reg_U usage; Phase 5B)
				if ((int)reg_U_rate.size() == NR)
					reg_U_rate[rr - 1] = reg_U[rr - 1];
				if ((int)reg_UN.size() == NR)
					reg_UN[rr - 1] = std::max(0.0, reg_LS[rr - 1] - L_r);
			}
			else
			{
				// Legacy (flag off): demand-proportional regional labour supply (back-compat).
				if (LS > 0 && mapped_reg_LD_total > 0)
				{
					reg_LS[rr - 1] = LS * (reg_LD_totals[rr - 1] / mapped_reg_LD_total);
				}
				else if (LS > 0)
				{
					reg_LS[rr - 1] = LS / NR;
				}
				else
				{
					reg_LS[rr - 1] = 0;
				}

				reg_U[rr - 1] = (reg_LS[rr - 1] > 0) ? (reg_LS[rr - 1] - reg_LD_totals[rr - 1]) / reg_LS[rr - 1] : 0;
			}
		}

		// ===== Phase 4 / 5B: regional disposable income, consumption, deposits =====
		// True regional household accounts. Households now hold regional deposits
		// (reg_Dh) that accumulate disposable income minus consumption. reg_C is an
		// accounting decomposition of national Consumption by disposable-income share.
		//   UN_r       = max(0, LS_r - L_r)                 (unemployed persons)
		//   Benefits_r = wu_rg[r] * w_lag_r * UN_r          (replacement_rate_r = wu_rg[r])
		//   YD_r       = Wages_r + Benefits_r + Dividends_r + i*Dh_lag_r - Taxes_h_r
		//   C_r        = Consumption * YD_r / sum_r YD_r     (fallback: by LS share)
		//   Dh_pre_r   = Dh_lag_r + YD_r - C_r
		if (flag_regional_labor == 1 && (int)reg_YD.size() == NR && (int)reg_C.size() == NR &&
			(int)reg_Dh_lag.size() == NR && (int)reg_Dh_pre_migration.size() == NR)
		{
			double total_reg_YD = 0.0;
			for (int rr = 0; rr < NR; ++rr)
			{
				// Unemployed persons (count), not the rate
				double unemployed_r = ((int)reg_UN.size() == NR) ? reg_UN[rr] : reg_U[rr] * reg_LS[rr];
				if (unemployed_r < 0)
					unemployed_r = 0;

				// Regional benefits: replacement_rate_r (= wu_rg[r]) * lagged regional wage * unemployed
				double wage_lag_r = ((int)reg_w_past.size() == NR) ? reg_w_past[rr] : w(2);
				double repl_rate_r = ((int)wu_rg.size() == NR) ? wu_rg[rr] : wu;
				double benefit_r = repl_rate_r * wage_lag_r * unemployed_r;
				reg_Benefits[rr] = benefit_r;

				double div_r = reg_Dividends_1[rr] + reg_Dividends_2[rr];
				double interest_r = r_depo * reg_Dh_lag[rr]; // interest on regional deposits
				double taxes_h_r = 0.0;						 // no regional household tax yet

				reg_YD[rr] = reg_Wages[rr] + benefit_r + div_r + interest_r - taxes_h_r;
				total_reg_YD += reg_YD[rr];
			}

			// Liquidity-feasible regional consumption decomposition.
			// Desired regional consumption follows the disposable-income share (fallback
			// LS share) but is capped at available household resources (lagged regional
			// deposits + disposable income), so pre-migration regional deposits can never
			// go negative. National Consumption is never forced onto regions by making
			// their deposits negative: any infeasible remainder is reported as a residual.
			std::vector<double> reg_C_raw(NR, 0.0);
			double sum_raw = 0.0;
			for (int rr = 0; rr < NR; ++rr)
			{
				// 1. Desired regional consumption (by disposable-income share)
				double share;
				if (total_reg_YD > 1e-12)
					share = reg_YD[rr] / total_reg_YD;
				else
					share = ((int)LS_region_share.size() == NR) ? LS_region_share[rr] : 1.0 / NR;
				double desired_r = Consumption * share;
				if (desired_r < 0.0)
					desired_r = 0.0; // no negative consumption

				// 2. Available household resources before migration
				double resources_r = reg_Dh_lag[rr] + reg_YD[rr];
				if (resources_r < 0.0)
					resources_r = 0.0;

				// 3. Cap regional consumption at available resources
				double c_raw = (desired_r < resources_r) ? desired_r : resources_r;
				reg_C_raw[rr] = c_raw;
				sum_raw += c_raw;
			}

			// 4. Rescale raw consumption to national Consumption only if feasible
			//    (scaling DOWN). If raw resources fall short, keep the feasible
			//    expenditure and record the unallocated national consumption.
			diag_reg_C_unallocated = 0.0;
			if (sum_raw >= Consumption && sum_raw > 1e-12)
			{
				double cscale = Consumption / sum_raw;
				for (int rr = 0; rr < NR; ++rr)
					reg_C[rr] = reg_C_raw[rr] * cscale;
			}
			else
			{
				for (int rr = 0; rr < NR; ++rr)
					reg_C[rr] = reg_C_raw[rr];
				diag_reg_C_unallocated = Consumption - sum_raw;
			}

			// 5. Pre-migration regional deposits (income/consumption applied)
			for (int rr = 0; rr < NR; ++rr)
			{
				reg_Dh_pre_migration[rr] = reg_Dh_lag[rr] + reg_YD[rr] - reg_C[rr];
			}
		}
	}

	if (LS > 0)
	{
		U(1) = (LS - LD) / LS;
	}
	else
	{
		U(1) = 0;
	}
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

	// ===== Phase 3A: regional wage setting (income/benefit/migration use) =====
	// Firms still pay the national wage; reg_w feeds household income, benefits
	// and migration utility only (cost-side routing is Phase 3B, postponed).
	//   wdot_reg_r = pi* + psi1(pi - pi*) + psi2*dAm_r - psi3*du_r
	//   wdot_r     = chi_w*wdot_nat + (1 - chi_w)*wdot_reg_r,  |wdot_r| <= dwage_max
	//   reg_w[r]   = reg_w_past[r] * (1 + wdot_r)
	if (flag_regional_labor == 1 && NR > 0 &&
		(int)reg_w.size() == NR && (int)reg_w_past.size() == NR &&
		(int)reg_U.size() == NR && (int)reg_U_past.size() == NR &&
		(int)reg_Am.size() == NR && (int)reg_Am_past.size() == NR)
	{
		for (int rr = 0; rr < NR; ++rr)
		{
			// Regional productivity growth
			double dAm_r = 0.0;
			if (reg_Am_past[rr] > 1e-12)
				dAm_r = (reg_Am[rr] - reg_Am_past[rr]) / reg_Am_past[rr];

			// Regional unemployment change (mirror national specification)
			double dU_r;
			if (u_low == 0.05)
			{
				double U_past_r = reg_U_past[rr];
				if (U_past_r < u_low)
					U_past_r = u_low;
				dU_r = (reg_U[rr] - U_past_r) / U_past_r;
			}
			else
			{
				dU_r = (reg_U[rr] - reg_U_past[rr]);
			}

			double dw_reg = d_cpi_target + psi1 * (d_cpi - d_cpi_target) + psi2 * dAm_r - psi3 * dU_r;

			// Mix with national wage growth (chi_w = 1 -> fully national, baseline)
			double dw_r = chi_w * dw + (1.0 - chi_w) * dw_reg;

			// Symmetric bound on regional wage growth
			if (dw_r > dwage_max)
				dw_r = dwage_max;
			if (dw_r < -dwage_max)
				dw_r = -dwage_max;

			reg_w[rr] = reg_w_past[rr] * (1 + dw_r);
			if (reg_w[rr] < w_min - 0.001)
				reg_w[rr] = w_min;
		}
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

void RG_BLOCK_SP(void)
{
	// Regional Government Block — Phase 1: Social Protection
	// Called BEFORE PAY_LAB_INV so that Benefits includes regional SP when
	// household deposits and consumption are computed.
	// Only active when NR > 0 and gamma_bar > 0 (gamma_bar=0 ensures complete neutrality)

	if (NR <= 0 || gamma_bar <= 0.0)
	{
		return;
	}

	// Use current-period national unemployment from LABOR (LS, LD are fresh).
	// This guarantees SP_total = (LS-LD)*w*wu when all wu_rg are the same,
	// exactly matching DSK's G = (LS-LD)*w*wu computed inside LABOR.
	// We distribute national unemployment to regions proportionally to reg_LS
	// so that heterogeneous wu_rg still works correctly.
	UPDATE_UNEMPLOYMENT_RATES();

	double national_unemployed = std::max(0.0, (double)(LS - LD));

	// Sum reg_LS for proportional allocation
	double total_reg_LS = 0.0;
	for (int rr = 0; rr < NR; rr++)
	{
		total_reg_LS += reg_LS[rr];
	}

	SP_total = 0.0;
	for (int rr = 0; rr < NR; rr++)
	{
		double share = (total_reg_LS > 0) ? reg_LS[rr] / total_reg_LS : 1.0 / NR;
		double unemployed_r = national_unemployed * share;
		SP_rg[rr] = unemployed_r * w(2) * wu_rg[rr];
		SP_total += SP_rg[rr];
	}

	// Add SP to Benefits and G BEFORE PAY_LAB_INV
	// PAY_LAB_INV will then credit Deposits_h += (Wages + Benefits) automatically
	Benefits += SP_total;
	G += SP_total;
}

void RG_BLOCK_FISCAL(void)
{
	// Regional Government Block — Phase 2: Revenue, Grants, EA, Public Capital
	// Called between REGIONAL_UPDATE and BANKING in the simulation loop.
	// Depends on Taxes_1/Taxes_2 (from PROFIT) and reg_Wages (from REGIONAL_UPDATE).
	// SP_rg values were already set by RG_BLOCK_SP; used here for EA residual.
	// Only active when NR > 0 and gamma_bar > 0 (gamma_bar=0 ensures complete neutrality)

	if (NR <= 0 || gamma_bar <= 0.0)
	{
		return;
	}

	// Step 1: Compute national grant pool
	GRANTPOOL = gamma_bar * Taxes;

	// Step 2: Compute per-region tax-sharing receipts, grants, EA
	for (int rr = 1; rr <= NR; rr++)
	{
		// 2a: Compute regional tax base from firms in this region
		double T_K_r = 0.0;
		double T_C_r = 0.0;

		for (int ii = 1; ii <= N1; ii++)
		{
			if (region_firm_assignment_K[ii - 1] == rr)
			{
				T_K_r += Taxes_1(ii);
			}
		}

		for (int jj = 1; jj <= N2; jj++)
		{
			if (region_firm_assignment_C[jj - 1] == rr)
			{
				T_C_r += Taxes_2(jj);
			}
		}

		// 2b: Tax-sharing receipts = share of (firm taxes + wage taxes in region)
		TS_rg[rr - 1] = tau_share_rg[rr - 1] * (T_K_r + T_C_r + aliqw * reg_Wages[rr - 1]);

		// 2c: Base grant from national pool
		GT_base_rg[rr - 1] = omega_rg[rr - 1] * GRANTPOOL;

		// 2d: Top-up grant covers shortfall if own revenue + base grant insufficient for SP
		// SP_rg was already computed by RG_BLOCK_SP
		double own_revenue = TS_rg[rr - 1] + GT_base_rg[rr - 1];
		GT_topup_rg[rr - 1] = std::max(0.0, SP_rg[rr - 1] - own_revenue);

		// 2e: Total grant
		GT_rg[rr - 1] = GT_base_rg[rr - 1] + GT_topup_rg[rr - 1];

		// 2f: Total regional revenue
		REV_rg[rr - 1] = TS_rg[rr - 1] + GT_rg[rr - 1];

		// -------------------------------------------------------------------
		// 2g-rec-0: Compute current-period damage indicators from shocks_capstock
		// (shocks_capstock still holds the shock applied this period by PRODMACH)
		// -------------------------------------------------------------------
		if ((flag_adaptation == 2 || flag_adaptation == 3) && flag_capshocks > 0)
		{
			double sum_shock = 0.0;
			int n_in_rg = 0;
			for (int jj = 1; jj <= N2; jj++)
			{
				if (region_firm_assignment_C[jj - 1] == rr && exiting_2(jj) == 0)
				{
					sum_shock += shocks_capstock(jj);
					n_in_rg++;
					if (shocks_capstock(jj) >= d_bar_rec)
					{
						affected_indicator(jj) = 1.0;
						n_aff_rg[rr - 1] += 1.0;
					}
				}
			}
			Saff_rg[rr - 1] = (n_in_rg > 0) ? sum_shock / n_in_rg : 0.0;
		}

		// -------------------------------------------------------------------
		// 2g-rec-1: Recovery obligation (uses LAGGED damage to avoid same-period feedback)
		// I_Rec_{r,t} = psi_r * Y_{r,t-1} * max(0, Saff_{r,t-1} - s_bar_r)
		// -------------------------------------------------------------------
		bool rec_active = (flag_adaptation == 2 || flag_adaptation == 3);
		if (rec_active && psi_rec_rg[rr - 1] > 0.0)
		{
			double gdp_lag = std::max(reg_GDP_r_lag[rr - 1], 1.0);
			I_Rec_rg[rr - 1] = psi_rec_rg[rr - 1] * gdp_lag * std::max(0.0, Saff_rg_lag[rr - 1] - s_bar_rec_rg[rr - 1]);
		}

		// -------------------------------------------------------------------
		// 2g-rec-2: Backlog update and disbursement
		// B_rec_{r,t} = I_Rec_{r,t} + (1 - delta_imp_r) * B_rec_{r,t-1}
		// GRecPaid_{r,t} = delta_imp_r * B_rec_{r,t}
		// -------------------------------------------------------------------
		if (rec_active)
		{
			B_rec_rg[rr - 1] = I_Rec_rg[rr - 1] + (1.0 - delta_imp_rg[rr - 1]) * B_rec_rg_lag[rr - 1];
			GRecPaid_rg[rr - 1] = delta_imp_rg[rr - 1] * B_rec_rg[rr - 1];
		}

		// -------------------------------------------------------------------
		// 2g-rec-3: Central government backstop
		// TREC_{r,t} = max(0, GRecPaid - max(0, TS_{r,t} - SP_{r,t}))
		// -------------------------------------------------------------------
		if (rec_active && GRecPaid_rg[rr - 1] > 0.0)
		{
			double own_surplus = std::max(0.0, TS_rg[rr - 1] - SP_rg[rr - 1]);
			TREC_rg[rr - 1] = std::max(0.0, GRecPaid_rg[rr - 1] - own_surplus);
		}

		// -------------------------------------------------------------------
		// 2g-rec-4: Revised residual R (Step 5 of math)
		// R = REV + TREC - SP - GRecPaid
		// Adaptation carve-out from R; remainder goes to public capital
		// -------------------------------------------------------------------
		double R_rg = std::max(0.0, REV_rg[rr - 1] + TREC_rg[rr - 1] - SP_rg[rr - 1] - GRecPaid_rg[rr - 1]);
		EA_rg[rr - 1] = R_rg; // EA_rg now = residual for public capital (after recovery)

		// 2g-adapt: Adaptation investment carve-out from residual (fiscally constrained)
		double EA_pub = R_rg; // default: all residual goes to public capital
		if ((flag_adaptation == 1 || flag_adaptation == 3) && R_rg > 0.0)
		{
			double gdp_safe = std::max(reg_GDP_n[rr - 1], 1.0);
			I_adapt_rg[rr - 1] = std::min(iota_adapt_rg[rr - 1] * gdp_safe, R_rg);
			EA_pub = R_rg - I_adapt_rg[rr - 1];
		}
		else
		{
			I_adapt_rg[rr - 1] = 0.0;
		}

		// 2g-adapt-stock: Update adaptation stock (depreciation + new investment)
		K_adapt_rg[rr - 1] = K_adapt_rg_lag[rr - 1] * (1.0 - delta_adapt) + I_adapt_rg[rr - 1];

		// 2g-adapt-channels: Split adaptation investment across NC_adapt channels and update per-channel stocks
		if (flag_adaptation == 1 || flag_adaptation == 3)
		{
			double gdp_lag = std::max(reg_GDP_n[rr - 1], 1.0);
			for (int cc = 0; cc < NC_adapt; cc++)
			{
				// Proportional split of total investment across channels
				I_adapt_c_rg[cc][rr - 1] = phi_alloc_c_rg[cc][rr - 1] * I_adapt_rg[rr - 1];
				// Update channel stock with channel-specific depreciation
				K_adapt_c_rg[cc][rr - 1] = K_adapt_c_rg_lag[cc][rr - 1] * (1.0 - delta_adapt_c_rg[cc][rr - 1]) + I_adapt_c_rg[cc][rr - 1];
				// Protection threshold from LAGGED stock (temporal consistency: investment this period acts next period)
				// h = hbar × (1 - exp(-kappa × K_lag / Y))
				h_thresh_c_rg[cc][rr - 1] = hbar_c_rg[cc][rr - 1] * (1.0 - std::exp(-kappa_c_rg[cc][rr - 1] * K_adapt_c_rg_lag[cc][rr - 1] / gdp_lag));
			}
		}
		// else: h_thresh_c_rg remains 0.0 (reset in SETVARS); no fragility protection

		// 2g-adapt-omega: Shock dampening factor from LAGGED stock (temporal consistency)
		if (flag_adaptation == 1 || flag_adaptation == 3)
		{
			double gdp_safe = std::max(reg_GDP_n[rr - 1], 1.0);
			Omega_adapt_rg[rr - 1] = omega_floor_adapt +
									 (1.0 - omega_floor_adapt) * std::exp(-phi_adapt * K_adapt_rg_lag[rr - 1] / gdp_safe);
		}
		else
		{
			Omega_adapt_rg[rr - 1] = 1.0; // no dampening when protection is off
		}

		// 2h: Total regional expenditure (balanced budget: REV + TREC = SP + GRecPaid + EA)
		EXP_rg[rr - 1] = SP_rg[rr - 1] + GRecPaid_rg[rr - 1] + EA_rg[rr - 1];

		// 2i: Public capital accumulation (stock with depreciation; uses EA net of adaptation)
		K_pub_rg[rr - 1] = K_pub_rg[rr - 1] * (1.0 - delta_pub) + EA_pub;
	}

	// -------------------------------------------------------------------
	// Step 2g-rec-5: Per-firm subsidy disbursement to LAGGED affected C-firms
	// sub_Rec(j) = GRecPaid_{r,t} / |J^aff_{r,t-1}| for j in J^aff_{r,t-1}
	// K-firm supplier gets machine order revenue; C-firm capital is restored
	// -------------------------------------------------------------------
	// Step 2g-rec-5: Per-firm subsidy assignment to LAGGED affected C-firms
	// sub_Rec(j) = GRecPaid_{r,t} / |J^aff_{r,t-1}| for j in J^aff_{r,t-1}
	// Capital is restored here. K-firm revenue routing happens in Step 4
	// alongside EA so that all govt spending flows through one unified channel.
	// -------------------------------------------------------------------
	if (flag_adaptation == 2 || flag_adaptation == 3)
	{
		for (int rr = 1; rr <= NR; rr++)
		{
			if (GRecPaid_rg[rr - 1] > 0.0 && n_aff_rg_lag[rr - 1] > 0.0)
			{
				double share = GRecPaid_rg[rr - 1] / n_aff_rg_lag[rr - 1];
				for (int jj = 1; jj <= N2; jj++)
				{
					if (region_firm_assignment_C[jj - 1] == rr && affected_indicator_lag(jj) == 1.0 && exiting_2(jj) == 0)
					{
						sub_Rec(jj) = share;
					}
				}
			}
		}
	}

	// Step 3: Compute national aggregates
	REV_rg_total = 0.0;
	TR_rg_total = 0.0;
	GT_base_total = 0.0;
	GT_topup_total = 0.0;
	TS_rg_total = 0.0;
	EA_total = 0.0;
	K_pub_total = 0.0;
	K_adapt_total = 0.0;
	I_adapt_total = 0.0;

	double gdp_sum_w = 0.0;
	double omega_gdp_sum = 0.0;

	for (int rr = 0; rr < NR; rr++)
	{
		REV_rg_total += REV_rg[rr];
		TR_rg_total += GT_rg[rr];
		GT_base_total += GT_base_rg[rr];
		GT_topup_total += GT_topup_rg[rr];
		TS_rg_total += TS_rg[rr];
		EA_total += EA_rg[rr];
		K_pub_total += K_pub_rg[rr];
		K_adapt_total += K_adapt_rg[rr];
		I_adapt_total += I_adapt_rg[rr];
		GRecPaid_total += GRecPaid_rg[rr];
		TREC_total += TREC_rg[rr];
		double gdp_w = std::max(reg_GDP_n[rr], 1.0);
		gdp_sum_w += gdp_w;
		omega_gdp_sum += Omega_adapt_rg[rr] * gdp_w;
	}

	Omega_adapt_national = (gdp_sum_w > 0.0) ? omega_gdp_sum / gdp_sum_w : 1.0;
	GovPurchases_Rec = GRecPaid_total;

	// Step 4: Route EA as government purchases to K-firms for public capital accumulation
	// Also add GRecPaid_total to G so the government budget reflects recovery outlay.
	// GovPurchases_1 must include BOTH EA_total and GovPurchases_Rec for SFC balance to hold.
	if (GRecPaid_total > 0.0)
		G += GRecPaid_total;

	if (EA_total > 0.0)
	{
		G += EA_total;
		GovPurchases_1 = EA_total;

		// Part A: public-capital residual (EA net of adaptation) -> K-firms WITHIN the region.
		for (int rr = 1; rr <= NR; rr++)
		{
			double ea_pub = EA_rg[rr - 1] - I_adapt_rg[rr - 1];
			if (ea_pub <= 0.0)
				continue;
			if (reg_N1[rr - 1] > 0)
			{
				// Market-share-based allocation among K-firms in region
				double total_share = 0.0;
				for (int ii = 1; ii <= N1; ii++)
				{
					if (region_firm_assignment_K[ii - 1] == rr)
					{
						total_share += f1(1, ii);
					}
				}
				double n_rg = reg_N1[rr - 1];
				for (int ii = 1; ii <= N1; ii++)
				{
					if (region_firm_assignment_K[ii - 1] == rr)
					{
						double ea_firm = (total_share > 0.0)
											 ? ea_pub * (f1(1, ii) / total_share)
											 : ea_pub / n_rg;
						Deposits_1(1, ii) += ea_firm;
						KfirmGovCredit(ii) += ea_firm;
						int bank = static_cast<int>(BankingSupplier_1(ii));
						Deposits(1, bank) += ea_firm;
						Inflows(bank) += ea_firm;
					}
				}
			}
			else
			{
				// No K-firms in region; public-capital portion cannot be disbursed.
				G -= ea_pub;
				GovPurchases_1 -= ea_pub;
			}
		}

		// Part B: adaptation investment I_adapt -> ALL national K-firms, weighted by the market
		// share PERCEIVED from the funding region (non-regional K-firms discounted by tau_regional).
		// Mirrors local C-firms sourcing machines: no strict regional confinement, but home-bias.
		bool kbias = (flag_regional_bias == 1 && tau_regional > 1e-12);
		for (int rr = 1; rr <= NR; rr++)
		{
			double iad = I_adapt_rg[rr - 1];
			if (iad <= 0.0)
				continue;
			double Wr = 0.0;
			for (int ii = 1; ii <= N1; ii++)
			{
				double phi = (!kbias || region_firm_assignment_K[ii - 1] == rr) ? 1.0 : 1.0 / (1.0 + tau_regional);
				Wr += f1(1, ii) * phi;
			}
			for (int ii = 1; ii <= N1; ii++)
			{
				double phi = (!kbias || region_firm_assignment_K[ii - 1] == rr) ? 1.0 : 1.0 / (1.0 + tau_regional);
				double ad_firm = (Wr > 0.0) ? iad * (f1(1, ii) * phi / Wr) : iad / N1;
				Deposits_1(1, ii) += ad_firm;
				KfirmGovCredit(ii) += ad_firm;
				int bank = static_cast<int>(BankingSupplier_1(ii));
				Deposits(1, bank) += ad_firm;
				Inflows(bank) += ad_firm;
			}
		}
	}
	// Route recovery disbursements directly to affected C-firms (C-firm direct grant)
	if (GRecPaid_total > 0.0 && (flag_adaptation == 2 || flag_adaptation == 3))
	{
		double GRecPaid_actual = 0.0;
		for (int jj = 1; jj <= N2; jj++)
			GRecPaid_actual += sub_Rec(jj);
		if (GRecPaid_actual > 0.0)
		{
			GovPurchases_2 = GRecPaid_actual;
			GovPurchases_Rec = GRecPaid_actual;
			// Correct G for any gap from exiting firms that received no disbursement
			G -= (GRecPaid_total - GRecPaid_actual);
			// Route per C-firm: update deposit and bank reserve inflow
			for (int jj = 1; jj <= N2; jj++)
			{
				if (sub_Rec(jj) > 0.0)
				{
					Deposits_2(1, jj) += sub_Rec(jj);
					int bank = static_cast<int>(BankingSupplier_2(jj));
					if (bank >= 1 && bank <= static_cast<int>(NB))
					{
						Deposits(1, bank) += sub_Rec(jj);
						Inflows(bank) += sub_Rec(jj);
					}
				}
			}
		}
	}
}
