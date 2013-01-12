# Libtrading

[![Build Status](https://secure.travis-ci.org/penberg/libtrading.png?branch=master)](http://travis-ci.org/penberg/libtrading)

Libtrading is a free and open source C library for trading securities, foreign
exchange and financial derivatives electronically. It implements market data,
order entry, drop copy, and related communications protocols used by exchanges
and alternative trading venues across the world.

The library is designed for high performance and robustness. Although latency
is very important in trading today's markets, achieving it it must not
jeopardise trading reliability.

## Supported Protocols

 * [BATS BOE][] ([header](https://github.com/penberg/libtrading/blob/master/include/libtrading/proto/boe_message.h))

 * [BATS PITCH][] ([header](https://github.com/penberg/libtrading/blob/master/include/libtrading/proto/pitch_message.h))

 * [FAST][] ([header](https://github.com/penberg/libtrading/blob/master/include/libtrading/proto/fast_message.h))

 * [FIX][] ([header](https://github.com/penberg/libtrading/blob/master/include/libtrading/proto/fix_message.h))

 * [MBT Quote API][] ([header](https://github.com/penberg/libtrading/blob/master/include/libtrading/proto/mbt_quote_message.h))

 * [NASDAQ ITCH 4.0][] ([header](https://github.com/penberg/libtrading/blob/master/include/libtrading/proto/itch40_message.h))

 * [NASDAQ ITCH 4.1][] ([header](https://github.com/penberg/libtrading/blob/master/include/libtrading/proto/itch41_message.h))

 * [NASDAQ OUCH 4.2][] ([header](https://github.com/penberg/libtrading/blob/master/include/libtrading/proto/ouch42_message.h))

 * [NASDAQ SoupBinTCP][] ([header](https://github.com/penberg/libtrading/blob/master/include/libtrading/proto/soupbin3_session.h))

 * [NYSE Arca XDP][] ([header](https://github.com/penberg/libtrading/blob/master/include/libtrading/proto/xdp_message.h))

[BATS BOE]:          http://www.batstrading.co.uk/resources/participant_resources/BATS_Europe_Binary_Order_Entry_Specification.pdf
[BATS PITCH]:        http://www.batstrading.com/resources/membership/BATS_PITCH_Specification.pdf
[FIX]:               http://fixprotocol.org/specifications/
[FAST]:              http://fixprotocol.org/fastspec/
[MBT Quote API]:     http://www.mbtrading.com/developersMain.aspx?page=api
[NASDAQ ITCH 4.0]:   http://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/tvitch-v4.pdf
[NASDAQ ITCH 4.1]:   http://nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTV-ITCH-V4_1.pdf
[NASDAQ OUCH 4.2]:   http://www.nasdaqtrader.com/content/technicalsupport/specifications/TradingProducts/OUCH4.2.pdf
[NASDAQ SoupBinTCP]: http://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/soupbintcp.pdf
[NYSE Arca XDP]:     http://www.nyxdata.com/nysedata/Default.aspx?tabid=1084

## Build Instructions

To build via Makefile simply execute:

    make

To run the test harness:

    make check

## License

Copyright (C) 2011-2013 Pekka Enberg and contributors

libtrading is available for use under the 3-clause (or "modified") BSD license.
See the file [LICENSE][] for details.

[LICENSE]:	https://github.com/penberg/libtrading/blob/master/LICENSE

## Contributors

* Marat Stanichenko
* Jussi Virtanen
