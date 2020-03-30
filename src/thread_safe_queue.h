#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include "synth_enumerator.h"

#include <mutex>

struct ThreadSafeQueue {
  std::vector<SpaceChunk> q;
  volatile int i;
  std::mutex m;

  ThreadSafeQueue() : i(0) { }

  SpaceChunk* getNextSpace() {
    m.lock();
    int j = this->i;
    this->i++;
    m.unlock();

    if (j >= (int)q.size()) {
      return NULL;
    }
    return &q[j];
  }
};

#endif
