#!/usr/bin/env python

import subprocess
import sys
import time

server = subprocess.Popen(["./tools/test-fix-server", "-p", "9000", "-c", "fix"])
time.sleep(1)
result = 1
try:
    client = subprocess.Popen(["./tools/test-fix-client", "localhost", "9000", "fix"])
    result = client.wait()
finally:
    server.terminate()
    sys.exit(result)
