import argparse
import os
import sys
from pathlib import Path

from common import plot
from common import utils

SCRIPTS_DIR: Path = Path(__file__).resolve().parent
SCRIPT_NAMES: list[str] = ["measure.py"]

ELFIO_ELF_MARKER = "elfio"


def is_elf(path: Path) -> bool:
    """Return True if the file starts with the ELF magic number."""
    try:
        with open(path, "rb") as f:
            return f.read(4) == b"\x7fELF"
    except OSError:
        return False


def find_elfio_elf(size_dir: Path) -> Path:
    """Find the single elfio ELF file anywhere under size_dir."""
    found = sorted(
        p for p in size_dir.rglob(f"*{ELFIO_ELF_MARKER}*") if p.is_file() and is_elf(p)
    )
    if not found:
        utils.fail(f"no elfio ELF file found under {size_dir}")
    if len(found) > 1:
        utils.fail(
            f"several elfio ELF files found under {size_dir}: "
            + ", ".join(str(p) for p in found)
        )
    return found[0]


def find_size_dirs(mode_dir: Path) -> list[Path]:
    """Return subdirectories of mode_dir named by sections count, in numeric order."""
    return sorted(
        (d for d in mode_dir.iterdir() if d.is_dir() and d.name.isdecimal()),
        key=lambda d: int(d.name),
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run loader measurements and build plots for every mode and "
        "sections count found under --tmp-root."
    )
    parser.add_argument(
        "--tmp-root",
        type=Path,
        required=True,
        metavar="DIR",
        help="root directory with the layout produced by the generator run: "
        "<mode>/<sections count>/; must exist",
    )
    parser.add_argument(
        "--loader",
        type=Path,
        required=True,
        metavar="FILE",
        help="path to the loader executable; must exist",
    )
    parser.add_argument(
        "--libs",
        type=utils.str_list,
        required=True,
        metavar="LIB[,LIB...]",
        help="comma-separated list of libraries to measure, " "e.g. libelf,elfio",
    )
    parser.add_argument(
        "--reliable",
        action="store_true",
        help="run the generator under 'sudo ionice -c 1 -n 0 nice -n -20' "
        "for more stable measurements (requires sudo); passed to measure.py",
    )
    return parser.parse_args()


def validate(args: argparse.Namespace) -> None:
    """Check that --tmp-root, the loader and all helper scripts exist."""
    errors: list[str] = []

    if not args.tmp_root.is_dir():
        errors.append(
            f"--tmp-root does not exist or is not a directory: {args.tmp_root}"
        )

    if not args.loader.is_file():
        errors.append(f"--loader does not exist or is not a file: {args.loader}")
    elif not os.access(args.loader, os.X_OK):
        errors.append(f"--loader is not executable: {args.loader}")

    for name in SCRIPT_NAMES:
        if not (SCRIPTS_DIR / name).is_file():
            errors.append(f"script not found: {SCRIPTS_DIR / name}")

    if errors:
        for e in errors:
            print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


def main() -> None:
    args = parse_args()
    validate(args)

    tmp_root: Path = args.tmp_root
    loader: Path = args.loader
    libs: list[str] = args.libs

    for mode in utils.MODES:
        tmp = tmp_root / mode
        if not tmp.is_dir():
            utils.fail(f"mode directory not found: {tmp}")

        for size_dir in find_size_dirs(tmp):
            elf = find_elfio_elf(size_dir)

            measure_args: list[str | Path] = [
                sys.executable,
                SCRIPTS_DIR / "measure.py",
                "--loader",
                loader,
                "--elf",
                elf,
                "--output-dir",
                size_dir,
                f"--libs={','.join(libs)}",
            ]
            if args.reliable:
                measure_args.append("--reliable")

            utils.run(measure_args)

        utils.run(
            [
                sys.executable,
                Path(plot.__file__).resolve(),
                "--stage",
                "loader",
                "--results-root",
                tmp,
            ]
        )


if __name__ == "__main__":
    main()
