import numpy as np
import json
import pytest
import matplotlib.pyplot as plt
import os
import pathlib


def load_json(filename):
    with open(filename, 'r') as f:
        return json.load(f)

GOLDEN_FILE = "golden_digital.json"

#@pytest.mark.parametrize("chip_id", ["0x2008b", "0x2009a", "0x2009b", "0x200aa"])
def test_digital_scan(chip_label, chip_id, golden_image_path):
    #define the json files we want to compare
    new_file = f"./outputs/digital-scan/000001_std_digitalscan/{chip_id}_OccupancyMap.json"
    gold_file = golden_image_path / GOLDEN_FILE  #same for all chips

    #before moving on, check that both file paths exist!
    assert pathlib.Path(new_file).exists(), f"Missing new scan: {new_file}"
    assert pathlib.Path(gold_file).exists(), f"Missing golden scan: {gold_file}"

    #load in the json files we want to compare, and only take the "Data" portion of the raw json file
    new_data = load_json(new_file)['Data']
    gold_data = load_json(gold_file)['Data']

    all_differences = []
    differences = []

    try:
        for i in range(len(gold_data)):
            gold_row = gold_data[i]
            new_row = new_data[i]
            
            for j in range(len(gold_row)):
                gold_value = gold_row[j]
                new_value = new_row[j]

                diff = new_value - gold_value
                all_differences.append(diff)

                if gold_value != new_value:
                    differences.append(f"Pixel ({i}, {j}): gold={gold_value}, new={new_value}, diff={diff}")
        
        if differences:
            diff_report = "\n".join(differences)
            pytest.fail(f"Pixel differences found:\n{diff_report}")


    #Create histogram
    finally:
        os.makedirs("./outputs", exist_ok=True)
        plt.hist(all_differences, bins=10, log=True)
        plt.title(f"Histogram of Pixel Differences for Digital Scan {chip_label}_{chip_id}")
        plt.xlabel("Difference")
        plt.ylabel("Frequency")
        plt.savefig(f"./outputs/diff_histogram_digital_{chip_label}_{chip_id}.png")
        plt.close()