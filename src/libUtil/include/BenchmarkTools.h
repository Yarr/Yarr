#ifndef BENCHMARKTOOLS_H
#define BENCHMARKTOOLS_H

#include <SpecController.h>
#include <stdint.h>

namespace BenchmarkTools {
    double measureWriteTime(SpecController *spec, uint32_t addr, uint32_t *data, size_t size, int repetitions);
    double measureReadTime(SpecController *spec, uint32_t addr, uint32_t *data, size_t size, int repetitions);
    double measureWriteSpeed(SpecController *spec, size_t package_size, int repetitions);
    double measureReadSpeed(SpecController *spec, size_t package_size, int repetitions);
}

#endif
