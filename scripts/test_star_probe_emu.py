import json
import subprocess


def run_test_star(
    emu_config="configs/controller/emuCfg_star_ppb.json",
    sequence=None,
):
    cmdline = ["bin/test_star", emu_config]

    rx_list = [0]
    tx_list = [0]

    cmdline.extend(["-r"] + [f"{r}" for r in rx_list])
    cmdline.extend(["-t"] + [f"{t}" for t in tx_list])

    if sequence is not None:
        cmdline.extend(["-s", sequence])

    print(f"Running {cmdline}")
    info = subprocess.run(cmdline)
    print(f"Complete with {info.returncode}")

    if info.returncode != 0:
        raise RuntimeError(f"Test sequence '{sequence}' failed")


def main():
    run_test_star()


if __name__ == "__main__":
    main()
