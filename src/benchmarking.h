#ifndef BENCHMARKING_H
#define BENCHMARKING_H

#include <chrono>
#include <string>
#include <unordered_map>

class Benchmarking {
public:
  Benchmarking() : in_bench(false), bench_name("") { }

  void start(std::string s);
  void end();

  void dump();

private:
  std::unordered_map<std::string, long long> bench;

  bool in_bench;
  std::string bench_name;
  std::chrono::time_point<std::chrono::high_resolution_clock> last;
};

#endif
