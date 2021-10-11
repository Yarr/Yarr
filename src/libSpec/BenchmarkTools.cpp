#include <BenchmarkTools.h>

#include <SpecCom.h>

#include <sys/time.h>
#include <stdint.h>
#include <string.h>

namespace BenchmarkTools {
   double measureWriteTime(SpecCom *spec, uint32_t addr, uint32_t *data, size_t size, int repetitions) {
       timeval start, end;
       gettimeofday(&start, NULL);
       for (int loop = 0; loop<repetitions; loop++)
           if (spec->writeDma(addr, data, size)) return -1;
       gettimeofday(&end, NULL);
       double time = (end.tv_sec - start.tv_sec) * 1000.0; //msecs
       time += (end.tv_usec - start.tv_usec) / 1000.0; //usecst
       return time;
   }

   double measureReadTime(SpecCom *spec, uint32_t addr, uint32_t *data, size_t size, int repetitions) {
       timeval start, end;
       gettimeofday(&start, NULL);
       for (int loop = 0; loop<repetitions; loop++)
           if (spec->readDma(addr, data, size)) return -1;
       gettimeofday(&end, NULL);
       double time = (end.tv_sec - start.tv_sec) * 1000.0; //msecs
       time += (end.tv_usec - start.tv_usec) / 1000.0; //usecst
       return time;
   }

   double measureWriteSpeed(SpecCom *spec, size_t package_size, int repetitions) {
       uint32_t *buffer = new uint32_t[package_size];
       memset(buffer, 0x5A, package_size*4);
       double time = BenchmarkTools::measureWriteTime(spec, 0x0, buffer, package_size, repetitions);
       double speed = (((package_size*4*repetitions)/1024.0/1024.0)/time)*1000.0;
       delete[] buffer;
       return speed;
   }
   
   double measureReadSpeed(SpecCom *spec, size_t package_size, int repetitions) {
       uint32_t *buffer = new uint32_t[package_size];
       memset(buffer, 0x5A, package_size*4);
       double time = BenchmarkTools::measureReadTime(spec, 0x0, buffer, package_size, repetitions);
       double speed = (((package_size*4*repetitions)/1024.0/1024.0)/time)*1000.0;
       delete[] buffer;
       return speed;
   }
}
