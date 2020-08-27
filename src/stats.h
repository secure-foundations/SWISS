#ifndef STATS_H
#define STATS_H

#include <vector>
#include <string>
#include <fstream>

struct Stats {
  std::vector<long long> z3_times;
  std::vector<long long> cvc4_times;
  std::vector<long long> total_times;
  std::vector<long long> model_min_times;

  void add_z3(long long a) { z3_times.push_back(a); }
  void add_cvc4(long long a) { cvc4_times.push_back(a); }
  void add_total(long long a) { total_times.push_back(a); }
  void add_model_min(long long a) { model_min_times.push_back(a); }

  void dump_vec(std::ofstream& f, std::string const& name,
      std::vector<long long> const& vec)
  {
    f << name << "\n";
    for (long long i : vec) {
      f << i << "\n";
    }
  }

  void dump(std::string const& filename)
  {
    std::ofstream f;
    f.open(filename);
    dump_vec(f, "z3", z3_times);
    dump_vec(f, "cvc4", cvc4_times);
    dump_vec(f, "total", total_times);
    dump_vec(f, "model_min", model_min_times);
    f << std::endl;
  }
};

extern Stats global_stats;

#endif
