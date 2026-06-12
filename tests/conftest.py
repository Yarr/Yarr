import os
import json
import pytest
from pathlib import Path

# Define chip groups (test stands)

CHIP_GROUPS = {
    "scipp": {        # Test Stand 1
        "chip1": "0x2008b",
        "chip2": "0x2009a",
        "chip3": "0x2009b",
        "chip4": "0x200aa",
    },
    "nakedsnail": {        # Test Stand 2
        "chip1": "0x262b3",
        "chip2": "0x262b4",
        "chip3": "0x262b5",
        "chip4": "0x262b6",
    },
    # Add additional test stands here:
}

# Define per-stand acceptance thresholds.
# Thresholds are stand-specific to account for known differences in module
# quality and operating conditions. The evaluation framework is identical
# for both stands.
# scipp: thresholds derived from months of operational data on an assembled module.
# nakedsnail: elevated RMS threshold reflects known characteristics of a non-ideal bare module.
# Add a new entry here when adding a new test stand.
STAND_CONFIGS = {
    "scipp": {
        "analog":    { "mean_diff_pct": 5.0, "pixel_diff_pct": 25.0 },
        "digital":   { "mean_diff_pct": 5.0, "pixel_diff_pct": 25.0 },
        "threshold": { "rms_max": 100, "outlier_pct": 2.0 },
        "noise":     { "rms_max": 100, "outlier_pct": 2.0 }
    },
    "nakedsnail": {
        # Loose analog thresholds reflect known degradation of chip2 on this module.
        # Golden should be retaken or module swapped out
        "analog":    { "mean_diff_pct": 20.0, "pixel_diff_pct": 40.0 },
        "digital":   { "mean_diff_pct": 5.0, "pixel_diff_pct": 25.0 },
        "threshold": { "rms_max": 1000, "outlier_pct": 3.0 },
        "noise":     { "rms_max": 200, "outlier_pct": 2.0 }
    },
    # Add additional test stands here:
}

# Define stand variable
def pytest_addoption(parser):
    parser.addoption(
        "--stand",
        action="store"
    )

# Detect which chip group to use based on CI runner tag
def detect_chip_group():

    # Selects the correct chip ID group by matching GitLab runner tags
    # (CI_RUNNER_TAGS) to CHIP_GROUPS keys.

    raw = os.getenv("CI_RUNNER_TAGS", "[]")

    try:
        runner_tags = json.loads(raw)
    except Exception:
        runner_tags = []

    for tag in runner_tags:
        if tag in CHIP_GROUPS:
            return CHIP_GROUPS[tag]

    raise RuntimeError(
        f"No matching chip group found for runner tags: {runner_tags}\n"
        f"Available chip groups: {list(CHIP_GROUPS.keys())}"
    )

# Load the correct chip ID set for this test run
CHIP_ID_MAP = detect_chip_group()

# Parameterize tests
@pytest.fixture(
    params=list(CHIP_ID_MAP.items()),
    ids=lambda p: p[0],  # will name tests according to chip label only and not chip ID
)
def chip(request):
    chip_label, chip_id = request.param
    return chip_label, chip_id

@pytest.fixture
def chip_label(chip):
    return chip[0]

@pytest.fixture
def chip_id(chip):
    return chip[1]

# Define golden scan paths
@pytest.fixture(scope='session')
def golden_image_path(request):
    return Path("test-data", request.config.getoption("--stand"), "golden-scans")

# Load per-stand acceptance thresholds
@pytest.fixture(scope='session')
def stand_config(request):
    stand = request.config.getoption("--stand")
    assert stand in STAND_CONFIGS, \
        f"No config found for stand '{stand}'. Available stands: {list(STAND_CONFIGS.keys())}"
    return STAND_CONFIGS[stand]