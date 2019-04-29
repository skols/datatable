//------------------------------------------------------------------------------
// Copyright 2018 H2O.ai
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//------------------------------------------------------------------------------
#ifdef DTTEST
#include <atomic>
#include <vector>
#include "parallel/api.h"
#include "utils/exceptions.h"
#include "ztest.h"
#include <omp.h>
#include "utils/time_it.h"
#include "utils/time_it_omp.h"
#include <iomanip>
#include "utils/macros.h"


namespace dttest {

void benchmark_parallel_for_static(size_t n, size_t nthreads) {
  if (nthreads == 0) nthreads = dt::num_threads_in_pool();
  std::vector<size_t> data(n);
  std::vector<cache_aligned<int64_t>> time_start_omp(nthreads, 0);
  std::vector<cache_aligned<int64_t>> time_start_tp(nthreads, 0);
  std::vector<cache_aligned<int64_t>> time_omp(nthreads, 0);
  std::vector<cache_aligned<int64_t>> time_tp(nthreads, 0);
  std::vector<cache_aligned<int64_t>> total_start_time(2, 0);
  std::vector<cache_aligned<int64_t>> total_time(2, 0);

  auto fn = [](size_t i) {
    size_t j = 0;
    for (size_t k = 0; k < 1000; ++k) {
      j += k * i;
    }
    return j;
  };

  {
    TimeIt t("omp_parallel_for");
    #pragma omp parallel for num_threads(nthreads)
    for (size_t i = 0; i < n; ++i) {
      size_t ith = static_cast<size_t>(omp_get_thread_num());
      if (time_omp[ith].v == 0) time_start_omp[ith].v = t.time();
      data[i] = fn(i);
      time_omp[ith].v = t.time();
    }
    total_time[0].v = t.time();
  }


  {
    TimeIt t("dt::parallel_for_static");
    dt::parallel_for_static(n,
      4096,
      nthreads,
      [&](size_t i) {
        size_t ith = dt::this_thread_index();
        if (time_tp[ith].v == 0) time_start_tp[ith].v = t.time();
        data[i] = fn(i);
        time_tp[ith].v = t.time();
      }
    );
    total_time[1].v = t.time();
  }

  int width = 15;

  std::cout << std::setw(width) << ""
            << std::setw(width) << "Start Time [us]"
            << std::setw(width) << ""
            << std::setw(width) << "Stop Time [us]"
            << "\n";

  std::cout << std::setw(width) << "Thread"
            << std::setw(width) << "OpenMP"
            << std::setw(width) << "Thread Pool"
            << std::setw(width) << "OpenMP"
            << std::setw(width) << "Thread Pool"
            << "\n";

  std::cout.imbue(std::locale(""));

  for (size_t i = 0; i < nthreads; ++i) {
    std::cout << std::setw(width) << i
              << std::setw(width) << time_start_omp[i].v
              << std::setw(width) << time_start_tp[i].v
              << std::setw(width) << time_omp[i].v
              << std::setw(width) << time_tp[i].v
              << "\n";
    total_start_time[0].v += time_start_omp[i].v;
    total_start_time[1].v += time_start_tp[i].v;

  }

  std::cout << std::setfill('-') << std::setw(width*5 + 1)
            << "\n";


  std::cout << std::setfill(' ')
            << std::setw(width) << "Total [us]"
            << std::setw(width) << total_start_time[0].v
            << std::setw(width) << total_start_time[1].v
            << std::setw(width) << total_time[0].v
            << std::setw(width) << total_time[1].v
            << "\n";

  std::cout << "\n";
}

} // namespace dttest
#endif
