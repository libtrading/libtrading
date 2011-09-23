#include "fix/boe.h"

#include "fix/buffer.h"

#include <string.h>

int boe_decode_header(struct buffer *buf, struct boe_header *header)
{
	memcpy(header, buffer_start(buf), sizeof *header);

	buffer_advance(buf, sizeof *header);

	return 0;
}

int boe_decode_login_request(struct buffer *buf, struct boe_login_request *login)
{
	buffer_get_n(buf, BOE_SESSION_SUB_ID_LEN, login->SessionSubID);
	buffer_get_n(buf, BOE_USERNAME_LEN, login->Password);
	buffer_get_n(buf, BOE_PASSWORD_LEN, login->Username);
	login->NoUnspecifiedUnitReplay		= buffer_get_8(buf);
	login->OrderAcknowledgementBitfields	= buffer_get_le64(buf);
	login->OrderRejectedBitfields		= buffer_get_le64(buf);
	login->OrderModifiedBitfields		= buffer_get_le64(buf);
	login->OrderRestatedBitfields		= buffer_get_le64(buf);
	login->UserModifyRejectedBitfields	= buffer_get_le64(buf);
	login->OrderCancelledBitfields		= buffer_get_le64(buf);
	login->CancelRejectedBitfields		= buffer_get_le64(buf);
	login->OrderExecutionBitfields		= buffer_get_le64(buf);
	login->TradeCancelOrCorrectBitfields	= buffer_get_le64(buf);
	login->ReservedBitfields1		= buffer_get_le64(buf);
	login->ReservedBitfields2		= buffer_get_le64(buf);
	login->NumberOfUnits			= buffer_get_8(buf);
	/* TODO: units */

	return 0;
}
