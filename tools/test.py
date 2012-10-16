#!/usr/bin/env python

import subprocess
import signal
import sys
import time

server = subprocess.Popen(["./tools/test-fix-server", "-p", "9000", "-c", "fix"])
result = 1
try:
    client = subprocess.Popen(["./tools/test-fix-client", "-i", "localhost", "9000", "fix"])
    result = client.wait()
finally:
    server.terminate()
    sys.exit(result)
