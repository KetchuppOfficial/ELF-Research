import argparse
import pathlib
import subprocess
import json

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

def main() -> None:
    parser = argparse.ArgumentParser(description="Script for running benchmarks on generator")

    parser.add_argument("--generator",
                        help="path to the generator",
                        type=str_to_file,
                        required=True)

    parser.add_argument("--output-dir",
                        help="directory to store output in",
                        type=str_to_dir)

    parser.add_argument("--config",
                        help="path to the generation config",
                        type=str_to_file)

    parser.add_argument("--libs",
                        help="libraries to run generator for",
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

    elf_dir = args.output_dir / "elfs"
    json_dir = args.output_dir / "generator"

    for lib in args.libs:
        generator_args = [
            args.generator,
            "--lib", lib,
            "-o", elf_dir / f"{lib}-elf",
            "--config", args.config
        ]

        benchmark_specific_args: list[str] = [
            f"--benchmark_out={json_dir}/{lib}.json",
            f"--benchmark_context=library={lib},size={sections_count}"
        ]

        print(f"Running benchmark on {lib}")
        res = subprocess.run(generator_args + benchmark_common_args + benchmark_specific_args,
                             check=False)
        if res.returncode != 0:
            return

if __name__ == "__main__":
    main()
