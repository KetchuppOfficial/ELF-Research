import argparse
import os
import sys
from pathlib import Path

from common import plot
from common import utils

DEFAULT_SECTION_SIZE = "0x1000"

SCRIPTS_DIR: Path = Path(__file__).resolve().parent
SCRIPT_NAMES: list[str] = ["generate_configs.py", "measure.py"]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Gwenerate configs, run measurements and build plots "
        "for every mode and sections count."
    )
    parser.add_argument(
        "--tmp-root",
        type=Path,
        required=True,
        metavar="DIR",
        help="directory for all results; created if it does not exist",
    )
    parser.add_argument(
        "--generator",
        type=Path,
        required=True,
        metavar="FILE",
        help="path to the generator executable; must exist",
    )
    parser.add_argument(
        "--libs",
        type=utils.str_list,
        required=True,
        metavar="LIB[,LIB...]",
        help="comma-separated list of libraries to measure, " "e.g. libelf,elfio",
    )
    parser.add_argument(
        "--sections-count",
        type=utils.int_list,
        required=True,
        metavar="N[,N...]",
        help="comma-separated list of sections counts (decimal), one run "
        "for each, e.g. 5000,10000,20000",
    )
    parser.add_argument(
        "--section-size",
        type=utils.hex_int,
        default=utils.hex_int(DEFAULT_SECTION_SIZE),
        metavar="HEX",
        help="size of a single section, in hexadecimal (with or without "
        f"the 0x prefix); default: {DEFAULT_SECTION_SIZE}",
    )
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

    for mode in utils.MODES:
        tmp = tmp_root / mode
        tmp.mkdir(parents=True, exist_ok=True)

        for size in sizes:
            size_dir = tmp / str(size)
            size_dir.mkdir(parents=True, exist_ok=True)

            utils.run(
                [
                    sys.executable,
                    SCRIPTS_DIR / "generate_configs.py",
                    "--mode",
                    mode,
                    "--section-size",
                    hex(section_size),
                    "--sections-count",
                    size,
                    "--dir",
                    size_dir,
                ]
            )

            utils.run(
                [
                    sys.executable,
                    SCRIPTS_DIR / "measure.py",
                    "--generator",
                    generator,
                    "--output-dir",
                    size_dir,
                    "--config",
                    size_dir / "config.json",
                    f"--libs={','.join(libs)}",
                ]
            )

        utils.run(
            [
                sys.executable,
                Path(plot.__file__).resolve(),
                "--stage",
                "generator",
                "--results-root",
                tmp,
            ]
        )


if __name__ == "__main__":
    main()
