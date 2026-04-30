#include "HotstuffBasic.h"

void HotstuffBasic::increment()
{
	if (this->phase == PHASE_NEWVIEW)
	{
		this->phase = PHASE_PREPARE;
	}
	else if (this->phase == PHASE_PREPARE)
	{
		this->phase = PHASE_PRECOMMIT;
	}
	else if (this->phase == PHASE_PRECOMMIT)
	{
		this->phase = PHASE_COMMIT;
	}
	else if (this->phase == PHASE_COMMIT)
	{
		this->phase = PHASE_NEWVIEW;
		this->view++;
	}
}

Sign HotstuffBasic::signText(std::string text)
{
	Sign sign = Sign(this->privateKey, this->replicaId, text);
	return sign;
}

Justification HotstuffBasic::updateRoundData(Hash hash1, Hash hash2, View view)
{
	RoundData roundData = RoundData(hash1, this->view, hash2, view, this->phase);
	Sign sign = this->signText(roundData.toString());
	Justification justification = Justification(roundData, sign);
	this->increment();
	return justification;
}

bool HotstuffBasic::verifySigns(Signs signs, ReplicaID replicaId, Nodes nodes, std::string text)
{
	bool b = signs.verify(replicaId, nodes, text);
	return b;
}

HotstuffBasic::HotstuffBasic()
{
	this->lockHash = Hash(true); // The genesis block
	this->lockView = 0;
	this->prepareHash = Hash(true); // The genesis block
	this->prepareView = 0;
	this->view = 0;
	this->phase = PHASE_NEWVIEW;
	this->generalQuorumSize = 0;
}

HotstuffBasic::HotstuffBasic(ReplicaID replicaId, Key privateKey, unsigned int generalQuorumSize)
{
	this->lockHash = Hash(true); // The genesis block
	this->lockView = 0;
	this->prepareHash = Hash(true); // The genesis block
	this->prepareView = 0;
	this->view = 0;
	this->phase = PHASE_NEWVIEW;
	this->replicaId = replicaId;
	this->privateKey = privateKey;
	this->generalQuorumSize = generalQuorumSize;
}

bool HotstuffBasic::verifyJustification(Nodes nodes, Justification justification)
{
	bool b = this->verifySigns(justification.getSigns(), this->replicaId, nodes, justification.getRoundData().toString());
	return b;
}

bool HotstuffBasic::verifyProposal(Nodes nodes, Proposal<Justification> proposal, Signs signs)
{
	bool b = this->verifySigns(signs, this->replicaId, nodes, proposal.toString());
	return b;
}

Justification HotstuffBasic::initializeMsgNewview()
{
	Justification justification_MsgNewview = this->updateRoundData(Hash(false), this->prepareHash, this->prepareView);
	return justification_MsgNewview;
}

Justification HotstuffBasic::respondProposal(Nodes nodes, Hash proposeHash, Justification justification_MsgNewview)
{
	RoundData roundData_MsgNewview = justification_MsgNewview.getRoundData();
	View proposeView_MsgNewview = roundData_MsgNewview.getProposeView();
	Hash justifyHash_MsgNewview = roundData_MsgNewview.getJustifyHash();
	View justifyView_MsgNewview = roundData_MsgNewview.getJustifyView();
	Phase phase_MsgNewview = roundData_MsgNewview.getPhase();
	if (this->verifyJustification(nodes, justification_MsgNewview) && this->view == proposeView_MsgNewview && phase_MsgNewview == PHASE_NEWVIEW && (this->lockHash == justifyHash_MsgNewview || this->lockView < justifyView_MsgNewview))
	{
		Justification justification_MsgPrepare = this->updateRoundData(proposeHash, justifyHash_MsgNewview, justifyView_MsgNewview);
		return justification_MsgPrepare;
	}
	else
	{
		if (DEBUG_MODULES)
		{
			std::cout << COLOUR_CYAN << this->replicaId << " fail to respond proposal" << COLOUR_NORMAL << std::endl;
		}
		return Justification();
	}
}

Signs HotstuffBasic::initializeMsgLdrprepare(Proposal<Justification> proposal_MsgLdrprepare)
{
	Sign sign_MsgLdrprepare = this->signText(proposal_MsgLdrprepare.toString());
	Signs signs_MsgLdrprepare = Signs(sign_MsgLdrprepare);
	return signs_MsgLdrprepare;
}

Justification HotstuffBasic::saveMsgPrepare(Nodes nodes, Justification justification_MsgPrepare)
{
	RoundData roundData_MsgPrepare = justification_MsgPrepare.getRoundData();
	Hash proposeHash_MsgPrepare = roundData_MsgPrepare.getProposeHash();
	View proposeView_MsgPrepare = roundData_MsgPrepare.getProposeView();
	Phase phase_MsgPrepare = roundData_MsgPrepare.getPhase();
	if (this->verifyJustification(nodes, justification_MsgPrepare) && justification_MsgPrepare.getSigns().getSize() == this->generalQuorumSize && this->view == proposeView_MsgPrepare && phase_MsgPrepare == PHASE_PREPARE)
	{
		this->prepareHash = proposeHash_MsgPrepare;
		this->prepareView = proposeView_MsgPrepare;
		Justification justification_MsgPrecommit = this->updateRoundData(proposeHash_MsgPrepare, Hash(), View());
		return justification_MsgPrecommit;
	}
	else
	{
		if (DEBUG_MODULES)
		{
			std::cout << COLOUR_CYAN << this->replicaId << " fail to store in MsgPrepare" << COLOUR_NORMAL << std::endl;
		}
		return Justification();
	}
}

Justification HotstuffBasic::lockMsgPrecommit(Nodes nodes, Justification justification_MsgPrecommit)
{
	RoundData roundData = justification_MsgPrecommit.getRoundData();
	Hash proposeHash_MsgPrecommit = roundData.getProposeHash();
	View proposeView_MsgPrecommit = roundData.getProposeView();
	Phase phase_MsgPrecommit = roundData.getPhase();
	if (this->verifyJustification(nodes, justification_MsgPrecommit) && justification_MsgPrecommit.getSigns().getSize() == this->generalQuorumSize && this->view == proposeView_MsgPrecommit && phase_MsgPrecommit == PHASE_PRECOMMIT)
	{
		this->prepareHash = proposeHash_MsgPrecommit;
		this->prepareView = proposeView_MsgPrecommit;
		this->lockHash = proposeHash_MsgPrecommit;
		this->lockView = proposeView_MsgPrecommit;
		if (DEBUG_MODULES)
		{
			std::cout << COLOUR_CYAN << this->replicaId << " locked" << COLOUR_NORMAL << std::endl;
		}
		Justification justification_MsgCommit = this->updateRoundData(proposeHash_MsgPrecommit, Hash(), View());
		return justification_MsgCommit;
	}
	else
	{
		if (DEBUG_MODULES)
		{
			std::cout << COLOUR_CYAN << this->replicaId << " fail to lock in MsgPrecommit" << COLOUR_NORMAL << std::endl;
		}
		return Justification();
	}
}