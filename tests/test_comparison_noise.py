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

def test_noise_scan(chip_label, chip_id, golden_image_path, stand_config):
    #define new and golden path files
    new_file = f"./outputs/threshold-scan/000001_std_thresholdscan_hr/{chip_id}_NoiseMap-0_0-500-0-0-0.json"
    gold_file = golden_image_path / f"{chip_id}_NoiseMap_golden.json"

    #before moving on, check that both file paths exist!
    assert pathlib.Path(new_file).exists(), f"Missing new scan: {new_file}"
    assert pathlib.Path(gold_file).exists(), f"Missing golden scan: {gold_file}"

    #load in the json files we want to compare, and only take the "Data" portion of the raw json file
    new_data = load_json(new_file)['Data']
    gold_data = load_json(gold_file)['Data']

    new_flat = np.array([v for row in new_data for v in row])
    gold_flat = np.array([v for row in gold_data for v in row])

    # Filter out pixels that are zero in one scan but not the other (XOR mask).
    # These pixels failed to fit in one scan but not the other and produce
    # artificial tails in the difference distribution.
    # Additionally filter out pixels that are zero in both scans.
    # These pixels consistently failed to fit in both scans and contribute
    # a spike at zero that distorts the Gaussian fit.
    # Only keep pixels with real measurements in both scans.
    mismatch_mask = (new_flat == 0) ^ (gold_flat == 0)
    both_zero_mask = (new_flat == 0) & (gold_flat == 0)
    valid = ~mismatch_mask & ~both_zero_mask
    new_flat = new_flat[valid]
    gold_flat = gold_flat[valid]

    all_differences = []
    differences = []

    try:
        diffs = new_flat - gold_flat
        all_differences = list(diffs)

        for idx, (gold_value, new_value, diff) in enumerate(zip(gold_flat, new_flat, diffs)):
            if gold_value != new_value:
                differences.append(f"Pixel {idx}: gold={gold_value}, new={new_value}, diff={diff}")

    finally:
        #Make sure "./outputs" exists
        os.makedirs("./outputs", exist_ok=True)

        #Convert list to numpy array
        diffs = np.array(all_differences)

        #Calculate RMS and Mean of the data
        mean = np.mean(diffs)
        rms = np.sqrt(np.mean(diffs**2))
        std = np.std(diffs)

        # Define Gaussian model for curve fitting
        def gaussian(x, A, mu, sigma):
            return A * np.exp(-0.5 * ((x - mu) / sigma)**2)

        # Try Gaussian fit but don't let failure prevent histogram from saving
        fit_success = False
        fit_error = None
        try:
            counts, bins = np.histogram(diffs, bins=6000)
            x_centers = (bins[:-1] + bins[1:]) / 2
            p0 = [max(counts), mean, std]
            popt, _ = curve_fit(gaussian, x_centers, counts, p0=p0)
            A, mu, sigma = popt
            fit_success = True
        except Exception as e:
            fit_error = str(e)
            print(f"\nWARNING: Gaussian fit failed for {chip_label} ({chip_id}): {fit_error}")
            mu = mean
            sigma = std

        # Load thresholds from stand config
        rms_max = stand_config["noise"]["rms_max"]
        outlier_max = stand_config["noise"]["outlier_pct"] / 100

        #Create Plot of Histogram
        plt.hist(all_differences, bins=6000, log=True)
        plt.title(f"Histogram of Pixel Differences for Threshold Noise {chip_label}_{chip_id}")
        plt.xlabel("Difference")
        plt.ylabel("Frequency")
        plt.yscale("log")
        plt.ylim(1, None)

        if fit_success:
            # Overlay fitted Gaussian
            x = np.linspace(bins[0], bins[-1], 1000)
            plt.plot(x, gaussian(x, A, mu, sigma), color="#ff7c7c", linewidth=2,
                    label=f"Gaussian Fit (μ={mu:.3f}, σ={sigma:.3f})")
            plt.axvline(mu, color='red', linestyle='--', label=f"Gaussian Fit Mean = {mu:.3f}")
        else:
            plt.text(0.5, 0.92, "WARNING: Gaussian fit failed",
                    transform=plt.gca().transAxes,
                    fontsize=11, color='red',
                    horizontalalignment='center',
                    bbox=dict(boxstyle="round,pad=0.4", facecolor="yellow", alpha=0.8))

        #plot mean line
        plt.axvline(mean, color='black', linestyle='--', label=f"Data mean = {mean:.3f}")
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
        plt.savefig(f"./outputs/diff_noise_histogram_{chip_label}_{chip_id}.png")
        plt.close()

        #Count number of pixels that are outside 3 standard deviations
        outliers = diffs[(diffs < (mean - 3*std)) | (diffs > (mean + 3*std))]
        percent_outliers = len(outliers)/len(diffs)

        print(f"\n--- Noise Scan Results: {chip_label} ({chip_id}) ---")
        print(f"Mean      : {mean:.3f}")
        print(f"RMS       : {rms:.3f} (threshold: {rms_max})")
        print(f"STD       : {std:.3f}")
        print(f"Outliers  : {percent_outliers*100:.2f}% (threshold: {stand_config['noise']['outlier_pct']}%)")

        fail_messages = []
        if rms > rms_max:
            fail_messages.append(
                f"RMS is over {rms_max}, gaussian too wide (RMS={rms:.2f})"
            )
        if percent_outliers > outlier_max:
            fail_messages.append(
                f"Pixel outliers over {stand_config['noise']['outlier_pct']}% threshold: "
                f"{percent_outliers*100:.2f}% pixels are outside ±3σ range."
            )

        if fail_messages:
            pytest.fail("\n".join(fail_messages))