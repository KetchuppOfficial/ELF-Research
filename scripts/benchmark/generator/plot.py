
#!/usr/bin/env python3

import argparse
import json
import re
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np


TIME_UNITS_TO_MS = {
    "ns": 1e-6,
    "us": 1e-3,
    "ms": 1.0,
    "s": 1e3,
}


def parse_args():
    parser = argparse.ArgumentParser(
        description=(
            "Plot Google Benchmark JSON results grouped by size and library."
        )
    )
    parser.add_argument(
        "--results-root",
        type=Path,
        default=Path("benchmark"),
        help=(
            "Root directory containing <size>/generator/*.json "
            "(default: benchmark)"
        ),
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        help="Directory for plots (default: same as --results-root)",
    )

    args = parser.parse_args()

    if args.output_dir is None:
        args.output_dir = args.results_root

    return args


def safe_filename(name):
    return re.sub(r"[^a-zA-Z0-9_.-]", "_", name)


def size_sort_key(size):
    try:
        return (0, float(size))
    except ValueError:
        return (1, size)


def load_results(results_root):
    # results[benchmark][operation][size][library] = (mean_ms, stddev_ms)
    results = {}
    sizes = set()
    libraries = set()

    json_files = sorted(results_root.glob("*/generator/**/*.json"))

    if not json_files:
        raise RuntimeError(
            f"No JSON files found under {results_root}/<size>/generator/"
        )

    for path in json_files:
        with path.open() as f:
            data = json.load(f)

        context = data.get("context", {})

        # The directory name is the authoritative size.
        size = path.relative_to(results_root).parts[0]
        library = str(context.get("library", path.stem))
        benchmark = str(context.get("benchmark", "benchmark"))

        entries = data.get("benchmarks", [])
        if not entries:
            print(f"Warning: no benchmark entries in {path}")
            continue

        aggregates = {}

        for entry in entries:
            aggregate = entry.get("aggregate_name")
            if aggregate in ("mean", "stddev"):
                name = entry.get("run_name", entry["name"])
                aggregates.setdefault(name, {})[aggregate] = entry

        operation_names = {
            entry.get("run_name", entry["name"])
            for entry in entries
            if entry.get("aggregate_name") not in (
                "mean",
                "stddev",
                "median",
                "cv",
                "median_abs_deviation",
            )
        }

        operation_names.update(
            name
            for name, stats in aggregates.items()
            if "mean" in stats
        )

        for name in operation_names:
            stats = aggregates.get(name, {})

            if "mean" in stats:
                # Use Google Benchmark's aggregate statistics.
                mean_entry = stats["mean"]
                mean = mean_entry["real_time"]

                stddev_entry = stats.get("stddev")
                stddev = (
                    stddev_entry["real_time"]
                    if stddev_entry is not None
                    else 0.0
                )

                unit = mean_entry.get(
                    "time_unit",
                    context.get("time_unit", "ms"),
                )
            else:
                # Fall back to individual runs/repetitions.
                repetitions = [
                    entry
                    for entry in entries
                    if entry.get("run_name", entry["name"]) == name
                    and "aggregate_name" not in entry
                    and "real_time" in entry
                ]

                if not repetitions:
                    continue

                units = {
                    entry.get(
                        "time_unit",
                        context.get("time_unit", "ms"),
                    )
                    for entry in repetitions
                }

                if len(units) != 1:
                    raise ValueError(
                        f"Inconsistent time units for {name} in {path}"
                    )

                unit = units.pop()
                values = [entry["real_time"] for entry in repetitions]
                mean = float(np.mean(values))
                stddev = (
                    float(np.std(values, ddof=1))
                    if len(values) > 1
                    else 0.0
                )

            if unit not in TIME_UNITS_TO_MS:
                raise ValueError(
                    f"Unsupported time unit {unit!r} in {path}"
                )

            factor = TIME_UNITS_TO_MS[unit]

            results.setdefault(benchmark, {}).setdefault(
                name, {}
            ).setdefault(size, {})[library] = (
                mean * factor,
                stddev * factor,
            )

            sizes.add(size)
            libraries.add(library)

    if not results:
        raise RuntimeError(
            f"No usable benchmark results found under {results_root}. "
            "Check that the JSON files contain benchmark entries "
            "with real_time values."
        )

    print("Sizes found:", sorted(sizes, key=size_sort_key))
    print("Libraries found:", sorted(libraries))

    return results, sorted(sizes, key=size_sort_key), sorted(libraries)


def plot_results(results, sizes, libraries, output_dir):
    output_dir.mkdir(parents=True, exist_ok=True)

    for benchmark, operations in sorted(results.items()):
        for operation, size_results in sorted(operations.items()):
            benchmark_sizes = [
                size for size in sizes if size in size_results
            ]

            if not benchmark_sizes:
                continue

            fig_width = max(10, len(benchmark_sizes) * 1.5)
            fig, ax = plt.subplots(figsize=(fig_width, 6))

            x = np.arange(len(benchmark_sizes))
            width = 0.8 / max(len(libraries), 1)

            for i, library in enumerate(libraries):
                means = []
                errors = []

                for size in benchmark_sizes:
                    value = size_results.get(size, {}).get(library)
                    means.append(value[0] if value else np.nan)
                    errors.append(value[1] if value else 0.0)

                offset = (
                    i - (len(libraries) - 1) / 2
                ) * width

                ax.bar(
                    x + offset,
                    means,
                    width,
                    yerr=errors,
                    capsize=3,
                    label=library,
                )

            ax.set_ylabel("Mean running time (ms)")
            ax.set_xlabel("Size")
            ax.set_title(f"{benchmark}: {operation}")
            ax.set_xticks(x)
            ax.set_xticklabels(benchmark_sizes)
            ax.legend(title="Library")
            ax.grid(axis="y", linestyle="--", alpha=0.4)

            fig.tight_layout()

            filename = (
                f"{safe_filename(benchmark)}_"
                f"{safe_filename(operation)}.png"
            )
            output = output_dir / filename

            fig.savefig(output, dpi=150)
            plt.close(fig)

            print(f"Saved {output}")


def main():
    args = parse_args()

    results, sizes, libraries = load_results(args.results_root)
    plot_results(results, sizes, libraries, args.output_dir)


if __name__ == "__main__":
    main()
