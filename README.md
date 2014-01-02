# Libtrading

[![Build Status](https://secure.travis-ci.org/penberg/libtrading.png?branch=master)](http://travis-ci.org/penberg/libtrading)

Libtrading is an open source API for high-performance, low-latency trading
applications. It implements network protocols used for communicating with
exchanges, dark pools, and other trading venues. The API supports FIX,
FIX/FAST, and many proprietary protocols such as ITCH and OUCH used by NASDAQ.

## Features

* C API
* High performance, low latency
* FIX dialect support
* SystemTap/DTrace probes

### Performance

Protocol | RTT (μs)
---------|---------
FAST     | 13
FIX      | 16

The following above were obtained by running Libtrading messaging
ping-pong tests on a 2-way 2.7GHz Sandy Bridge i7 CPU running Fedora 19
with Linux 3.11.6.  The processes were pinned to separate physical cores
and the numbers include time spent in the Linux TCP/IP stack.

### Exchange Coverage

Exchange              | Market Data   | Order Entry | Status
----------------------|---------------|-------------|-----------
Moscow Exchange       | FAST/FIX      | FIX         | certified
BATS BZX Exchange     | PITCH         | BOE, FIX    |
IEX                   |               | FIX         |
London Stock Exchange | ITCH          |             |
MB Trading            | MBT Quote API | FIX         |
NASDAQ                | ITCH          | OUCH, FIX   |
NASDAQ OMX Nordic     | ITCH          | FIX         |

### Protocol Coverage

Protocol              | API
----------------------|------
[FAST][]              | ([header](https://github.com/penberg/libtrading/blob/master/include/libtrading/proto/fast_message.h))
[FIX][]               | ([header](https://github.com/penberg/libtrading/blob/master/include/libtrading/proto/fix_message.h))
[LSE ITCH][]          | ([header](https://github.com/penberg/libtrading/blob/master/include/libtrading/proto/lse_itch_message.h))
[MBT Quote API][]     | ([header](https://github.com/penberg/libtrading/blob/master/include/libtrading/proto/mbt_quote_message.h))
[NASDAQ ITCH 4.0][]   | ([header](https://github.com/penberg/libtrading/blob/master/include/libtrading/proto/nasdaq_itch40_message.h))
[NASDAQ ITCH 4.1][]   | ([header](https://github.com/penberg/libtrading/blob/master/include/libtrading/proto/nasdaq_itch41_message.h))
[NASDAQ Nordic Equity TotalView-ITCH 1.86][] | ([header](https://github.com/penberg/libtrading/blob/master/include/libtrading/proto/omx_itch186_message.h))
[NASDAQ OUCH 4.2][]   | ([header](https://github.com/penberg/libtrading/blob/master/include/libtrading/proto/ouch42_message.h))
[NASDAQ SoupBinTCP][] | ([header](https://github.com/penberg/libtrading/blob/master/include/libtrading/proto/soupbin3_session.h))
[NYSE Arca XDP][]     | ([header](https://github.com/penberg/libtrading/blob/master/include/libtrading/proto/xdp_message.h))
[NYSE Daily TAQ][]    | ([header](https://github.com/penberg/libtrading/blob/master/include/libtrading/proto/nyse_taq_message.h))

[BATS BOE]:          http://www.batstrading.co.uk/resources/participant_resources/BATS_Europe_Binary_Order_Entry_Specification.pdf
[BATS PITCH]:        http://www.batstrading.com/resources/membership/BATS_PITCH_Specification.pdf
[FIX]:               http://fixprotocol.org/specifications/
[FAST]:              http://fixprotocol.org/fastspec/
[LSE ITCH]:          http://www.londonstockexchange.com/products-and-services/millennium-exchange/millennium-exchange-migration/mit303-issue93final.pdf
[MBT Quote API]:     http://www.mbtrading.com/developersMain.aspx?page=api
[NASDAQ Nordic Equity TotalView-ITCH 1.86]: http://nordic.nasdaqomxtrader.com/digitalAssets/82/82004_nordicequitytotalview-itch1.86.pdf
[NASDAQ ITCH 4.0]:   http://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/tvitch-v4.pdf
[NASDAQ ITCH 4.1]:   http://nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTV-ITCH-V4_1.pdf
[NASDAQ OUCH 4.2]:   http://www.nasdaqtrader.com/content/technicalsupport/specifications/TradingProducts/OUCH4.2.pdf
[NASDAQ SoupBinTCP]: http://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/soupbintcp.pdf
[NYSE Arca XDP]:     http://www.nyxdata.com/nysedata/Default.aspx?tabid=1084
[NYSE Daily TAQ]:    http://www.nyxdata.com/data-products/daily-taq

## Documentation

* [Quick Start Guide](docs/quickstart.md)

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

## License

Copyright (C) 2011-2013 Pekka Enberg and contributors

libtrading is available for use under the 3-clause (or "modified") BSD license.
See the file [LICENSE][] for details.

[LICENSE]:	https://github.com/penberg/libtrading/blob/master/LICENSE

## Contributors

* Marat Stanichenko
* Jussi Virtanen
