#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef uint8_t HEADER;

#define HEADER_TRANSACTION 0x0
#define HEADER_START 0x1
#define HEADER_REPLY 0x2

#define HEADER_NEWVIEW_HOTSTUFF 0x3
#define HEADER_LDRPREPARE_HOTSTUFF 0x4
#define HEADER_PREPARE_HOTSTUFF 0x5
#define HEADER_PRECOMMIT_HOTSTUFF 0x6
#define HEADER_COMMIT_HOTSTUFF 0x7

#define HEADER_NEWVIEW_DAMYSUS 0x8
#define HEADER_LDRPREPARE_DAMYSUS 0x9
#define HEADER_PREPARE_DAMYSUS 0x10
#define HEADER_PRECOMMIT_DAMYSUS 0x11

#define HEADER_NEWVIEW_HOTSUS 0x12
#define HEADER_LDRPREPARE_HOTSUS 0x13
#define HEADER_PREPARE_HOTSUS 0x14
#define HEADER_PRECOMMIT_HOTSUS 0x15
#define HEADER_EXNEWVIEW_HOTSUS 0x16
#define HEADER_EXLDRPREPARE_HOTSUS 0x17
#define HEADER_EXPREPARE_HOTSUS 0x18
#define HEADER_EXPRECOMMIT_HOTSUS 0x19
#define HEADER_EXCOMMIT_HOTSUS 0x20

#define PHASE_NEWVIEW 0x0
#define PHASE_PREPARE 0x1
#define PHASE_PRECOMMIT 0x2
#define PHASE_COMMIT 0x3
#define PHASE_EXNEWVIEW 0x4
#define PHASE_EXPREPARE 0x5
#define PHASE_EXPRECOMMIT 0x6
#define PHASE_EXCOMMIT 0x7

#define PROTOCOL_HOTSTUFF 0x0
#define PROTOCOL_DAMYSUS 0x1

typedef uint8_t Phase;
typedef uint8_t Protocol;
typedef unsigned int ReplicaID;		// process ids
typedef unsigned int ClientID;		// client ids
typedef unsigned int TransactionID; // transaction ids
typedef unsigned int PortID;
typedef unsigned int View;

#endif
