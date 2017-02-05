#ifndef BENCHMARKTOOLS_H
#define BENCHMARKTOOLS_H

#include <SpecCom.h>
#include <stdint.h>

namespace BenchmarkTools {
    double measureWriteTime(SpecCom *spec, uint32_t addr, uint32_t *data, size_t size, int repetitions);
    double measureReadTime(SpecCom *spec, uint32_t addr, uint32_t *data, size_t size, int repetitions);
    double measureWriteSpeed(SpecCom *spec, size_t package_size, int repetitions);
    double measureReadSpeed(SpecCom *spec, size_t package_size, int repetitions);
}

#endif
