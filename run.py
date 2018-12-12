from file_to_json import get_json
import subprocess

def main():
  json_src = get_json()

  proc = subprocess.Popen(["./synthesis"], stdin=subprocess.PIPE)
  proc.communicate(json_src)
  proc.wait()

if __name__ == "__main__":
  main()
