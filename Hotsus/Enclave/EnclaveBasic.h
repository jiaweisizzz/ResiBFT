#ifndef ENCLAVE_SHARE_H
#define ENCLAVE_SHARE_H

#include <map>
#include <string>
#include <openssl/err.h>
#include <openssl/pem.h>
#include "Enclave_t.h"
#include "../App/config.h"
#include "../App/key.h"
#include "../App/types.h"

unsigned int getGeneralQuorumSize_t();
unsigned int getTrustedQuorumSize_t();
std::string printReplicaId_t();
sgx_status_t initializeVariables_t(ReplicaID *me, Pids_t *others, unsigned int *GeneralQuorumSize, unsigned int *TrustedQuorumSize);
sgx_status_t setTrustedQuorumSize_t(unsigned int *TrustedQuorumSize);

std::string hash2string_t(Hash_t hash_t);
std::string roundData2string_t(RoundData_t roundData_t);
std::string proposal2string_t(Proposal_t proposal_t);
std::string exproposal2string_t(Exproposal_t exproposal_t);

bool equalHashes_t(Hash_t hash1, Hash_t hash2);
Hash_t initiateHash_t();
Hash_t initiateDummyHash_t();

Sign_t signData_t(std::string text);
bool verify_t(Signs_t signs_t, std::string text);
bool verifyJustification_t(Justification_t *justification_t);
bool verifyProposal_t(Proposal_t *proposal_t, Signs_t *signs_t);
bool verifyExproposal_t(Exproposal_t *exproposal_t, Signs_t *signs_t);

#endif
