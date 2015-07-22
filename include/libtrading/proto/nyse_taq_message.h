#ifndef LIBTRADING_NYSE_TAQ_MESSAGE_H
#define LIBTRADING_NYSE_TAQ_MESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libtrading/types.h"

#include <stddef.h>

struct buffer;

struct nyse_taq_msg_daily_quote {
	char			Time[9];
	char			Exchange;
	char			Symbol[16];
	char			BidPrice[11];
	char			BidSize[7];
	char			AskPrice[11];
	char			AskSize[7];
	char			QuoteCondition;
	char			MarketMaker[4];
	char			BidExchange;
	char			AskExchange;
	char			SequenceNumber[16];
	char			NationalBBOInd;
	char			NASDAQBBOInd;
	char			QuoteCancelCorrection;
	char			SourceOfQuote;
	char			RetailInterestIndicator;
	char			ShortSaleRestrictionIndicator;
	char			LULDBBOIndicatorCQS;
	char			LULDBBOIndicatorUTP;
	char			FINRAADFMPIDIndicator;
	char			LineChange[2];
} __attribute__((packed));

struct nyse_taq_msg_daily_trade {
	char			Time[9];
	char			Exchange;
	char			Symbol[16];
	char			SaleCondition[4];
	char			TradeVolume[9];
	char			TradePrice[11];
	char			TradeStopStockIndicator;
	char			TradeCorrectionIndicator[2];
	char			TradeSequenceNumber[16];
	char			SourceOfTrade;
	char			TradeReportingFacility;
	char			LineChange[2];
} __attribute__((packed));

struct nyse_taq_msg_daily_nbbo {
	char			Time[9];
	char			Exchange;
	char			Symbol[16];
	char			BidPrice[11];
	char			BidSize[7];
	char			AskPrice[11];
	char			AskSize[7];
	char			QuoteCondition;
	char			MarketMaker[4];
	char			BidExchange;
	char			AskExchange;
	char			SequenceNumber[16];
	char			NationalBBOInd;
	char			NASDAQBBOInd;
	char			QuoteCancelCorrection;
	char			SourceOfQuote;
	char			NBBOQuoteCondition;
	char			BestBidExchange;
	char			BestBidPrice[11];
	char			BestBidSize[7];
	char			BestBidMarketMaker[4];
	char			BestBidMMLocation[2];
	char			BestBidMMDeskLocation;
	char			BestAskExchange;
	char			BestAskPrice[11];
	char			BestAskSize[7];
	char			BestAskMarketMaker[4];
	char			BestAskMMLocation[2];
	char			BestAskMMDeskLocation;
	char			LULDNBBOIndicatorCQS;
	char			LULDNBBOIndicatorUTP;
	char			LineChange[2];
} __attribute__((packed));

void *nyse_taq_msg_decode(struct buffer *buf, size_t size);

static inline struct nyse_taq_msg_daily_quote *nyse_taq_msg_daily_quote_decode(struct buffer *buf)
{
	return nyse_taq_msg_decode(buf, sizeof(struct nyse_taq_msg_daily_quote));
}

static inline struct nyse_taq_msg_daily_trade *nyse_taq_msg_daily_trade_decode(struct buffer *buf)
{
	return nyse_taq_msg_decode(buf, sizeof(struct nyse_taq_msg_daily_trade));
}

static inline struct nyse_taq_msg_daily_nbbo *nyse_taq_msg_daily_nbbo_decode(struct buffer *buf)
{
	return nyse_taq_msg_decode(buf, sizeof(struct nyse_taq_msg_daily_nbbo));
}

#ifdef __cplusplus
}
#endif

#endif
