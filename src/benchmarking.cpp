#include "benchmarking.h"

#include <chrono>
#include <cassert>
#include <iostream>

using namespace std;

std::unordered_map<std::string, long long> total_bench;

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

  {
    auto iter = bench.find(bench_name);
    if (iter == bench.end()) {
      bench.insert(make_pair(bench_name, ms));
    } else {
      iter->second = iter->second + ms;
    }
  }

  if (include_in_global) {
    auto iter = total_bench.find(bench_name);
    if (iter == total_bench.end()) {
      total_bench.insert(make_pair(bench_name, ms));
    } else {
      iter->second = iter->second + ms;
    }
  }
}

void Benchmarking::dump() {
  for (auto p : bench) {
    std::cout << p.first << " : " << p.second << " ms" << std::endl;
    std::cout.flush();
  }
}

void benchmarking_dump_totals() {
  for (auto p : total_bench) {
    std::cout << "TOTAL " << p.first << " : " << p.second << " ms" << std::endl;
    std::cout.flush();
  }
}
