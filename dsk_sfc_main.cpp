#include "dsk_sfc_include.h"
using namespace std;

int main(int argc, char *argv[])
{
  CLI::App app{"DSK_SFC, the Dystopian Schumpeter meeting Keynes Stock Flow Consistent model"};

  string inputstring = "default.json";
  app.add_option("inputfile", inputstring, "A path to the json input file")
      ->required()
      ->check(CLI::ExistingFile);

  string str_runname;
  app.add_option("-r,--run", str_runname, "A name for the run (without spaces)")
      ->default_val("test");

  int exseed{1};
  app.add_option("-s", exseed, "A seed (positive integer)")
      ->default_val(1)
      ->check(CLI::PositiveNumber);

  int fullout{0};
  app.add_option("-f,--fulloutput", fullout, "If set to 1, full output will be saved");

  int cerr_enabled{0};
  app.add_option("-c,--cerr", cerr_enabled, "If set to 1, print error messages to the console");

  int verbose{0};
  app.add_option("-v,--verbose", verbose, "If set to 1, print simulation progress");

  CLI11_PARSE(app, argc, argv);

  if (cerr_enabled == 0)
  {
    std::cerr.setstate(std::ios_base::failbit);
  }

  // Initialise variables needed to process console inputs
  std::string seedstring = to_string(exseed);
  char const *seednumber = seedstring.c_str();
  char *exec_dir;
  char *runname = &str_runname[0];
  fulloutput = fullout;

  // Path to executable as char
  exec_dir = argv[0];

  // Seed needs to be converted to negative integer to work with functions generating random draws
  exseed = -exseed;

  // Read JSON file and convert it to a document from which to read param, init and flag values
  using namespace rapidjson;

  std::ifstream ifs{inputstring};
  if (!ifs.is_open())
  {
    cout << "Could not open input file for reading!" << endl;
    return EXIT_FAILURE;
  }

  IStreamWrapper isw{ifs};
  Document inputs{};
  inputs.ParseStream(isw);

  if (inputs.HasParseError())
  {
    cout << "Parsing of input file failed!" << endl;
    return EXIT_FAILURE;
  }
  if (verbose)
  {
    cout << "Finished parsing input file" << endl;
  }

  SETPARAMS(inputs);
  if (verbose)
  {
    cout << "Exiting function SETPARAMS" << endl;
  }

  FOLDERS(exec_dir);
  if (verbose)
  {
    cout << "Finished creating output folders" << endl;
  }

  // Generate name for the error file
  strcpy(errorfilename, "output/errors/Errors_");
  char *name_error = strcat(errorfilename, runname);
  name_error = strcat(errorfilename, "_");
  name_error = strcat(errorfilename, seednumber);
  strcat(errorfilename, ".txt");

  char pathname[32];
  char *filepath = strcpy(pathname, "output");

  if (fulloutput == 1)
  {

    GENFILEPRODALL1_en(filepath, "/A1all_en", runname, seednumber);
    GENFILEPRODALL2_en(filepath, "/A2all_en", runname, seednumber);
    GENFILEPRODALL1_ef(filepath, "/A1all_ef", runname, seednumber);
    GENFILEPRODALL2_ef(filepath, "/A2all_ef", runname, seednumber);
    GENFILEPROD1(filepath, "/A1", runname, seednumber);
    GENFILEPROD2(filepath, "/A2", runname, seednumber);
    GENFILEPRODALL1(filepath, "/A1_all", runname, seednumber);
    GENFILEPRODALL2(filepath, "/A2_all", runname, seednumber);
    GENFILEOUTPUT1(filepath, "/out", runname, seednumber);
    GENFILENWALL1(filepath, "/NW1all", runname, seednumber);
    GENFILENWALL2(filepath, "/NW2all", runname, seednumber);
    GENFILENWALL3(filepath, "/NWBall", runname, seednumber);
    GENFILEDEBALL2(filepath, "/Deb2all", runname, seednumber);
  }

  if (flag_shockexperiment == 1)
  {
    GENFILESHOCKEXP(filepath, "/resultsexp", runname, seednumber);
    if (flag_exogenousshocks == 0)
    {
      GENFILESHOCKPARS(filepath, "/shockpars", runname, seednumber);
    }

    if (NR > 0)
    {
      region_resultsexp_names.resize(NR);
      region_resultsexp_streams.resize(NR);
      for (int rr = 1; rr <= NR; ++rr)
      {
        std::ostringstream oss;
        oss << filepath << "/resultsexp_reg" << rr << "_" << runname << "_" << seednumber << ".txt";
        region_resultsexp_names[rr - 1] = oss.str();
        region_resultsexp_streams[rr - 1].open(region_resultsexp_names[rr - 1], std::ios::out | std::ios::trunc);
      }
    }
  }
  else if (flag_validation == 1)
  {
    GENFILEVALIDATION1(filepath, "/validation1", seednumber);
    GENFILEVALIDATION2(filepath, "/validation2", seednumber);
    GENFILEVALIDATION3(filepath, "/validation3", seednumber);
    GENFILEVALIDATION4(filepath, "/validation4", seednumber);
    GENFILEVALIDATION5(filepath, "/validation5", seednumber);
    GENFILEVALIDATION6(filepath, "/validation6", seednumber);
    GENFILEVALIDATION7(filepath, "/validation7", seednumber);
    GENFILEVALIDATION8(filepath, "/validation8", seednumber);
    GENFILEVALIDATION9(filepath, "/validation9", seednumber);
    GENFILEVALIDATION10(filepath, "/validation10", seednumber);
    GENFILEVALIDATION11(filepath, "/validation11", seednumber);
    GENFILEVALIDATION12(filepath, "/validation12", seednumber);
  }
  else if (fulloutput == 0)
  {
    GENFILEYMC(filepath, "/ymc", runname, seednumber);
  }

  if (verbose)
  {
    cout << "Finished creating output file names" << endl;
  }

  RESIZE();
  if (verbose)
  {
    cout << "Exiting function RESIZE" << endl;
  }

  INITIALIZE(exseed);
  if (verbose)
  {
    cout << "Exiting function INITIALIZE; Entering simulation loop" << endl;
  }

#ifdef __linux__
  // Set the run to time out after T*2 seconds
  signal(SIGALRM, catchAlarm);
  alarm(T * 2);
#endif

  // enter loop over simulation periods
  for (t = 1; t <= T; t++)
  {
    if (verbose)
    {
      cout << "Entering simulation period " << t << endl;
    }

    SETVARS();
    if (verbose)
    {
      cout << "Exiting function SETVARS in period " << t << endl;
    }

    DEPOSITINTEREST();
    if (verbose)
    {
      cout << "Exiting function DEPOSITINTEREST in period " << t << endl;
    }

    if (t % freqclim == 0)
    {
      CLIMATE_POLICY();
      if (verbose)
      {
        cout << "Exiting function CLIMATE_POLICY in period " << t << endl;
      }
    }

    MACH();
    if (verbose)
    {
      cout << "Exiting function MACH in period " << t << endl;
    }

    TOTCREDIT();
    if (verbose)
    {
      cout << "Exiting function TOTCREDIT in period " << t << endl;
    }

    LOANRATES();
    if (verbose)
    {
      cout << "Exiting function LOANRATES in period " << t << endl;
    }

    BROCHURE();
    if (verbose)
    {
      cout << "Exiting function BROCHURE in period " << t << endl;
    }

    INVEST();
    if (verbose)
    {
      cout << "Exiting function INVEST in period " << t << endl;
    }

    ALLOCATECREDIT();
    if (verbose)
    {
      cout << "Exiting function ALLOCATECREDIT in period " << t << endl;
    }

    PRODMACH();
    if (verbose)
    {
      cout << "Exiting function PRODMACH in period " << t << endl;
    }

    EMISS_IND();
    if (verbose)
    {
      cout << "Exiting function EMISS_IND in period " << t << endl;
    }

    ENERGY();
    if (verbose)
    {
      cout << "Exiting function ENERGY in period " << t << endl;
    }

    PAY_LAB_INV();
    if (verbose)
    {
      cout << "Exiting function PAY_LAB_INV in period " << t << endl;
    }

    COMPET2();
    if (verbose)
    {
      cout << "Exiting function COMPET2 in period " << t << endl;
    }

    PROFIT();
    if (verbose)
    {
      cout << "Exiting function PROFIT in period " << t << endl;
    }

    MACRO();
    if (verbose)
    {
      cout << "Exiting function MACRO in period " << t << endl;
    }

    ENTRYEXIT();
    if (verbose)
    {
      cout << "Exiting function ENTRYEXIT in period " << t << endl;
    }

    BANKING();
    if (verbose)
    {
      cout << "Exiting function BANKING in period " << t << endl;
    }

    BAILOUT();
    if (verbose)
    {
      cout << "Exiting function BAILOUT in period " << t << endl;
    }

    GOV_BUDGET();
    if (verbose)
    {
      cout << "Exiting function GOV_BUDGET in period " << t << endl;
    }

    TAYLOR();
    if (verbose)
    {
      cout << "Exiting function TAYLOR in period " << t << endl;
    }

    SETTLEMENT();
    if (verbose)
    {
      cout << "Exiting function SETTLEMENT in period " << t << endl;
    }

    TECHANGEND();
    if (verbose)
    {
      cout << "Exiting function TECHANGEND in period " << t << endl;
    }

    if (t > t_start_climbox && t % freqclim == 0)
    {
      if (flag_cum_emissions == 0)
      {
        CLIMATEBOX();
        if (verbose)
        {
          cout << "Exiting function CLIMATEBOX in period " << t << endl;
        }
      }
      else
      {
        CLIMATEBOX_CUM_EMISS();
        if (verbose)
        {
          cout << "Exiting function CLIMATEBOX_CUM_EMISS in period " << t << endl;
        }
      }
    }

    if (flag_exogenousshocks == 1)
    {
      SINGLESHOCK();
      if (verbose)
      {
        cout << "Exiting function SINGLESHOCK in period " << t << endl;
      }
    }
    else
    {
      SHOCKS();
      if (verbose)
      {
        cout << "Exiting function SHOCKS in period " << t << endl;
      }
    }

    DEPOSITCHECK();
    if (verbose)
    {
      cout << "Exiting function DEPOSITCHECK in period " << t << endl;
    }
    NEGATIVITYCHECK();
    if (verbose)
    {
      cout << "Exiting function NEGATIVITYCHECK in period " << t << endl;
    }
    CHECKSUMS();
    if (verbose)
    {
      cout << "Exiting function CHECKSUMS in period " << t << endl;
    }
    ADJUSTSTOCKS();
    if (verbose)
    {
      cout << "Exiting function ADJUSTSTOCKS in period " << t << endl;
    }
    SFC_CHECK();
    if (verbose)
    {
      cout << "Exiting function SFC_CHECK in period " << t << endl;
    }

    SAVE();
    if (verbose)
    {
      cout << "Exiting function SAVE in period " << t << endl;
    }

    UPDATE();
    if (verbose)
    {
      cout << "Exiting function UPDATE in period " << t << endl;
    }

    UPDATECLIMATE();
    if (verbose)
    {
      cout << "Exiting function UPDATECLIMATE in period " << t << endl;
    }

    OVERBOOST();

    if (verbose)
    {
      cout << "Exiting function OVERBOOST; end of period " << t << endl;
    }
  }
  if (verbose)
  {
    cout << "End of simulation loop" << endl;
  }

  if (NR > 0)
  {
    for (auto &stream : region_resultsexp_streams)
    {
      if (stream.is_open())
      {
        stream.close();
      }
    }
  }

  return (EXIT_SUCCESS);
}

///////////MODEL FUNCTIONS/////////////////////////////

void SETPARAMS(const rapidjson::Document &inputs)
{
  // Read the values of parameters, flags, initial values and one-off shocks from the document "inputs" and set the respective variables to those values
  N1 = inputs["params"][0]["N1"].GetInt();
  N1f = N1;
  N2 = inputs["params"][0]["N2"].GetInt();
  T = inputs["params"][0]["T"].GetInt();
  varphi = inputs["params"][0]["varphi"].GetDouble();
  nu = inputs["params"][0]["nu"].GetDouble();
  xi = inputs["params"][0]["xi"].GetDouble();
  o1 = inputs["params"][0]["o1"].GetDouble();
  o2 = inputs["params"][0]["o2"].GetDouble();
  uu11 = inputs["params"][0]["uu11"].GetDouble();
  uu21 = inputs["params"][0]["uu21"].GetDouble();
  uu12 = inputs["params"][0]["uu12"].GetDouble();
  uu22 = inputs["params"][0]["uu22"].GetDouble();
  uu31 = inputs["params"][0]["uu31"].GetDouble();
  uu41 = inputs["params"][0]["uu41"].GetDouble();
  uu32 = inputs["params"][0]["uu32"].GetDouble();
  uu42 = inputs["params"][0]["uu42"].GetDouble();
  uu51 = inputs["params"][0]["uu51"].GetDouble();
  uu61 = inputs["params"][0]["uu61"].GetDouble();
  uu52 = inputs["params"][0]["uu52"].GetDouble();
  uu62 = inputs["params"][0]["uu62"].GetDouble();
  uinf = inputs["params"][0]["uinf"].GetDouble();
  usup = inputs["params"][0]["usup"].GetDouble();
  b_a11 = inputs["params"][0]["b_a11"].GetDouble();
  b_b11 = inputs["params"][0]["b_b11"].GetDouble();
  b_a12 = inputs["params"][0]["b_a12"].GetDouble();
  b_b12 = inputs["params"][0]["b_b12"].GetDouble();
  b_a2 = inputs["params"][0]["b_a2"].GetDouble();
  b_b2 = inputs["params"][0]["b_b2"].GetDouble();
  b_a3 = inputs["params"][0]["b_a3"].GetDouble();
  b_b3 = inputs["params"][0]["b_b3"].GetDouble();
  mi1 = inputs["params"][0]["mi1"].GetDouble();
  mi2 = inputs["params"][0]["mi2"].GetDouble();
  Gamma = inputs["params"][0]["Gamma"].GetDouble();
  chi = inputs["params"][0]["chi"].GetDouble();
  omega1 = inputs["params"][0]["omega1"].GetDouble();
  omega2 = inputs["params"][0]["omega2"].GetDouble();
  psi1 = inputs["params"][0]["psi1"].GetDouble();
  psi2 = inputs["params"][0]["psi2"].GetDouble();
  psi3 = inputs["params"][0]["psi3"].GetDouble();
  deltami2 = inputs["params"][0]["deltami2"].GetDouble();
  w_min = inputs["params"][0]["w_min"].GetDouble();
  pmin = inputs["params"][0]["pmin"].GetDouble();
  theta = inputs["params"][0]["theta"].GetDouble();
  u = inputs["params"][0]["u"].GetDouble();
  alfa = inputs["params"][0]["alfa"].GetDouble();
  b = inputs["params"][0]["b"].GetDouble();
  dim_mach = inputs["params"][0]["dim_mach"].GetDouble();
  agemax = inputs["params"][0]["agemax"].GetDouble();
  a = inputs["params"][0]["a"].GetDouble();
  credit_multiplier = inputs["params"][0]["credit_multiplier"].GetDouble();
  beta_basel = inputs["params"][0]["beta_basel"].GetDouble();
  bankmarkdown = inputs["params"][0]["bankmarkdown"].GetDouble();
  centralbankmarkdown = inputs["params"][0]["centralbankmarkdown"].GetDouble();
  d1 = inputs["params"][0]["d1"].GetDouble();
  d2 = inputs["params"][0]["d2"].GetDouble();
  db = inputs["params"][0]["db"].GetDouble();
  repayment_share = inputs["params"][0]["repayment_share"].GetDouble();
  bonds_share = inputs["params"][0]["bonds_share"].GetDouble();
  pareto_a = inputs["params"][0]["pareto_a"].GetDouble();
  pareto_k = inputs["params"][0]["pareto_k"].GetDouble();
  pareto_p = inputs["params"][0]["pareto_p"].GetDouble();
  d_cpi_target = inputs["params"][0]["d_cpi_target"].GetDouble();
  ustar = inputs["params"][0]["ustar"].GetDouble();
  w1sup = inputs["params"][0]["w1sup"].GetDouble();
  w1inf = inputs["params"][0]["w1inf"].GetDouble();
  w2sup = inputs["params"][0]["w2sup"].GetDouble();
  w2inf = inputs["params"][0]["w2inf"].GetDouble();
  k_const = inputs["params"][0]["k_const"].GetDouble();
  aliqw = inputs["params"][0]["aliqw"].GetDouble();
  taylor1 = inputs["params"][0]["taylor1"].GetDouble();
  taylor2 = inputs["params"][0]["taylor2"].GetDouble();
  bondsmarkdown = inputs["params"][0]["bondsmarkdown"].GetDouble();
  mdw = inputs["params"][0]["mdw"].GetDouble();
  phi2 = inputs["params"][0]["phi2"].GetDouble();
  b1sup = inputs["params"][0]["b1sup"].GetDouble();
  b1inf = inputs["params"][0]["b1inf"].GetDouble();
  b2sup = inputs["params"][0]["b2sup"].GetDouble();
  b2inf = inputs["params"][0]["b2inf"].GetDouble();
  aliq = inputs["params"][0]["aliq"].GetDouble();
  aliqb = inputs["params"][0]["aliqb"].GetDouble();
  wu = inputs["params"][0]["wu"].GetDouble();
  de = inputs["params"][0]["de"].GetDouble();
  a1 = inputs["params"][0]["a1"].GetDouble();
  a2 = inputs["params"][0]["a2"].GetDouble();
  a3 = inputs["params"][0]["a3"].GetDouble();
  u_low = inputs["params"][0]["u_low"].GetDouble();
  f2_entry_min = inputs["params"][0]["f2_entry_min"].GetDouble();
  kappa = inputs["params"][0]["kappa"].GetDouble();
  taylor = inputs["params"][0]["taylor"].GetDouble();
  omicron = inputs["params"][0]["omicron"].GetDouble();
  I_max = inputs["params"][0]["I_max"].GetDouble();
  persistence = inputs["params"][0]["persistence"].GetDouble();
  omega3 = inputs["params"][0]["omega3"].GetDouble();
  d_f = inputs["params"][0]["d_f"].GetDouble();
  g_ls = inputs["params"][0]["g_ls"].GetDouble();
  aliqe = inputs["params"][0]["aliqe"].GetDouble();
  tre = inputs["params"][0]["tre"].GetDouble();
  passthrough = inputs["params"][0]["passthrough"].GetDouble();

  share_RD_en = inputs["climparams"][0]["share_RD_en"].GetDouble();
  share_de_0 = inputs["climparams"][0]["share_de_0"].GetDouble();
  payback_en = inputs["climparams"][0]["payback_en"].GetInt();
  life_plant = inputs["climparams"][0]["life_plant"].GetInt();
  exp_quota = inputs["climparams"][0]["exp_quota"].GetDouble();
  o1_en = inputs["climparams"][0]["o1_en"].GetDouble();
  uu1_en = inputs["climparams"][0]["uu1_en"].GetDouble();
  uu2_en = inputs["climparams"][0]["uu2_en"].GetDouble();
  exp_quota_param = inputs["params"][0]["exp_quota_param"].GetDouble();
  ge_subsidy = inputs["params"][0]["ge_subsidy"].GetDouble();
  t_start_climbox = inputs["climparams"][0]["t_start_climbox"].GetInt();
  T_pre = inputs["climparams"][0]["T_pre"].GetDouble();
  intercept_temp = inputs["climparams"][0]["intercept_temp"].GetDouble();
  slope_temp = inputs["climparams"][0]["slope_temp"].GetDouble();
  tc1 = inputs["climparams"][0]["tc1"].GetDouble();
  tc2 = inputs["climparams"][0]["tc2"].GetDouble();
  ndep = inputs["climparams"][0]["ndep"].GetInt();
  laydep.ReSize(ndep);
  laydep(1) = inputs["climparams"][0]["laydep1"].GetDouble();
  laydep(2) = inputs["climparams"][0]["laydep2"].GetDouble();
  laydep(3) = inputs["climparams"][0]["laydep3"].GetDouble();
  laydep(4) = inputs["climparams"][0]["laydep4"].GetDouble();
  laydep(5) = inputs["climparams"][0]["laydep5"].GetDouble();
  fertil = inputs["climparams"][0]["fertil"].GetDouble();
  heatstress = inputs["climparams"][0]["heatstress"].GetDouble();
  humtime = inputs["climparams"][0]["humtime"].GetDouble();
  biotime = inputs["climparams"][0]["biotime"].GetDouble();
  humfrac = inputs["climparams"][0]["humfrac"].GetDouble();
  eddydif = inputs["climparams"][0]["eddydif"].GetDouble();
  ConrefT = inputs["climparams"][0]["ConrefT"].GetDouble();
  rev0 = inputs["climparams"][0]["rev0"].GetDouble();
  revC = inputs["climparams"][0]["revC"].GetDouble();
  niterclim = inputs["climparams"][0]["niterclim"].GetInt();
  forCO2 = inputs["climparams"][0]["forCO2"].GetDouble();
  otherforcefac = inputs["climparams"][0]["otherforcefac"].GetDouble();
  outrad = inputs["climparams"][0]["outrad"].GetDouble();
  secyr = inputs["climparams"][0]["secyr"].GetDouble();
  seasurf = inputs["climparams"][0]["seasurf"].GetDouble();
  heatcap = inputs["climparams"][0]["heatcap"].GetDouble();
  freqclim = inputs["climparams"][0]["freqclim"].GetInt();
  g_emiss_global = inputs["climparams"][0]["g_emiss_global"].GetDouble();
  emiss_share = inputs["climparams"][0]["emiss_share"].GetDouble();

  // Regional inputs (derivative-only accounting layer)
  regionalaccountingtolerance = 1e-6;
  if (inputs.HasMember("regions"))
  {
    NR = inputs["regions"]["NR"].GetInt();
    region_K_shares.ReSize(NR);
    region_C_shares.ReSize(NR);
    region_energy_dirty_shares.ReSize(NR);
    region_energy_green_shares.ReSize(NR);
    relocation_probability_K.ReSize(NR);
    relocation_probability_C.ReSize(NR);
    ge_growth_probability.ReSize(NR);
    de_growth_probability.ReSize(NR);

    for (i = 1; i <= NR; i++)
    {
      region_K_shares(i) = inputs["regions"]["K_firm_shares"][(i - 1)].GetDouble();
      region_C_shares(i) = inputs["regions"]["C_firm_shares"][(i - 1)].GetDouble();
      region_energy_dirty_shares(i) = inputs["regions"]["energy"]["dirty_capacity_shares"][(i - 1)].GetDouble();
      region_energy_green_shares(i) = inputs["regions"]["energy"]["green_capacity_shares"][(i - 1)].GetDouble();
      relocation_probability_K(i) = inputs["regions"]["relocation_probability_K"][(i - 1)].GetDouble();
      relocation_probability_C(i) = inputs["regions"]["relocation_probability_C"][(i - 1)].GetDouble();
      de_growth_probability(i) = inputs["regions"]["de_growth_probability"][(i - 1)].GetDouble();
      ge_growth_probability(i) = inputs["regions"]["ge_growth_probability"][(i - 1)].GetDouble();
    }
  }

  nshocks = inputs["climshockparams"][0]["nshocks"].GetInt();
  a_0.ReSize(nshocks);
  a_0(1) = inputs["climshockparams"][0]["a1_0"].GetDouble();
  a_0(2) = inputs["climshockparams"][0]["a2_0"].GetDouble();
  a_0(3) = inputs["climshockparams"][0]["a3_0"].GetDouble();
  a_0(4) = inputs["climshockparams"][0]["a4_0"].GetDouble();
  a_0(5) = inputs["climshockparams"][0]["a5_0"].GetDouble();
  a_0(6) = inputs["climshockparams"][0]["a6_0"].GetDouble();
  a_0(7) = inputs["climshockparams"][0]["a7_0"].GetDouble();
  a_0(8) = inputs["climshockparams"][0]["a8_0"].GetDouble();
  a_0(9) = inputs["climshockparams"][0]["a9_0"].GetDouble();
  b_0.ReSize(nshocks);
  b_0(1) = inputs["climshockparams"][0]["b1_0"].GetDouble();
  b_0(2) = inputs["climshockparams"][0]["b2_0"].GetDouble();
  b_0(3) = inputs["climshockparams"][0]["b3_0"].GetDouble();
  b_0(4) = inputs["climshockparams"][0]["b4_0"].GetDouble();
  b_0(5) = inputs["climshockparams"][0]["b5_0"].GetDouble();
  b_0(6) = inputs["climshockparams"][0]["b6_0"].GetDouble();
  b_0(7) = inputs["climshockparams"][0]["b7_0"].GetDouble();
  b_0(8) = inputs["climshockparams"][0]["b8_0"].GetDouble();
  b_0(9) = inputs["climshockparams"][0]["b9_0"].GetDouble();
  shockexponent1.ReSize(nshocks);
  shockexponent1(1) = inputs["climshockparams"][0]["shockexponent1_1"].GetDouble();
  shockexponent1(2) = inputs["climshockparams"][0]["shockexponent2_1"].GetDouble();
  shockexponent1(3) = inputs["climshockparams"][0]["shockexponent3_1"].GetDouble();
  shockexponent1(4) = inputs["climshockparams"][0]["shockexponent4_1"].GetDouble();
  shockexponent1(5) = inputs["climshockparams"][0]["shockexponent5_1"].GetDouble();
  shockexponent1(6) = inputs["climshockparams"][0]["shockexponent6_1"].GetDouble();
  shockexponent1(7) = inputs["climshockparams"][0]["shockexponent7_1"].GetDouble();
  shockexponent1(8) = inputs["climshockparams"][0]["shockexponent8_1"].GetDouble();
  shockexponent1(9) = inputs["climshockparams"][0]["shockexponent9_1"].GetDouble();
  shockexponent2.ReSize(nshocks);
  shockexponent2(1) = inputs["climshockparams"][0]["shockexponent1_2"].GetDouble();
  shockexponent2(2) = inputs["climshockparams"][0]["shockexponent2_2"].GetDouble();
  shockexponent2(3) = inputs["climshockparams"][0]["shockexponent3_2"].GetDouble();
  shockexponent2(4) = inputs["climshockparams"][0]["shockexponent4_2"].GetDouble();
  shockexponent2(5) = inputs["climshockparams"][0]["shockexponent5_2"].GetDouble();
  shockexponent2(6) = inputs["climshockparams"][0]["shockexponent6_2"].GetDouble();
  shockexponent2(7) = inputs["climshockparams"][0]["shockexponent7_2"].GetDouble();
  shockexponent2(8) = inputs["climshockparams"][0]["shockexponent8_2"].GetDouble();
  shockexponent2(9) = inputs["climshockparams"][0]["shockexponent9_2"].GetDouble();

  A0 = inputs["inits"][0]["A0"].GetDouble();
  LS0 = inputs["inits"][0]["LS0"].GetDouble();
  W10 = inputs["inits"][0]["W10"].GetDouble();
  W20 = inputs["inits"][0]["W20"].GetDouble();
  L0 = inputs["inits"][0]["L0"].GetDouble();
  w0 = inputs["inits"][0]["w0"].GetDouble();
  K0 = inputs["inits"][0]["K0"].GetDouble();
  bankmarkup_init = inputs["inits"][0]["bankmarkup_init"].GetDouble();
  A0_en = inputs["inits"][0]["A0_en"].GetInt();
  A0_ef = inputs["inits"][0]["A0_ef"].GetInt();
  K_ge0_perc = inputs["inits"][0]["K_ge0_perc"].GetDouble();
  pf0 = inputs["inits"][0]["pf0"].GetDouble();
  mi_en0 = inputs["inits"][0]["mi_en0"].GetDouble();
  A_de0 = inputs["inits"][0]["A_de0"].GetDouble();
  EM0 = inputs["inits"][0]["EM0"].GetDouble();
  CF_ge0 = inputs["inits"][0]["CF_ge0"].GetDouble();
  t_CO2_0 = inputs["inits"][0]["t_CO2_0"].GetDouble();
  t_CO2_en_0 = inputs["inits"][0]["t_CO2_en_0"].GetDouble();
  r_base = inputs["inits"][0]["r_base"].GetDouble();
  d_Am_init = inputs["inits"][0]["d_Am_init"].GetDouble();
  D_h0 = inputs["inits"][0]["D_h0"].GetDouble();
  NW_b0 = inputs["inits"][0]["NW_b0"].GetDouble();
  pm = inputs["inits"][0]["pm"].GetDouble();
  D_e0 = inputs["inits"][0]["D_e0"].GetDouble();

  Emiss_yearly_0 = inputs["climinits"][0]["Emiss_yearly_0"].GetDouble();
  Cum_emissions_0 = inputs["climinits"][0]["Cum_emissions_0"].GetDouble();
  T_0_cumemiss = inputs["climinits"][0]["T_0_cumemiss"].GetDouble();
  Con00 = inputs["climinits"][0]["Con00"].GetDouble();
  Conref = Con00 * laydep(1);
  NPP0 = inputs["climinits"][0]["NPP0"].GetDouble();
  Cat0 = inputs["climinits"][0]["Cat0"].GetDouble();
  Catinit0 = inputs["climinits"][0]["Catinit0"].GetDouble();
  Honinit0.ReSize(ndep);
  Coninit0.ReSize(ndep);
  Toninit0.ReSize(ndep);
  Honinit0(1) = inputs["climinits"][0]["Honinit01"].GetDouble();
  Honinit0(2) = inputs["climinits"][0]["Honinit02"].GetDouble();
  Honinit0(3) = inputs["climinits"][0]["Honinit03"].GetDouble();
  Honinit0(4) = inputs["climinits"][0]["Honinit04"].GetDouble();
  Honinit0(5) = inputs["climinits"][0]["Honinit05"].GetDouble();
  Coninit0(1) = inputs["climinits"][0]["Coninit01"].GetDouble();
  Coninit0(2) = inputs["climinits"][0]["Coninit02"].GetDouble();
  Coninit0(3) = inputs["climinits"][0]["Coninit03"].GetDouble();
  Coninit0(4) = inputs["climinits"][0]["Coninit04"].GetDouble();
  Coninit0(5) = inputs["climinits"][0]["Coninit05"].GetDouble();
  Toninit0(1) = inputs["climinits"][0]["Toninit01"].GetDouble();
  Toninit0(2) = inputs["climinits"][0]["Toninit02"].GetDouble();
  Toninit0(3) = inputs["climinits"][0]["Toninit03"].GetDouble();
  Toninit0(4) = inputs["climinits"][0]["Toninit04"].GetDouble();
  Toninit0(5) = inputs["climinits"][0]["Toninit05"].GetDouble();
  Tmixedinit0 = Toninit0(1);
  biominit0 = inputs["climinits"][0]["biominit0"].GetDouble();
  huminit0 = inputs["climinits"][0]["huminit0"].GetDouble();
  Catinit1 = inputs["climinits"][0]["Catinit1"].GetDouble();
  Honinit1.ReSize(ndep);
  Coninit1.ReSize(ndep);
  Toninit1.ReSize(ndep);
  Honinit1(1) = inputs["climinits"][0]["Honinit11"].GetDouble();
  Honinit1(2) = inputs["climinits"][0]["Honinit12"].GetDouble();
  Honinit1(3) = inputs["climinits"][0]["Honinit13"].GetDouble();
  Honinit1(4) = inputs["climinits"][0]["Honinit14"].GetDouble();
  Honinit1(5) = inputs["climinits"][0]["Honinit15"].GetDouble();
  Coninit1(1) = inputs["climinits"][0]["Coninit11"].GetDouble();
  Coninit1(2) = inputs["climinits"][0]["Coninit12"].GetDouble();
  Coninit1(3) = inputs["climinits"][0]["Coninit13"].GetDouble();
  Coninit1(4) = inputs["climinits"][0]["Coninit14"].GetDouble();
  Coninit1(5) = inputs["climinits"][0]["Coninit15"].GetDouble();
  Toninit1(1) = inputs["climinits"][0]["Toninit11"].GetDouble();
  Toninit1(2) = inputs["climinits"][0]["Toninit12"].GetDouble();
  Toninit1(3) = inputs["climinits"][0]["Toninit13"].GetDouble();
  Toninit1(4) = inputs["climinits"][0]["Toninit14"].GetDouble();
  Toninit1(5) = inputs["climinits"][0]["Toninit15"].GetDouble();
  Tmixedinit1 = Toninit1(1);
  biominit1 = inputs["climinits"][0]["biominit1"].GetDouble();
  huminit1 = inputs["climinits"][0]["huminit1"].GetDouble();

  flag_shockexperiment = inputs["flags"][0]["flag_shockexperiment"].GetInt();
  flag_exogenousshocks = inputs["flags"][0]["flag_exogenousshocks"].GetInt();
  flag_cum_emissions = inputs["flags"][0]["flag_cum_emissions"].GetInt();
  flag_tax_CO2 = inputs["flags"][0]["flag_tax_CO2"].GetInt();
  flag_capshocks = inputs["flags"][0]["flag_capshocks"].GetInt();
  flag_outputshocks = inputs["flags"][0]["flag_outputshocks"].GetInt();
  flag_inventshocks = inputs["flags"][0]["flag_inventshocks"].GetInt();
  flag_encapshocks = inputs["flags"][0]["flag_encapshocks"].GetInt();
  flag_popshocks = inputs["flags"][0]["flag_popshocks"].GetInt();
  flag_energyshocks = inputs["flags"][0]["flag_energyshocks"].GetInt();
  flag_demandshocks = inputs["flags"][0]["flag_demandshocks"].GetInt();
  flag_RDshocks = inputs["flags"][0]["flag_RDshocks"].GetInt();
  flag_prodshocks1 = inputs["flags"][0]["flag_prodshocks1"].GetInt();
  flag_prodshocks2 = inputs["flags"][0]["flag_prodshocks2"].GetInt();
  flag_share_END = inputs["flags"][0]["flag_share_END"].GetInt();
  flag_energy_exp = inputs["flags"][0]["flag_energy_exp"].GetInt();
  flagbailout = inputs["flags"][0]["flagbailout"].GetInt();
  flag_entry = inputs["flags"][0]["flag_entry"].GetInt();
  flag_nonCO2_force = inputs["flags"][0]["flag_nonCO2_force"].GetInt();
  flag_validation = inputs["flags"][0]["flag_validation"].GetInt();
  flag_inventories = inputs["flags"][0]["flag_inventories"].GetInt();
  flag_scrap_age = inputs["flags"][0]["flag_scrap_age"].GetInt();
  flag_uniformshocks = inputs["flags"][0]["flag_uniformshocks"].GetInt();
  flag_desc = inputs["flags"][0]["flag_desc"].GetInt();

  shocks_cfirms.ReSize(N2);
  shocks_kfirms.ReSize(N1);
  for (i = 1; i <= N1; i++)
  {
    shocks_kfirms(i) = inputs["shocks_kfirms"][(i - 1)].GetDouble();
  }
  for (j = 1; j <= N2; j++)
  {
    shocks_cfirms(j) = inputs["shocks_cfirms"][(j - 1)].GetDouble();
  }
  shock_scalar = inputs["shock_scalar"].GetDouble();
}

void RESIZE(void)
{
  // Resize all vectors, matrices and arrays to the correct dimension
  X_a.ReSize(nshocks);
  X_b.ReSize(nshocks);
  N1f = N1;
  A.ReSize(T, N1);
  C.ReSize(T, N1);
  C_secondhand.ReSize(T, N1);
  A_en.ReSize(T, N1);
  A_ef.ReSize(T, N1);
  A_de.ReSize(T);
  EM_de.ReSize(T);
  C_de.ReSize(T);
  G_de.ReSize(T);
  G_ge.ReSize(T);
  G_ge_n.ReSize(T);
  CF_ge.ReSize(T);
  IC_en_quota.ReSize(T);
  G_de_temp.ReSize(T);
  shocks_encapstock_de.ReSize(T);
  shocks_encapstock_ge.ReSize(T);
  D2.ReSize(2, N2);
  De.ReSize(N2);
  f2.ReSize(3, N2);
  E2.ReSize(N2);
  c2.ReSize(N2);
  c2p.ReSize(N2);
  Q2.ReSize(N2);
  N.ReSize(2, N2);
  Inventories.ReSize(2, N2);
  Deposits_2.ReSize(2, N2);
  NW_2.ReSize(2, N2);
  NW_2_c.ReSize(N2);
  CapitalStock.ReSize(2, N2);
  deltaCapitalStock.ReSize(2, N2);
  DebtServiceToSales2.ReSize(N2);
  scrap_age.ReSize(N2);
  I.ReSize(N2);
  EI.ReSize(2, N2);
  EI_n.ReSize(N2);
  SI.ReSize(N2);
  SI_n.ReSize(N2);
  S2.ReSize(2, N2);
  Sales2.ReSize(N2);
  Loans_2.ReSize(2, N2);
  DebtService_2.ReSize(2, N2);
  CreditDemand.ReSize(N2);
  p2.ReSize(N2);
  DebtServiceToSales2_temp.ReSize(N2);
  mu2.ReSize(2, N2);
  LoanInterest_2.ReSize(N2);
  DebtRemittances2.ReSize(N2);
  baddebt_2.ReSize(N2);
  EId.ReSize(N2);
  SId.ReSize(N2);
  SId_age.ReSize(N2);
  SId_cost.ReSize(N2);
  SIp.ReSize(N2);
  SIp_age.ReSize(N2);
  SIp_cost.ReSize(N2);
  EIp.ReSize(N2);
  Ip.ReSize(N2);
  A2.ReSize(N2);
  A2e.ReSize(N2);
  c2e.ReSize(N2);
  Ld2.ReSize(N2);
  Ld2_control.ReSize(N2);
  l2.ReSize(N2);
  n_mach.ReSize(N2);
  Qd.ReSize(N2);
  K.ReSize(N2);
  K_cur.ReSize(N2);
  Kd.ReSize(N2);
  Ktrig.ReSize(N2);
  Pi2.ReSize(N2);
  Q2temp.ReSize(N2);
  Wages_2.ReSize(N2);
  Investment_2.ReSize(N2);
  EnergyPayments_2.ReSize(N2);
  InterestDeposits_2.ReSize(N2);
  Taxes_2.ReSize(N2);
  Taxes_CO2_2.ReSize(N2);
  f_temp2.ReSize(N2);
  D_temp2.ReSize(N2);
  dN.ReSize(N2);
  dNm.ReSize(N2);
  fornit.ReSize(N2);
  Cmach.ReSize(N2);
  CmachEI.ReSize(N2);
  CmachSI.ReSize(N2);
  CmachSI_age.ReSize(N2);
  CmachSI_cost.ReSize(N2);
  Ne.ReSize(N2);
  mol.ReSize(N2);
  Dividends_2.ReSize(N2);
  Match.ReSize(N2, N1);
  BankingSupplier_2.ReSize(N2);
  r_deb_h.ReSize(N2);
  k.ReSize(N2);
  D2_en.ReSize(N2);
  A2e_en.ReSize(N2);
  A2e_ef.ReSize(N2);
  A2e2.ReSize(N2);
  A2e_en2.ReSize(N2);
  A2e_ef2.ReSize(N2);
  A2_en.ReSize(N2);
  A2_ef.ReSize(N2);
  Emiss2.ReSize(N2);
  Injection_2.ReSize(N2);
  exiting_2.ReSize(N2);
  exit_payments2.ReSize(N2);
  exit_equity2.ReSize(N2);
  exit_marketshare2.ReSize(N2);
  n_mach_entry.ReSize(N2);
  shocks_capstock.ReSize(N2);
  shocks_invent.ReSize(N2);
  Loss_Capital.ReSize(N2);
  Loss_Inventories.ReSize(N2);
  k_entry.ReSize(N2);
  EntryShare.ReSize(N2);
  CompEntry.ReSize(N2);
  K_temp.ReSize(N2);
  K_loss.ReSize(N2);
  marker_age.ReSize(N2);
  risk_c.ReSize(N2);
  risk_k.ReSize(N2);
  pass_2.ReSize(N2);
  pass_1.ReSize(N1);
  Deposits_1.ReSize(2, N1);
  NW_1.ReSize(2, N1);
  NW_1_c.ReSize(N1);
  Wages_1.ReSize(N1);
  S1.ReSize(N1);
  Sales1.ReSize(N1);
  p1.ReSize(N1);
  RD.ReSize(2, N1);
  f1.ReSize(2, N1);
  Q1.ReSize(N1);
  D1.ReSize(N1);
  A1.ReSize(N1);
  Anew.ReSize(N1);
  A1inn.ReSize(N1);
  A1pinn.ReSize(N1);
  A1imm.ReSize(N1);
  A1pimm.ReSize(N1);
  Pi1.ReSize(N1);
  Ld1.ReSize(N1);
  Ld1rd.ReSize(N1);
  ee1.ReSize(N1);
  nclient.ReSize(N1);
  Dividends_1.ReSize(N1);
  EnergyPayments_1.ReSize(N1);
  InterestDeposits_1.ReSize(N1);
  Taxes_1.ReSize(N1);
  Taxes_CO2_1.ReSize(N1);
  BankingSupplier_1.ReSize(N1);
  c1.ReSize(N1);
  A1p.ReSize(N1);
  RDin.ReSize(N1);
  RDim.ReSize(N1);
  Inn.ReSize(N1);
  Imm.ReSize(N1);
  A1f.ReSize(N1);
  A1pf.ReSize(N1);
  A1w.ReSize(N1 + N1f);
  A1pw.ReSize(N1 + N1f);
  Td.ReSize(N1 + 1);
  Tdw.ReSize(N1 + N1f + 1);
  D1_en.ReSize(N1);
  A1_en.ReSize(N1);
  A1_ef.ReSize(N1);
  A1p_en.ReSize(N1);
  A1p_ef.ReSize(N1);
  EE_inn.ReSize(N1);
  EEp_inn.ReSize(N1);
  EE_imm.ReSize(N1);
  EEp_imm.ReSize(N1);
  EF_inn.ReSize(N1);
  EFp_inn.ReSize(N1);
  EF_imm.ReSize(N1);
  EFp_imm.ReSize(N1);
  Emiss1.ReSize(N1);
  Injection_1.ReSize(N1);
  Balances_1.ReSize(N1);
  baddebt_1.ReSize(N1);
  exiting_1.ReSize(N1);
  c1p.ReSize(N1);
  shocks_machprod.ReSize(N1);
  shocks_techprod.ReSize(N1);
  shocks_rd.ReSize(N1);
  shocks_labprod1.ReSize(N1);
  shocks_labprod2.ReSize(N2);
  shocks_eneff1.ReSize(N1);
  shocks_eneff2.ReSize(N2);
  shocks_output1.ReSize(N1);
  shocks_output2.ReSize(N2);
  S1_temp.ReSize(2, N1);
  S2_temp.ReSize(2, N2);

  if (N2 >= 200)
  {
    NB = 10;
  }
  else
  {
    NB = 1;
  }

  NbClient_1.ReSize(NB);
  NbClient_2.ReSize(NB);
  bonds_dem.ReSize(NB);
  DebtServiceToSales2_bank.ReSize(N2, NB);
  DS2_rating.ReSize(N2, NB);
  fB.ReSize(2, NB);
  NW_b.ReSize(2, NB);
  NW_b_c.ReSize(NB);
  Loans_b.ReSize(2, NB);
  BankMatch_1.ReSize(N1, NB);
  BankMatch_2.ReSize(N2, NB);
  BankCredit.ReSize(NB);
  Taxes_b.ReSize(NB);
  BaselBankCredit.ReSize(NB);
  InterestDeposits.ReSize(NB);
  Deposits.ReSize(2, NB);
  BankProfits.ReSize(NB);
  Dividends_b.ReSize(NB);
  Bailout_b.ReSize(NB);
  LossAbsorbed.ReSize(NB);
  BankEquity_temp.ReSize(NB);
  Bank_active.ReSize(NB);
  GB_b.ReSize(2, NB);
  bonds_purchased.ReSize(NB);
  BondRepayments_b.ReSize(NB);
  NL_1.ReSize(NB);
  NL_2.ReSize(NB);
  BankProfits_temp.ReSize(NB);
  r_deb.ReSize(NB);
  bankmarkup.ReSize(NB);
  Fragility.ReSize(NB);
  buffer.ReSize(NB);
  Bond_share.ReSize(NB);
  Deposits_hb.ReSize(2, NB);
  Deposits_eb.ReSize(2, NB);
  Advances_b.ReSize(2, NB);
  Reserves_b.ReSize(2, NB);
  InterestBonds_b.ReSize(NB);
  LoanInterest.ReSize(NB);
  InterestReserves_b.ReSize(NB);
  InterestAdvances_b.ReSize(NB);
  Outflows.ReSize(NB);
  Inflows.ReSize(NB);
  DepositShare_e.ReSize(NB);
  DepositShare_h.ReSize(NB);
  baddebt_b.ReSize(NB);
  capitalRecovered.ReSize(NB);
  capitalRecovered2.ReSize(NB);
  capitalRecoveredShare.ReSize(NB);
  LossEntry_b.ReSize(NB);
  ReserveBalance.ReSize(NB);
  ShareBonds.ReSize(NB);
  ShareReserves.ReSize(NB);
  ShareAdvances.ReSize(NB);
  Adjustment.ReSize(NB);
  prior.ReSize(NB);

  Con0.ReSize(ndep);
  fluxC.ReSize((ndep - 1));
  Con.ReSize(2, ndep);
  Hon.ReSize(2, ndep);
  Ton.ReSize(2, ndep);
  Cax.ReSize(niterclim);
  Caxx.ReSize(niterclim);
  Cay.ReSize(niterclim);
  Cayy.ReSize(niterclim);
  Caa.ReSize(niterclim);
  fluxH.ReSize((ndep - 1));
  Emiss_TOT.ReSize((freqclim * 2));

  age.resize(T);
  C_pb.resize(T);
  g_pb.resize(T);
  g_c.resize(T);
  g_c2.resize(T);
  g_c3.resize(T);
  g.resize(T);
  gtemp.resize(T);
  g_price.resize(T);
  g_secondhand.resize(T);
  g_secondhand_p.resize(T);
  age_secondhand.resize(T);
  for (std::size_t i = 0; i < T; i++)
  {
    age[i].resize(N1);
    C_pb[i].resize(N1);
    g_pb[i].resize(N1);
    g_c[i].resize(N1);
    g_c2[i].resize(N1);
    g_c3[i].resize(N1);
    g[i].resize(N1);
    gtemp[i].resize(N1);
    g_price[i].resize(N1);
    g_secondhand[i].resize(N1);
    g_secondhand_p[i].resize(N1);
    age_secondhand[i].resize(N1);
    for (std::size_t j = 0; j < N1; j++)
    {
      age[i][j].resize(N2);
      C_pb[i][j].resize(N2);
      g_pb[i][j].resize(N2);
      g_c[i][j].resize(N2);
      g_c2[i][j].resize(N2);
      g_c3[i][j].resize(N2);
      g[i][j].resize(N2);
      gtemp[i][j].resize(N2);
      g_price[i][j].resize(N2);
      g_secondhand[i][j] = 0;
      g_secondhand_p[i][j] = 0;
      age_secondhand[i][j] = 0;
      for (std::size_t ii = 0; ii < N2; ii++)
      {
        age[i][j][ii] = 0;
        g[i][j][ii] = 0;
        gtemp[i][j][ii] = 0;
        g_c[i][j][ii] = 0;
        g_c2[i][j][ii] = 0;
        g_c3[i][j][ii] = 0;
        g_price[i][j][ii] = 0;
      }
    }
  }

  if (NR > 0)
  {
    region_firm_assignment_K.assign(N1, 0);
    region_firm_assignment_C.assign(N2, 0);
    region_labor_supply.assign(NR, 0.0);
    region_unemployment.assign(NR, 0.0);
    region_dirty_capacity.assign(NR, 0.0);
    region_green_capacity.assign(NR, 0.0);
    region_dirty_capacity_lag.assign(NR, 0.0);
    region_green_capacity_lag.assign(NR, 0.0);
    // Regional accounting variables
    reg_GDP_r.assign(NR, 0.0);
    reg_Consumption_r.assign(NR, 0.0);
    reg_Investment_r.assign(NR, 0.0);
    reg_U.assign(NR, 0.0);
    reg_Am.assign(NR, 0.0);
    reg_Loans_2.assign(NR, 0.0);
    reg_Inventories.assign(NR, 0.0);
    reg_N.assign(NR, 0.0);
    reg_GDP_n.assign(NR, 0.0);
    reg_Qge.assign(NR, 0.0);
    reg_D_en_TOT.assign(NR, 0.0);
    reg_Emiss_TOT.assign(NR, 0.0);
    reg_Cum_emissions.assign(NR, 0.0);
    // Note: region_resultsexp_names and region_resultsexp_streams already
    // initialized and opened in main(), don't resize them here
  }
}

void INITIALIZE(int Exseed)
{
  // Set seed
  seed = Exseed;
  p_seed = &seed;
  tolerance = 1e-06;
  N1r = double(N1);
  N2r = double(N2);

  if (NR > 0)
  {
    auto assign_regions_by_share = [&](std::vector<int> &targets, const RowVector &shares, int total)
    {
      int filled = 0;
      for (int r = 1; r <= NR; ++r)
      {
        int quota = (r == NR) ? total - filled : int(round(shares(r) * total));
        for (int idx = 0; idx < quota && filled < total; ++idx)
        {
          targets[filled] = r;
          ++filled;
        }
      }
      while (filled < total)
      {
        targets[filled] = NR;
        ++filled;
      }
    };

    assign_regions_by_share(region_firm_assignment_K, region_K_shares, N1);
    assign_regions_by_share(region_firm_assignment_C, region_C_shares, N2);
    region_labor_supply.assign(NR, 0.0);
    region_unemployment.assign(NR, 0.0);
    region_dirty_capacity.assign(NR, 0.0);
    region_green_capacity.assign(NR, 0.0);
    region_dirty_capacity_lag.assign(NR, 0.0);
    region_green_capacity_lag.assign(NR, 0.0);
    // Regional accounting variables
    reg_GDP_r.assign(NR, 0.0);
    reg_Consumption_r.assign(NR, 0.0);
    reg_Investment_r.assign(NR, 0.0);
    reg_U.assign(NR, 0.0);
    reg_Am.assign(NR, 0.0);
    reg_Loans_2.assign(NR, 0.0);
    reg_Inventories.assign(NR, 0.0);
    reg_N.assign(NR, 0.0);
    reg_GDP_n.assign(NR, 0.0);
    reg_Qge.assign(NR, 0.0);
    reg_D_en_TOT.assign(NR, 0.0);
    reg_Emiss_TOT.assign(NR, 0.0);
    reg_Cum_emissions.assign(NR, 0.0);
  }

  INTFILE();
  // Determines number of firm customers of each bank
  ALLOCATEBANKCUSTOMERS();

  BankingSupplier_2 = 0;
  BankMatch_2 = 0;
  NbClient_2 = 0;
  BankingSupplier_1 = 0;
  BankMatch_1 = 0;
  NbClient_1 = 0;
  NB_long = NB;
  // Initial deposits for firms
  Deposits_1 = W10;
  Deposits_2 = W20;
  Loans_2 = L0;
  Deposits = 0;
  Loans_b = 0;

  for (j = 1; j <= N2; j++)
  {
    // Match C-firms to banks and give those banks the initial deposits
    while (BankingSupplier_2(j) == 0)
    {
      rni = int(ran1(p_seed) * N1 * N2) % NB_long + 1;
      if (NbClient_2(rni) < NL_2(rni))
      {
        BankMatch_2(j, rni) = 1;
        BankingSupplier_2(j) = rni;
        NbClient_2(rni)++;
        Deposits(1, rni) += Deposits_2(1, j);
        Deposits(2, rni) += Deposits_2(2, j);
        Loans_b(1, rni) += Loans_2(1, j);
        Loans_b(2, rni) += Loans_2(2, j);
      }
      else
      {
        BankMatch_2(j, rni) = 0;
      }
    }
  }

  for (i = 1; i <= N1; i++)
  {

    // Match K-firms to banks and give those banks the initial deposits
    while (BankingSupplier_1(i) == 0)
    {
      rni = int(ran1(p_seed) * N1 * N2) % NB_long + 1;
      if (NbClient_1(rni) < NL_1(rni))
      {
        BankMatch_1(i, rni) = 1;
        BankingSupplier_1(i) = rni;
        NbClient_1(rni)++;
        Deposits(1, rni) += Deposits_1(1, i);
        Deposits(2, rni) += Deposits_1(2, i);
      }
      else
      {
        BankMatch_1(i, rni) = 0;
      }
    }
  }

  // Set initial prices
  p2 = (1 + mi2) * (w0 / A0 + mi_en0 / A0_en);
  p1 = (1 + mi1) * (w0 / (A0 * pm * a) + mi_en0 / A0_en);
  Am(2) = (A0 * N2 + A0 * a * pm * N1) / (N1 + N2);
  // Households & Energy sector deposits should be positive at begining
  Deposits_h = D_h0;
  Deposits_e = D_e0;
  // Distribute remaining stocks between banks
  for (i = 1; i <= NB; i++)
  {
    fB(1, i) = (NbClient_2(i) + NbClient_1(i)) / (N2 + N1);
    fB(2, i) = (NbClient_2(i) + NbClient_1(i)) / (N2 + N1);
    GB_b(1, i) = varphi * Loans_b(1, i);
    GB_b(2, i) = varphi * Loans_b(2, i);
    Deposits(1, i) += fB(1, i) * (Deposits_h(1) + Deposits_e(1));
    Deposits(2, i) += fB(2, i) * (Deposits_h(2) + Deposits_e(2));
    Deposits_hb(1, i) += fB(1, i) * (Deposits_h(1));
    Deposits_hb(2, i) += fB(2, i) * (Deposits_h(2));
    Deposits_eb(1, i) += fB(1, i) * (Deposits_e(1));
    Deposits_eb(2, i) += fB(2, i) * (Deposits_e(2));
    DepositShare_e(i) = Deposits_eb(1, i) / (Deposits_e(1));
    DepositShare_h(i) = Deposits_hb(1, i) / (Deposits_h(1));
    Reserves_b(1, i) = Deposits(1, i) + NW_b0 * fB(1, i) - GB_b(1, i) - Loans_b(1, i);
    Reserves_b(2, i) = Deposits(2, i) + NW_b0 * fB(2, i) - GB_b(2, i) - Loans_b(2, i);
    NW_b(1, i) = Loans_b(1, i) + Reserves_b(1, i) + GB_b(1, i) - Deposits(1, i) - Advances_b(1, i);
    NW_b(2, i) = Loans_b(2, i) + Reserves_b(2, i) + GB_b(2, i) - Deposits(2, i) - Advances_b(2, i);
  }
  // GB_b=0;
  GB_cb(1) = NW_b0 + W10 * N1 + W20 * N2 + D_h0 + Deposits_e(1) - GB_b.Row(1).Sum() - Loans_b.Row(1).Sum();
  GB_cb(2) = NW_b0 + W10 * N1 + W20 * N2 + D_h0 + Deposits_e(2) - GB_b.Row(2).Sum() - Loans_b.Row(2).Sum();
  GB(1) = GB_cb(1) + GB_b.Row(1).Sum();
  GB(2) = GB_cb(2) + GB_b.Row(2).Sum();
  DebtService_2 = 0;
  Advances_b = 0;
  Advances = 0;
  // CB Stock of gov. bonds implies an equal stock of reserves
  Reserves = GB_cb;
  // Set nominal value of capital stock
  CapitalStock = K0 / dim_mach * (1 + mi1) * (w0 / (A0 * pm * a) + mi_en0 / A0_en);
  deltaCapitalStock = 0;
  CapitalStock_e = 0;

  NW_h = Deposits_h;
  NW_1 = Deposits_1;
  NW_gov = -GB;
  NW_cb = GB_cb - Reserves;
  NW_e = Deposits_e;
  r = pow((1 + r_base), 0.25) - 1;
  if (d_cpi_target < 0.02)
  {
    d_cpi_target_a = pow((1 + d_cpi_target), 4) - 1;
  }
  else
  {
    d_cpi_target_a = d_cpi_target;
  }
  r_depo = r * (1 - bankmarkdown);
  r_cbreserves = r * (1 - centralbankmarkdown);
  r_bonds = r * (1 - bondsmarkdown);
  ProfitCB(2) = 0;
  w = w0;
  A1 = A0;
  A1p = A0 * pm;
  A2 = A0;
  LDentot = 0;
  Wages_en = 0;
  d_Am = d_Am_init;
  dw2 = 1;
  d_cpi2 = 1;
  pass_1 = 1;
  pass_2 = 1;
  exp_quota_0 = exp_quota;
  uu1_en_g = 0;
  uu2_en_g = 0;

  // Initialise energy productivities etc. and climate box
  if (flag_tax_CO2 > 0)
  {
    t_CO2_en = t_CO2_en_0;
  }

  A1_en = A0_en;
  A1p_en = A0_en;
  A2_en = A0_en;
  A_en = A0_en;

  A1_ef = A0_ef;
  A1p_ef = A0_ef;
  A2_ef = A0_ef;
  A_ef = A0_ef;

  A_de = A_de0;
  EM_de = EM0;
  pf = pf0;
  mi_en = mi_en0;
  G_de = 0;
  G_ge = 0;
  CF_ge(1) = CF_ge0;
  c_en(2) = mi_en0;

  if (flag_nonCO2_force == 0)
  {
    Cat(1) = Catinit0;
    biom(1) = biominit0;
    hum(1) = huminit0;
    Tmixed(1) = Tmixedinit0;

    for (j = 1; j <= ndep; j++)
    {
      Con(1, j) = Coninit0(j);
      Hon(1, j) = Honinit0(j);
      Ton(1, j) = Toninit0(j);
    }
  }
  else
  {
    Cat(1) = Catinit1;
    biom(1) = biominit1;
    hum(1) = huminit1;
    Tmixed(1) = Tmixedinit1;

    for (j = 1; j <= ndep; j++)
    {
      Con(1, j) = Coninit1(j);
      Hon(1, j) = Honinit1(j);
      Ton(1, j) = Toninit1(j);
    }
  }

  if (flag_cum_emissions == 1)
  {
    Tmixed(1) = T_0_cumemiss;
  }

  Cum_emissions = Cum_emissions_0;
  Emiss_yearly_calib = Emiss_yearly_0;
  g_rate_em_y = 0;
  Emiss_TOT = 0;
  Emiss_yearly = 0;

  cpi(2) = (1 + mi2) * (w0 / A0 + mi_en0 / A0_en);
  cpi(3) = (1 + mi2) * (w0 / A0 + mi_en0 / A0_en);
  cpi(4) = (1 + mi2) * (w0 / A0 + mi_en0 / A0_en);
  cpi(5) = (1 + mi2) * (w0 / A0 + mi_en0 / A0_en);

  A = A0;
  C = w0 / A0 + mi_en0 / A0_en;
  c2 = w0 / A0 + mi_en0 / A0_en;
  c1 = w0 / (A0 * pm * a) + mi_en0 / A0_en;
  Em2 = 1;
  l2 = 1;
  // Initial capital stock in terms of productive capacity
  K = K0;
  LS = LS0;
  E2 = 1;
  f1 = 1 / N1r;
  f2 = 1 / N2r;
  t0 = 1;
  Fragility = 0;

  for (i = 1; i <= NB; i++)
  {
    Bank_active(i) = 1;
    bankmarkup(i) = bankmarkup_init;
    r_deb(i) = r * (1 + bankmarkup(i));
  }

  EI = 0;
  I = ROUND((((K0) / (agemax + 1))) / dim_mach) * dim_mach;
  mu2 = mi2;
  Td.element(0) = 0;
  step = N2 / N1;
  cont = 0;
  Match = 0;

  // Match C-firms to K-firms & distribute initial stocks of machines
  for (i = 1; i <= N1; i++)
  {
    cont += step;
    for (j = 0; j < step; j++)
    {
      Match(cont - j, i) = 1;
      fornit(cont - j) = i;
    }
  }

  for (j = 1; j <= N2; j++)
  {
    n_mach(j) = K(j) / dim_mach;
    while (n_mach(j) > 0)
    {
      i++;
      if (i > N1)
      {
        i = 1;
      }
      if (fornit(j) != i)
      {
        age0 = int(ran1(p_seed) * (agemax + 1)) % int((agemax + 1)) + 1;
        g[0][i - 1][j - 1]++;
        g_price[0][i - 1][j - 1] = p1(i);
        gtemp[0][i - 1][j - 1]++;
        g_c[0][i - 1][j - 1]++;
        g_c2[0][i - 1][j - 1]++;
        g_c3[0][i - 1][j - 1]++;
        age[0][i - 1][j - 1] = age0;
        n_mach(j)--;
      }
    }
  }

  // Set initial demand for consumption goods
  D20 = w0 * (LS0 * (1 - ustar) - I.Sum() / dim_mach / (A0 * pm * a));
  D2 = D20 / N2r;
  De = D2.Row(1);
  N = omicron * D20 / N2r;
  // Nominal value of C-firm inventories
  Inventories = (omicron * D20 / N2r) * p2(1);
  NW_2 = Deposits_2 + CapitalStock + Inventories - Loans_2;
  S1 = (((I(1) / dim_mach) * N2r) / N1r) * p1;
  S2 = D20 / N2r * p2(1);
  mol = D20 / N2r * p2(1) - D20 / N2r / A0 * w0 - D20 / N2r / A0_en * mi_en0;
  Dividends(2) = mol(1) * N2r * d2 + S1.Sum() * d1 + r_bonds * GB_b.Row(1).Sum() + de * mi_en0 * De.Sum() / A0_en;
  U(2) = (LS0 - ((D20) / A0 + (((I.element(1) / dim_mach) * N2r)) / (A0 * pm * a))) / LS0;
  // Initialise technological change
  A1top = A0;
  A1ptop = A0 * pm;
  A1f = A0;
  A1pf = A0 * pm;
  A1_en_top = A0_en;
  A1p_en_top = A0_en;
  A1_ef_top = A0_ef;
  A1p_ef_top = A0_ef;

  RD.Row(1) = nu * S1;
  RD.Row(2) = nu * S1;
  t = 0;
  TECHANGEND();
}

void SETVARS(void)
{
  for (j = 1; j <= N2; ++j)
  {
    Loans_2(1, j) = Loans_2(2, j);
    Deposits_2(1, j) = Deposits_2(2, j);
    deltaCapitalStock(1, j) = 0;
    InterestDeposits_2(j) = 0;
    CapitalStock(1, j) = CapitalStock(2, j);
    c2(j) = 0;
    c2p(j) = 0;
    A2(j) = 0;
    A2e(j) = 0;
    A2e_en(j) = 0;
    A2e_ef(j) = 0;
    A2e2(j) = 0;
    A2e_en2(j) = 0;
    A2e_ef2(j) = 0;
    c2e(j) = 0;
    EI(1, j) = 0;
    SI(j) = 0;
    I(j) = 0;
    Dividends_2(j) = 0;
    Taxes_2(j) = 0;
    Injection_2(j) = 0;
    DebtRemittances2(j) = 0;
    A2_en(j) = 0;
    A2_ef(j) = 0;
    baddebt_2(j) = 0;
    S2(1, j) = 0;
    S2_temp(1, j) = 0;
    D2(1, j) = 0;
    Q2(j) = 0;
    k(j) = 0;
    EId(j) = 0;
    SId(j) = 0;
    SId_age(j) = 0;
    SId_cost(j) = 0;
    exiting_2(j) = 0;
    exit_payments2(j) = 0;
    exit_equity2(j) = 0;
    exit_marketshare2(j) = 0;
    n_mach_entry(j) = 0;
    Loss_Capital(j) = 0;
    Loss_Inventories(j) = 0;
    k_entry(j) = 0;
    marker_age(j) = 0;
    K_loss(j) = 0;
  }

  for (i = 1; i <= N1; i++)
  {
    Deposits_1(1, i) = Deposits_1(2, i);
    InterestDeposits_1(i) = 0;
    Q1(i) = 0;
    D1(i) = 0;
    S1(i) = 0;
    S1_temp(1, i) = 0;
    Dividends_1(i) = 0;
    Taxes_1(i) = 0;
    Injection_1(i) = 0;
    baddebt_1(i) = 0;
    exiting_1(i) = 0;
  }

  for (i = 1; i <= NB; i++)
  {
    Loans_b(1, i) = Loans_b(2, i);
    Deposits(1, i) = Deposits(2, i);
    Deposits_hb(1, i) = Deposits_hb(2, i);
    Deposits_eb(1, i) = Deposits_eb(2, i);
    GB_b(1, i) = GB_b(2, i);
    Advances_b(1, i) = Advances_b(2, i);
    Reserves_b(1, i) = Reserves_b(2, i);
    InterestDeposits(i) = 0;
    baddebt_b(i) = 0;
    Outflows(i) = 0;
    Inflows(i) = 0;
    NW_b(1, i) = NW_b(2, i);
    Dividends_b(i) = 0;
    Taxes_b(i) = 0;
    LoanInterest(i) = 0;
    bonds_dem(i) = 0;
    Bailout_b(i) = 0;
    LossAbsorbed(i) = 0;
    capitalRecovered(i) = 0;
    capitalRecovered2(i) = 0;
    capitalRecoveredShare(i) = 0;
    LossEntry_b(i) = 0;
  }

  Taxes = 0;
  Taxes_CO2 = 0;
  Subsidy_Exp = 0;
  subsidy_ge_exp = 0;
  Taxes_e_shock = 0;
  Taxes_f_shock = 0;
  Wages = 0;
  Deposits_recovered_1 = 0;
  Deposits_recovered_2 = 0;
  EntryCosts = 0;
  BankTransfer = 0;
  Deposits_h(1) = Deposits_h(2);
  Deposits_e(1) = Deposits_e(2);
  GB_cb(1) = GB_cb(2);
  GB(1) = GB(2);
  Advances(1) = Advances(2);
  Reserves(1) = Reserves(2);
  Dividends(1) = 0;
  EnergyPayments = 0;
  InterestReserves = 0;
  InterestAdvances = 0;
  FirmTransfers = 0;
  FirmTransfers_1 = 0;
  FirmTransfers_2 = 0;
  Divtot_1 = 0;
  Divtot_2 = 0;
  Pitot1 = 0;
  Pitot2 = 0;
  LD2tot = 0;
  LD1tot = 0;
  dNtot = 0;
  dNmtot = 0;
  DebtServiceToSales2_bank = 0;
  Bailout = 0;
  NewBonds = 0;
  cpi(1) = 0;
  kpi = 0;
  Em2(1) = 0;
  ns1 = 0;
  ns2 = 0;
  mD1 = 0;
  mD2 = 0;
  n_mach_exit = 0;
  n_mach_needed = 0;
  Emiss_en = 0;
  Am(1) = 0;
  Am2 = 0;
  Am1 = 0;
  Am_a = 0;
  Am_en(1) = 0;
  ftot = 0;
  Consumption_r = 0;
  A_mi = 0;
  A1_mi = 0;
  A2_en_mi = 0;
  A2_ef_mi = 0;
  A1_en_mi = 0;
  A1_ef_mi = 0;
  A_sd = 0;
  H1 = 0;
  H2 = 0;
  HB = 0;
  PC_en = 0;
  IC_en = 0;
  p2_entry = 0;
  f2_exit = 0;
  EntryShare = 0;
  CompEntry = 0;
  C_secondhand = 1000000;
  counter_bankfailure = 0;
  FuelCost = 0;
  A1p_en_dead = 0;
  A1p_en_survive = 0;
  A2_en_dead = 0;
  A2_en_survive = 0;
}

void DEPOSITINTEREST(void)
{
  // Firms, Households and the Energy sector receive deposit interest
  for (i = 1; i <= N1; i++)
  {
    sendingBank = BankingSupplier_1(i);
    InterestDeposits_1(i) = r_depo * Deposits_1(2, i);
    Deposits_1(1, i) += InterestDeposits_1(i);
    Deposits(1, sendingBank) += InterestDeposits_1(i);
    InterestDeposits(sendingBank) += InterestDeposits_1(i);
  }

  for (j = 1; j <= N2; j++)
  {
    sendingBank = BankingSupplier_2(j);
    InterestDeposits_2(j) = r_depo * Deposits_2(2, j);
    InterestDeposits(sendingBank) += InterestDeposits_2(j);
    Deposits_2(1, j) += InterestDeposits_2(j);
    Deposits(1, sendingBank) += InterestDeposits_2(j);
  }

  InterestDeposits_h = r_depo * Deposits_h(2);
  Deposits_h(1) += InterestDeposits_h;
  InterestDeposits_e = r_depo * Deposits_e(2);
  Deposits_e(1) += InterestDeposits_e;

  for (i = 1; i <= NB; i++)
  {
    InterestDeposits(i) += r_depo * Deposits_hb(2, i);
    Deposits_hb(1, i) += r_depo * Deposits_hb(2, i);
    Deposits(1, i) += r_depo * Deposits_hb(2, i);
    InterestDeposits(i) += r_depo * Deposits_eb(2, i);
    Deposits_eb(1, i) += r_depo * Deposits_eb(2, i);
    Deposits(1, i) += r_depo * Deposits_eb(2, i);
  }

  for (i = 1; i <= NB; i++)
  {
    if (Deposits_hb.Row(1).Sum() > 0)
    {
      DepositShare_h(i) = Deposits_hb(1, i) / Deposits_hb.Row(1).Sum();
    }
    else
    {
      DepositShare_h(i) = (NL_1(i) + NL_2(i)) / (N1 + N2);
    }

    if (Deposits_eb.Row(1).Sum() > 0)
    {
      DepositShare_e(i) = Deposits_eb(1, i) / Deposits_eb.Row(1).Sum();
    }
    else
    {
      DepositShare_e(i) = (NL_1(i) + NL_2(i)) / (N1 + N2);
    }
  }
}

void MACH(void)
{
  ofstream Errors(errorfilename, ios::app);
  if ((flag_energyshocks == 1 && t == 246) || (flag_energyshocks == 3 && t == 246) || (flag_energyshocks == 5 && t == 246))
  {
    pass_1 = 0;
    pass_2 = 0;
  }

  if ((flag_energyshocks == 1 && t > 245) || (flag_energyshocks == 3 && t > 245) || (flag_energyshocks == 5 && t > 245))
  {
    for (i = 1; i <= N1; i++)
    {
      if (pass_1(i) == 0)
      {
        rnd = ran1(p_seed);
        if (rnd <= passthrough)
        {
          pass_1(i) = 1;
        }
      }
    }
    for (j = 1; j <= N2; j++)
    {
      if (pass_2(j) == 0)
      {
        rnd = ran1(p_seed);
        if (rnd <= passthrough)
        {
          pass_2(j) = 1;
        }
      }
    }
  }

  // Determine production cost and selling price for K-firms
  for (i = 1; i <= N1; i++)
  {
    for (tt = t0; tt <= t; tt++)
    {
      if (A(tt, i) > 0 & A_en(tt, i) > 0)
      {
        C(tt, i) = w(2) / A(tt, i) + c_en(2) / A_en(tt, i) + t_CO2 * A_ef(tt, i) / A_en(tt, i);
      }
      else
      {
        std::cerr << "\n\n ERROR: A_en(tt,i) = 0 in period " << t << " for K-firm " << i << endl;
        Errors << "\n A_en(tt,i) = 0 in period " << t << " for K-firm " << i << endl;
        exit(EXIT_FAILURE);
      }
    }

    c1(i) = w(2) / ((1 - shocks_labprod1(i)) * A1p(i) * a) + c_en(2) / ((1 - shocks_eneff1(i)) * A1p_en(i)) + t_CO2 * A1p_ef(i) / ((1 - shocks_eneff1(i)) * A1p_en(i));
    if (pass_1(i) == 1)
    {
      c1p(i) = c1(i);
    }
    else
    {
      c1p(i) = w(2) / ((1 - shocks_labprod1(i)) * A1p(i) * a) + c_en_preshock / ((1 - shocks_eneff1(i)) * A1p_en(i)) + t_CO2 * A1p_ef(i) / ((1 - shocks_eneff1(i)) * A1p_en(i));
    }

    rnd = ran1(p_seed);
    if (rnd <= theta)
    {
      p1(i) = (1 + mi1) * c1p(i);
    }

    if (p1(i) < pmin)
    {
      p1(i) = pmin;
    }
  }

  // C-firms receive capital ordered in the last period
  for (j = 1; j <= N2; j++)
  {
    K(j) += EI(2, j) - scrap_age(j);
    CapitalStock(1, j) += deltaCapitalStock(2, j);
    for (i = 1; i <= N1; i++)
    {
      for (tt = t0; tt <= t; tt++)
      {
        g[tt - 1][i - 1][j - 1] = gtemp[tt - 1][i - 1][j - 1];
        g_c[tt - 1][i - 1][j - 1] = g[tt - 1][i - 1][j - 1];
        g_c2[tt - 1][i - 1][j - 1] = g[tt - 1][i - 1][j - 1];
        g_c3[tt - 1][i - 1][j - 1] = g[tt - 1][i - 1][j - 1];
      }
    }
    scrap_age(j) = 0;
  }

  // C-firms determine cost of production, revise mark-up and set their price
  for (j = 1; j <= N2; j++)
  {
    n_mach(j) = K(j) / dim_mach;
    for (i = 1; i <= N1; i++)
    {
      for (tt = t0; tt <= t; tt++)
      {
        if (n_mach(j) > 0)
        {
          c2(j) += (w(2) / ((1 - shocks_labprod2(j)) * A(tt, i)) + c_en(2) / ((1 - shocks_eneff2(j)) * A_en(tt, i)) + t_CO2 * A_ef(tt, i) / ((1 - shocks_eneff2(j)) * A_en(tt, i))) * g[tt - 1][i - 1][j - 1] / n_mach(j);
          if (pass_2(j) == 1)
          {
            c2p(j) += (w(2) / ((1 - shocks_labprod2(j)) * A(tt, i)) + c_en(2) / ((1 - shocks_eneff2(j)) * A_en(tt, i)) + t_CO2 * A_ef(tt, i) / ((1 - shocks_eneff2(j)) * A_en(tt, i))) * g[tt - 1][i - 1][j - 1] / n_mach(j);
          }
          else
          {
            c2p(j) += (w(2) / ((1 - shocks_labprod2(j)) * A(tt, i)) + c_en_preshock / ((1 - shocks_eneff2(j)) * A_en(tt, i)) + t_CO2 * A_ef(tt, i) / ((1 - shocks_eneff2(j)) * A_en(tt, i))) * g[tt - 1][i - 1][j - 1] / n_mach(j);
          }
          A2(j) += (1 - shocks_labprod2(j)) * A(tt, i) * g[tt - 1][i - 1][j - 1] / n_mach(j);
          A2_en(j) += (1 - shocks_eneff2(j)) * A_en(tt, i) * g[tt - 1][i - 1][j - 1] / n_mach(j);
          A2_ef(j) += A_ef(tt, i) * g[tt - 1][i - 1][j - 1] / n_mach(j);
        }
        else
        {
          std::cerr << "\n\n ERROR n_mach = 0 in period " << t << " for C-firm " << j << endl;
          Errors << "\n n_mach = 0 in period " << t << " for C-firm " << j << endl;
          exit(EXIT_FAILURE);
        }
      }
    }

    if (f2(3, j) > 0)
    {
      mu2(1, j) = max(0.0, mu2(2, j) * (1 + deltami2 * ((f2(2, j) - f2(3, j)) / f2(3, j))));
    }
    else
    {
      mu2(1, j) = mu2(2, j);
    }

    if (mu2(1, j) <= 0)
    {
      std::cerr << "\n\n ERROR: mark-up out of range in period " << t << " for C-firm " << j << endl;
      Errors << "\n Mark-up out of range in period " << t << " for C-firm " << j << endl;
      exit(EXIT_FAILURE);
    }

    rnd = ran1(p_seed);
    if (rnd <= theta)
    {
      p2(j) = (1 + mu2(1, j)) * c2p(j);
    }

    if (p2(j) < pmin)
    {
      p2(j) = pmin;
    }
  }

  Errors.close();
}

void BROCHURE(void)
{
  ofstream Errors(errorfilename, ios::app);

  ftot = 0;
  nclient = 0;

  for (j = 1; j <= N2; j++)
  {
    if (fornit(j) < 1 || fornit(j) > N1)
    {
      fornit(j) = int(ran1(p_seed) * N1 * N2) % N1 + 1;
      Match(j, fornit(j)) = 1;
    }
  }

  // K-firms send brochures to potential customers
  for (i = 1; i <= N1; i++)
  {
    for (j = 1; j <= N2; j++)
    {
      nclient(i) += Match(j, i);
    }

    newbroch = int(ROUND(nclient(i) * Gamma));

    if (newbroch == 0)
    {
      newbroch++;
    }

    while (newbroch > 0)
    {
      rni = int(ran1(p_seed) * N1 * N2) % N2 + 1;
      Match(rni, i) = 1;
      newbroch--;
    }
  }

  // C-firms choose their preferred supplier of machine tools

  for (j = 1; j <= N2; j++)
  {
    indforn = int(fornit(j));
    for (i = 1; i <= N1; i++)
    {
      if (A1(i) > 0)
      {
        if (Match(j, i) == 1 && p1(i) + (w(2) / A1(i) + c_en(2) / A1_en(i) + t_CO2 * A1_ef(i) / A1_en(i)) * b < p1(indforn) + (w(2) / A1(indforn) + c_en(2) / A1_en(indforn) + t_CO2 * A1_ef(indforn) / A1_en(indforn)) * b)
        {
          indforn = i;
        }
      }
      else
      {
        std::cerr << "\n\n ERROR: A1(i) = 0 in period " << t << " for K-firm " << i << endl;
        Errors << "\n A1(i) = 0 in period " << t << " for K-firm " << i << endl;
        exit(EXIT_FAILURE);
      }
    }
    fornit(j) = indforn;

    for (i = 1; i <= N1; i++)
    {
      if (i != indforn)
      {
        Match(j, i) = 0;
      }
    }
  }

  nclient = 0;
  for (i = 1; i <= N1; i++)
  {
    for (j = 1; j <= N2; j++)
    {
      if (Match(j, i) == 1)
      {
        nclient(i)++;
      }
    }
  }

  Errors.close();
}

void INVEST(void)
{
  flag = 0;

  for (j = 1; j <= N2; j++)
  {
    // C-firms determine expected demand, desired production, and demand for investment
    De(j) = alfa * De(j) + (1 - alfa) * D2(2, j);
    if (De(j) <= 0)
    {
      De(j) = 1;
    }

    Ne(j) = De(j) * omicron;
    Qd(j) = De(j) + Ne(j) - N(2, j);

    if (Qd(j) < 0)
    {
      Qd(j) = 0;
    }

    Kd(j) = Qd(j) / u;

    // C-firms determine which machines should be scrapped
    SCRAPPING();

    Ktrig(j) = ROUND((K(j) - scrap_age(j)) / dim_mach) * dim_mach;

    if (Kd(j) >= Ktrig(j))
    {
      if (I_max > 0)
      {
        K_top = Ktrig(j) * (1 + I_max);
        K_top = ROUND(K_top / dim_mach) * dim_mach;
        if (K_top < (K(j) + dim_mach))
        {
          K_top += dim_mach;
        }
      }
      else
      {
        K_top = Kd(j) + 1;
      }

      if (Kd(j) > K_top)
      {
        EId(j) = K_top - Ktrig(j);
      }
      else
      {
        EId(j) = floor((Kd(j) - (K(j) - scrap_age(j))) / dim_mach) * dim_mach;
      }
    }
    else
    {
      EId(j) = 0;
    }

    if (SId(j) == 0 && EId(j) == 0 && marker_age(j) == 1)
    {
      EId(j) = dim_mach;
    }

    if (Qd(j) > K(j))
    {
      Qd(j) = K(j);
    }

    // C-firms determine effective production cost
    if (Qd(j) > 0 && Qd(j) < K(j))
    {
      COSTPROD();
    }
    else
    {
      A2e(j) = A2(j);
      c2e(j) = c2(j);
      A2e_en(j) = A2_en(j);
      A2e_ef(j) = A2_ef(j);
    }
  }

  ORD();
}

void SCRAPPING(void)
{
  ofstream Errors(errorfilename, ios::app);
  K_temp(j) = K(j) / dim_mach;
  indforn = int(fornit(j));
  // C-firms determine which machines should be scrapped due to age and/or due to superior tech being available
  for (i = 1; i <= N1; i++)
  {
    for (tt = t0; tt <= t; tt++)
    {
      C_pb[tt - 1][i - 1][j - 1] = 0;
      g_pb[tt - 1][i - 1][j - 1] = 0;

      if (g[tt - 1][i - 1][j - 1] > 0 && age[tt - 1][i - 1][j - 1] > (agemax))
      {
        if (flag_scrap_age == 1)
        {
          g_pb[tt - 1][i - 1][j - 1] = min(g[tt - 1][i - 1][j - 1], (K_temp(j) - 1));
          C_pb[tt - 1][i - 1][j - 1] = C(tt, i);
          scrap_age(j) += dim_mach * g_pb[tt - 1][i - 1][j - 1];
        }
        else
        {
          g_pb[tt - 1][i - 1][j - 1] = min(g[tt - 1][i - 1][j - 1], (K_temp(j) - 1));
          C_pb[tt - 1][i - 1][j - 1] = C(tt, i);
          SId(j) += dim_mach * g_pb[tt - 1][i - 1][j - 1];
          SId_age(j) += dim_mach * g_pb[tt - 1][i - 1][j - 1];
          if (SId(j) == 0 && K_temp(j) == 1)
          {
            marker_age(j) = 1;
          }
        }
        K_temp(j) -= g_pb[tt - 1][i - 1][j - 1];
      }

      if (g[tt - 1][i - 1][j - 1] > 0 && g_pb[tt - 1][i - 1][j - 1] == 0 && A(tt, i) < A1(indforn))
      {
        if (w(2) > 0 && A(tt, i) > 0 && A1(indforn) > 0 && A1_en(indforn) > 0 && A_en(tt, i) > 0)
        {
          payback = p1(indforn) / (w(2) / A(tt, i) + c_en(2) / A_en(tt, i) + t_CO2 * A_ef(tt, i) / A_en(tt, i) - w(2) / A1(indforn) - c_en(2) / A1_en(indforn) - t_CO2 * A1_ef(indforn) / A1_en(indforn));
        }
        else
        {
          std::cerr << "\n\n ERROR: payback division by zero in period " << t << " for C-firm " << j << endl;
          Errors << "\n Payback division by zero in period " << t << " for C-firm " << j << endl;
          exit(EXIT_FAILURE);
        }

        if (payback <= b && payback > 0)
        {
          g_pb[tt - 1][i - 1][j - 1] = g[tt - 1][i - 1][j - 1];
          C_pb[tt - 1][i - 1][j - 1] = C(tt, i);
          SId(j) += dim_mach * g_pb[tt - 1][i - 1][j - 1];
          SId_cost(j) += dim_mach * g_pb[tt - 1][i - 1][j - 1];
        }
      }
    }
  }

  Errors.close();
}

void COSTPROD(void)
{
  ofstream Errors(errorfilename, ios::app);

  // C-firms determine effective production cost based on desired production; most efficient machines used first
  nmachprod = ceil(Qd(j) / dim_mach);
  nmp_temp = nmachprod;

  while (nmp_temp > 0)
  {
    cmin = 10000000000000000;
    imin = 0;
    jmin = 0;
    tmin = 0;

    for (i = 1; i <= N1; i++)
    {
      for (tt = t0; tt <= t; tt++)
      {
        if (g_c[tt - 1][i - 1][j - 1] > 0 && (w(2) / ((1 - shocks_labprod2(j)) * A(tt, i)) + c_en(2) / A_en(tt, i) + t_CO2 * A_ef(tt, i) / A_en(tt, i)) < cmin)
        {
          cmin = w(2) / ((1 - shocks_labprod2(j)) * A(tt, i)) + c_en(2) / A_en(tt, i) + t_CO2 * A_ef(tt, i) / A_en(tt, i);
          imin = i;
          jmin = j;
          tmin = tt;
        }
      }
    }

    if (nmachprod > 0)
    {
      if (g_c[tmin - 1][imin - 1][jmin - 1] >= nmp_temp)
      {
        A2e(j) += (1 - shocks_labprod2(j)) * A(tmin, imin) * nmp_temp / nmachprod;
        A2e_en(j) += (1 - shocks_eneff2(j)) * A_en(tmin, imin) * nmp_temp / nmachprod;
        A2e_ef(j) += A_ef(tmin, imin) * nmp_temp / nmachprod;
        c2e(j) += (w(2) / ((1 - shocks_labprod2(j)) * A(tmin, imin)) + c_en(2) / ((1 - shocks_eneff2(j)) * A_en(tmin, imin)) + t_CO2 * A_ef(tmin, imin) / ((1 - shocks_eneff2(j)) * A_en(tmin, imin))) * nmp_temp / nmachprod;
        g_c[tmin - 1][imin - 1][jmin - 1] -= nmp_temp;
        nmp_temp = 0;
      }
      else
      {
        A2e(j) += (1 - shocks_labprod2(j)) * A(tmin, imin) * g_c[tmin - 1][imin - 1][jmin - 1] / nmachprod;
        A2e_en(j) += (1 - shocks_eneff2(j)) * A_en(tmin, imin) * g_c[tmin - 1][imin - 1][jmin - 1] / nmachprod;
        A2e_ef(j) += A_ef(tmin, imin) * g_c[tmin - 1][imin - 1][jmin - 1] / nmachprod;
        c2e(j) += (w(2) / ((1 - shocks_labprod2(j)) * A(tmin, imin)) + c_en(2) / ((1 - shocks_eneff2(j)) * A_en(tmin, imin)) + t_CO2 * A_ef(tmin, imin) / ((1 - shocks_eneff2(j)) * A_en(tmin, imin))) * g_c[tmin - 1][imin - 1][jmin - 1] / nmachprod;
        nmp_temp -= g_c[tmin - 1][imin - 1][jmin - 1];
        g_c[tmin - 1][imin - 1][jmin - 1] = 0;
      }
    }
    else
    {
      std::cerr << "\n\n ERROR: nmachprod = 0!!!" << endl;
      Errors << "\n nmachprod = 0 in period " << t << endl;
      exit(EXIT_FAILURE);
    }
  }

  Errors.close();
}

void ORD(void)
{

  // C-firms calculate internal funds & the maximum amount of loans they are willing to take on; based on this, investment is possibly scaled back
  for (j = 1; j <= N2; ++j)
  {
    InternalFunds = max(0.0, Deposits_2(1, j) - Loans_2(1, j));

    if (Qd(j) > 0)
    {
      prestmax = phi2 * mol(j) - Loans_2(1, j);
    }
    else
    {
      prestmax = 0;
    }

    if (prestmax < 0)
    {
      prestmax = 0;
    }

    InternalFunds = max(0.0, InternalFunds - c2e(j) * Qd(j));

    indforn = int(fornit(j));
    p1prova = p1(indforn);
    if ((SId_age(j) / dim_mach) * p1prova < InternalFunds)
    {
      SIp_age(j) = SId_age(j);
      InternalFunds -= (SId_age(j) / dim_mach) * p1(indforn);
    }
    else
    {
      if ((SId_age(j) / dim_mach) * p1prova - InternalFunds <= prestmax)
      {
        SIp_age(j) = SId_age(j);
        prestmax -= (SId_age(j) / dim_mach) * p1(indforn) - InternalFunds;
        InternalFunds = 0;
      }
      else
      {
        SIp_age(j) = floor((InternalFunds + prestmax) / p1(indforn)) * dim_mach;
        if (SIp_age(j) < 0)
        {
          SIp_age(j) = 0;
          InternalFunds = 0;
          prestmax = 0;
        }
        else
        {
          InternalFunds = 0;
          prestmax = 0;
        }
      }
    }

    if ((EId(j) / dim_mach) * p1prova < InternalFunds)
    {
      EIp(j) = EId(j);
      InternalFunds -= (EId(j) / dim_mach) * p1(indforn);
    }
    else
    {
      p1prova = p1(indforn);
      if ((EId(j) / dim_mach) * p1prova - InternalFunds <= prestmax)
      {
        EIp(j) = EId(j);
        prestmax -= (EId(j) / dim_mach) * p1(indforn) - InternalFunds;
        InternalFunds = 0;
      }
      else
      {
        EIp(j) = floor((InternalFunds + prestmax) / p1(indforn)) * dim_mach;
        if (EIp(j) < 0)
        {
          EIp(j) = 0;
          InternalFunds = 0;
          prestmax = 0;
        }
        else
        {
          InternalFunds = 0;
          prestmax = 0;
        }
      }
    }

    if ((SId_cost(j) / dim_mach) * p1prova < InternalFunds)
    {
      SIp_cost(j) = SId_cost(j);
    }
    else
    {
      if ((SId_cost(j) / dim_mach) * p1prova - InternalFunds <= prestmax)
      {
        SIp_cost(j) = SId_cost(j);
      }
      else
      {
        SIp_cost(j) = floor((InternalFunds + prestmax) / p1(indforn)) * dim_mach;
        if (SIp_cost(j) < 0)
        {
          SIp_cost(j) = 0;
        }
      }
    }

    SIp(j) = SIp_age(j) + SIp_cost(j);

    Ip(j) = EIp(j) + SIp(j);

    // Determine cost of planned investment; NB: EI and SI are expressed in terms of productive capacity, not number of machines, hence need to be divided by dim_mach
    if (Ip(j) > 0)
    {
      CmachEI(j) = p1(indforn) * EIp(j) / dim_mach;
      CmachSI(j) = p1(indforn) * SIp(j) / dim_mach;
      CmachSI_age(j) = p1(indforn) * SIp_age(j) / dim_mach;
      CmachSI_cost(j) = p1(indforn) * SIp_cost(j) / dim_mach;
      Cmach(j) = p1(indforn) * Ip(j) / dim_mach;
    }
    else
    {
      CmachEI(j) = 0;
      CmachSI(j) = 0;
      CmachSI_age(j) = 0;
      CmachSI_cost(j) = 0;
      Cmach(j) = 0;
    }
  }
}

void ALLOCATECREDIT(void)
{
  ofstream Errors(errorfilename, ios::app);

  // C-firms calculate credit demand based on outstanding loans (need to be rolled over), planned investment and production
  for (j = 1; j <= N2; ++j)
  {
    if (Loans_2(1, j) + Cmach(j) + (c2e(j) * Qd(j)) <= Deposits_2(1, j))
    {
      CreditDemand(j) = 0;
    }
    else
    {
      CreditDemand(j) = Loans_2(1, j) + Cmach(j) + (c2e(j) * Qd(j)) - Deposits_2(1, j);
    }
  }

  // Banks allocate credit
  for (i = 1; i <= NB; i++)
  {
    for (j = 1; j <= NL_2(i); j++)
    {
      DS2_rating.Column(i).Minimum1(rated_firm_2);
      DS2_rating(rated_firm_2, i) = DS2_rating.Column(i).Maximum() + 1;
      if (BankMatch_2(rated_firm_2, i) == 1)
      {
        // If customer does not need credit, they use deposits to repay their outstanding loans
        if (CreditDemand(rated_firm_2) == 0)
        {
          Q2(rated_firm_2) = Qd(rated_firm_2);
          EI(1, rated_firm_2) = EIp(rated_firm_2);
          SI(rated_firm_2) = SIp(rated_firm_2);
          I(rated_firm_2) = EI(1, rated_firm_2) + SI(rated_firm_2);
          Deposits_2(1, rated_firm_2) = Deposits_2(1, rated_firm_2) - Loans_2(1, rated_firm_2);
          Loans_b(1, i) = max(0.0, Loans_b(1, i) - Loans_2(1, rated_firm_2));
          Deposits(1, i) = Deposits(1, i) - Loans_2(1, rated_firm_2);
          Loans_2(1, rated_firm_2) = 0;
        }
        else if (CreditDemand(rated_firm_2) > 0)
        {
          // If remaining credit supply of bank is sufficient, demand is fully satisfied
          if (CreditDemand(rated_firm_2) <= BankCredit(i))
          {
            Deposits_2(1, rated_firm_2) = max(0.0, Deposits_2(1, rated_firm_2) + CreditDemand(rated_firm_2) - Loans_2(1, rated_firm_2));
            Deposits(1, i) = Deposits(1, i) + CreditDemand(rated_firm_2) - Loans_2(1, rated_firm_2);
            Loans_b(1, i) = Loans_b(1, i) + CreditDemand(rated_firm_2) - Loans_2(1, rated_firm_2);
            Loans_2(1, rated_firm_2) = CreditDemand(rated_firm_2);
            BankCredit(i) -= CreditDemand(rated_firm_2);
            Q2(rated_firm_2) = Qd(rated_firm_2);
            EI(1, rated_firm_2) = EIp(rated_firm_2);
            SI(rated_firm_2) = SIp(rated_firm_2);
            I(rated_firm_2) = EI(1, rated_firm_2) + SI(rated_firm_2);
          }
          else
          {
            // If remaining credit supply is insufficient, first remove replacement investment due to high cost
            if (Loans_2(1, rated_firm_2) + CmachEI(rated_firm_2) + CmachSI_age(rated_firm_2) + (c2e(rated_firm_2) * Qd(rated_firm_2)) - Deposits_2(1, rated_firm_2) <= BankCredit(i))
            {
              if (Loans_2(1, rated_firm_2) + CmachEI(rated_firm_2) + CmachSI_age(rated_firm_2) + (c2e(rated_firm_2) * Qd(rated_firm_2)) - Deposits_2(1, rated_firm_2) >= 0)
              {
                Loans_b(1, i) = Loans_b(1, i) + CmachEI(rated_firm_2) + CmachSI_age(rated_firm_2) + (c2e(rated_firm_2) * Qd(rated_firm_2)) - Deposits_2(1, rated_firm_2);
                Loans_2(1, rated_firm_2) = Loans_2(1, rated_firm_2) + CmachEI(rated_firm_2) + CmachSI_age(rated_firm_2) + (c2e(rated_firm_2) * Qd(rated_firm_2)) - Deposits_2(1, rated_firm_2);
                Deposits(1, i) = Deposits(1, i) + CmachEI(rated_firm_2) + CmachSI_age(rated_firm_2) + (c2e(rated_firm_2) * Qd(rated_firm_2)) - Deposits_2(1, rated_firm_2);
                Deposits_2(1, rated_firm_2) = CmachEI(rated_firm_2) + CmachSI_age(rated_firm_2) + (c2e(rated_firm_2) * Qd(rated_firm_2));
              }
              else
              {
                Loans_b(1, i) = max(0.0, Loans_b(1, i) - Loans_2(1, rated_firm_2));
                Deposits(1, i) = Deposits(1, i) - Loans_2(1, rated_firm_2);
                Deposits_2(1, rated_firm_2) = Deposits_2(1, rated_firm_2) - Loans_2(1, rated_firm_2);
                Loans_2(1, rated_firm_2) = 0;
              }

              BankCredit(i) -= Loans_2(1, rated_firm_2);
              Q2(rated_firm_2) = Qd(rated_firm_2);
              EI(1, rated_firm_2) = EIp(rated_firm_2);
              SI(rated_firm_2) = SIp_age(rated_firm_2);
              I(rated_firm_2) = EI(1, rated_firm_2) + SI(rated_firm_2);
            }
            // If remaining credit supply is still insufficient, remove also replacement investment due to age
            else if (Loans_2(1, rated_firm_2) + CmachEI(rated_firm_2) + (c2e(rated_firm_2) * Qd(rated_firm_2)) - Deposits_2(1, rated_firm_2) <= BankCredit(i))
            {
              if (Loans_2(1, rated_firm_2) + CmachEI(rated_firm_2) + (c2e(rated_firm_2) * Qd(rated_firm_2)) - Deposits_2(1, rated_firm_2) >= 0)
              {
                Loans_b(1, i) = Loans_b(1, i) + CmachEI(rated_firm_2) + (c2e(rated_firm_2) * Qd(rated_firm_2)) - Deposits_2(1, rated_firm_2);
                Loans_2(1, rated_firm_2) = Loans_2(1, rated_firm_2) + CmachEI(rated_firm_2) + (c2e(rated_firm_2) * Qd(rated_firm_2)) - Deposits_2(1, rated_firm_2);
                Deposits(1, i) = Deposits(1, i) + CmachEI(rated_firm_2) + (c2e(rated_firm_2) * Qd(rated_firm_2)) - Deposits_2(1, rated_firm_2);
                Deposits_2(1, rated_firm_2) = CmachEI(rated_firm_2) + (c2e(rated_firm_2) * Qd(rated_firm_2));
              }
              else
              {
                Loans_b(1, i) = max(0.0, Loans_b(1, i) - Loans_2(1, rated_firm_2));
                Deposits(1, i) = Deposits(1, i) - Loans_2(1, rated_firm_2);
                Deposits_2(1, rated_firm_2) = Deposits_2(1, rated_firm_2) - Loans_2(1, rated_firm_2);
                Loans_2(1, rated_firm_2) = 0;
              }

              BankCredit(i) -= Loans_2(1, rated_firm_2);
              Q2(rated_firm_2) = Qd(rated_firm_2);
              EI(1, rated_firm_2) = EIp(rated_firm_2);
              SI(rated_firm_2) = 0;
              I(rated_firm_2) = EI(1, rated_firm_2) + SI(rated_firm_2);
            }
            // If remaining credit supply is still insufficient, remove also expansion investment
            else if (Loans_2(1, rated_firm_2) + (c2e(rated_firm_2) * Qd(rated_firm_2)) - Deposits_2(1, rated_firm_2) <= BankCredit(i))
            {
              if (Loans_2(1, rated_firm_2) + (c2e(rated_firm_2) * Qd(rated_firm_2)) - Deposits_2(1, rated_firm_2) >= 0)
              {
                Loans_b(1, i) = Loans_b(1, i) + (c2e(rated_firm_2) * Qd(rated_firm_2)) - Deposits_2(1, rated_firm_2);
                Loans_2(1, rated_firm_2) = Loans_2(1, rated_firm_2) + (c2e(rated_firm_2) * Qd(rated_firm_2)) - Deposits_2(1, rated_firm_2);
                Deposits(1, i) = Deposits(1, i) + (c2e(rated_firm_2) * Qd(rated_firm_2)) - Deposits_2(1, rated_firm_2);
                Deposits_2(1, rated_firm_2) = (c2e(rated_firm_2) * Qd(rated_firm_2));
              }
              else
              {
                Loans_b(1, i) = max(0.0, Loans_b(1, i) - Loans_2(1, rated_firm_2));
                Deposits(1, i) = Deposits(1, i) - Loans_2(1, rated_firm_2);
                Deposits_2(1, rated_firm_2) = Deposits_2(1, rated_firm_2) - Loans_2(1, rated_firm_2);
                Loans_2(1, rated_firm_2) = 0;
              }
              BankCredit(i) -= Loans_2(1, rated_firm_2);
              Q2(rated_firm_2) = Qd(rated_firm_2);
              EI(1, rated_firm_2) = 0;
              SI(rated_firm_2) = 0;
              I(rated_firm_2) = EI(1, rated_firm_2) + SI(rated_firm_2);
            }
            else if (Loans_2(1, rated_firm_2) + (c2e(rated_firm_2) * Qd(rated_firm_2)) - Deposits_2(1, rated_firm_2) > BankCredit(i))
            {
              // If possible, scale back desired production to a level which can be financed
              if (Loans_2(1, rated_firm_2) - Deposits_2(1, rated_firm_2) <= BankCredit(i))
              {
                Q2(rated_firm_2) = (BankCredit(i) - Loans_2(1, rated_firm_2) + Deposits_2(1, rated_firm_2)) / c2e(rated_firm_2);

                // If production needs to be scaled back too much, firm exits
                if (Q2(rated_firm_2) < 1)
                {
                  if (Loans_2(1, rated_firm_2) > Deposits_2(1, rated_firm_2))
                  {
                    Loans_b(1, i) -= Deposits_2(1, rated_firm_2);
                    Deposits(1, i) -= Deposits_2(1, rated_firm_2);
                    Loans_2(1, rated_firm_2) -= Deposits_2(1, rated_firm_2);
                    Deposits_2(1, rated_firm_2) = 0;
                    baddebt_2(rated_firm_2) = Loans_2(1, rated_firm_2);
                    baddebt_b(i) += Loans_2(1, rated_firm_2);
                    BankCredit(i) -= Loans_2(1, rated_firm_2);
                    Loans_b(1, i) = max(0.0, Loans_b(1, i) - Loans_2(1, rated_firm_2));
                    Loans_2(1, rated_firm_2) = 0;
                  }
                  else
                  {
                    Loans_b(1, i) = max(0.0, Loans_b(1, i) - Loans_2(1, rated_firm_2));
                    baddebt_2(rated_firm_2) = Loans_2(1, rated_firm_2) - Deposits_2(1, rated_firm_2);
                    Deposits_recovered_2 += Deposits_2(1, rated_firm_2) - Loans_2(1, rated_firm_2);
                    Deposits(1, i) -= Deposits_2(1, rated_firm_2);
                    Outflows(i) += Deposits_2(1, rated_firm_2) - Loans_2(1, rated_firm_2);
                    Deposits_2(1, rated_firm_2) = 0;
                    Loans_2(1, rated_firm_2) = 0;
                  }
                  Q2(rated_firm_2) = 0;
                  exiting_2(rated_firm_2) = 1;
                  f2(1, rated_firm_2) = 0;
                  f2(2, rated_firm_2) = 0;
                  f2(3, rated_firm_2) = 0;
                }
                else
                {
                  Loans_b(1, i) = Loans_b(1, i) + BankCredit(i) - Loans_2(1, rated_firm_2);
                  Deposits_2(1, rated_firm_2) = Deposits_2(1, rated_firm_2) - Loans_2(1, rated_firm_2) + BankCredit(i);
                  Deposits(1, i) = Deposits(1, i) - Loans_2(1, rated_firm_2) + BankCredit(i);
                  Loans_2(1, rated_firm_2) = BankCredit(i);
                  BankCredit(i) = 0;
                }
                EI(1, rated_firm_2) = 0;
                SI(rated_firm_2) = 0;
                I(rated_firm_2) = EI(1, rated_firm_2) + SI(rated_firm_2);
              }
              // Otherwise, firm is forced to exit
              else
              {
                Q2(rated_firm_2) = 0;
                BankCredit(i) = 0;
                if (Loans_2(1, rated_firm_2) > Deposits_2(1, rated_firm_2))
                {
                  Loans_b(1, i) -= Deposits_2(1, rated_firm_2);
                  Deposits(1, i) -= Deposits_2(1, rated_firm_2);
                  Loans_2(1, rated_firm_2) -= Deposits_2(1, rated_firm_2);
                  Deposits_2(1, rated_firm_2) = 0;
                  baddebt_2(rated_firm_2) = Loans_2(1, rated_firm_2);
                  baddebt_b(i) += Loans_2(1, rated_firm_2);
                  Loans_b(1, i) = max(0.0, Loans_b(1, i) - Loans_2(1, rated_firm_2));
                  Loans_2(1, rated_firm_2) = 0;
                }
                else
                {
                  Loans_b(1, i) = max(0.0, Loans_b(1, i) - Loans_2(1, rated_firm_2));
                  baddebt_2(rated_firm_2) = Loans_2(1, rated_firm_2) - Deposits_2(1, rated_firm_2);
                  Deposits_recovered_2 += Deposits_2(1, rated_firm_2) - Loans_2(1, rated_firm_2);
                  Deposits(1, i) -= Deposits_2(1, rated_firm_2);
                  Outflows(i) += Deposits_2(1, rated_firm_2) - Loans_2(1, rated_firm_2);
                  Deposits_2(1, rated_firm_2) = 0;
                  Loans_2(1, rated_firm_2) = 0;
                }
                EI(1, rated_firm_2) = 0;
                SI(rated_firm_2) = 0;
                I(rated_firm_2) = EI(1, rated_firm_2) + SI(rated_firm_2);
                exiting_2(rated_firm_2) = 1;
                f2(1, rated_firm_2) = 0;
                f2(2, rated_firm_2) = 0;
                f2(3, rated_firm_2) = 0;
              }
            }
          }
        }
      }
    }
  }

  // C-firms calculate labour demand based on production which can be financed
  for (j = 1; j <= N2; ++j)
  {
    if (A2e(j) > 0)
    {
      Ld2(j) = Q2(j) / A2e(j);
    }
    else
    {
      std::cerr << "\n\n ERROR: A2e(j) = 0 in period " << t << " for C-firm " << j << endl;
      Errors << "\n ERROR: A2e(j) = 0 in period " << t << " for C-firm " << j << endl;
      exit(EXIT_FAILURE);
    }
  }

  Errors.close();
}

void PRODMACH(void)
{
  ofstream Errors(errorfilename, ios::app);

  // K-firms receive demand from C-firms and calculate labour demand
  for (i = 1; i <= N1; i++)
  {
    for (j = 1; j <= N2; j++)
    {
      if (Match(j, i) == 1)
      {
        D1(i) += I(j) / dim_mach;
      }
    }

    Q1(i) = D1(i);

    if (A1p(i) > 0)
    {
      Ld1(i) = Q1(i) / ((1 - shocks_labprod1(i)) * A1p(i) * a);
    }
    else
    {
      std::cerr << "\n\n ERROR: A1p(i)=0 in period " << t << " for K-firm " << i << endl;
      Errors << "\n A1p(i)=0 in period " << t << " for K-firm " << i << endl;
      exit(EXIT_FAILURE);
    }
  }

  // NB: These are labour demands from last period!
  if ((LD1rdtot + LDentot) > LS)
  {
    std::cerr << "\n\n ERROR: Remaining labour supply is negative in period " << t << endl;
    Errors << "\n Remaining labour supply is negative in period " << t << endl;
    exit(EXIT_FAILURE);
  }

  // Determine total labour demand
  LABOR();

  // Calculate Energy demand based on actual production
  for (i = 1; i <= N1; i++)
  {
    if (A1p_en(i) > 0)
    {
      D1_en(i) = Q1(i) / ((1 - shocks_eneff1(i)) * A1p_en(i));
    }
    else
    {
      std::cerr << "\n\n ERROR: A1p_en(i)=0 in period " << t << " for K-firm " << i << endl;
      Errors << "\n A1p_en(i) = 0 in period " << t << " for K-firm " << i << endl;
      exit(EXIT_FAILURE);
    }
  }

  for (j = 1; j <= N2; j++)
  {
    if (A2e_en(j) > 0)
    {
      D2_en(j) = Q2(j) / A2e_en(j);
    }
    else
    {
      std::cerr << "\n\n ERROR: A2e_en(j) = 0 in period " << t << " for C-firm " << j << endl;
      Errors << "\n A2e_en(j) = 0 in period " << t << " for C-firm " << j << endl;
      exit(EXIT_FAILURE);
    }
  }

  if ((flag_outputshocks == 1))
  {
    for (i = 1; i <= N1; i++)
    {
      Qpast = Q1(i);
      if (Qpast > 0)
      {
        Q1(i) = Q1(i) * (1 - shocks_output1(i));
        for (j = 1; j <= N2; j++)
        {
          if (Match(j, i) == 1)
          {
            I(j) = floor((I(j) / dim_mach) * Q1(i) / Qpast) * dim_mach;
            if (I(j) < EI(1, j))
            {
              EI(1, j) = I(j);
            }
            SI(j) = I(j) - EI(1, j);
          }
        }
      }
    }
  }

  if ((flag_outputshocks == 2))
  {
    loss = Q1.Sum() * shocks_output1(1);
    while (loss > 0)
    {
      rani = int(ran1(p_seed) * N1 * N2) % N1 + 1;
      Qpast = Q1(rani);
      if (Qpast > 0)
      {
        if (Q1(rani) >= loss)
        {
          Q1(rani) -= loss;
          loss = 0;
        }
        else
        {
          loss -= Q1(rani);
          Q1(rani) = 0;
        }
        for (j = 1; j <= N2; j++)
        {
          if (Match(j, rani) == 1)
          {
            I(j) = floor((I(j) / dim_mach) * Q1(rani) / Qpast) * dim_mach;
            if (I(j) < EI(1, j))
            {
              EI(1, j) = I(j);
            }
            SI(j) = I(j) - EI(1, j);
          }
        }
      }
    }
  }

  if ((flag_outputshocks == 3))
  {
    loss = Q1.Sum() * shocks_output1(1);
    while (loss > 0)
    {
      rani = 0;
      while (rani == 0)
      {
        rani = int(ran1(p_seed) * N1 * N2) % N1 + 1;
        rnd = ran1(p_seed);
        if (risk_k(rani) < rnd)
        {
          rani = 0;
        }
      }
      Qpast = Q1(rani);
      if (Qpast > 0)
      {
        if (Q1(rani) >= loss)
        {
          Q1(rani) -= loss;
          loss = 0;
        }
        else
        {
          loss -= Q1(rani);
          Q1(rani) = 0;
        }
        for (j = 1; j <= N2; j++)
        {
          if (Match(j, rani) == 1)
          {
            I(j) = floor((I(j) / dim_mach) * Q1(rani) / Qpast) * dim_mach;
            if (I(j) < EI(1, j))
            {
              EI(1, j) = I(j);
            }
            SI(j) = I(j) - EI(1, j);
          }
        }
      }
    }
  }

  // Calculate nominal value of actual investment
  for (j = 1; j <= N2; j++)
  {
    indforn = int(fornit(j));
    SI_n(j) = p1(indforn) * SI(j) / dim_mach;
    EI_n(j) = p1(indforn) * EI(1, j) / dim_mach;
    S1(indforn) += p1(indforn) * I(j) / dim_mach;
  }

  // Old machines are scrapped; temporary machine frequency arrays are updated based on expansion & replacement investment
  for (j = 1; j <= N2; j++)
  {

    CANCMACH();

    for (i = 1; i <= N1; i++)
    {
      for (tt = t0; tt <= t; tt++)
      {
        if (gtemp[tt - 1][i - 1][j - 1] == 0)
        {
          age[tt - 1][i - 1][j - 1] = 0;
        }
      }
    }

    indforn = int(fornit(j));
    gtemp[t - 1][indforn - 1][j - 1] += I(j) / dim_mach;
    g_price[t - 1][indforn - 1][j - 1] = p1(indforn);
    deltaCapitalStock(1, j) += EI_n(j);

    if (I(j) > 0)
    {
      if (gtemp[t - 1][indforn - 1][j - 1] == I(j) / dim_mach)
      {
        age[t - 1][indforn - 1][j - 1] = 0;
      }
    }
  }

  if (flag_capshocks == 1)
  {
    // Shock to C-firms' capital stock
    for (j = 1; j <= N2; j++)
    {
      K_cur(j) = K(j);
      loss = 0;
      for (i = 1; i <= N1; i++)
      {
        for (tt = t0; tt <= (t); tt++)
        {
          loss += gtemp[tt - 1][i - 1][j - 1];
        }
      }
      loss = floor(loss * shocks_capstock(j));
      while (loss > 0)
      {
        rani = int(ran1(p_seed) * N1 * N2) % N1 + 1;
        rant = int(ran1(p_seed) * t0 * (t)) % ((t)-t0 + 1) + t0;
        if (gtemp[rant - 1][rani - 1][j - 1] > 0)
        {
          if (loss < gtemp[rant - 1][rani - 1][j - 1])
          {
            if (rant == t)
            {
              deltaCapitalStock(1, j) -= loss * g_price[rant - 1][rani - 1][j - 1];
              gtemp[rant - 1][rani - 1][j - 1] -= loss;
              if (SI(j) > 0)
              {
                K(j) -= min(SI(j), loss * dim_mach);
                loss = max(0.0, loss * dim_mach - SI(j)) / dim_mach;
              }
              EI(1, j) -= loss * dim_mach;
              EI(1, j) = max(EI(1, j), 0.0);
              loss = 0;
            }
            else
            {
              Loss_Capital(j) += min(CapitalStock(1, j), g_price[rant - 1][rani - 1][j - 1] * loss);
              CapitalStock(1, j) -= min(CapitalStock(1, j), g_price[rant - 1][rani - 1][j - 1] * loss);
              K(j) -= loss * dim_mach;
              K_cur(j) -= loss * dim_mach;
              gtemp[rant - 1][rani - 1][j - 1] -= loss;
              g_c2[rant - 1][rani - 1][j - 1] -= loss;
              g_c3[rant - 1][rani - 1][j - 1] -= loss;
              loss = 0;
            }
          }
          else
          {
            if (rant == t)
            {
              deltaCapitalStock(1, j) -= g_price[rant - 1][rani - 1][j - 1] * gtemp[rant - 1][rani - 1][j - 1];
              if (SI(j) > 0)
              {
                K(j) -= gtemp[rant - 1][rani - 1][j - 1] * dim_mach - EI(1, j);
              }
              EI(1, j) = 0;
              loss -= gtemp[rant - 1][rani - 1][j - 1];
              gtemp[rant - 1][rani - 1][j - 1] = 0;
            }
            else
            {
              Loss_Capital(j) += min(CapitalStock(1, j), g_price[rant - 1][rani - 1][j - 1] * gtemp[rant - 1][rani - 1][j - 1]);
              CapitalStock(1, j) -= min(CapitalStock(1, j), g_price[rant - 1][rani - 1][j - 1] * gtemp[rant - 1][rani - 1][j - 1]);
              K(j) -= gtemp[rant - 1][rani - 1][j - 1] * dim_mach;
              K_cur(j) -= gtemp[rant - 1][rani - 1][j - 1] * dim_mach;
              loss -= gtemp[rant - 1][rani - 1][j - 1];
              g_c2[rant - 1][rani - 1][j - 1] -= gtemp[rant - 1][rani - 1][j - 1];
              g_c3[rant - 1][rani - 1][j - 1] -= gtemp[rant - 1][rani - 1][j - 1];
              gtemp[rant - 1][rani - 1][j - 1] = 0;
            }
          }
        }
      }
      if (shocks_capstock(j) > 0)
      {
        Q2(j) = min(Q2(j), K_cur(j));
        ADJUSTEMISSENLAB();
      }
    }
  }

  if (flag_capshocks == 2)
  {
    // Shock to C-firms' capital stock
    loss = 0;
    K_temp_sum = 0;
    K_cur = K;
    for (j = 1; j <= N2; j++)
    {
      for (i = 1; i <= N1; i++)
      {
        for (tt = t0; tt <= (t); tt++)
        {
          loss += gtemp[tt - 1][i - 1][j - 1];
          K_temp_sum += gtemp[tt - 1][i - 1][j - 1];
        }
      }
    }

    if (loss == N2)
    {
      loss = 0;
    }
    else
    {
      loss = min(K_temp_sum - N2, floor(loss * shocks_capstock(1)));
    }

    while (loss > 0)
    {
      ranj = int(ran1(p_seed) * N1 * N2) % N2 + 1;
      lossj = 0;
      for (i = 1; i <= N1; i++)
      {
        for (tt = t0; tt <= (t); tt++)
        {
          lossj += gtemp[tt - 1][i - 1][ranj - 1];
        }
      }

      lossj = min(loss, (lossj - 1));
      while (lossj > 0)
      {
        rani = int(ran1(p_seed) * N1 * N2) % N1 + 1;
        rant = int(ran1(p_seed) * t0 * (t)) % ((t)-t0 + 1) + t0;
        if (gtemp[rant - 1][rani - 1][ranj - 1] > 0)
        {
          if (lossj < gtemp[rant - 1][rani - 1][ranj - 1])
          {
            if (rant == t)
            {
              K_loss(ranj) += lossj;
              deltaCapitalStock(1, ranj) -= lossj * g_price[rant - 1][rani - 1][ranj - 1];
              gtemp[rant - 1][rani - 1][ranj - 1] -= lossj;
              loss -= lossj;
              if (SI(ranj) > 0)
              {
                K(ranj) -= min(SI(ranj), lossj * dim_mach);
                lossj = max(0.0, lossj * dim_mach - SI(ranj)) / dim_mach;
              }
              EI(1, ranj) -= lossj * dim_mach;
              EI(1, ranj) = max(EI(1, ranj), 0.0);
              lossj = 0;
            }
            else
            {
              K_loss(ranj) += lossj;
              Loss_Capital(ranj) += min(CapitalStock(1, ranj), g_price[rant - 1][rani - 1][ranj - 1] * lossj);
              CapitalStock(1, ranj) -= min(CapitalStock(1, ranj), g_price[rant - 1][rani - 1][ranj - 1] * lossj);
              K(ranj) -= lossj * dim_mach;
              K_cur(ranj) -= lossj * dim_mach;
              gtemp[rant - 1][rani - 1][ranj - 1] -= lossj;
              g_c2[rant - 1][rani - 1][ranj - 1] -= lossj;
              g_c3[rant - 1][rani - 1][ranj - 1] -= lossj;
              loss -= lossj;
              lossj = 0;
            }
          }
          else
          {
            if (rant == t)
            {
              K_loss(ranj) += gtemp[rant - 1][rani - 1][ranj - 1];
              deltaCapitalStock(1, ranj) -= g_price[rant - 1][rani - 1][ranj - 1] * gtemp[rant - 1][rani - 1][ranj - 1];
              if (SI(ranj) > 0)
              {
                K(ranj) -= gtemp[rant - 1][rani - 1][ranj - 1] * dim_mach - EI(1, ranj);
              }
              EI(1, ranj) = 0;
              lossj -= gtemp[rant - 1][rani - 1][ranj - 1];
              loss -= gtemp[rant - 1][rani - 1][ranj - 1];
              gtemp[rant - 1][rani - 1][ranj - 1] = 0;
            }
            else
            {
              K_loss(ranj) += gtemp[rant - 1][rani - 1][ranj - 1];
              Loss_Capital(ranj) += min(CapitalStock(1, ranj), g_price[rant - 1][rani - 1][ranj - 1] * gtemp[rant - 1][rani - 1][ranj - 1]);
              CapitalStock(1, ranj) -= min(CapitalStock(1, ranj), g_price[rant - 1][rani - 1][ranj - 1] * gtemp[rant - 1][rani - 1][ranj - 1]);
              K(ranj) -= gtemp[rant - 1][rani - 1][ranj - 1] * dim_mach;
              K_cur(ranj) -= gtemp[rant - 1][rani - 1][ranj - 1] * dim_mach;
              lossj -= gtemp[rant - 1][rani - 1][ranj - 1];
              loss -= gtemp[rant - 1][rani - 1][ranj - 1];
              g_c2[rant - 1][rani - 1][ranj - 1] -= gtemp[rant - 1][rani - 1][ranj - 1];
              g_c3[rant - 1][rani - 1][ranj - 1] -= gtemp[rant - 1][rani - 1][ranj - 1];
              gtemp[rant - 1][rani - 1][ranj - 1] = 0;
            }
          }
        }
      }
    }

    if (K_loss.Sum() > 0)
    {
      for (j = 1; j <= N2; j++)
      {
        if (K_loss(j) > 0)
        {
          Q2(j) = min(Q2(j), K_cur(j));
          ADJUSTEMISSENLAB();
        }
      }
    }
  }

  if (flag_capshocks == 3)
  {
    // Shock to C-firms' capital stock
    loss = 0;
    K_temp_sum = 0;
    K_cur = K;
    for (j = 1; j <= N2; j++)
    {
      for (i = 1; i <= N1; i++)
      {
        for (tt = t0; tt <= (t); tt++)
        {
          loss += gtemp[tt - 1][i - 1][j - 1];
          K_temp_sum += gtemp[tt - 1][i - 1][j - 1];
        }
      }
    }

    if (loss == N2)
    {
      loss = 0;
    }
    else
    {
      loss = min(K_temp_sum - N2, floor(loss * shocks_capstock(1)));
    }

    while (loss > 0)
    {
      ranj = 0;
      while (ranj == 0)
      {
        ranj = int(ran1(p_seed) * N1 * N2) % N2 + 1;
        rnd = ran1(p_seed);
        if (risk_c(ranj) < rnd)
        {
          ranj = 0;
        }
      }

      lossj = 0;
      for (i = 1; i <= N1; i++)
      {
        for (tt = t0; tt <= (t); tt++)
        {
          lossj += gtemp[tt - 1][i - 1][ranj - 1];
        }
      }

      lossj = min(loss, (lossj - 1));
      while (lossj > 0)
      {
        rani = int(ran1(p_seed) * N1 * N2) % N1 + 1;
        rant = int(ran1(p_seed) * t0 * (t)) % ((t)-t0 + 1) + t0;
        if (gtemp[rant - 1][rani - 1][ranj - 1] > 0)
        {
          if (lossj < gtemp[rant - 1][rani - 1][ranj - 1])
          {
            if (rant == t)
            {
              K_loss(ranj) += lossj;
              deltaCapitalStock(1, ranj) -= lossj * g_price[rant - 1][rani - 1][ranj - 1];
              gtemp[rant - 1][rani - 1][ranj - 1] -= lossj;
              loss -= lossj;
              if (SI(ranj) > 0)
              {
                K(ranj) -= min(SI(ranj), lossj * dim_mach);
                lossj = max(0.0, lossj * dim_mach - SI(ranj)) / dim_mach;
              }
              EI(1, ranj) -= lossj * dim_mach;
              EI(1, ranj) = max(EI(1, ranj), 0.0);
              lossj = 0;
            }
            else
            {
              K_loss(ranj) += lossj;
              Loss_Capital(ranj) += min(CapitalStock(1, ranj), g_price[rant - 1][rani - 1][ranj - 1] * lossj);
              CapitalStock(1, ranj) -= min(CapitalStock(1, ranj), g_price[rant - 1][rani - 1][ranj - 1] * lossj);
              K(ranj) -= lossj * dim_mach;
              K_cur(ranj) -= lossj * dim_mach;
              gtemp[rant - 1][rani - 1][ranj - 1] -= lossj;
              g_c2[rant - 1][rani - 1][ranj - 1] -= lossj;
              g_c3[rant - 1][rani - 1][ranj - 1] -= lossj;
              loss -= lossj;
              lossj = 0;
            }
          }
          else
          {
            if (rant == t)
            {
              K_loss(ranj) += gtemp[rant - 1][rani - 1][ranj - 1];
              deltaCapitalStock(1, ranj) -= g_price[rant - 1][rani - 1][ranj - 1] * gtemp[rant - 1][rani - 1][ranj - 1];
              if (SI(ranj) > 0)
              {
                K(ranj) -= gtemp[rant - 1][rani - 1][ranj - 1] * dim_mach - EI(1, ranj);
              }
              EI(1, ranj) = 0;
              lossj -= gtemp[rant - 1][rani - 1][ranj - 1];
              loss -= gtemp[rant - 1][rani - 1][ranj - 1];
              gtemp[rant - 1][rani - 1][ranj - 1] = 0;
            }
            else
            {
              K_loss(ranj) += gtemp[rant - 1][rani - 1][ranj - 1];
              Loss_Capital(ranj) += min(CapitalStock(1, ranj), g_price[rant - 1][rani - 1][ranj - 1] * gtemp[rant - 1][rani - 1][ranj - 1]);
              CapitalStock(1, ranj) -= min(CapitalStock(1, ranj), g_price[rant - 1][rani - 1][ranj - 1] * gtemp[rant - 1][rani - 1][ranj - 1]);
              K(ranj) -= gtemp[rant - 1][rani - 1][ranj - 1] * dim_mach;
              K_cur(ranj) -= gtemp[rant - 1][rani - 1][ranj - 1] * dim_mach;
              lossj -= gtemp[rant - 1][rani - 1][ranj - 1];
              loss -= gtemp[rant - 1][rani - 1][ranj - 1];
              g_c2[rant - 1][rani - 1][ranj - 1] -= gtemp[rant - 1][rani - 1][ranj - 1];
              g_c3[rant - 1][rani - 1][ranj - 1] -= gtemp[rant - 1][rani - 1][ranj - 1];
              gtemp[rant - 1][rani - 1][ranj - 1] = 0;
            }
          }
        }
      }
    }

    if (K_loss.Sum() > 0)
    {
      for (j = 1; j <= N2; j++)
      {
        if (K_loss(j) > 0)
        {
          Q2(j) = min(Q2(j), K_cur(j));
          ADJUSTEMISSENLAB();
        }
      }
    }
  }

  EN_DEM();

  Errors.close();
}

void ADJUSTEMISSENLAB(void)
{
  ofstream Errors(errorfilename, ios::app);
  nmachprod = ceil(Q2(j) / dim_mach);
  nmp_temp = nmachprod;

  while (nmp_temp > 0)
  {
    cmin = 10000000000000000;
    imin = 0;
    jmin = 0;
    tmin = 0;

    for (i = 1; i <= N1; i++)
    {
      for (tt = t0; tt <= t; tt++)
      {
        if (g_c2[tt - 1][i - 1][j - 1] > 0 && (w(2) / ((1 - shocks_labprod2(j)) * A(tt, i)) + c_en(2) / A_en(tt, i) + t_CO2 * A_ef(tt, i) / A_en(tt, i)) < cmin)
        {
          cmin = w(2) / ((1 - shocks_labprod2(j)) * A(tt, i)) + c_en(2) / A_en(tt, i) + t_CO2 * A_ef(tt, i) / A_en(tt, i);
          imin = i;
          jmin = j;
          tmin = tt;
        }
      }
    }

    if (nmachprod > 0)
    {
      if (g_c2[tmin - 1][imin - 1][jmin - 1] >= nmp_temp)
      {
        A2e2(j) += (1 - shocks_labprod2(j)) * A(tmin, imin) * nmp_temp / nmachprod;
        A2e_en2(j) += (1 - shocks_eneff2(j)) * A_en(tmin, imin) * nmp_temp / nmachprod;
        A2e_ef2(j) += A_ef(tmin, imin) * nmp_temp / nmachprod;
        g_c2[tmin - 1][imin - 1][jmin - 1] -= nmp_temp;
        nmp_temp = 0;
      }
      else
      {
        A2e2(j) += (1 - shocks_labprod2(j)) * A(tmin, imin) * g_c2[tmin - 1][imin - 1][jmin - 1] / nmachprod;
        A2e_en2(j) += (1 - shocks_eneff2(j)) * A_en(tmin, imin) * g_c2[tmin - 1][imin - 1][jmin - 1] / nmachprod;
        A2e_ef2(j) += A_ef(tmin, imin) * g_c2[tmin - 1][imin - 1][jmin - 1] / nmachprod;
        nmp_temp -= g_c2[tmin - 1][imin - 1][jmin - 1];
        g_c2[tmin - 1][imin - 1][jmin - 1] = 0;
      }
    }
    else
    {
      std::cerr << "\n\n ERROR: nmachprod = 0!!!" << endl;
      Errors << "\n nmachprod = 0 in period " << t << endl;
      exit(EXIT_FAILURE);
    }
  }

  if (A2e2(j) > 0)
  {
    Ld2_control(j) = Q2(j) / A2e2(j);
  }
  else
  {
    if (Q2(j) > 0)
    {
      std::cerr << "\n\n ERROR: Q2>0 and A2e2 == 0!!!" << endl;
      Errors << "\n  Q2>0 and A2e2 == 0 in period " << t << endl;
      exit(EXIT_FAILURE);
    }
    else
    {
      Ld2_control(j) = 0;
    }
  }

  if (Ld2_control(j) > Ld2(j))
  {
    A2e2(j) = 0;
    A2e_en2(j) = 0;
    A2e_ef2(j) = 0;
    Ldtemp = Ld2(j);
    nmachprod = 0;
    while (Ldtemp > 0)
    {
      cmin = 10000000000000000;
      imin = 0;
      jmin = 0;
      tmin = 0;

      for (i = 1; i <= N1; i++)
      {
        for (tt = t0; tt <= t; tt++)
        {
          if (g_c3[tt - 1][i - 1][j - 1] > 0 && (w(2) / ((1 - shocks_labprod2(j)) * A(tt, i)) + c_en(2) / A_en(tt, i) + t_CO2 * A_ef(tt, i) / A_en(tt, i)) < cmin)
          {
            cmin = w(2) / ((1 - shocks_labprod2(j)) * A(tt, i)) + c_en(2) / A_en(tt, i) + t_CO2 * A_ef(tt, i) / A_en(tt, i);
            imin = i;
            jmin = j;
            tmin = tt;
          }
        }
      }

      if ((g_c3[tmin - 1][imin - 1][jmin - 1] * dim_mach / A(tmin, imin)) > Ldtemp)
      {
        nmachprod += ceil(Ldtemp * A(tmin, imin) / dim_mach);
        A2e2(j) += (1 - shocks_labprod2(j)) * A(tmin, imin) * ceil(Ldtemp * A(tmin, imin) / dim_mach);
        A2e_en2(j) += (1 - shocks_eneff2(j)) * A_en(tmin, imin) * ceil(Ldtemp * A(tmin, imin) / dim_mach);
        A2e_ef2(j) += A_ef(tmin, imin) * ceil(Ldtemp * A(tmin, imin) / dim_mach);
        g_c3[tmin - 1][imin - 1][jmin - 1] -= ceil(Ldtemp * A(tmin, imin) / dim_mach);
        Ldtemp = 0;
      }
      else
      {
        Ldtemp -= (g_c3[tmin - 1][imin - 1][jmin - 1] * dim_mach / A(tmin, imin));
        nmachprod += g_c3[tmin - 1][imin - 1][jmin - 1];
        A2e2(j) += (1 - shocks_labprod2(j)) * A(tmin, imin) * g_c3[tmin - 1][imin - 1][jmin - 1];
        A2e_en2(j) += (1 - shocks_eneff2(j)) * A_en(tmin, imin) * g_c3[tmin - 1][imin - 1][jmin - 1];
        A2e_ef2(j) += A_ef(tmin, imin) * g_c3[tmin - 1][imin - 1][jmin - 1];
        g_c3[tmin - 1][imin - 1][jmin - 1] = 0;
      }
    }
    A2e2(j) /= nmachprod;
    Q2(j) = A2e2(j) * Ld2(j);
    A2e_en2(j) /= nmachprod;
    A2e_ef2(j) /= nmachprod;
  }

  if (A2e_en2(j) > 0)
  {
    D2_en(j) = Q2(j) / A2e_en2(j);
  }
  else
  {
    if (Q2(j) > 0)
    {
      std::cerr << "\n\n ERROR: Q2>0 and A2e_en2 == 0!!!" << endl;
      Errors << "\n  Q2>0 and A2e_en2 == 0 in period " << t << endl;
      exit(EXIT_FAILURE);
    }
    else
    {
      D2_en(j) = 0;
    }
  }

  if (A2e_ef2(j) > 0 && A2e_en2(j) > 0)
  {
    Emiss2(j) = A2e_ef2(j) / A2e_en2(j) * Q2(j);
  }
  else
  {
    if (Q2(j) > 0)
    {
      std::cerr << "\n\n ERROR: Q2>0 and A2e_ef2 or A2e_en2 == 0!!!" << endl;
      Errors << "\n  Q2>0 and A2e_ef2 or A2e_en2 == 0 in period " << t << endl;
      exit(EXIT_FAILURE);
    }
    else
    {
      Emiss2(j) = 0;
    }
  }

  Errors.close();
}

void CANCMACH(void)
{
  // Based on actual replacement investment carried out, machines are scrapped
  scrapmax = (scrap_age(j) + SI(j)) / dim_mach;
  indforn = int(fornit(j));

  scrap_n = 0;
  if (scrapmax > 0)
  {
    // First scrap machines which are too old, then ones with high production cost
    for (i = 1; i <= N1; i++)
    {
      for (tt = t0; tt <= t; tt++)
      {
        if (scrapmax > 0 && g_pb[tt - 1][i - 1][j - 1] > 0 && age[tt - 1][i - 1][j - 1] > (agemax))
        {
          if (g_pb[tt - 1][i - 1][j - 1] >= scrapmax)
          {
            gtemp[tt - 1][i - 1][j - 1] -= scrapmax;
            g_pb[tt - 1][i - 1][j - 1] -= scrapmax;
            scrap_n += scrapmax * g_price[tt - 1][i - 1][j - 1];
            scrapmax = 0;
          }
          else
          {
            scrapmax -= g_pb[tt - 1][i - 1][j - 1];
            gtemp[tt - 1][i - 1][j - 1] -= g_pb[tt - 1][i - 1][j - 1];
            scrap_n += g_pb[tt - 1][i - 1][j - 1] * g_price[tt - 1][i - 1][j - 1];
            g_pb[tt - 1][i - 1][j - 1] = 0;
          }
        }
      }
    }
  }

  while (scrapmax > 0)
  {
    cmax = 0;

    for (i = 1; i <= N1; i++)
    {
      for (tt = t0; tt <= t; tt++)
      {
        if (g_pb[tt - 1][i - 1][j - 1] > 0 && C_pb[tt - 1][i - 1][j - 1] > cmax)
        {
          ind_i = i;
          ind_tt = tt;
          cmax = C_pb[tt - 1][i - 1][j - 1];
        }
      }
    }

    if (g_pb[ind_tt - 1][ind_i - 1][j - 1] >= scrapmax)
    {
      gtemp[ind_tt - 1][ind_i - 1][j - 1] -= scrapmax;
      g_pb[ind_tt - 1][ind_i - 1][j - 1] -= scrapmax;
      scrap_n += scrapmax * g_price[ind_tt - 1][ind_i - 1][j - 1];
      scrapmax = 0;
    }
    else
    {
      scrapmax -= g_pb[ind_tt - 1][ind_i - 1][j - 1];
      scrap_n += g_pb[ind_tt - 1][ind_i - 1][j - 1] * g_price[ind_tt - 1][ind_i - 1][j - 1];
      gtemp[ind_tt - 1][ind_i - 1][j - 1] -= g_pb[ind_tt - 1][ind_i - 1][j - 1];
      g_pb[ind_tt - 1][ind_i - 1][j - 1] = 0;
    }
  }

  if (SId_age(j) > SI(j))
  {
    K_temp(j) = 0;
    for (i = 1; i <= N1; i++)
    {
      for (tt = t0; tt <= t; tt++)
      {
        K_temp(j) += gtemp[tt - 1][i - 1][j - 1];
      }
    }

    for (i = 1; i <= N1; i++)
    {
      for (tt = t0; tt <= t; tt++)
      {
        if (age[tt - 1][i - 1][j - 1] > (agemax) && gtemp[tt - 1][i - 1][j - 1] > 0 && K_temp(j) > 1)
        {
          if (gtemp[tt - 1][i - 1][j - 1] <= (K_temp(j) - 1))
          {
            scrap_n += gtemp[tt - 1][i - 1][j - 1] * g_price[tt - 1][i - 1][j - 1];
            scrap_age(j) += gtemp[tt - 1][i - 1][j - 1] * dim_mach;
            K_temp(j) -= gtemp[tt - 1][i - 1][j - 1];
            gtemp[tt - 1][i - 1][j - 1] = 0;
          }
          else
          {
            scrap_n += (K_temp(j) - 1) * g_price[tt - 1][i - 1][j - 1];
            scrap_age(j) += (K_temp(j) - 1) * dim_mach;
            gtemp[tt - 1][i - 1][j - 1] -= (K_temp(j) - 1);
            K_temp(j) = 1;
          }
        }
      }
    }
  }

  deltaCapitalStock(1, j) += (SI_n(j) - scrap_n);
}

void PAY_LAB_INV(void)
{
  ofstream Errors(errorfilename, ios::app);

  // C-firms pay wages
  for (j = 1; j <= N2; j++)
  {
    sendingBank = BankingSupplier_2(j);
    Wages_2(j) = w(2) * (Ld2(j));
    if (Deposits_2(1, j) >= Wages_2(j))
    {
      Deposits_2(1, j) -= Wages_2(j);
      Wages += Wages_2(j);
      Deposits(1, sendingBank) -= Wages_2(j);
      Outflows(sendingBank) += Wages_2(j);
    }
    else
    {
      if (Wages_2(j) > 0)
      {
        deviation = ((Wages_2(j) - Deposits_2(1, j)) / Wages_2(j));
        if (deviation <= tolerance)
        {
          Wages_2(j) = Deposits_2(1, j);
          Deposits_2(1, j) -= Wages_2(j);
          Wages += Wages_2(j);
          Deposits(1, sendingBank) -= Wages_2(j);
          Outflows(sendingBank) += Wages_2(j);
        }
        else
        {
          std::cerr << "\n\n ERROR: C-Firm " << j << " cannot pay wages in period " << t << endl;
          Errors << "\n C-Firm " << j << " cannot pay wages in period " << t << endl;
          exit(EXIT_FAILURE);
        }
      }
    }

    // C-firms pay for investment
    indforn = int(fornit(j));
    Investment_2(j) = EI_n(j) + SI_n(j);
    receivingBank = BankingSupplier_1(indforn);
    if (Deposits_2(1, j) >= Investment_2(j))
    {
      Deposits_2(1, j) -= Investment_2(j);
      Deposits(1, sendingBank) -= Investment_2(j);
      Outflows(sendingBank) += Investment_2(j);

      Deposits_1(1, indforn) += Investment_2(j);
      Deposits(1, receivingBank) += Investment_2(j);
      Inflows(receivingBank) += Investment_2(j);
    }
    else
    {
      if (Investment_2(j) > 0)
      {
        deviation = ((Investment_2(j) - Deposits_2(1, j)) / Investment_2(j));
        if (deviation <= tolerance)
        {
          S1(indforn) -= Investment_2(j);
          S1(indforn) += Deposits_2(1, j);
          Investment_2(j) = Deposits_2(1, j);
          Deposits_2(1, j) -= Investment_2(j);
          Deposits(1, sendingBank) -= Investment_2(j);
          Outflows(sendingBank) += Investment_2(j);

          Deposits_1(1, indforn) += Investment_2(j);
          Deposits(1, receivingBank) += Investment_2(j);
          Inflows(receivingBank) += Investment_2(j);
        }
        else
        {
          std::cerr << "\n\n ERROR: C-Firm " << j << " cannot pay for investment in period " << t << endl;
          Errors << "\n C-Firm " << j << " cannot pay for investment in period " << t << endl;
          exit(EXIT_FAILURE);
        }
      }
    }
  }

  // K-firms pay wages
  for (i = 1; i <= N1; i++)
  {
    sendingBank = BankingSupplier_1(i);
    Wages_1(i) = w(2) * (Ld1(i) + Ld1rd(i));
    if (Deposits_1(1, i) >= Wages_1(i))
    {
      Deposits_1(1, i) -= Wages_1(i);
      Wages += Wages_1(i);
      Deposits(1, sendingBank) -= Wages_1(i);
      Outflows(sendingBank) += Wages_1(i);
    }
    else
    {
      if (Wages_1(i) > 0)
      {
        deviation = ((Wages_1(i) - Deposits_1(1, i)) / Wages_1(i));
        if (deviation <= tolerance)
        {
          Wages_1(i) = Deposits_1(1, i);
          Deposits_1(1, i) -= Wages_1(i);
          Wages += Wages_1(i);
          Deposits(1, sendingBank) -= Wages_1(i);
          Outflows(sendingBank) += Wages_1(i);
        }
        else
        {
          Wages_1(i) = Deposits_1(1, i);
          Deposits_1(1, i) -= Wages_1(i);
          Wages += Wages_1(i);
          Deposits(1, sendingBank) -= Wages_1(i);
          Outflows(sendingBank) += Wages_1(i);
          exiting_1(i) = 1;
        }
      }
    }
  }

  // Energy sector pays wages
  if (Deposits_e(1) >= Wages_en)
  {
    Deposits_e(1) -= Wages_en;
    Wages += Wages_en;
  }
  else
  {
    deviation = ((Wages_en - Deposits_e(1)) / Wages_en);
    if (deviation <= tolerance)
    {
      Wages_en = Deposits_e(1);
      Deposits_e(1) -= Wages_en;
      Wages += Wages_en;
    }
    else
    {
      std::cerr << "\n\n ERROR: Energy sector cannot pay wages in period " << t << endl;
      Errors << "\n Energy sector cannot pay wages in period " << t << endl;
      exit(EXIT_FAILURE);
    }
  }

  for (i = 1; i <= NB; i++)
  {
    Deposits(1, i) -= Wages_en * DepositShare_e(i);
    Outflows(i) += Wages_en * DepositShare_e(i);
    Deposits_eb(1, i) -= Wages_en * DepositShare_e(i);
  }

  Deposits_h(1) += (Wages + Benefits);

  // Households receive wages & Benefits
  for (i = 1; i <= NB; i++)
  {
    Deposits(1, i) += (Wages + Benefits) * DepositShare_h(i);
    Inflows(i) += (Wages + Benefits) * DepositShare_h(i);
    Deposits_hb(1, i) += (Wages + Benefits) * DepositShare_h(i);
  }

  Errors.close();
}

void COMPET2(void)
{
  ofstream Errors(errorfilename, ios::app);
  // C-firms' market shares are updated using quasi-replicator dynamics. Firms with too low market share exit
  p2m = p2.Sum() / N2r;
  l2m = l2.Sum() / N2r;

  ftot = 0;

  for (j = 1; j <= N2; j++)
  {
    if (omega1 <= 1)
    {
      E2(j) = -omega1 * p2(j) / p2m - omega2 * l2(j) / l2m;
    }
    else
    {
      E2(j) = -pow(p2(j) / p2m, omega1) - pow(l2(j) / l2m, omega2);
    }
    ftot(2) += f2(2, j);
  }

  if (ftot(2) > 0)
  {
    for (j = 1; j <= N2; j++)
    {
      f2(2, j) /= ftot(2);
      Em2(1) += E2(j) * f2(2, j);
    }
  }
  else
  {
    std::cerr << "\n\n ERROR: f2 = 0 for all firms in period " << t << endl;
    Errors << "\n f2 = 0 for all firms in period " << t << endl;
    exit(EXIT_FAILURE);
  }

  ftot = 0;

  if (Em2(1) == 0)
  {
    std::cerr << "\n\n ERROR: Em2(1)=0 in period " << t << endl;
    Errors << "\n Em2(1)=0 in period " << t << endl;
    exit(EXIT_FAILURE);
  }

  for (j = 1; j <= N2; j++)
  {
    f2(1, j) = f2(2, j) * ((2 * omega3) / (1 + exp((-chi) * ((E2(j) - Em2(1)) / Em2(1)))) + (1 - omega3));

    if (f2(1, j) <= (1 / (N2r * 500)))
    {
      f2(1, j) = 0;
      f2(2, j) = 0;
      f2(3, j) = 0;
      if (exiting_2(j) == 0 && exit_payments2(j) == 0)
      {
        exit_marketshare2(j) = 1;
      }
    }
    ftot(1) += f2(1, j);
    ftot(2) += f2(2, j);
    ftot(3) += f2(3, j);
  }

  if (ftot(1) == 0 || ftot(2) == 0 || ftot(3) == 0)
  {
    std::cerr << "\n\n ERROR: ftot=0 in period " << t << endl;
    Errors << "\n ftot=0 in period " << t << endl;
    exit(EXIT_FAILURE);
  }

  for (j = 1; j <= N2; j++)
  {
    f2(1, j) /= ftot(1);
    f2(2, j) /= ftot(2);
    f2(3, j) /= ftot(3);
  }

  Errors.close();
}

void PROFIT(void)
{
  ofstream Errors(errorfilename, ios::app);

  // Calculate K-firm profit
  for (i = 1; i <= N1; i++)
  {
    kpi += Q1(i) / Q1.Sum() * p1(i);
    EnergyPayments_1(i) = c_en(1) * D1_en(i);
    Pi1(i) = S1(i) + InterestDeposits_1(i) - Wages_1(i) - EnergyPayments_1(i) - t_CO2 * Emiss1(i);
    sendingBank = BankingSupplier_1(i);

    // K-firms pay for energy; exit if they are unable
    if (Deposits_1(1, i) >= EnergyPayments_1(i))
    {
      Deposits_1(1, i) -= EnergyPayments_1(i);
      EnergyPayments += EnergyPayments_1(i);
      Deposits(1, sendingBank) -= EnergyPayments_1(i);
      Outflows(sendingBank) += EnergyPayments_1(i);
    }
    else if (Deposits_1(1, i) >= 0)
    {
      deviation = ((EnergyPayments_1(i) - Deposits_1(1, i)) / EnergyPayments_1(i));
      EnergyPayments_1(i) = Deposits_1(1, i);
      Deposits_1(1, i) -= EnergyPayments_1(i);
      EnergyPayments += EnergyPayments_1(i);
      Deposits(1, sendingBank) -= EnergyPayments_1(i);
      Outflows(sendingBank) += EnergyPayments_1(i);
      if (deviation > tolerance)
      {
        exiting_1(i) = 1;
      }
    }

    // K-firms pay CO2 tax
    if (exiting_1(i) == 0)
    {
      Taxes_CO2_1(i) = t_CO2 * Emiss1(i);
      if (Deposits_1(1, i) >= Taxes_CO2_1(i))
      {
        Taxes_CO2 += Taxes_CO2_1(i);
        Deposits_1(1, i) -= Taxes_CO2_1(i);
        Deposits(1, sendingBank) -= Taxes_CO2_1(i);
        Outflows(sendingBank) += Taxes_CO2_1(i);
      }
      else if (Deposits_1(1, i) >= 0)
      {
        Taxes_CO2_1(i) = Deposits_1(1, i);
        Deposits_1(1, i) -= Taxes_CO2_1(i);
        Taxes_CO2 += Taxes_CO2_1(i);
        Deposits(1, sendingBank) -= Taxes_CO2_1(i);
        Outflows(sendingBank) += Taxes_CO2_1(i);
      }
    }
    else
    {
      Taxes_CO2_1(i) = 0;
    }

    if (Pi1(i) > 0 && exiting_1(i) == 0)
    {
      // If profit is positive, pay tax
      if (Deposits_1(1, i) >= aliq * Pi1(i))
      {
        Taxes_1(i) = aliq * Pi1(i);
        Deposits_1(1, i) -= Taxes_1(i);
        Deposits(1, sendingBank) -= Taxes_1(i);
        Outflows(sendingBank) += Taxes_1(i);
        Taxes += Taxes_1(i);
      }
      else if (Deposits_1(1, i) >= 0)
      {
        Taxes_1(i) = Deposits_1(1, i);
        Deposits_1(1, i) -= Taxes_1(i);
        Deposits(1, sendingBank) -= Taxes_1(i);
        Outflows(sendingBank) += Taxes_1(i);
        Taxes += Taxes_1(i);
      }

      // Pay dividend
      if (Deposits_1(1, i) >= d1 * Pi1(i))
      {
        Dividends_1(i) = d1 * Pi1(i);
        Deposits_1(1, i) -= Dividends_1(i);
        Deposits(1, sendingBank) -= Dividends_1(i);
        Outflows(sendingBank) += Dividends_1(i);
      }
      else if (Deposits_1(1, i) >= 0)
      {
        Dividends_1(i) = Deposits_1(1, i);
        Deposits_1(1, i) -= Dividends_1(i);
        Deposits(1, sendingBank) -= Dividends_1(i);
        Outflows(sendingBank) += Dividends_1(i);
      }

      Divtot_1 += Dividends_1(i);
      Dividends(1) += Dividends_1(i);
    }
    Pitot1 += Pi1(i);

    if (Deposits_1(1, i) < 0)
    {
      std::cerr << "\n\n ERROR: K-firm " << i << " has negative deposits in period " << t << endl;
      Errors << "\n K-firm " << i << " has negative deposits in period " << t << endl;
      exit(EXIT_FAILURE);
    }

    // Calculate mean deposits of non-failing K-firms for entry below
    if (nclient(i) >= 1 && exiting_1(i) == 0)
    {
      mD1 += Deposits_1(1, i);
      ns1++;
    }
    else
    {
      exiting_1(i) = 1;
    }

    // Prepare failing K-firms for exit
    if (exiting_1(i) == 1)
    {
      Deposits_recovered_1 += Deposits_1(1, i);
      sendingBank = BankingSupplier_1(i);
      baddebt_1(i) = -Deposits_1(1, i);
      Deposits(1, sendingBank) -= Deposits_1(1, i);
      Outflows(sendingBank) += Deposits_1(1, i);
      Deposits_1(1, i) = 0;
    }
  }

  // Households receive K-firm dividends and pay tax
  Taxes_h = aliqw * Wages;
  Deposits_h(1) += (Deposits_recovered_1 + Divtot_1 - Taxes_h);
  Taxes += Taxes_h;

  for (i = 1; i <= NB; i++)
  {
    Deposits(1, i) += (Deposits_recovered_1 + Divtot_1) * DepositShare_h(i);
    Inflows(i) += (Deposits_recovered_1 + Divtot_1) * DepositShare_h(i);
    Deposits_hb(1, i) += (Deposits_recovered_1 + Divtot_1) * DepositShare_h(i);
    Deposits(1, i) -= Taxes_h * DepositShare_h(i);
    Outflows(i) += Taxes_h * DepositShare_h(i);
    Deposits_hb(1, i) -= Taxes_h * DepositShare_h(i);
  }

  // Consumption demand is determined; Check if consumption can be financed
  Cons = (1 - shock_cons) * (a1 * (Wages + Benefits - Taxes_h) + a2 * (InterestDeposits_h + Dividends(2)) + a3 * Deposits_h(2));

  if (Deposits_h(1) < Cons)
  {
    Cons = Deposits_h(1);
  }

  for (j = 1; j <= N2; j++)
  {
    cpi(1) += p2(j) * f2(1, j);
  }

  if (cpi(1) < 0.01)
  {
    std::cerr << "\n\n ERROR: CPI < 0.01 in period " << t << endl;
    Errors << "\n CPI < 0.01 in period " << t << endl;
    exit(EXIT_FAILURE);
  }

  // Consumption takes place
  ALLOC();

  if (Deposits_h(1) < 0)
  {
    std::cerr << "\n\n ERROR: Household deposits negative after consumption in period " << t << endl;
    Errors << "\n  Household deposits negative after consumption in period " << t << endl;
    exit(EXIT_FAILURE);
  }

  // Calculate C-firm profits
  for (j = 1; j <= N2; j++)
  {
    dN(j) = N(1, j) - N(2, j);
    Inventories(1, j) = N(1, j) * p2(j);
    dNm(j) = Inventories(1, j) - Inventories(2, j);
    dNtot += dN(j);
    dNmtot += dNm(j);
    EnergyPayments_2(j) = c_en(1) * D2_en(j);
    mol(j) = S2(1, j) - Wages_2(j) - EnergyPayments_2(j);
    if (exiting_2(j) == 0)
    {
      LoanInterest_2(j) = r_deb_h(j) * Loans_2(1, j);
    }
    else
    {
      LoanInterest_2(j) = 0;
    }

    // NB: Profit here also includes changes in nominal value of tangible assets
    Pi2(j) = S2(1, j) + InterestDeposits_2(j) + dNm(j) + deltaCapitalStock(1, j) - Investment_2(j) - Wages_2(j) - EnergyPayments_2(j) - LoanInterest_2(j) - t_CO2 * Emiss2(j);

    // C-firm  pays energy; if unable to do so it exits
    if (Deposits_2(1, j) >= EnergyPayments_2(j))
    {
      sendingBank = BankingSupplier_2(j);
      Deposits_2(1, j) -= EnergyPayments_2(j);
      EnergyPayments += EnergyPayments_2(j);
      Deposits(1, sendingBank) -= EnergyPayments_2(j);
      Outflows(sendingBank) += EnergyPayments_2(j);
    }
    else if (Deposits_2(1, j) >= 0)
    {
      sendingBank = BankingSupplier_2(j);
      EnergyPayments_2(j) = Deposits_2(1, j);
      Deposits_2(1, j) = 0;
      Deposits(1, sendingBank) -= EnergyPayments_2(j);
      Outflows(sendingBank) += EnergyPayments_2(j);
      EnergyPayments += EnergyPayments_2(j);
      Taxes_2(j) = 0;
      LoanInterest_2(j) = 0;
      DebtRemittances2(j) = 0;
      exiting_2(j) = 1;
      exit_payments2(j) = 1;
      Pi2(j) = S2(1, j) + InterestDeposits_2(j) + dNm(j) + deltaCapitalStock(1, j) - Investment_2(j) - Wages_2(j) - EnergyPayments_2(j) - LoanInterest_2(j) - t_CO2 * Emiss2(j);
    }

    // Non-exiting firms make principal & interest payments on loans; if unable to do so they exit
    if (exiting_2(j) == 0)
    {
      DebtRemittances2(j) = repayment_share * Loans_2(1, j);
      if (Deposits_2(1, j) >= (LoanInterest_2(j) + DebtRemittances2(j)))
      {
        receivingBank = BankingSupplier_2(j);
        Deposits_2(1, j) -= (LoanInterest_2(j) + DebtRemittances2(j));
        Deposits(1, receivingBank) -= (LoanInterest_2(j) + DebtRemittances2(j));
        DebtService_2(1, j) = LoanInterest_2(j) + DebtRemittances2(j);
        Loans_2(1, j) -= DebtRemittances2(j);
        Loans_b(1, receivingBank) -= DebtRemittances2(j);
        LoanInterest(receivingBank) += LoanInterest_2(j);
      }
      else
      {
        LoanInterest_2(j) = 0;
        DebtRemittances2(j) = 0;
        Taxes_2(j) = 0;
        exiting_2(j) = 1;
        exit_payments2(j) = 1;
        Pi2(j) = S2(1, j) + InterestDeposits_2(j) + dNm(j) + deltaCapitalStock(1, j) - Investment_2(j) - Wages_2(j) - EnergyPayments_2(j) - LoanInterest_2(j) - t_CO2 * Emiss2(j);
      }
    }

    // C-firms pay CO2 tax
    if (exiting_2(j) == 0)
    {
      Taxes_CO2_2(j) = t_CO2 * Emiss2(j);
      sendingBank = BankingSupplier_2(j);
      if (Deposits_2(1, j) >= Taxes_CO2_2(j))
      {
        Taxes_CO2 += Taxes_CO2_2(j);
        Deposits_2(1, j) -= Taxes_CO2_2(j);
        Deposits(1, sendingBank) -= Taxes_CO2_2(j);
        Outflows(sendingBank) += Taxes_CO2_2(j);
      }
      else if (Deposits_2(1, j) >= 0)
      {
        Taxes_CO2_2(j) = Deposits_2(1, j);
        Deposits_2(1, j) -= Taxes_CO2_2(j);
        Taxes_CO2 += Taxes_CO2_2(j);
        Deposits(1, sendingBank) -= Taxes_CO2_2(j);
        Outflows(sendingBank) += Taxes_CO2_2(j);
      }
    }
    else
    {
      Taxes_CO2_2(j) = 0;
    }

    // C-firm pays tax
    if (Pi2(j) > 0 && exiting_2(j) == 0)
    {
      Taxes_2(j) = aliq * Pi2(j);
      sendingBank = BankingSupplier_2(j);
      if (Deposits_2(1, j) >= Taxes_2(j))
      {
        Deposits_2(1, j) -= Taxes_2(j);
        Taxes += Taxes_2(j);
        Deposits(1, sendingBank) -= Taxes_2(j);
        Outflows(sendingBank) += Taxes_2(j);
      }
      else if (Deposits_2(1, j) >= 0)
      {
        Taxes_2(j) = Deposits_2(1, j);
        Deposits_2(1, j) = 0;
        Taxes += Taxes_2(j);
        Deposits(1, sendingBank) -= Taxes_2(j);
        Outflows(sendingBank) += Taxes_2(j);
      }
    }

    // Non-exiting firms pay dividends
    if (Pi2(j) > 0 && exiting_2(j) == 0)
    {
      sendingBank = BankingSupplier_2(j);
      if (Deposits_2(1, j) >= d2 * Pi2(j))
      {
        Dividends_2(j) = d2 * Pi2(j);
        Deposits_2(1, j) -= Dividends_2(j);
        Deposits(1, sendingBank) -= Dividends_2(j);
        Outflows(sendingBank) += Dividends_2(j);
      }
      else if (Deposits_2(1, j) >= 0)
      {
        Dividends_2(j) = Deposits_2(1, j);
        Deposits_2(1, j) -= Dividends_2(j);
        Deposits(1, sendingBank) -= Dividends_2(j);
        Outflows(sendingBank) += Dividends_2(j);
      }
      Divtot_2 += Dividends_2(j);
      Dividends(1) += Dividends_2(j);
    }

    Pitot2 += Pi2(j);

    if (Deposits_2(1, j) < 0)
    {
      std::cerr << "\n\n ERROR: C-firm " << j << " has negative deposits in period " << t << endl;
      Errors << "\n C-firm " << j << " has negative deposits in period " << t << endl;
      exit(EXIT_FAILURE);
    }
  }

  // Energy sector receives revenue
  Deposits_e(1) += EnergyPayments;

  for (i = 1; i <= NB; i++)
  {
    Deposits(1, i) += EnergyPayments * DepositShare_e(i);
    Deposits_eb(1, i) += EnergyPayments * DepositShare_e(i);
    Inflows(i) += EnergyPayments * DepositShare_e(i);
  }

  // Profit of energy sector
  ProfitEnergy = EnergyPayments + InterestDeposits_e - Wages_en + (CapitalStock_e(1) - CapitalStock_e(2)) - t_CO2_en * Emiss_en - FuelCost;
  if ((flag_energyshocks == 1 && t > 245) || (flag_energyshocks == 2 && t > 245))
  {
    Taxes_e_shock = aliqe * (EnergyPayments - (EnergyPayments / c_en(1)) * (c_en_preshock));
    Transfer_shock = tre * (EnergyPayments - (EnergyPayments / c_en(1)) * (c_en_preshock));
    if (Deposits_e(1) >= Taxes_e_shock)
    {
      Deposits_e(1) -= Taxes_e_shock;
      for (i = 1; i <= NB; i++)
      {
        Deposits(1, i) -= Taxes_e_shock * DepositShare_e(i);
        Outflows(i) += Taxes_e_shock * DepositShare_e(i);
        Deposits_eb(1, i) -= Taxes_e_shock * DepositShare_e(i);
      }
    }
    else
    {
      std::cerr << "\n\n Energy sector cannot pay excess profit tax in period " << t << endl;
      Errors << "\n Energy sector cannot pay excess profit tax in period " << t << endl;
      exit(EXIT_FAILURE);
    }
  }

  if (Deposits_e(1) >= FuelCost)
  {
    Deposits_e(1) -= FuelCost;
    for (i = 1; i <= NB; i++)
    {
      Deposits(1, i) -= FuelCost * DepositShare_e(i);
      Outflows(i) += FuelCost * DepositShare_e(i);
      Deposits_eb(1, i) -= FuelCost * DepositShare_e(i);
    }
  }
  else
  {
    std::cerr << "\n\n Energy sector cannot pay for fuel in period " << t << endl;
    Errors << "\n Energy sector cannot pay for fuel in period " << t << endl;
    exit(EXIT_FAILURE);
  }

  // Energy sector pays C02 tax and receives subsidy
  Deposits_e(1) += Subsidy_Exp;
  if (Deposits_e(1) >= t_CO2_en * Emiss_en)
  {
    Taxes_CO2_e = t_CO2_en * Emiss_en;
    Taxes_CO2 += Taxes_CO2_e;
  }
  else if (Deposits_e(1) >= 0)
  {
    Taxes_CO2_e = Deposits_e(1);
    Taxes_CO2 += Taxes_CO2_e;
  }
  else
  {
    std::cerr << "\n\n Energy sector has negative deposits in period " << t << endl;
    Errors << "\n Energy sector has negative deposits in period " << t << endl;
    exit(EXIT_FAILURE);
  }
  Deposits_e(1) -= Taxes_CO2_e;
  for (i = 1; i <= NB; i++)
  {
    Deposits(1, i) -= Taxes_CO2_e * DepositShare_e(i);
    Outflows(i) += Taxes_CO2_e * DepositShare_e(i);
    Deposits_eb(1, i) -= Taxes_CO2_e * DepositShare_e(i);
    Deposits(1, i) += Subsidy_Exp * DepositShare_e(i);
    Inflows(i) += Subsidy_Exp * DepositShare_e(i);
    Deposits_eb(1, i) += Subsidy_Exp * DepositShare_e(i);
  }

  // Energy sector dividends
  if ((ProfitEnergy - Taxes_e_shock) > 0)
  {
    if (Deposits_e(1) >= de * (ProfitEnergy - Taxes_e_shock))
    {
      Dividends_e = de * (ProfitEnergy - Taxes_e_shock);
    }
    else if (Deposits_e(1) >= 0)
    {
      Dividends_e = Deposits_e(1);
    }
    else
    {
      std::cerr << "\n\n Energy sector has negative deposits in period " << t << endl;
      Errors << "\n Energy sector has negative deposits in period " << t << endl;
      exit(EXIT_FAILURE);
    }
    Dividends(1) += Dividends_e;
    Deposits_e(1) -= Dividends_e;
    for (i = 1; i <= NB; i++)
    {
      Deposits(1, i) -= Dividends_e * DepositShare_e(i);
      Outflows(i) += Dividends_e * DepositShare_e(i);
      Deposits_eb(1, i) -= Dividends_e * DepositShare_e(i);
    }
  }
  else
  {
    Dividends_e = 0;
  }

  // Fossil fuel agent receives fuel payment
  if ((flag_energyshocks == 3 && t > 245) || (flag_energyshocks == 4 && t > 245))
  {
    Taxes_f_shock = aliqe * (FuelCost - (FuelCost / pf) * pf_preshock);
    Transfer_shock = tre * (FuelCost - (FuelCost / pf) * pf_preshock);
    G = G + Transfer_shock;
  }

  Deposits_fuel(1) += (FuelCost - Taxes_f_shock);
  Deposits_fuel_cb(1) += (FuelCost - Taxes_f_shock);
  TransferFuel = d_f * Deposits_fuel(1);
  Deposits_fuel(1) -= TransferFuel;
  Deposits_fuel_cb(1) -= TransferFuel;

  // Households receive energy sector, C-firm dividend and small transfer from fossil fuel agent
  Deposits_h(1) += (Divtot_2 + Dividends_e + TransferFuel);

  for (i = 1; i <= NB; i++)
  {
    Deposits(1, i) += (Divtot_2 + Dividends_e + TransferFuel) * DepositShare_h(i);
    Deposits_hb(1, i) += (Divtot_2 + Dividends_e + TransferFuel) * DepositShare_h(i);
    Inflows(i) += (Divtot_2 + Dividends_e + TransferFuel) * DepositShare_h(i);
  }

  // C-firms which exit due to negative equity, inability to make payments or low market share are prepared for exit
  for (j = 1; j <= N2; j++)
  {
    NW_2(1, j) = CapitalStock(1, j) + deltaCapitalStock(1, j) + Inventories(1, j) + Deposits_2(1, j) - Loans_2(1, j);
    if (NW_2(1, j) < 0 && exit_payments2(j) == 0 && exiting_2(j) == 0 && exit_marketshare2(j) == 0)
    {
      exit_equity2(j) = 1;
      exiting_2(j) = 1;
    }

    if (exit_payments2(j) == 1 || exit_equity2(j) == 1)
    {
      sendingBank = BankingSupplier_2(j);
      baddebt_2(j) = Loans_2(1, j) - Deposits_2(1, j);
      if (Loans_2(1, j) > Deposits_2(1, j))
      {
        baddebt_b(sendingBank) += (Loans_2(1, j) - Deposits_2(1, j));
        Loans_b(1, sendingBank) = max(0.0, Loans_b(1, sendingBank) - Loans_2(1, j));
        Deposits(1, sendingBank) -= Deposits_2(1, j);
      }
      else
      {
        Deposits_recovered_2 += Deposits_2(1, j) - Loans_2(1, j);
        Outflows(sendingBank) += Deposits_2(1, j) - Loans_2(1, j);
        Loans_b(1, sendingBank) = max(0.0, Loans_b(1, sendingBank) - Loans_2(1, j));
        Deposits(1, sendingBank) -= Deposits_2(1, j);
      }
      Deposits_2(1, j) = 0;
      Loans_2(1, j) = 0;
    }

    if (exit_marketshare2(j) == 1 && exit_payments2(j) == 0 && exit_equity2(j) == 0)
    {
      exiting_2(j) = 1;
      sendingBank = BankingSupplier_2(j);
      baddebt_2(j) = Loans_2(1, j) - Deposits_2(1, j);
      if (Loans_2(1, j) > Deposits_2(1, j))
      {
        baddebt_b(sendingBank) += (Loans_2(1, j) - Deposits_2(1, j));
        Loans_b(1, sendingBank) = max(0.0, Loans_b(1, sendingBank) - Loans_2(1, j));
        Deposits(1, sendingBank) -= Deposits_2(1, j);
      }
      else
      {
        Deposits_recovered_2 += Deposits_2(1, j) - Loans_2(1, j);
        Outflows(sendingBank) += Deposits_2(1, j) - Loans_2(1, j);
        Loans_b(1, sendingBank) = max(0.0, Loans_b(1, sendingBank) - Loans_2(1, j));
        Deposits(1, sendingBank) -= Deposits_2(1, j);
      }
      Deposits_2(1, j) = 0;
      Loans_2(1, j) = 0;
    }

    if (exiting_2(j) == 1 && exit_payments2(j) == 0 && exit_equity2(j) == 0 && exit_marketshare2(j) == 0)
    {
      exit_payments2(j) = 1;
    }
  }

  Deposits_h(1) += Deposits_recovered_2;

  for (i = 1; i <= NB; i++)
  {
    Deposits(1, i) += Deposits_recovered_2 * DepositShare_h(i);
    Deposits_hb(1, i) += Deposits_recovered_2 * DepositShare_h(i);
    Inflows(i) += Deposits_recovered_2 * DepositShare_h(i);
  }

  Errors.close();
}

void ALLOC(void)
{
  n = 1;
  ftot = 0;
  Utilisation = Q2.Sum() / K.Sum();
  Cres = Cons;
  cpi_temp = cpi(1);

  if (flag_outputshocks == 1)
  {
    for (j = 1; j <= N2; j++)
    {
      Q2(j) = (1 - shocks_output2(j)) * Q2(j);
    }
  }

  if (flag_outputshocks == 2)
  {
    loss = Q2.Sum() * shocks_output2(1);
    while (loss > 0)
    {
      ranj = int(ran1(p_seed) * N1 * N2) % N2 + 1;
      if (Q2(ranj) >= loss)
      {
        Q2(ranj) -= loss;
        loss = 0;
      }
      else
      {
        loss -= Q2(ranj);
        Q2(ranj) = 0;
      }
    }
  }

  if (flag_outputshocks == 3)
  {
    loss = Q2.Sum() * shocks_output2(1);
    while (loss > 0)
    {
      ranj = 0;
      while (ranj == 0)
      {
        ranj = int(ran1(p_seed) * N1 * N2) % N2 + 1;
        rnd = ran1(p_seed);
        if (risk_c(ranj) < rnd)
        {
          ranj = 0;
        }
      }
      if (Q2(ranj) >= loss)
      {
        Q2(ranj) -= loss;
        loss = 0;
      }
      else
      {
        loss -= Q2(ranj);
        Q2(ranj) = 0;
      }
    }
  }

  if (flag_inventshocks == 1)
  {
    for (j = 1; j <= N2; j++)
    {
      if (N(2, j) > 0)
      {
        ptemp = Inventories(2, j) / N(2, j);
        Loss_Inventories(j) += shocks_invent(j) * N(2, j) * ptemp;
        N(2, j) = (1 - shocks_invent(j)) * N(2, j);
        Inventories(2, j) -= Loss_Inventories(j);
      }
    }
  }

  if (flag_inventshocks == 2)
  {
    loss = N.Row(2).Sum() * shocks_invent(1);
    while (loss > 0)
    {
      ranj = int(ran1(p_seed) * N1 * N2) % N2 + 1;
      if (N(2, ranj) > 0)
      {
        ptemp = Inventories(2, ranj) / N(2, ranj);
        if (N(2, ranj) >= loss)
        {
          ptemp = Inventories(2, ranj) / N(2, ranj);
          Loss_Inventories(ranj) += loss * ptemp;
          Inventories(2, ranj) -= Loss_Inventories(ranj);
          N(2, ranj) -= loss;
          loss = 0;
        }
        else
        {
          Loss_Inventories(ranj) += Inventories(2, ranj);
          Inventories(2, ranj) = 0;
          loss -= N(2, ranj);
          N(2, ranj) = 0;
        }
      }
    }
  }

  if (flag_inventshocks == 3)
  {
    loss = N.Row(2).Sum() * shocks_invent(1);
    while (loss > 0)
    {
      ranj = 0;
      while (ranj == 0)
      {
        ranj = int(ran1(p_seed) * N1 * N2) % N2 + 1;
        rnd = ran1(p_seed);
        if (risk_c(ranj) < rnd)
        {
          ranj = 0;
        }
      }
      if (N(2, ranj) >= loss && N(2, ranj) > 0)
      {
        ptemp = Inventories(2, ranj) / N(2, ranj);
        Loss_Inventories(ranj) += loss * ptemp;
        Inventories(2, ranj) -= Loss_Inventories(ranj);
        N(2, ranj) -= loss;
        loss = 0;
      }
      else
      {
        Loss_Inventories(ranj) += Inventories(2, ranj);
        Inventories(2, ranj) = 0;
        loss -= N(2, ranj);
        N(2, ranj) = 0;
      }
    }
  }

  for (j = 1; j <= N2; j++)
  {
    f_temp2(j) = f2(1, j);
    ftot(1) += f_temp2(j);
    Q2temp(j) = Q2(j) + N(2, j);
  }

  // Consumption demand is distributed among C-firms based on market shares
  while (Cres >= 1 && ftot(1) > 0)
  {
    Cresbis = Cres;
    for (j = 1; j <= N2; j++)
    {
      if (f_temp2(j) > 0)
      {
        D_temp2(j) = Cres / cpi_temp * f_temp2(j);

        if (n == 1)
        {
          D2(1, j) += D_temp2(j);
        }

        if (D_temp2(j) <= Q2temp(j))
        {
          if (n > 1)
          {
            D2(1, j) += D_temp2(j);
          }
          S2(1, j) += p2(j) * D_temp2(j);
          Cresbis -= D_temp2(j) * p2(j);
          if (n == 1)
          {
            l2(j) = 1;
          }
          Q2temp(j) -= D_temp2(j);
        }
        else
        {
          if (n > 1)
          {
            D2(1, j) += Q2temp(j);
          }
          S2(1, j) += p2(j) * Q2temp(j);
          Cresbis -= Q2temp(j) * p2(j);
          f_temp2(j) = 0;
          if (n == 1)
          {
            l2(j) = 1 + (D_temp2(j) - Q2temp(j));
          }
          Q2temp(j) = 0;
        }
      }
    }
    ftot(1) = f_temp2.Sum();
    f_temp2 /= ftot(1);
    Cres = Cresbis;
    cpi_temp = 0;
    for (j = 1; j <= N2; j++)
    {
      cpi_temp += p2(j) * f_temp2(j);
    }
    n++;
  }

  // Nominal consumption is calculated
  Consumption = S2.Row(1).Sum();
  // This is done to ensure that household deposits do not become negative due to consumption (may happen due to rounding issues when liquidity constraint is binding)
  while (Consumption > Cons)
  {
    for (j = 1; j <= N2; j++)
    {
      if (S2(1, j) > (S2(1, j) / S2.Row(1).Sum() * (Consumption - Cons)))
      {
        S2(1, j) -= (S2(1, j) / S2.Row(1).Sum() * (Consumption - Cons));
      }
    }
    Consumption = S2.Row(1).Sum();
  }

  // Real consumption is calculated
  for (j = 1; j <= N2; j++)
  {
    Consumption_r += S2(1, j) / p2(j);
  }

  // Households pay for consumption
  Deposits_h(1) -= Consumption;

  for (i = 1; i <= NB; i++)
  {
    Deposits(1, i) -= Consumption * DepositShare_h(i);
    Outflows(i) += Consumption * DepositShare_h(i);
    Deposits_hb(1, i) -= Consumption * DepositShare_h(i);
  }

  // C-firms receive revenue & we re-compute the CPI
  cpi(1) = S2.Row(1).Sum() / Consumption_r;
  for (j = 1; j <= N2; j++)
  {
    receivingBank = BankingSupplier_2(j);
    Deposits_2(1, j) += S2(1, j);
    Deposits(1, receivingBank) += S2(1, j);
    Inflows(receivingBank) += S2(1, j);
    N(1, j) = N(2, j) + flag_inventories * (Q2(j) - S2(1, j) / p2(j));
    if (N(1, j) < 0)
    {
      if (fabs(N(1, j)) / Q2.Sum() < tolerance)
      {
        N(1, j) = 0;
      }
      else
      {
        std::cerr << "\n\n ERROR: Inventories of C-firm " << j << " are negative in period " << t << endl;
        Errors << "\n Inventories of C-firm " << j << " are negative in period " << t << endl;
        exit(EXIT_FAILURE);
      }
    }
  }

  S1_temp.Row(1) = S1;
  S2_temp.Row(1) = S2.Row(1);
}

void ENTRYEXIT(void)
{
  ofstream Errors(errorfilename, ios::app);

  // Save sales as S1 & S2 are reset during entry

  Sales1 = S1;
  Sales2 = S2.Row(1);

  for (i = 1; i <= N1; i++)
  {
    if (Deposits_1(1, i) < 0)
    {
      std::cerr << "\n\n ERROR: K-firm " << i << " has negative deposits in period " << t << endl;
      Errors << "\n  K-firm " << i << " has negative deposits in period " << t << endl;
      exit(EXIT_FAILURE);
    }
  }

  if (ns1 > 0)
  {
    mD1 /= ns1;
  }
  else
  {
    mD1 = Deposits_1.Row(2).Sum() / N1r;
  }

  // Exiting K-firms lose customers; K-firm to be copied is chosen
  for (i = 1; i <= N1; i++)
  {
    if (exiting_1(i) == 1)
    {
      A1p_en_dead += A1p_en(i);
      flag = 0;
      for (j = 1; j <= N2; j++)
      {
        Match(j, i) = 0;
        if (fornit(j) == i)
        {
          fornit(j) = 0;
        }
      }
      if (exiting_1.Sum() < N1r)
      {
        while (flag == 0)
        {
          rni = int(ran1(p_seed) * N1 * N2) % N1 + 1;
          if (exiting_1(rni) == 0)
          {
            ee1(i) = rni;
            flag = 1;
          }
        }
      }
      else
      {
        ee1(i) = i;
      }
    }
    else
    {
      A1p_en_survive += A1p_en(i);
    }
  }

  A1p_en_survive = A1p_en_survive / (N1r - exiting_1.Sum());
  if (exiting_1.Sum() > 0)
  {
    A1p_en_dead = A1p_en_dead / exiting_1.Sum();
  }
  else
  {
    A1p_en_dead = A1p_en_survive;
  }

  for (i = 1; i <= N1; i++)
  {
    if (exiting_1(i) == 1)
    {
      // For entering K-firms, most variables are copied from a surviving K-firm as in the original DSK
      iii = int(ee1(i));
      f1(1, i) = 0;
      f1(2, i) = 0;
      Ld1rd(i) = 0;
      A1(i) = A1(iii);
      shocks_labprod1(i) = shocks_labprod1(iii);
      shocks_eneff1(i) = shocks_eneff1(iii);
      p1(i) = p1(iii);
      A1p(i) = A1p(iii);
      A(t, i) = A1(iii);
      A1_ef(i) = A1_ef(iii);
      A1p_ef(i) = A1p_ef(iii);
      A1_en(i) = A1_en(iii);
      A1p_en(i) = A1p_en(iii);

      // Entering K-firms' deposits, however, are received as a transfer from households
      receivingBank = BankingSupplier_1(i);
      multip_entry = ran1(p_seed);
      multip_entry = w1inf + multip_entry * (w1sup - w1inf);
      if (Deposits_h(1) >= multip_entry * mD1)
      {
        // If households have sufficient deposits, transfer is equal to mean deposits of surviving firms
        injection = multip_entry * mD1;
        Deposits_h(1) -= injection;
        FirmTransfers += injection;
        FirmTransfers_1 += injection;
        Injection_1(i) = injection;
        for (j = 1; j <= NB; j++)
        {
          Deposits_hb(1, j) -= DepositShare_h(j) * injection;
          Deposits(1, j) -= DepositShare_h(j) * injection;
          Outflows(j) += DepositShare_h(j) * injection;
        }
        Deposits_1(1, i) += injection;
        Deposits(1, receivingBank) += injection;
        Inflows(receivingBank) += injection;
      }
      else
      {
        // If households cannot finance K-firm entry, this is either financed by gov. (flag_entry=0) or banks simply create the deposits (without corresponding loan) and book this as a loss (flag_entry=1)
        std::cerr << "\n\n Households cannot finance K-firm entry in period " << t << endl;
        Errors << "\n Households cannot finance K-firm entry in period " << t << endl;
        injection2 = 0;
        if (Deposits_h(1) > 0)
        {
          injection2 = Deposits_h(1);
          Deposits_h(1) -= injection2;
          FirmTransfers += injection2;
          FirmTransfers_1 += injection2;
          Injection_1(i) = injection2;
          for (j = 1; j <= NB; j++)
          {
            Deposits(1, j) -= Deposits_hb(1, j);
            Outflows(j) += Deposits_hb(1, j);
            Deposits_hb(1, j) = 0;
            DepositShare_h(j) = (NL_1(j) + NL_2(j)) / (N1 + N2);
          }
          Deposits_1(1, i) += injection2;
          Deposits(1, receivingBank) += injection2;
          Inflows(receivingBank) += injection2;
        }

        if (flag_entry == 1)
        {
          injection = multip_entry * mD1 - injection2;
          LossEntry_b(receivingBank) += injection;
          Injection_1(i) += injection;
          Deposits_1(1, i) += injection;
          Deposits(1, receivingBank) += injection;
        }
        else
        {
          injection = multip_entry * mD1 - injection2;
          EntryCosts += injection;
          FirmTransfers_1 += injection;
          Injection_1(i) += injection;
          Deposits_1(1, i) += injection;
          Deposits(1, receivingBank) += injection;
          Inflows(receivingBank) += injection;
        }
      }

      if (NR > 0)
      {
        double draw_region = ran1(p_seed);
        double cumprob_region = 0;
        int chosen_region = NR;
        for (int rr = 1; rr <= NR; ++rr)
        {
          cumprob_region += relocation_probability_K(rr);
          if (draw_region <= cumprob_region)
          {
            chosen_region = rr;
            break;
          }
        }
        region_firm_assignment_K[i - 1] = chosen_region;
      }

      S1(i) = p1(i) * step;
      stepbis = step;
      while (stepbis > 0)
      {
        rni = int(ran1(p_seed) * N1 * N2) % N2 + 1;
        if (Match(rni, i) == 0)
        {
          Match(rni, i) = 1;
          stepbis--;
        }
      }
    }
  }

  for (j = 1; j <= N2; j++)
  {
    if (exiting_2(j) == 1)
    {
      f2_exit += f2(1, j);
      A2_en_dead += A2_en(j);
    }
    else
    {
      A2_en_survive += A2_en(j);
    }
  }

  A2_en_survive = A2_en_survive / (N2r - exiting_2.Sum());
  if (exiting_2.Sum() > 0)
  {
    A2_en_dead = A2_en_dead / exiting_2.Sum();
  }
  else
  {
    A2_en_dead = A2_en_survive;
  }

  if (exiting_2.Sum() > 0)
  {
    CurrentDemand = D2.Row(1).Sum();
    if (f2_exit > 0)
    {
      n_mach_needed = max(exiting_2.Sum(), ceil(f2_exit * CurrentDemand / dim_mach / u));
    }
    else
    {
      f2_exit = f2_entry_min * exiting_2.Sum();
      n_mach_needed = max(exiting_2.Sum(), ceil(f2_exit * CurrentDemand / dim_mach / u));
    }
  }

  for (j = 1; j <= N2; j++)
  {
    if (exiting_2(j) == 1)
    {
      for (i = 1; i <= N1; i++)
      {
        for (tt = t0; tt <= t; tt++)
        {
          if (gtemp[tt - 1][i - 1][j - 1] > 0)
          {
            n_mach_exit += gtemp[tt - 1][i - 1][j - 1];
          }
        }
      }
    }
  }

  for (j = 1; j <= N2; j++)
  {
    if (exiting_2(j) == 1)
    {
      for (i = 1; i <= N1; i++)
      {
        for (tt = t; tt >= t0; tt--)
        {
          if (gtemp[tt - 1][i - 1][j - 1] > 0)
          {
            C_secondhand(tt, i) = C(tt, i);
          }
        }
      }
    }
    else
    {
      // Calculate mean deposits of surviving C-firms
      mD2 += Deposits_2(1, j);
      ns2++;
    }
  }

  // The capital stock of exiting C-firms is transferred to their respective banks up to the value of bad debt; the value of second-hand capital is marked down depending on age of the machines
  n_mach_exit2 = min(n_mach_needed, n_mach_exit);
  while (n_mach_exit2 > 0)
  {
    for (j = 1; j <= N2; j++)
    {
      if (exiting_2(j) == 1)
      {
        receivingBank = BankingSupplier_2(j);
        baddebt_2_temp = baddebt_2(j);
        for (i = 1; i <= N1; i++)
        {
          for (tt = t; tt >= t0; tt--)
          {
            if (gtemp[tt - 1][i - 1][j - 1] > 0 && n_mach_exit2 > 0 && C(tt, i) <= C_secondhand.Minimum())
            {
              markdownCapital = max(0.0, (1 - (double)age[tt - 1][i - 1][j - 1] / (agemax)));
              g_secondhand[tt - 1][i - 1] += min(n_mach_exit2, gtemp[tt - 1][i - 1][j - 1]);
              age_secondhand[tt - 1][i - 1] = age[tt - 1][i - 1][j - 1];
              g_secondhand_p[tt - 1][i - 1] = markdownCapital * g_price[tt - 1][i - 1][j - 1];
              if (baddebt_2_temp > 0)
              {
                capitalRecovered(receivingBank) += min(n_mach_exit2, gtemp[tt - 1][i - 1][j - 1]) * g_secondhand_p[tt - 1][i - 1];
                baddebt_2_temp -= min(n_mach_exit2, gtemp[tt - 1][i - 1][j - 1]) * g_secondhand_p[tt - 1][i - 1];
              }
              n_mach_exit2 -= min(n_mach_exit2, gtemp[tt - 1][i - 1][j - 1]);
              C_secondhand(tt, i) = 1000000;
            }
          }
        }
      }
    }
  }

  if (ns2 > 0)
  {
    mD2 /= ns2;
  }
  else
  {
    std::cerr << "\n\n ERROR: All C-firms are exiting in period " << t << endl;
    Errors << "\n All C-firms are exiting in period " << t << endl;
    exit(EXIT_FAILURE);
  }

  // K-firms lose exiting C-firms as customers
  for (j = 1; j <= N2; j++)
  {
    if (exiting_2(j) == 1)
    {
      flag = 0;
      indforn = int(fornit(j));
      if (indforn >= 1)
      {
        Match(j, fornit(j)) = 0;
        fornit(j) = 0;
      }
    }
  }

  // Determine the number of machines which each entering C-firm will have based on number of available second-hand machines
  n_exit2 = exiting_2.Sum();
  n_mach_resid = min(n_mach_needed, n_mach_exit);
  if (n_exit2 > n_mach_resid)
  {
    std::cerr << "\n\n ERROR: Not enough second hand capital in period " << t << endl;
    Errors << "\n Not enough second hand capital in period " << t << endl;
    exit(EXIT_FAILURE);
  }

  for (j = 1; j <= N2; j++)
  {
    if (exiting_2(j) == 1)
    {
      n_mach_entry(j) = 1;
      n_mach_resid--;
      k_entry(j) = ran1(p_seed);
    }
  }

  n_mach_resid2 = n_mach_resid;

  for (j = 1; j <= N2; j++)
  {
    if (exiting_2(j) == 1 && n_mach_resid > 0)
    {
      n_mach_entry(j) += floor(k_entry(j) / k_entry.Sum() * n_mach_resid2);
      n_mach_resid -= floor(k_entry(j) / k_entry.Sum() * n_mach_resid2);
    }
  }

  if (n_mach_resid < 0 && n_mach_exit < n_mach_needed)
  {
    std::cerr << "\n\n ERROR: Remaining second hand machines are negative in period " << t << endl;
    Errors << "\n Remaining second hand machines are negative " << t << endl;
    exit(EXIT_FAILURE);
  }

  while (n_mach_resid > 0)
  {
    rni = int(ran1(p_seed) * N1 * N2) % N2 + 1;
    if (exiting_2(rni) == 1 && n_mach_resid > 0)
    {
      n_mach_entry(rni)++;
      n_mach_resid--;
    }
  }

  // Second-hand capital is purchased by households; below will be transferred to newly entering firms
  if (Deposits_h(1) >= (capitalRecovered.Sum()))
  {
    Deposits_h(1) -= capitalRecovered.Sum();
    FirmTransfers += capitalRecovered.Sum();
    FirmTransfers_2 += capitalRecovered.Sum();
    for (i = 1; i <= NB; i++)
    {
      Deposits_hb(1, i) -= DepositShare_h(i) * capitalRecovered.Sum();
      Deposits(1, i) -= DepositShare_h(i) * capitalRecovered.Sum();
      if (capitalRecovered(i) >= DepositShare_h(i) * capitalRecovered.Sum())
      {
        Inflows(i) += (capitalRecovered(i) - DepositShare_h(i) * capitalRecovered.Sum());
      }
      else
      {
        Outflows(i) += (DepositShare_h(i) * capitalRecovered.Sum() - capitalRecovered(i));
      }
    }
  }
  else if (Deposits_h(1) >= 0)
  {
    // Household deposits are insufficient to purchase second hand capital; households buy as much as they can, the rest is financed either by government (flag_entry=0) or booked as a loss by banks (flag_entry=1)
    std::cerr << "\n\n Households cannot purchase second-hand capital in period " << t << endl;
    Errors << "\n Households cannot purchase second-hand capital in period " << t << endl;
    FirmTransfers += Deposits_h(1);
    FirmTransfers_2 += Deposits_h(1);
    capitalRecovered2 = capitalRecovered;
    for (i = 1; i <= NB; i++)
    {
      if (capitalRecovered2(i) >= Deposits_hb(1, i))
      {
        capitalRecovered2(i) -= Deposits_hb(1, i);
        Deposits(1, i) -= Deposits_hb(1, i);
        Deposits_h(1) -= Deposits_hb(1, i);
        Deposits_hb(1, i) = 0;
      }
      else
      {
        Deposits(1, i) -= Deposits_hb(1, i);
        Deposits_hb(1, i) -= capitalRecovered2(i);
        Deposits_h(1) -= capitalRecovered2(i);
        capitalRecovered2(i) = 0;
        Outflows(i) += Deposits_hb(1, i);
        Deposits_hb(1, i) = 0;
      }
      DepositShare_h(i) = (NL_1(i) + NL_2(i)) / (N1 + N2);
    }

    for (i = 1; i <= NB; i++)
    {
      capitalRecoveredShare(i) = capitalRecovered2(i) / capitalRecovered2.Sum();
    }

    for (i = 1; i <= NB; i++)
    {
      capitalRecovered2(i) -= (capitalRecoveredShare(i) * Deposits_h(1));
      Inflows(i) += (capitalRecoveredShare(i) * Deposits_h(1));
    }

    if (flag_entry == 1)
    {
      for (i = 1; i <= NB; i++)
      {
        LossEntry_b(i) += capitalRecovered2(i);
      }
    }
    else
    {
      EntryCosts += capitalRecovered2.Sum();
      BankTransfer += capitalRecovered2.Sum();
      for (i = 1; i <= NB; i++)
      {
        Inflows(i) += capitalRecovered2(i);
      }
    }
    Deposits_h(1) = 0;
  }
  else
  {
    std::cerr << "\n\n ERROR: Household deposits are negative in period " << t << endl;
    Errors << "\n Household deposits are negative in period " << t << endl;
    exit(EXIT_FAILURE);
  }

  for (j = 1; j <= N2; j++)
  {
    if (exiting_2(j) == 1)
    {
      N(1, j) = 0;
      N(2, j) = 0;
      // Any inventories of exiting firms are destroyed and hence booked as a loss
      Injection_2(j) = -Inventories(1, j);
      Inventories(1, j) = 0;
      Inventories(2, j) = 0;
      // Households give entering C-firms a transfer of deposits
      receivingBank = BankingSupplier_2(j);
      multip_entry = ran1(p_seed);
      multip_entry = w2inf + multip_entry * (w2sup - w2inf);
      if (Deposits_h(1) >= multip_entry * mD2)
      {
        injection = multip_entry * mD2;
        Deposits_h(1) -= injection;
        FirmTransfers += injection;
        FirmTransfers_2 += injection;
        Injection_2(j) += injection;
        for (i = 1; i <= NB; i++)
        {
          Deposits_hb(1, i) -= DepositShare_h(i) * injection;
          Deposits(1, i) -= DepositShare_h(i) * injection;
          Outflows(i) += DepositShare_h(i) * injection;
        }
        Deposits_2(1, j) = injection;
        Deposits(1, receivingBank) += injection;
        Inflows(receivingBank) += injection;
      }
      else
      {
        // If households cannot finance C-firm entry, this is done by government or banks as in the case of K-firms
        std::cerr << "\n\n Households cannot finance C-firm entry in period " << t << endl;
        Errors << "\n Households cannot finance C-firm entry in period " << t << endl;

        injection2 = 0;
        if (Deposits_h(1) > 0)
        {
          injection2 = Deposits_h(1);
          Deposits_h(1) -= injection2;
          FirmTransfers += injection2;
          FirmTransfers_2 += injection2;
          Injection_2(j) += injection2;
          for (i = 1; i <= NB; i++)
          {
            Deposits(1, i) -= Deposits_hb(1, i);
            Outflows(i) += Deposits_hb(1, i);
            Deposits_hb(1, i) = 0;
            if (NR > 0)
            {
              DepositShare_h(i) = (NL_1(i) + NL_2(i)) / (N1 + N2);
            }
          }
          Deposits_2(1, j) = injection2;
          Deposits(1, receivingBank) += injection2;
          Inflows(receivingBank) += injection2;
        }

        if (flag_entry == 1)
        {
          injection = multip_entry * mD2 - injection2;
          LossEntry_b(receivingBank) += injection;
          Injection_2(j) += injection;
          Deposits_2(1, j) += injection;
          Deposits(1, receivingBank) += injection;
        }
        else
        {
          injection = multip_entry * mD2 - injection2;
          EntryCosts += injection;
          FirmTransfers_2 += injection;
          Injection_2(j) += injection;
          Deposits_2(1, j) += injection;
          Deposits(1, receivingBank) += injection;
          Inflows(receivingBank) += injection;
        }
      }

      if (NR > 0)
      {
        double draw_region_c = ran1(p_seed);
        double cumprob_region_c = 0;
        int chosen_region_c = NR;
        for (int rr = 1; rr <= NR; ++rr)
        {
          cumprob_region_c += relocation_probability_C(rr);
          if (draw_region_c <= cumprob_region_c)
          {
            chosen_region_c = rr;
            break;
          }
        }
        region_firm_assignment_C[j - 1] = chosen_region_c;
      }

      n_mach(j) = 0;
      K(j) = 0;
      // First subtract the capital stock previously held by the exiting firm
      Injection_2(j) -= (CapitalStock(1, j) + deltaCapitalStock(1, j));
      CapitalStock(1, j) = 0;
      // Clear the exiting firms' entries in the frequency arrays
      n_mach_resid = n_mach_entry(j);
      for (i = 1; i <= N1; i++)
      {
        for (tt = t0; tt <= t; tt++)
        {
          g[tt - 1][i - 1][j - 1] = 0;
          gtemp[tt - 1][i - 1][j - 1] = 0;
          g_c[tt - 1][i - 1][j - 1] = 0;
          g_c2[tt - 1][i - 1][j - 1] = 0;
          g_c3[tt - 1][i - 1][j - 1] = 0;
          age[tt - 1][i - 1][j - 1] = 0;
        }
      }

      // Give the entering firm an initial capital stock from the pool of second-hand capital
      while (n_mach_resid > 0)
      {
        rni = int(ran1(p_seed) * N1 * N2) % N1 + 1;
        for (tt = t0; tt <= t; tt++)
        {
          if (g_secondhand[tt - 1][rni - 1] > 0)
          {
            if (g_secondhand[tt - 1][rni - 1] >= n_mach_resid)
            {
              g[tt - 1][rni - 1][j - 1] += n_mach_resid;
              gtemp[tt - 1][rni - 1][j - 1] += n_mach_resid;
              g_c[tt - 1][rni - 1][j - 1] += n_mach_resid;
              g_c2[tt - 1][rni - 1][j - 1] += n_mach_resid;
              g_c3[tt - 1][rni - 1][j - 1] += n_mach_resid;
              age[tt - 1][rni - 1][j - 1] = age_secondhand[tt - 1][rni - 1];
              n_mach(j) += n_mach_resid;
              CapitalStock(1, j) += n_mach_resid * g_secondhand_p[tt - 1][rni - 1];
              g_price[tt - 1][rni - 1][j - 1] = g_secondhand_p[tt - 1][rni - 1];
              K(j) += n_mach_resid * dim_mach;
              g_secondhand[tt - 1][rni - 1] -= n_mach_resid;
              n_mach_resid = 0;
            }
            else
            {
              g[tt - 1][rni - 1][j - 1] += g_secondhand[tt - 1][rni - 1];
              gtemp[tt - 1][rni - 1][j - 1] += g_secondhand[tt - 1][rni - 1];
              g_c[tt - 1][rni - 1][j - 1] += g_secondhand[tt - 1][rni - 1];
              g_c2[tt - 1][rni - 1][j - 1] += g_secondhand[tt - 1][rni - 1];
              g_c3[tt - 1][rni - 1][j - 1] += g_secondhand[tt - 1][rni - 1];
              age[tt - 1][rni - 1][j - 1] = age_secondhand[tt - 1][rni - 1];
              n_mach(j) += g_secondhand[tt - 1][rni - 1];
              CapitalStock(1, j) += g_secondhand[tt - 1][rni - 1] * g_secondhand_p[tt - 1][rni - 1];
              g_price[tt - 1][rni - 1][j - 1] = g_secondhand_p[tt - 1][rni - 1];
              K(j) += g_secondhand[tt - 1][rni - 1] * dim_mach;
              n_mach_resid -= g_secondhand[tt - 1][rni - 1];
              g_secondhand[tt - 1][rni - 1] = 0;
            }
          }
        }
      }

      // New capital stock is added to the net worth injection
      Injection_2(j) += CapitalStock(1, j);
      rni = int(ran1(p_seed) * N1 * N2) % N1 + 1;
      fornit(j) = rni;
      Match(j, rni) = 1;
      EI(1, j) = 0;
      scrap_age(j) = 0;
      deltaCapitalStock(1, j) = 0;
      // Set the newly entering firm's cost, mark-up and price
      c2(j) = 0;
      for (i = 1; i <= N1; i++)
      {
        for (tt = t0; tt <= t; tt++)
        {
          c2(j) += (w(2) / ((1 - shocks_labprod2(j)) * A(tt, i)) + c_en(2) / ((1 - shocks_eneff2(j)) * A_en(tt, i)) + t_CO2 * A_ef(tt, i) / ((1 - shocks_eneff2(j)) * A_en(tt, i))) * g[tt - 1][i - 1][j - 1] / n_mach(j);
        }
      }
      mu2(1, j) = mi2;
      p2(j) = (1 + mu2(1, j)) * c2(j);
      p2_entry += p2(j);
      DebtService_2(1, j) = 0;
    }
  }

  if (exiting_2.Sum() > 0)
  {
    CurrentDemand = D2.Row(1).Sum();
    if (f2_exit > 0)
    {
      p2_entry /= exiting_2.Sum();
      for (j = 1; j <= N2; j++)
      {
        if (exiting_2(j) == 1)
        {
          CompEntry(j) = -p2(j) / p2_entry;
        }
      }
      CompEntry_m = CompEntry.Sum() / exiting_2.Sum();
      for (j = 1; j <= N2; j++)
      {
        if (exiting_2(j) == 1)
        {
          EntryShare(j) = max(f2_entry_min, 1 / exiting_2.Sum() * (1 + chi * (CompEntry(j) - CompEntry_m) / CompEntry_m));
        }
      }
      EntryShare = EntryShare / EntryShare.Sum();
      for (j = 1; j <= N2; j++)
      {
        if (exiting_2(j) == 1)
        {
          f2(1, j) = EntryShare(j) * f2_exit;
          f2(2, j) = f2(1, j);
          f2(3, j) = f2(1, j);
          D2(1, j) = min(K(j), f2(1, j) * CurrentDemand);
          l2(j) = 1 + (f2(1, j) * CurrentDemand - D2(1, j));
          De(j) = D2(1, j);
          S2(1, j) = p2(j) * D2(1, j);
          mol(j) = S2(1, j) - D2(1, j) * c2(j);
          if (mol(j) < 0)
          {
            mol(j) = 0;
          }
        }
      }
    }
    else
    {
      f2_exit = f2_entry_min * exiting_2.Sum();
      p2_entry /= exiting_2.Sum();
      for (j = 1; j <= N2; j++)
      {
        if (exiting_2(j) == 1)
        {
          CompEntry(j) = -p2(j) / p2_entry;
        }
      }
      CompEntry_m = CompEntry.Sum() / exiting_2.Sum();
      for (j = 1; j <= N2; j++)
      {
        if (exiting_2(j) == 1)
        {
          EntryShare(j) = max(f2_entry_min, 1 / exiting_2.Sum() * (1 + chi * (CompEntry(j) - CompEntry_m) / CompEntry_m));
        }
      }
      EntryShare = EntryShare / EntryShare.Sum();
      for (j = 1; j <= N2; j++)
      {
        if (exiting_2(j) == 1)
        {
          f2(1, j) = EntryShare(j) * f2_exit;
          f2(2, j) = f2(1, j);
          f2(3, j) = f2(1, j);
          D2(1, j) = min(K(j), f2(1, j) * CurrentDemand);
          l2(j) = 1 + (f2(1, j) * CurrentDemand - D2(1, j));
          De(j) = D2(1, j);
          S2(1, j) = p2(j) * D2(1, j);
          mol(j) = S2(1, j) - D2(1, j) * c2(j);
          if (mol(j) < 0)
          {
            mol(j) = 0;
          }
        }
      }
    }
  }

  ftot(1) = f2.Row(1).Sum();
  ftot(2) = f2.Row(2).Sum();
  ftot(3) = f2.Row(3).Sum();
  for (j = 1; j <= N2; j++)
  {
    f2(1, j) /= ftot(1);
    f2(2, j) /= ftot(2);
    f2(3, j) /= ftot(3);
  }

  // Update C-firm K-firm network
  nclient = 0;
  for (i = 1; i <= N1; i++)
  {
    for (j = 1; j <= N2; j++)
    {
      nclient(i) += Match(j, i);
    }
  }
  for (i = 1; i <= N1; i++)
  {
    if (nclient(i) == 0)
    {
      stepbis = step;
      while (stepbis > 0)
      {
        rni = int(ran1(p_seed) * N1 * N2) % N2 + 1;
        if (Match(rni, i) == 0)
        {
          Match(rni, i) = 1;
          stepbis--;
        }
      }
    }
  }

  nclient = 0;
  for (i = 1; i <= N1; i++)
  {
    for (j = 1; j <= N2; j++)
    {
      nclient(i) += Match(j, i);
    }
  }

  Errors.close();
}

void TECHANGEND(void)
{
  ofstream Errors(errorfilename, ios::app);

  // Endogenous technological change
  Inn = 0;
  Imm = 0;

  A1inn = 0.00001;
  A1pinn = 0.00001;
  A1imm = 0.00001;
  A1pimm = 0.00001;

  EE_inn = 0.00001;
  EEp_inn = 0.00001;
  EE_imm = 0.00001;
  EEp_imm = 0.00001;

  EF_inn = 100000;
  EFp_inn = 100000;
  EF_imm = 100000;
  EFp_imm = 100000;

  A1_en_max = 0;
  A1p_en_max = 0;
  A1_ef_max = 100000;
  A1p_ef_max = 100000;

  for (i = 1; i <= N1; i++)
  {
    // K-firms determine R&D spending and associated labour demand
    RD(1, i) = nu * S1(i);
    if (S1(i) == 0)
    {
      RD(1, i) = RD(2, i);
      if (nclient(i) < 1)
      {
        std::cerr << "\n\n ERROR: nclient < 1 for K-firm " << i << " in period " << t << endl;
        Errors << "\n nclient < 1 for K-firm " << i << " in period " << t << endl;
        exit(EXIT_FAILURE);
      }
    }

    if (w(1) > 0)
    {
      Ld1rd(i) = RD(1, i) / w(1);
    }
    else
    {
      std::cerr << "\n\n ERROR: w=0 in period " << t << endl;
      Errors << "\n w=0 in period " << t << endl;
      exit(EXIT_FAILURE);
    }

    // Divide between innovation and imitation
    RDin(i) = Ld1rd(i) * xi;
    RDim(i) = Ld1rd(i) * (1 - xi);
    // If a shock to resources devoted to R&D applies, reduce them correspondingly
    if (flag_RDshocks == 4)
    {
      RDin(i) = RDin(i) * (1 - shocks_rd(i));
      RDim(i) = RDim(i) * (1 - shocks_rd(i));
    }

    // Determine whether firm innovates and/or imitates (also taking into account possible shocks to R&D effectiveness)
    parber = 1 - exp(-o1 * RDin(i));
    if (flag_RDshocks == 3)
    {
      parber = (1 - shocks_rd(i)) * parber;
    }
    Inn(i) = bnldev(parber, 1, p_seed);

    parber = 1 - exp(-o2 * RDim(i));
    if (flag_RDshocks == 3)
    {
      parber = (1 - shocks_rd(i)) * parber;
    }
    Imm(i) = bnldev(parber, 1, p_seed);

    if (Inn(i) == 1)
    {
      // If firm innovates, determine characteristics of new technology, possibly taking into account shocks
      // Labour productivity
      if (flag_RDshocks == 1)
      {
        b_a1_shock = (1 - shocks_rd(i)) * b_a11;
        if (b_a1_shock <= 0)
        {
          b_a1_shock = 0.0001;
        }
        rnd = betadev(b_a1_shock, b_b11, p_seed);
      }
      else
      {
        rnd = betadev(b_a11, b_b11, p_seed);
      }
      if (flag_RDshocks == 2)
      {
        rnd = (uu11 * (1 + shocks_rd(i))) + rnd * ((uu21 * (1 - shocks_rd(i))) - (uu11 * (1 + shocks_rd(i))));
      }
      else
      {
        rnd = uu11 + rnd * (uu21 - uu11);
      }
      A1inn(i) = A1(i) * (1 + rnd);

      if (flag_RDshocks == 1)
      {
        b_a1_shock = (1 - shocks_rd(i)) * b_a12;
        if (b_a1_shock <= 0)
        {
          b_a1_shock = 0.0001;
        }
        rnd = betadev(b_a1_shock, b_b12, p_seed);
      }
      else
      {
        rnd = betadev(b_a12, b_b12, p_seed);
      }
      if (flag_RDshocks == 2)
      {
        rnd = (uu12 * (1 + shocks_rd(i))) + rnd * ((uu22 * (1 - shocks_rd(i))) - (uu12 * (1 + shocks_rd(i))));
      }
      else
      {
        rnd = uu12 + rnd * (uu22 - uu12);
      }
      A1pinn(i) = A1p(i) * (1 + rnd);

      // Energy efficiency
      if (flag_RDshocks == 1)
      {
        b_a2_shock = (1 - shocks_rd(i)) * b_a2;
        if (b_a2_shock <= 0)
        {
          b_a2_shock = 0.0001;
        }
        rnd = betadev(b_a2_shock, b_b2, p_seed);
      }
      else
      {
        rnd = betadev(b_a2, b_b2, p_seed);
      }
      if (flag_RDshocks == 2)
      {
        rnd = (uu31 * (1 + shocks_rd(i))) + rnd * ((uu41 * (1 - shocks_rd(i))) - (uu31 * (1 + shocks_rd(i))));
      }
      else
      {
        rnd = uu31 + rnd * (uu41 - uu31);
      }
      EE_inn(i) = A1_en(i) * (1 + rnd);

      if (flag_RDshocks == 1)
      {
        b_a2_shock = (1 - shocks_rd(i)) * b_a2;
        if (b_a2_shock <= 0)
        {
          b_a2_shock = 0.0001;
        }
        rnd = betadev(b_a2_shock, b_b2, p_seed);
      }
      else
      {
        rnd = betadev(b_a2, b_b2, p_seed);
      }
      if (flag_RDshocks == 2)
      {
        rnd = (uu32 * (1 + shocks_rd(i))) + rnd * ((uu42 * (1 - shocks_rd(i))) - (uu32 * (1 + shocks_rd(i))));
      }
      else
      {
        rnd = uu32 + rnd * (uu42 - uu32);
      }
      EEp_inn(i) = A1p_en(i) * (1 + rnd);

      // Environmental friendliness
      if (flag_RDshocks == 1)
      {
        b_a3_shock = (1 - shocks_rd(i)) * b_a3;
        if (b_a3_shock <= 0)
        {
          b_a3_shock = 0.0001;
        }
        rnd = betadev(b_a3_shock, b_b3, p_seed);
      }
      else
      {
        rnd = betadev(b_a3, b_b3, p_seed);
      }

      if (flag_RDshocks == 2)
      {
        rnd = (uu51 * (1 + shocks_rd(i))) + rnd * ((uu61 * (1 - shocks_rd(i))) - (uu51 * (1 + shocks_rd(i))));
      }
      else
      {
        rnd = uu51 + rnd * (uu61 - uu51);
      }
      EF_inn(i) = A1_ef(i) * (1 - rnd);

      if (flag_RDshocks == 1)
      {
        b_a3_shock = (1 - shocks_rd(i)) * b_a3;
        if (b_a3_shock <= 0)
        {
          b_a3_shock = 0.0001;
        }
        rnd = betadev(b_a3_shock, b_b3, p_seed);
      }
      else
      {
        rnd = betadev(b_a3, b_b3, p_seed);
      }
      if (flag_RDshocks == 2)
      {
        rnd = (uu52 * (1 + shocks_rd(i))) + rnd * ((uu62 * (1 - shocks_rd(i))) - (uu52 * (1 + shocks_rd(i))));
      }
      else
      {
        rnd = uu52 + rnd * (uu62 - uu52);
      }
      EFp_inn(i) = A1p_ef(i) * (1 - rnd);

      if (A1pinn(i) == 0 || A1inn(i) == 0 || A1p(i) == 0 || A1(i) == 0)
      {
        std::cerr << "\n\n ERROR: A1=0 for K-firm " << i << " in period " << t << endl;
        Errors << "\n A1=0 for K-firm " << i << " in period " << t << endl;
        exit(EXIT_FAILURE);
      }
      if (EEp_inn(i) == 0 || EE_inn(i) == 0 || A1p_en(i) == 0 || A1_en(i) == 0)
      {
        std::cerr << "\n\n ERROR: A1_en=0 for K-firm " << i << " in period " << t << endl;
        Errors << "\n A1_en=0 for K-firm " << i << " in period " << t << endl;
        exit(EXIT_FAILURE);
      }
    }

    if (Imm(i) == 1)
    {
      // If K-firm imitates, determine which other firm's technology it will imitate
      Tdtot = 0;
      for (ii = 1; ii <= N1; ii++)
      {
        Td.element(ii) = sqrt(((A1(ii) - A1(i)) * (A1(ii) - A1(i))) + ((A1p(ii) - A1p(i)) * (A1p(ii) - A1p(i))) + ((A1_en(ii) - A1_en(i)) * (A1_en(ii) - A1_en(i))) + ((A1_ef(ii) - A1_ef(i)) * (A1_ef(ii) - A1_ef(i))) + ((A1p_en(ii) - A1p_en(i)) * (A1p_en(ii) - A1p_en(i))) + ((A1p_ef(ii) - A1p_ef(i)) * (A1p_ef(ii) - A1p_ef(i))));
        if (Td.element(ii) > 0)
        {
          Td.element(ii) = 1 / Td.element(ii);
        }
        else
        {
          Td.element(ii) = 0;
        }
        Tdtot += Td.element(ii);
      }
      for (ii = 1; ii <= N1; ii++)
      {
        Td.element(ii) /= Tdtot;
        Td.element(ii) += Td.element(ii - 1);
      }
      rnd = ran1(p_seed);
      for (ii = 1; ii <= N1; ii++)
      {
        if (rnd <= Td.element(ii) && rnd > Td.element(ii - 1))
        {
          A1imm(i) = A1(ii);
          A1pimm(i) = A1p(ii);
          EE_imm(i) = A1_en(ii);
          EEp_imm(i) = A1p_en(ii);
          EF_imm(i) = A1_ef(ii);
          EFp_imm(i) = A1p_ef(ii);
        }
      }

      if (A1pimm(i) == 0 || A1imm(i) == 0 || A1p(i) == 0 || A1(i) == 0)
      {
        std::cerr << "\n\n ERROR: A1=0 for K-firm " << i << " in period " << t << endl;
        Errors << "\n A1=0 for K-firm " << i << " in period " << t << endl;
        exit(EXIT_FAILURE);
      }
      if (EEp_imm(i) == 0 || EE_imm(i) == 0 || A1p_en(i) == 0 || A1_en(i) == 0)
      {
        std::cerr << "\n\n ERROR: A1_en=0 for K-firm " << i << " in period " << t << endl;
        Errors << "\n A1_en=0 for K-firm " << i << " in period " << t << endl;
        exit(EXIT_FAILURE);
      }
      if (EFp_imm(i) == 0 || EF_imm(i) == 0 || A1p_ef(i) == 0 || A1_ef(i) == 0)
      {
        std::cerr << "\n\n ERROR: A1_ef=0 for K-firm " << i << " in period " << t << endl;
        Errors << "\n A1_ef=0 for K-firm " << i << " in period " << t << endl;
        exit(EXIT_FAILURE);
      }
    }

    // If the imitated technology is superior, adopt it
    if (((1 + mi1) * (w(1) / (A1pimm(i) * a) + c_en(1) / EEp_imm(i) + t_CO2 * EFp_imm(i) / EEp_imm(i))) + (w(1) / A1imm(i) + c_en(1) / EE_imm(i) + t_CO2 * EF_imm(i) / EE_imm(i)) * b < ((1 + mi1) * (w(1) / (A1p(i) * a) + c_en(1) / A1p_en(i) + t_CO2 * A1p_ef(i) / A1p_en(i)) + (w(1) / A1(i) + c_en(1) / A1_en(i) + t_CO2 * A1_ef(i) / A1_en(i)) * b))
    {
      A1(i) = A1imm(i);
      A1p(i) = A1pimm(i);
      A1_en(i) = EE_imm(i);
      A1p_en(i) = EEp_imm(i);
      A1_ef(i) = EF_imm(i);
      A1p_ef(i) = EFp_imm(i);
    }

    // If the innovated technology is superior, adopt it
    if (((1 + mi1) * (w(1) / (A1pinn(i) * a) + c_en(1) / EEp_inn(i) + t_CO2 * EFp_inn(i) / EEp_inn(i))) + (w(1) / A1inn(i) + c_en(1) / EE_inn(i) + t_CO2 * EF_inn(i) / EE_inn(i)) * b < ((1 + mi1) * (w(1) / (A1p(i) * a) + c_en(1) / A1p_en(i) + t_CO2 * A1p_ef(i) / A1p_en(i)) + (w(1) / A1(i) + c_en(1) / A1_en(i) + t_CO2 * A1_ef(i) / A1_en(i)) * b))
    {
      A1(i) = A1inn(i);
      A1p(i) = A1pinn(i);
      A1_en(i) = EE_inn(i);
      A1p_en(i) = EEp_inn(i);
      A1_ef(i) = EF_inn(i);
      A1p_ef(i) = EFp_inn(i);
    }

    // Update the productivity matrices
    if (t < T)
    {
      A(t + 1, i) = A1(i);
      A_en(t + 1, i) = A1_en(i);
      A_ef(t + 1, i) = A1_ef(i);
    }
  }

  LD1rdtot = Ld1rd.Sum();

  // Determine the best technologies in the system post-R&D
  A1top = A1(1);
  A1ptop = A1p(1);
  A1_en_top = A1_en(1);
  A1_ef_top = A1_ef(1);
  A1p_en_top = A1p_en(1);
  A1p_ef_top = A1p_ef(1);

  for (i = 1; i <= N1; i++)
  {
    if (A1(i) > A1top)
    {
      A1top = A1(i);
    }
    if (A1p(i) > A1ptop)
    {
      A1ptop = A1p(i);
    }
    if (A1_en(i) > A1_en_top)
    {
      A1_en_top = A1_en(i);
    }
    if (A1p_en(i) > A1p_en_top)
    {
      A1p_en_top = A1p_en(i);
    }
    if (A1_ef(i) < A1_ef_top)
    {
      A1_ef_top = A1_ef(i);
    }
    if (A1p_ef(i) < A1p_ef_top)
    {
      A1p_ef_top = A1p_ef(i);
    }
  }

  for (i = 1; i <= N1; i++)
  {
    if (A1p(i) > A1pmax)
    {
      A1pmax = A1p(i);
    }
    if (A1_en(i) > A1_en_max)
    {
      A1_en_max = A1_en(i);
    }
    if (A1p_en(i) > A1p_en_max)
    {
      A1p_en_max = A1p_en(i);
    }
    if (A1_ef(i) < A1_ef_max)
    {
      A1_ef_max = A1_ef(i);
    }
    if (A1p_ef(i) < A1p_ef_max)
    {
      A1p_ef_max = A1p_ef(i);
    }
  }

  Errors.close();
}

void DEPOSITCHECK(void)
{
  ofstream Errors(errorfilename, ios::app);
  for (i = 1; i <= NB; i++)
  {
    deviation = fabs(DepositShare_e(i) - Deposits_eb(1, i) / Deposits_eb.Row(1).Sum());
    if (deviation > tolerance)
    {
      std::cerr << "Share error Deposits_eb for bank " << i << " in period " << t << endl;
      Errors << "\n Share error Deposits_eb for bank " << i << " in period " << t << endl;
    }
    deviation = fabs(DepositShare_h(i) - Deposits_hb(1, i) / Deposits_hb.Row(1).Sum());
    if (deviation > tolerance)
    {
      std::cerr << "Share error Deposits_hb for bank " << i << " in period " << t << endl;
      Errors << "\n Share error Deposits_hb for bank " << i << " in period " << t << endl;
    }
    DepositsCheck_1 = Deposits(1, i) - Deposits_hb(1, i) - Deposits_eb(1, i);
    DepositsCheck_2 = 0;
    for (j = 1; j <= N1; j++)
    {
      if (BankMatch_1(j, i) == 1)
      {
        DepositsCheck_2 += Deposits_1(1, j);
      }
    }
    for (j = 1; j <= N2; j++)
    {
      if (BankMatch_2(j, i) == 1)
      {
        DepositsCheck_2 += Deposits_2(1, j);
      }
    }
    deviation = fabs((DepositsCheck_1 - DepositsCheck_2) / Deposits(1, i));
    if (deviation > tolerance)
    {
      std::cerr << "Share error firm deposits for bank " << i << " in period " << t << endl;
      Errors << "\n Share error firm deposits for bank " << i << " in period " << t << endl;
    }
  }
  Errors.close();
}

void NEGATIVITYCHECK(void)
{
  ofstream Errors(errorfilename, ios::app);
  for (j = 1; j <= N2; ++j)
  {
    if (Loans_2(1, j) < 0)
    {
      std::cerr << "Error loans for C-firm " << j << " in period " << t << endl;
      Errors << "\n Error loans for C-firm " << j << " in period " << t << endl;
    }
    if (Deposits_2(1, j) < 0)
    {
      std::cerr << "Error deposits for C-firm " << j << " in period " << t << endl;
      Errors << "\n Error deposits for C-firm " << j << " in period " << t << endl;
    }
    if (CapitalStock(1, j) < 0)
    {
      std::cerr << "Error Capital for C-firm " << j << " in period " << t << endl;
      Errors << "\n Error Capital for C-firm " << j << " in period " << t << endl;
    }
    if (Inventories(1, j) < 0)
    {
      std::cerr << "Error Inventories for C-firm " << j << " in period " << t << endl;
      Errors << "\n Error Inventories for C-firm " << j << " in period " << t << endl;
    }
    if (Investment_2(j) < 0)
    {
      std::cerr << "Error Investment for C-firm " << j << " in period " << t << endl;
      Errors << "\n Error Investment for C-firm " << j << " in period " << t << endl;
    }
    if (Taxes_2(j) < 0)
    {
      std::cerr << "Error Taxes for C-firm " << j << " in period " << t << endl;
      Errors << "\n Error Taxes for C-firm " << j << " in period " << t << endl;
    }
    if (Wages_2(j) < 0)
    {
      std::cerr << "Error Wages for C-firm " << j << " in period " << t << endl;
      Errors << "\n Error Wages for C-firm " << j << " in period " << t << endl;
    }
    if (EnergyPayments_2(j) < 0)
    {
      std::cerr << "Error EnergyPayments for C-firm " << j << " in period " << t << endl;
      Errors << "\n Error EnergyPayments for C-firm " << j << " in period " << t << endl;
    }
    if (Dividends_2(j) < 0)
    {
      std::cerr << "Error Dividends for C-firm " << j << " in period " << t << endl;
      Errors << "\n Error Dividends for C-firm " << j << " in period " << t << endl;
    }
    if (LoanInterest_2(j) < 0)
    {
      std::cerr << "Error LoanInterest for C-firm " << j << " in period " << t << endl;
      Errors << "\n Error LoanInterest for C-firm " << j << " in period " << t << endl;
    }
    if (InterestDeposits_2(j) < 0)
    {
      std::cerr << "Error InterestDeposits for C-firm " << j << " in period " << t << endl;
      Errors << "\n Error InterestDeposits for C-firm " << j << " in period " << t << endl;
    }
    if (S2(1, j) < 0)
    {
      std::cerr << "Error S2 for C-firm " << j << " in period " << t << endl;
      Errors << "\n Error S2 for C-firm " << j << " in period " << t << endl;
    }
    if (DebtRemittances2(j) < 0)
    {
      std::cerr << "Error DebtRemittances for C-firm " << j << " in period " << t << endl;
      Errors << "\n Error DebtRemittances for C-firm " << j << " in period " << t << endl;
    }
    if (Taxes_CO2_2(j) < 0)
    {
      std::cerr << "Error Taxes_CO2 for C-firm " << j << " in period " << t << endl;
      Errors << "\n Error Taxes_CO2 for C-firm " << j << " in period " << t << endl;
    }
  }

  for (i = 1; i <= N1; i++)
  {
    if (Deposits_1(1, i) < 0)
    {
      std::cerr << "Error Deposits for K-firm " << i << " in period " << t << endl;
      Errors << "\n Error Deposits for K-firm " << i << " in period " << t << endl;
    }
    if (S1(i) < 0)
    {
      std::cerr << "Error S1 for K-firm " << i << " in period " << t << endl;
      Errors << "\n Error S1 for K-firm " << i << " in period " << t << endl;
    }
    if (Taxes_1(i) < 0)
    {
      std::cerr << "Error Taxes for K-firm " << i << " in period " << t << endl;
      Errors << "\n Error Taxes for K-firm " << i << " in period " << t << endl;
    }
    if (Wages_1(i) < 0)
    {
      std::cerr << "Error Wages for K-firm " << i << " in period " << t << endl;
      Errors << "\n Error Wages for K-firm " << i << " in period " << t << endl;
    }
    if (EnergyPayments_1(i) < 0)
    {
      std::cerr << "Error EnergyPayments for K-firm " << i << " in period " << t << endl;
      Errors << "\n Error EnergyPayments for K-firm " << i << " in period " << t << endl;
    }
    if (Dividends_1(i) < 0)
    {
      std::cerr << "Error Dividends for K-firm " << i << " in period " << t << endl;
      Errors << "\n Error Dividends for K-firm " << i << " in period " << t << endl;
    }
    if (InterestDeposits_1(i) < 0)
    {
      std::cerr << "Error InterestDeposits for K-firm " << i << " in period " << t << endl;
      Errors << "\n Error InterestDeposits for K-firm " << i << " in period " << t << endl;
    }
    if (Taxes_CO2_1(i) < 0)
    {
      std::cerr << "Error Taxes_CO2 for K-firm " << i << " in period " << t << endl;
      Errors << "\n Error Taxes_CO2 for K-firm " << i << " in period " << t << endl;
    }
  }

  for (i = 1; i <= NB; i++)
  {
    if (Loans_b(1, i) < (-tolerance * cpi(1)))
    {
      std::cerr << "Error Loans for Bank " << i << " in period " << t << endl;
      Errors << "\n Error Loans for Bank " << i << " in period " << t << endl;
    }
    if (Deposits(1, i) < (-tolerance * cpi(1)))
    {
      std::cerr << "Error Deposits for Bank " << i << " in period " << t << endl;
      Errors << "\n Error Deposits for Bank " << i << " in period " << t << endl;
    }
    if (Deposits_hb(1, i) < (-tolerance * cpi(1)))
    {
      std::cerr << "Error Deposits_hb for Bank " << i << " in period " << t << endl;
      Errors << "\n Error Deposits_hb  for Bank " << i << " in period " << t << endl;
    }
    if (Deposits_eb(1, i) < (-tolerance * cpi(1)))
    {
      std::cerr << "Error Deposits_eb for Bank " << i << " in period " << t << endl;
      Errors << "\n Error Deposits_eb for Bank " << i << " in period " << t << endl;
    }
    if (GB_b(1, i) < (-tolerance * cpi(1)))
    {
      std::cerr << "Error GB for Bank " << i << " in period " << t << endl;
      Errors << "\n Error GB for Bank " << i << " in period " << t << endl;
    }
    if (Reserves_b(1, i) < (-tolerance * cpi(1)))
    {
      std::cerr << "Error Reserves for Bank " << i << " in period " << t << endl;
      Errors << "\n Error Reserves for Bank " << i << " in period " << t << endl;
    }
    if (Advances_b(1, i) < (-tolerance * cpi(1)))
    {
      std::cerr << "Error Advances for Bank " << i << " in period " << t << endl;
      Errors << "\n Error Advances for Bank " << i << " in period " << t << endl;
    }
    if (Taxes_b(i) < 0)
    {
      std::cerr << "Error Taxes for Bank " << i << " in period " << t << endl;
      Errors << "\n Error Taxes for Bank " << i << " in period " << t << endl;
    }
    if (Dividends_b(i) < 0)
    {
      std::cerr << "Error Dividends for Bank " << i << " in period " << t << endl;
      Errors << "\n Error Dividends for Bank " << i << " in period " << t << endl;
    }
    if (LoanInterest(i) < 0)
    {
      std::cerr << "Error LoanInterest for Bank " << i << " in period " << t << endl;
      Errors << "\n Error LoanInterest for Bank " << i << " in period " << t << endl;
    }
    if (InterestDeposits(i) < 0)
    {
      std::cerr << "Error InterestDeposits for Bank " << i << " in period " << t << endl;
      Errors << "\n Error InterestDeposits for Bank " << i << " in period " << t << endl;
    }
    if (InterestBonds_b(i) < 0)
    {
      std::cerr << "Error InterestBonds for Bank " << i << " in period " << t << endl;
      Errors << "\n Error InterestBonds for Bank " << i << " in period " << t << endl;
    }
    if (BondRepayments_b(i) < 0)
    {
      std::cerr << "Error BondRepayments for Bank " << i << " in period " << t << endl;
      Errors << "\n Error BondRepayments for Bank " << i << " in period " << t << endl;
    }
    if (InterestReserves_b(i) < 0)
    {
      std::cerr << "Error InterestReserves for Bank " << i << " in period " << t << endl;
      Errors << "\n Error InterestReserves for Bank " << i << " in period " << t << endl;
    }
    if (InterestAdvances_b(i) < 0)
    {
      std::cerr << "Error InterestAdvances for Bank " << i << " in period " << t << endl;
      Errors << "\n Error InterestAdvances for Bank " << i << " in period " << t << endl;
    }
    if (Bailout_b(i) < 0)
    {
      std::cerr << "Error Bailout for Bank " << i << " in period " << t << endl;
      Errors << "\n Error Bailout for Bank " << i << " in period " << t << endl;
    }
  }

  if (Deposits_h(1) < 0)
  {
    std::cerr << "Error Deposits_h in period " << t << endl;
    Errors << "\n Error Deposits_h in period " << t << endl;
  }
  if (Consumption < 0)
  {
    std::cerr << "Error Consumption in period " << t << endl;
    Errors << "\n Error Consumption in period " << t << endl;
  }
  if (Benefits < 0)
  {
    std::cerr << "Error Benefits in period " << t << endl;
    Errors << "\n Error Benefits in period " << t << endl;
  }
  if (Taxes_h < 0)
  {
    std::cerr << "Error Taxes_h in period " << t << endl;
    Errors << "\n Error Taxes_h in period " << t << endl;
  }
  if (Wages < 0)
  {
    std::cerr << "Error Wages in period " << t << endl;
    Errors << "\n Error Wages in period " << t << endl;
  }
  if (Dividends(1) < 0)
  {
    std::cerr << "Error Dividends in period " << t << endl;
    Errors << "\n Error Dividends in period " << t << endl;
  }
  if (InterestDeposits_h < 0)
  {
    std::cerr << "Error InterestDeposits_h in period " << t << endl;
    Errors << "\n Error InterestDeposits_h in period " << t << endl;
  }

  if (G < 0)
  {
    std::cerr << "Error G in period " << t << endl;
    Errors << "\n Error G in period " << t << endl;
  }
  if (Taxes < 0)
  {
    std::cerr << "Error Taxes in period " << t << endl;
    Errors << "\n Error Taxes in period " << t << endl;
  }
  if (Bailout < 0)
  {
    std::cerr << "Error Bailout in period " << t << endl;
    Errors << "\n Error Bailout in period " << t << endl;
  }
  if (InterestReserves < 0)
  {
    std::cerr << "Error InterestReserves in period " << t << endl;
    Errors << "\n Error InterestReserves in period " << t << endl;
  }
  if (InterestAdvances < 0)
  {
    std::cerr << "Error InterestAdvances in period " << t << endl;
    Errors << "\n Error InterestAdvances in period " << t << endl;
  }

  if (EnergyPayments < 0)
  {
    std::cerr << "Error EnergyPayments in period " << t << endl;
    Errors << "\n Error EnergyPayments in period " << t << endl;
  }
  if (Wages_en < 0)
  {
    std::cerr << "Error Wages_en in period " << t << endl;
    Errors << "\n Error Wages_en in period " << t << endl;
  }
  if (Dividends_e < 0)
  {
    std::cerr << "Error Dividends_e in period " << t << endl;
    Errors << "\n Error Dividends_e in period " << t << endl;
  }
  if (InterestDeposits_e < 0)
  {
    std::cerr << "Error InterestDeposits_e in period " << t << endl;
    Errors << "\n Error InterestDeposits_e in period " << t << endl;
  }
  Errors.close();
}

void CHECKSUMS(void)
{
  ofstream Errors(errorfilename, ios::app);
  deviation = fabs((Deposits_h(1) - Deposits_hb.Row(1).Sum()) / Deposits_hb.Row(1).Sum());
  if (deviation > tolerance && Deposits_hb.Row(1).Sum() > tolerance && Deposits_h(1) > tolerance)
  {
    std::cerr << "Sum error Deposits_h in period " << t << endl;
    Errors << "\n Sum error Deposits_h in period " << t << endl;
  }
  deviation = fabs((Deposits_e(1) - Deposits_eb.Row(1).Sum()) / Deposits_eb.Row(1).Sum());
  if (deviation > tolerance && Deposits_eb.Row(1).Sum() > tolerance && Deposits_e(1) > tolerance)
  {
    std::cerr << "Sum error Deposits_e in period " << t << endl;
    Errors << "\n Sum error Deposits_e in period " << t << endl;
  }
  deviation = fabs((GB_cb(1) + GB_b.Row(1).Sum() - GB(1)) / GB(1));
  if (deviation > tolerance)
  {
    std::cerr << "Sum error GB in period " << t << endl;
    Errors << "\n Sum error GB in period " << t << endl;
  }
  deviation = fabs((Deposits_1.Row(1).Sum() + Deposits_2.Row(1).Sum() + Deposits_hb.Row(1).Sum() + Deposits_eb.Row(1).Sum() - Deposits.Row(1).Sum()) / Deposits.Row(1).Sum());
  if (deviation > tolerance && Deposits.Row(1).Sum() > tolerance)
  {
    std::cerr << "Sum error Deposits in period " << t << endl;
    Errors << "\n Sum error Deposits in period " << t << endl;
  }
  deviation = fabs((Reserves(1) - Reserves_b.Row(1).Sum()) / Reserves_b.Row(1).Sum());
  if (deviation > tolerance && Reserves_b.Row(1).Sum() > tolerance && Reserves(1) > tolerance)
  {
    std::cerr << "Sum error Reserves in period " << t << endl;
    Errors << "\n Sum error Reserves in period " << t << endl;
  }
  deviation = fabs((Advances(1) - Advances_b.Row(1).Sum()) / Advances_b.Row(1).Sum());
  if (deviation > tolerance && Advances_b.Row(1).Sum() > tolerance && Advances(1) > tolerance)
  {
    std::cerr << "Sum error Advances in period " << t << endl;
    Errors << "\n Sum error Advances in period " << t << endl;
  }

  deviation = fabs((Loans_2.Row(1).Sum() - Loans_b.Row(1).Sum()) / Loans_b.Row(1).Sum());
  if (deviation > tolerance && Loans_b.Row(1).Sum() > tolerance && Loans_2.Row(1).Sum() > tolerance)
  {
    std::cerr << "Sum error Loans in period " << t << endl;
    Errors << "\n Sum error Loans in period " << t << endl;
  }
  Errors.close();
}

void ADJUSTSTOCKS(void)
{
  ofstream Errors(errorfilename, ios::app);
  deviation = 0;
  for (i = 1; i <= NB; i++)
  {
    prior(i) = Loans_b(1, i) + Reserves_b(1, i) + GB_b(1, i) - Deposits(1, i) - Advances_b(1, i);
  }
  prior_cb = Advances(1) + GB_cb(1) - Reserves(1);

  if (Advances(1) <= 0 || Advances_b.Row(1).Sum() <= 0)
  {
    Advances(1) = 0;
    for (i = 1; i <= NB; i++)
    {
      Advances_b(1, i) = 0;
    }
  }

  if (Reserves(1) <= 0 || Reserves_b.Row(1).Sum() <= 0)
  {
    Reserves(1) = 0;
    for (i = 1; i <= NB; i++)
    {
      Reserves_b(1, i) = 0;
    }
  }

  if (GB(1) < 0)
  {
    GB_cb(1) = GB(1);
    for (i = 1; i <= NB; i++)
    {
      GB_b(1, i) = 0;
    }
  }

  if (GB(1) > 0 && fabs(GB_cb(1) - GB(1)) / GB(1) < tolerance && GB_b.Row(1).Sum() < tolerance)
  {
    GB_cb(1) = GB(1);
    for (i = 1; i <= NB; i++)
    {
      GB_b(1, i) = 0;
    }
  }

  if (GB(1) > 0 && fabs(GB_cb(1) - GB(1)) / GB(1) > tolerance && GB_b.Row(1).Sum() > tolerance)
  {
    if (GB_b.Row(1).Sum() > 0)
    {
      for (i = 1; i <= NB; i++)
      {
        ShareBonds(i) = GB_b(1, i) / GB_b.Row(1).Sum();
      }
    }
    else
    {
      for (i = 1; i <= NB; i++)
      {
        ShareBonds(i) = (NL_1(i) + NL_2(i)) / (N1 + N2);
      }
    }
  }

  for (i = 1; i <= NB; i++)
  {

    if (Deposits_h(1) > 0 && Deposits_hb.Row(1).Sum() > 0)
    {
      DepositShare_h(i) = Deposits_hb(1, i) / Deposits_hb.Row(1).Sum();
    }
    else
    {
      DepositShare_h(i) = (NL_2(i) + NL_1(i)) / (N1 + N2);
    }

    if (Deposits_e(1) > 0 && Deposits_eb.Row(1).Sum() > 0)
    {
      DepositShare_e(i) = Deposits_eb(1, i) / Deposits_eb.Row(1).Sum();
    }
    else
    {
      DepositShare_e(i) = (NL_2(i) + NL_1(i)) / (N1 + N2);
    }

    if (Reserves(1) > 0)
    {
      ShareReserves(i) = Reserves_b(1, i) / Reserves_b.Row(1).Sum();
    }

    if (Advances(1) > 0)
    {
      ShareAdvances(i) = Advances_b(1, i) / Advances_b.Row(1).Sum();
    }
  }

  for (i = 1; i <= NB; i++)
  {
    Loans_b(1, i) = 0;
    Deposits(1, i) = 0;

    for (j = 1; j <= N2; j++)
    {
      if (BankMatch_2(j, i) == 1)
      {
        Loans_b(1, i) += Loans_2(1, j);
        Deposits(1, i) += Deposits_2(1, j);
      }
    }
    for (j = 1; j <= N1; j++)
    {
      if (BankMatch_1(j, i) == 1)
      {
        Deposits(1, i) += Deposits_1(1, j);
      }
    }

    if (Deposits_h(1) > 0)
    {
      Deposits_hb(1, i) = DepositShare_h(i) * Deposits_h(1);
      Deposits(1, i) += (DepositShare_h(i) * Deposits_h(1));
    }
    else
    {
      Deposits_hb(1, i) = 0;
    }

    if (Deposits_e(1) > 0)
    {
      Deposits_eb(1, i) = DepositShare_e(i) * Deposits_e(1);
      Deposits(1, i) += (DepositShare_e(i) * Deposits_e(1));
    }
    else
    {
      Deposits_eb(1, i) = 0;
    }

    if (Reserves(1) > 0)
    {
      Reserves_b(1, i) = ShareReserves(i) * Reserves(1);
    }
    else
    {
      Reserves_b(1, i) = 0;
    }

    if (Advances(1) > 0)
    {
      Advances_b(1, i) = ShareAdvances(i) * Advances(1);
    }
    else
    {
      Advances_b(1, i) = 0;
    }

    if (GB(1) > 0 && fabs(GB_cb(1) - GB(1)) / GB(1) > tolerance)
    {
      GB_b(1, i) = ShareBonds(i) * (GB(1) - GB_cb(1));
    }

    post = Loans_b(1, i) + Reserves_b(1, i) + GB_b(1, i) - Deposits(1, i) - Advances_b(1, i);
    Adjustment(i) = post - prior(i);
    deviation += fabs(Adjustment(i));
  }

  post_cb = Advances(1) + GB_cb(1) - Reserves(1);
  Adjustment_cb = post_cb - prior_cb;
  deviation += fabs(Adjustment_cb);

  if (deviation / GDP_n(1) > tolerance)
  {
    std::cerr << "\n\n ERROR: Adjustment in stocks exceeds tolerance in period " << t << endl;
    Errors << "\n Adjustment in stocks exceeds tolerance in period " << t << endl;
  }
  Errors.close();
}

void SFC_CHECK(void)
{
  ofstream Errors(errorfilename, ios::app);

  // Calculate the sectoral balances
  Balance_h = Wages + Benefits + InterestDeposits_h + Dividends(1) + TransferFuel - Taxes_h - Consumption - FirmTransfers;
  Balance_1 = Sales1.Sum() + InterestDeposits_1.Sum() + FirmTransfers_1 - Wages_1.Sum() - EnergyPayments_1.Sum() - Dividends_1.Sum() - Taxes_1.Sum() - Taxes_CO2_1.Sum();
  Balance_2 = Sales2.Sum() + InterestDeposits_2.Sum() + FirmTransfers_2 - Wages_2.Sum() - Investment_2.Sum() - LoanInterest_2.Sum() - EnergyPayments_2.Sum() - Dividends_2.Sum() - Taxes_2.Sum() - Taxes_CO2_2.Sum();
  Balance_b = LoanInterest.Sum() + InterestBonds_b.Sum() + InterestReserves_b.Sum() + Bailout_b.Row(1).Sum() + BankTransfer - InterestDeposits.Sum() - Taxes_b.Sum() - InterestAdvances_b.Sum() - Dividends_b.Sum();
  Balance_e = EnergyPayments + InterestDeposits_e + Subsidy_Exp - Wages_en - Dividends_e - Taxes_CO2_e - FuelCost - Taxes_e_shock;
  Balance_cb = InterestBonds_cb + InterestAdvances - InterestReserves - TransferCB;
  Balance_g = Taxes + TransferCB + Taxes_CO2 + Taxes_e_shock + Taxes_f_shock - InterestBonds - Bailout - EntryCosts - G - Subsidy_Exp;
  Balance_f = FuelCost - TransferFuel - Taxes_f_shock;

  // Sectoral balances should sum to zero
  BalanceSum = Balance_h + Balance_1 + Balance_2 + Balance_b + Balance_e + Balance_cb + Balance_g + Balance_f;
  // Deviation needs to be scaled somehow since model variables (and hence possibly deviations due to rounding) will grow over time
  deviation = fabs(BalanceSum) / (fabs(Balance_h) + fabs(Balance_1) + fabs(Balance_2) + fabs(Balance_b) + fabs(Balance_e) + fabs(Balance_cb) + fabs(Balance_g) + fabs(Balance_f));
  if (deviation > tolerance)
  {
    std::cerr << "\n\n ERROR: Sectoral balances do not sum to zero in period " << t << endl;
    Errors << "\n Sectoral balances do not sum to zero in period " << t << endl;
  }

  // Compare stock and flow measures of bank net worth
  for (i = 1; i <= NB; i++)
  {
    NW_b(1, i) += Adjustment(i);
    if (NW_b(1, i) <= 0 && Bank_active(i) == 1)
    {
      std::cerr << "\n\n ERROR: NW of active bank " << i << " is negative in period " << t << endl;
      Errors << "\n NW of active bank " << i << " is negative in period " << t << endl;
    }
    NW_b_c(i) = Loans_b(1, i) + GB_b(1, i) + Reserves_b(1, i) - Deposits(1, i) - Advances_b(1, i);
  }
  deviation = fabs((NW_b_c.Sum() - NW_b.Row(1).Sum()) / NW_b_c.Sum());
  if (deviation > tolerance)
  {
    std::cerr << "\n\n ERROR: Stock and flow measures of net worth for BANKS are not consistent in period " << t << endl;
    Errors << "\n Stock and flow measures of net worth for BANKS are not consistent in period " << t << endl;
  }

  // Compare stock and flow measures of K-firm net worth
  for (i = 1; i <= N1; i++)
  {
    Balances_1(i) = Sales1(i) + InterestDeposits_1(i) - Wages_1(i) - EnergyPayments_1(i) - Dividends_1(i) - Taxes_1(i) - Taxes_CO2_1(i);
    NW_1(1, i) = Deposits_1(1, i);
    NW_1_c(i) = NW_1(2, i) + Balances_1(i) + baddebt_1(i) + Injection_1(i);
  }
  deviation = fabs((NW_1_c.Sum() - NW_1.Row(1).Sum()) / NW_1_c.Sum());
  if (deviation > tolerance)
  {
    std::cerr << "\n\n ERROR: Stock and flow measures of net worth for K-FIRMS are not consistent in period " << t << endl;
    Errors << "\n Stock and flow measures of net worth for K-FIRMS are not consistent in period " << t << endl;
  }

  // Compare stock and flow measures of C-firm net worth
  for (i = 1; i <= N2; i++)
  {
    NW_2(1, i) = CapitalStock(1, i) + deltaCapitalStock(1, i) + Inventories(1, i) + Deposits_2(1, i) - Loans_2(1, i);
    NW_2_c(i) = NW_2(2, i) + Pi2(i) + baddebt_2(i) + Injection_2(i) - Dividends_2(i) - Taxes_2(i) - Taxes_CO2_2(i) - Loss_Capital(i) - Loss_Inventories(i);
  }
  deviation = fabs((NW_2_c.Sum() - NW_2.Row(1).Sum()) / NW_2_c.Sum());
  if (deviation > tolerance)
  {
    std::cerr << "\n\n ERROR: Stock and flow measures of net worth for C-FIRMS are not consistent in period " << t << endl;
    Errors << "\n Stock and flow measures of net worth for C-FIRMS are not consistent in period " << t << endl;
  }

  // Compare stock and flow measures of Household net worth
  NW_h(1) = Deposits_h(1);
  NW_h_c = NW_h(2) + Balance_h + Deposits_recovered_1 + Deposits_recovered_2;
  deviation = fabs((NW_h(1) - NW_h_c) / NW_h_c);
  if (deviation > tolerance && NW_h(1) > 0 && NW_h_c > tolerance)
  {
    std::cerr << "\n\n ERROR: Stock and flow measures of net worth for HOUSEHOLDS are not consistent in period " << t << endl;
    Errors << "\n Stock and flow measures of net worth for HOUSEHOLDS are not consistent in period " << t << endl;
  }

  // Compare stock and flow measures of CB net worth
  NW_cb(1) = GB_cb(1) + Advances(1) - Reserves(1) - Deposits_fuel_cb(1);
  NW_cb_c = NW_cb(2) + Balance_cb + Adjustment_cb;
  deviation = fabs((NW_cb(1) - NW_cb_c) / NW_cb_c);
  if (deviation > tolerance && fabs(NW_cb(1) / GDP_n(1)) > tolerance && fabs(NW_cb_c / GDP_n(1)) > tolerance)
  {
    std::cerr << "\n\n ERROR: Stock and flow measures of net worth for the CENTRAL BANK are not consistent in period " << t << endl;
    Errors << "\n Stock and flow measures of net worth for the CENTRAL BANK are not consistent in period " << t << endl;
  }

  // Compare stock and flow measures of government net worth
  NW_gov(1) = -GB(1);
  NW_gov_c = NW_gov(2) + Balance_g;
  deviation = fabs((NW_gov(1) - NW_gov_c) / NW_gov_c);
  if (deviation > tolerance)
  {
    std::cerr << "\n\n ERROR: Stock and flow measures of net worth for the GOVERNMENT are not consistent in period " << t << endl;
    Errors << "\n Stock and flow measures of net worth for the GOVERNMENT are not consistent in period " << t << endl;
  }

  // Compare stock and flow measures of Energy sector net worth
  NW_e(1) = Deposits_e(1) + CapitalStock_e(1);
  NW_e_c = NW_e(2) + Balance_e + CapitalStock_e(1) - CapitalStock_e(2);
  deviation = fabs((NW_e(1) - NW_e_c) / NW_e_c);
  if (deviation > tolerance)
  {
    std::cerr << "\n\n ERROR: Stock and flow measures of net worth for the ENERGY SECTOR are not consistent in period " << t << endl;
    Errors << "\n Stock and flow measures of net worth for the ENERGY SECTOR are not consistent in period " << t << endl;
  }

  NW_f(1) = Deposits_fuel(1);
  NW_f_c = Deposits_fuel(2) + FuelCost - TransferFuel - Taxes_f_shock;
  deviation = fabs((NW_f(1) - NW_f_c) / NW_f_c);
  if (deviation > tolerance)
  {
    std::cerr << "\n\n ERROR: Stock and flow measures of net worth for the FOSSIL FUEL SECTOR are not consistent in period " << t << endl;
    Errors << "\n Stock and flow measures of net worth for the FOSSIL FUEL SECTOR are not consistent in period " << t << endl;
  }

  // Sum of all sectoral net worths should be equal to nominal value of tangible assets in the economy
  NWSum = NW_h(1) + NW_1.Row(1).Sum() + NW_2.Row(1).Sum() + NW_b.Row(1).Sum() + NW_e(1) + NW_cb(1) + NW_gov(1) + NW_f(1);
  RealAssets = CapitalStock.Row(1).Sum() + deltaCapitalStock.Row(1).Sum() + Inventories.Row(1).Sum() + CapitalStock_e(1);
  deviation = fabs((NWSum - RealAssets) / RealAssets);
  if (deviation > tolerance)
  {
    std::cerr << "\n\n ERROR: Aggregate net worth not equal to tangible assets in period " << t << endl;
    Errors << "\n Aggregate net worth not equal to tangible assets in period " << t << endl;
  }

  Errors.close();
}

void OVERBOOST(void)
{
  // Reset t0 to shorten time taken to iterate over technology arrays
  t00 = t0;
  flag = 0;
  for (tt = t00; tt <= t && flag == 0; tt++)
  {
    for (i = 1; i <= N1 && flag == 0; i++)
    {
      for (j = 1; j <= N2 && flag == 0; j++)
      {
        if (g[tt - 1][i - 1][j - 1] > 0 || gtemp[tt - 1][i - 1][j - 1] > 0)
          flag = 1;
      }
    }
    if (flag == 0)
    {
      t0++;
    }
  }
}

void UPDATE(void)
{
  ofstream Errors(errorfilename, ios::app);

  if (cpi(2) <= 0)
  {
    std::cerr << "\n\n ERROR: cpi(t-1)<=0 in period " << t << endl;
    Errors << "\n cpi(t-1)<=0 in period " << t << endl;
    exit(EXIT_FAILURE);
  }
  if (Am(2) <= 0)
  {
    std::cerr << "\n\n ERROR: Am(t-1)=0 in period " << t << endl;
    Errors << "\n Am(t-1)=0 in period " << t << endl;
    exit(EXIT_FAILURE);
  }

  // Update mark-up in the energy sector
  if (t > 1)
  {
    dw2 = kappa * dw2 + (1 - kappa) * w(1) / w(2);
    d_cpi2 = kappa * d_cpi2 + (1 - kappa) * cpi(1) / cpi(2);
    mi_en *= dw2;
    CF_ge *= dw2;
    mi_en_preshock *= dw2;
    mi_en_shock *= dw2;
    pf *= dw2;
    pf_preshock *= dw2;
    pf_shock *= dw2;
    c_en_preshock *= dw2;

    if (flag_energyshocks == 1 && t > 248)
    {
      mi_en = mi_en + 0.175 * (mi_en_preshock - mi_en);
    }

    if (flag_energyshocks == 3 && t > 248)
    {
      pf = pf + 0.175 * (pf_preshock - pf);
    }
  }

  // Update lagged variables needed in next period
  Deposits_h(2) = Deposits_h(1);
  Deposits_e(2) = Deposits_e(1);
  Deposits_fuel(2) = Deposits_fuel(1);
  Deposits_fuel_cb(2) = Deposits_fuel_cb(1);
  NW_f(2) = NW_f(1);
  GB_cb(2) = GB_cb(1);
  GB(2) = GB(1);
  Advances(2) = Advances(1);
  Reserves(2) = Reserves(1);
  CapitalStock_e(2) = CapitalStock_e(1);
  NW_h(2) = NW_h(1);
  NW_e(2) = NW_e(1);
  NW_gov(2) = NW_gov(1);
  NW_cb(2) = NW_cb(1);
  Dividends(2) = Dividends(1);
  U(2) = U(1);
  w(2) = w(1);
  Em2(2) = Em2(1);
  ProfitCB(2) = ProfitCB(1);
  c_en(2) = c_en(1);
  D_en_TOT(2) = D_en_TOT(1);
  GDP_r(2) = GDP_r(1);
  GDP_n(2) = GDP_n(1);
  cpi(5) = cpi(4);
  cpi(4) = cpi(3);
  cpi(3) = cpi(2);
  cpi(2) = cpi(1);
  if (t == 2)
  {
    cpi_init = cpi(1);
    GDP_init = GDP_n(1);
  }
  Am(2) = Am(1);
  Am_en(2) = Am_en(1);

  for (i = 1; i <= N1; i++)
  {
    Deposits_1(2, i) = Deposits_1(1, i);
    RD(2, i) = RD(1, i);
    f1(2, i) = f1(1, i);
    NW_1(2, i) = NW_1(1, i);
    S1_temp(2, i) = S1_temp(1, i);
  }

  for (j = 1; j <= N2; j++)
  {
    Deposits_2(2, j) = Deposits_2(1, j);
    Loans_2(2, j) = Loans_2(1, j);
    DebtService_2(2, j) = DebtService_2(1, j);
    f2(3, j) = f2(2, j);
    f2(2, j) = f2(1, j);
    D2(2, j) = D2(1, j);
    N(2, j) = N(1, j);
    Inventories(2, j) = Inventories(1, j);
    EI(2, j) = EI(1, j);
    deltaCapitalStock(2, j) = deltaCapitalStock(1, j);
    S2(2, j) = S2(1, j);
    S2_temp(2, j) = S2_temp(1, j);
    mu2(2, j) = mu2(1, j);
    CapitalStock(2, j) = CapitalStock(1, j);
    NW_2(2, j) = NW_2(1, j);

    for (i = 1; i <= N1; i++)
    {
      for (tt = t0; tt <= t; tt++)
      {
        if (gtemp[tt - 1][i - 1][j - 1] > 0)
        {
          age[tt - 1][i - 1][j - 1] = age[tt - 1][i - 1][j - 1] + 1;
        }
      }
    }
  }

  for (i = 1; i <= NB; i++)
  {
    fB(2, i) = fB(1, i);
    Deposits(2, i) = Deposits(1, i);
    Deposits_hb(2, i) = Deposits_hb(1, i);
    Deposits_eb(2, i) = Deposits_eb(1, i);
    GB_b(2, i) = GB_b(1, i);
    Loans_b(2, i) = Loans_b(1, i);
    Advances_b(2, i) = Advances_b(1, i);
    Reserves_b(2, i) = Reserves_b(1, i);
    NW_b(2, i) = NW_b(1, i);
  }

  // Update regional lagged capacity variables
  if (NR > 0)
  {
    for (int rr = 0; rr < NR; ++rr)
    {
      region_dirty_capacity_lag[rr] = region_dirty_capacity[rr];
      region_green_capacity_lag[rr] = region_green_capacity[rr];
    }
  }

  Errors.close();
}

///////////WRITE OUTPUT/////////////////////////////

void WRITEPROD(void)
{
  // When fulloutput==1, save individual productivity values
  if (fulloutput == 1)
  {
    ofstream inv_prodall1(nomefile6, ios::app);
    inv_prodall1.setf(ios::fixed);
    inv_prodall1.precision(4);
    inv_prodall1.setf(ios::right);

    if (t > 1)
    {
      inv_prodall1 << "\n";
    }

    for (i = 1; i <= N1; i++)
    {
      inv_prodall1.width(20);
      inv_prodall1 << A1p(i);
    }
    inv_prodall1.close();

    ofstream inv_prodall1_en(nomefile8, ios::app);
    inv_prodall1_en.setf(ios::fixed);
    inv_prodall1_en.precision(4);
    inv_prodall1_en.setf(ios::right);
    if (t > 1)
    {
      inv_prodall1_en << "\n";
    }

    for (i = 1; i <= N1; i++)
    {
      inv_prodall1_en.width(20);
      inv_prodall1_en << A1p_en(i);
    }
    inv_prodall1_en.close();

    ofstream inv_prodall1_ef(nomefile10, ios::app);
    inv_prodall1_ef.setf(ios::fixed);
    inv_prodall1_ef.precision(4);
    inv_prodall1_ef.setf(ios::right);
    if (t > 1)
    {
      inv_prodall1_ef << "\n";
    }

    for (i = 1; i <= N1; i++)
    {
      inv_prodall1_ef.width(20);
      inv_prodall1_ef << A1p_ef(i);
    }
    inv_prodall1_ef.close();

    ofstream inv_prodall2(nomefile7, ios::app);
    inv_prodall2.setf(ios::fixed);
    inv_prodall2.precision(4);
    inv_prodall2.setf(ios::right);

    if (t > 1)
    {
      inv_prodall2 << "\n";
    }

    for (j = 1; j <= N2; j++)
    {
      inv_prodall2.width(20);
      inv_prodall2 << A2(j);
    }
    inv_prodall2.close();

    ofstream inv_prodall2_en(nomefile9, ios::app);
    inv_prodall2_en.setf(ios::fixed);
    inv_prodall2_en.precision(4);
    inv_prodall2_en.setf(ios::right);

    if (t > 1)
    {
      inv_prodall2_en << "\n";
    }

    for (j = 1; j <= N2; j++)
    {
      inv_prodall2_en.width(20);
      inv_prodall2_en << A2_en(j);
    }
    inv_prodall2_en.close();

    ofstream inv_prodall2_ef(nomefile11, ios::app);
    inv_prodall2_ef.setf(ios::fixed);
    inv_prodall2_ef.precision(4);
    inv_prodall2_ef.setf(ios::right);

    if (t > 1)
    {
      inv_prodall2_ef << "\n";
    }

    for (j = 1; j <= N2; j++)
    {
      inv_prodall2_ef.width(20);
      inv_prodall2_ef << A2_ef(j);
    }
    inv_prodall2_ef.close();
  }

  if (fulloutput == 1 && t >= 201)
  {
    ofstream inv_prod2(nomefile5, ios::app);
    inv_prod2.setf(ios::fixed);
    inv_prod2.precision(4);
    inv_prod2.setf(ios::right);

    if (t > 201)
    {
      inv_prod2 << "\n";
    }

    for (j = 1; j <= N2; j++)
    {
      inv_prod2.width(20);
      A2scr = log(A2(j)) - A_mi;
      inv_prod2 << A2scr;
    }
    inv_prod2.close();

    ofstream inv_prod1(nomefile4, ios::app);
    inv_prod1.setf(ios::fixed);
    inv_prod1.precision(4);
    inv_prod1.setf(ios::right);

    if (t > 201)
      inv_prod1 << "\n";

    for (i = 1; i <= N1; i++)
    {
      inv_prod1.width(20);
      if (nclient(i) >= 1)
      {
        A1scr = log(A1p(i)) - A1_mi;
        inv_prod1 << A1scr;
      }
      else
      {
        inv_prod1 << "NaN";
      }
    }
    inv_prod1.close();
  }
}

void WRITEDEB(void)
{
  if (fulloutput == 1)
  {
    // When fulloutput==1, save stock of loans of all individual C-firms
    ofstream inv_deball2(nomefile15, ios::app);
    inv_deball2.setf(ios::fixed);
    inv_deball2.precision(4);
    inv_deball2.setf(ios::right);
    if (t > 1)
    {
      inv_deball2 << "\n";
    }
    for (j = 1; j <= N2; j++)
    {
      inv_deball2.width(60);
      if (exiting_2(j) == 1)
      {
        inv_deball2 << 0;
      }
      else
      {
        inv_deball2 << Loans_2(1, j);
      }
    }
    inv_deball2.close();
  }
}

void SAVE(void)
{
  if (fulloutput == 1)
  {
    WRITENW();
    // Output file containing larger set of macro variables
    ofstream inv_output1(nomefile1, ios::app);
    inv_output1.setf(ios::fixed);
    inv_output1.precision(4);
    inv_output1.setf(ios::right);
    inv_output1.width(6);
    inv_output1 << t; // 1
    inv_output1.width(30);
    inv_output1 << GDP_n(1); // 2
    inv_output1.width(30);
    inv_output1 << GDP_r(1); // 3
    inv_output1.width(30);
    inv_output1 << Investment_r; // 4
    inv_output1.width(30);
    inv_output1 << Q1tot; // 6
    inv_output1.width(30);
    inv_output1 << Q2tot; // 7
    inv_output1.width(30);
    inv_output1 << Consumption_r; // 8
    inv_output1.width(30);
    inv_output1 << Pitot1; // 9
    inv_output1.width(30);
    inv_output1 << Pitot2; // 10
    inv_output1.width(30);
    inv_output1 << Deposits_1.Row(1).Sum(); // 11
    inv_output1.width(30);
    inv_output1 << Deposits_2.Row(1).Sum(); // 12
    inv_output1.width(30);
    inv_output1 << LD; // 13
    inv_output1.width(30);
    inv_output1 << U(1); // 14
    inv_output1.width(30);
    inv_output1 << w(1); // 15
    inv_output1.width(30);
    inv_output1 << cpi(1); // 16
    inv_output1.width(30);
    inv_output1 << Am(1); // 17
    inv_output1.width(30);
    inv_output1 << A_sd; // 18
    inv_output1.width(30);
    inv_output1 << G; // 19
    inv_output1.width(30);
    inv_output1 << Taxes; // 20
    inv_output1.width(30);
    inv_output1 << Deficit; // 21
    inv_output1.width(30);
    inv_output1 << GB(1); // 22
    inv_output1.width(30);
    inv_output1 << Loans_2.Row(1).Sum(); // 23
    inv_output1.width(30);
    inv_output1 << NW_b.Row(1).Sum(); // 24
    inv_output1.width(30);
    inv_output1 << baddebt_b.Sum(); // 25
    inv_output1.width(30);
    inv_output1 << r_bonds; // 26
    inv_output1.width(30);
    inv_output1 << r; // 27
    inv_output1.width(30);
    inv_output1 << Bailout; // 28
    inv_output1.width(30);
    inv_output1 << GDP_rg; // 29
    inv_output1.width(30);
    inv_output1 << Divtot_1; // 30
    inv_output1.width(30);
    inv_output1 << Divtot_2; // 31
    inv_output1.width(30);
    inv_output1 << Dividends_b.Sum(); // 32
    inv_output1.width(30);
    inv_output1 << Dividends(1); // 33
    inv_output1.width(30);
    inv_output1 << CreditSupply_all; // 34
    inv_output1.width(30);
    inv_output1 << CreditDemand_all << endl; // 35
    inv_output1.close();
  }

  if (flag_shockexperiment == 1)
  {
    // Output file for analysing results of single climate shock scenario
    ofstream inv_res(nomefile3, ios::app);
    auto write_resultsexp_row = [&](std::ostream &target)
    {
      target.setf(ios::fixed);
      target.precision(10);
      target.setf(ios::right);
      target.width(60);
      target << t; // 1
      target.width(60);
      target << GDP_r(1); // 2
      target.width(60);
      target << Consumption_r; // 3
      target.width(60);
      target << Investment_r; // 4
      target.width(60);
      target << 1 - U(1); // 5
      target.width(60);
      target << cpi(1) / cpi(2); // 6
      target.width(60);
      target << Emiss1_TOT + Emiss2_TOT + Emiss_en; // 7
      target.width(60);
      target << D_en_TOT(1); // 8
      target.width(60);
      target << LS; // 9
      target.width(60);
      target << K.Sum(); // 10
      target.width(60);
      target << A1p_en_dead / A1p_en_survive; // 11
      target.width(60);
      target << A2_en_dead / A2_en_survive; // 12
      target.width(60);
      target << Am_en(1); // 13
      target.width(60);
      target << Am_a; // 14
      target.width(60);
      target << exit_marketshare2.Sum(); // 15
      target.width(60);
      target << exit_payments2.Sum(); // 16
      target.width(60);
      target << exit_equity2.Sum(); // 17
      target.width(60);
      target << exiting_1.Sum(); // 18
      target.width(60);
      target << Bailout; // 19
      target.width(60);
      target << baddebt_b.Sum() / (GDP_n(1) * 4); // 20
      target.width(60);
      target << counter_bankfailure; // 21
      target.width(60);
      target << CapitalStock.Row(1).Sum() / (GDP_n(1) * 4); // 22
      target.width(60);
      target << NW_cb(1) / (GDP_n(1) * 4); // 23
      target.width(60);
      target << NW_h(1) / (GDP_n(1) * 4); // 24
      target.width(60);
      target << NW_2.Row(1).Sum() / (GDP_n(1) * 4); // 25
      target.width(60);
      target << NW_b.Row(1).Sum() / (GDP_n(1) * 4); // 26
      target.width(60);
      target << BankProfits.Sum(); // 27
      target.width(60);
      target << Loans_2.Row(1).Sum() / (GDP_n(1) * 4); // 28
      target.width(60);
      target << CreditDemand_all / CreditSupply_all; // 29
      target.width(60);
      target << NW_gov(1) / (GDP_n(1) * 4); // 30
      target.width(60);
      target << NW_e(1) / (GDP_n(1) * 4); // 31
      target.width(60);
      target << NW_1.Row(1).Sum() / (GDP_n(1) * 4); // 32
      target.width(60);
      target << min(1.0, counter_bankfailure); // 33
      target.width(60);
      target << cpi(1); // 34
      target.width(60);
      target << exp_quota; // 35
      target.width(60);
      target << w(1) / cpi(1); // 36
      target.width(60);
      target << Am2; // 37
      target.width(60);
      target << Am1; // 38
      target.width(60);
      target << GDP_n(1); // 39
      target.width(60);
      target << Investment_n * dim_mach * a; // 40
      target.width(60);
      target << Consumption; // 41
      target.width(60);
      target << t_CO2_en; // 42
      target.width(60);
      target << ReplacementInvestment_r; // 43
      target.width(60);
      target << Emiss1_TOT; // 44
      target.width(60);
      target << Emiss2_TOT; // 45
      target.width(60);
      target << Emiss_en; // 46
      target.width(60);
      target << Tmixed(1); // 47
      target.width(60);
      target << EnergyPayments; // 48
      target.width(60);
      target << FuelCost / GDP_n(1); // 49
      target.width(60);
      target << K_gelag / (K_gelag + K_delag); // 50
      target.width(60);
      target << Deposits_e(1) / (GDP_n(1) * 4); // 51
      target.width(60);
      target << CapitalStock_e(1) / (GDP_n(1) * 4); // 52
      target.width(60);
      target << Pitot1; // 53
      target.width(60);
      target << Pitot2; // 54
      target.width(60);
      target << ProfitEnergy; // 55
      target.width(60);
      target << Wages / cpi(1); // 56
      target.width(60);
      target << Dividends_1.Sum(); // 57
      target.width(60);
      target << Dividends_2.Sum(); // 58
      target.width(60);
      target << Consumption; // 59
      target.width(60);
      target << c_en(1); // 60
      target.width(60);
      target << Deposits_2.Row(1).Sum() / (GDP_n(1) * 4); // 61
      target.width(60);
      target << Deposits_1.Row(1).Sum() / (GDP_n(1) * 4); // 62
      target.width(60);
      target << (Deposits_2.Row(1).Sum() + Deposits_1.Row(1).Sum()) / (GDP_n(1) * 4); // 63
      target.width(60);
      target << exit_payments2.Sum() + exit_equity2.Sum(); // 64
      target.width(60);
      target << Pitot1 + Pitot2; // 65
      target.width(60);
      target << (Dividends_2.Sum() + Dividends_1.Sum()) / cpi(1); // 66
      target.width(60);
      target << Dividends_e / cpi(1); // 67
      target.width(60);
      target << Dividends_b.Sum() / cpi(1); // 68
      target.width(60);
      target << (NW_1.Row(1).Sum() + NW_2.Row(1).Sum()) / (GDP_n(1) * 4); // 69
      target.width(60);
      target << GB(1) / (GDP_n(1) * 4); // 70
      target.width(60);
      target << (cpi(1) / cpi(5)) - 1; // 71
      target.width(60);
      target << pow(GDP_r(1) / GDP_r(2), 4) - 1; // 72
      target.width(60);
      target << NW_f(1) / (GDP_n(1) * 4); // 73
      target.width(60);
      target << Wages / (Pitot1 + Pitot2 + BankProfits.Sum() + ProfitEnergy + FuelCost + Wages) << endl; // 74
    };

    write_resultsexp_row(inv_res);

    if (NR > 0)
    {
      auto write_regional_row = [&](std::ostream &target, int region)
      {
        // Aggregate production and accounting variables for this region
        double reg_S1 = 0, reg_S2 = 0, reg_Q1 = 0, reg_Q2 = 0;
        double reg_K = 0, reg_Investment = 0, reg_EI = 0, reg_SI = 0;
        double reg_Ld1 = 0, reg_Ld2 = 0, reg_LS_used = 0;
        double reg_Emiss1 = 0, reg_Emiss2 = 0;
        double reg_Pi1 = 0, reg_Pi2 = 0;
        double reg_NW1 = 0, reg_NW2 = 0;
        double reg_Deposits1 = 0, reg_Deposits2 = 0;
        double reg_CapitalStock1 = 0, reg_CapitalStock2 = 0;
        double reg_Loans2_sum = 0;        // Loans of C-firms (nominal)
        double reg_Inventories_nom = 0;   // Inventories nominal (C-firms)
        double reg_N_real_sum = 0;        // Inventories real (C-firms)
        double reg_Consumption_r_val = 0; // Real consumption (initialized here)
        double reg_A1_sum = 0, reg_A2_sum = 0, reg_A1_weight = 0, reg_A2_weight = 0;
        double reg_A1en_dead = 0, reg_A1en_survive = 0, reg_A2en_dead = 0, reg_A2en_survive = 0;

        // Aggregate K-firms
        for (int i = 1; i <= N1; ++i)
        {
          if (region_firm_assignment_K[i - 1] == region)
          {
            reg_S1 += S1(i);
            reg_Q1 += Q1(i);
            reg_Ld1 += Ld1(i);
            reg_Pi1 += Pi1(i);
            reg_NW1 += NW_1(1, i);
            reg_Deposits1 += Deposits_1(1, i);
            reg_CapitalStock1 += CapitalStock(1, i);
            // exit counts not reported in the reduced regional output
            if (nclient(i) >= 1)
            {
              reg_A1_sum += A1p(i) * S1(i);
              reg_A1_weight += S1(i);
            }
            if (A1p_en_survive > 0)
            {
              if (exiting_1(i) == 1 && nclient(i) >= 1)
                reg_A1en_dead += A1p_en(i);
              else if (nclient(i) >= 1)
                reg_A1en_survive += A1p_en(i);
            }
          }
        }

        // Aggregate C-firms
        for (int j = 1; j <= N2; ++j)
        {
          if (region_firm_assignment_C[j - 1] == region)
          {
            reg_S2 += S2(1, j);
            reg_Q2 += Q2(j);
            // Real consumption uses same formula as national: S2/p2
            reg_Consumption_r_val += S2(1, j) / p2(j);
            reg_K += K(j);
            reg_Investment += I(j);
            reg_EI += EI(1, j);
            reg_SI += SI(j);
            reg_Ld2 += Ld2(j);
            reg_Emiss2 += Emiss2(j);
            reg_Pi2 += Pi2(j);
            reg_NW2 += NW_2(1, j);
            reg_Deposits2 += Deposits_2(1, j);
            reg_CapitalStock2 += CapitalStock(1, j);
            // Accumulators specific to requested accounting variables
            reg_Loans2_sum += Loans_2(1, j);
            reg_Inventories_nom += Inventories(1, j);
            reg_N_real_sum += N(1, j);
            reg_A2_sum += A2(j) * S2(1, j);
            reg_A2_weight += S2(1, j);
            if (A2_en_survive > 0)
            {
              if (exiting_2(j) == 1)
                reg_A2en_dead += A2_en(j);
              else
                reg_A2en_survive += A2_en(j);
            }
          }
        }

        reg_LS_used = reg_Ld1 + reg_Ld2;

        // Calculate regional labor demand including R&D and energy sector proportionally
        // Regional share of total firm labor demand
        double reg_LD_firms = reg_LS_used;       // Ld1 + Ld2 for this region
        double total_LD_firms = LD1tot + LD2tot; // Total Ld1 + Ld2 across all firms

        // Allocate R&D and energy sector labor proportionally to regional firm labor demand
        double reg_LD_rd = 0;
        double reg_LD_en = 0;
        if (total_LD_firms > 0)
        {
          reg_LD_rd = LD1rdtot * (reg_LD_firms / total_LD_firms);
          reg_LD_en = LDentot * (reg_LD_firms / total_LD_firms);
        }

        // Total regional labor demand
        double reg_LD = reg_LD_firms + reg_LD_rd + reg_LD_en;

        // Allocate regional labor supply proportionally to total labor demand
        double reg_LS = (LD > 0 && LS > 0) ? LS * (reg_LD / LD) : 0;

        // Regional employment rate = regional LD / regional LS
        double reg_employment_rate = (reg_LS > 0) ? reg_LD / reg_LS : 0;

        // Persist unemployment to global vector for consistency (reg_U = unemployment)
        if ((int)reg_U.size() >= region)
          reg_U[region - 1] = 1.0 - reg_employment_rate;
        double reg_Am1 = (reg_A1_weight > 0) ? reg_A1_sum / reg_A1_weight : 0;
        double reg_Am2 = (reg_A2_weight > 0) ? reg_A2_sum / reg_A2_weight : 0;
        double reg_Am_combined = (reg_A1_weight + reg_A2_weight > 0)
                                     ? (reg_A1_sum + reg_A2_sum) / (reg_A1_weight + reg_A2_weight)
                                     : 0;

        // Regional GDP (real): sum of real output produced
        double reg_GDP_r = reg_Q1 * dim_mach + reg_Q2;

        // Regional GDP (nominal): calculate using same formula as national
        double reg_GDP_n_val = 0;
        for (int i = 1; i <= N1; ++i)
        {
          if (region_firm_assignment_K[i - 1] == region)
          {
            reg_GDP_n_val += Q1(i) * dim_mach * p1(i) * a;
          }
        }
        for (int j = 1; j <= N2; ++j)
        {
          if (region_firm_assignment_C[j - 1] == region)
          {
            reg_GDP_n_val += Q2(j) * p2(j);
          }
        }

        // Regional real investment: use same formula as national (EI + SI)
        double reg_Investment_r = reg_EI + reg_SI;

        // Regional real consumption: already calculated as sum of S2/p2 in C-firm loop

        // Regional energy sector
        double reg_Emiss_en = 0;
        double reg_D_en = 0;
        double reg_dirty_cap = region_dirty_capacity[region - 1];
        double reg_green_cap = region_green_capacity[region - 1];
        double reg_total_cap = reg_dirty_cap + reg_green_cap;
        double reg_green_cap_lag = region_green_capacity_lag[region - 1];
        double reg_dirty_cap_lag = region_dirty_capacity_lag[region - 1];
        double reg_total_cap_lag = reg_green_cap_lag + reg_dirty_cap_lag;
        double reg_green_share = (reg_total_cap_lag > 0) ? reg_green_cap_lag / reg_total_cap_lag : 0;

        // Approximate regional energy emissions and demand as share of total based on capacity
        if (K_delag + K_gelag > 0)
        {
          double cap_share = reg_total_cap / (K_delag + K_gelag);
          reg_Emiss_en = Emiss_en * cap_share;
          reg_D_en = D_en_TOT(1) * cap_share;
        }

        // Regional green energy production proportional to green capacity
        double reg_Qge_val = 0;
        if (K_ge > 0)
        {
          reg_Qge_val = Q_ge * (reg_green_cap / K_ge);
        }

        // Regional total emissions
        double reg_Emiss_total = reg_Emiss1 + reg_Emiss2 + reg_Emiss_en;

        // Approximate regional cumulative emissions as share of national cumulative emissions
        double national_emiss_tot = Emiss1_TOT + Emiss2_TOT + Emiss_en;
        double reg_Cum_emission_val = 0;
        if (national_emiss_tot > 0 && Cum_emissions > 0)
        {
          double share = reg_Emiss_total / national_emiss_tot;
          reg_Cum_emission_val = Cum_emissions * share;
        }

        // Write regional resultsexp row (only requested variables, 14 columns)
        target.setf(ios::fixed);
        target.precision(10);
        target.setf(ios::right);
        target.width(60);
        target << t; // 1: time
        target.width(60);
        target << reg_GDP_r; // 2: reg_GDP_r (Real GDP)
        target.width(60);
        target << reg_Consumption_r_val; // 3: reg_Consumption_r (Total real consumption)
        target.width(60);
        target << reg_Investment_r; // 4: reg_Investment_r (Total real investment)
        target.width(60);
        target << (1.0 - reg_U[region - 1]); // 5: 1 - reg_U(1) (Employment rate)
        target.width(60);
        target << reg_Am_combined; // 6: reg_Am(1) (Mean productivity across K and C-firms)
        target.width(60);
        target << reg_Loans2_sum; // 7: reg_Loans_2 (Loans of C-firms)
        target.width(60);
        target << reg_Inventories_nom; // 8: reg_Inventories (Nominal value of C-firms' inventories)
        target.width(60);
        target << reg_N_real_sum; // 9: reg_N.Row(1).sum (Inventories real)
        target.width(60);
        target << reg_GDP_n_val; // 10: reg_GDP_n (Nominal GDP)
        target.width(60);
        target << ((reg_D_en > 0) ? (reg_Qge_val / reg_D_en) : 0); // 11: reg_Qge / reg_D_en (Green energy share of demand)
        target.width(60);
        target << reg_D_en; // 12: reg_D_en_TOT(1) (Total energy demand)
        target.width(60);
        target << reg_Emiss_total; // 13: reg_Emiss_TOT(1) (Total emissions)
        target.width(60);
        target << reg_Cum_emission_val << endl; // 14: reg_Cum_emission (Cumulative emissions)
      };

      for (int rr = 1; rr <= NR; ++rr)
      {
        if (region_resultsexp_streams[rr - 1].is_open())
        {
          write_regional_row(region_resultsexp_streams[rr - 1], rr);
        }
      }
    }

    inv_res.close();

    if (flag_exogenousshocks == 0)
    {
      ofstream inv_shockpars(nomefile28, ios::app);
      inv_shockpars.setf(ios::fixed);
      inv_shockpars.precision(10);
      inv_shockpars.setf(ios::right);
      inv_shockpars.width(60);
      inv_shockpars << t; // 1
      inv_shockpars.width(60);
      inv_shockpars << X_a(1); // 2
      inv_shockpars.width(60);
      inv_shockpars << X_a(2); // 3
      inv_shockpars.width(60);
      inv_shockpars << X_a(3); // 4
      inv_shockpars.width(60);
      inv_shockpars << X_a(4); // 5
      inv_shockpars.width(60);
      inv_shockpars << X_a(5); // 6
      inv_shockpars.width(60);
      inv_shockpars << X_a(6); // 7
      inv_shockpars.width(60);
      inv_shockpars << X_a(7); // 8
      inv_shockpars.width(60);
      inv_shockpars << X_a(8); // 9
      inv_shockpars.width(60);
      inv_shockpars << X_a(9); // 10
      inv_shockpars.width(60);
      inv_shockpars << X_b(1); // 11
      inv_shockpars.width(60);
      inv_shockpars << X_b(2); // 12
      inv_shockpars.width(60);
      inv_shockpars << X_b(3); // 13
      inv_shockpars.width(60);
      inv_shockpars << X_b(4); // 14
      inv_shockpars.width(60);
      inv_shockpars << X_b(5); // 15
      inv_shockpars.width(60);
      inv_shockpars << X_b(6); // 16
      inv_shockpars.width(60);
      inv_shockpars << X_b(7); // 17
      inv_shockpars.width(60);
      inv_shockpars << X_b(8); // 18
      inv_shockpars.width(60);
      inv_shockpars << X_b(9) << endl; // 19
      inv_shockpars.close();
    }
  }
  else if (flag_validation == 1)
  {
    ofstream inv_val1(nomefile16, ios::app);
    inv_val1.setf(ios::fixed);
    inv_val1.precision(10);
    inv_val1.setf(ios::right);
    inv_val1.width(60);
    inv_val1 << t; // 1
    inv_val1.width(60);
    inv_val1 << GDP_r(1); // 2
    inv_val1.width(60);
    inv_val1 << Consumption_r; // 3
    inv_val1.width(60);
    inv_val1 << Investment_r; // 4
    inv_val1.width(60);
    inv_val1 << 1 - U(1); // 5
    inv_val1.width(60);
    inv_val1 << cpi(1) / cpi(2); // 6
    inv_val1.width(60);
    inv_val1 << Emiss1_TOT + Emiss2_TOT; // 7
    inv_val1.width(60);
    inv_val1 << D_en_TOT(1); // 8
    inv_val1.width(60);
    inv_val1 << RD.Row(1).Sum() + RD_en_de + RD_en_ge; // 9
    inv_val1.width(60);
    inv_val1 << Loans_2.Row(1).Sum(); // 10
    inv_val1.width(60);
    inv_val1 << baddebt_b.Sum(); // 11
    inv_val1.width(60);
    inv_val1 << Deposits_h(1) / (GDP_n(1) * 4); // 12
    inv_val1.width(60);
    inv_val1 << Deposits_e(1) / (GDP_n(1) * 4); // 13
    inv_val1.width(60);
    inv_val1 << Deposits_1.Row(1).Sum() / (GDP_n(1) * 4); // 14
    inv_val1.width(60);
    inv_val1 << Deposits_2.Row(1).Sum() / (GDP_n(1) * 4); // 15
    inv_val1.width(60);
    inv_val1 << GB(1) / (GDP_n(1) * 4); // 16
    inv_val1.width(60);
    inv_val1 << GB_cb(1) / (GDP_n(1) * 4); // 17
    inv_val1.width(60);
    inv_val1 << Loans_2.Row(1).Sum() / (GDP_n(1) * 4); // 18
    inv_val1.width(60);
    inv_val1 << Advances(1) / (GDP_n(1) * 4); // 19
    inv_val1.width(60);
    inv_val1 << Reserves(1) / (GDP_n(1) * 4); // 20
    inv_val1.width(60);
    inv_val1 << NW_2.Row(1).Sum() / (GDP_n(1) * 4); // 21
    inv_val1.width(60);
    inv_val1 << NW_b.Row(1).Sum() / (GDP_n(1) * 4); // 22
    inv_val1.width(60);
    inv_val1 << NW_1.Row(1).Sum() / (GDP_n(1) * 4); // 23
    inv_val1.width(60);
    inv_val1 << CapitalStock.Row(1).Sum() / (GDP_n(1) * 4); // 24
    inv_val1.width(60);
    inv_val1 << EnergyPayments / (GDP_n(1)); // 25
    inv_val1.width(60);
    inv_val1 << GDP_n(1); // 26
    inv_val1.width(60);
    inv_val1 << Am(1); // 27
    inv_val1.width(60);
    inv_val1 << Am_en(1); // 28
    inv_val1.width(60);
    inv_val1 << Am1; // 29
    inv_val1.width(60);
    inv_val1 << Am2; // 30
    inv_val1.width(60);
    inv_val1 << cpi(1); // 31
    inv_val1.width(60);
    inv_val1 << kpi; // 32
    inv_val1.width(60);
    inv_val1 << exit_marketshare2.Sum(); // 33
    inv_val1.width(60);
    inv_val1 << exit_payments2.Sum(); // 34
    inv_val1.width(60);
    inv_val1 << exit_equity2.Sum(); // 35
    inv_val1.width(60);
    inv_val1 << exiting_1.Sum(); // 36
    inv_val1.width(60);
    inv_val1 << Bailout / GDP_n(1); // 37
    inv_val1.width(60);
    inv_val1 << baddebt_b.Sum() / GDP_n(1); // 38
    inv_val1.width(60);
    inv_val1 << counter_bankfailure; // 39
    inv_val1.width(60);
    inv_val1 << Emiss_en; // 40
    inv_val1.width(60);
    inv_val1 << Tmixed(1); // 41
    inv_val1.width(60);
    inv_val1 << H1; // 42
    inv_val1.width(60);
    inv_val1 << H2; // 43
    inv_val1.width(60);
    inv_val1 << exit_payments2.Sum() + exit_equity2.Sum(); // 44
    inv_val1.width(60);
    inv_val1 << NW_e(1) / (GDP_n(1) * 4); // 45
    inv_val1.width(60);
    inv_val1 << NW_gov(1) / (GDP_n(1) * 4); // 46
    inv_val1.width(60);
    inv_val1 << NW_f(1) / (GDP_n(1) * 4); // 47
    inv_val1.width(60);
    inv_val1 << Balance_h / (GDP_n(1)); // 48
    inv_val1.width(60);
    inv_val1 << Balance_1 / (GDP_n(1)); // 49
    inv_val1.width(60);
    inv_val1 << Balance_2 / (GDP_n(1)); // 50
    inv_val1.width(60);
    inv_val1 << Balance_b / (GDP_n(1)); // 51
    inv_val1.width(60);
    inv_val1 << Balance_e / (GDP_n(1)); // 52
    inv_val1.width(60);
    inv_val1 << Balance_cb / (GDP_n(1)); // 53
    inv_val1.width(60);
    inv_val1 << Balance_g / (GDP_n(1)); // 54
    inv_val1.width(60);
    inv_val1 << Balance_f / (GDP_n(1)) << endl; // 55
    inv_val1.close();

    if (t % 50 == 0)
    {
      ofstream inv_val2(nomefile17, ios::app);
      inv_val2.setf(ios::fixed);
      inv_val2.precision(10);
      inv_val2.setf(ios::right);
      if (t > 50)
      {
        inv_val2 << "\n";
      }
      for (j = 1; j <= N2; j++)
      {
        inv_val2.width(60);
        if (exiting_2(j) == 1)
        {
          inv_val2 << "NA";
        }
        else
        {
          inv_val2 << S2_temp(1, j);
        }
      }
      inv_val2.close();

      ofstream inv_val3(nomefile18, ios::app);
      inv_val3.setf(ios::fixed);
      inv_val3.precision(10);
      inv_val3.setf(ios::right);
      if (t > 50)
      {
        inv_val3 << "\n";
      }
      for (i = 1; i <= N1; i++)
      {
        inv_val3.width(60);
        if (exiting_1(i) == 1)
        {
          inv_val3 << "NA";
        }
        else
        {
          inv_val3 << S1_temp(1, i);
        }
      }
      inv_val3.close();

      ofstream inv_val4(nomefile19, ios::app);
      inv_val4.setf(ios::fixed);
      inv_val4.precision(10);
      inv_val4.setf(ios::right);
      if (t > 50)
      {
        inv_val4 << "\n";
      }
      for (j = 1; j <= N2; j++)
      {
        inv_val4.width(60);
        A2scr = log(A2(j)) - A_mi;
        inv_val4 << A2scr;
      }
      inv_val4.close();

      ofstream inv_val5(nomefile20, ios::app);
      inv_val5.setf(ios::fixed);
      inv_val5.precision(10);
      inv_val5.setf(ios::right);
      if (t > 50)
      {
        inv_val5 << "\n";
      }
      for (i = 1; i <= N1; i++)
      {
        inv_val5.width(60);
        A1scr = log(A1p(i)) - A1_mi;
        inv_val5 << A1scr;
      }
      inv_val5.close();

      ofstream inv_val6(nomefile21, ios::app);
      inv_val6.setf(ios::fixed);
      inv_val6.precision(10);
      inv_val6.setf(ios::right);
      if (t > 50)
      {
        inv_val6 << "\n";
      }
      for (j = 1; j <= N2; j++)
      {
        inv_val6.width(60);
        inv_val6 << log(A2_en(j)) - A2_en_mi;
      }
      inv_val6.close();

      ofstream inv_val7(nomefile22, ios::app);
      inv_val7.setf(ios::fixed);
      inv_val7.precision(10);
      inv_val7.setf(ios::right);
      if (t > 50)
      {
        inv_val7 << "\n";
      }
      for (j = 1; j <= N2; j++)
      {
        inv_val7.width(60);
        inv_val7 << log(A2_ef(j)) - A2_ef_mi;
      }
      inv_val7.close();

      ofstream inv_val8(nomefile23, ios::app);
      inv_val8.setf(ios::fixed);
      inv_val8.precision(10);
      inv_val8.setf(ios::right);
      if (t > 50)
      {
        inv_val8 << "\n";
      }
      for (i = 1; i <= N1; i++)
      {
        inv_val8.width(60);
        inv_val8 << log(A1p_en(i)) - A1_en_mi;
      }
      inv_val8.close();

      ofstream inv_val9(nomefile24, ios::app);
      inv_val9.setf(ios::fixed);
      inv_val9.precision(10);
      inv_val9.setf(ios::right);
      if (t > 50)
      {
        inv_val9 << "\n";
      }
      for (i = 1; i <= N1; i++)
      {
        inv_val9.width(60);
        inv_val9 << log(A1p_ef(i)) - A1_ef_mi;
      }
      inv_val9.close();

      ofstream inv_val10(nomefile25, ios::app);
      inv_val10.setf(ios::fixed);
      inv_val10.precision(10);
      inv_val10.setf(ios::right);
      if (t > 50)
      {
        inv_val10 << "\n";
      }
      for (j = 1; j <= N2; j++)
      {
        inv_val10.width(60);
        if (exiting_2(j) == 1)
        {
          inv_val10 << "NA";
        }
        else
        {
          inv_val10 << I(j);
        }
      }
      inv_val10.close();

      ofstream inv_val11(nomefile26, ios::app);
      inv_val11.setf(ios::fixed);
      inv_val11.precision(10);
      inv_val11.setf(ios::right);
      if (t > 50)
      {
        inv_val11 << "\n";
      }
      for (j = 1; j <= N2; j++)
      {
        inv_val11.width(60);
        inv_val11 << (S2_temp(1, j) - S2_temp(2, j)) / S2_temp(2, j);
      }
      inv_val11.close();

      ofstream inv_val12(nomefile27, ios::app);
      inv_val12.setf(ios::fixed);
      inv_val12.precision(10);
      inv_val12.setf(ios::right);
      if (t > 50)
      {
        inv_val12 << "\n";
      }
      for (i = 1; i <= N1; i++)
      {
        inv_val12.width(60);
        inv_val12 << (S1_temp(1, i) - S1_temp(2, i)) / S1_temp(2, i);
      }
      inv_val12.close();
    }
  }
  else
  {
    // Output file containing selected macro variables
    ofstream inv_ymc(nomefile2, ios::app);
    inv_ymc.setf(ios::fixed);
    inv_ymc.precision(4);
    inv_ymc.setf(ios::right);
    inv_ymc.width(60);
    inv_ymc << t; // 1
    inv_ymc.width(60);
    inv_ymc << GDP_r(1); // 2
    inv_ymc.width(60);
    inv_ymc << Consumption_r; // 3
    inv_ymc.width(60);
    inv_ymc << Investment_r; // 4
    inv_ymc.width(60);
    inv_ymc << 1 - U(1); // 5
    inv_ymc.width(60);
    inv_ymc << cpi(1); // 6
    inv_ymc.width(60);
    inv_ymc << cpi(1) / cpi(2); // 7
    inv_ymc.width(60);
    inv_ymc << Am(1); // 8
    inv_ymc.width(60);
    inv_ymc << Deficit; // 9
    inv_ymc.width(60);
    inv_ymc << GB(1); // 10
    inv_ymc.width(60);
    inv_ymc << w(1); // 11
    inv_ymc.width(60);
    inv_ymc << w(1) / w(2); // 12
    inv_ymc.width(60);
    inv_ymc << r; // 13
    inv_ymc.width(60);
    inv_ymc << r_bonds; // 14
    inv_ymc.width(60);
    inv_ymc << Loans_2.Row(1).Sum(); // 15
    inv_ymc.width(60);
    inv_ymc << Deposits.Row(1).Sum(); // 16
    inv_ymc.width(60);
    inv_ymc << baddebt_b.Sum(); // 17
    inv_ymc.width(60);
    inv_ymc << CreditSupply_all; // 18
    inv_ymc.width(60);
    inv_ymc << CreditDemand_all; // 19
    inv_ymc.width(60);
    inv_ymc << Inventories.Row(1).Sum(); // 20
    inv_ymc.width(60);
    inv_ymc << N.Row(1).Sum(); // 21
    inv_ymc.width(60);
    inv_ymc << Bailout; // 22
    inv_ymc.width(60);
    inv_ymc << GDP_n(1); // 23
    inv_ymc.width(60);
    inv_ymc << Q_ge / D_en_TOT(1); // 24
    inv_ymc.width(60);
    inv_ymc << D_en_TOT(1); // 25
    inv_ymc.width(60);
    inv_ymc << Emiss_TOT(1); // 26
    inv_ymc.width(60);
    inv_ymc << Cum_emissions; // 27
    inv_ymc.width(60);
    inv_ymc << Tmixed(1) << endl; // 28
    inv_ymc.close();
  }
}

void WRITENW(void)
{
  if (fulloutput == 1)
  {
    // When fulloutput==1, save the individual net worths of all K-firms, C-firms and Banks
    ofstream inv_nwall1(nomefile12, ios::app);
    inv_nwall1.setf(ios::fixed);
    inv_nwall1.precision(4);
    inv_nwall1.setf(ios::right);
    if (t > 1)
    {
      inv_nwall1 << "\n";
    }
    for (i = 1; i <= N1; i++)
    {
      inv_nwall1.width(60);
      inv_nwall1 << NW_1(1, i);
    }
    inv_nwall1.close();

    ofstream inv_nwall2(nomefile13, ios::app);
    inv_nwall2.setf(ios::fixed);
    inv_nwall2.precision(4);
    inv_nwall2.setf(ios::right);
    if (t > 1)
    {
      inv_nwall2 << "\n";
    }
    for (j = 1; j <= N2; j++)
    {
      inv_nwall2.width(60);
      inv_nwall2 << NW_2(1, j);
    }
    inv_nwall2.close();

    ofstream inv_nwall3(nomefile14, ios::app);
    inv_nwall3.setf(ios::fixed);
    inv_nwall3.precision(4);
    inv_nwall3.setf(ios::right);
    if (t > 1)
    {
      inv_nwall3 << "\n";
    }
    for (i = 1; i <= NB; i++)
    {
      inv_nwall3.width(60);
      inv_nwall3 << NW_b(1, i);
    }
    inv_nwall3.close();
  }
}

///////////GENERATE OUTPUT FOLDERS, FILES & NAMES/////////////////////
// These functions generate the directories for saving output and the names of the .txt files in which model output is saved
int make_directory(const char *name)
{
#ifdef __linux__
  return mkdir(name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#elif __APPLE__
  return mkdir(name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#else
  return mkdir(name);
#endif
}

void FOLDERS(char *path)
{
  std::string outstr(path);
  for (j = outstr.length(); j > 0; j--)
  {
    if (outstr[j - 1] == '/')
    {
      break;
    }
    else
    {
      outstr.pop_back();
    }
  }
  outstr += "output";
  std::string errstr = outstr + "/errors";
  char out_dir[outstr.length()];
  char err_dir[errstr.length()];
  strcpy(out_dir, outstr.c_str());
  strcpy(err_dir, errstr.c_str());
  const int out_fol = make_directory(out_dir);
  const int err_fol = make_directory(err_dir);
}

void GENFILEOUTPUT1(char *path, const char *s1, char runname[], char const *seednumber)
{
  strcpy(nomefile1, path);
  char *name1 = strcat(nomefile1, s1);
  name1 = strcat(nomefile1, "_");
  name1 = strcat(nomefile1, runname);
  name1 = strcat(nomefile1, "_");
  name1 = strcat(nomefile1, seednumber);
  strcat(nomefile1, ".txt");
}

void GENFILEYMC(char *path, const char *s2, char runname[], char const *seednumber)
{
  strcpy(nomefile2, path);
  char *name2 = strcat(nomefile2, s2);
  name2 = strcat(nomefile2, "_");
  name2 = strcat(nomefile2, runname);
  name2 = strcat(nomefile2, "_");
  name2 = strcat(nomefile2, seednumber);
  strcat(nomefile2, ".txt");
}

void GENFILESHOCKEXP(char *path, const char *s3, char runname[], char const *seednumber)
{
  strcpy(nomefile3, path);
  char *name3 = strcat(nomefile3, s3);
  name3 = strcat(nomefile3, "_");
  name3 = strcat(nomefile3, runname);
  name3 = strcat(nomefile3, "_");
  name3 = strcat(nomefile3, seednumber);
  strcat(nomefile3, ".txt");
}

void GENFILEPROD1(char *path, const char *s4, char runname[], char const *seednumber)
{
  strcpy(nomefile4, path);
  char *name5 = strcat(nomefile4, s4);
  name5 = strcat(nomefile4, "_");
  name5 = strcat(nomefile4, runname);
  name5 = strcat(nomefile4, "_");
  name5 = strcat(nomefile4, seednumber);
  strcat(nomefile4, ".txt");
}

void GENFILEPROD2(char *path, const char *s5, char runname[], char const *seednumber)
{
  strcpy(nomefile5, path);
  char *name4 = strcat(nomefile5, s5);
  name4 = strcat(nomefile5, "_");
  name4 = strcat(nomefile5, runname);
  name4 = strcat(nomefile5, "_");
  name4 = strcat(nomefile5, seednumber);
  strcat(nomefile5, ".txt");
}

void GENFILEPRODALL1(char *path, const char *s6, char runname[], char const *seednumber)
{
  strcpy(nomefile6, path);
  char *name6 = strcat(nomefile6, s6);
  name6 = strcat(nomefile6, "_");
  name6 = strcat(nomefile6, runname);
  name6 = strcat(nomefile6, "_");
  name6 = strcat(nomefile6, seednumber);
  strcat(nomefile6, ".txt");
}

void GENFILEPRODALL2(char *path, const char *s7, char runname[], char const *seednumber)
{
  strcpy(nomefile7, path);
  char *name7 = strcat(nomefile7, s7);
  name7 = strcat(nomefile7, "_");
  name7 = strcat(nomefile7, runname);
  name7 = strcat(nomefile7, "_");
  name7 = strcat(nomefile7, seednumber);
  strcat(nomefile7, ".txt");
}

void GENFILEPRODALL1_en(char *path, const char *s8, char runname[], char const *seednumber)
{
  strcpy(nomefile8, path);
  const char *name8 = strcat(nomefile8, s8);
  name8 = strcat(nomefile8, "_");
  name8 = strcat(nomefile8, runname);
  name8 = strcat(nomefile8, "_");
  name8 = strcat(nomefile8, seednumber);
  strcat(nomefile8, ".txt");
}

void GENFILEPRODALL2_en(char *path, const char *s9, char runname[], char const *seednumber)
{
  strcpy(nomefile9, path);
  const char *name9 = strcat(nomefile9, s9);
  name9 = strcat(nomefile9, "_");
  name9 = strcat(nomefile9, runname);
  name9 = strcat(nomefile9, "_");
  name9 = strcat(nomefile9, seednumber);
  strcat(nomefile9, ".txt");
}

void GENFILEPRODALL1_ef(char *path, const char *s10, char runname[], char const *seednumber)
{
  strcpy(nomefile10, path);
  const char *name10 = strcat(nomefile10, s10);
  name10 = strcat(nomefile10, "_");
  name10 = strcat(nomefile10, runname);
  name10 = strcat(nomefile10, "_");
  name10 = strcat(nomefile10, seednumber);
  strcat(nomefile10, ".txt");
}

void GENFILEPRODALL2_ef(char *path, const char *s11, char runname[], char const *seednumber)
{
  strcpy(nomefile11, path);
  const char *name11 = strcat(nomefile11, s11);
  name11 = strcat(nomefile11, "_");
  name11 = strcat(nomefile11, runname);
  name11 = strcat(nomefile11, "_");
  name11 = strcat(nomefile11, seednumber);
  strcat(nomefile11, ".txt");
}

void GENFILENWALL1(char *path, const char *s12, char runname[], char const *seednumber)
{
  strcpy(nomefile12, path);
  char *name12 = strcat(nomefile12, s12);
  name12 = strcat(nomefile12, "_");
  name12 = strcat(nomefile12, runname);
  name12 = strcat(nomefile12, "_");
  name12 = strcat(nomefile12, seednumber);
  strcat(nomefile12, ".txt");
}

void GENFILENWALL2(char *path, const char *s13, char runname[], char const *seednumber)
{
  strcpy(nomefile13, path);
  char *name13 = strcat(nomefile13, s13);
  name13 = strcat(nomefile13, "_");
  name13 = strcat(nomefile13, runname);
  name13 = strcat(nomefile13, "_");
  name13 = strcat(nomefile13, seednumber);
  strcat(nomefile13, ".txt");
}

void GENFILENWALL3(char *path, const char *s14, char runname[], char const *seednumber)
{
  strcpy(nomefile14, path);
  char *name14 = strcat(nomefile14, s14);
  name14 = strcat(nomefile14, "_");
  name14 = strcat(nomefile14, runname);
  name14 = strcat(nomefile14, "_");
  name14 = strcat(nomefile14, seednumber);
  strcat(nomefile14, ".txt");
}

void GENFILEDEBALL2(char *path, const char *s15, char runname[], char const *seednumber)
{
  strcpy(nomefile15, path);
  char *name15 = strcat(nomefile15, s15);
  name15 = strcat(nomefile15, "_");
  name15 = strcat(nomefile15, runname);
  name15 = strcat(nomefile15, "_");
  name15 = strcat(nomefile15, seednumber);
  strcat(nomefile15, ".txt");
}

void GENFILEVALIDATION1(char *path, const char *s16, char const *seednumber)
{
  strcpy(nomefile16, path);
  char *name16 = strcat(nomefile16, s16);
  name16 = strcat(nomefile16, "_");
  name16 = strcat(nomefile16, seednumber);
  strcat(nomefile16, ".txt");
}

void GENFILEVALIDATION2(char *path, const char *s17, char const *seednumber)
{
  strcpy(nomefile17, path);
  char *name17 = strcat(nomefile17, s17);
  name17 = strcat(nomefile17, "_");
  name17 = strcat(nomefile17, seednumber);
  strcat(nomefile17, ".txt");
}

void GENFILEVALIDATION3(char *path, const char *s18, char const *seednumber)
{
  strcpy(nomefile18, path);
  char *name18 = strcat(nomefile18, s18);
  name18 = strcat(nomefile18, "_");
  name18 = strcat(nomefile18, seednumber);
  strcat(nomefile18, ".txt");
}

void GENFILEVALIDATION4(char *path, const char *s19, char const *seednumber)
{
  strcpy(nomefile19, path);
  char *name19 = strcat(nomefile19, s19);
  name19 = strcat(nomefile19, "_");
  name19 = strcat(nomefile19, seednumber);
  strcat(nomefile19, ".txt");
}

void GENFILEVALIDATION5(char *path, const char *s20, char const *seednumber)
{
  strcpy(nomefile20, path);
  char *name20 = strcat(nomefile20, s20);
  name20 = strcat(nomefile20, "_");
  name20 = strcat(nomefile20, seednumber);
  strcat(nomefile20, ".txt");
}

void GENFILEVALIDATION6(char *path, const char *s21, char const *seednumber)
{
  strcpy(nomefile21, path);
  char *name21 = strcat(nomefile21, s21);
  name21 = strcat(nomefile21, "_");
  name21 = strcat(nomefile21, seednumber);
  strcat(nomefile21, ".txt");
}

void GENFILEVALIDATION7(char *path, const char *s22, char const *seednumber)
{
  strcpy(nomefile22, path);
  char *name22 = strcat(nomefile22, s22);
  name22 = strcat(nomefile22, "_");
  name22 = strcat(nomefile22, seednumber);
  strcat(nomefile22, ".txt");
}

void GENFILEVALIDATION8(char *path, const char *s23, char const *seednumber)
{
  strcpy(nomefile23, path);
  char *name23 = strcat(nomefile23, s23);
  name23 = strcat(nomefile23, "_");
  name23 = strcat(nomefile23, seednumber);
  strcat(nomefile23, ".txt");
}

void GENFILEVALIDATION9(char *path, const char *s24, char const *seednumber)
{
  strcpy(nomefile24, path);
  char *name24 = strcat(nomefile24, s24);
  name24 = strcat(nomefile24, "_");
  name24 = strcat(nomefile24, seednumber);
  strcat(nomefile24, ".txt");
}

void GENFILEVALIDATION10(char *path, const char *s25, char const *seednumber)
{
  strcpy(nomefile25, path);
  char *name25 = strcat(nomefile25, s25);
  name25 = strcat(nomefile25, "_");
  name25 = strcat(nomefile25, seednumber);
  strcat(nomefile25, ".txt");
}

void GENFILEVALIDATION11(char *path, const char *s26, char const *seednumber)
{
  strcpy(nomefile26, path);
  char *name26 = strcat(nomefile26, s26);
  name26 = strcat(nomefile26, "_");
  name26 = strcat(nomefile26, seednumber);
  strcat(nomefile26, ".txt");
}

void GENFILEVALIDATION12(char *path, const char *s27, char const *seednumber)
{
  strcpy(nomefile27, path);
  char *name27 = strcat(nomefile27, s27);
  name27 = strcat(nomefile27, "_");
  name27 = strcat(nomefile27, seednumber);
  strcat(nomefile27, ".txt");
}

void GENFILESHOCKPARS(char *path, const char *s28, char runname[], char const *seednumber)
{
  strcpy(nomefile28, path);
  char *name3 = strcat(nomefile28, s28);
  name3 = strcat(nomefile28, "_");
  name3 = strcat(nomefile28, runname);
  name3 = strcat(nomefile28, "_");
  name3 = strcat(nomefile28, seednumber);
  strcat(nomefile28, ".txt");
}

// This function generates the actual output files
void INTFILE(void)
{
  ofstream Errors(errorfilename);

  if (flag_shockexperiment == 1)
  {
    // One-off climate shock experiment
    ofstream inv_res(nomefile3);
    ofstream inv_shockpars(nomefile28);
  }
  else if (flag_validation == 1)
  {
    ofstream inv_val1(nomefile16);
    ofstream inv_val2(nomefile17);
    ofstream inv_val3(nomefile18);
    ofstream inv_val4(nomefile19);
    ofstream inv_val5(nomefile20);
    ofstream inv_val6(nomefile21);
    ofstream inv_val7(nomefile22);
    ofstream inv_val8(nomefile23);
    ofstream inv_val9(nomefile24);
    ofstream inv_val10(nomefile25);
    ofstream inv_val11(nomefile26);
    ofstream inv_val12(nomefile27);
  }
  else if (fulloutput == 0)
  {
    ofstream inv_ymc(nomefile2);
  }

  if (fulloutput == 1)
  {
    // When fulloutput==1, full model output will be saved
    ofstream inv_output1(nomefile1);
    ofstream inv_prod1(nomefile4);
    ofstream inv_prod2(nomefile5);
    ofstream inv_prodall1(nomefile6);
    ofstream inv_prodall2(nomefile7);
    ofstream inv_prodall1_en(nomefile8);
    ofstream inv_prodall2_en(nomefile9);
    ofstream inv_prodall1_ef(nomefile10);
    ofstream inv_prodall2_ef(nomefile11);
    ofstream inv_nwall1(nomefile12);
    ofstream inv_nwall2(nomefile13);
    ofstream inv_nwall3(nomefile14);
    ofstream inv_deball2(nomefile15);
  }
}

///////////AUXILIARY/////////////////////

void catchAlarm(int sig)
{
  ofstream Errors(errorfilename, ios::app);
  std::cerr << "\n\n Run timed out!" << endl;
  Errors << "\n Run timed out! " << endl;
  Errors.close();
  exit(EXIT_FAILURE);
}

double ROUND(double x)
{
  double x_floor = floor(x);
  double resto = x - x_floor;
  if (resto > 0.5)
    x = x_floor + 1;
  else
    x = x_floor;
  return x;
}

void ALLOCATEBANKCUSTOMERS(void)
{
  NL_2 = 0;
  double sum_NL_2;
  sum_NL_2 = 0;

  while (sum_NL_2 != N2)
  {
    sum_NL_2 = 0;
    for (i = 1; i <= NB; i++)
    {
      pareto_rv = bpareto(pareto_a, pareto_k, pareto_p);
      NL_2(i) = pareto_rv;
      sum_NL_2 += pareto_rv;
    }
  }

  NL_1 = 0;
  double sum_NL_1;
  sum_NL_1 = 0;

  while (sum_NL_1 != N1)
  {
    sum_NL_1 = 0;
    for (i = 1; i <= NB; i++)
    {
      pareto_rv = bpareto(pareto_a, min(pareto_k * N1r / N2r, 0.9), ceil(pareto_p * N1r / N2r));
      NL_1(i) = pareto_rv;
      sum_NL_1 += pareto_rv;
    }
  }
}

double bpareto(double par_a, double par_k, double par_p)
{

  double z;  // Uniform random number from 0 to 1
  double rv; // RV to be returned

  // Pull a uniform RV (0 < z < 1)
  do
  {
    z = double(ran1(p_seed));
  } while ((z == 0) || (z == 1));

  // Generate the bounded Pareto rv using the inversion method
  rv = pow((pow(par_k, par_a) / (z * pow((par_k / par_p), par_a) - z + 1)), (1.0 / par_a));
  // make the variable an integer
  rv = ceil(rv);

  return (rv);
}