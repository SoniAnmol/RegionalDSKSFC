import io
import re
import numpy as np
import plotly.graph_objects as go

def _write_multipage_pdf(scenarios, pdf_pages, output_path, width, height, scale, build_fn, **build_kwargs):
    """Build a multi-page PDF, one page per entry in *pdf_pages*.

    Parameters
    ----------
    scenarios : dict
        Full scenarios dict (same format as the *scenarios* argument of
        ``build_macro_snapshot_metric_figure``).
    pdf_pages : list[list[str] | None]
        Each element defines one PDF page.
        - ``None``     → include *all* scenarios on this page.
        - list of str  → include only the scenario keys listed on this page.
    output_path : str or Path
        Destination PDF file path.
    width, height, scale : int
        kaleido export dimensions.
    build_fn : callable
        Figure-building function (e.g. ``build_macro_snapshot_metric_figure``).
    **build_kwargs
        Extra keyword arguments forwarded verbatim to ``build_fn``.
    """
    from pypdf import PdfWriter, PdfReader

    writer = PdfWriter()
    for page_keys in pdf_pages:
        page_scen = (
            scenarios
            if page_keys is None
            else {k: v for k, v in scenarios.items() if k in page_keys}
        )
        fig = build_fn(scenarios=page_scen, **build_kwargs)
        pdf_bytes = fig.to_image(format="pdf", width=width, height=height, scale=scale)
        reader = PdfReader(io.BytesIO(pdf_bytes))
        for page in reader.pages:
            writer.add_page(page)

    with open(output_path, "wb") as f:
        writer.write(f)


def build_macro_snapshot_metric_figure(
    scenarios,
    metric,
    region='macro',
    metric_label = '',
    ylabel=None,
    snap_ts=None,
    crisis_thr=-5.0,
    title=None,
    line_type = 'lines+markers',
    ncol_legend=3,
    CI_type='band',
    ymin=None,
    ymax=None,
    pdf_path=None,
    pdf_pages=None,
    pdf_width=750,
    pdf_height=500,
    pdf_scale=2,
):
    """Build a single-panel Plotly figure for one macro metric across scenarios.

    Parameters
    ----------
    scenarios : dict
        Mapping like:
        {
            "Scenario name": {"df": <pandas.DataFrame>, "color": "<color>"},
            ...
        }
    metric : str
        One of: "Unemployment", "GDP_r_growth", "GDP_r_volatility", "crisis".
    ylabel : str | None
        Y-axis label. If None, inferred from metric.
    snap_ts : list[int] | None
        Snapshot timesteps to plot. Defaults to [200, 300, 400, 500, 600].
    crisis_thr : float
        Threshold for crisis likelihood when metric == "crisis".
    title : str | None
        Figure title. If None, generated from metric.
    ncol_legend : int
        Number of columns in the bottom legend. Default is 3.
    CI_type : str
        How to display the ±1 SD uncertainty. Options:
        - 'band'      : filled semi-transparent band (default).
        - 'error_bar' : symmetric error bars matching the line colour and dash.
    pdf_path : str or Path or None
        If provided together with *pdf_pages*, a multi-page PDF is written to
        this path.  The single-panel figure (all scenarios) is still returned.
    pdf_pages : list[list[str] | None] or None
        Defines the scenario subsets for each PDF page.  Requires *pdf_path*.
        Each element is either ``None`` (all scenarios) or a list of scenario
        name keys from the *scenarios* dict.  Example::

            PDF_PAGES = [
                None,                                        # page 1: all scenarios
                ["DSK: No Shock"],                           # page 2
                ["DSK: No Shock", "Reg DSK: No Shock"],     # page 3
                ["DSK: CS"],                                 # page 4
                ["DSK: CS", "Reg DSK: CS"],                 # page 5
                ["DSK: LP + EF"],                           # page 6
                ["DSK: LP + EF", "Reg DSK: LP + EF"],       # page 7
            ]

    pdf_width, pdf_height, pdf_scale : int
        kaleido export dimensions for each PDF page. Defaults: 750 × 500 @ 2×.

    Returns
    -------
    plotly.graph_objects.Figure
        Configured single-panel figure (all scenarios).
    """
    if snap_ts is None:
        snap_ts = [200, 300, 400, 500, 600]

    if ylabel is None:
        ylabel = metric_label

    if title is None:
        title = f"{ylabel}"

    def _macro(df):
        if "region" in df.columns and df["region"].nunique() > 1:
            return df[df["region"] == region].copy()
        return df

    def _snap_stats(df, metric_name):
        work = _macro(df)
        work = work[work["t"].isin(snap_ts)][["run", "t", metric_name]].dropna(subset=[metric_name])
        by_t = work.groupby(["t", "run"])[metric_name].mean().groupby("t")
        return by_t.mean(), by_t.std().fillna(0)

    def _crisis_snap_stats(df, threshold=crisis_thr):
        work = _macro(df)
        work = work[work["t"].isin(snap_ts)][["run", "t", "GDP_r_growth"]].dropna(subset=["GDP_r_growth"]).copy()
        work["_crisis"] = (work["GDP_r_growth"] < threshold).astype(float)
        by_t = work.groupby(["t", "run"])["_crisis"].mean().groupby("t")
        return by_t.mean(), by_t.std().fillna(0)

    fig = go.Figure()

    for scenario_name, spec in scenarios.items():
        df = spec["df"]
        color = spec["color"]
        dash = spec["dash"]

        if metric == "crisis":
            mean_ts, std_ts = _crisis_snap_stats(df)
        else:
            mean_ts, std_ts = _snap_stats(df, metric)

        t_vals = mean_ts.index.values
        m_vals = mean_ts.values
        s_vals = std_ts.reindex(mean_ts.index).fillna(0).values

        if CI_type == 'band':
            # Phantom trace: legend icon shows combined fill + line style
            fig.add_trace(
                go.Scatter(
                    x=[None], y=[None],
                    mode="lines",
                    fill="toself",
                    fillcolor=color,
                    opacity=0.4,
                    line=dict(color=color, width=1, dash=dash),
                    name=scenario_name,
                    legendgroup=scenario_name,
                    showlegend=False,
                    visible="legendonly",
                )
            )

            # SD band
            fig.add_trace(
                go.Scatter(
                    x=np.concatenate([t_vals, t_vals[::-1]]),
                    y=np.concatenate([m_vals + s_vals, (m_vals - s_vals)[::-1]]),
                    fill="toself",
                    fillcolor=color,
                    opacity=0.12,
                    line=dict(width=1),
                    name=scenario_name,
                    legendgroup=scenario_name,
                    showlegend=False,
                    visible=True,
                    hoverinfo="skip",
                )
            )

            # Mean line
            fig.add_trace(
                go.Scatter(
                    x=t_vals,
                    y=m_vals,
                    mode=line_type,
                    name=scenario_name,
                    line=dict(color=color, width=2, dash=dash),
                    marker=dict(color=color, size=8, symbol="circle",
                                line=dict(color=color, width=1)),
                    legendgroup=scenario_name,
                    showlegend=True,
                )
            )

        else:  # 'error_bar'
            # Mean line with symmetric ±1 SD error bars; single legend entry
            fig.add_trace(
                go.Scatter(
                    x=t_vals,
                    y=m_vals,
                    mode=line_type,
                    name=scenario_name,
                    line=dict(color=color, width=2, dash=dash),
                    marker=dict(color=color, size=8, symbol="circle",
                                line=dict(color=color, width=1)),
                    error_y=dict(
                        type="data",
                        array=s_vals,
                        visible=True,
                        color=color,
                        thickness=1.5,
                        width=4,
                    ),
                    legendgroup=scenario_name,
                    showlegend=True,
                )
            )

    fig.update_xaxes(range=[min(snap_ts), max(snap_ts)+10], title_text="Timestep (t)", tickvals=snap_ts)
    if ymin is None:
        fig.update_yaxes(title_text=ylabel)
    else:
        fig.update_yaxes(range=[ymin, ymax], title_text=ylabel)

    if min(snap_ts) <= 199 <= max(snap_ts):
        fig.add_vline(
            x=200,
            line_width=1.5,
            line_dash="dash",
            line_color="black",
            annotation_text="climate shocks start",
            annotation_position="top right",
        )

    _ncol_legend = ncol_legend + 1

    fig.update_layout(
        height=500,
        width=700,
        template="simple_white",
        margin=dict(l=0, r=10, t=30, b=10),
        paper_bgcolor='rgba(0,0,0,0)',
        plot_bgcolor='rgba(0,0,0,0)',
        legend=dict(
            orientation="h",
            entrywidth=(1 / _ncol_legend)* 1.25,
            entrywidthmode="fraction",
            x=0.5,
            y=-0.2,
            xanchor="center",
            yanchor="top",
            tracegroupgap=4,
            # title_text="",
            
        ),
    )

    if pdf_path is not None and pdf_pages is not None:
        _write_multipage_pdf(
            scenarios=scenarios,
            pdf_pages=pdf_pages,
            output_path=pdf_path,
            width=pdf_width,
            height=pdf_height,
            scale=pdf_scale,
            build_fn=build_macro_snapshot_metric_figure,
            metric=metric,
            metric_label=metric_label,
            ylabel=ylabel,
            snap_ts=snap_ts,
            crisis_thr=crisis_thr,
            title=title,
            line_type=line_type,
            ncol_legend=ncol_legend,
            CI_type=CI_type,
            ymin=ymin,
            ymax=ymax,
        )

    return fig


