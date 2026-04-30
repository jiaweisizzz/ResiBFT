#include <set>
#include "EnclaveBasic.h"

Hash_t prepareHash_Damysus_t = initiateHash_t(); // Hash of the last prepared block
View prepareView_Damysus_t = 0;					 // View of [prepareHash_Damysus_t]
View view_Damysus_t = 0;						 // Current view
Phase phase_Damysus_t = PHASE_NEWVIEW;			 // Current phase

void increment_Damysus_t()
{
	if (phase_Damysus_t == PHASE_NEWVIEW)
	{
		phase_Damysus_t = PHASE_PREPARE;
	}
	else if (phase_Damysus_t == PHASE_PREPARE)
	{
		phase_Damysus_t = PHASE_PRECOMMIT;
	}
	else if (phase_Damysus_t == PHASE_PRECOMMIT)
	{
		phase_Damysus_t = PHASE_NEWVIEW;
		view_Damysus_t++;
	}
}

Justification_t updateRoundData_Damysus_t(Hash_t hash1, Hash_t hash2, View view)
{
	RoundData_t roundData_t;
	roundData_t.proposeHash = hash1;
	roundData_t.proposeView = view_Damysus_t;
	roundData_t.justifyHash = hash2;
	roundData_t.justifyView = view;
	roundData_t.phase = phase_Damysus_t;
	Sign_t sign_t = signData_t(roundData2string_t(roundData_t));
	Signs_t signs_t;
	signs_t.size = 1;
	signs_t.signs[0] = sign_t;
	Justification_t justification_t;
	justification_t.set = 1;
	justification_t.roundData = roundData_t;
	justification_t.signs = signs_t;
	increment_Damysus_t();
	return justification_t;
}

sgx_status_t TEE_verifyJustificationDamysus(Justification_t *justification_t, bool *b)
{
	sgx_status_t status_t = SGX_SUCCESS;

	*b = verifyJustification_t(justification_t);

	return status_t;
}

sgx_status_t TEE_verifyProposalDamysus(Proposal_t *proposal_t, Signs_t *signs_t, bool *b)
{
	sgx_status_t status_t = SGX_SUCCESS;

	*b = verifyProposal_t(proposal_t, signs_t);

	return status_t;
}

sgx_status_t TEE_initializeMsgNewviewDamysus(Justification_t *justification_MsgNewview_t)
{
	sgx_status_t status_t = SGX_SUCCESS;

	Hash_t hash_t = initiateDummyHash_t();
	*justification_MsgNewview_t = updateRoundData_Damysus_t(hash_t, prepareHash_Damysus_t, prepareView_Damysus_t);

	return status_t;
}

sgx_status_t TEE_initializeAccumulatorDamysus(Justifications_t *justifications_MsgNewview_t, Accumulator_t *accumulator_MsgLdrprepare_t)
{
	sgx_status_t status_t = SGX_SUCCESS;

	View proposeView_MsgNewview = justifications_MsgNewview_t->justifications[0].roundData.proposeView;
	View highView = 0;
	Hash_t highHash_t = initiateHash_t();
	std::set<ReplicaID> signers;

	for (int i = 0; i < MAX_NUM_SIGNATURES && i < getTrustedQuorumSize_t(); i++)
	{
		Justification_t justification_MsgNewview_t = justifications_MsgNewview_t->justifications[i];
		RoundData_t roundData_MsgNewview_t = justification_MsgNewview_t.roundData;
		View justifyView_MsgNewview = roundData_MsgNewview_t.justifyView;
		Hash_t justifyHash_MsgNewview_t = roundData_MsgNewview_t.justifyHash;
		Signs_t signs_MsgNewview_t = justification_MsgNewview_t.signs;
		ReplicaID signer = signs_MsgNewview_t.signs[0].signer;
		if (verify_t(signs_MsgNewview_t, roundData2string_t(roundData_MsgNewview_t)) && roundData_MsgNewview_t.proposeView == proposeView_MsgNewview && roundData_MsgNewview_t.phase == PHASE_NEWVIEW)
		{
			if (signers.find(signer) == signers.end())
			{
				signers.insert(signer);
				if (justifyView_MsgNewview >= highView)
				{
					highView = justifyView_MsgNewview;
					highHash_t = justifyHash_MsgNewview_t;
				}
			}
		}
	}

	accumulator_MsgLdrprepare_t->set = true;
	accumulator_MsgLdrprepare_t->proposeView = proposeView_MsgNewview;
	accumulator_MsgLdrprepare_t->prepareHash = highHash_t;
	accumulator_MsgLdrprepare_t->prepareView = highView;
	accumulator_MsgLdrprepare_t->size = signers.size();

	return status_t;
}

sgx_status_t TEE_respondProposalDamysus(Hash_t *proposeHash_t, Accumulator_t *accumulator_MsgLdrprepare_t, Justification_t *justification_MsgPrepare_t)
{
	sgx_status_t status_t = SGX_SUCCESS;

	if (view_Damysus_t == accumulator_MsgLdrprepare_t->proposeView && accumulator_MsgLdrprepare_t->size == getTrustedQuorumSize_t())
	{
		*justification_MsgPrepare_t = updateRoundData_Damysus_t(*proposeHash_t, accumulator_MsgLdrprepare_t->prepareHash, accumulator_MsgLdrprepare_t->prepareView);
	}
	else
	{
		justification_MsgPrepare_t->set = false;
		if (DEBUG_TEE)
		{
			TEE_Print((printReplicaId_t() + " fail to respond accumulator").c_str());
		}
	}

	return status_t;
}

sgx_status_t TEE_initializeMsgLdrprepareDamysus(Proposal_t *proposal_MsgLdrprepare_t, Signs_t *signs_MsgLdrprepare_t)
{
	sgx_status_t status_t = SGX_SUCCESS;

	Sign_t sign_MsgLdrprepare_t = signData_t(proposal2string_t(*proposal_MsgLdrprepare_t));
	signs_MsgLdrprepare_t->size = 1;
	signs_MsgLdrprepare_t->signs[0] = sign_MsgLdrprepare_t;

	return status_t;
}

sgx_status_t TEE_saveMsgPrepareDamysus(Justification_t *justification_MsgPrepare_t, Justification_t *justification_MsgPrecommit_t)
{
	sgx_status_t status_t = SGX_SUCCESS;

	RoundData_t roundData_MsgPrepare_t = justification_MsgPrepare_t->roundData;
	Hash_t proposeHash_MsgPrepare_t = roundData_MsgPrepare_t.proposeHash;
	View proposeView_MsgPrepare_t = roundData_MsgPrepare_t.proposeView;
	Phase phase_MsgPrepare_t = roundData_MsgPrepare_t.phase;
	if (justification_MsgPrepare_t->signs.size == getTrustedQuorumSize_t() && verifyJustification_t(justification_MsgPrepare_t) && view_Damysus_t == proposeView_MsgPrepare_t && phase_MsgPrepare_t == PHASE_PREPARE)
	{
		prepareHash_Damysus_t = proposeHash_MsgPrepare_t;
		prepareView_Damysus_t = proposeView_MsgPrepare_t;
		*justification_MsgPrecommit_t = updateRoundData_Damysus_t(proposeHash_MsgPrepare_t, initiateHash_t(), 0);
	}
	else
	{
		justification_MsgPrecommit_t->set = false;
		if (DEBUG_TEE)
		{
			TEE_Print((printReplicaId_t() + " fail to store in MsgPrepare").c_str());
		}
	}

	return status_t;
}
