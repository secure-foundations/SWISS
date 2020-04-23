#ifndef BENCHMARKING_H
#define BENCHMARKING_H

#include <chrono>
#include <string>
#include <unordered_map>

class Benchmarking {
public:
  Benchmarking(bool include_in_global = true) :
      include_in_global(include_in_global) ,
      in_bench(false),
      bench_name("") { }

  void start(std::string s);
  void end();

  void dump();

private:
  std::unordered_map<std::string, long long> bench;
  bool include_in_global;

  bool in_bench;
  std::string bench_name;
  std::chrono::time_point<std::chrono::high_resolution_clock> last;
};

void benchmarking_dump_totals();

inline std::chrono::time_point<std::chrono::high_resolution_clock>
now() {
  return std::chrono::high_resolution_clock::now();
}

inline long long as_ms(std::chrono::high_resolution_clock::duration dur)
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
}

inline long long as_ns(std::chrono::high_resolution_clock::duration dur)
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>(dur).count();
}

#endif
