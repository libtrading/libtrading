#!/usr/bin/env python

from threading import Thread
import os, signal, sys, time

def start(program, *args):
  pid = os.fork()
  if not pid:
    os.execvp(program, (program,) +  args)
  return pid

def wait():
  return os.wait()[1]

def main():
  server_pid = start("./fix", "server", "-p", "9000")
  time.sleep(1)
  result = 1
  try:
    start("./fix", "client")
    result = os.wait()[1] >> 8
  finally:
    os.kill(server_pid, signal.SIGKILL)
    sys.exit(result)

if __name__ == '__main__':
  main()
