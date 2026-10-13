import argparse
import json
import pathlib

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

    args = parser.parse_args()

    with open(args.config, "r") as config:
        data = json.load(config)
        sections_count: int = len(data["sections"])

    benchmark_common_args: list[str] = [
        "--benchmark_time_unit=ms",
        "--benchmark_repetitions=5",
        "--benchmark_min_time=2x",
        "--benchmark_enable_random_interleaving",
        "--benchmark_out_format=json",
    ]

    elf_dir: pathlib.Path = args.output_dir / "elfs"
    elf_dir.mkdir(parents=True, exist_ok=True)

    json_dir: pathlib.Path = args.output_dir / "generator"
    json_dir.mkdir(parents=True, exist_ok=True)

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
        utils.run(generator_args + benchmark_common_args + benchmark_specific_args)


if __name__ == "__main__":
    main()
