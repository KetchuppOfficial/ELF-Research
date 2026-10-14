import argparse
import json
import re
import sys
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path

import matplotlib

matplotlib.use("Agg")  # works without a display (servers, CI)

import matplotlib.pyplot as plt  # noqa: E402
import numpy as np  # noqa: E402

# --------------------------------------------------------------------------
# Constants
# --------------------------------------------------------------------------

TIME_UNITS_TO_MS = {
    "ns": 1e-6,
    "us": 1e-3,
    "ms": 1.0,
    "s": 1e3,
}

STAGES = ("loader", "generator")

# Google Benchmark aggregates that are not separate operations.
AGGREGATES = {"mean", "stddev", "median", "cv", "median_abs_deviation"}

# Styling
LABEL_SIZE = 14
TICK_SIZE = 12
LEGEND_TITLE_SIZE = 14
LEGEND_SIZE = 12
MIN_FIG_WIDTH = 10
FIG_HEIGHT = 6
GROUP_WIDTH = 0.8
CAPSIZE = 3
GRID = dict(linestyle="--", linewidth=0.5, alpha=0.4)


# --------------------------------------------------------------------------
# Data types
# --------------------------------------------------------------------------


@dataclass(frozen=True)
class Stat:
    mean_ms: float
    stddev_ms: float


@dataclass(frozen=True)
class Record:
    benchmark: str
    operation: str
    size: str
    library: str
    stat: Stat


# --------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------


def parse_args():
    parser = argparse.ArgumentParser(
        description="Plot Google Benchmark JSON results grouped by size and library."
    )
    parser.add_argument(
        "--stage",
        choices=STAGES,
        required=True,
        help="Which benchmark stage to plot: reads <size>/<stage>/**/*.json",
    )
    parser.add_argument(
        "--results-root",
        type=Path,
        default=Path("benchmark"),
        help="Root directory containing <size>/<stage>/*.json (default: benchmark)",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        help="Directory for plots (default: same as --results-root)",
    )
    parser.add_argument(
        "--linear-y",
        action="store_true",
        help="Use a linear Y axis instead of the default logarithmic one",
    )
    parser.add_argument(
        "--format",
        default="png",
        choices=("png", "pdf", "svg"),
        help="Output image format (default: png)",
    )
    parser.add_argument(
        "--dpi", type=int, default=150, help="Raster resolution (default: 150)"
    )
    parser.add_argument(
        "--xlabel",
        default="The number of sections",
        help="X axis label (default: 'The number of sections')",
    )

    args = parser.parse_args()

    if args.output_dir is None:
        args.output_dir = args.results_root

    return args


# --------------------------------------------------------------------------
# Utilities
# --------------------------------------------------------------------------


def safe_filename(name):
    return re.sub(r"[^a-zA-Z0-9_.-]", "_", name)


def size_sort_key(size):
    try:
        return (0, float(size))
    except ValueError:
        return (1, size)


# --------------------------------------------------------------------------
# Loading and parsing results
# --------------------------------------------------------------------------


def group_by_run(entries):
    """Group benchmark entries by run_name.

    Returns {name: {"runs": [...], "aggregates": {aggregate_name: entry}}}.
    """
    groups = defaultdict(lambda: {"runs": [], "aggregates": {}})
    for entry in entries:
        name = entry.get("run_name", entry["name"])
        aggregate = entry.get("aggregate_name")
        if aggregate is None:
            groups[name]["runs"].append(entry)
        elif aggregate in AGGREGATES:
            groups[name]["aggregates"][aggregate] = entry
    return groups


def summarize(group, default_unit="ms"):
    """Reduce a group of entries to a Stat (in ms), or None if there is no data.

    The mean/stddev aggregates take priority; otherwise the statistics are
    computed from the individual repetitions.
    """
    aggregates = group["aggregates"]

    if "mean" in aggregates:
        mean_entry = aggregates["mean"]
        unit = mean_entry.get("time_unit", default_unit)
        mean = mean_entry["real_time"]
        stddev = aggregates["stddev"]["real_time"] if "stddev" in aggregates else 0.0
    else:
        runs = [r for r in group["runs"] if "real_time" in r]
        if not runs:
            return None

        units = {r.get("time_unit", default_unit) for r in runs}
        if len(units) != 1:
            raise ValueError(f"Inconsistent time units: {sorted(units)}")
        unit = units.pop()

        values = [r["real_time"] for r in runs]
        mean = float(np.mean(values))
        stddev = float(np.std(values, ddof=1)) if len(values) > 1 else 0.0

    if unit not in TIME_UNITS_TO_MS:
        raise ValueError(f"Unsupported time unit {unit!r}")

    factor = TIME_UNITS_TO_MS[unit]
    return Stat(mean * factor, stddev * factor)


def parse_data(data, size, fallback_library):
    """Parse the contents of one JSON file into a list of Records (no I/O)."""
    context = data.get("context", {})
    library = str(context.get("library", fallback_library))
    benchmark = str(context.get("benchmark", "benchmark"))
    default_unit = context.get("time_unit", "ms")

    records = []
    for name, group in group_by_run(data.get("benchmarks", [])).items():
        stat = summarize(group, default_unit)
        if stat is not None:
            records.append(Record(benchmark, name, size, library, stat))
    return records


def parse_file(path, results_root):
    """Read a JSON file and return a list of Records."""
    with path.open() as f:
        data = json.load(f)

    if not data.get("benchmarks"):
        print(f"Warning: no benchmark entries in {path}", file=sys.stderr)
        return []

    # The directory name is the authoritative size.
    size = path.relative_to(results_root).parts[0]

    try:
        return parse_data(data, size, fallback_library=path.stem)
    except ValueError as e:
        raise ValueError(f"{e} in {path}") from e


def load_records(results_root, stage):
    json_files = sorted(results_root.glob(f"*/{stage}/**/*.json"))
    if not json_files:
        raise RuntimeError(f"No JSON files found under {results_root}/<size>/{stage}/")

    records = []
    for path in json_files:
        records.extend(parse_file(path, results_root))

    if not records:
        raise RuntimeError(
            f"No usable benchmark results found under {results_root}. "
            "Check that the JSON files contain benchmark entries "
            "with real_time values."
        )

    return records


def organize(records):
    """Turn a flat list into {(benchmark, operation): {size: {library: Stat}}}."""
    organized = defaultdict(lambda: defaultdict(dict))
    for r in records:
        organized[(r.benchmark, r.operation)][r.size][r.library] = r.stat
    return organized


# --------------------------------------------------------------------------
# Plotting
# --------------------------------------------------------------------------


def log_safe_errors(means, errors):
    """Asymmetric error bars [lower, upper] for a log scale.

    The lower error is clamped so that mean - err never reaches zero or below.
    """
    lower = np.minimum(errors, means * 0.999)
    return [np.nan_to_num(lower), np.nan_to_num(errors)]


def plot_operation(size_results, sizes, libraries, colors, args, output):
    """Draw and save a single plot (one operation)."""
    operation_sizes = [s for s in sizes if s in size_results]
    operation_libraries = [
        lib
        for lib in libraries
        if any(lib in size_results[s] for s in operation_sizes)
    ]
    if not operation_sizes or not operation_libraries:
        return False

    fig_width = max(MIN_FIG_WIDTH, len(operation_sizes) * 1.5)
    fig, ax = plt.subplots(figsize=(fig_width, FIG_HEIGHT))

    x = np.arange(len(operation_sizes))
    width = GROUP_WIDTH / len(operation_libraries)

    for i, library in enumerate(operation_libraries):
        stats = [size_results[s].get(library) for s in operation_sizes]
        means = np.array([st.mean_ms if st else np.nan for st in stats], dtype=float)
        errors = np.array([st.stddev_ms if st else 0.0 for st in stats], dtype=float)

        offset = (i - (len(operation_libraries) - 1) / 2) * width
        yerr = errors if args.linear_y else log_safe_errors(means, errors)

        ax.bar(
            x + offset,
            means,
            width,
            yerr=yerr,
            capsize=CAPSIZE,
            label=library,
            color=colors[library],
        )

    ylabel = "Mean running time (ms)"
    if not args.linear_y:
        ax.set_yscale("log")
        ylabel += ", log scale"

    ax.set_xlabel(args.xlabel, size=LABEL_SIZE)
    ax.set_ylabel(ylabel, size=LABEL_SIZE)
    ax.set_xticks(x)
    ax.set_xticklabels(operation_sizes, fontsize=TICK_SIZE)
    ax.tick_params(axis="y", labelsize=TICK_SIZE)
    ax.legend(title="Library", title_fontsize=LEGEND_TITLE_SIZE, fontsize=LEGEND_SIZE)
    ax.grid(axis="y", which="both", **GRID)

    fig.tight_layout()
    fig.savefig(output, dpi=args.dpi)
    plt.close(fig)
    return True


def plot_results(records, args):
    args.output_dir.mkdir(parents=True, exist_ok=True)

    sizes = sorted({r.size for r in records}, key=size_sort_key)
    libraries = sorted({r.library for r in records})
    # Each library keeps the same color across all plots.
    colors = {lib: f"C{i}" for i, lib in enumerate(libraries)}

    print("Sizes found:", sizes)
    print("Libraries found:", libraries)

    for (benchmark, operation), size_results in sorted(organize(records).items()):
        filename = (
            f"{safe_filename(benchmark)}_{safe_filename(operation)}.{args.format}"
        )
        output = args.output_dir / filename

        if plot_operation(size_results, sizes, libraries, colors, args, output):
            print(f"Saved {output}")


# --------------------------------------------------------------------------
# main
# --------------------------------------------------------------------------


def main():
    args = parse_args()

    try:
        records = load_records(args.results_root, args.stage)
        plot_results(records, args)
    except (RuntimeError, ValueError, json.JSONDecodeError) as e:
        sys.exit(f"Error: {e}")


if __name__ == "__main__":
    main()
