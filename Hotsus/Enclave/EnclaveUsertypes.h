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

#endif
