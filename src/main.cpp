#include "logic.h"
#include "contexts.h"

#include <iostream>
#include <iterator>
#include <string>

using namespace std;

int main() {
  std::istreambuf_iterator<char> begin(std::cin), end;
  std::string json_src(begin, end);

  shared_ptr<Module> module = parse_module(json_src);

  z3::context ctx;

  auto bgctx = shared_ptr<BackgroundContext>(new BackgroundContext(ctx, module));
  auto indctx = shared_ptr<InductionContext>(new InductionContext(bgctx, module));
}
