#ifndef LIBTRADING_SOUPBIN3_SESSION_H
#define LIBTRADING_SOUPBIN3_SESSION_H

#include "libtrading/types.h"

struct buffer;

/*
 * Packet types:
 */
enum soupbin3_packet_type {
	SOUPBIN3_PACKET_DEBUG			= '+', /* Section 2.1 */

	/* Section 2.2. - Packets Sent by the Server */
	SOUPBIN3_PACKET_LOGIN_ACCEPTED		= 'A', /* Section 2.2.1. */
	SOUPBIN3_PACKET_LOGIN_REJECTED		= 'J', /* Section 2.2.2. */
	SOUPBIN3_PACKET_SEQ_DATA		= 'S', /* Section 2.2.3. */
	SOUPBIN3_PACKET_SERVER_HEARTBEAT	= 'H', /* Section 2.2.4. */
	SOUPBIN3_PACKET_END_OF_SESSION		= 'Z', /* Section 2.2.5. */

	/* Section 2.3. - Packets Sent by the Client */
	SOUPBIN3_PACKET_LOGIN_REQUEST		= 'L', /* Section 2.3.1. */
	SOUPBIN3_PACKET_UNSEQ_DATA		= 'U', /* Section 2.3.2. */
	SOUPBIN3_PACKET_CLIENT_HEARTBEAT	= 'R', /* Section 2.3.3. */
	SOUPBIN3_PACKET_LOGOUT_REQUEST		= 'O', /* Section 2.3.4. */
};

struct soupbin3_packet {
	be16			PacketLength;
	char			PacketType;	/* SOUPBIN3_PACKET_<type> */
};

/* SOUPBIN_PACKET_DEBUG */
struct soupbin3_packet_debug {
	be16			PacketLength;
	char			PacketType;
	char			Text[];
} packed;

/* SOUPBIN_PACKET_LOGIN_ACCEPTED */
struct soupbin3_packet_login_accepted {
	be16			PacketLength;
	char			PacketType;
	char			Session[10];
	char			SequenceNumber[20];
} packed;

/* SOUPBIN_PACKET_LOGIN_REJECTED */
struct soupbin3_packet_login_rejected {
	be16			PacketLength;
	char			PacketType;
	char			RejectReasonCode;
} packed;

/* SOUPBIN_PACKET_SEQ_DATA */
struct soupbin3_packet_seq_data {
	be16			PacketLength;
	char			PacketType;
	char			Message[];
} packed;

/* SOUPBIN_PACKET_SERVER_HEARTBEAT */
struct soupbin3_packet_server_heartbeat {
	be16			PacketLength;
	char			PacketType;
} packed;

/* SOUPBIN_PACKET_END_OF_SESSION */
struct soupbin3_packet_end_of_session {
	be16			PacketLength;
	char			PacketType;
} packed;

/* SOUPBIN_PACKET_LOGIN_REQUEST */
struct soupbin3_packet_login_request {
	be16			PacketLength;
	char			PacketType;
	char			Username[6];
	char			Password[10];
	char			RequestedSession[10];
	char			RequestedSequenceNumber[20];
} packed;

/* SOUPBIN_PACKET_UNSEQ_DATA */
struct soupbin3_packet_unseq_data {
	be16			PacketLength;
	char			PacketType;
	char			Message[];
} packed;

/* SOUPBIN_PACKET_CLIENT_HEARTBEAT */
struct soupbin3_packet_client_heartbeat {
	be16			PacketLength;
	char			PacketType;
} packed;

/* SOUPBIN_PACKET_LOGOUT_REQUEST */
struct soupbin3_packet_logout_request {
	be16			PacketLength;
	char			PacketType;
} packed;

struct soupbin3_session {
	int				sockfd;
	struct buffer			*rx_buffer;
};

struct soupbin3_session *soupbin3_session_new(int sockfd);
void soupbin3_session_delete(struct soupbin3_session *session);
int soupbin3_session_recv(struct soupbin3_session *session, struct soupbin3_packet *packet);

#endif
