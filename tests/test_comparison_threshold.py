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

def test_threshold_scan(chip_label, chip_id, golden_image_path, stand_config):
    #define new and golden path files
    new_file = f"./outputs/threshold-scan/000001_std_thresholdscan_hr/{chip_id}_ThresholdMap-0_0-500-0-0-0.json"
    gold_file = golden_image_path / f"{chip_id}_ThresholdMap_golden.json"

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
    mismatch_mask = (new_flat == 0) ^ (gold_flat == 0)

    # Additionally filter out pixels that are zero in both scans.
    # These pixels consistently failed to fit in both scans and contribute
    # a spike at zero that distorts the Gaussian fit.
    both_zero_mask = (new_flat == 0) & (gold_flat == 0)

    # Combined mask - only keep pixels with real measurements in both scans
    combined_mask = ~mismatch_mask & ~both_zero_mask
    new_flat = new_flat[combined_mask]
    gold_flat = gold_flat[combined_mask]

    all_differences = []
    differences = []

    try:
        diffs = new_flat - gold_flat
        all_differences = list(diffs)

        for idx, (gold_value, new_value, diff) in enumerate(zip(gold_flat, new_flat, diffs)):
            if gold_value != new_value:
                differences.append(f"Pixel {idx}: gold={gold_value}, new={new_value}, diff={diff}")

    #Create histogram
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

        # Fit Gaussian on 300 bins for reliable convergence,
        # then plot histogram with 6000 bins for visual detail
        fit_success = False
        fit_error = None
        try:
            counts_fit, bins_fit = np.histogram(diffs, bins=300)
            x_centers_fit = (bins_fit[:-1] + bins_fit[1:]) / 2
            p0 = [max(counts_fit), mean, std]
            popt, _ = curve_fit(gaussian, x_centers_fit, counts_fit, p0=p0)
            A, mu_fit, sigma_fit = popt
            fit_success = True
        except Exception as e:
            fit_error = str(e)
            print(f"\nWARNING: Gaussian fit failed for {chip_label} ({chip_id}): {fit_error}")
            mu_fit = mean
            sigma_fit = std

        # Load thresholds from stand config
        rms_max = stand_config["threshold"]["rms_max"]
        outlier_max = stand_config["threshold"]["outlier_pct"] / 100

        # Plot histogram with 6000 bins
        n, bins_plot, _ = plt.hist(all_differences, bins=6000, log=True, color='steelblue', label='Data')
        plt.ylim(1, max(n) * 10)
        plt.title(f"Histogram of Pixel Differences for Threshold {chip_label}_{chip_id}")
        plt.xlabel("Difference")
        plt.ylabel("Frequency (log)")
        plt.yscale("log")

        if fit_success:
            x = np.linspace(bins_plot[0], bins_plot[-1], 1000)
            plt.plot(x, gaussian(x, A, mu_fit, sigma_fit), color="#ff7c7c", linewidth=2,
                    label=f"Gaussian Fit (μ={mu_fit:.3f}, σ={sigma_fit:.3f})")
            plt.axvline(mu_fit, color='red', linestyle='--', label=f"Gaussian Fit Mean = {mu_fit:.3f}")
        else:
            plt.text(0.5, 0.92, "WARNING: Gaussian fit failed",
                    transform=plt.gca().transAxes,
                    fontsize=11, color='red',
                    horizontalalignment='center',
                    bbox=dict(boxstyle="round,pad=0.4", facecolor="yellow", alpha=0.8))

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
        plt.savefig(f"./outputs/diff_threshold_histogram_{chip_label}_{chip_id}.png")
        plt.close()

        #Count number of pixels that are outside 3 standard deviations
        outliers = diffs[(diffs < (mean - 3*std)) | (diffs > (mean + 3*std))]
        percent_outliers = len(outliers)/len(diffs)

        print(f"\n--- Threshold Scan Results: {chip_label} ({chip_id}) ---")
        print(f"Mean      : {mean:.3f}")
        print(f"RMS       : {rms:.3f} (threshold: {rms_max})")
        print(f"STD       : {std:.3f}")
        print(f"Outliers  : {percent_outliers*100:.2f}% (threshold: {stand_config['threshold']['outlier_pct']}%)")

        fail_messages = []
        if rms > rms_max:
            fail_messages.append(
                f"RMS is over {rms_max}, gaussian too wide (RMS={rms:.2f})"
            )
        if percent_outliers > outlier_max:
            fail_messages.append(
                f"Pixel outliers over {stand_config['threshold']['outlier_pct']}% threshold: "
                f"{percent_outliers*100:.2f}% pixels are outside ±3σ range."
            )

        if fail_messages:
            pytest.fail("\n".join(fail_messages))