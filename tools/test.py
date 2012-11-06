#!/usr/bin/env python

import subprocess
import getopt
import signal
import time
import sys

numfail = 0

tests = [
  #
  # FIX session level test cases. See Volume 2 of the FIX 4.4 specification for
  # details.
  #
  "4b_TestRequestMessageIsReceived"
]

def green(s):
    return "\033[32m" + s + "\033[0m"

def red(s):
    return "\033[31m" + s + "\033[0m"


try:
    opts, args = getopt.getopt(sys.argv[1:], "v")
except getopt.GetoptError, err:
    print str(err)
    sys.exit(1)

verbose = 0

for o, a in opts:
    if o == "-v":
        verbose += 1

if verbose == 2:
  out = open('/dev/stdout', 'w')
  err = open('/dev/stderr', 'w')
else:
  out = open('/dev/null', 'w')
  err = open('/dev/null', 'w')

def popen_server():
    args = [
      "./tools/fix/fix_server",
      "-p", "9000", 
      "-c", "fix",
      "-f", "./tools/fix/scripts/%s" % test]
    return subprocess.Popen(args, stderr = err, stdout = out)

def popen_client():
    args = [
      "./tools/fix/fix_client",
      "-h", "localhost", 
      "-p", "9000", 
      "-c", "fix",
      "-f", "./tools/fix/scripts/%s" % test]
    return subprocess.Popen(args, stderr = err, stdout = out)

for test in tests:
    if verbose == 2:
        print "%s\n%s\n" % (test, "-" * len(test))

    server = popen_server()

    result = 1
    try:
        client = popen_client()
        result = client.wait()
    finally:
	if result != 0:
            if verbose == 2:
                status = "\n%s\n" % red("FAIL")
            else:
                status = "%-40s [%s]\n" % (test, red("FAIL"))

            print status

            numfail = numfail + 1

            while server.poll() is None:
                server.terminate()
            while client.poll() is None:
                client.terminate()
        else:
            if verbose == 2:
                status = "\n%s\n" % green("PASS")
            elif verbose == 1:
                status = "%-40s [%s]\n" % (test, green("PASS"))

            if verbose != 0:
                print status

if numfail > 0:
    print red("Tests run: %d, Failures: %d" % (len(tests), numfail))
else:
    print "OK (%d tests)" % (len(tests))

if numfail != 0:
    result = 1
else:
    result = 0
sys.exit(result)
