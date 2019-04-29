import datatable as dt
from datatable.lib import core

n = 1000000
max_num_threads = 8

for nthreads in range(max_num_threads):
  core.benchmark_parallel_for_static(n = n, nthreads = nthreads + 1)


