#ifndef LIBTRADING_BATS_PITCH_MESSAGE_H
#define LIBTRADING_BATS_PITCH_MESSAGE_H

#include "libtrading/types.h"

struct buffer;

/*
 * Message types:
 */
enum pitch_msg_type {
	PITCH_MSG_SYMBOL_CLEAR			= 's',
	PITCH_MSG_ADD_ORDER_SHORT		= 'A',
	PITCH_MSG_ADD_ORDER_LONG		= 'd',
	PITCH_MSG_ORDER_EXECUTED		= 'E',
	PITCH_MSG_ORDER_CANCEL			= 'X',
	PITCH_MSG_TRADE_SHORT			= 'P',
	PITCH_MSG_TRADE_LONG			= 'r',
	PITCH_MSG_TRADE_BREAK			= 'B',
	PITCH_MSG_TRADING_STATUS		= 'H',
	PITCH_MSG_AUCTION_UPDATE		= 'I',
	PITCH_MSG_AUCTION_SUMMARY		= 'J',
};

struct pitch_message {
	char			Timestamp[8];
	u8			MessageType;
} packed;

struct pitch_msg_symbol_clear {
	char			Timestamp[8];
	u8			MessageType;
	char			StockSymbol[8];
};

struct pitch_msg_add_order_short {
	char			Timestamp[8];
	u8			MessageType;
	char			OrderID[12];
	char			SideIndicator;
	char			Shares[6];
	char			StockSymbol[6];
	char			Price[10];
	char			Display;
} packed;

struct pitch_msg_add_order_long {
	char			Timestamp[8];
	u8			MessageType;
	char			OrderID[12];
	char			SideIndicator;
	char			Shares[6];
	char			StockSymbol[8];
	char			Price[10];
	char			Display;
	char			ParticipantID[4];
} packed;

struct pitch_msg_order_executed {
	char			Timestamp[8];
	u8			MessageType;
	char			OrderID[12];
	char			ExecutedShares[6];
	char			ExecutionID[12];
} packed;

struct pitch_msg_order_cancel {
	char			Timestamp[8];
	u8			MessageType;
	char			OrderID[12];
	char			CanceledShares[6];
} packed;

struct pitch_msg_trade_short {
	char			Timestamp[8];
	u8			MessageType;
	char			OrderID[12];
	char			SideIndicator;
	char			Shares[6];
	char			StockSymbol[6];
	char			Price[10];
	char			ExecutionID[12];
} packed;

struct pitch_msg_trade_long {
	char			Timestamp[8];
	u8			MessageType;
	char			OrderID[12];
	char			SideIndicator;
	char			Shares[6];
	char			StockSymbol[8];
	char			Price[10];
	char			ExecutionID[12];
} packed;

struct pitch_msg_trade_break {
	char			Timestamp[8];
	u8			MessageType;
	char			ExecutionID[12];
} packed;

struct pitch_msg_trading_status {
	char			Timestamp[8];
	u8			MessageType;
	char			StockSymbol[8];
	char			HaltStatus;
	char			RegSHOAction;
	char			Reserved1;
	char			Reserved2;
} packed;

struct pitch_msg_auction_update {
	char			Timestamp[8];
	u8			MessageType;
	char			StockSymbol[8];
	char			AuctionType;
	char			ReferencePrice[10];
	char			BuyShares[10];
	char			SellShares[10];
	char			IndicativePrice[10];
	char			AuctionOnlyPrice[10];
} packed;

struct pitch_msg_auction_summary {
	char			Timestamp[8];
	u8			MessageType;
	char			StockSymbol[8];
	char			AuctionType;
	char			Price[10];
	char			Shares[10];
} packed;

struct pitch_message *pitch_message_decode(struct buffer *buf);

#endif
