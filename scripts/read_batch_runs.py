# Read files on the given path
from pathlib import Path
import re
import pandas as pd
import numpy as np
import yaml
import matplotlib.pyplot as plt
from scipy import stats
import seaborn as sns


def infer_numeric_id(name):
    """Infer integer ID from a folder/file name by extracting the first digit group."""
    match = re.search(r"(\d+)", str(name))
    if not match:
        return None
    return int(match.group(1))


def parse_resultsexp_filename(filename):
    """Parse macro-level results filename: resultsexp_<scenario>_<run>_<seed>.txt"""
    match = re.match(
        r"^resultsexp_(?P<scenario>[^_]+)_(?P<run>[^_]+)_(?P<seed>\d+)\.txt$",
        filename,
        flags=re.IGNORECASE,
    )
    if not match:
        return None

    groups = match.groupdict()
    scenario_id = infer_numeric_id(groups["scenario"])
    run_id = infer_numeric_id(groups["run"])
    if scenario_id is None or run_id is None:
        return None

    return {
        "scenario": scenario_id,
        "run": run_id,
        "seed": int(groups["seed"]),
    }


def parse_ymc_filename(filename):
    """Parse macro-level ymc filename: ymc_<scenario>_<run>_<seed>.txt"""
    match = re.match(
        r"^ymc_(?P<scenario>[^_]+)_(?P<run>[^_]+)_(?P<seed>\d+)\.txt$",
        filename,
        flags=re.IGNORECASE,
    )
    if not match:
        return None

    groups = match.groupdict()
    scenario_id = infer_numeric_id(groups["scenario"])
    run_id = infer_numeric_id(groups["run"])
    if scenario_id is None or run_id is None:
        return None

    return {
        "scenario": scenario_id,
        "run": run_id,
        "seed": int(groups["seed"]),
    }


def parse_shockpars_filename(filename):
    """Parse shockpars filename: shockpars_<scenario>_<run>_<seed>.txt"""
    match = re.match(
        r"^shockpars_(?P<scenario>[^_]+)_(?P<run>[^_]+)_(?P<seed>\d+)\.txt$",
        filename,
        flags=re.IGNORECASE,
    )
    if not match:
        return None

    groups = match.groupdict()
    scenario_id = infer_numeric_id(groups["scenario"])
    run_id = infer_numeric_id(groups["run"])
    if scenario_id is None or run_id is None:
        return None

    return {
        "scenario": scenario_id,
        "run": run_id,
        "seed": int(groups["seed"]),
    }


def parse_ymc_reg_filename(filename):
    """Parse region-level ymc filename: ymc_<region>_<scenario>_<run>_<seed>.txt"""
    match = re.match(
        r"^ymc_(?P<region>[^_]+)_(?P<scenario>[^_]+)_(?P<run>[^_]+)_(?P<seed>\d+)\.txt$",
        filename,
        flags=re.IGNORECASE,
    )
    if not match:
        return None

    groups = match.groupdict()
    region_id = infer_numeric_id(groups["region"])
    scenario_id = infer_numeric_id(groups["scenario"])
    run_id = infer_numeric_id(groups["run"])
    if region_id is None or scenario_id is None or run_id is None:
        return None

    return {
        "region": region_id,
        "scenario": scenario_id,
        "run": run_id,
        "seed": int(groups["seed"]),
    }


def parse_resultsexp_reg_filename(filename):
    """Parse region-level resultsexp filename: resultsexp_<region>_<scenario>_<run>_<seed>.txt"""
    match = re.match(
        r"^resultsexp_(?P<region>[^_]+)_(?P<scenario>[^_]+)_(?P<run>[^_]+)_(?P<seed>\d+)\.txt$",
        filename,
        flags=re.IGNORECASE,
    )
    if not match:
        return None

    groups = match.groupdict()
    region_id = infer_numeric_id(groups["region"])
    scenario_id = infer_numeric_id(groups["scenario"])
    run_id = infer_numeric_id(groups["run"])
    if region_id is None or scenario_id is None or run_id is None:
        return None

    return {
        "region": region_id,
        "scenario": scenario_id,
        "run": run_id,
        "seed": int(groups["seed"]),
    }


def is_matching_resultsexp_filename(file_path, scenario_num, run_num):
    if not file_path.is_file() or file_path.suffix.lower() != ".txt":
        return False
    parsed = parse_resultsexp_filename(file_path.name)
    return parsed is not None and parsed["scenario"] == int(scenario_num) and parsed["run"] == int(run_num)


def is_matching_ymc_filename(file_path, scenario_num, run_num):
    """Check if a YMC file is available for the given scenario and run."""
    if not file_path.is_file() or file_path.suffix.lower() != ".txt":
        return False
    parsed = parse_ymc_filename(file_path.name)
    return parsed is not None and parsed["scenario"] == int(scenario_num) and parsed["run"] == int(run_num)


def is_matching_shockpars_filename(file_path, scenario_num, run_num):
    """Check if a shockpars file is available for the given scenario and run."""
    if not file_path.is_file() or file_path.suffix.lower() != ".txt":
        return False
    parsed = parse_shockpars_filename(file_path.name)
    return parsed is not None and parsed["scenario"] == int(scenario_num) and parsed["run"] == int(run_num)


def is_matching_ymc_reg_filename(file_path, scenario_num, run_num):
    if not file_path.is_file() or file_path.suffix.lower() != ".txt":
        return False
    parsed = parse_ymc_reg_filename(file_path.name)
    return parsed is not None and parsed["scenario"] == int(scenario_num) and parsed["run"] == int(run_num)


def is_matching_resultsexp_reg_filename(file_path, scenario_num, run_num):
    if not file_path.is_file() or file_path.suffix.lower() != ".txt":
        return False
    parsed = parse_resultsexp_reg_filename(file_path.name)
    return parsed is not None and parsed["scenario"] == int(scenario_num) and parsed["run"] == int(run_num)


def list_resultsexp_files_for_run(run_folder, scenario_num, run_num):
    return [
        file_path
        for file_path in run_folder.iterdir()
        if is_matching_resultsexp_filename(file_path, scenario_num, run_num)
    ]


def list_resultsexp_reg_files_for_run(run_folder, scenario_num, run_num):
    return [
        file_path
        for file_path in run_folder.iterdir()
        if is_matching_resultsexp_reg_filename(file_path, scenario_num, run_num)
    ]


def list_ymc_files_for_run(run_folder, scenario_num, run_num):
    return [
        file_path
        for file_path in run_folder.iterdir()
        if is_matching_ymc_filename(file_path, scenario_num, run_num)
    ]


def list_ymc_reg_files_for_run(run_folder, scenario_num, run_num):
    return [
        file_path
        for file_path in run_folder.iterdir()
        if is_matching_ymc_reg_filename(file_path, scenario_num, run_num)
    ]


def list_shockpars_files_for_run(run_folder, scenario_num, run_num):
    return [
        file_path
        for file_path in run_folder.iterdir()
        if is_matching_shockpars_filename(file_path, scenario_num, run_num)
    ]


def read_simulation_results_file(file_path):
    """Read one simulation output file into a DataFrame."""
    try:
        return pd.read_csv(file_path, sep=r"\s+", header=None)
    except Exception as exc:
        print(f"Skipping unreadable file {file_path}: {exc}")
        return None


def read_simple_yaml_column_names(yaml_path):
    """Read column names from simple YAML list lines: '- name: <column>'"""
    if yaml_path is None:
        return []

    yaml_path = Path(yaml_path)
    if not yaml_path.exists():
        print(f"YAML file not found at: {yaml_path}")
        return []

    column_names = []
    try:
        with yaml_path.open("r", encoding="utf-8") as handle:
            for line in handle:
                match = re.match(r"^\s*-\s*name:\s*(\S+)\s*$", line)
                if match:
                    column_names.append(match.group(1))
    except Exception as exc:
        print(f"Skipping YAML column names from {yaml_path}: {exc}")
        return []

    return column_names


def read_resultsexp_column_names(yaml_path="/Users/anmolsoni/Documents/RegionalDSKSFC/schema/resultsexp.yaml"):
    return read_simple_yaml_column_names(yaml_path)


def read_ymc_column_names(yaml_path="/Users/anmolsoni/Documents/RegionalDSKSFC/schema/ymc.yaml"):
    return read_simple_yaml_column_names(yaml_path)


def read_ymc_reg_column_names(yaml_path="/Users/anmolsoni/Documents/RegionalDSKSFC/schema/ymc_reg.yaml"):
    return read_simple_yaml_column_names(yaml_path)


def read_resultsexp_reg_column_names(yaml_path="/Users/anmolsoni/Documents/RegionalDSKSFC/schema/resultsexp_reg.yaml"):
    return read_simple_yaml_column_names(yaml_path)


def read_shockpars_column_names(yaml_path="/Users/anmolsoni/Documents/RegionalDSKSFC/schema/shockpars.yaml", NR=None):
    """Generate shockpars columns from schema; NR is number of regions."""
    if yaml_path is None:
        return []
    if NR is None:
        raise ValueError("NR must be provided to generate shockpars column names")

    yaml_path = Path(yaml_path)
    if not yaml_path.exists():
        raise FileNotFoundError(f"YAML file not found at: {yaml_path}")

    with yaml_path.open("r", encoding="utf-8") as handle:
        schema = yaml.safe_load(handle)

    column_names = [column["name"] for column in schema.get("fixed_columns", [])]

    parameters = schema.get("parameters", {})
    nshocks = parameters.get("nshocks")
    if nshocks is None:
        raise ValueError("`nshocks` must be defined in shockpars schema under `parameters`")

    regional_blocks = schema.get("regional_blocks", {})
    block_order = regional_blocks.get("order", [])

    for region in range(1, int(NR) + 1):
        for block_name in block_order:
            block = regional_blocks[block_name]
            pattern = block["name_pattern"]
            for shock in range(1, int(nshocks) + 1):
                column_names.append(pattern.format(shock=shock, region=region))

    return column_names


def apply_column_names(df, column_names, id_columns, verbose=False):
    """Apply data column names while preserving id columns at the end."""
    if not column_names:
        if verbose:
            print("No YAML column names loaded; leaving numeric columns unchanged.")
        return df

    actual_data_cols = df.shape[1] - len(id_columns)
    expected_data_cols = len(column_names)
    if actual_data_cols != expected_data_cols and verbose:
        print(
            "Column count mismatch: "
            f"data_cols={actual_data_cols}, yaml_cols={expected_data_cols}."
        )

    if actual_data_cols > expected_data_cols:
        extra_cols = [f"col_{idx}" for idx in range(expected_data_cols, actual_data_cols)]
        data_cols = column_names + extra_cols
    else:
        data_cols = column_names[:actual_data_cols]

    all_cols = data_cols + id_columns
    # Deduplicate: suffix repeated names with _1, _2, ... to prevent InvalidIndexError
    # in downstream pd.concat when DataFrames have different column sets.
    seen: dict[str, int] = {}
    deduped = []
    for col in all_cols:
        if col in seen:
            seen[col] += 1
            new_name = f"{col}_{seen[col]}"
            if verbose:
                print(f"Duplicate column '{col}' renamed to '{new_name}'.")
            deduped.append(new_name)
        else:
            seen[col] = 0
            deduped.append(col)

    df.columns = deduped
    return df


def transform_shockpars_wide_to_long(shock_df, scenario_num, run_num):
    """
    Transform wide shockpars dataframe to long format with columns:
    t, X_a_1..X_a_9, X_b_1..X_b_9, region, run, scenario

    - Macro columns: X_a_<shock>, X_b_<shock> -> region='macro'
    - Regional columns: X_a_reg_<shock>_<region>, X_b_reg_<shock>_<region> -> region=<region>
    """
    if shock_df is None or shock_df.empty:
        return pd.DataFrame()

    shock_range = range(1, 10)

    if "t" in shock_df.columns:
        t_series = shock_df["t"]
    else:
        t_series = pd.Series(np.arange(len(shock_df)), index=shock_df.index)

    macro_payload = {"t": t_series}
    for shock in shock_range:
        macro_payload[f"X_a_{shock}"] = shock_df[f"X_a_{shock}"] if f"X_a_{shock}" in shock_df.columns else np.nan
        macro_payload[f"X_b_{shock}"] = shock_df[f"X_b_{shock}"] if f"X_b_{shock}" in shock_df.columns else np.nan

    macro_df = pd.DataFrame(macro_payload)
    macro_df["region"] = "macro"
    macro_df["run"] = int(run_num)
    macro_df["scenario"] = int(scenario_num)

    regional_frames = []
    region_set = set()
    reg_pattern = re.compile(r"^X_[ab]_reg_(\d+)_(\d+)$")

    for column_name in shock_df.columns:
        match = reg_pattern.match(str(column_name))
        if match:
            region_set.add(int(match.group(2)))

    for region in sorted(region_set):
        payload = {"t": t_series}
        for shock in shock_range:
            xa_reg = f"X_a_reg_{shock}_{region}"
            xb_reg = f"X_b_reg_{shock}_{region}"
            payload[f"X_a_{shock}"] = shock_df[xa_reg] if xa_reg in shock_df.columns else np.nan
            payload[f"X_b_{shock}"] = shock_df[xb_reg] if xb_reg in shock_df.columns else np.nan

        regional_df = pd.DataFrame(payload)
        regional_df["region"] = int(region)
        regional_df["run"] = int(run_num)
        regional_df["scenario"] = int(scenario_num)
        regional_frames.append(regional_df)

    if regional_frames:
        return pd.concat([macro_df] + regional_frames, ignore_index=True)
    return macro_df


def load_simulation_results_matrix(base_path, verbose=True):
    """
    Build a consolidated DataFrame of simulation runs under base_path.

    For each scenario, the merged result (base data + shock panel) is written to
    ``<scenario_folder>/scenario_<N>.csv.gz`` (gzip-compressed CSV) before the
    scenario DataFrame is added to the global accumulator.  The function still
    returns the full combined DataFrame for in-memory use.
    """
    base_path = Path(base_path)
    if not base_path.exists() or not base_path.is_dir():
        raise ValueError(f"Invalid base directory: {base_path}")

    all_scenario_dfs = []

    scenario_folders = [path for path in base_path.iterdir() if path.is_dir()]
    if verbose:
        print(f"Found {len(scenario_folders)} scenario candidate folders under {base_path}")

    for scenario_folder in sorted(scenario_folders, key=lambda path: path.name):
        scenario_num = infer_numeric_id(scenario_folder.name)
        if scenario_num is None:
            if verbose:
                print(f"Skipping scenario folder without numeric ID: {scenario_folder.name}")
            continue

        run_folders = [path for path in scenario_folder.iterdir() if path.is_dir()]
        run_ids_in_scenario = []
        macro_files_loaded = 0
        region_files_loaded = 0
        shock_files_loaded = 0
        max_region_in_scenario = None

        # Per-scenario accumulators (shock frames scoped here so each scenario
        # only merges its own shocks).
        scenario_data_frames = []
        scenario_shock_frames = []

        for run_folder in sorted(run_folders, key=lambda path: path.name):
            run_num = infer_numeric_id(run_folder.name)
            if run_num is None:
                continue
            run_ids_in_scenario.append(run_num)

            resultsexp_files = list_resultsexp_files_for_run(
                run_folder,
                scenario_num=scenario_num,
                run_num=run_num,
            )

            if not resultsexp_files:
                ymc_files = list_ymc_files_for_run(
                    run_folder,
                    scenario_num=scenario_num,
                    run_num=run_num,
                )

                for ymc_file in ymc_files:
                    df = read_simulation_results_file(ymc_file)
                    if df is None or df.empty:
                        continue

                    df["scenario"] = scenario_num
                    df["run"] = run_num
                    df["region"] = "macro"
                    df = apply_column_names(
                        df,
                        column_names=read_ymc_column_names(),
                        id_columns=["scenario", "run", "region"],
                        verbose=verbose,
                    )
                    scenario_data_frames.append(df)
                    macro_files_loaded += 1

                if ymc_files:
                    ymc_reg_files = list_ymc_reg_files_for_run(
                        run_folder,
                        scenario_num=scenario_num,
                        run_num=run_num,
                    )
                    for ymc_reg_file in ymc_reg_files:
                        parsed_reg = parse_ymc_reg_filename(ymc_reg_file.name)
                        if parsed_reg is None:
                            continue

                        df = read_simulation_results_file(ymc_reg_file)
                        if df is None or df.empty:
                            continue

                        df["scenario"] = scenario_num
                        df["run"] = run_num
                        df["region"] = parsed_reg["region"]
                        df = apply_column_names(
                            df,
                            column_names=read_ymc_reg_column_names(),
                            id_columns=["scenario", "run", "region"],
                            verbose=verbose,
                        )
                        scenario_data_frames.append(df)
                        region_files_loaded += 1
                        if max_region_in_scenario is None or parsed_reg["region"] > max_region_in_scenario:
                            max_region_in_scenario = parsed_reg["region"]

            else:
                for resultsexp_file in resultsexp_files:
                    df = read_simulation_results_file(resultsexp_file)
                    if df is None or df.empty:
                        continue

                    df["scenario"] = scenario_num
                    df["run"] = run_num
                    df["region"] = "macro"
                    df = apply_column_names(
                        df,
                        column_names=read_resultsexp_column_names(),
                        id_columns=["scenario", "run", "region"],
                        verbose=verbose,
                    )
                    scenario_data_frames.append(df)
                    macro_files_loaded += 1

                resultsexp_reg_files = list_resultsexp_reg_files_for_run(
                    run_folder,
                    scenario_num=scenario_num,
                    run_num=run_num,
                )
                for resultsexp_reg_file in resultsexp_reg_files:
                    parsed_reg = parse_resultsexp_reg_filename(resultsexp_reg_file.name)
                    if parsed_reg is None:
                        continue

                    df = read_simulation_results_file(resultsexp_reg_file)
                    if df is None or df.empty:
                        continue

                    df["scenario"] = scenario_num
                    df["run"] = run_num
                    df["region"] = parsed_reg["region"]
                    df = apply_column_names(
                        df,
                        column_names=read_resultsexp_reg_column_names(),
                        id_columns=["scenario", "run", "region"],
                        verbose=verbose,
                    )
                    scenario_data_frames.append(df)
                    region_files_loaded += 1
                    if max_region_in_scenario is None or parsed_reg["region"] > max_region_in_scenario:
                        max_region_in_scenario = parsed_reg["region"]

                shockpars_files = list_shockpars_files_for_run(
                    run_folder,
                    scenario_num=scenario_num,
                    run_num=run_num,
                )
                for shockpars_file in shockpars_files:
                    shock_df = read_simulation_results_file(shockpars_file)
                    if shock_df is None or shock_df.empty:
                        continue

                    nr = int(max_region_in_scenario) if max_region_in_scenario is not None else 0
                    shock_columns = read_shockpars_column_names(NR=nr)

                    if shock_columns:
                        actual_data_cols = shock_df.shape[1]
                        expected_data_cols = len(shock_columns)
                        if actual_data_cols != expected_data_cols and verbose:
                            print(
                                "Column count mismatch: "
                                f"data_cols={actual_data_cols}, yaml_cols={expected_data_cols}."
                            )
                        if actual_data_cols > expected_data_cols:
                            extra_cols = [f"col_{idx}" for idx in range(expected_data_cols, actual_data_cols)]
                            shock_df.columns = shock_columns + extra_cols
                        else:
                            shock_df.columns = shock_columns[:actual_data_cols]
                    elif verbose:
                        print("No YAML column names loaded for shockpars; skipping transformation.")
                        continue

                    shock_long_df = transform_shockpars_wide_to_long(
                        shock_df,
                        scenario_num=scenario_num,
                        run_num=run_num,
                    )
                    if not shock_long_df.empty:
                        scenario_shock_frames.append(shock_long_df)
                        shock_files_loaded += 1

        if verbose:
            max_run = max(run_ids_in_scenario) if run_ids_in_scenario else None
            print(
                f"Scenario {scenario_num}: run_folders={len(run_ids_in_scenario)}, "
                f"max_run={max_run}, macro_files_loaded={macro_files_loaded}, "
                f"region_files_loaded={region_files_loaded}, max_region={max_region_in_scenario}, "
                f"shock_files_loaded={shock_files_loaded}"
            )

        if not scenario_data_frames:
            if verbose:
                print(f"  Scenario {scenario_num}: no data frames collected.")
            continue

        # Build scenario-level DataFrame and merge its shock panel.
        # Guard: detect any frames with duplicate column names before concat;
        # pd.concat raises InvalidIndexError when column reindexing is required
        # and any individual frame's column index is non-unique.
        for _i, _df in enumerate(scenario_data_frames):
            _dupes = _df.columns[_df.columns.duplicated(keep=False)].unique().tolist()
            if _dupes:
                print(
                    f"  WARNING scenario {scenario_num}: frame {_i} "
                    f"(region={_df['region'].iloc[0] if 'region' in _df.columns else '?'}, "
                    f"run={_df['run'].iloc[0] if 'run' in _df.columns else '?'}) "
                    f"has duplicate columns {_dupes}; dropping duplicates (keeping first)."
                )
                scenario_data_frames[_i] = _df.loc[:, ~_df.columns.duplicated(keep='first')]
        scenario_df = pd.concat(scenario_data_frames, ignore_index=True)

        if scenario_shock_frames:
            shock_panel = pd.concat(scenario_shock_frames, ignore_index=True)
            merge_keys = ["scenario", "run", "region"]
            if "t" in scenario_df.columns and "t" in shock_panel.columns:
                merge_keys.append("t")

            candidate_cols = [col for col in shock_panel.columns if col not in merge_keys]
            add_cols = [col for col in candidate_cols if col not in scenario_df.columns]
            if add_cols:
                scenario_df = scenario_df.merge(
                    shock_panel[merge_keys + add_cols],
                    on=merge_keys,
                    how="left",
                )

        # Save this scenario's consolidated data to <scenario_folder>/scenario_<N>.csv.gz
        out_path = scenario_folder / f"scenario_{scenario_num}.csv.gz"
        try:
            scenario_df.to_csv(out_path, index=False, compression="gzip")
            if verbose:
                print(f"  Scenario {scenario_num}: saved to {out_path.name}")
        except Exception as exc:
            print(f"  Scenario {scenario_num}: failed to write csv.gz: {exc}")

        all_scenario_dfs.append(scenario_df)

    if not all_scenario_dfs:
        if verbose:
            print("No matching files found.")
        return pd.DataFrame()

    final_df = pd.concat(all_scenario_dfs, ignore_index=True)

    if verbose:
        print(f"Consolidated shape={final_df.shape}")
    return final_df


def check_regional_consistency(
    df,
    cols=None,
    group_keys=('scenario', 'run', 't'),
    macro_label='macro',
    alpha=0.05,
    rel_tol=0.01,
    abs_tol=1e-8,
    aggregate='sum',
    plot_style='violin',
    test='cohens_d',
    output=None,
):
    """
    Compare region != macro_label aggregate against macro values for each variable.

    Parameters
    ----------
    cols : list[str] | dict[str, list[str]]
        - list: all variables use `aggregate` mode ('sum' or 'mean').
        - dict: supports keys 'aggregate' (sum) and 'mean' (mean across regions).
          Useful for rate variables (e.g., EmploymentRate) that should be averaged,
          not summed, across regions.
    plot_style : str | None
        How to visualise each variable.  One of:
          ``'violin'``  — violin with inner box (default)
          ``'box'``     — notched box-and-whisker
          ``None``      — skip plotting
    test : str
        Statistical check reported in the summary table and subplot titles.
        One of:
          ``'cohens_d'`` — Cohen\'s d effect size (default); |d| < 0.2 passes.
                          Unaffected by sample size; recommended for large n.
          ``'ttest'``   — Welch\'s two-sample t-test (unequal variances); p > alpha passes.
                          Warning: with large n (>1000) nearly always rejects H0
                          even for negligible differences.

    Returns
    -------
    summary_df : pd.DataFrame
        One row per variable with error metrics and statistical-test columns.
    comparison_df : pd.DataFrame
        Row-level merged data with aggregated and macro columns plus diffs.
    """
    group_keys = [k for k in group_keys if k in df.columns]
    if not group_keys:
        raise ValueError('No valid group keys found in dataframe.')
    if cols is None:
        shock_cols = ['X_a_1', 'X_b_1', 'X_a_2', 'X_b_2', 'X_a_3', 'X_b_3',
                      'X_a_4', 'X_b_4', 'X_a_5', 'X_b_5', 'X_a_6', 'X_b_6', 'X_a_7', 'X_b_7',
                      'X_a_8', 'X_b_8', 'X_a_9', 'X_b_9']
        if df[shock_cols].notna().values.any():
            cols = {'mean': ['CreditDemand_over_Supply',
                             'exit_marketshare2', 'EmploymentRate',
                             ],
                    'aggregate': ['GDP_r', 'Consumption_r', 'CapitalStock',
                                  'Loans_2', 'D_en_TOT', 'Emiss_TOT', 'LS',
                                  'Emiss1_TOT', 'Emiss2_TOT', 'Emiss_en', 'K_tot', 'exit_payments2', 'exit_equity2', 'exiting_1', 'ReplacementInvestment_r', 'EnergyPayments',
                                  'Pitot1', 'Pitot2', 'Dividends_1', 'Dividends_2',
                                  'Consumption', 'exit_total', 'Pitot_total', 'Dividends_firms_real', 'Dividends_b', 'Am_en', 'Am_a', 'GDP_n',
                                  ]}
        else:
            cols = {
                'aggregate': ['GDP_r', 'Consumption_r', 'Loans_2', 'D_en_TOT',
                              'Emiss_TOT', 'Q1tot', 'Q2tot', 'N1r', 'N2r', 'LS',
                              'Q_ge', 'Q_de', 'Emiss1_TOT', 'Emiss2_TOT', 'Emiss_en'],
                'mean': ['EmploymentRate']
            }

    if not isinstance(cols, (list, tuple, dict)):
        raise TypeError('cols must be a list/tuple or a dict with aggregate rules.')

    if isinstance(cols, dict):
        aggregate_cols = list(cols.get('aggregate', []))
        mean_cols = list(cols.get('mean', []))
        requested_cols = aggregate_cols + mean_cols
    else:
        requested_cols = list(cols)
        if aggregate not in ('sum', 'mean'):
            raise ValueError("aggregate must be either 'sum' or 'mean'.")
        aggregate_cols = requested_cols if aggregate == 'sum' else []
        mean_cols = requested_cols if aggregate == 'mean' else []

    missing_cols = sorted(set(requested_cols) - set(df.columns))
    if missing_cols:
        print(f"Warning: skipping missing columns: {missing_cols}")

    aggregate_cols = [c for c in aggregate_cols if c in df.columns]
    mean_cols = [c for c in mean_cols if c in df.columns]
    required_cols = aggregate_cols + [c for c in mean_cols if c not in aggregate_cols]

    if not required_cols:
        raise ValueError('None of the requested columns exist in dataframe.')

    is_macro = df['region'].astype(str).eq(str(macro_label))
    macro_df = df.loc[is_macro, group_keys + required_cols].copy()
    regional_df = df.loc[~is_macro, group_keys + required_cols].copy()

    agg_df = regional_df[group_keys].drop_duplicates().copy()

    if aggregate_cols:
        sum_df = regional_df.groupby(group_keys, as_index=False)[aggregate_cols].sum()
        agg_df = agg_df.merge(sum_df, on=group_keys, how='left')

    if mean_cols:
        mean_df = regional_df.groupby(group_keys, as_index=False)[mean_cols].mean()
        agg_df = agg_df.merge(mean_df, on=group_keys, how='left')

    comp = agg_df.merge(
        macro_df,
        on=group_keys,
        how='inner',
        suffixes=('_agg_regions', '_macro')
    )

    rows = []
    for col in required_cols:
        x = comp[f'{col}_agg_regions'].astype(float)
        y = comp[f'{col}_macro'].astype(float)
        diff = x - y
        denom = np.maximum(np.abs(y), abs_tol)
        rel_diff = diff / denom

        try:
            t_pvalue = stats.ttest_ind(x.dropna(), y.dropna(), equal_var=False, nan_policy='omit').pvalue
        except Exception:
            t_pvalue = np.nan

        # Cohen's d for paired differences: effect size unaffected by sample size.
        # |d| < 0.2 is the conventional threshold for a negligible effect.
        d_std = float(diff.std(ddof=1))
        cohens_d = float(diff.mean() / d_std) if d_std > 0 else 0.0

        # Derive the primary pass/fail and label depending on chosen test
        _use_ttest = str(test).lower() == 'ttest'
        if _use_ttest:
            stat_value  = float(t_pvalue) if pd.notna(t_pvalue) else np.nan
            passes_stat = bool(pd.notna(t_pvalue) and t_pvalue > alpha)
            stat_label  = 'p'
        else:
            stat_value  = cohens_d
            passes_stat = bool(abs(cohens_d) < 0.2)
            stat_label  = 'd'

        try:
            ks_pvalue = stats.ks_2samp(x.dropna(), y.dropna(), method='asymp').pvalue
        except Exception:
            ks_pvalue = np.nan

        valid = x.notna() & y.notna()
        if valid.sum() > 1 and x[valid].std() > 0 and y[valid].std() > 0:
            corr = float(x[valid].corr(y[valid]))
        else:
            corr = np.nan

        close_ratio = np.isclose(x, y, rtol=rel_tol, atol=abs_tol, equal_nan=True).mean()
        mean_abs_pct_err = (np.abs(rel_diff).mean()) * 100.0

        rows.append({
            'variable': col,
            'n_obs': int(diff.notna().sum()),
            'mean_bias': float(diff.mean()),
            'mae': float(np.abs(diff).mean()),
            'rmse': float(np.sqrt(np.mean(np.square(diff)))),
            'mean_abs_pct_err': float(mean_abs_pct_err),
            'max_abs_pct_err': float(np.abs(rel_diff).max() * 100.0),
            'corr': corr,
            'welch_t_pvalue': float(t_pvalue) if pd.notna(t_pvalue) else np.nan,
            'cohens_d': cohens_d,
            'ks_pvalue': float(ks_pvalue) if pd.notna(ks_pvalue) else np.nan,
            'close_ratio': float(close_ratio),
            'passes_tol_check': bool(close_ratio >= (1.0 - alpha)),
            'passes_stat_check': passes_stat,
        })

        comp[f'{col}_diff'] = diff
        comp[f'{col}_rel_diff'] = rel_diff

    summary_df = pd.DataFrame(rows).sort_values('variable').reset_index(drop=True)

    _VALID_STYLES = ('violin', 'box')
    _style = str(plot_style).lower() if plot_style else None
    if _style in _VALID_STYLES:
        vars_to_plot = summary_df['variable'].tolist()
        n_vars = len(vars_to_plot)
        ncols = 3
        nrows = int(np.ceil(n_vars / ncols))

        _colors = ['tab:blue', 'tab:orange']
        _labels = ['macro', 'agg_regions']

        fig, axes = plt.subplots(nrows, ncols, figsize=(4.5 * ncols, 4.2 * nrows))
        axes = np.array(axes).reshape(-1)

        for idx, var in enumerate(vars_to_plot):
            ax = axes[idx]
            col_agg   = f'{var}_agg_regions'
            col_macro = f'{var}_macro'

            if col_agg not in comp.columns or col_macro not in comp.columns:
                ax.set_visible(False)
                continue

            macro_vals = comp[col_macro].replace([np.inf, -np.inf], np.nan).dropna().values
            agg_vals   = comp[col_agg].replace([np.inf, -np.inf], np.nan).dropna().values

            if len(macro_vals) == 0 or len(agg_vals) == 0:
                ax.set_visible(False)
                continue

            # Auto log10: apply when all values are strictly positive and
            # the combined range spans > 2 orders of magnitude (max/min > 100).
            all_raw = np.concatenate([macro_vals, agg_vals])
            use_log = (
                bool((all_raw > 0).all())
                and float(all_raw.max()) / float(all_raw.min()) > 100
            )
            if use_log:
                macro_vals = np.log10(macro_vals)
                agg_vals   = np.log10(agg_vals)
            y_label = 'log₁₀(Value)' if use_log else 'Value'

            data = [macro_vals, agg_vals]

            if _style == 'violin':
                from scipy.stats import gaussian_kde

                all_vals = np.concatenate([macro_vals, agg_vals])
                v_min, v_max = all_vals.min(), all_vals.max()
                if v_min == v_max:
                    v_min, v_max = v_min - 1, v_max + 1
                y_grid = np.linspace(v_min, v_max, 300)
                half_w = 0.38

                # Left half = macro, right half = agg_regions
                for vals, sign, color in [
                    (macro_vals, -1, _colors[0]),
                    (agg_vals,   +1, _colors[1]),
                ]:
                    if len(vals) < 2 or np.std(vals) == 0:
                        continue
                    density = gaussian_kde(vals)(y_grid)
                    density = density / density.max() * half_w
                    x_outer = sign * density
                    ax.fill_betweenx(y_grid, 0, x_outer,
                                     color=color, alpha=0.45)
                    ax.plot(x_outer, y_grid,
                            color=color, linewidth=0.9, alpha=0.85)

                # Central box: combined IQR extent + per-series median ticks
                q25_L, med_L, q75_L = np.percentile(macro_vals, [25, 50, 75])
                q25_R, med_R, q75_R = np.percentile(agg_vals,   [25, 50, 75])
                q25_c = min(q25_L, q25_R)
                q75_c = max(q75_L, q75_R)
                iqr_c = q75_c - q25_c
                lo_w = max(all_vals.min(), q25_c - 1.5 * iqr_c)
                hi_w = min(all_vals.max(), q75_c + 1.5 * iqr_c)

                ax.plot([0, 0], [lo_w, q25_c],
                        color='black', linewidth=1.0, zorder=3)
                ax.plot([0, 0], [q75_c, hi_w],
                        color='black', linewidth=1.0, zorder=3)

                box_hw = 0.045
                rect = plt.Rectangle(
                    (-box_hw, q25_c), 2 * box_hw, iqr_c,
                    facecolor='white', edgecolor='black',
                    linewidth=1.0, zorder=4,
                )
                ax.add_patch(rect)
                # Left-half tick = macro median, right-half tick = agg_regions median
                ax.plot([-box_hw, 0], [med_L, med_L],
                        color=_colors[0], linewidth=2.2, zorder=5)
                ax.plot([0, box_hw], [med_R, med_R],
                        color=_colors[1], linewidth=2.2, zorder=5)

                ax.set_xlim(-half_w - 0.08, half_w + 0.08)
                ax.set_xticks([])

            else:  # 'box'
                bp = ax.boxplot(
                    data, positions=[1, 2], widths=0.55,
                    patch_artist=True, notch=True,
                    manage_ticks=False, showfliers=True,
                    whiskerprops=dict(linewidth=1.3),
                    capprops=dict(linewidth=1.3),
                    medianprops=dict(linewidth=2.2),
                    flierprops=dict(marker='o', markersize=3, alpha=0.45,
                                    linestyle='none'),
                )
                for patch, col in zip(bp['boxes'], _colors):
                    patch.set_facecolor(col)
                    patch.set_alpha(0.50)
                    patch.set_edgecolor(col)
                    patch.set_linewidth(1.5)
                for i, w in enumerate(bp['whiskers']):
                    w.set_color(_colors[i // 2])
                for i, c in enumerate(bp['caps']):
                    c.set_color(_colors[i // 2])
                for i, m in enumerate(bp['medians']):
                    m.set_color(_colors[i])
                for i, f in enumerate(bp['fliers']):
                    f.set(markerfacecolor=_colors[i], markeredgecolor=_colors[i])

            if _style != 'violin':
                ax.set_xticks([1, 2])
                ax.set_xticklabels(_labels, fontsize=8)
            ax.tick_params(axis='y', labelsize=7)
            ax.set_ylabel(y_label, fontsize=8)
            ax.grid(True, axis='y', linestyle='--', linewidth=0.5,
                    color='grey', alpha=0.4)
            ax.set_axisbelow(True)

            row = summary_df.loc[summary_df['variable'] == var].iloc[0]
            pass_tol  = '✓' if row['passes_tol_check'] else '✗'
            pass_stat = '✓' if row['passes_stat_check'] else '✗'
            _use_ttest = str(test).lower() == 'ttest'
            stat_str = (
                f"p={row['welch_t_pvalue']:.2g}({pass_stat})"
                if _use_ttest
                else f"d={row['cohens_d']:.3g}({pass_stat})"
            )
            ax.set_title(
                f"{var}\nMAPE={row['mean_abs_pct_err']:.3g}%  ",
                # f"tol={pass_tol}  {stat_str}",
                fontsize=8.5,
            )

            # Legend on first subplot only
            if idx == 0:
                handles = [
                    plt.Rectangle((0, 0), 1, 1, fc=_colors[0], alpha=0.55, label='macro'),
                    plt.Rectangle((0, 0), 1, 1, fc=_colors[1], alpha=0.55, label='agg_regions'),
                ]
                ax.legend(handles=handles, frameon=False, fontsize=8)

        for j in range(n_vars, len(axes)):
            axes[j].set_visible(False)

        # style_label = 'Violin' if _style == 'violin' else 'Box-and-Whisker'
        # plt.suptitle(
        #     f'Regional Aggregation Verification: {style_label} — Macro vs Aggregated Regional Values',
        #     y=1.01, fontsize=13,
        # )
        plt.tight_layout()
        if output is not None:
            plt.savefig(output, dpi=300, format='pdf', bbox_inches='tight')
        plt.show()
    print(summary_df)
    return None
