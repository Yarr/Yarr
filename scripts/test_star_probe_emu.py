import json
import subprocess

from pathlib import Path


def run_test_star(
    emu_config="configs/controller/emuCfg_star_ppb.json",
    sequence=None,
    do_resets=False,
    report_mappings=False,
    record_ids=False,
    show_output=False,
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

        output = Path("test_star_probe.out")
        if output.exists():
            output.unlink()

    print(f"Running {cmdline}")
    output = None
    if not show_output:
        output = subprocess.PIPE
    info = subprocess.run(cmdline, stdout=output)
    print(f"Complete with {info.returncode}")

    if record_ids and show_output:
        with open("test_star_probe.out") as log_file:
            print(" Start of ID logs")
            print("==============================")
            print(log_file.read())
            print("==============================")
            print(" End of ID logs")

    if info.returncode != 0:
        raise RuntimeError(f"Test sequence '{sequence}' failed")


def main():
    # Run default (was the default quick CI check)
    run_test_star()
    run_test_star(do_resets=True)

    # Check that some HPRs are received and don't have bad flags
    run_test_star(sequence="checkHCCHPRs")
    run_test_star(sequence="checkABCHPRs")
    run_test_star(sequence="probeHCCs")
    run_test_star(sequence="probeABCs")
    run_test_star(sequence="probeHCCs", show_output=True, record_ids=True)
    run_test_star(sequence="probeABCs", show_output=True, record_ids=True)


if __name__ == "__main__":
    main()
