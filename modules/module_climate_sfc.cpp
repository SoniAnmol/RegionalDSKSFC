#include "module_climate_sfc.h"


void CLIMATE_POLICY(void)
{
    if(flag_tax_CO2==1)
    {
        //Constant tax increasing with inflation
        t_CO2=cpi(2)/cpi_init*t_CO2_0; 
        t_CO2_en=cpi(2)/cpi_init*t_CO2_en_0;
    }

    if(flag_tax_CO2==2)
    {
        //linear increase as function of time
        t_CO2=t_CO2_0+(t-t_start_climbox)/tc1;
        t_CO2_en=t_CO2_en_0+(t-t_start_climbox)/tc1;  
    }

    if(flag_tax_CO2==3)
    {
        //Exponential increase + inflation correction
        t_CO2=cpi(2)/cpi_init*(t_CO2_0*pow(tc2,(t-t_start_climbox)));
        t_CO2_en=cpi(2)/cpi_init*(t_CO2_en_0*pow(tc2,(t-t_start_climbox)));
    }

    if(flag_tax_CO2==4)
    {
        //Increasing with nominal GDP
        t_CO2=GDP_n(2)/GDP_init*t_CO2_0; 
        if(flag_desc==1 && t>=200)
        {
            if(t==200)
            {
              GDP_init=GDP_n(2);
              t_CO2_en_0=t_CO2_en*2;
              t_CO2_en=GDP_n(2)/GDP_init*t_CO2_en_0;
            }
            else if(t>200 && t<=212)
            {
              t_CO2_en_0=t_CO2_en_0*2;
              t_CO2_en=GDP_n(2)/GDP_init*t_CO2_en_0;
            }
            else
            {
              t_CO2_en=GDP_n(2)/GDP_init*t_CO2_en_0;
            }
        }
        else if(flag_desc==4 && t>=200)
        {
            if(t==200)
            {
              GDP_init=GDP_n(2);
              t_CO2_en_0=t_CO2_en*1.5;
              t_CO2_en=GDP_n(2)/GDP_init*t_CO2_en_0;
            }
            else if(t>200 && t<=212)
            {
              t_CO2_en_0=t_CO2_en_0*1.5;
              t_CO2_en=GDP_n(2)/GDP_init*t_CO2_en_0;
            }
            else
            {
              t_CO2_en=GDP_n(2)/GDP_init*t_CO2_en_0;
            }
        }
		else
        {
            t_CO2_en=GDP_n(2)/GDP_init*t_CO2_en_0;
        }
    }

    t_CO2=max(t_CO2,0.0);  
    t_CO2_en=max(t_CO2_en,0.0);
}

void CLIMATEBOX(void)
{
    //C-Roads climate box

    //Fix the equilibrium pre-industrial carbon concentrations in each ocean layer
    for (j=1; j<=ndep; j++)
    { 
        Con0(j)=Con00*laydep(j); 
    } 

    //Get emissions 
    Emiss_yearly(1)=0; 
    Emiss_yearly(2)=0; 
    for (tt=1; tt<=freqclim; tt++)
    {
        Emiss_yearly(1)+=Emiss_TOT(tt);  
        Emiss_yearly(2)+=Emiss_TOT(tt+freqclim); 
    }
    
    //During first run of climate box, initialise calib. emissions
    if(t>t_start_climbox &&  t<(t_start_climbox+freqclim+1))
    {
        Emiss_yearly_calib(2)=Emiss_yearly_0;
        Emiss_yearly_calib(1)=Emiss_yearly_0;
        Emiss_gauge=Emiss_yearly(1);
        Emiss_global=Emiss_gauge/emiss_share-Emiss_gauge;
        Emiss_gauge+=Emiss_global;
    }

    Emiss_global=Emiss_global*g_emiss_global;
    Emiss_yearly(1)+=Emiss_global;
        
    //Calculate new annual industrial emissions
    g_rate_em_y=Emiss_yearly(1)/Emiss_gauge;
    Emiss_yearly_calib(1)=Emiss_yearly_0*g_rate_em_y;  

    //Atmosphere-Biosphere carbon exchange (+emissions)
    //Atmospheric carbon taken up by plants
    NPP=NPP0*(1+fertil*log(Cat(2)/Cat0))*(1+heatstress*Tmixed(2));
    //Carbon released into atmosphere by decaying humus
    humrelease=hum(2)/humtime;
    //Carbon released into atmosphere by rotting plants
    biorelease=biom(2)/biotime*(1-humfrac);

    //Exclude ocean uptake which is computed below 
    dCat1=Emiss_yearly_calib(1)+humrelease+biorelease-NPP;
    //Initial guess for Cat (excluding ocean uptake) 
    Cat1=Cat(2)+dCat1;

    //Update biomass carbon
    biom(1)=biom(2)+NPP-biom(2)/biotime;
    //Update humus carbon
    hum(1)=hum(2)+biom(2)/biotime*humfrac-hum(2)/humtime;

    //Carbon mixing between ocean layers
    //Carbon flux between layers through diffusion
    for (j=1; j<=ndep-1; j++)
    {
        fluxC(j)=eddydif*(Con(2,j)/laydep(j)-Con(2,j+1)/laydep(j+1))/((laydep(j+1)+laydep(j))/2);
    }

    //Update carbon content in layers
    //Preliminary carbon content of mixed layer (before exchange with atmosphere)
    Con1=Con(2,1)-fluxC(1);
    
    for (j=2; j<=ndep-1; j++)  
    {
        Con(1,j)=Con(2,j)+fluxC(j-1)-fluxC(j); 
    }

    Con(1,ndep)=Con(2,ndep)+fluxC(ndep-1); 

    //Ocean-atmposphere carbon exchange
    //Total carbon in atmosphere and upper ocean layer
    Ctot1=Con1+Cat1; 

    //Re-distribute carbon between upper ocean layer and atmosphere iteratively until an equilibrium is reached.
    //Start from an initial guess for Cat which implies a value for Cay.
    //The slope of the curve (Caa) is estimated by using two additional nearby x and y values (Caxx and Caay). 

    //Initial guess for Cat 
    Cax(1)=Cat1;
    Cay(1)=Ctot1-Cax(1)-Conref*(1-ConrefT*Tmixed(2))*pow((Cax(1)/Cat0),1/(rev0+revC*log(Cax(1)/Cat0)));
    //Nearby value for Cat
    Caxx(1)=Cax(1)+1.5*Cay(1);
    Cayy(1)=Ctot1-Caxx(1)-Conref*(1-ConrefT*Tmixed(2))*pow((Caxx(1)/Cat0),1/(rev0+revC*log(Caxx(1)/Cat0)));
    //Initial guess for gradient 
    Caa(1)=(Cayy(1)-Cay(1))/(Caxx(1)-Cax(1));

    i=1; 
    do
    {
        Cax(i+1)=Cax(i)-Cay(i)/Caa(i);
        Cay(i+1)=Ctot1-Cax(i+1)-Conref*(1-ConrefT*Tmixed(2))*pow((Cax(i+1)/Cat0),1/(rev0+revC*log(Cax(i+1)/Cat0)));
        if (abs(Cay(i+1))<1e-10)
        {
            Cax(niterclim) = Cax(i+1);
            break;
        } 
        Caxx(i+1)=Cax(i+1)-2*Cay(i)/Caa(i);
        Cayy(i+1)=Ctot1-Caxx(i+1)-Conref*(1-ConrefT*Tmixed(2))*pow((Caxx(i+1)/Cat0),1/(rev0+revC*log(Caxx(i+1)/Cat0)));
        Caa(i+1)=(Cayy(i+1)-Cay(i+1))/(Caxx(i+1)-Cax(i+1));
        i++;
    }
    while (i<=niterclim-1);

    //Final value for atmospheric carbon
    Cat(1)=Cax(niterclim);
    //Final value for carbon in upper ocean layer
    Con(1,1)=Ctot1-Cat(1);


    //Radiation and temperature change

    //Compute radiative forcing
    FCO2=forCO2*log(Cat(1)/Cat0); 

    //No non-CO2 forcing 
    if (flag_nonCO2_force==0) 
    {
        Fin=FCO2;
    }
    
    //Non-CO2 forcing taken into account in a rough way
    if (flag_nonCO2_force==1)
    {
        Fin=FCO2*otherforcefac;
    }

    //Extra outradiated energy due to global warming 
    Fout=outrad*Tmixed(2);

    //Ocean mixing 
    //Carbon flux between layers 
    for (j=1; j<=ndep-1; j++) 
    {
        fluxH(j)=eddydif*(Hon(2,j)/laydep(j)-Hon(2,j+1)/laydep(j+1))/((laydep(j+1)+laydep(j))/2);
    }

    //Update heat content and temperature in layers (including radiative contribution to top layer)
    //Top layer heat content
    Hon(1,1)=Hon(2,1)-fluxH(1)+(Fin-Fout)*secyr/seasurf;
    //Top layer temperature w.r.t pre-industrial value 
    Ton(1,1)=Hon(1,1)/laydep(1)/heatcap;
    //"Surface" temperature is that of the mixed layer 
    Tmixed(1)=Ton(1,1);

    for (j=2; j<=ndep-1; j++)  
    {
        //Middle layer heat content
        Hon(1,j)=Hon(2,j)+fluxH(j-1)-fluxH(j);
        //Middle layer temperature w.r.t pre-industrial value
        Ton(1,j)=Hon(1,j)/laydep(j)/heatcap;
    }
    
    //Bottom layer heat content
    Hon(1,ndep)=Hon(2,ndep)+fluxH(ndep-1);
    //Middle layer temperature w.r.t pre-industrial value
    Ton(1,ndep)=Hon(1,ndep)/laydep(ndep)/heatcap;
}

void CLIMATEBOX_CUM_EMISS(void)
{
    //Simple climate box based on cumulative emissions

    Emiss_yearly(1)=0; 
    Emiss_yearly(2)=0; 
    for (tt=1; tt<=freqclim; tt++)
    {
        Emiss_yearly(1)+=Emiss_TOT(tt);  
        Emiss_yearly(2)+=Emiss_TOT(tt+freqclim); 
    }
    
    if(t>t_start_climbox &&  t<(t_start_climbox+freqclim+1))
    {
        Cum_emissions=Cum_emissions_0;
        Emiss_yearly_calib(2)=Emiss_yearly_0;
        Emiss_global=Emiss_yearly(1)/emiss_share-Emiss_yearly(1);
    }

    Emiss_yearly(2)+=Emiss_global;
    Emiss_global=Emiss_global*g_emiss_global;
    Emiss_yearly(1)+=Emiss_global;
    
    g_rate_em_y=(Emiss_yearly(1)-Emiss_yearly(2))/Emiss_yearly(2);
    
    Emiss_yearly_calib(1)=Emiss_yearly_calib(2)*(1+g_rate_em_y);

    Cum_emissions+=Emiss_yearly_calib(1);

    Tmixed(1)= intercept_temp + slope_temp*Cum_emissions;
}

void SINGLESHOCK(void)
{
    //This is used for the experiment in which there is only a single fully exogenous climate shock (in period 300)
    //Set shock size
    if(t==300)
    {
        if(flag_prodshocks1>0)
        {
            if(flag_uniformshocks==0)
            {
                X_a(1)=(shocks_kfirms(1)*(b_0(1)-2/3)+1/3)/(1-shocks_kfirms(1));
                for (i=1; i<=N1; i++)
                {
                    rnd=betadev(X_a(1),b_0(1),p_seed);
                    shocks_machprod(i)=rnd;
                    rnd=betadev(X_a(1),b_0(1),p_seed);
                    shocks_techprod(i)=rnd;
                }
            }
            else
            {
                shocks_machprod=shocks_kfirms;
                shocks_techprod=shocks_kfirms;
            }
        }

        if(flag_prodshocks2==1)
        {
            //Shock to labour productivity of both K-firms and C-firms --> this affects overall productivity of C-firms, not just recent vintage of machines
            if(flag_uniformshocks==0)
            {
                X_a(2)=(shocks_cfirms(1)*(b_0(2)-2/3)+1/3)/(1-shocks_cfirms(1));
                for (j=1; j<=N2; j++)
                {
                    rnd=betadev(X_a(2),b_0(2),p_seed);
                    shocks_labprod2(j)=rnd;
                }
                X_a(2)=(shocks_kfirms(1)*(b_0(2)-2/3)+1/3)/(1-shocks_kfirms(1));
                for (i=1; i<=N1; i++)
                {
                    rnd=betadev(X_a(2),b_0(2),p_seed);
                    shocks_labprod1(i)=rnd;
                }
            }
            else
            {
                shocks_labprod2=shocks_cfirms;
                shocks_labprod1=shocks_kfirms;
            }
        }

        if(flag_prodshocks2==2)
        {
            //Shock to energy efficiency of both K-firms and C-firms --> this affects overall efficiency of C-firms, not just recent vintage of machines
            if(flag_uniformshocks==0)
            {
                X_a(2)=(shocks_cfirms(1)*(b_0(2)-2/3)+1/3)/(1-shocks_cfirms(1));
                for (j=1; j<=N2; j++)
                {
                    rnd=betadev(X_a(2),b_0(2),p_seed);
                    shocks_eneff2(j)=rnd;
                }
                X_a(2)=(shocks_kfirms(1)*(b_0(2)-2/3)+1/3)/(1-shocks_kfirms(1));
                for (i=1; i<=N1; i++)
                {
                    rnd=betadev(X_a(2),b_0(2),p_seed);
                    shocks_eneff1(i)=rnd;
                }
            }
            else
            {
                shocks_eneff2=shocks_cfirms;
                shocks_eneff1=shocks_kfirms;
            }
        }

        if(flag_prodshocks2==3)
        {
            //Shock to labour productivity and energy efficiency of both K-firms and C-firms --> as above
            if(flag_uniformshocks==0)
            {
                X_a(2)=(shocks_cfirms(1)*(b_0(2)-2/3)+1/3)/(1-shocks_cfirms(1));
                for (j=1; j<=N2; j++)
                {
                    rnd=betadev(X_a(2),b_0(2),p_seed);
                    shocks_labprod2(j)=rnd;
                    rnd=betadev(X_a(2),b_0(2),p_seed);
                    shocks_eneff2(j)=rnd;
                }
                X_a(2)=(shocks_kfirms(1)*(b_0(2)-2/3)+1/3)/(1-shocks_kfirms(1));
                for (i=1; i<=N1; i++)
                {
                    rnd=betadev(X_a(2),b_0(2),p_seed);
                    shocks_labprod1(i)=rnd;
                    rnd=betadev(X_a(2),b_0(2),p_seed);
                    shocks_eneff1(i)=rnd;
                }
            }
            else
            {
                shocks_labprod2=shocks_cfirms;
                shocks_labprod1=shocks_kfirms;
                shocks_eneff2=shocks_cfirms;
                shocks_eneff1=shocks_kfirms;
            }
        }
        
        if(flag_encapshocks>0)
        {
            shocks_encapstock_de=shock_scalar;
            shocks_encapstock_ge=shock_scalar;
        }
        
        if(flag_popshocks>0)
        {
            shock_pop=shock_scalar;
        }

        if(flag_demandshocks>0)
        {
            shock_cons=shock_scalar;
        }

        if(flag_capshocks>0)
        {
            //Shock to C-firms' capital stocks
            if(flag_uniformshocks==0)
            {
                X_a(6)=(shocks_cfirms(1)*(b_0(6)-2/3)+1/3)/(1-shocks_cfirms(1));
                for (j=1; j<=N2; j++)
                {
                    rnd=betadev(X_a(6),b_0(6),p_seed);
                    shocks_capstock(j)=rnd;
                }
            }
            else
            {
                shocks_capstock=shocks_cfirms;
            }

            if(flag_capshocks==3)
            {
                for (j=1; j<=N2; j++)
                {
                    risk_c(j)=gasdev(p_seed);
                }
                risk_c=risk_c-risk_c.Minimum()+0.01;
                risk_c=risk_c/risk_c.Maximum();
            }
        }

        if(flag_outputshocks>0)
        {
            //Shock to current output
            if(flag_uniformshocks==0)
            {
                X_a(7)=(shocks_cfirms(1)*(b_0(7)-2/3)+1/3)/(1-shocks_cfirms(1));
                for (j=1; j<=N2; j++)
                {
                    rnd=betadev(X_a(7),b_0(7),p_seed);
                    shocks_output2(j)=rnd;
                }
                X_a(7)=(shocks_kfirms(1)*(b_0(7)-2/3)+1/3)/(1-shocks_kfirms(1));
                for (i=1; i<=N1; i++)
                {
                    rnd=betadev(X_a(7),b_0(7),p_seed);
                    shocks_output1(i)=rnd;
                }
            }
            else
            {
                shocks_output2=shocks_cfirms;
                shocks_output1=shocks_kfirms;
            }

            if(flag_outputshocks==3)
            {
                for (j=1; j<=N2; j++)
                {
                    risk_c(j)=gasdev(p_seed);
                }
                risk_c=risk_c-risk_c.Minimum()+0.01;
                risk_c=risk_c/risk_c.Maximum();
                for (i=1; i<=N1; i++)
                {   
                    risk_k(i)=gasdev(p_seed);
                }
                risk_k=risk_k-risk_k.Minimum()+0.01;
                risk_k=risk_k/risk_k.Maximum();
            }
        }

        if(flag_inventshocks>0)
        {
            if(flag_uniformshocks==0)
            {
                X_a(8)=(shocks_cfirms(1)*(b_0(8)-2/3)+1/3)/(1-shocks_cfirms(1));
                for (j=1; j<=N2; j++)
                {
                    rnd=betadev(X_a(8),b_0(8),p_seed);
                    shocks_invent(j)=rnd;
                }
            }
            else
            {
                shocks_invent=shocks_cfirms;
            }

            if(flag_inventshocks==3 && risk_c.Sum()==0)
            {
                for (j=1; j<=N2; j++)
                {
                    risk_c(j)=gasdev(p_seed);
                }
                risk_c=risk_c-risk_c.Minimum()+0.01;
                risk_c=risk_c/risk_c.Maximum();
            }
        }

        if(flag_RDshocks>0)
        {
            //Shock to R&D
            if(flag_uniformshocks==0)
            {
                X_a(9)=(shocks_kfirms(1)*(b_0(9)-2/3)+1/3)/(1-shocks_kfirms(1));
                for (i=1; i<=N1; i++)
                {
                    rnd=betadev(X_a(9),b_0(9),p_seed);
                    shocks_rd(i)=rnd;
                }
            }
            else
            {
                shocks_rd=shocks_kfirms;
            }
        }

    }
    else if(t>300)
    {
        shocks_rd=persistence*shocks_rd;
        shocks_labprod2=persistence*shocks_labprod2;
        shocks_labprod1=persistence*shocks_labprod1;
        shocks_eneff2=persistence*shocks_eneff2;
        shocks_eneff1=persistence*shocks_eneff1;
        shocks_output2=persistence*shocks_output2;
        shocks_output1=persistence*shocks_output1;
        shocks_capstock=persistence*shocks_capstock;
        shocks_encapstock_de=persistence*shocks_encapstock_de;
        shocks_encapstock_ge=persistence*shocks_encapstock_ge;
        shocks_invent=persistence*shocks_invent;
        shocks_machprod=persistence*shocks_machprod;
        shocks_techprod=persistence*shocks_techprod;
        shock_pop=persistence*shock_pop;
        shock_cons=persistence*shock_cons;
    }
    
    if(t>=300)
    {
        if (flag_prodshocks1==1)
        {
            for (i=1; i<=N1; i++)
            {
                A1(i)=max(A0,A1(i)*(1-shocks_machprod(i)));

                A1p(i)=max(A0*pm,A1p(i)*(1-shocks_techprod(i)));
            }
        }

    
        if (flag_prodshocks1==2)
        {
            for (i=1; i<=N1; i++)
            {
                A1_en(i)=max(A0_en,A1_en(i)*(1-shocks_machprod(i)));
                
                A1p_en(i)=max(A0_en,A1p_en(i)*(1-shocks_techprod(i)));
            }    
        }
        
        if (flag_prodshocks1==3)
        {
            for (i=1; i<=N1; i++)
            {
                A1(i)=max(A0,A1(i)*(1-shocks_machprod(i)));
                A1_en(i)=max(A0_en,A1_en(i)*(1-shocks_machprod(i)));

                A1p(i)=max(A0*pm,A1p(i)*(1-shocks_techprod(i)));
                A1p_en(i)=max(A0_en,A1p_en(i)*(1-shocks_techprod(i)));
            }
        }

        if (flag_prodshocks1==4)
        {
            for (i=1; i<=N1; i++)
            {
                A1(i)=max(A0,A1(i)*(1-shocks_machprod(i)));
                for (tt=t0; tt<=t; tt++)
                {
                    A(tt,i)=max(A0,A(tt,i)*(1-shocks_machprod(i)));
                }

                A1p(i)=max(A0*pm,A1p(i)*(1-shocks_techprod(i)));
            }
        }

        if (flag_prodshocks1==5)
        {
            for (i=1; i<=N1; i++)
            {
                A1_en(i)=max(A0_en,A1_en(i)*(1-shocks_machprod(i)));
                for (tt=t0; tt<=t; tt++)
                {
                    A_en(tt,i)=max(A0_en,A_en(tt,i)*(1-shocks_machprod(i)));
                }

                A1p_en(i)=max(A0_en,A1p_en(i)*(1-shocks_techprod(i)));
            }    
        }

        if (flag_prodshocks1==6)
        {
            for (i=1; i<=N1; i++)
            {
                A1(i)=max(A0,A1(i)*(1-shocks_machprod(i)));
                A1_en(i)=max(A0_en,A1_en(i)*(1-shocks_machprod(i)));
                for (tt=t0; tt<=t; tt++)
                {
                    A(tt,i)=max(A0,A(tt,i)*(1-shocks_machprod(i)));
                    A_en(tt,i)=max(A0_en,A_en(tt,i)*(1-shocks_machprod(i)));
                }

                A1p(i)=max(A0*pm,A1p(i)*(1-shocks_techprod(i)));
                A1p_en(i)=max(A0_en,A1p_en(i)*(1-shocks_techprod(i)));
            }
        }


        if (flag_encapshocks>0)
        {
            //Shock to energy sector capital stock
            for (tt=1; tt<=t; tt++)
            {
                if (G_de(tt)>0)
                {
                    G_de(tt)=ROUND(G_de(tt)*(1-shocks_encapstock_de(t)));
                }
                
                if (G_ge(tt)>0)
                {
                    G_ge(tt)=ROUND(G_ge(tt)*(1-shocks_encapstock_ge(t)));
                    G_ge_n(tt)=ROUND(G_ge_n(tt)*(1-shocks_encapstock_ge(t)));
                }
            }
            CapitalStock_e(1)=G_ge_n.Sum();
        }

        if (flag_popshocks>0) 
        {
            //Shock to population (labour supply)
            LS=LS*(1-shock_pop);
        }
    }
}


void SHOCKS(void)
{  
    //This function generates repeated endogenous climate shocks
    
    //Update parameter for disaster generating function
    for(i=1; i<=nshocks; i++)
    {
        X_a(i)=a_0(i)*pow(1+log((Tmixed(2)+T_pre)/(T_pre+Tmixedinit1)),shockexponent1(i));
        X_b(i)=b_0(i)*pow((T_pre+Tmixedinit1)/(Tmixed(2)+T_pre),shockexponent2(i));
    }

    if (t>(t_start_climbox+4))
    {
        if (flag_prodshocks1==1)
        {
            for (i=1; i<=N1; i++)
            {
                rnd=betadev(X_a(1),X_b(1),p_seed);
                shocks_machprod(i)=rnd;
                A1(i)=max(A0,A1(i)*(1-shocks_machprod(i)));

                rnd=betadev(X_a(1),X_b(1),p_seed);
                shocks_techprod(i)=rnd;
                A1p(i)=max(A0*pm,A1p(i)*(1-shocks_techprod(i)));
            } 
        }

    
        if (flag_prodshocks1==2)
        {
            for (i=1; i<=N1; i++)
            {
                rnd=betadev(X_a(1),X_b(1),p_seed);
                shocks_machprod(i)=rnd;
                A1_en(i)=max(A0_en,A1_en(i)*(1-shocks_machprod(i)));
                
                rnd=betadev(X_a(1),X_b(1),p_seed); 
                shocks_techprod(i)=rnd;
                A1p_en(i)=max(A0_en,A1p_en(i)*(1-shocks_techprod(i)));
            }    
        }
        
        if (flag_prodshocks1==3)
        {
            for (i=1; i<=N1; i++)
            {
                rnd=betadev(X_a(1),X_b(1),p_seed); 
                shocks_machprod(i)=rnd;
                A1(i)=max(A0,A1(i)*(1-shocks_machprod(i)));
                A1_en(i)=max(A0_en,A1_en(i)*(1-shocks_machprod(i)));

                rnd=betadev(X_a(1),X_b(1),p_seed); 
                shocks_techprod(i)=rnd;
                A1p(i)=max(A0*pm,A1p(i)*(1-shocks_techprod(i)));
                A1p_en(i)=max(A0_en,A1p_en(i)*(1-shocks_techprod(i)));
            }
        }

        if (flag_prodshocks1==4)
        {
            for (i=1; i<=N1; i++)
            {
                rnd=betadev(X_a(1),X_b(1),p_seed);
                shocks_machprod(i)=rnd;
                A1(i)=max(A0,A1(i)*(1-shocks_machprod(i)));
                for (tt=t0; tt<=t; tt++)
                {
                    A(tt,i)=max(A0,A(tt,i)*(1-shocks_machprod(i)));
                }

                rnd=betadev(X_a(1),X_b(1),p_seed);
                shocks_techprod(i)=rnd;
                A1p(i)=max(A0*pm,A1p(i)*(1-shocks_techprod(i)));
            } 
        }

        if (flag_prodshocks1==5)
        {
            for (i=1; i<=N1; i++)
            {
                rnd=betadev(X_a(1),X_b(1),p_seed);
                shocks_machprod(i)=rnd;
                A1_en(i)=max(A0_en,A1_en(i)*(1-shocks_machprod(i)));
                for (tt=t0; tt<=t; tt++)
                {
                    A_en(tt,i)=max(A0_en,A_en(tt,i)*(1-shocks_machprod(i)));
                }
                
                rnd=betadev(X_a(1),X_b(1),p_seed); 
                shocks_techprod(i)=rnd;
                A1p_en(i)=max(A0_en,A1p_en(i)*(1-shocks_techprod(i)));
            }    
        }

        if (flag_prodshocks1==6)
        {
            for (i=1; i<=N1; i++)
            {
                rnd=betadev(X_a(1),X_b(1),p_seed); 
                shocks_machprod(i)=rnd;
                A1(i)=max(A0,A1(i)*(1-shocks_machprod(i)));
                A1_en(i)=max(A0_en,A1_en(i)*(1-shocks_machprod(i)));
                for (tt=t0; tt<=t; tt++)
                {
                    A(tt,i)=max(A0,A(tt,i)*(1-shocks_machprod(i)));
                    A_en(tt,i)=max(A0_en,A_en(tt,i)*(1-shocks_machprod(i)));
                }

                rnd=betadev(X_a(1),X_b(1),p_seed); 
                shocks_techprod(i)=rnd;
                A1p(i)=max(A0*pm,A1p(i)*(1-shocks_techprod(i)));
                A1p_en(i)=max(A0_en,A1p_en(i)*(1-shocks_techprod(i)));
            }
        }

        if(flag_prodshocks2==1)
        {
            for (j=1; j<=N2; j++)
            {
                rnd=betadev(X_a(2),X_b(2),p_seed);
                shocks_labprod2(j)=rnd;
            }

            for (i=1; i<=N1; i++)
            {
                rnd=betadev(X_a(2),X_b(2),p_seed);
                shocks_labprod1(i)=rnd;
            }
        }

        if(flag_prodshocks2==2)
        {
                for (j=1; j<=N2; j++)
                {
                    rnd=betadev(X_a(2),X_b(2),p_seed);
                    shocks_eneff2(j)=rnd;
                }

                for (i=1; i<=N1; i++)
                {
                    rnd=betadev(X_a(2),X_b(2),p_seed);
                    shocks_eneff1(i)=rnd;
                }
        }

        if(flag_prodshocks2==3)
        {
            for (j=1; j<=N2; j++)
            {
                rnd=betadev(X_a(2),X_b(2),p_seed);
                shocks_labprod2(j)=rnd;
                rnd=betadev(X_a(2),X_b(2),p_seed);
                shocks_eneff2(j)=rnd;
            }

            for (i=1; i<=N1; i++)
            {
                rnd=betadev(X_a(2),X_b(2),p_seed);
                shocks_labprod1(i)=rnd;
                rnd=betadev(X_a(2),X_b(2),p_seed);
                shocks_eneff1(i)=rnd;
            }
        }

        if (flag_encapshocks>0)
        {
            //Shock to capital stock of the energy sector
            for (tt=1; tt<=t; tt++)
            {
                shocks_encapstock_de(t)=X_a(3)/(X_a(3)+X_b(3));
                if (G_de(tt)>0)
                {
                    G_de(tt)=ROUND(G_de(tt)*(1-shocks_encapstock_de(t)));
                }
                
                shocks_encapstock_ge(t)=X_a(3)/(X_a(3)+X_b(3));
                if (G_ge(tt)>0)
                {
                    G_ge(tt)=ROUND(G_ge(tt)*(1-shocks_encapstock_ge(t)));
                    G_ge_n(tt)=ROUND(G_ge_n(tt)*(1-shocks_encapstock_ge(t)));
                }
            }
            CapitalStock_e(1)=G_ge_n.Sum();
        }

        if (flag_popshocks>0)
        {
            //Shock to population (labour supply)
            shock_pop=X_a(4)/(X_a(4)+X_b(4));
            LS=LS*(1-shock_pop);
        }

        if (flag_demandshocks>0)
        {
            //Shock to aggregate consumption demand
            shock_cons=X_a(5)/(X_a(5)+X_b(5));
        }

        if (flag_capshocks>0)
        {
            //Shock to capital stock of C-firms
            if(flag_capshocks==1)
            {
                for (j=1; j<=N2; j++)
                {
                    rnd=betadev(X_a(6),X_b(6),p_seed);
                    shocks_capstock(j)=rnd;
                }
            }
            else
            {
                shocks_capstock=X_a(6)/(X_a(6)+X_b(6));
            }

            if(flag_capshocks==3 && risk_c.Sum()==0)
            {
                for (j=1; j<=N2; j++)
                {
                    risk_c(j)=gasdev(p_seed);
                }
                risk_c=risk_c-risk_c.Minimum()+0.01;
                risk_c=risk_c/risk_c.Maximum();
            }
        }

        if(flag_outputshocks>0)
        {
            //On current output
            if(flag_outputshocks==1)
            {
                for (j=1; j<=N2; j++)
                {
                    rnd=betadev(X_a(7),X_b(7),p_seed);
                    shocks_output2(j)=rnd;
                }
            }
            else
            {
                shocks_output2=X_a(7)/(X_a(7)+X_b(7));
            }

            if(flag_outputshocks==3 && risk_c.Sum()==0)
            {
                for (j=1; j<=N2; j++)
                {
                    risk_c(j)=gasdev(p_seed);
                }
                risk_c=risk_c-risk_c.Minimum()+0.01;
                risk_c=risk_c/risk_c.Maximum();
            }

            if(flag_outputshocks==1)
            {
                for (i=1; i<=N1; i++)
                {
                    rnd=betadev(X_a(7),X_b(7),p_seed);
                    shocks_output1(i)=rnd;
                }
            }
            else
            {
                shocks_output1=X_a(7)/(X_a(7)+X_b(7));
            }

            if(flag_outputshocks==3 && risk_k.Sum()==0)
            {
                for (i=1; i<=N1; i++)
                {
                    risk_k(i)=gasdev(p_seed);
                }
                risk_k=risk_k-risk_k.Minimum()+0.01;
                risk_k=risk_k/risk_k.Maximum();
            }
        }

        if (flag_inventshocks>0)
        {
            //Shock to C-firms' inventories
            if(flag_inventshocks==1)
            {
                for (j=1; j<=N2; j++)
                {
                    rnd=betadev(X_a(8),X_b(8),p_seed);
                    shocks_invent(j)=rnd;
                }
            }
            else
            {
                shocks_invent=X_a(8)/(X_a(8)+X_b(8));
            }

            if(flag_inventshocks==3 && risk_c.Sum()==0)
            {
                for (j=1; j<=N2; j++)
                {
                    risk_c(j)=gasdev(p_seed);
                }
                risk_c=risk_c-risk_c.Minimum()+0.01;
                risk_c=risk_c/risk_c.Maximum();
            }
        }

        if (flag_RDshocks>0)
        {
            //Shock to R&D effectiveness
            for (i=1; i<=N1; i++)
            {
                rnd=betadev(X_a(9),X_b(9),p_seed);
                shocks_rd(i)=rnd;
            }    
        }
    }

}

void UPDATECLIMATE(void)
{
    //Update variables entering the climate box for the next period
    if(t%freqclim==0)
    {
        Tmixed(2)=Tmixed(1);
        Emiss_yearly_calib(2)=Emiss_yearly_calib(1);
        Emiss_TOT(2)=Emiss_TOT(1);

        Cat(2)=Cat(1);
        biom(2)=biom(1);
        hum(2)=hum(1);  
        for (j=1;  j<=ndep; j++)
        {
            Con(2,j)=Con(1,j);
            Hon(2,j)=Hon(1,j);
            Ton(2,j)=Ton(1,j);
        }
    }
    
    for (j=1; j<=freqclim*2-1; j++)
    {
        Emiss_TOT(freqclim*2-j+1)=Emiss_TOT(freqclim*2-j);
    }

}