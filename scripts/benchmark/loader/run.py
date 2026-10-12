import argparse
import os
import subprocess
import sys
from collections.abc import Sequence
from pathlib import Path
from typing import NoReturn, Union

MODES: list[str] = ["1xn", "nxn"]

# Helper scripts live next to this script
SCRIPTS_DIR: Path = Path(__file__).resolve().parent
SCRIPT_NAMES: list[str] = ["measure.py", "plot.py"]

# Name fragment that identifies the ELF produced for the elfio library
ELFIO_ELF_MARKER = "elfio"

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
        p for p in size_dir.rglob(f"*{ELFIO_ELF_MARKER}*")
        if p.is_file() and is_elf(p)
    )
    if not found:
        fail(f"no elfio ELF file found under {size_dir}")
    if len(found) > 1:
        fail(f"several elfio ELF files found under {size_dir}: "
             + ", ".join(str(p) for p in found))
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
        "--tmp-root", type=Path, required=True, metavar="DIR",
        help="root directory with the layout produced by the generator run: "
             "<mode>/<sections count>/; must exist")
    parser.add_argument(
        "--loader", type=Path, required=True, metavar="FILE",
        help="path to the loader executable; must exist")
    parser.add_argument(
        "--libs", type=str_list, required=True, metavar="LIB[,LIB...]",
        help="comma-separated list of libraries to measure, "
             "e.g. libelf,elfio")
    return parser.parse_args()


def validate(args: argparse.Namespace) -> None:
    """Check that --tmp-root, the loader and all helper scripts exist."""
    errors: list[str] = []

    if not args.tmp_root.is_dir():
        errors.append(f"--tmp-root does not exist or is not a directory: {args.tmp_root}")

    if not args.loader.is_file():
        errors.append(f"--loader does not exist or is not a file: {args.loader}")
    elif not os.access(args.loader, os.X_OK):
        errors.append(f"--loader is not executable: {args.loader}")

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
    loader: Path = args.loader
    libs: list[str] = args.libs

    for mode in MODES:
        tmp = tmp_root / mode
        if not tmp.is_dir():
            fail(f"mode directory not found: {tmp}")

        for size_dir in find_size_dirs(tmp):
            elf = find_elfio_elf(size_dir)

            run([sys.executable, SCRIPTS_DIR / "measure.py",
                 "--loader", loader,
                 "--elf", elf,
                 "--output-dir", size_dir,
                 "--config", size_dir / "config.json",
                 f"--libs={','.join(libs)}"])

        # # Plot once per mode, after all sizes are measured
        run([sys.executable, SCRIPTS_DIR / "plot.py", "--results-root", tmp])


if __name__ == "__main__":
    main()
