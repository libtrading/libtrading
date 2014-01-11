# Libtrading

[![Build Status](https://secure.travis-ci.org/libtrading/libtrading.png?branch=master)](http://travis-ci.org/libtrading/libtrading)

Libtrading is an open source API for high-performance, low-latency trading
applications. It implements network protocols used for communicating with
exchanges, dark pools, and other trading venues. The API supports FIX,
FIX/FAST, and many proprietary protocols such as ITCH and OUCH used by NASDAQ.

## Features

* C API
* High performance, low latency
* FIX dialect support
* SystemTap/DTrace probes

## Install

Install prerequisite packages:

**Fedora**

```
$ yum install zlib-devel libxml2-devel glib2-devel vim-common ncurses-devel python-yaml
```

Then run:

```
$ make install
```

You can also run the test harness:

```
$ make check
```

## Usage

To measure FIX engine performance locally, start a FIX server:

```
$ ./tools/fix/fix_server -m 1 -p 7070
Server is listening to port 7070...
```

and then run the FIX client latency tester against it:

```
$ ./tools/fix/fix_client -n 100000 -m order -p 7070 -h localhost
Client Logon OK
Messages sent: 100000
Round-trip time: min/avg/max = 15.0/16.8/129.0 μs
Client Logout OK
```

## Documentation

* [Quick Start Guide](docs/quickstart.md)
* [Exchange Coverage](https://github.com/libtrading/libtrading/wiki/Exchange-Coverage)
* [Protocol Coverage](https://github.com/libtrading/libtrading/wiki/Protocol-Coverage)

## Performance

Protocol | RTT (μs)
---------|---------
FAST     | 13
FIX      | 16

The following above were obtained by running Libtrading messaging
ping-pong tests on a 2-way 2.7GHz Sandy Bridge i7 CPU running Fedora 19
with Linux 3.11.6.  The processes were pinned to separate physical cores
and the numbers include time spent in the Linux TCP/IP stack.

FIX engine round-trip time frequency plot for the above looks as follows:

<img src="http://libtrading.org/latency-frequency-plot.svg">

## License

Copyright (C) 2011-2014 Pekka Enberg and contributors

Libtrading is distributed under the 2-clause BSD license.

## Contributors

* Marat Stanichenko
* Jussi Virtanen
