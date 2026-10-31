#!/usr/bin/env python3
"""
Interactive K-goods trade matrices across eta_K_search calibration values.

Rows:
    C-firm buyer region

Columns:
    K-firm supplier region

Cells:
    Share of buyer-region K-goods purchases sourced from each supplier
    region, in percent.

Diagonal:
    Intra-regional K-goods sourcing.

Method
------
For each eta_K_search:

1. Load all successful calibration replications.
2. For each run and buyer region, average mach_buy_from_R* over t=300..700.
3. Convert each buyer row to sourcing shares summing to 100%.
4. Monte Carlo average the run-level matrices.
5. Display the resulting 3x3 matrix with an interactive Plotly dropdown.

This deliberately excludes all cons_buy_from_R* columns.
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path

import numpy as np
import pandas as pd
import plotly.graph_objects as go


# ============================================================
# CONFIGURATION
# ============================================================

DEFAULT_CALIBRATION_DIR = "output/eta_k_calibration"
DEFAULT_SCHEMA = "schema/ymc_reg.yaml"

TMIN = 300
TMAX = 700


# ============================================================
# HELPERS
# ============================================================


def eta_tag(eta):
    """
    Match the eta label used by calibrate_eta_k.py.

    Example:
        0.25 -> 0p2500
    """
    return f"{eta:.4f}".replace("-", "m").replace(".", "p")


def load_schema_names(path):
    """
    Read column names from schema/ymc_reg.yaml.

    PyYAML is deliberately not required.
    """

    path = Path(path)

    if not path.exists():
        raise FileNotFoundError(f"Regional schema not found: {path}")

    pattern = re.compile(r"^\s*-\s+name:\s*([^#\s]+)")

    names = []

    with path.open(
        "r",
        encoding="utf-8",
    ) as f:
        for line in f:
            match = pattern.match(line)

            if match:
                names.append(match.group(1).strip())

    if not names:
        raise ValueError(f"No column names found in {path}")

    return names


def infer_regions(schema_names):
    """
    Infer regional IDs from mach_buy_from_R* columns.
    """

    regions = []

    for name in schema_names:
        match = re.fullmatch(
            r"mach_buy_from_R(\d+)",
            name,
        )

        if match:
            regions.append(int(match.group(1)))

    regions = sorted(set(regions))

    if not regions:
        raise ValueError("No mach_buy_from_R* columns found in regional schema.")

    return regions


def region_labels(regions):
    """
    Use the same labels as stylised_facts_cleaned.py.
    """

    if regions == [1, 2, 3]:
        return {
            1: "North",
            2: "Central",
            3: "South",
        }

    return {r: f"R{r}" for r in regions}


# ============================================================
# CALIBRATION RUN TABLE
# ============================================================


def load_successful_runs(
    calibration_dir,
):

    path = Path(calibration_dir) / "replication_results.csv"

    if not path.exists():
        raise FileNotFoundError(f"Cannot find {path}")

    df = pd.read_csv(path)

    required = {
        "eta",
        "replication",
        "seed",
        "success",
    }

    missing = required - set(df.columns)

    if missing:
        raise ValueError(
            f"Missing columns in replication_results.csv: {sorted(missing)}"
        )

    success = df["success"]

    # Handle CSV strings such as "True"/"False".
    if success.dtype == object:
        success = (
            success.astype(str)
            .str.strip()
            .str.lower()
            .map(
                {
                    "true": True,
                    "false": False,
                }
            )
        )

    df = df.loc[
        success == True  # noqa: E712
    ].copy()

    if df.empty:
        raise RuntimeError("No successful calibration replications found.")

    df["eta"] = pd.to_numeric(
        df["eta"],
        errors="raise",
    )

    df["replication"] = pd.to_numeric(
        df["replication"],
        errors="raise",
    ).astype(int)

    df["seed"] = pd.to_numeric(
        df["seed"],
        errors="raise",
    ).astype(int)

    return df.sort_values(
        [
            "eta",
            "replication",
        ]
    ).reset_index(drop=True)


# ============================================================
# READ ONE CALIBRATION RUN
# ============================================================


def read_run_matrix(
    repo_root,
    eta,
    replication,
    seed,
    regions,
    schema_names,
    tmin,
    tmax,
):
    """
    Construct one run-level K-goods sourcing matrix.

    This follows the logic in stylised_facts_cleaned.py:

        time-average nominal flows
        -> row-normalise
        -> sourcing percentages

    Rows:
        buyer region

    Columns:
        K-firm supplier region
    """

    nr = len(regions)

    k_cols = [f"mach_buy_from_R{r}" for r in regions]

    run_name = f"etaK_{eta_tag(eta)}_rep{replication:03d}"

    matrix = np.zeros(
        (nr, nr),
        dtype=float,
    )

    for buyer in regions:
        path = repo_root / "output" / (f"ymc_{buyer}_{run_name}_{seed}.txt")

        if not path.exists():
            raise FileNotFoundError(str(path))

        df = pd.read_csv(
            path,
            sep=r"\s+",
            header=None,
        )

        if df.shape[1] != len(schema_names):
            raise ValueError(
                f"{path.name}: "
                f"{df.shape[1]} columns found, "
                f"{len(schema_names)} expected."
            )

        df.columns = schema_names

        required = [
            "t",
            *k_cols,
        ]

        missing = [col for col in required if col not in df.columns]

        if missing:
            raise ValueError(f"{path.name} missing columns: {missing}")

        df["t"] = pd.to_numeric(
            df["t"],
            errors="coerce",
        )

        df = df.loc[
            (df["t"] >= tmin) & (df["t"] <= tmax),
            required,
        ].copy()

        if df.empty:
            raise ValueError(f"No observations t={tmin}..{tmax} in {path.name}")

        flows = (
            df[k_cols]
            .apply(
                pd.to_numeric,
                errors="coerce",
            )
            .fillna(0.0)
        )

        if (flows < -1e-12).any().any():
            raise ValueError(f"Negative K-goods flow in {path.name}")

        # Same operation as:
        #
        # buy.groupby(["run","region"])[buy_cols].mean()
        #
        # in stylised_facts_cleaned.py.
        matrix[buyer - 1, :] = flows.mean(axis=0).to_numpy()

    # --------------------------------------------------------
    # Convert nominal K-goods flows into buyer sourcing shares.
    # --------------------------------------------------------

    row_total = matrix.sum(
        axis=1,
        keepdims=True,
    )

    shares = np.full_like(
        matrix,
        np.nan,
    )

    valid = row_total[:, 0] > 0

    shares[valid, :] = 100.0 * matrix[valid, :] / row_total[valid, :]

    return shares


# ============================================================
# MONTE CARLO MATRICES BY eta
# ============================================================


def build_eta_matrices(
    repo_root,
    runs,
    regions,
    schema_names,
    tmin,
    tmax,
):

    matrices = {}

    missing_files = []

    for eta, eta_runs in runs.groupby("eta"):
        run_matrices = []

        for _, row in eta_runs.iterrows():
            try:
                matrix = read_run_matrix(
                    repo_root=repo_root,
                    eta=float(eta),
                    replication=int(row["replication"]),
                    seed=int(row["seed"]),
                    regions=regions,
                    schema_names=(schema_names),
                    tmin=tmin,
                    tmax=tmax,
                )

            except FileNotFoundError as exc:
                missing_files.append(str(exc))

                continue

            run_matrices.append(matrix)

        if not run_matrices:
            continue

        stack = np.stack(
            run_matrices,
            axis=0,
        )

        matrices[float(eta)] = {
            "mean": (
                np.nanmean(
                    stack,
                    axis=0,
                )
            ),
            "sd": (
                np.nanstd(
                    stack,
                    axis=0,
                    ddof=1,
                )
            ),
            "n_runs": len(run_matrices),
        }

    return (
        matrices,
        missing_files,
    )


# ============================================================
# SAVE CSV MATRICES
# ============================================================


def save_matrix_csvs(
    matrices,
    regions,
    labels,
    output_dir,
):

    long_records = []

    for eta in sorted(matrices):
        matrix = matrices[eta]["mean"]

        frame = pd.DataFrame(
            matrix,
            index=[labels[r] for r in regions],
            columns=[labels[r] for r in regions],
        )

        frame.index.name = "buyer_region"

        frame.columns.name = "K_supplier_region"

        frame.to_csv(output_dir / (f"k_trade_matrix_eta_{eta_tag(eta)}_pct.csv"))

        for i, buyer in enumerate(regions):
            for j, supplier in enumerate(regions):
                long_records.append(
                    {
                        "eta_K_search": eta,
                        "buyer_region": (labels[buyer]),
                        "supplier_region": (labels[supplier]),
                        "mean_share_pct": (matrix[i, j]),
                        "is_local": (buyer == supplier),
                        "n_runs": (matrices[eta]["n_runs"]),
                    }
                )

    pd.DataFrame(long_records).to_csv(
        output_dir / "k_trade_matrices_by_eta.csv",
        index=False,
    )


# ============================================================
# INTERACTIVE MATRIX
# ============================================================


def make_heatmap(
    matrices,
    regions,
    labels,
    output_path,
):

    etas = sorted(matrices)

    region_names = [labels[r] for r in regions]

    traces = []

    for index, eta in enumerate(etas):
        matrix = matrices[eta]["mean"]

        traces.append(
            go.Heatmap(
                z=matrix,
                x=region_names,
                y=region_names,
                zmin=0,
                zmax=100,
                colorscale="Blues",
                visible=(index == 0),
                text=np.round(
                    matrix,
                    1,
                ),
                texttemplate=("%{text:.1f}%"),
                hovertemplate=(
                    "Buyer: %{y}<br>K supplier: %{x}<br>Share: %{z:.2f}%<extra></extra>"
                ),
                colorbar=dict(title=("Buyer K-goods<br>purchases (%)")),
            )
        )

    buttons = []

    for index, eta in enumerate(etas):
        visible = [False] * len(etas)

        visible[index] = True

        n_runs = matrices[eta]["n_runs"]

        buttons.append(
            dict(
                label=(f"ηK = {eta:g}"),
                method="update",
                args=[
                    {"visible": visible},
                    {
                        "title": (
                            "Regional K-goods "
                            "sourcing matrix"
                            "<br><sup>"
                            f"ηK = {eta:g}; "
                            f"{n_runs} runs"
                            "</sup>"
                        )
                    },
                ],
            )
        )

    first_eta = etas[0]

    fig = go.Figure(data=traces)

    fig.update_layout(
        title=(f"Regional K-goods sourcing matrix<br><sup>ηK = {first_eta:g}</sup>"),
        xaxis_title=("K-firm supplier region"),
        yaxis_title=("C-firm buyer region"),
        yaxis=dict(
            autorange="reversed",
            scaleanchor="x",
            scaleratio=1,
        ),
        width=760,
        height=690,
        margin=dict(
            t=130,
            l=90,
            r=120,
            b=90,
        ),
        updatemenus=[
            dict(
                buttons=buttons,
                direction="down",
                x=0.0,
                y=1.17,
                showactive=True,
            )
        ],
        annotations=[
            dict(
                text=(
                    "Rows sum to 100%. "
                    "Diagonal cells show "
                    "intra-regional K-goods sourcing."
                ),
                x=0.5,
                y=-0.14,
                xref="paper",
                yref="paper",
                showarrow=False,
            )
        ],
    )

    fig.write_html(
        output_path,
        include_plotlyjs=True,
        full_html=True,
    )


# ============================================================
# LOCAL-SHARE RESPONSE TO eta
# ============================================================


def make_local_share_plot(
    matrices,
    output_path,
):

    etas = sorted(matrices)

    mean_local = []

    for eta in etas:
        matrix = matrices[eta]["mean"]

        mean_local.append(float(np.nanmean(np.diag(matrix))))

    fig = go.Figure()

    fig.add_trace(
        go.Scatter(
            x=etas,
            y=mean_local,
            mode="lines+markers",
            name=("Mean diagonal share"),
            hovertemplate=("ηK=%{x:.2f}<br>Local K share=%{y:.2f}%<extra></extra>"),
        )
    )

    fig.update_layout(
        title=("Intra-regional K-goods sourcing across ηK"),
        xaxis_title=("eta_K_search"),
        yaxis_title=("Mean diagonal K-goods share (%)"),
        yaxis=dict(
            range=[
                0,
                100,
            ]
        ),
        width=800,
        height=500,
    )

    fig.write_html(
        output_path,
        include_plotlyjs=True,
        full_html=True,
    )


# ============================================================
# MAIN
# ============================================================


def parse_args():

    parser = argparse.ArgumentParser(
        formatter_class=(argparse.ArgumentDefaultsHelpFormatter)
    )

    parser.add_argument(
        "--calibration-dir",
        default=(DEFAULT_CALIBRATION_DIR),
    )

    parser.add_argument(
        "--schema",
        default=DEFAULT_SCHEMA,
    )

    parser.add_argument(
        "--tmin",
        type=int,
        default=TMIN,
    )

    parser.add_argument(
        "--tmax",
        type=int,
        default=TMAX,
    )

    parser.add_argument(
        "--output-dir",
        default=None,
    )

    return parser.parse_args()


def main():

    args = parse_args()

    repo_root = Path.cwd().resolve()

    if not (repo_root / "scripts").exists():
        raise RuntimeError("Run this script from the RegionalDSKSFC repository root.")

    calibration_dir = (repo_root / args.calibration_dir).resolve()

    schema_path = (repo_root / args.schema).resolve()

    schema_names = load_schema_names(schema_path)

    regions = infer_regions(schema_names)

    labels = region_labels(regions)

    runs = load_successful_runs(calibration_dir)

    output_dir = (
        Path(args.output_dir).resolve()
        if args.output_dir
        else (calibration_dir / "k_trade_matrices")
    )

    output_dir.mkdir(
        parents=True,
        exist_ok=True,
    )

    (
        matrices,
        missing_files,
    ) = build_eta_matrices(
        repo_root=repo_root,
        runs=runs,
        regions=regions,
        schema_names=schema_names,
        tmin=args.tmin,
        tmax=args.tmax,
    )

    if not matrices:
        example = "\n".join(missing_files[:5])

        raise RuntimeError(
            "\nNo raw calibration "
            "regional ymc files were found.\n\n"
            "The calibration was probably "
            "run without --keep-raw.\n\n"
            "Rerun calibration with:\n\n"
            "python scripts/calibrate_eta_k.py "
            "--eta-min 0 --eta-max 4 "
            "--eta-step 0.25 "
            "--replications 50 "
            "--workers 10 --keep-raw\n\n"
            "Then run this script again.\n\n"
            f"Example missing files:\n"
            f"{example}"
        )

    save_matrix_csvs(
        matrices=matrices,
        regions=regions,
        labels=labels,
        output_dir=output_dir,
    )

    make_heatmap(
        matrices=matrices,
        regions=regions,
        labels=labels,
        output_path=(output_dir / "k_trade_matrix_by_eta.html"),
    )

    make_local_share_plot(
        matrices=matrices,
        output_path=(output_dir / "k_trade_local_share_by_eta.html"),
    )

    print("\n" + "=" * 72)

    print("K-GOODS TRADE MATRICES BY eta_K_search")

    print("=" * 72)

    print(f"Window: t={args.tmin}..{args.tmax}")

    print(f"Eta values: {sorted(matrices)}")

    print(f"Outputs: {output_dir}")

    print("\nInteractive matrix:")

    print(output_dir / "k_trade_matrix_by_eta.html")

    print("\nLocal-share response:")

    print(output_dir / "k_trade_local_share_by_eta.html")

    if missing_files:
        print(
            f"\nWarning: "
            f"{len(missing_files)} "
            "raw regional files "
            "were missing and skipped."
        )

    print("=" * 72)


if __name__ == "__main__":
    main()
