#!/usr/bin/env python

import subprocess
import getopt
import signal
import time
import sys

numfail = 0

fix_tests = [
  #
  # FIX session level test cases. See Volume 2 of the FIX 4.4 specification for
  # details.
  #
  "2b_MsgSeqNumHigherThanExpected",
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

def popen_fix_server():
    args = [
      "./tools/fix/fix_server",
      "-p", "9000", 
      "-c", "fix",
      "-f", "./tools/fix/scripts/%s" % test]
    return subprocess.Popen(args, stderr = err, stdout = out)

def popen_fix_client():
    args = [
      "./tools/fix/fix_client",
      "-h", "localhost", 
      "-p", "9000", 
      "-c", "fix",
      "-f", "./tools/fix/scripts/%s" % test]
    return subprocess.Popen(args, stderr = err, stdout = out)

for test in fix_tests:
    if verbose == 2:
        print "%s\n%s\n" % (test, "-" * len(test))

    server = popen_fix_server()

    result = 1
    try:
        client = popen_fix_client()
        result = client.wait()
    finally:
	if result != 0:
            if verbose == 2:
                status = "\n%s\n" % red("FAIL")
            else:
                status = "%-40s [%s]" % (test, red("FAIL"))

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
                status = "%-40s [%s]" % (test, green("PASS"))

            if verbose != 0:
                print status

if numfail > 0:
    print "\n" + red("FIX tests run: %d, Failures: %d" % (len(fix_tests), numfail))
else:
    print "\nOK (%d FIX tests)" % (len(fix_tests))

if numfail != 0:
    result = 1
else:
    result = 0

if result != 0:
    sys.exit(result)

fast_tests = [
  "IncrementOperator",
  "ConstantOperator",
  "DeltaOperator",
  "CopyOperator",
  "NoneOperator",
]

def popen_fast_server():
    args = [
      "./tools/fast/fast_server",
      "-p", "9000",
      "-c", "fast",
      "-f", "./tools/fast/scripts/%s" % test,
      "-t", "./tools/fast/scripts/%s.xml" % test]
    return subprocess.Popen(args, stderr = err, stdout = out)

def popen_fast_client():
    args = [
      "./tools/fast/fast_client",
      "-h", "localhost",
      "-p", "9000",
      "-c", "fast",
      "-f", "./tools/fast/scripts/%s" % test,
      "-t", "./tools/fast/scripts/%s.xml" % test]
    return subprocess.Popen(args, stderr = err, stdout = out)

for test in fast_tests:
    if verbose == 2:
        print "%s\n%s\n" % (test, "-" * len(test))

    server = popen_fast_server()

    result = 1
    try:
        client = popen_fast_client()
        result = client.wait()
    finally:
	if result != 0:
            if verbose == 2:
                status = "\n%s\n" % red("FAIL")
            else:
                status = "%-40s [%s]" % (test, red("FAIL"))

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
                status = "%-40s [%s]" % (test, green("PASS"))

            if verbose != 0:
                print status

if numfail > 0:
    print "\n" + red("FAST tests run: %d, Failures: %d" % (len(fast_tests), numfail))
else:
    print "\nOK (%d FAST tests)" % (len(fast_tests))

if numfail != 0:
    result = 1
else:
    result = 0

sys.exit(result)
