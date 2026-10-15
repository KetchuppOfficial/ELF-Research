import argparse
import os
import subprocess
import sys
from collections.abc import Sequence
from pathlib import Path
from typing import Any, NoReturn

MODES: list[str] = ["1xn", "nxn"]

# fmt: off
RELIABLE_PREFIX: list[str] = [
    "sudo",
    #
    "ionice", "-c", "1", "-n", "0",
    #
    "nice", "-n", "-20",
]
# fmt: on

BENCHMARK_COMMON_ARGS: list[str] = [
    "--benchmark_time_unit=ms",
    "--benchmark_repetitions=5",
    "--benchmark_enable_random_interleaving",
    "--benchmark_out_format=json",
]


def fail(message: str, code: int = 1) -> NoReturn:
    """Print an error message to stderr and exit with the given code."""
    print(f"Error: {message}", file=sys.stderr)
    sys.exit(code)


def run(cmd: Sequence[str | os.PathLike], **kwargs: Any) -> subprocess.CompletedProcess:
    """Run a command; on failure, exit this script with the command's return code.

    Extra keyword arguments are passed to subprocess.run unchanged
    (cwd, env, capture_output, text, timeout, ...).
    """
    args = [str(part) for part in cmd]
    try:
        return subprocess.run(args, check=True, **kwargs)
    except subprocess.CalledProcessError as e:
        msg = f"command exited with code {e.returncode}: {' '.join(args)}"
        if e.stderr:
            msg += f"\nstderr:\n{e.stderr}"
        fail(msg, e.returncode)


def str_to_file(s: str) -> Path:
    file: Path = Path(s).resolve()
    if not file.is_file():
        raise FileNotFoundError(s)
    return file


def str_to_dir(s: str) -> Path:
    dir: Path = Path(s).resolve()
    if not dir.is_dir():
        raise NotADirectoryError(s)
    return dir


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
