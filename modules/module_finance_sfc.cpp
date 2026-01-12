#include "module_finance_sfc.h"

///FUNCTIONS///

void TOTCREDIT(void)
{
  //Banks determine the maximum amount of credit they are willing to extend based on a regulatory ratio 
  for(j=1; j<=NB; j++)   
  {
    buffer(j)= credit_multiplier*(1+(beta_basel*Fragility(j)));
	  BaselBankCredit(j) =NW_b(1,j)/buffer(j);
    BankCredit(j)=BaselBankCredit(j);
  } 
} 

void LOANRATES(void) 
{  
  //Banks rank their C-firm customers according to their debt service to sales ratio
  for (j=1; j<=N2; ++j) 
  {
    DebtServiceToSales2(j)=DebtService_2(2,j)/(S2(2,j)+tolerance);
  }

  for (i=1; i<=NB; ++i)            
  {
    for (j=1; j<=N2; ++j)    
    {	
      DebtServiceToSales2_bank(j,i)=DebtServiceToSales2(j);
    } 
    for (j=1; j<=N2; ++j)    
    {	
      if (BankMatch_2(j,i)==0)
      {
        DebtServiceToSales2_bank(j,i)=DebtServiceToSales2_bank.Column(i).Maximum()+1;
      }   
    } 
  } 


  for (i=1; i<=NB; ++i)            
  {
    for (j=1; j<=N2; ++j)   
    {
      DebtServiceToSales2_temp(j) = DebtServiceToSales2_bank(j,i);
    }


    for (j=1; j<=N2; ++j)
    {
      DS2_min=DebtServiceToSales2_temp.Minimum1(DS2_min_index);
      DS2_rating(DS2_min_index,i)=j;
      DebtServiceToSales2_temp(DS2_min_index)=DebtServiceToSales2_temp.Maximum()+1;
    }
  }
  
  //Individual rates offered to each borrower are determined based on the ranking
  for(j=1; j<=N2;j++)
  {
    for (i=1; i<=NB; ++i)
    {
      if(BankMatch_2(j,i)==1)
      {
        k(j)= DS2_rating(j,i);
        if(k(j) <= NL_2(i)*0.25)
        {  
          r_deb_h(j)=r_deb(i);
        }
        else if(k(j) > NL_2(i)*0.25 & k(j) <= NL_2(i)*0.5 )
        {              
          r_deb_h(j)=r_deb(i)*(1+k_const);
        }
        else if(k(j) > NL_2(i)*0.5 & k(j) <= NL_2(i)*0.75 )
        {
          r_deb_h(j)=r_deb(i)*(1+2*k_const);
        }
        else
        {
          r_deb_h(j)=r_deb(i)*(1+3*k_const);
        }
      }
    }
  }

} 

void BANKING(void)
{
  //Banks determine their profits and pay dividends and taxes
  for(j=1; j<=NB; j++)
  {
    InterestReserves_b(j)=r_cbreserves*Reserves_b(2,j);
    Inflows(j)+=InterestReserves_b(j);
    InterestReserves+=InterestReserves_b(j);

    InterestAdvances_b(j)=r*Advances_b(2,j);
    Outflows(j)+=InterestAdvances_b(j);
    InterestAdvances+=InterestAdvances_b(j);

    BankProfits(j)= LoanInterest(j)+r_bonds*GB_b(2,j)+r_cbreserves*Reserves_b(2,j)-r*Advances_b(2,j)-InterestDeposits(j)-baddebt_b(j)+capitalRecovered(j)-LossEntry_b(j);
        
    if (BankProfits(j)>0) 
    {
      Dividends_b(j)=db*BankProfits(j);
      Taxes_b(j)=aliqb*BankProfits(j);
      Taxes+=aliqb*BankProfits(j);
      Deposits(1,j)+=Dividends_b(j);
      Deposits_hb(1,j)+=Dividends_b(j);
      Deposits_h(1)+=Dividends_b(j);
      Dividends(1)+=Dividends_b(j);
      Outflows(j)+=Taxes_b(j);
    }

    //Update net worth and "leverage"
    NW_b(1,j)=NW_b(2,j)+BankProfits(j)-Dividends_b(j)-Taxes_b(j);

    if (NW_b(1,j)>0)
    {
      Fragility(j)=(baddebt_b(j)+LossEntry_b(j))/(NW_b(1,j));
    }
    else
    {
      Fragility(j)=0;
    }
  } 

  for(i=1; i<=NB; i++)
  {
    if(Deposits_hb.Row(1).Sum()>0)
    {
      DepositShare_h(i)=Deposits_hb(1,i)/Deposits_hb.Row(1).Sum();
    }
    else
    {
      DepositShare_h(i)=(NL_1(i)+NL_2(i))/(N1+N2);
    }
  }
}

void BAILOUT(void)
{
  BankEquity_temp=0;

  for (i=1; i<=NB; ++i)
  {
    BankEquity_temp(i)=NW_b(1,i)/(NL_1(i)+NL_2(i));
  }
  
  max_equity=BankEquity_temp.Maximum();
  
  //Failing banks are rescued
  for (j=1; j<=NB; j++)
  {
    //If a bank has previously been bought by another bank, it will be inactive
    if(Bank_active(j)==1)
    {
      //Banks fail if their net worth is negative
      if(NW_b(1,j) < 0)
      { 
        counter_bankfailure+=1;
        //When flagbailout==0, all failing banks are rescued by the government
        if (flagbailout==0) 
        { 
          if(max_equity>0)
          {
            multip_bailout=ran1(p_seed);
            multip_bailout=b1inf+multip_bailout*(b1sup-b1inf);

            if((multip_bailout*max_equity*(NL_1(j)+NL_2(j)))<(credit_multiplier*Loans_b(1,j)))
            {
              Bailout_b(j)=-NW_b(1,j)+credit_multiplier*Loans_b(1,j);
            }
            else
            {
              Bailout_b(j)=-NW_b(1,j)+multip_bailout*max_equity*(NL_1(j)+NL_2(j));
            }
            Bailout+=Bailout_b(j);
          }
          else
          {
            multip_bailout=ran1(p_seed);
            multip_bailout=b2inf+multip_bailout*(b2sup-b2inf);
            
            if((multip_bailout*NW_b(2,j))<(credit_multiplier*Loans_b(1,j)))
            {
              Bailout_b(j)=-NW_b(1,j)+credit_multiplier*Loans_b(1,j);
            }
            else
            {
              Bailout_b(j)=-NW_b(1,j)+multip_bailout*NW_b(2,j);
            }

            Bailout+=Bailout_b(j);
          }

          Inflows(j)+=Bailout_b(j);
          NW_b(1,j)+=Bailout_b(j);
        }	
        //When flagbailout==1, we check first whether the largest surviving bank can purchase the failing bank
        else
        {

          LossAbsorbed(j)=-NW_b(1,j);

          BankEquity_temp=0;
          for (i=1; i<=NB; ++i)            
          {
            BankEquity_temp(i)=NW_b(1,i); 
          }
          
          maxbank=0;
          max_equity=BankEquity_temp.Maximum1(maxbank);
          // If the failing bank can be purchased by the largest surviving bank, the surviving bank absorbs the negative net worth of the failing bank
		      if (max_equity>0 && (NW_b(1,maxbank) - LossAbsorbed(j)> 0))
          {
			      //All relevant values are added to those of the purchasing bank
            Inflows(maxbank)+=Inflows(j);
            Outflows(maxbank)+=Outflows(j);
            Deposits(1,maxbank)+=Deposits(1,j);
            Deposits(2,maxbank)+=Deposits(2,j);
            Advances_b(1,maxbank)+=Advances_b(1,j);
            Advances_b(2,maxbank)+=Advances_b(2,j);
            Reserves_b(1,maxbank)+=Reserves_b(1,j);
            Reserves_b(2,maxbank)+=Reserves_b(2,j);
            GB_b(1,maxbank)+=GB_b(1,j);
            GB_b(2,maxbank)+=GB_b(2,j);
            Deposits_hb(1,maxbank)+=Deposits_hb(1,j);
            Deposits_hb(2,maxbank)+=Deposits_hb(2,j);
            Deposits_eb(1,maxbank)+=Deposits_eb(1,j);
            Deposits_eb(2,maxbank)+=Deposits_eb(2,j);
            DepositShare_h(maxbank)+=DepositShare_h(j);
            DepositShare_e(maxbank)+=DepositShare_e(j);
            Loans_b(1,maxbank)+=Loans_b(1,j);
            Loans_b(2,maxbank)+=Loans_b(2,j);
            capitalRecovered(maxbank)+=capitalRecovered(j);
            LossEntry_b(maxbank)+=LossEntry_b(j);
            NW_b(1,maxbank)-=LossAbsorbed(j);
            NW_b(2,maxbank)+=NW_b(2,j);
            NL_2(maxbank)+=NL_2(j);
            NL_1(maxbank)+=NL_1(j);
            //Failing bank becomes inactive
            Bank_active(j)=0;
            //Purchasing bank receives failing bank's customers
            for (i=1; i<=N2; i++)
            {
              if(BankMatch_2(i,j)==1)
              {
                BankMatch_2(i,maxbank)=1;   
                BankMatch_2(i,j)=0;
                BankingSupplier_2(i)=maxbank;
              }
            }

            for (i=1; i<=N1; i++)
            {
              if(BankMatch_1(i,j)==1)
              {
                BankMatch_1(i,maxbank)=1;   
                BankMatch_1(i,j)=0;
                BankingSupplier_1(i)=maxbank;
              }
            }

            //All relevant variables of failing bank are set to 0
					  Inflows(j)=0;
            Outflows(j)=0;
            Deposits(1,j)=0;
            Advances_b(1,j)=0;
            Reserves_b(1,j)=0;
            GB_b(1,j)=0;
            Deposits_hb(1,j)=0;
            Deposits_eb(1,j)=0;
            DepositShare_h(j)=0;
            DepositShare_e(j)=0;
            Loans_b(1,j)=0;
            capitalRecovered(j)=0;
            LossEntry_b(j)=0;
            NW_b(1,j)=0;
            NL_2(j)=0;
            NL_1(j)=0;
            Deposits(2,j)=0;
            Advances_b(2,j)=0;
            Reserves_b(2,j)=0;
            GB_b(2,j)=0;
            Deposits_hb(2,j)=0;
            Deposits_eb(2,j)=0;
            Loans_b(2,j)=0;
            NW_b(2,j)=0;
          }
          else
          {
            //If the largest bank is unable to save the failing bank, the government steps in
            multip_bailout=0;
            multip_bailout=ran1(p_seed);
            multip_bailout=b2inf+multip_bailout*(b2sup-b2inf);

            BankEquity_temp=0;

            for (i=1; i<=NB; ++i)
            {
              BankEquity_temp(i)=NW_b(1,i)/(NL_1(i)+NL_2(i));
            }
            
            max_equity=BankEquity_temp.Maximum();

            if(max_equity>0)
            {
              multip_bailout=ran1(p_seed);
              multip_bailout=b1inf+multip_bailout*(b1sup-b1inf);

              if((multip_bailout*max_equity*(NL_1(j)+NL_2(j)))<(credit_multiplier*Loans_b(1,j)))
              {
                Bailout_b(j)=-NW_b(1,j)+credit_multiplier*Loans_b(1,j);
              }
              else
              {
                Bailout_b(j)=-NW_b(1,j)+multip_bailout*max_equity*(NL_1(j)+NL_2(j));
              }
              Bailout+=Bailout_b(j);
            }
            else
            {
              multip_bailout=ran1(p_seed);
              multip_bailout=b2inf+multip_bailout*(b2sup-b2inf);
              
              if((multip_bailout*NW_b(2,j))<(credit_multiplier*Loans_b(1,j)))
              {
                Bailout_b(j)=-NW_b(1,j)+credit_multiplier*Loans_b(1,j);
              }
              else
              {
                Bailout_b(j)=-NW_b(1,j)+multip_bailout*NW_b(2,j);
              }

              Bailout+=Bailout_b(j);
            }

            Inflows(j)+=Bailout_b(j);
            NW_b(1,j)+=Bailout_b(j);
            Bailout+=Bailout_b(j);
          }
        }
      }
    }
  } 

}

void SETTLEMENT(void)
{
  //Calculate central bank profit
  ProfitCB(1)=InterestBonds_cb+InterestAdvances-InterestReserves;
  
  //End of period reserve balances are calculated for each bank by comparing sum of transactions implying outflows of reserves to those implying inflows of reserves
  for (j=1; j<=NB; j++)
  {
    if(Bank_active(j)==1)
    {
      ReserveBalance(j)=Inflows(j)-Outflows(j);
      //If the bank experienced a net inflow of reserves over the period, it repays any CB advances it may have and adds the rest to stock of reserves
      if(ReserveBalance(j)>=0)
      {
        if(Advances_b(1,j)>0)
        {
          if(Advances_b(1,j)>=ReserveBalance(j))
          {
            Advances_b(1,j)-=ReserveBalance(j);
            Advances(1)-=ReserveBalance(j);
            ReserveBalance(j)=0;
          }
          else
          {
            Reserves_b(1,j)+=ReserveBalance(j)-Advances_b(1,j);
            Reserves(1)+=ReserveBalance(j)-Advances_b(1,j);
            Advances(1)-=Advances_b(1,j);
            Advances_b(1,j)=0;
            ReserveBalance(j)=0;
          }
        }
        else
        {
          Reserves_b(1,j)+=ReserveBalance(j);
          Reserves(1)+=ReserveBalance(j);
          ReserveBalance(j)=0;
        }
      }
      //If the bank experienced a net outflow of reserves, it first draws down its stock of reserves and if necessary takes advances from the CB
      else
      {
        if(Reserves_b(1,j)>0)
        {
          if(Reserves_b(1,j)>=(-ReserveBalance(j)))
          {
            Reserves_b(1,j)-=(-ReserveBalance(j));
            Reserves(1)-=(-ReserveBalance(j));
            ReserveBalance(j)=0;
          }
          else
          {
            Advances_b(1,j)+=(-ReserveBalance(j))-Reserves_b(1,j);
            Advances(1)+=(-ReserveBalance(j))-Reserves_b(1,j);
            Reserves(1)-=Reserves_b(1,j);
            Reserves_b(1,j)=0;
            ReserveBalance(j)=0;
          }
        }
        else
        {
          Advances_b(1,j)+=(-ReserveBalance(j));
          Advances(1)+=(-ReserveBalance(j));
          ReserveBalance(j)=0;
        }
      }
    }
  }
}

