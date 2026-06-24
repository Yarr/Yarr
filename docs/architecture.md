# YARR Architecture

This page is aimed at developers who want to understand the framework internals or add support for a new front-end chip or hardware controller.

## Core abstractions

The framework is built around four abstract interfaces defined in `src/libYarr/`:

| Class | Role |
|---|---|
| `HwController` | Combines `TxCore` (send commands) and `RxCore` (receive data). One instance per DAQ card or emulator. |
| `FrontEnd` | Represents a detector end-point handled as one object inside the DAQ. Holds the chip configuration and translates high-level commands into register writes via `TxCore`. |
| `LoopActionBase` | One level in a nested scan loop. `execPart1()` injects/triggers; inner loops run; `execPart2()` collects data. |
| `DataProcessor` | Processes raw data from a `ClipBoard<T>` queue in a separate thread (histogrammers and analysis algorithms). |

`ScanBase` / `LoopEngine` wire loop actions into a tree and drive execution. `Bookkeeper` is the central state bag: it holds all `FrontEnd` instances, `TxCore`/`RxCore` pointers, per-chip histograms, and pixel masks.

## Static factory / registry pattern

Every concrete class registers itself at program startup via a static boolean:

```cpp
// src/libSpec/SpecController.cpp
bool spec_registered =
    StdDict::registerHwController("spec",
        []() { return std::make_unique<SpecController>(); });

// src/libItkpixv2/Itkpixv2.cpp
bool itkpixv2_registered =
    StdDict::registerFrontEnd("ITKPIXV2",
        []() { return std::make_unique<Itkpixv2>(); });
```

The string key is what appears in JSON config files (`"type": "spec"`, `"chipType": "ITKPIXV2"`). The registries are:

| File | Registry |
|---|---|
| `src/libYarr/AllChips.cpp` | Front-end chips (`StdDict::registerFrontEnd`) |
| `src/libYarr/AllHwControllers.cpp` | Hardware controllers (`StdDict::registerHwController`) |
| `src/libYarr/AllStdActions.cpp` | Loop actions (`StdDict::registerLoopAction`) |
| `src/libYarr/AllAnalyses.cpp` | Analysis algorithms |
| `src/libYarr/AllHistogrammers.cpp` | Histogrammer algorithms |

Each chip library is linked into the monolithic `libYarr.so` via `-Wl,--no-as-needed` to ensure the static initialisers run even if no symbol is directly referenced.

## Scan execution flow

```
scanConsole
  └─ Bookkeeper          holds FrontEnds + TxCore/RxCore
       └─ LoopEngine     drives the nested loop stack
            ├─ OuterLoop.execPart1()   configure pixels / set parameter
            │    └─ InnerLoop.execPart1/2()  inject + trigger
            │         └─ StdDataLoop   drains RxCore into ClipBoard
            └─ OuterLoop.execPart2()   (optional post-step cleanup)

ClipBoard<RawData>  →  DataProcessor threads  →  ClipBoard<HistData>  →  Analysis
```

## Adding a new front-end chip

1. Create a new library directory `src/lib<ChipName>/`.
2. Subclass `FrontEnd` (and usually `FrontEndCfg`) for the chip's register map.
3. Register the class at file scope:
   ```cpp
   bool mychip_registered =
       StdDict::registerFrontEnd("MYCHIP",
           []() { return std::make_unique<MyChip>(); });
   ```
4. Add chip-specific `LoopActionBase` subclasses (mask loop, trigger loop, …) and register them with `StdDict::registerLoopAction`.
5. Add histogrammers and analysis algorithms and register them with `StdDict::registerHistogrammer` / `StdDict::registerAnalysis`.
6. Add the new library to `CMakeLists.txt` and include it in the `YARR_FRONT_ENDS_TO_BUILD` list.
7. Link the library into `libYarr.so` with `-Wl,--no-as-needed` so the static registrations fire.

## Adding a new hardware controller

1. Create a new library directory `src/lib<ControllerName>/`.
2. Subclass `HwController` (inherits `TxCore` + `RxCore`).
3. Register at file scope:
   ```cpp
   bool myctrl_registered =
       StdDict::registerHwController("myctrl",
           []() { return std::make_unique<MyController>(); });
   ```
4. Add the library to `CMakeLists.txt` and include it in `YARR_CONTROLLERS_TO_BUILD`.

## Key source locations

| Path | Contents |
|---|---|
| `src/libYarr/` | Core interfaces: `HwController`, `FrontEnd`, `LoopActionBase`, `Bookkeeper`, `ClipBoard`, `DataProcessor`, `ScanBase`, `LoopEngine` |
| `src/libUtil/` | Histograms (`Histo1d`/`2d`/`3d`), JSON (nlohmann), logging (spdlog) |
| `src/libScan/` | `ScanConsole`, `AllXxx` registries |
| `src/lib{Rd53b,Itkpixv2,Star,Rd53a,Fei4}/` | Chip-specific FrontEnd, loop actions, data processors |
| `src/lib{Spec,Emu,Bdaq,FelixClient}/` | Controller implementations |
