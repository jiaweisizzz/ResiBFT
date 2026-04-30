#ifndef USER_TYPES_H
#define USER_TYPES_H

#include <stdbool.h>
#include <openssl/sha.h>
#include "../App/types.h"
#include "../App/config.h"
#include "../App/key.h"

typedef struct _Pids_t
{
	unsigned int num_nodes;
	ReplicaID pids[MAX_NUM_NODES];
} Pids_t;

typedef struct _Transaction_t
{
	ClientID clientId;
	TransactionID transactionId;
	unsigned char transactionData[PAYLOAD_SIZE];
} Transaction_t;

typedef struct _Hash_t
{
	bool set;
	unsigned char hash[SHA256_DIGEST_LENGTH];
} Hash_t;

typedef struct _Block_t
{
	bool set;
	Hash_t previousHash;
	unsigned int size;
	Transaction_t transactions[MAX_NUM_TRANSACTIONS];
} Block_t;

typedef struct _RoundData_t
{
	Hash_t proposeHash;
	View proposeView;
	Hash_t justifyHash;
	View justifyView;
	Phase phase;
} RoundData_t;

typedef struct _Sign_t
{
	bool set;
	ReplicaID signer;
	unsigned char signtext[SIGN_LEN];
} Sign_t;

typedef struct _Signs_t
{
	unsigned int size;
	Sign_t signs[MAX_NUM_SIGNATURES];
} Signs_t;

typedef struct _Justification_t
{
	bool set;
	RoundData_t roundData;
	Signs_t signs;
} Justification_t;

typedef struct _Justifications_t
{
	Justification_t justifications[MAX_NUM_SIGNATURES];
} Justifications_t;

typedef struct _Accumulator_t
{
	bool set;
	View proposeView;
	Hash_t prepareHash;
	View prepareView;
	unsigned int size;
} Accumulator_t;

typedef struct _Proposal_t
{
	Accumulator_t accumulator;
	Block_t block;
} Proposal_t;

typedef struct _Exproposal_t
{
	Justification_t justification;
	Block_t block;
} Exproposal_t;

// ResiBFT Types
typedef struct _VCType_t
{
	VCType vcType;
} VCType_t;

typedef struct _VerificationCertificate_t
{
	VCType_t type;
	View view;
	Hash_t blockHash;
	ReplicaID signer;
	Sign_t sign;
} VerificationCertificate_t;

typedef struct _Checkpoint_t
{
	Block_t block;
	View view;
	Justification_t qcPrecommit;
	Signs_t qcVc;
} Checkpoint_t;

typedef struct _BlockCertificate_t
{
	Block_t block;
	View view;
	Justification_t qcPrecommit;
	Signs_t qcVc;
} BlockCertificate_t;

// ============================================
// Lightweight structures for TEE interface
// These avoid the fixed-size arrays that cause parameter size issues
// ============================================

// Single sign for TEE output (instead of Signs_t with 210 slots)
typedef struct _SignSingle_t
{
	bool set;
	ReplicaID signer;
	unsigned char signtext[SIGN_LEN];
} SignSingle_t;

// Lightweight justification for TEE output (single signature)
typedef struct _JustificationLight_t
{
	bool set;
	RoundData_t roundData;
	SignSingle_t sign;
} JustificationLight_t;

// Minimal item for accumulator input - only what's needed to find highest prepare
// Size: 4(signer) + 4(justifyView) + 33(justifyHash) = 41 bytes
typedef struct _AccumulatorInputItem_t
{
	ReplicaID signer;
	View justifyView;
	Hash_t justifyHash;
} AccumulatorInputItem_t;

// Lightweight accumulator input - only proposeView and justify info per signer
// Size: 4(size) + 4(proposeView) + 210*41 = ~8.6KB (much smaller than 3.5MB)
typedef struct _AccumulatorInput_t
{
	unsigned int size;
	View proposeView;
	AccumulatorInputItem_t items[MAX_NUM_SIGNATURES];
} AccumulatorInput_t;

#endif
