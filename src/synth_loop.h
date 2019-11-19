#ifndef SYNTH_LOOP_H
#define SYNTH_LOOP_H

#include "logic.h"
#include "synth_enumerator.h"

struct Transcript;

void synth_loop(std::shared_ptr<Module> module, Options const& options);
void synth_loop_incremental(std::shared_ptr<Module> module, Options const& options);
void synth_loop_incremental_breadth(std::shared_ptr<Module> module, Options const& options);

//void synth_loop_from_transcript(std::shared_ptr<Module> module, int arity, int depth);

#endif
