#include <set>
#include "EnclaveBasic.h"

Hash_t prepareHash_Hotsus_t = initiateHash_t();	  // Hash of the last prepared block
View prepareView_Hotsus_t = 0;					  // View of [prepareHash_Hotsus_t]
Hash_t exprepareHash_Hotsus_t = initiateHash_t(); // Hash of the last prepared block
View exprepareView_Hotsus_t = 0;				  // Copy of [exprepareView_Hotsus_t]
View view_Hotsus_t = 0;							  // Current view
Phase phase_Hotsus_t = PHASE_EXNEWVIEW;			  // Current phase
bool switcher = false;
bool authenticator = false;

void increment_Hotsus_t()
{
	if (phase_Hotsus_t == PHASE_NEWVIEW)
	{
		phase_Hotsus_t = PHASE_PREPARE;
	}
	else if (phase_Hotsus_t == PHASE_PREPARE)
	{
		phase_Hotsus_t = PHASE_PRECOMMIT;
	}
	else if (phase_Hotsus_t == PHASE_PRECOMMIT)
	{
		if (switcher)
		{
			phase_Hotsus_t = PHASE_EXNEWVIEW;
		}
		else
		{
			phase_Hotsus_t = PHASE_NEWVIEW;
			view_Hotsus_t++;
		}
	}
}

void incrementExtra_Hotsus_t()
{
	if (phase_Hotsus_t == PHASE_EXNEWVIEW)
	{
		phase_Hotsus_t = PHASE_EXPREPARE;
	}
	else if (phase_Hotsus_t == PHASE_EXPREPARE)
	{
		phase_Hotsus_t = PHASE_EXPRECOMMIT;
	}
	else if (phase_Hotsus_t == PHASE_EXPRECOMMIT)
	{
		phase_Hotsus_t = PHASE_EXCOMMIT;
	}
	else if (phase_Hotsus_t == PHASE_EXCOMMIT)
	{
		if (authenticator)
		{
			phase_Hotsus_t = PHASE_NEWVIEW;
		}
		else
		{
			phase_Hotsus_t = PHASE_EXNEWVIEW;
		}
		view_Hotsus_t++;
	}
}

Justification_t updateRoundData_Hotsus_t(Hash_t hash1, Hash_t hash2, View view)
{
	RoundData_t roundData_t;
	roundData_t.proposeHash = hash1;
	roundData_t.proposeView = view_Hotsus_t;
	roundData_t.justifyHash = hash2;
	roundData_t.justifyView = view;
	roundData_t.phase = phase_Hotsus_t;
	Sign_t sign_t = signData_t(roundData2string_t(roundData_t));
	Signs_t signs_t;
	signs_t.size = 1;
	signs_t.signs[0] = sign_t;
	Justification_t justification_t;
	justification_t.set = 1;
	justification_t.roundData = roundData_t;
	justification_t.signs = signs_t;
	increment_Hotsus_t();
	return justification_t;
}

Justification_t updateExtraRoundData_Hotsus_t(Hash_t hash1, Hash_t hash2, View view)
{
	RoundData_t roundData_t;
	roundData_t.proposeHash = hash1;
	roundData_t.proposeView = view_Hotsus_t;
	roundData_t.justifyHash = hash2;
	roundData_t.justifyView = view;
	roundData_t.phase = phase_Hotsus_t;
	Sign_t sign_t = signData_t(roundData2string_t(roundData_t));
	Signs_t signs_t;
	signs_t.size = 1;
	signs_t.signs[0] = sign_t;
	Justification_t justification_t;
	justification_t.set = 1;
	justification_t.roundData = roundData_t;
	justification_t.signs = signs_t;
	incrementExtra_Hotsus_t();
	return justification_t;
}

sgx_status_t TEE_verifyJustificationHotsus(Justification_t *justification_t, bool *b)
{
	sgx_status_t status_t = SGX_SUCCESS;

	*b = verifyJustification_t(justification_t);

	return status_t;
}

sgx_status_t TEE_verifyProposalHotsus(Proposal_t *proposal_t, Signs_t *signs_t, bool *b)
{
	sgx_status_t status_t = SGX_SUCCESS;

	*b = verifyProposal_t(proposal_t, signs_t);

	return status_t;
}

sgx_status_t TEE_changeSwitcher()
{
	sgx_status_t status_t = SGX_SUCCESS;

	if (switcher == false)
	{
		switcher = true;
	}
	else
	{
		switcher = false;
	}

	return status_t;
}

sgx_status_t TEE_changeAuthenticator()
{
	sgx_status_t status_t = SGX_SUCCESS;

	if (authenticator == false)
	{
		authenticator = true;
	}
	else
	{
		authenticator = false;
	}

	return status_t;
}

sgx_status_t TEE_verifyExproposalHotsus(Exproposal_t *exproposal_t, Signs_t *signs_t, bool *b)
{
	sgx_status_t status_t = SGX_SUCCESS;

	*b = verifyExproposal_t(exproposal_t, signs_t);

	return status_t;
}

sgx_status_t TEE_initializeMsgNewviewHotsus(Justification_t *justification_MsgNewview_t)
{
	sgx_status_t status_t = SGX_SUCCESS;

	Hash_t hash_t = initiateDummyHash_t();
	*justification_MsgNewview_t = updateRoundData_Hotsus_t(hash_t, prepareHash_Hotsus_t, prepareView_Hotsus_t);

	return status_t;
}

sgx_status_t TEE_initializeAccumulatorHotsus(Justifications_t *justifications_MsgNewview_t, Accumulator_t *accumulator_MsgLdrprepare_t)
{
	sgx_status_t status_t = SGX_SUCCESS;

	View proposeView_MsgNewview = justifications_MsgNewview_t->justifications[0].roundData.proposeView;
	View highView = 0;
	Hash_t highHash_t = initiateHash_t();
	std::set<ReplicaID> signers;

	for (int i = 0; i < MAX_NUM_SIGNATURES; i++)
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

sgx_status_t TEE_initializeMsgLdrprepareHotsus(Proposal_t *proposal_MsgLdrprepare_t, Signs_t *signs_MsgLdrprepare_t)
{
	sgx_status_t status_t = SGX_SUCCESS;

	Sign_t sign_MsgLdrprepare_t = signData_t(proposal2string_t(*proposal_MsgLdrprepare_t));
	signs_MsgLdrprepare_t->size = 1;
	signs_MsgLdrprepare_t->signs[0] = sign_MsgLdrprepare_t;

	return status_t;
}

sgx_status_t TEE_respondProposalHotsus(Hash_t *proposeHash_t, Accumulator_t *accumulator_MsgLdrprepare_t, Justification_t *justification_MsgPrepare_t)
{
	sgx_status_t status_t = SGX_SUCCESS;

	View proposeView_MsgLdrprepare = accumulator_MsgLdrprepare_t->proposeView;
	Hash_t prepareHash_MsgLdrprepare_t = accumulator_MsgLdrprepare_t->prepareHash;
	View prepareView_MsgLdrprepare = accumulator_MsgLdrprepare_t->prepareView;
	unsigned int size_MsgLdrprepare = accumulator_MsgLdrprepare_t->size;
	if (view_Hotsus_t == proposeView_MsgLdrprepare && size_MsgLdrprepare == getTrustedQuorumSize_t())
	{
		*justification_MsgPrepare_t = updateRoundData_Hotsus_t(*proposeHash_t, prepareHash_MsgLdrprepare_t, prepareView_MsgLdrprepare);
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

sgx_status_t TEE_saveMsgPrepareHotsus(Justification_t *justification_MsgPrepare_t, Justification_t *justification_msgExprecommit_t)
{
	sgx_status_t status_t = SGX_SUCCESS;

	RoundData_t roundData_MsgPrepare_t = justification_MsgPrepare_t->roundData;
	Hash_t proposeHash_MsgPrepare_t = roundData_MsgPrepare_t.proposeHash;
	View proposeView_MsgPrepare_t = roundData_MsgPrepare_t.proposeView;
	Phase phase_MsgPrepare_t = roundData_MsgPrepare_t.phase;
	if (verifyJustification_t(justification_MsgPrepare_t) && justification_MsgPrepare_t->signs.size == getTrustedQuorumSize_t() && view_Hotsus_t == proposeView_MsgPrepare_t && phase_MsgPrepare_t == PHASE_PREPARE)
	{
		exprepareHash_Hotsus_t = prepareHash_Hotsus_t;
		exprepareView_Hotsus_t = prepareView_Hotsus_t;
		prepareHash_Hotsus_t = proposeHash_MsgPrepare_t;
		prepareView_Hotsus_t = proposeView_MsgPrepare_t;
		*justification_msgExprecommit_t = updateRoundData_Hotsus_t(proposeHash_MsgPrepare_t, initiateHash_t(), 0);
	}
	else
	{
		justification_msgExprecommit_t->set = false;
		if (DEBUG_TEE)
		{
			TEE_Print((printReplicaId_t() + " fail to store in MsgPrepare").c_str());
		}
	}

	return status_t;
}

sgx_status_t TEE_initializeMsgExnewviewHotsus(Justification_t *justification_MsgExnewview_t)
{
	sgx_status_t status_t = SGX_SUCCESS;

	Hash_t hash_t = initiateDummyHash_t();
	*justification_MsgExnewview_t = updateExtraRoundData_Hotsus_t(hash_t, exprepareHash_Hotsus_t, exprepareView_Hotsus_t);

	return status_t;
}

sgx_status_t TEE_respondExproposalHotsus(Hash_t *proposeHash_t, Justification_t *justification_MsgExnewview_t, Justification_t *justification_MsgExprepare_t)
{
	sgx_status_t status_t = SGX_SUCCESS;

	RoundData_t roundData_MsgNewview_t = justification_MsgExnewview_t->roundData;
	Signs_t signs_MsgNewview_t = justification_MsgExnewview_t->signs;
	View proposeView_MsgNewview = roundData_MsgNewview_t.proposeView;
	Hash_t justifyHash_MsgNewview_t = roundData_MsgNewview_t.justifyHash;
	View justifyView_MsgNewview = roundData_MsgNewview_t.justifyView;
	Phase phase_MsgNewview = roundData_MsgNewview_t.phase;
	if (verify_t(signs_MsgNewview_t, roundData2string_t(roundData_MsgNewview_t)) && view_Hotsus_t == proposeView_MsgNewview && phase_MsgNewview == PHASE_EXNEWVIEW)
	{
		*justification_MsgExprepare_t = updateExtraRoundData_Hotsus_t(*proposeHash_t, justifyHash_MsgNewview_t, justifyView_MsgNewview);
	}
	else
	{
		justification_MsgExprepare_t->set = false;
		if (DEBUG_TEE)
		{
			TEE_Print((printReplicaId_t() + " fail to respond exproposal").c_str());
		}
	}

	return status_t;
}

sgx_status_t TEE_initializeMsgExldrprepareHotsus(Exproposal_t *proposal_MsgExldrprepare_t, Signs_t *signs_MsgExldrprepare_t)
{
	sgx_status_t status_t = SGX_SUCCESS;

	Sign_t sign_MsgExldrprepare_t = signData_t(exproposal2string_t(*proposal_MsgExldrprepare_t));
	signs_MsgExldrprepare_t->size = 1;
	signs_MsgExldrprepare_t->signs[0] = sign_MsgExldrprepare_t;

	return status_t;
}

sgx_status_t TEE_saveMsgExprepareHotsus(Justification_t *justification_MsgExprepare_t, Justification_t *justification_MsgExprecommit_t)
{
	sgx_status_t status_t = SGX_SUCCESS;

	RoundData_t roundData_MsgExprepare_t = justification_MsgExprepare_t->roundData;
	Hash_t proposeHash_MsgExprepare_t = roundData_MsgExprepare_t.proposeHash;
	View proposeView_MsgExprepare_t = roundData_MsgExprepare_t.proposeView;
	Phase phase_MsgExprepare_t = roundData_MsgExprepare_t.phase;
	if (verifyJustification_t(justification_MsgExprepare_t) && justification_MsgExprepare_t->signs.size == getGeneralQuorumSize_t() && view_Hotsus_t == proposeView_MsgExprepare_t && phase_MsgExprepare_t == PHASE_EXPREPARE)
	{
		prepareHash_Hotsus_t = proposeHash_MsgExprepare_t;
		prepareView_Hotsus_t = proposeView_MsgExprepare_t;
		*justification_MsgExprecommit_t = updateExtraRoundData_Hotsus_t(proposeHash_MsgExprepare_t, initiateHash_t(), 0);
	}
	else
	{
		justification_MsgExprecommit_t->set = false;
		if (DEBUG_TEE)
		{
			TEE_Print((printReplicaId_t() + " fail to store in MsgExprepare").c_str());
		}
	}

	return status_t;
}

sgx_status_t TEE_lockMsgExprecommitHotsus(Justification_t *justification_MsgExprecommit_t, Justification_t *justification_MsgExcommit_t)
{
	sgx_status_t status_t = SGX_SUCCESS;

	RoundData_t roundData_MsgExprecommit_t = justification_MsgExprecommit_t->roundData;
	Hash_t proposeHash_MsgExprecommit_t = roundData_MsgExprecommit_t.proposeHash;
	View proposeView_MsgExprecommit_t = roundData_MsgExprecommit_t.proposeView;
	Phase phase_MsgExprecommit_t = roundData_MsgExprecommit_t.phase;
	if (verifyJustification_t(justification_MsgExprecommit_t) && justification_MsgExprecommit_t->signs.size == getGeneralQuorumSize_t() && view_Hotsus_t == proposeView_MsgExprecommit_t && phase_MsgExprecommit_t == PHASE_EXPRECOMMIT)
	{
		prepareHash_Hotsus_t = proposeHash_MsgExprecommit_t;
		prepareView_Hotsus_t = proposeView_MsgExprecommit_t;
		exprepareHash_Hotsus_t = proposeHash_MsgExprecommit_t;
		exprepareView_Hotsus_t = proposeView_MsgExprecommit_t;
		*justification_MsgExcommit_t = updateExtraRoundData_Hotsus_t(proposeHash_MsgExprecommit_t, initiateHash_t(), 0);
	}
	else
	{
		justification_MsgExcommit_t->set = false;
		if (DEBUG_TEE)
		{
			TEE_Print((printReplicaId_t() + " fail to lock in MsgExprecommit").c_str());
		}
	}

	return status_t;
}