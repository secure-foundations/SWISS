#ifndef SYNTH_LOOP_H
#define SYNTH_LOOP_H

#include "logic.h"
#include "synth_enumerator.h"

struct Transcript;

struct SynthesisResult {
  bool done;
  std::vector<value> new_values;
  std::vector<value> all_values;

  SynthesisResult() { }

  SynthesisResult(bool done,
        std::vector<value> const& new_values,
        std::vector<value> const& all_values)
    : done(done), new_values(new_values), all_values(all_values) { }
};

SynthesisResult synth_loop(
  std::shared_ptr<Module> module,
  std::vector<EnumOptions> const& enum_options,
  Options const& options,
  bool use_input_chunks,
  std::vector<SpaceChunk> const& chunks);

SynthesisResult synth_loop_incremental(
  std::shared_ptr<Module> module,
  std::vector<EnumOptions> const& enum_options,
  Options const& options);

SynthesisResult synth_loop_incremental_breadth(
  std::shared_ptr<Module> module,
  std::vector<EnumOptions> const& enum_options,
  Options const& options,
  bool use_input_chunks,
  std::vector<SpaceChunk> const& chunks,
  std::vector<value> all_invs,
  std::vector<value> new_invs);

//void synth_loop_from_transcript(std::shared_ptr<Module> module, int arity, int depth);

#endif
