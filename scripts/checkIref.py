import argparse
import json
import logging
import subprocess
import sys
from pathlib import Path

log = logging.getLogger(__name__)

parser = argparse.ArgumentParser(prog="PROG", usage="%(prog)s [options]")
# Remove default values before merging
parser.add_argument(
    "-c", "--connectivity_file", help="Specify connectivity config JSON path."
)
# for testing/developer purposes, use "../module-qc-database-tools/module_data/20UPIM13602155/20UPIM13602155_L2_warm.json",
parser.add_argument(
    "-r", "--hw_controller_file", help="Specify hardware controller JSON path."
)
# for testing/developer purposes, use "configs/controller/specCfg-rd53b-16x1.json",
parser.add_argument(
    "-n",
    "--number_of_chips",
    help="Specify number of chips in given module.",
    default=4,
)
args = parser.parse_args()


def get_chip_config_paths(connectivity_file: Path) -> list[str]:
    # NB: this code assumes that the chip order in the connectivity file is correct
    chip_config_paths = [Path()] * args.number_of_chips
    data = json.loads(connectivity_file.read_text())
    for i, chip_data in enumerate(data["chips"]):
        if chip_data["path"] == "relToCon":
            chip_config_paths[i] = connectivity_file.parent.joinpath(
                chip_data["config"]
            )
        else:
            msg = f"Cannot handle chip config with path={chip_data['path']} for chip at index={i}"
            raise RuntimeError(msg)
    return chip_config_paths


def fetchIref_fromConfig(chip_config_path: Path) -> int:
    chip_config = json.loads(chip_config_path.read_text())
    return chip_config["ITKPIXV2"]["Parameter"]["IrefTrim"]


def fetchIrefs_fromConfig(chip_config_paths) -> list[int]:
    Irefs = [-1] * args.number_of_chips
    for i, chip_config_path in enumerate(chip_config_paths):
        Irefs[i] = fetchIref_fromConfig(chip_config_path)
    log.info(Irefs)
    return Irefs


def fetchIrefs_fromReadRegister(
    hw_controller_file: Path, connectivity_file: Path
) -> list[int]:
    # cmd = './bin/read-register -r configs/controller/specCfg-rd53b-16x1.json -c ../module-qc-database-tools/module_data/20UPIM13602155/20UPIM13602155_L2_warm.json IrefTrimSense'
    cmd = (
        "./bin/read-register -r "
        + hw_controller_file
        + " -c "
        + connectivity_file
        + " IrefTrimSense"
    )
    with subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=True) as proc:
        output, error = proc.communicate()

    if error:
        log.error(error)

    Iref_array = [
        int(v)
        for v in str(output)
        .replace("'", "")
        .replace("b", "")
        .replace("\\n", ",")[:-1]
        .split(",")
    ]
    log.info(Iref_array)
    return Iref_array


def Iref_discrepancy_fixer(Irefs_config, Irefs_register):
    wc_fromRR = get_fourDigit_binary(Irefs_register)
    wc_fromCf = get_fourDigit_binary(Irefs_config)
    log.info("Current wirebonding configuration: %s", wc_fromRR)
    log.info("Correct wirebonding configuration: %s", wc_fromCf)


def get_fourDigit_binary(number):
    string = bin(number)[2:]
    while len(string) < 4:
        string = "0" + string
    return string


def main():
    """
    Purpose: read Iref value from register and from config file.
    These values should match.
    """
    connectivity_file = Path(args.connectivity_file).resolve()
    hw_controller_file = Path(args.hw_controller_file).resolve()
    chip_config_paths = get_chip_config_paths(connectivity_file)

    Irefs_config = fetchIrefs_fromConfig(chip_config_paths)
    Irefs_register = fetchIrefs_fromReadRegister(hw_controller_file, connectivity_file)

    GREEN = "\033[92m"
    RED = "\033[91m"
    RESET = "\033[0m"

    check = [False] * args.number_of_chips
    for i in range(0, len(check), 1):
        check[i] = Irefs_config[i] == Irefs_register[i]
        log.info(
            "Iref for chip %d matches expected value? %s%s = %s %s%s",
            GREEN if check[i] else RED,
            i + 1,
            Irefs_config[i],
            Irefs_register[i],
            check[i],
            RESET,
        )
        Iref_discrepancy_fixer(Irefs_config[i], Irefs_register[i])
    log.info(check)
    if not all(check):
        sys.exit(1)

    sys.exit(0)


if __name__ == "__main__":
    main()
