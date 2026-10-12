import argparse
import pathlib
import subprocess
import json
import re
import os


def str_to_file(s: str) -> pathlib.Path:
    file: pathlib.Path = pathlib.Path(s).resolve()
    if not file.is_file():
        raise FileNotFoundError(s)
    return file

def str_to_dir(s: str) -> pathlib.Path:
    dir: pathlib.Path = pathlib.Path(s).resolve()
    if not dir.is_dir():
        raise NotADirectoryError(s)
    return dir

# Matches e.g. "Number of section headers:         31" and, for files with
# extended numbering, "Number of section headers:         0 (70000)".
_SHNUM_RE = re.compile(r"Number of section headers:\s+(\d+)(?:\s+\((\d+)\))?")

def count_elf_sections(path: str | Path) -> int:
    """Return the number of sections in an ELF file, as reported by `readelf -h`.

    The count includes the null section 0.
    Raises RuntimeError if readelf fails or its output cannot be parsed.
    """
    # Force the C locale so the field names are not translated
    env = {**os.environ, "LC_ALL": "C"}
    try:
        proc = subprocess.run(
            ["readelf", "-h", str(path)],
            capture_output=True, text=True, env=env, check=True,
        )
    except FileNotFoundError:
        raise RuntimeError("readelf not found; install binutils") from None
    except subprocess.CalledProcessError as e:
        raise RuntimeError(f"readelf failed for {path}: {e.stderr.strip()}") from e

    match = _SHNUM_RE.search(proc.stdout)
    if match is None:
        raise RuntimeError(f"cannot find section count in readelf output for {path}")

    # With extended numbering the real count is in parentheses
    return int(match.group(2) or match.group(1))

def main() -> None:
    parser = argparse.ArgumentParser(description="Script for running benchmarks on loader")

    parser.add_argument("--loader",
                        help="path to the loader",
                        type=str_to_file,
                        required=True)

    parser.add_argument("--elf",
                        help="path to the ELF file to load",
                        type=str_to_file,
                        required=True)

    parser.add_argument("--output-dir",
                        help="directory to store output in",
                        type=str_to_dir)

    parser.add_argument("--config",
                        help="path to the generation config",
                        type=str_to_file)

    parser.add_argument("--libs",
                        help="libraries to run loader for",
                        type=lambda s: s.split(','),
                        required=True)

    args = parser.parse_args()

    with open(args.config, 'r') as config:
        data = json.load(config)
        sections_count: int = len(data["sections"])

    benchmark_common_args: list[str] = [
        "--benchmark_time_unit=ms",
        "--benchmark_repetitions=5",
        "--benchmark_min_time=2x",
        "--benchmark_enable_random_interleaving",
        "--benchmark_out_format=json"
    ]

    json_dir: pathlib.Path = args.output_dir / "loader"
    json_dir.mkdir(exist_ok=True)

    for lib in args.libs:
        loader_args = [
            args.loader,
            args.elf,
            "--lib", lib,
        ]

        sections_count: int = count_elf_sections(args.elf) - 2

        benchmark_specific_args: list[str] = [
            f"--benchmark_out={json_dir}/{lib}.json",
            f"--benchmark_context=library={lib},size={sections_count}"
        ]

        print(f"Running benchmark on {lib}")
        res = subprocess.run(loader_args + benchmark_common_args + benchmark_specific_args,
                             check=False)
        if res.returncode != 0:
            return

if __name__ == "__main__":
    main()
