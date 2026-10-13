import argparse
import json

from common import utils


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate a config for use by ELF generator"
    )

    parser.add_argument(
        "--section-size",
        help="size of a section",
        required=True,
        type=utils.hex_int,
        default=0x1000,
    )
    parser.add_argument(
        "--sections-count", help="the number of sections", required=True, type=int
    )
    parser.add_argument("--dir", help="output directory", required=True)
    parser.add_argument(
        "--mode", help="generation_mode", required=True, choices=["nxn", "1xn"]
    )

    args = parser.parse_args()

    sections = [
        {"addr": (i + 1) * args.section_size, "size": args.section_size}
        for i in range(args.sections_count)
    ]

    with open(args.dir + "/config.json", "w") as config:
        match args.mode:
            case "nxn":
                json.dump(
                    {
                        "entry_point": args.section_size,
                        "segments": sections,
                        "sections": sections,
                    },
                    config,
                    sort_keys=True,
                    indent=2,
                )
            case "1xn":
                json.dump(
                    {
                        "entry_point": args.section_size,
                        "segments": [
                            {
                                "addr": args.section_size,
                                "size": args.section_size * args.sections_count,
                            }
                        ],
                        "sections": sections,
                    },
                    config,
                    sort_keys=True,
                    indent=2,
                )
            case _:
                raise RuntimeError(f"unknown mode {args.mode}")


if __name__ == "__main__":
    main()
