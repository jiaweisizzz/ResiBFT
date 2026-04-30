#ifndef HOTSUSBASIC_H
#define HOTSUSBASIC_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include "Hash.h"
#include "Proposal.h"
#include "Justification.h"

class HotsusBasic
{
private:
	Hash prepareHash;				// Hash of the last prepared block
	View prepareView;				// View of [prepareHash]
	Hash exprepareHash;				// Hash of the last prepared block
	View exprepareView;				// View of [exprepareHash]
	View view;						// Current view
	Phase phase;					// Current phase
	ReplicaID replicaId;			// Unique identifier
	Key privateKey;					// Private key
	unsigned int generalQuorumSize; // General quorum size
	unsigned int trustedQuorumSize; // Trusted quorum size
	bool switcher;
	bool authenticator;

	void increment();
	void incrementExtra();
	Sign signText(std::string text);
	Justification updateRoundData(Hash hash1, Hash hash2, View view);
	Justification updateExtraRoundData(Hash hash1, Hash hash2, View view);
	bool verifySigns(Signs signs, ReplicaID replicaId, Nodes nodes, std::string text);

public:
	HotsusBasic();
	HotsusBasic(ReplicaID replicaId, Key privateKey, unsigned int generalQuorumSize, unsigned int trustedQuorumSize);

	bool verifyJustification(Nodes nodes, Justification justification);
	bool verifyProposal(Nodes nodes, Proposal<Accumulator> proposal, Signs signs);
	bool verifyExproposal(Nodes nodes, Proposal<Justification> exproposal, Signs signs);

	void changeSwitcher();
	void changeAuthenticator();
	void setTrustedQuorumSize(unsigned int trustedQuorumSize);

	Justification initializeMsgNewview();
	Justification respondProposal(Nodes nodes, Hash proposeHash, Accumulator accumulator_MsgLdrprepare);
	void skipRound();
	Justification initializeMsgExnewview();
	Justification respondExproposal(Nodes nodes, Hash proposeHash, Justification justification_MsgExnewview);
	Signs initializeMsgExldrprepare(Proposal<Justification> proposal_MsgExldrprepare);
	Justification saveMsgExprepare(Nodes nodes, Justification justification_MsgExprepare);
	Justification lockMsgExprecommit(Nodes nodes, Justification justification_MsgExprecommit);
};

#endif
