#!/usr/bin/env python3
"""
Calibrate eta_K_search in RegionalDSKSFC to a stylised European
sub-national intra-regional K-goods sourcing target.

Calibration target
------------------
Santamaria, Ventura and Yesilbayraktar (2023), "Exploring European
regional trade", Journal of International Economics 146, 103747,
report that home trade accounts for about 40% of European regional trade.

DOI:
    10.1016/j.jinteco.2023.103747

Their evidence covers goods trade across 269 European NUTS-2 regions in
24 countries. The 40% value is used here as a stylised regional home-trade
target, not as a machinery-specific empirical estimate.

Model calibration moment
------------------------
The model is quarterly. Four consecutive model periods are first summed
into one annual bilateral K-goods flow matrix. The primary calibration
statistic is the mean annual intra-regional K-goods purchase share:

    annual_intra_regional_k_share
        = annual intra-regional K-goods purchases / annual K-goods purchases

eta_K_search is selected to minimise:

    abs(mean_annual_intra_regional_k_share - target_intra_regional_k_share)

where the mean is taken across Monte Carlo replications after the burn-in.

The script calibrates only to realised K-goods purchases, not C-goods trade and not brochure draws.

Example
-------
Run from the repository root:

    python scripts/calibrate_eta_k.py \
        --eta-min 0 --eta-max 4 --eta-step 0.25 \
        --replications 50 --workers 4

Default stylised target:
    target_intra_regional_k_share = 0.40
"""

from __future__ import annotations

import argparse
import concurrent.futures
import copy
import json
import math
import os
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


# Regional output columns are resolved from schema/ymc_reg.yaml by name.
# This avoids hard-coded column offsets and explicitly separates K-goods
# (mach_buy_from_R*) from C-goods (cons_buy_from_R*).
DEFAULT_REGIONAL_SCHEMA = "schema/ymc_reg.yaml"
PERIODS_PER_YEAR = 4  # One model period is one quarter.


def load_regional_schema_names(schema_path):
    """Read column names from the simple ymc_reg.yaml schema without PyYAML."""
    path = Path(schema_path)
    if not path.exists():
        raise FileNotFoundError(f"Regional schema not found: {path}")

    names = []
    pattern = re.compile(r"^\s*-\s+name:\s*([^#\s]+)")
    with path.open("r", encoding="utf-8") as f:
        for line in f:
            match = pattern.match(line)
            if match:
                names.append(match.group(1).strip())

    if not names:
        raise ValueError(f"No column names found in regional schema: {path}")

    return names


@dataclass(frozen=True)
class RunSpec:
    eta: float
    replication: int
    seed: int


def intra_regional_k_share(flow):
    """Return the value-weighted intra-regional K-goods purchase share."""
    flow = np.asarray(flow, dtype=float)

    if flow.ndim != 2 or flow.shape[0] != flow.shape[1]:
        raise ValueError("Machine-flow matrix must be square.")
    if np.any(~np.isfinite(flow)) or np.any(flow < 0):
        raise ValueError("Machine-flow matrix contains invalid values.")

    total = float(flow.sum())
    if total <= 0:
        raise ValueError("No positive machine purchases in calibration window.")

    return float(np.trace(flow) / total)


def patch_input(base_input, eta):
    data = copy.deepcopy(base_input)
    data["flags"][0]["flag_regional_bias"] = 2
    data["flags"][0]["flag_validation"] = 0
    data["params"][0]["eta_K_search"] = float(eta)
    return data


def read_quarterly_k_flows(
    repo_root,
    run_name,
    seed,
    nr,
    burn_in,
    schema_names,
    consistency_tol=1e-7,
):
    """
    Read quarterly bilateral K-goods flows from mach_buy_from_R* columns only.

    C-goods flows (cons_buy_from_R*) are intentionally excluded.

    The function also verifies, period by period, that the bilateral K-flow
    matrix reproduces the independently written mach_import_share statistic.

    Returns
    -------
    quarterly_flows : dict[int, np.ndarray]
        Mapping t -> buyer-region x seller-region K-goods flow matrix.
    regional_files : list[Path]
        Regional output files used.
    max_consistency_error : float
        Maximum accounting-identity error across checked region-periods.
    checked_periods : int
        Number of region-period observations checked.
    """
    required_names = (
        ["t", "mach_import_share"]
        + [f"mach_buy_from_R{s}" for s in range(1, nr + 1)]
    )

    missing = [name for name in required_names if name not in schema_names]
    if missing:
        raise ValueError(
            "Regional schema is missing required K-goods columns: "
            + ", ".join(missing)
        )

    k_flow_names = [f"mach_buy_from_R{s}" for s in range(1, nr + 1)]

    quarterly_flows = {}
    regional_files = []
    max_consistency_error = 0.0
    checked_periods = 0
    time_sets = []

    for buyer_region in range(1, nr + 1):
        path = (
            repo_root
            / "output"
            / f"ymc_{buyer_region}_{run_name}_{seed}.txt"
        )
        regional_files.append(path)

        if not path.exists():
            raise FileNotFoundError(f"Missing output: {path}")

        df = pd.read_csv(path, sep=r"\s+", header=None)

        if df.shape[1] != len(schema_names):
            raise ValueError(
                f"{path.name} has {df.shape[1]} columns but "
                f"{len(schema_names)} are defined in the regional schema. "
                "Refusing to use positional guesses."
            )

        df.columns = schema_names
        df["t"] = pd.to_numeric(df["t"], errors="coerce")

        # Keep post-burn-in observations. Partial model-years are removed
        # later, after annual grouping.
        df = df.loc[df["t"] >= burn_in].copy()
        if df.empty:
            raise ValueError(
                f"No observations with t >= {burn_in} in {path.name}."
            )

        df["t"] = df["t"].astype(int)
        time_sets.append(set(df["t"].tolist()))

        flows = (
            df[k_flow_names]
            .apply(pd.to_numeric, errors="coerce")
            .fillna(0.0)
        )

        if (flows < -1e-12).any().any():
            raise ValueError(f"Negative K-goods bilateral flow in {path.name}.")

        # Independent accounting identity:
        # reported mach_import_share must equal off-diagonal K purchases
        # divided by total K purchases for this buyer region and quarter.
        local_name = f"mach_buy_from_R{buyer_region}"
        row_total = flows.sum(axis=1)
        local_flow = flows[local_name]
        positive = row_total > 1e-12

        if positive.any():
            implied_import_share = (
                (row_total[positive] - local_flow[positive])
                / row_total[positive]
            )

            reported_import_share = pd.to_numeric(
                df.loc[positive, "mach_import_share"],
                errors="coerce",
            )

            valid = reported_import_share.notna()
            if valid.any():
                err = np.abs(
                    implied_import_share.loc[valid].to_numpy()
                    - reported_import_share.loc[valid].to_numpy()
                )
                local_max = float(err.max()) if len(err) else 0.0
                max_consistency_error = max(
                    max_consistency_error,
                    local_max,
                )
                checked_periods += int(valid.sum())

                if local_max > consistency_tol:
                    raise ValueError(
                        f"K-flow accounting check failed in {path.name}: "
                        f"max |implied import share - reported "
                        f"mach_import_share| = {local_max:.3e} "
                        f"> {consistency_tol:.1e}."
                    )

        # Fill the buyer row of each quarterly bilateral matrix.
        flow_values = flows.to_numpy(dtype=float)
        times = df["t"].to_numpy(dtype=int)

        for idx, t_value in enumerate(times):
            if t_value not in quarterly_flows:
                quarterly_flows[t_value] = np.zeros((nr, nr), dtype=float)
            quarterly_flows[t_value][buyer_region - 1, :] = flow_values[idx, :]

    # Require the same set of post-burn-in quarters in every regional file.
    common_times = set.intersection(*time_sets)
    union_times = set.union(*time_sets)

    if common_times != union_times:
        raise ValueError(
            "Regional K-flow files do not contain identical time observations."
        )

    quarterly_flows = {
        t_value: quarterly_flows[t_value]
        for t_value in sorted(common_times)
    }

    return (
        quarterly_flows,
        regional_files,
        max_consistency_error,
        checked_periods,
    )


def annualize_k_flows(
    quarterly_flows,
    periods_per_year=PERIODS_PER_YEAR,
):
    """
    Aggregate quarterly K-goods flows into complete model-years.

    Model year y consists of periods:
        4(y-1)+1, ..., 4y

    Any partial year created by the burn-in or simulation endpoint is dropped.
    For each complete model-year, four quarterly bilateral matrices are summed
    first and the intra-regional K-goods share is then computed from that
    annual flow matrix.

    Returns
    -------
    annual_rows : list[dict]
        One row per complete model-year.
    annual_matrices : dict[int, np.ndarray]
        Annual bilateral K-flow matrices.
    """
    if periods_per_year != 4:
        raise ValueError(
            "This project defines one model period as one quarter; "
            "annual aggregation therefore requires 4 periods."
        )

    groups = {}
    for t_value, matrix in quarterly_flows.items():
        model_year = ((int(t_value) - 1) // periods_per_year) + 1
        groups.setdefault(model_year, []).append((int(t_value), matrix))

    annual_rows = []
    annual_matrices = {}

    for model_year in sorted(groups):
        items = sorted(groups[model_year], key=lambda x: x[0])
        expected_start = (model_year - 1) * periods_per_year + 1
        expected_times = list(
            range(expected_start, expected_start + periods_per_year)
        )
        actual_times = [t_value for t_value, _ in items]

        # Drop incomplete years, including a partial first year after burn-in.
        if actual_times != expected_times:
            continue

        annual_matrix = np.sum(
            [matrix for _, matrix in items],
            axis=0,
        )

        total = float(annual_matrix.sum())
        if total <= 0:
            # No realised K-goods trade in the entire model-year.
            continue

        share = intra_regional_k_share(annual_matrix)
        annual_matrices[model_year] = annual_matrix

        annual_rows.append(
            {
                "model_year": model_year,
                "t_start": expected_times[0],
                "t_end": expected_times[-1],
                "annual_intra_regional_k_share": share,
                "annual_total_k_purchases": total,
            }
        )

    if not annual_rows:
        raise ValueError(
            "No complete four-quarter model-years with positive K-goods "
            "purchases remain after the burn-in."
        )

    return annual_rows, annual_matrices



def eta_tag(eta):
    return f"{eta:.4f}".replace("-", "m").replace(".", "p")


def run_one(spec, args, base_input, repo_root, executable, output_dir):
    run_name = f"etaK_{eta_tag(spec.eta)}_rep{spec.replication:03d}"

    generated_inputs = output_dir / "generated_inputs"
    generated_inputs.mkdir(parents=True, exist_ok=True)
    input_path = generated_inputs / f"{run_name}.json"

    model_input = patch_input(base_input, spec.eta)
    nr = int(model_input["regions"]["NR"])

    with input_path.open("w", encoding="utf-8") as f:
        json.dump(model_input, f, indent=2)

    cmd = [
        str(executable),
        str(input_path),
        "-r", run_name,
        "-s", str(spec.seed),
        "-f", "0",
        "-c", "0",
        "-v", "0",
    ]

    result = subprocess.run(
        cmd,
        cwd=repo_root,
        capture_output=True,
        text=True,
    )

    if result.returncode != 0:
        return {
            "eta": spec.eta,
            "replication": spec.replication,
            "seed": spec.seed,
            "success": False,
            "error": result.stderr[-1500:],
            "annual_records": [],
        }

    try:
        (
            quarterly_flows,
            files,
            consistency_error,
            checked_periods,
        ) = read_quarterly_k_flows(
            repo_root=repo_root,
            run_name=run_name,
            seed=spec.seed,
            nr=nr,
            burn_in=args.burn_in,
            schema_names=args.schema_names,
            consistency_tol=args.consistency_tol,
        )

        annual_rows, annual_matrices = annualize_k_flows(
            quarterly_flows,
            periods_per_year=PERIODS_PER_YEAR,
        )

        annual_df = pd.DataFrame(annual_rows)

        # Primary calibration statistic:
        # arithmetic mean of complete annual K-goods home-trade shares.
        mean_annual_share = float(
            annual_df["annual_intra_regional_k_share"].mean()
        )

        # Secondary diagnostic closest to the paper's "average bilateral
        # matrix over years" construction. Summing all complete annual matrices
        # and then computing the share gives the trade-value-weighted pooled
        # post-burn-in share.
        pooled_matrix = np.sum(
            list(annual_matrices.values()),
            axis=0,
        )
        pooled_share = intra_regional_k_share(pooled_matrix)

        annual_records = []
        for row in annual_rows:
            annual_records.append(
                {
                    "eta": spec.eta,
                    "replication": spec.replication,
                    "seed": spec.seed,
                    **row,
                }
            )

    except Exception as exc:
        return {
            "eta": spec.eta,
            "replication": spec.replication,
            "seed": spec.seed,
            "success": False,
            "error": str(exc),
            "annual_records": [],
        }

    if not args.keep_raw:
        for path in files:
            path.unlink(missing_ok=True)
        (
            repo_root
            / "output"
            / f"ymc_{run_name}_{spec.seed}.txt"
        ).unlink(missing_ok=True)

    return {
        "eta": spec.eta,
        "replication": spec.replication,
        "seed": spec.seed,
        "success": True,
        "mean_annual_intra_regional_k_share": mean_annual_share,
        "pooled_intra_regional_k_share": pooled_share,
        "n_complete_model_years": int(len(annual_rows)),
        "total_k_purchases_complete_years": float(pooled_matrix.sum()),
        "k_flow_consistency_error": consistency_error,
        "k_flow_checked_periods": checked_periods,
        "annual_records": annual_records,
    }



def build_eta_grid(eta_min, eta_max, eta_step):
    if eta_min < 0 or eta_max < eta_min or eta_step <= 0:
        raise ValueError("Invalid eta grid.")

    n = int(math.floor((eta_max - eta_min) / eta_step + 1e-12))
    values = [eta_min + i * eta_step for i in range(n + 1)]
    if values[-1] < eta_max - 1e-10:
        values.append(eta_max)
    return [round(x, 10) for x in values]


def run_grid(
    eta_values,
    args,
    base_input,
    repo_root,
    executable,
    output_dir,
):
    # Common random numbers: each eta is evaluated on the same seed set.
    specs = [
        RunSpec(
            eta=eta,
            replication=rep,
            seed=args.seed_base + rep - 1,
        )
        for eta in eta_values
        for rep in range(1, args.replications + 1)
    ]

    rows = []
    annual_rows = []

    def store_result(row):
        annual_records = row.pop("annual_records", [])
        rows.append(row)
        annual_rows.extend(annual_records)

    def do_run(spec):
        return run_one(
            spec,
            args,
            base_input,
            repo_root,
            executable,
            output_dir,
        )

    if args.workers == 1:
        iterator = ((spec, do_run(spec)) for spec in specs)
        for idx, (spec, row) in enumerate(iterator, start=1):
            store_result(row)
            status = "OK" if row["success"] else "FAILED"
            print(
                f"[{idx:>3}/{len(specs)}] eta={spec.eta:.3f} "
                f"rep={spec.replication:03d} {status}"
            )
    else:
        with concurrent.futures.ThreadPoolExecutor(
            max_workers=args.workers
        ) as executor:
            futures = {
                executor.submit(do_run, spec): spec
                for spec in specs
            }

            for idx, future in enumerate(
                concurrent.futures.as_completed(futures),
                start=1,
            ):
                spec = futures[future]
                try:
                    row = future.result()
                except Exception as exc:
                    row = {
                        "eta": spec.eta,
                        "replication": spec.replication,
                        "seed": spec.seed,
                        "success": False,
                        "error": repr(exc),
                        "annual_records": [],
                    }

                store_result(row)
                status = "OK" if row["success"] else "FAILED"
                print(
                    f"[{idx:>3}/{len(specs)}] eta={spec.eta:.3f} "
                    f"rep={spec.replication:03d} {status}"
                )

    return pd.DataFrame(rows), pd.DataFrame(annual_rows)



def summarise(results, target_intra_regional_k_share):
    """
    Summarise run-level annual K-goods calibration moments by eta.

    Each successful replication contributes one observation:
    its mean annual intra-regional K-goods share across complete model-years.
    """
    good = results[results["success"] == True].copy()  # noqa: E712

    if good.empty:
        raise RuntimeError("All calibration runs failed.")

    summary = (
        good.groupby("eta", as_index=False)
        .agg(
            n=("mean_annual_intra_regional_k_share", "size"),
            mean_annual_k_share=(
                "mean_annual_intra_regional_k_share",
                "mean",
            ),
            sd_annual_k_share=(
                "mean_annual_intra_regional_k_share",
                "std",
            ),
            median_annual_k_share=(
                "mean_annual_intra_regional_k_share",
                "median",
            ),
            mean_pooled_k_share=(
                "pooled_intra_regional_k_share",
                "mean",
            ),
            mean_complete_model_years=(
                "n_complete_model_years",
                "mean",
            ),
            max_k_flow_consistency_error=(
                "k_flow_consistency_error",
                "max",
            ),
        )
        .sort_values("eta")
    )

    summary["sd_annual_k_share"] = (
        summary["sd_annual_k_share"].fillna(0.0)
    )
    summary["se_annual_k_share"] = (
        summary["sd_annual_k_share"] / np.sqrt(summary["n"])
    )

    summary["ci_low"] = (
        summary["mean_annual_k_share"]
        - 1.96 * summary["se_annual_k_share"]
    )
    summary["ci_high"] = (
        summary["mean_annual_k_share"]
        + 1.96 * summary["se_annual_k_share"]
    )

    summary["target_intra_regional_k_share"] = (
        target_intra_regional_k_share
    )
    summary["absolute_error"] = (
        summary["mean_annual_k_share"]
        - target_intra_regional_k_share
    ).abs()

    return summary


def summarise_annual_diagnostics(annual_results):
    """Monte Carlo summary for each eta x model-year annual K-goods share."""
    if annual_results.empty:
        return pd.DataFrame()

    grouped = (
        annual_results
        .groupby(["eta", "model_year"], as_index=False)
        .agg(
            n=("annual_intra_regional_k_share", "size"),
            mean_annual_k_share=(
                "annual_intra_regional_k_share",
                "mean",
            ),
            sd_annual_k_share=(
                "annual_intra_regional_k_share",
                "std",
            ),
            mean_annual_total_k_purchases=(
                "annual_total_k_purchases",
                "mean",
            ),
            t_start=("t_start", "first"),
            t_end=("t_end", "first"),
        )
        .sort_values(["eta", "model_year"])
    )

    grouped["sd_annual_k_share"] = (
        grouped["sd_annual_k_share"].fillna(0.0)
    )
    grouped["se_annual_k_share"] = (
        grouped["sd_annual_k_share"] / np.sqrt(grouped["n"])
    )
    grouped["ci_low"] = (
        grouped["mean_annual_k_share"]
        - 1.96 * grouped["se_annual_k_share"]
    )
    grouped["ci_high"] = (
        grouped["mean_annual_k_share"]
        + 1.96 * grouped["se_annual_k_share"]
    )

    return grouped


def save_plot(summary, target_intra_regional_k_share, path):
    """Save eta calibration curve using annual K-goods shares."""
    best = summary.loc[summary["absolute_error"].idxmin()]

    fig, ax = plt.subplots(figsize=(7, 4.5))

    ax.plot(
        summary["eta"],
        100 * summary["mean_annual_k_share"],
        marker="o",
        label="Mean annual K-goods share",
    )

    ax.fill_between(
        summary["eta"],
        100 * summary["ci_low"],
        100 * summary["ci_high"],
        alpha=0.2,
        label="95% MC interval",
    )

    ax.axhline(
        100 * target_intra_regional_k_share,
        linestyle="--",
        label="K-goods proxy target (40%)",
    )

    ax.scatter(
        [best["eta"]],
        [100 * best["mean_annual_k_share"]],
        s=70,
        zorder=5,
        label="Selected eta",
    )

    ax.set_xlabel(r"$\eta_K$")
    ax.set_ylabel("Annual intra-regional K-goods purchases (%)")
    ax.set_title(r"Calibration of $\eta_K$ using annual K-goods trade")
    ax.set_ylim(bottom=0)
    ax.grid(alpha=0.25)
    ax.legend()

    fig.tight_layout()
    fig.savefig(path, bbox_inches="tight")
    plt.close(fig)


def save_annual_diagnostic_plot(
    annual_summary,
    best_eta,
    target_intra_regional_k_share,
    path,
):
    """Plot annual K-goods home-trade shares through model time at best eta."""
    subset = annual_summary[
        np.isclose(annual_summary["eta"], best_eta)
    ].copy()

    if subset.empty:
        return

    fig, ax = plt.subplots(figsize=(8, 4.5))

    ax.plot(
        subset["model_year"],
        100 * subset["mean_annual_k_share"],
        linewidth=1.2,
        label="MC mean annual K-goods share",
    )

    ax.fill_between(
        subset["model_year"],
        100 * subset["ci_low"],
        100 * subset["ci_high"],
        alpha=0.2,
        label="95% MC interval",
    )

    ax.axhline(
        100 * target_intra_regional_k_share,
        linestyle="--",
        label="K-goods proxy target (40%)",
    )

    ax.set_xlabel("Model year (4 quarters)")
    ax.set_ylabel("Intra-regional K-goods purchases (%)")
    ax.set_title(
        rf"Annual K-goods sourcing diagnostic at $\eta_K={best_eta:g}$"
    )
    ax.set_ylim(bottom=0)
    ax.grid(alpha=0.25)
    ax.legend()

    fig.tight_layout()
    fig.savefig(path, bbox_inches="tight")
    plt.close(fig)



def parse_args():
    parser = argparse.ArgumentParser(
        description=(
            "Calibrate eta_K_search to a stylised European "
            "intra-regional K-goods purchase share."
        ),
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument(
        "--base-input",
        default="inputs/run/scenario1_inputs.json",
    )
    parser.add_argument(
        "--executable",
        default="./dsk_SFC",
    )
    parser.add_argument(
        "--output-dir",
        default="output/eta_k_calibration",
    )
    parser.add_argument(
        "--target-intra-regional-k-share",
        "--target-local-share",
        dest="target_intra_regional_k_share",
        type=float,
        default=0.40,
        help=(
            "Target intra-regional K-goods purchase share. Default 0.40 uses "
            "Santamaria et al. (2023) as a stylised proxy benchmark."
        ),
    )
    parser.add_argument("--eta-min", type=float, default=0.0)
    parser.add_argument("--eta-max", type=float, default=4.0)
    parser.add_argument("--eta-step", type=float, default=0.25)
    parser.add_argument("--replications", type=int, default=50)
    parser.add_argument("--seed-base", type=int, default=1)
    parser.add_argument("--burn-in", type=int, default=300)
    parser.add_argument("--workers", type=int, default=1)
    parser.add_argument(
        "--regional-schema",
        default=DEFAULT_REGIONAL_SCHEMA,
        help="Schema used to resolve regional output columns by name.",
    )
    parser.add_argument(
        "--consistency-tol",
        type=float,
        default=1e-7,
        help=(
            "Tolerance for the K-flow accounting identity check against "
            "mach_import_share."
        ),
    )
    parser.add_argument("--keep-raw", action="store_true")

    return parser.parse_args()


def main():
    args = parse_args()

    if not 0.0 < args.target_intra_regional_k_share < 1.0:
        raise ValueError(
            "--target-local-share must lie strictly between 0 and 1."
        )

    repo_root = Path.cwd().resolve()
    base_input_path = (repo_root / args.base_input).resolve()
    executable = (repo_root / args.executable).resolve()
    output_dir = (repo_root / args.output_dir).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    if not base_input_path.exists():
        raise FileNotFoundError(f"Base input not found: {base_input_path}")
    if not executable.exists():
        raise FileNotFoundError(
            f"Executable not found: {executable}. Compile the model first."
        )
    if not os.access(executable, os.X_OK):
        raise PermissionError(f"Not executable: {executable}")
    if args.replications < 1:
        raise ValueError("--replications must be >= 1.")
    if args.workers < 1:
        raise ValueError("--workers must be >= 1. Use 1 for serial execution.")

    with base_input_path.open("r", encoding="utf-8") as f:
        base_input = json.load(f)

    nr = int(base_input["regions"]["NR"])
    if nr < 2:
        raise ValueError("eta_K_search calibration requires NR >= 2.")

    schema_path = (repo_root / args.regional_schema).resolve()
    args.schema_names = load_regional_schema_names(schema_path)

    eta_values = build_eta_grid(
        args.eta_min,
        args.eta_max,
        args.eta_step,
    )

    print("=" * 68)
    print("eta_K_search calibration")
    print("=" * 68)
    print(f"Model regions:             {nr}")
    print(
        f"Target intra-regional K share:    "
        f"{args.target_intra_regional_k_share:.3f} "
        f"({100 * args.target_intra_regional_k_share:.1f}%)"
    )
    print(f"Calibration window:        t >= {args.burn_in}")
    print("Annual aggregation:        4 quarters per model year")
    print("Partial model-years:       dropped")
    print(f"Eta grid:                  {eta_values}")
    print(f"Replications/eta:          {args.replications}")
    print(f"Workers:                   {args.workers}")
    print(f"Regional schema:           {args.regional_schema}")
    print("K-flow columns:            "
          + ", ".join(f"mach_buy_from_R{s}" for s in range(1, nr + 1)))
    print("C-goods columns used:      none")
    print()

    results, annual_results = run_grid(
        eta_values,
        args,
        base_input,
        repo_root,
        executable,
        output_dir,
    )

    results.to_csv(
        output_dir / "replication_results.csv",
        index=False,
    )

    annual_results.to_csv(
        output_dir / "annual_replication_results.csv",
        index=False,
    )

    summary = summarise(
        results,
        args.target_intra_regional_k_share,
    )

    summary.to_csv(
        output_dir / "calibration_results.csv",
        index=False,
    )

    annual_summary = summarise_annual_diagnostics(annual_results)
    annual_summary.to_csv(
        output_dir / "annual_k_share_by_year.csv",
        index=False,
    )

    best = summary.loc[
        summary["absolute_error"].idxmin()
    ]

    best_eta = float(best["eta"])

    best_input = patch_input(
        base_input,
        best_eta,
    )

    with (output_dir / "best_input.json").open(
        "w",
        encoding="utf-8",
    ) as f:
        json.dump(
            best_input,
            f,
            indent=2,
        )

    save_plot(
        summary,
        args.target_intra_regional_k_share,
        output_dir / "calibration_curve.pdf",
    )

    save_annual_diagnostic_plot(
        annual_summary,
        best_eta,
        args.target_intra_regional_k_share,
        output_dir / "annual_k_share_diagnostic.pdf",
    )

    metadata = {
        "calibration_target": "mean annual intra-regional K-goods purchase share",
        "target_intra_regional_k_share": float(args.target_intra_regional_k_share),
        "target_source": (
            "Santamaria, Ventura and Yesilbayraktar (2023), "
            "Exploring European regional trade"
        ),
        "target_source_doi": "10.1016/j.jinteco.2023.103747",
        "target_interpretation": (
            "The reported 40% European regional home-trade share is used only "
            "as a proxy target for the model's K-goods sector. C-goods "
            "trade is excluded from the calibration statistic."
        ),
        "best_eta_K_search": best_eta,
        "best_simulated_mean_annual_k_share": float(best["mean_annual_k_share"]),
        "best_simulated_mean_pooled_k_share": float(best["mean_pooled_k_share"]),
        "absolute_error": float(best["absolute_error"]),
        "successful_replications_at_best_eta": int(best["n"]),
        "max_k_flow_consistency_error": float(
            best["max_k_flow_consistency_error"]
        ),
        "regional_schema": args.regional_schema,
        "k_flow_columns": [
            f"mach_buy_from_R{s}" for s in range(1, nr + 1)
        ],
        "c_goods_columns_used": [],
        "replications_per_eta": int(args.replications),
        "burn_in": int(args.burn_in),
        "periods_per_year": PERIODS_PER_YEAR,
        "annual_aggregation": (
            "Sum four quarterly bilateral K-flow matrices, then compute "
            "the annual intra-regional K-goods share. Partial years are dropped."
        ),
        "eta_grid": eta_values,
    }

    with (output_dir / "calibration_metadata.json").open(
        "w",
        encoding="utf-8",
    ) as f:
        json.dump(
            metadata,
            f,
            indent=2,
        )

    print()
    print("=" * 68)
    print("Calibration result")
    print("=" * 68)
    print(f"Selected eta_K_search:     {best_eta:.3f}")
    print(
        f"Target intra-regional K share:    "
        f"{args.target_intra_regional_k_share:.3f} "
        f"({100 * args.target_intra_regional_k_share:.1f}%)"
    )
    print(
        f"Simulated mean annual K share:    "
        f"{best['mean_annual_k_share']:.3f} "
        f"({100 * best['mean_annual_k_share']:.1f}%)"
    )
    print(
        f"Simulated pooled K share:         "
        f"{best['mean_pooled_k_share']:.3f} "
        f"({100 * best['mean_pooled_k_share']:.1f}%)"
    )
    print(
        f"Complete model-years/run:         "
        f"{best['mean_complete_model_years']:.1f}"
    )
    print(f"Absolute error:                   {best['absolute_error']:.3f}")
    print(f"Successful replications:   {int(best['n'])}")
    print(
        f"Max K-flow consistency error: "
        f"{best['max_k_flow_consistency_error']:.3e}"
    )
    print()
    print(f"Results: {output_dir}")


if __name__ == "__main__":
    main()
