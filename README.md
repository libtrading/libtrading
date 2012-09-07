# libtrading [![Build Status](https://secure.travis-ci.org/penberg/libtrading.png)](http://travis-ci.org/penberg/libtrading)

libtrading is a library for electronic trading. Its purpose is to support
market data and order entry network protocols used by trading venues across the
world.

![alt text](https://github.com/penberg/libtrading/raw/master/htdocs/ticker-tape.jpg "Ticker Tape")

## Supported Protocols

 * [BOE](http://www.batstrading.co.uk/resources/participant_resources/BATS_Europe_Binary_Order_Entry_Specification.pdf) ([header](https://github.com/penberg/libtrading/blob/master/include/trading/boe_message.h))

 * [FIX](http://fixprotocol.org/specifications/) ([header](https://github.com/penberg/libtrading/blob/master/include/trading/fix_message.h))

 * [ITCH 4.1](http://nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTV-ITCH-V4_1.pdf) ([header](https://github.com/penberg/libtrading/blob/master/include/trading/itch41_message.h))

 * [MBT Quote API](http://www.mbtrading.com/developersMain.aspx?page=api) ([header](https://github.com/penberg/libtrading/blob/master/include/trading/mbt_quote_message.h))

 * [OUCH 4.2](http://www.nasdaqtrader.com/content/technicalsupport/specifications/TradingProducts/OUCH4.2.pdf) ([header](https://github.com/penberg/libtrading/blob/master/include/trading/ouch42_message.h))

 * [PITCH](http://www.batstrading.com/resources/membership/BATS_PITCH_Specification.pdf) ([header](https://github.com/penberg/libtrading/blob/master/include/trading/pitch_message.h))

 * [SoupBinTCP](http://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/soupbintcp.pdf) ([header](https://github.com/penberg/libtrading/blob/master/include/trading/soupbin3_session.h))

## Build Instructions

To build via Makefile simply execute:

    make

## License

Copyright (C) 2011-2012 Pekka Enberg

libtrading is available for use under the 3-clause (or "modified") BSD license.
See the file [LICENSE](https://github.com/penberg/libtrading/blob/master/LICENSE)
for details.
