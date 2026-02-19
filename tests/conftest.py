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
    "berkeley": {        # Test Stand 2
        "chip1": "0x3001a",
        "chip2": "0x3001b",
        "chip3": "0x3001c",
        "chip4": "0x3001d",
    },
    # Add additional test stands here:
}


#define stand variable
def pytest_addoption(parser):
    parser.addoption(
        "--stand",
        action="store"
    )

# Detect which chip group to use based on CI runner tag
def detect_chip_group():

    # Selects the correct chip ID group by matching GitLab runner tags 
    #(CI_RUNNER_TAGS) to CHIP_GROUPS keys.

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

#Parameterize tests
@pytest.fixture(
    params = list(CHIP_ID_MAP.items()),
    ids = lambda p: p[0],  # will name tests according to chip label only and not chip ID
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

#define golden scan paths
@pytest.fixture(scope='session')
def golden_image_path(request):
    return Path("test-data", request.config.getoption("--stand"), "golden-scans")

