import numpy as np
import json
import pytest
import matplotlib.pyplot as plt
import os
from scipy.optimize import curve_fit
import pathlib


def load_json(filename):
    with open(filename, 'r') as f:
        return json.load(f)

#@pytest.mark.parametrize("chip_id", ["0x2008b", "0x2009a", "0x2009b", "0x200aa"])
def test_threshold_scan(chip_label, chip_id, golden_image_path):
    #define new and golden path files
    #new_file = f"{chip_id}_ThresholdMap-0_0-500-0-0-0.json"
    new_file = f"./outputs/threshold-scan/000001_std_thresholdscan_hr/{chip_id}_ThresholdMap-0_0-500-0-0-0.json"
    gold_file = golden_image_path / f"{chip_id}_ThresholdMap_golden.json"

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

    #Create histogram
    finally:
        #Make sure "./outputs" exists
        os.makedirs("./outputs", exist_ok=True)

        #Convert list to numpy array
        diffs=np.array(all_differences)
        masked_diffs = diffs[(diffs >= -150) & (diffs <= 150)]

        #Calculate RMS and Mean of the data
        mean = np.mean(diffs)
        rms = np.sqrt(np.mean(diffs**2))
        std = np.std(diffs)

        #Create Plot of Histogram
        plt.hist(all_differences, bins=6000, log=True)
        plt.title(f"Histogram of Pixel Differences for Threshold {chip_label}_{chip_id}")
        plt.xlabel("Difference")
        plt.ylabel("Frequency")

        plt.axvline(mean, color='red', linestyle='--', label=f"Mean = {mean:.3f}")
        plt.legend()

        #formatting plot
        text_str = f"Mean: {mean:.3f}\nRMS: {rms:.3f}\nSTD: {std:.3f}"
        plt.text(
            0.98, 0.95,
            text_str,
            transform=plt.gca().transAxes,
            fontsize=10,
            verticalalignment='top',
            horizontalalignment='right',
            bbox=dict(boxstyle="round,pad=0.4", facecolor="white", alpha=0.8)
        )
        plt.savefig(f"./outputs/diff_threshold_histogram_{chip_label}_{chip_id}.png")
        plt.close()

        #Calculate RMS and Mean of masked data
        masked_mean = np.mean(masked_diffs)
        masked_rms = np.sqrt(np.mean(masked_diffs**2))
        masked_std = np.std(masked_diffs)

        # Define Gaussian model for curve fitting
        def gaussian(x, A, mu, sigma):
            return A * np.exp(-0.5 * ((x - mu) / sigma)**2)

        # Compute histogram
        counts, bins = np.histogram(masked_diffs, bins=6000)
        x_centers = (bins[:-1] + bins[1:]) / 2

        # Fit Gaussian curve to histogram data
        p0 = [max(counts), masked_mean, masked_std]  # Initial guesses
        popt, _ = curve_fit(gaussian, x_centers, counts, p0=p0)
        A, mu, sigma = popt

        #Create Plot of Histogram
        plt.hist(masked_diffs, bins=6000, log=True)
        plt.title(f"Histogram of Pixel Differences for Threshold Range Masked {chip_label}_{chip_id}")
        plt.xlabel("Difference")
        plt.ylabel("Frequency")
        plt.yscale("log")
        plt.ylim(1,None)

        # Overlay fitted Gaussian
        x = np.linspace(bins[0], bins[-1], 1000)
        plt.plot(x, gaussian(x, A, mu, sigma), color="#ff7c7c", linewidth=2,
                label=f"Gaussian Fit (μ={mu:.3f}, σ={sigma:.3f})")
                
        #plot data mean line
        plt.axvline(mean, color='black', linestyle='--', label=f"Data mean = {masked_mean:.3f}")
        plt.legend()

        #plot mean gaussian line
        plt.axvline(mu, color='red', linestyle='--', label=f"Gaussian Fit Mean = {mu:.3f}")
        plt.legend()

        #formatting plot
        text_str = f"Mean: {masked_mean:.3f}\nRMS: {masked_rms:.3f}\nSTD: {masked_std:.3f}"
        plt.text(
            0.98, 0.95,
            text_str,
            transform=plt.gca().transAxes,
            fontsize=10,
            verticalalignment='top',
            horizontalalignment='right',
            bbox=dict(boxstyle="round,pad=0.4", facecolor="white", alpha=0.8)
        )
        plt.savefig(f"./outputs/diff_threshold_histogram_masked_{chip_label}_{chip_id}.png")
        plt.close()

        #Count number of pixels that are outside 3 standard deviations
        outliers = masked_diffs[(masked_diffs < (masked_mean - 3*masked_std)) | (masked_diffs > (masked_mean + 3*masked_std))]
        percent_outliers = len(outliers)/len(masked_diffs)
        threshold = 0.02

        fail_messages = []
        if masked_rms > 100:
            fail_messages.append(f"Masked RMS is over 100, gaussian too wide (RMS={masked_rms:.2f})")

        if percent_outliers > threshold:
            fail_messages.append(
                f"Pixel outliers over 2% threshold: {percent_outliers*100:.2f}% pixels are outside ±3σ range."
            )

        if fail_messages:
            pytest.fail("\n".join(fail_messages))