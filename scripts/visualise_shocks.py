import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from PIL import Image

def plot_shocks(
    df,
    active_channel=(2, 6, 8),
    time_steps=(200, 300, 400, 500, 600),
):
    """Compute and plot average climate shock statistics for selected channels.

    For each channel in ``active_channel``, the shock is computed as
    ``X_a_channel / (X_a_channel + X_b_channel)`` (NaN when denominator is zero).
    The function then averages shocks across active channels per row, summarizes
    mean/std/CV at selected ``time_steps``, scales results to percent, maps model
    time ``t`` to year via ``1950 + t / 4``, and renders an error-bar plot.

    Parameters
    ----------
    df : pandas.DataFrame
        Input data containing column ``t`` and, for each channel ``c``, columns
        ``X_a_c`` and ``X_b_c``.
    active_channel : tuple[int, ...], default (2, 6, 8)
        Channel IDs to include in the shock average.
    time_steps : tuple[int, ...], default (200, 300, 400, 500, 600)
        Model time steps used for summary statistics and x-axis years.

    Returns
    -------
    pandas.DataFrame
        Transposed summary table in percent with index ``[mean, std, cv]`` and
        columns labeled by mapped calendar year.

    Raises
    ------
    KeyError
        If required ``X_a_*`` or ``X_b_*`` columns for any active channel are
        missing from ``df``.
    """
    df = df.copy()  # avoid modifying a slice/view in-place
    active_shock_cols = []

    # Build shock columns safely
    for channel in active_channel:
        xa = f"X_a_{channel}"
        xb = f"X_b_{channel}"
        if xa not in df.columns or xb not in df.columns:
            raise KeyError(f"Missing required columns: {xa} and/or {xb}")

        den = df[xa] + df[xb]
        col_name = f"shock_{channel}"
        df[col_name] = np.where(den != 0, df[xa] / den, np.nan)
        active_shock_cols.append(col_name)

    df["shock_avg_active"] = df[active_shock_cols].mean(axis=1)

    stats = (
        df.loc[df["t"].isin(time_steps)]
        .groupby("t")["shock_avg_active"]
        .agg(["mean", "std"])
        .reindex(time_steps)  # keep requested order
    )
    stats["cv"] = np.where(stats["mean"] != 0, stats["std"] / stats["mean"], np.nan)

    # Percent scale table
    final_table = stats.T * 100

    # Rename t -> year
    year_map = {t: int(1950 + t / 4) for t in final_table.columns}
    final_table.rename(columns=year_map, inplace=True)

    # Plot
    years = list(final_table.columns)
    mean_vals = final_table.loc["mean"].values
    std_vals = final_table.loc["std"].fillna(0).values
    x = np.arange(len(years))

    plt.figure()
    plt.errorbar(x, mean_vals, yerr=std_vals, marker="o", capsize=5)

    y_offset = 0.02 * np.nanmax(mean_vals) if np.isfinite(np.nanmax(mean_vals)) else 0
    for i, val in enumerate(mean_vals):
        if np.isfinite(val):
            plt.text(x[i], val + y_offset, f"{val:.2f}%", ha="center")

    plt.gca().spines[["right", "top"]].set_visible(False)
    plt.xticks(x, years)
    plt.yticks([])
    plt.xlabel("Year")
    plt.ylabel("Average Climate Shock ± 1 SD")
    plt.tight_layout()
    plt.show()

    return final_table


def create_gif(image_paths, output_gif_path, duration=2500):
    images = [Image.open(image_path) for image_path in image_paths]
    # Save as GIF
    images[0].save(
        output_gif_path,
        save_all=True,
        append_images=images[1:],
        duration=duration,
        loop=0,  # 0 means infinite loop
    )