#ifndef SYNTH_LOOP_H
#define SYNTH_LOOP_H

#include "logic.h"

struct Transcript;

void synth_loop(std::shared_ptr<Module> module, int arity, int depth,
    Transcript* init_transcript = NULL);

void synth_loop_incremental(std::shared_ptr<Module> module, int arity, int depth);

void synth_loop_from_transcript(std::shared_ptr<Module> module, int arity, int depth);

#endif
