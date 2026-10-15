import argparse
import json
import pathlib
import shutil
import subprocess
import sys

from common import utils


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Script for running benchmarks on generator"
    )

    parser.add_argument(
        "--generator",
        help="path to the generator",
        type=utils.str_to_file,
        required=True,
    )

    parser.add_argument(
        "--output-dir", help="directory to store output in", type=utils.str_to_dir
    )

    parser.add_argument(
        "--config", help="path to the generation config", type=utils.str_to_file
    )

    parser.add_argument(
        "--libs",
        help="libraries to run generator for",
        type=utils.str_list,
        required=True,
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

    with open(args.config, "r") as config:
        data = json.load(config)
        sections_count: int = len(data["sections"])

    elf_dir: pathlib.Path = args.output_dir / "elfs"
    elf_dir.mkdir(parents=True, exist_ok=True)

    json_dir: pathlib.Path = args.output_dir / "generator"
    json_dir.mkdir(parents=True, exist_ok=True)

    command_prefix: list[str] = utils.RELIABLE_PREFIX if args.reliable else []

    for lib in args.libs:
        generator_args = [
            args.generator,
            "--lib",
            lib,
            "-o",
            elf_dir / f"{lib}-elf",
            "--config",
            args.config,
        ]

        benchmark_specific_args: list[str] = [
            f"--benchmark_out={json_dir}/{lib}.json",
            f"--benchmark_context=library={lib},size={sections_count}",
        ]

        print(f"Running benchmark on {lib}")
        utils.run(
            command_prefix
            + generator_args
            + utils.BENCHMARK_COMMON_ARGS
            + benchmark_specific_args
        )


if __name__ == "__main__":
    main()
