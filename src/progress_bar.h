#ifndef PROGRESS_BAR
#define PROGRESS_BAR

#include <cstdio>

class ProgressBar {
public:
  int cur;
  int total;

  ProgressBar(int total) : total(total) {
    cur = 0; 
    printf("0/%d", total);
    fflush(stdout);
  }

  void update(int amt) {
    cur = amt;
    if (amt % 100 == 0) {
      printf("\r%d/%d", cur, total);
      fflush(stdout);
    }
  }

  void finish() {
    printf("\r\n");
    fflush(stdout);
  }
};

#endif
