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

Two test binaries are produced:

```bash
bin/testYarr   # tests for libYarr (scan engine, loop actions, data processors)
bin/testUtil   # tests for libUtil (Histo1d/2d/3d, JSON helpers, logging)
```

Run both:

```bash
bin/testYarr && bin/testUtil
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
bin/star_test configs/controller/emuCfg_star.json -r 0 -t 0 -V Star_vH0A0
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
