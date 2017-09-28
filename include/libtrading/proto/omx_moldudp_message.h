#ifndef LIBTRADING_OMX_MOLDUDP_MESSAGE_H
#define LIBTRADING_OMX_MOLDUDP_MESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libtrading/types.h"

struct omx_moldudp_header {
	char			Session[10];
	le32			SequenceNumber;
	le16			MessageCount;
} __attribute__((packed));

struct omx_moldudp_message {
	le16			MessageLength;
} __attribute__((packed));

struct omx_moldudp_request {
	char			Session[10];
	le32			SequenceNumber;
	le16			RequestedMessageCount;
} __attribute__((packed));

static inline struct omx_moldudp_message *omx_moldudp_payload(struct omx_moldudp_header *msg)
{
	return (void *) msg + sizeof(struct omx_moldudp_header);
}

static inline void *omx_moldudp_message_payload(struct omx_moldudp_message *msg)
{
	return (void *) msg + sizeof(struct omx_moldudp_message);
}

/*
 * MoldUDP64
 */

struct omx_moldudp64_header {
	char			Session[10];
	le64			SequenceNumber;
	le16			MessageCount;
} __attribute__((packed));

struct omx_moldudp64_message {
	le16			MessageLength;
} __attribute__((packed));

struct omx_moldudp64_request {
	char			Session[10];
	le64			SequenceNumber;
	le16			RequestedMessageCount;
} __attribute__((packed));

static inline struct omx_moldudp64_message *omx_moldudp64_payload(struct omx_moldudp64_header *msg)
{
	return (void *) msg + sizeof(struct omx_moldudp64_header);
}

static inline void *omx_moldudp64_message_payload(struct omx_moldudp64_message *msg)
{
	return (void *) msg + sizeof(struct omx_moldudp64_message);
}


#ifdef __cplusplus
}
#endif

#endif
