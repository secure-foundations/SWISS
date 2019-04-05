#include "benchmarking.h"

#include <chrono>
#include <cassert>
#include <iostream>

using namespace std;

void Benchmarking::start(string s) {
  assert(!in_bench);
  in_bench = true;
  bench_name = s;
  last = chrono::high_resolution_clock::now();
}

void Benchmarking::end() {
  assert(in_bench);
  in_bench = false;

  auto end = chrono::high_resolution_clock::now();
  auto dur = end - last;
  long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();

  auto iter = bench.find(bench_name);
  if (iter == bench.end()) {
    bench.insert(make_pair(bench_name, ms));
  } else {
    iter->second = iter->second + ms;
  }
}

void Benchmarking::dump() {
  for (auto p : bench) {
    std::cout << p.first << " : " << p.second << " ms" << std::endl;
    std::cout.flush();
  }
}
