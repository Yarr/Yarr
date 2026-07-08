import json
import subprocess


def run_test_star(
    emu_config="configs/controller/emuCfg_star_ppb.json",
    sequence=None,
    do_resets=False,
    report_mappings=False,
    record_ids=False,
):
    cmdline = ["bin/test_star", emu_config]

    if report_mappings:
        # Report mapping between actions
        cmdline.append("-v")

    if do_resets:
        cmdline.append("-R")

    rx_list = [0]
    tx_list = [0]

    cmdline.extend(["-r"] + [f"{r}" for r in rx_list])
    cmdline.extend(["-t"] + [f"{t}" for t in tx_list])

    if sequence is not None:
        cmdline.extend(["-s", sequence])

    if record_ids:
        # Save IDs to test_star_probe.out
        cmdline.extend(["-l", "configs/logging/trace_star_id_to_file.json"])

    print(f"Running {cmdline}")
    info = subprocess.run(cmdline)
    print(f"Complete with {info.returncode}")

    if info.returncode != 0:
        raise RuntimeError(f"Test sequence '{sequence}' failed")


def main():
    # Run default (was the default quick CI check)
    run_test_star()
    run_test_star(do_resets=True)

    # Check that some HPRs are received and don't have bad flags
    run_test_star(sequence="checkHCCHPRs")
    run_test_star(sequence="checkABCHPRs")


if __name__ == "__main__":
    main()
