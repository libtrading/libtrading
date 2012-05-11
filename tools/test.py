#!/usr/bin/env python

import os
import signal
import sys
import time

def start(program, *args):
  pid = os.fork()
  if not pid:
    os.execvp(program, (program,) +  args)
  return pid

def wait():
  return os.wait()[1] >> 8

def main():
  server_pid = start("./trade", "server", "-p", "9000", "-c", "fix")
  time.sleep(1)
  result = 1
  try:
    start("./trade", "client", "localhost", "9000", "fix")
    result = wait()
  finally:
    os.kill(server_pid, signal.SIGKILL)
    sys.exit(result)

if __name__ == '__main__':
  main()
