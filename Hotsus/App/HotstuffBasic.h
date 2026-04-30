#ifndef HOTSTUFFBASIC_H
#define HOTSTUFFBASIC_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include "Hash.h"
#include "Proposal.h"
#include "Justification.h"

class HotstuffBasic
{
private:
	Hash lockHash;					// Hash of the last locked block
	View lockView;					// View of [lockHash]
	Hash prepareHash;				// Hash of the last prepared block
	View prepareView;				// View of [prepareHash]
	View view;						// Current view
	Phase phase;					// Current phase
	ReplicaID replicaId;			// Unique identifier
	Key privateKey;					// Private key
	unsigned int generalQuorumSize; // Quorum size

	void increment();
	Sign signText(std::string text);
	Justification updateRoundData(Hash hash1, Hash hash2, View view);
	bool verifySigns(Signs signs, ReplicaID replicaId, Nodes nodes, std::string text);

public:
	HotstuffBasic();
	HotstuffBasic(ReplicaID replicaId, Key privateKey, unsigned int generalQuorumSize);

	bool verifyJustification(Nodes nodes, Justification justification);
	bool verifyProposal(Nodes nodes, Proposal<Justification> proposal, Signs signs);

	Justification initializeMsgNewview();
	Justification respondProposal(Nodes nodes, Hash proposeHash, Justification justification_MsgNewview);
	Signs initializeMsgLdrprepare(Proposal<Justification> proposal_MsgLdrprepare);
	Justification saveMsgPrepare(Nodes nodes, Justification justification_MsgPrepare);
	Justification lockMsgPrecommit(Nodes nodes, Justification justification_MsgPrecommit);
};

#endif
