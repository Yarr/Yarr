# Running Tests

YARR uses [Catch2](https://github.com/catchorg/Catch2) for unit and integration tests. The test binaries are built when `BUILD_TESTS=on` is passed to cmake.

## Building the tests

```bash
mkdir build && cd build
cmake .. -DBUILD_TESTS=on
make -j$(nproc) install
cd ..
```

## Running the tests

The following test binaries are produced (all require `BUILD_TESTS=on`):

| Binary | Library | What it covers |
|---|---|---|
| `bin/testYarr` | libYarr | Scan engine, loop actions, data processors |
| `bin/testUtil` | libUtil | Histo1d/2d/3d, JSON helpers, logging |
| `bin/testEmu` | libEmu | ClipBoard/queue internals |
| `bin/testFei4` | libFei4 | FEI4 feedback loop |
| `bin/testStarEmu` | libStarEmu | Star emulator, broadcast, packet encoding |
| `bin/testStar` | libStar | Star/ABC analysis, chip config, scan loops, data processor |
| `bin/testItkpixv2` | libItkpixv2 | ITkPixV2 data processor (including edge cases, error handling, benchmarks) |
| `bin/testRd53b` | libRd53b | RD53B data processor (including edge cases and benchmarks) |
| `bin/testFelix` | libFelixClient | FELIX elink control (only built when a FELIX backend is available) |

Run all unconditional tests:

```bash
bin/testYarr && bin/testUtil && bin/testEmu && bin/testFei4 && \
bin/testStarEmu && bin/testStar && bin/testItkpixv2 && bin/testRd53b
```

### Filtering by tag or name

Catch2 supports filtering by tag (`[tag]`) or substring of the test name:

```bash
bin/testYarr '[occupancy]'       # run only occupancy-related tests
bin/testYarr 'loop engine'       # run tests whose name contains "loop engine"
bin/testYarr '[!benchmark]'      # skip slow benchmarks
```

Run `bin/testYarr --list-tests` or `bin/testYarr --list-tags` to see all available tests and tags.

## Emulator-based integration tests

Full scan integration tests use the ITkPixV2 software emulator and require no hardware:

```bash
# Reset the run counter first so output directories are predictable
truncate $HOME/.yarr/runCounter --size=0

bin/scanConsole \
  -r configs/controller/emuCfg_itkpixv2.json \
  -c configs/connectivity/example_itkpixv2_setup.json \
  -s configs/scans/itkpixv2/std_digitalscan.json
```

For Star/ABC strip emulator tests:

```bash
bin/test_star configs/controller/emuCfg_star.json -r 0 -t 0 -V Star_vH0A0
bash scripts/test_star_scans_emu.sh
```

## Adding new tests

Tests live alongside their library under `src/lib<Name>/tests/`. Each test file is a standard Catch2 source file. Add it to the corresponding `CMakeLists.txt` with `target_sources(test<Name> PRIVATE mytest.cpp)` (see existing `CMakeLists.txt` files in those directories for the pattern).

Tag your test cases with descriptive tags to make filtering easy:

```cpp
TEST_CASE("My feature works correctly", "[myfeature]") {
    REQUIRE(myFunction() == expectedValue);
}
```
