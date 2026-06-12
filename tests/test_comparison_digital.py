import numpy as np
import json
import pytest
import matplotlib.pyplot as plt
import os
import pathlib


def load_json(filename):
    with open(filename, 'r') as f:
        return json.load(f)

#@pytest.mark.parametrize("chip_id", ["0x2008b", "0x2009a", "0x2009b", "0x200aa"])
def test_digital_scan(chip_label, chip_id, golden_image_path):
    #define the json files we want to compare
    new_file = f"./outputs/digital-scan/000001_std_digitalscan/{chip_id}_OccupancyMap.json"
    gold_file = golden_image_path / f"{chip_id}_digital_golden.json"

    #before moving on, check that both file paths exist!
    assert pathlib.Path(new_file).exists(), f"Missing new scan: {new_file}"
    assert pathlib.Path(gold_file).exists(), f"Missing golden scan: {gold_file}"

    #load in the json files we want to compare, and only take the "Data" portion of the raw json file
    new_data = load_json(new_file)['Data']
    gold_data = load_json(gold_file)['Data']

    new_flat = np.array([v for row in new_data for v in row])
    gold_flat = np.array([v for row in gold_data for v in row])

    all_differences = []

    try:
        for i in range(len(gold_data)):
            gold_row = gold_data[i]
            new_row = new_data[i]

            for j in range(len(gold_row)):
                gold_value = gold_row[j]
                new_value = new_row[j]
                diff = new_value - gold_value
                all_differences.append(diff)

        # Compute mean occupancy difference
        new_mean = new_flat.mean()
        gold_mean = gold_flat.mean()
        mean_diff_pct = abs(new_mean - gold_mean) / gold_mean * 100

        # Compute pixel difference percentage
        pixels_differing = (new_flat != gold_flat).sum()
        pixels_differing_pct = pixels_differing / len(new_flat) * 100

        # Print percentages so user can always see them regardless of pass/fail
        print(f"\n--- Digital Scan Results: {chip_label} ({chip_id}) ---")
        print(f"Golden mean occupancy : {gold_mean:.3f}")
        print(f"New scan mean occupancy: {new_mean:.3f}")
        print(f"Mean difference       : {mean_diff_pct:.2f}% (threshold: 5%)")
        print(f"Pixels differing      : {pixels_differing} ({pixels_differing_pct:.2f}%) (threshold: 25%)")

        fail_messages = []
        if mean_diff_pct > 5:
            fail_messages.append(
                f"Mean occupancy changed by {mean_diff_pct:.2f}%, exceeds 5% threshold "
                f"(golden={gold_mean:.3f}, new={new_mean:.3f})"
            )
        if pixels_differing_pct > 25:
            fail_messages.append(
                f"Pixels differing from golden: {pixels_differing_pct:.2f}%, exceeds 25% threshold "
                f"({pixels_differing} out of {len(new_flat)} pixels)"
            )

        if fail_messages:
            pytest.fail("\n".join(fail_messages))

    #Create plots
    finally:
        os.makedirs("./outputs", exist_ok=True)

        fig, axes = plt.subplots(1, 2, figsize=(14, 5))

        # Left plot: occupancy overlay
        axes[0].hist(new_flat, bins=50, log=True, color='steelblue', label='New scan')
        axes[0].hist(gold_flat, bins=50, log=True, color='orange', alpha=0.6, label='Golden')
        axes[0].set_title(f"Occupancy Distribution {chip_label}_{chip_id}")
        axes[0].set_xlabel("Occupancy")
        axes[0].set_ylabel("Frequency (log)")
        axes[0].legend()

        # Right plot: difference histogram
        axes[1].hist(all_differences, bins=10, log=True, color='steelblue')
        axes[1].set_title(f"Pixel Differences {chip_label}_{chip_id}")
        axes[1].set_xlabel("Difference")
        axes[1].set_ylabel("Frequency (log)")

        plt.tight_layout()
        plt.savefig(f"./outputs/diff_histogram_digital_{chip_label}_{chip_id}.png")
        plt.close()
