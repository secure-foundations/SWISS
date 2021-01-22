import os
import protocol_parsing
import tempfile
import json
import subprocess
import sys

def main():
  if len(sys.argv) > 1:
    fname_one = sys.argv[1]
  else:
    fname_one = None

  for rl in sorted(list(os.listdir("benchmarks"))):
    if rl.endswith(".answers") and (fname_one == None or fname_one + ".answers" == rl):
      l = os.path.join('benchmarks', rl)
      pyv_name = l.replace(".answers", ".pyv")
      ivy_name = l.replace(".answers", ".ivy")
      if os.path.exists(ivy_name):
        name = ivy_name
      else:
        name = pyv_name
      print(name)
      mjf, j, answers = protocol_parsing.parse_module_and_invs_myparser(name, l)

      tfile = tempfile.mktemp()
      with open(tfile, "w") as f:
        f.write(json.dumps(answers))

      proc = subprocess.Popen(["./synthesis", "--check-inductiveness-linear", tfile, "--input-module", mjf])
      ret = proc.wait()
      assert ret == 0

if __name__ == '__main__':
  main()
