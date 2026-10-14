import argparse
import os
import pathlib
import re
import shutil
import subprocess
import sys

from common import utils
from pathlib import Path

# Matches e.g. "Number of section headers:         31" and, for files with
# extended numbering, "Number of section headers:         0 (70000)".
_SHNUM_RE = re.compile(r"Number of section headers:\s+(\d+)(?:\s+\((\d+)\))?")


def count_elf_sections(path: str | Path) -> int:
    """Return the number of sections in an ELF file, as reported by `readelf -h`.

    The count includes the null section 0.
    Raises RuntimeError if readelf fails or its output cannot be parsed.
    """
    # Force the C locale so the field names are not translated
    proc = utils.run(
        ["readelf", "-h", str(path)],
        env={**os.environ, "LC_ALL": "C"},
        capture_output=True,
        text=True,
    )
    match = _SHNUM_RE.search(proc.stdout)
    if match is None:
        raise RuntimeError(f"cannot find section count in readelf output for {path}")

    # With extended numbering the real count is in parentheses
    return int(match.group(2) or match.group(1))


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Script for running benchmarks on loader"
    )

    parser.add_argument(
        "--loader", help="path to the loader", type=utils.str_to_file, required=True
    )

    parser.add_argument(
        "--elf",
        help="path to the ELF file to load",
        type=utils.str_to_file,
        required=True,
    )

    parser.add_argument(
        "--output-dir", help="directory to store output in", type=utils.str_to_dir
    )

    parser.add_argument(
        "--libs", help="libraries to run loader for", type=utils.str_list, required=True
    )

    parser.add_argument(
        "--reliable",
        help=(
            "run generator under 'sudo ionice -c 1 -n 0 nice -n -20' "
            "for more stable measurements (requires sudo)"
        ),
        action="store_true",
    )

    args = parser.parse_args()

    if args.reliable:
        if shutil.which("sudo") is None:
            sys.exit("--reliable requires sudo, but it was not found in PATH")
        # Ask for the password once up front, so it isn't requested mid-benchmark
        # and doesn't distort the measurements.
        if subprocess.run(["sudo", "-v"], check=False).returncode != 0:
            sys.exit("--reliable requires sudo privileges")

    benchmark_common_args: list[str] = [
        "--benchmark_time_unit=ms",
        "--benchmark_repetitions=5",
        "--benchmark_min_time=2x",
        "--benchmark_enable_random_interleaving",
        "--benchmark_out_format=json",
    ]

    json_dir: pathlib.Path = args.output_dir / "loader"
    json_dir.mkdir(exist_ok=True)

    command_prefix: list[str] = utils.RELIABLE_PREFIX if args.reliable else []

    sections_count: int = count_elf_sections(args.elf) - 2

    for lib in args.libs:
        loader_args = [
            args.loader,
            args.elf,
            "--lib",
            lib,
        ]

        benchmark_specific_args: list[str] = [
            f"--benchmark_out={json_dir}/{lib}.json",
            f"--benchmark_context=library={lib},size={sections_count}",
        ]

        print(f"Running benchmark on {lib}")
        utils.run(
            command_prefix
            + loader_args
            + benchmark_common_args
            + benchmark_specific_args
        )


if __name__ == "__main__":
    main()
