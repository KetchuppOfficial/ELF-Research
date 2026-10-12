#!/usr/bin/env python3
import argparse
import os
import subprocess
import sys
from collections.abc import Sequence
from pathlib import Path
from typing import NoReturn, Union

MODES: list[str] = ["1xn", "nxn"]

DEFAULT_SECTION_SIZE = "0x1000"

# Helper scripts live next to this script
SCRIPTS_DIR: Path = Path(__file__).resolve().parent
SCRIPT_NAMES: list[str] = ["generate_configs.py", "measure.py", "plot.py"]

CmdPart = Union[str, os.PathLike]


def fail(message: str, code: int = 1) -> NoReturn:
    """Print an error message to stderr and exit with the given code."""
    print(f"Error: {message}", file=sys.stderr)
    sys.exit(code)


def run(cmd: Sequence[CmdPart]) -> None:
    """Run a command; on failure, exit this script with the command's return code."""
    args = [str(part) for part in cmd]
    try:
        subprocess.run(args, check=True)
    except subprocess.CalledProcessError as e:
        fail(f"command exited with code {e.returncode}: {' '.join(args)}",
             e.returncode or 1)
    except OSError as e:
        fail(f"failed to start {args[0]}: {e}")


def hex_int(value: str) -> int:
    """Parse a positive hexadecimal number; the '0x' prefix is optional."""
    try:
        result = int(value, 16)
    except ValueError:
        raise argparse.ArgumentTypeError(f"invalid hexadecimal number: {value!r}")
    if result <= 0:
        raise argparse.ArgumentTypeError(f"value must be positive: {value!r}")
    return result


def int_list(value: str) -> list[int]:
    """Parse a comma-separated list of positive decimal integers, e.g. '5000,10000'."""
    items = [item.strip() for item in value.split(",")]
    result: list[int] = []
    for item in items:
        try:
            number = int(item)
        except ValueError:
            raise argparse.ArgumentTypeError(f"invalid integer in list: {item!r}")
        if number <= 0:
            raise argparse.ArgumentTypeError(f"values must be positive: {item!r}")
        result.append(number)
    return result


def str_list(value: str) -> list[str]:
    """Parse a comma-separated list of non-empty names, e.g. 'libelf,elfio'."""
    result = [item.strip() for item in value.split(",")]
    if not all(result):
        raise argparse.ArgumentTypeError(f"empty item in list: {value!r}")
    return result


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate configs, run measurements and build plots "
                    "for every mode and sections count."
    )
    parser.add_argument(
        "--tmp-root", type=Path, required=True, metavar="DIR",
        help="directory for all results; created if it does not exist")
    parser.add_argument(
        "--generator", type=Path, required=True, metavar="FILE",
        help="path to the generator executable; must exist")
    parser.add_argument(
        "--libs", type=str_list, required=True, metavar="LIB[,LIB...]",
        help="comma-separated list of libraries to measure, "
             "e.g. libelf,elfio")
    parser.add_argument(
        "--sections-count", type=int_list, required=True, metavar="N[,N...]",
        help="comma-separated list of sections counts (decimal), one run "
             "for each, e.g. 5000,10000,20000")
    parser.add_argument(
        "--section-size", type=hex_int,
        default=hex_int(DEFAULT_SECTION_SIZE), metavar="HEX",
        help="size of a single section, in hexadecimal (with or without "
             f"the 0x prefix); default: {DEFAULT_SECTION_SIZE}")
    return parser.parse_args()


def validate(args: argparse.Namespace) -> None:
    """Check that the generator and all helper scripts exist."""
    errors: list[str] = []

    if not args.generator.is_file():
        errors.append(f"--generator does not exist or is not a file: {args.generator}")
    elif not os.access(args.generator, os.X_OK):
        errors.append(f"--generator is not executable: {args.generator}")

    for name in SCRIPT_NAMES:
        if not (SCRIPTS_DIR / name).is_file():
            errors.append(f"script not found: {SCRIPTS_DIR / name}")

    if errors:
        # Report all problems at once, not just the first one
        for e in errors:
            print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


def main() -> None:
    args = parse_args()
    validate(args)

    tmp_root: Path = args.tmp_root
    generator: Path = args.generator
    sizes: list[int] = args.sections_count
    section_size: int = args.section_size
    libs: list[str] = args.libs

    tmp_root.mkdir(parents=True, exist_ok=True)

    for mode in MODES:
        tmp = tmp_root / mode
        tmp.mkdir(parents=True, exist_ok=True)

        for size in sizes:
            size_dir = tmp / str(size)
            size_dir.mkdir(parents=True, exist_ok=True)

            # Child scripts run under the same interpreter as this script
            run([sys.executable, SCRIPTS_DIR / "generate_configs.py",
                 "--mode", mode,
                 "--section-size", hex(section_size),
                 "--sections-count", size,
                 "--dir", size_dir])

            run([sys.executable, SCRIPTS_DIR / "measure.py",
                 "--generator", generator,
                 "--output-dir", size_dir,
                 "--config", size_dir / "config.json",
                 f"--libs={','.join(libs)}"])

        # Plot once per mode, after all sizes are measured
        run([sys.executable, SCRIPTS_DIR / "plot.py", "--results-root", tmp])


if __name__ == "__main__":
    main()