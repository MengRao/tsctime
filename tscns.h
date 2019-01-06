/*
MIT License

Copyright (c) 2019 Meng Rao <raomeng1@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once
#include <time.h>

class TSCNS
{
public:
  // If you haven't calibrated tsc_ghz on this machine, set tsc_ghz as 0.0 and wait some time for calibration.
  // The wait time should be at least 1 second and the longer the more accurate for calibration.
  // We suggest that user waits as long as possible(e.g. for hours of sleep) once, and save the resultant tsc_ghz
  // returned from calibrate() somewhere(e.g. config file) on this machine for future use.
  //
  // If you have calibrated before on this machine as above, set tsc_ghz and skip calibration.
  void init(double tsc_ghz = 0.0) {
    syncTime(base_tsc, base_ns);
    if (tsc_ghz <= 0.0) return;
    tsc_ghz_inv = 1.0 / tsc_ghz;
    adjustOffset();
  }

  double calibrate() {
    uint64_t delayed_tsc, delayed_ns;
    syncTime(delayed_tsc, delayed_ns);
    tsc_ghz_inv = (double)(int64_t)(delayed_ns - base_ns) / (int64_t)(delayed_tsc - base_tsc);
    adjustOffset();
    return 1.0 / tsc_ghz_inv;
  }

  // You can change to using rdtscp if ordering is important
  uint64_t rdtsc() { return __builtin_ia32_rdtsc(); }

  uint64_t tsc2ns(uint64_t tsc) { return ns_offset + (int64_t)((int64_t)tsc * tsc_ghz_inv); }

  uint64_t rdns() { return tsc2ns(rdtsc()); }

  uint64_t rdsysns() {
    timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000000000 + ts.tv_nsec;
  }

private:
  // Kernel sync time by finding the first
  void syncTime(uint64_t& tsc, uint64_t& ns) {
    const int N = 10;
    uint64_t tscs[N + 1];
    uint64_t nses[N + 1];

    tscs[0] = rdtsc();
    for (int i = 1; i <= N; i++) {
      nses[i] = rdsysns();
      tscs[i] = rdtsc();
    }

    int best = N;
    for (int i = N - 1; i > 0; i--) {
      if (tscs[i] - tscs[i - 1] < tscs[best] - tscs[best - 1]) best = i;
    }
    tsc = (tscs[best] + tscs[best - 1]) >> 1;
    ns = nses[best];
  }

  void adjustOffset() { ns_offset = base_ns - (int64_t)((int64_t)base_tsc * tsc_ghz_inv); }

  alignas(64) double tsc_ghz_inv = 1.0; // make sure tsc_ghz_inv and ns_offset are on the same cache line
  uint64_t ns_offset = 0;
  uint64_t base_tsc = 0;
  uint64_t base_ns = 0;
};