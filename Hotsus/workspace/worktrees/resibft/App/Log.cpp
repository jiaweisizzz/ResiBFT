#include "Log.h"

Log::Log() {}

// Basic Hotstuff
bool msgNewviewHotstuffFrom(std::set<MsgNewviewHotstuff> msgNewviews, std::set<ReplicaID> signers)
{
	for (std::set<MsgNewviewHotstuff>::iterator itMsg = msgNewviews.begin(); itMsg != msgNewviews.end(); itMsg++)
	{
		MsgNewviewHotstuff msgNewview = *itMsg;
		std::set<ReplicaID> allSigners = msgNewview.signs.getSigners();
		for (std::set<ReplicaID>::iterator itSigner = allSigners.begin(); itSigner != allSigners.end(); itSigner++)
		{
			signers.erase(*itSigner);
			if (signers.empty())
			{
				return true;
			}
		}
	}
	return false;
}

unsigned int Log::storeMsgNewviewHotstuff(MsgNewviewHotstuff msgNewview)
{
	RoundData roundData_MsgNewview = msgNewview.roundData;
	View proposeView_MsgNewview = roundData_MsgNewview.getProposeView();
	std::set<ReplicaID> signers = msgNewview.signs.getSigners();
	std::map<View, std::set<MsgNewviewHotstuff>>::iterator itView = this->newviewsHotstuff.find(proposeView_MsgNewview);
	if (itView != this->newviewsHotstuff.end())
	{
		std::set<MsgNewviewHotstuff> msgNewviews = itView->second;
		if (!msgNewviewHotstuffFrom(msgNewviews, signers))
		{
			msgNewviews.insert(msgNewview);
			this->newviewsHotstuff[proposeView_MsgNewview] = msgNewviews;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Updated entry for MsgNewview in view " << proposeView_MsgNewview << " and the number of MsgNewview is: " << msgNewviews.size() << COLOUR_NORMAL << std::endl;
			}
			return msgNewviews.size();
		}
	}
	else
	{
		std::set<MsgNewviewHotstuff> msgNewviews = {msgNewview};
		this->newviewsHotstuff[proposeView_MsgNewview] = msgNewviews;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "No entry for MsgNewview in view " << proposeView_MsgNewview << " and the number of MsgNewview is: 1" << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
	return 0;
}

bool msgLdrprepareHotstuffFrom(std::set<MsgLdrprepareHotstuff> msgLdrprepares, std::set<ReplicaID> signers)
{
	for (std::set<MsgLdrprepareHotstuff>::iterator itMsg = msgLdrprepares.begin(); itMsg != msgLdrprepares.end(); itMsg++)
	{
		MsgLdrprepareHotstuff msgLdrprepare = *itMsg;
		std::set<ReplicaID> allSigners = msgLdrprepare.signs.getSigners();
		for (std::set<ReplicaID>::iterator itSigner = allSigners.begin(); itSigner != allSigners.end(); itSigner++)
		{
			signers.erase(*itSigner);
			if (signers.empty())
			{
				return true;
			}
		}
	}
	return false;
}

unsigned int Log::storeMsgLdrprepareHotstuff(MsgLdrprepareHotstuff msgLdrprepare)
{
	Proposal<Justification> proposal_MsgLdrprepare = msgLdrprepare.proposal;
	View proposeView_MsgLdrprepare = proposal_MsgLdrprepare.getCertification().getRoundData().getProposeView();
	std::set<ReplicaID> signers = msgLdrprepare.signs.getSigners();
	std::map<View, std::set<MsgLdrprepareHotstuff>>::iterator itView = this->ldrpreparesHotstuff.find(proposeView_MsgLdrprepare);
	if (itView != this->ldrpreparesHotstuff.end())
	{
		std::set<MsgLdrprepareHotstuff> msgLdrprepares = itView->second;
		if (!msgLdrprepareHotstuffFrom(msgLdrprepares, signers))
		{
			msgLdrprepares.insert(msgLdrprepare);
			this->ldrpreparesHotstuff[proposeView_MsgLdrprepare] = msgLdrprepares;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Updated entry for MsgLdrprepare in view " << proposeView_MsgLdrprepare << " and the number of MsgLdrprepare is: " << msgLdrprepares.size() << COLOUR_NORMAL << std::endl;
			}
			return msgLdrprepares.size();
		}
	}
	else
	{
		std::set<MsgLdrprepareHotstuff> msgLdrprepares = {msgLdrprepare};
		this->ldrpreparesHotstuff[proposeView_MsgLdrprepare] = msgLdrprepares;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "No entry for MsgLdrprepare in view " << proposeView_MsgLdrprepare << " and the number of MsgLdrprepare is: 1" << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
	return 0;
}

bool msgPrepareHotstuffFrom(std::set<MsgPrepareHotstuff> msgPrepares, std::set<ReplicaID> signers)
{
	for (std::set<MsgPrepareHotstuff>::iterator itMsg = msgPrepares.begin(); itMsg != msgPrepares.end(); itMsg++)
	{
		MsgPrepareHotstuff msgPrepare = *itMsg;
		std::set<ReplicaID> allSigners = msgPrepare.signs.getSigners();
		for (std::set<ReplicaID>::iterator itSigner = allSigners.begin(); itSigner != allSigners.end(); itSigner++)
		{
			signers.erase(*itSigner);
			if (signers.empty())
			{
				return true;
			}
		}
	}
	return false;
}

unsigned int Log::storeMsgPrepareHotstuff(MsgPrepareHotstuff msgPrepare)
{
	RoundData roundData_MsgPrepare = msgPrepare.roundData;
	View proposeView_MsgPrepare = roundData_MsgPrepare.getProposeView();
	std::set<ReplicaID> signers = msgPrepare.signs.getSigners();
	std::map<View, std::set<MsgPrepareHotstuff>>::iterator itView = this->preparesHotstuff.find(proposeView_MsgPrepare);
	if (itView != this->preparesHotstuff.end())
	{
		std::set<MsgPrepareHotstuff> msgPrepares = itView->second;
		if (!msgPrepareHotstuffFrom(msgPrepares, signers))
		{
			msgPrepares.insert(msgPrepare);
			this->preparesHotstuff[proposeView_MsgPrepare] = msgPrepares;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Updated entry for MsgPrepare in view " << proposeView_MsgPrepare << " and the number of MsgPrepare is: " << msgPrepares.size() << COLOUR_NORMAL << std::endl;
			}
			return msgPrepares.size();
		}
	}
	else
	{
		std::set<MsgPrepareHotstuff> msgPrepares = {msgPrepare};
		this->preparesHotstuff[proposeView_MsgPrepare] = msgPrepares;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "No entry for MsgPrepare in view " << proposeView_MsgPrepare << " and the number of MsgPrepare is: 1" << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
	return 0;
}

bool msgPrecommitHotstuffFrom(std::set<MsgPrecommitHotstuff> msgPrecommits, std::set<ReplicaID> signers)
{
	for (std::set<MsgPrecommitHotstuff>::iterator itMsg = msgPrecommits.begin(); itMsg != msgPrecommits.end(); itMsg++)
	{
		MsgPrecommitHotstuff msgPrecommit = *itMsg;
		std::set<ReplicaID> allSigners = msgPrecommit.signs.getSigners();
		for (std::set<ReplicaID>::iterator itSigner = allSigners.begin(); itSigner != allSigners.end(); itSigner++)
		{
			signers.erase(*itSigner);
			if (signers.empty())
			{
				return true;
			}
		}
	}
	return false;
}

unsigned int Log::storeMsgPrecommitHotstuff(MsgPrecommitHotstuff msgPrecommit)
{
	RoundData roundData_MsgPrecommit = msgPrecommit.roundData;
	View proposeView_MsgPrecommit = roundData_MsgPrecommit.getProposeView();
	std::set<ReplicaID> signers = msgPrecommit.signs.getSigners();
	std::map<View, std::set<MsgPrecommitHotstuff>>::iterator itView = this->precommitsHotstuff.find(proposeView_MsgPrecommit);
	if (itView != this->precommitsHotstuff.end())
	{
		std::set<MsgPrecommitHotstuff> msgPrecommits = itView->second;
		if (!msgPrecommitHotstuffFrom(msgPrecommits, signers))
		{
			msgPrecommits.insert(msgPrecommit);
			this->precommitsHotstuff[proposeView_MsgPrecommit] = msgPrecommits;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Updated entry for MsgPrecommit in view " << proposeView_MsgPrecommit << " and the number of MsgPrecommit is: " << msgPrecommits.size() << COLOUR_NORMAL << std::endl;
			}
			return msgPrecommits.size();
		}
	}
	else
	{
		std::set<MsgPrecommitHotstuff> msgPrecommits = {msgPrecommit};
		this->precommitsHotstuff[proposeView_MsgPrecommit] = msgPrecommits;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "No entry for MsgPrecommit in view " << proposeView_MsgPrecommit << " and the number of MsgPrecommit is: 1" << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
	return 0;
}

bool msgCommitHotstuffFrom(std::set<MsgCommitHotstuff> msgCommits, std::set<ReplicaID> signers)
{
	for (std::set<MsgCommitHotstuff>::iterator itMsg = msgCommits.begin(); itMsg != msgCommits.end(); itMsg++)
	{
		MsgCommitHotstuff msgCommit = *itMsg;
		std::set<ReplicaID> allSigners = msgCommit.signs.getSigners();
		for (std::set<ReplicaID>::iterator itSigner = allSigners.begin(); itSigner != allSigners.end(); itSigner++)
		{
			signers.erase(*itSigner);
			if (signers.empty())
			{
				return true;
			}
		}
	}
	return false;
}

unsigned int Log::storeMsgCommitHotstuff(MsgCommitHotstuff msgCommit)
{
	RoundData roundData_MsgCommit = msgCommit.roundData;
	View proposeView_MsgCommit = roundData_MsgCommit.getProposeView();
	std::set<ReplicaID> signers = msgCommit.signs.getSigners();
	std::map<View, std::set<MsgCommitHotstuff>>::iterator itView = this->commitsHotstuff.find(proposeView_MsgCommit);
	if (itView != this->commitsHotstuff.end())
	{
		std::set<MsgCommitHotstuff> msgCommits = itView->second;
		if (!msgCommitHotstuffFrom(msgCommits, signers))
		{
			msgCommits.insert(msgCommit);
			this->commitsHotstuff[proposeView_MsgCommit] = msgCommits;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Updated entry for MsgCommit in view " << proposeView_MsgCommit << " and the number of MsgCommit is: " << msgCommits.size() << COLOUR_NORMAL << std::endl;
			}
			return msgCommits.size();
		}
	}
	else
	{
		std::set<MsgCommitHotstuff> msgCommits = {msgCommit};
		this->commitsHotstuff[proposeView_MsgCommit] = msgCommits;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "No entry for MsgCommit in view " << proposeView_MsgCommit << " and the number of MsgCommit is: 1" << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
	return 0;
}

Signs Log::getMsgNewviewHotstuff(View view, unsigned int n)
{
	Signs signs;
	std::map<View, std::set<MsgNewviewHotstuff>>::iterator itView = this->newviewsHotstuff.find(view);
	if (itView != this->newviewsHotstuff.end())
	{
		std::set<MsgNewviewHotstuff> msgNewviews = itView->second;
		for (std::set<MsgNewviewHotstuff>::iterator itMsg = msgNewviews.begin(); signs.getSize() < n && itMsg != msgNewviews.end(); itMsg++)
		{
			MsgNewviewHotstuff msgNewview = *itMsg;
			Signs signs_MsgNewview = msgNewview.signs;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Adding signatures: " << signs_MsgNewview.toPrint() << COLOUR_NORMAL << std::endl;
			}
			signs.addUpto(signs_MsgNewview, n);
		}
	}
	if (DEBUG_LOG)
	{
		std::cout << COLOUR_GREEN << "Log signatures: " << signs.toPrint() << COLOUR_NORMAL << std::endl;
	}
	return signs;
}

Signs Log::getMsgPrepareHotstuff(View view, unsigned int n)
{
	Signs signs;
	std::map<View, std::set<MsgPrepareHotstuff>>::iterator itView = this->preparesHotstuff.find(view);
	if (itView != this->preparesHotstuff.end())
	{
		std::set<MsgPrepareHotstuff> msgPrepares = itView->second;
		for (std::set<MsgPrepareHotstuff>::iterator itMsg = msgPrepares.begin(); signs.getSize() < n && itMsg != msgPrepares.end(); itMsg++)
		{
			MsgPrepareHotstuff msgPrepare = *itMsg;
			Signs signs_MsgPrepare = msgPrepare.signs;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Adding signatures: " << signs_MsgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
			}
			signs.addUpto(signs_MsgPrepare, n);
		}
	}
	if (DEBUG_LOG)
	{
		std::cout << COLOUR_GREEN << "Log signatures: " << signs.toPrint() << COLOUR_NORMAL << std::endl;
	}
	return signs;
}

Signs Log::getMsgPrecommitHotstuff(View view, unsigned int n)
{
	Signs signs;
	std::map<View, std::set<MsgPrecommitHotstuff>>::iterator itView = this->precommitsHotstuff.find(view);
	if (itView != this->precommitsHotstuff.end())
	{
		std::set<MsgPrecommitHotstuff> msgPrecommits = itView->second;
		for (std::set<MsgPrecommitHotstuff>::iterator itMsg = msgPrecommits.begin(); signs.getSize() < n && itMsg != msgPrecommits.end(); itMsg++)
		{
			MsgPrecommitHotstuff msgPrecommit = *itMsg;
			Signs signs_MsgPrecommit = msgPrecommit.signs;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Adding signatures: " << signs_MsgPrecommit.toPrint() << COLOUR_NORMAL << std::endl;
			}
			signs.addUpto(signs_MsgPrecommit, n);
		}
	}
	if (DEBUG_LOG)
	{
		std::cout << COLOUR_GREEN << "Log signatures: " << signs.toPrint() << COLOUR_NORMAL << std::endl;
	}
	return signs;
}

Signs Log::getMsgCommitHotstuff(View view, unsigned int n)
{
	Signs signs;
	std::map<View, std::set<MsgCommitHotstuff>>::iterator itView = this->commitsHotstuff.find(view);
	if (itView != this->commitsHotstuff.end())
	{
		std::set<MsgCommitHotstuff> msgCommits = itView->second;
		for (std::set<MsgCommitHotstuff>::iterator itMsg = msgCommits.begin(); signs.getSize() < n && itMsg != msgCommits.end(); itMsg++)
		{
			MsgCommitHotstuff msgCommit = *itMsg;
			Signs signs_MsgCommit = msgCommit.signs;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Adding signatures: " << signs_MsgCommit.toPrint() << COLOUR_NORMAL << std::endl;
			}
			signs.addUpto(signs_MsgCommit, n);
		}
	}
	if (DEBUG_LOG)
	{
		std::cout << COLOUR_GREEN << "Log signatures: " << signs.toPrint() << COLOUR_NORMAL << std::endl;
	}
	return signs;
}

Justification Log::findHighestMsgNewviewHotstuff(View view)
{
	std::map<View, std::set<MsgNewviewHotstuff>>::iterator itView = this->newviewsHotstuff.find(view);
	Justification justification_MsgNewview = Justification();
	if (itView != this->newviewsHotstuff.end())
	{
		std::set<MsgNewviewHotstuff> msgNewviews = itView->second;
		View highView = 0;
		for (std::set<MsgNewviewHotstuff>::iterator itMsg = msgNewviews.begin(); itMsg != msgNewviews.end(); itMsg++)
		{
			MsgNewviewHotstuff msgNewview = *itMsg;
			RoundData roundData_MsgNewview = msgNewview.roundData;
			Signs signs_MsgNewview = msgNewview.signs;
			View justifyView_MsgNewview = roundData_MsgNewview.getJustifyView();
			if (justifyView_MsgNewview >= highView)
			{
				highView = justifyView_MsgNewview;
				justification_MsgNewview = Justification(roundData_MsgNewview, signs_MsgNewview);
			}
		}
	}
	return justification_MsgNewview;
}

MsgLdrprepareHotstuff Log::firstMsgLdrprepareHotstuff(View view)
{
	std::map<View, std::set<MsgLdrprepareHotstuff>>::iterator itView = this->ldrpreparesHotstuff.find(view);
	if (itView != this->ldrpreparesHotstuff.end())
	{
		std::set<MsgLdrprepareHotstuff> msgLdrprepares = itView->second;
		if (msgLdrprepares.size() > 0)
		{
			std::set<MsgLdrprepareHotstuff>::iterator itMsg = msgLdrprepares.begin();
			MsgLdrprepareHotstuff msgLdrprepare = *itMsg;
			return msgLdrprepare;
		}
	}
	Proposal<Justification> proposal;
	Signs signs;
	MsgLdrprepareHotstuff msgLdrprepare = MsgLdrprepareHotstuff(proposal, signs);
	return msgLdrprepare;
}

Justification Log::firstMsgPrepareHotstuff(View view)
{
	std::map<View, std::set<MsgPrepareHotstuff>>::iterator itView = this->preparesHotstuff.find(view);
	if (itView != this->preparesHotstuff.end())
	{
		std::set<MsgPrepareHotstuff> msgPrepares = itView->second;
		if (msgPrepares.size() > 0)
		{
			std::set<MsgPrepareHotstuff>::iterator itMsg = msgPrepares.begin();
			MsgPrepareHotstuff msgPrepare = *itMsg;
			RoundData roundData_MsgPrepare = msgPrepare.roundData;
			Signs signs_MsgPrepare = msgPrepare.signs;
			Justification justification_MsgPrepare = Justification(roundData_MsgPrepare, signs_MsgPrepare);
			return justification_MsgPrepare;
		}
	}
	Justification justification = Justification();
	return justification;
}

Justification Log::firstMsgPrecommitHotstuff(View view)
{
	std::map<View, std::set<MsgPrecommitHotstuff>>::iterator itView = this->precommitsHotstuff.find(view);
	if (itView != this->precommitsHotstuff.end())
	{
		std::set<MsgPrecommitHotstuff> msgPrecommits = itView->second;
		if (msgPrecommits.size() > 0)
		{
			std::set<MsgPrecommitHotstuff>::iterator itMsg = msgPrecommits.begin();
			MsgPrecommitHotstuff msgPrecommit = *itMsg;
			RoundData roundData_MsgPrecommit = msgPrecommit.roundData;
			Signs signs_MsgPrecommit = msgPrecommit.signs;
			Justification justification_MsgPrecommit = Justification(roundData_MsgPrecommit, signs_MsgPrecommit);
			return justification_MsgPrecommit;
		}
	}
	Justification justification = Justification();
	return justification;
}

Justification Log::firstMsgCommitHotstuff(View view)
{
	std::map<View, std::set<MsgCommitHotstuff>>::iterator itView = this->commitsHotstuff.find(view);
	if (itView != this->commitsHotstuff.end())
	{
		std::set<MsgCommitHotstuff> msgCommits = itView->second;
		if (msgCommits.size() > 0)
		{
			std::set<MsgCommitHotstuff>::iterator itMsg = msgCommits.begin();
			MsgCommitHotstuff msgCommit = *itMsg;
			RoundData roundData_MsgCommit = msgCommit.roundData;
			Signs signs_MsgCommit = msgCommit.signs;
			Justification justification_MsgCommit = Justification(roundData_MsgCommit, signs_MsgCommit);
			return justification_MsgCommit;
		}
	}
	Justification justification = Justification();
	return justification;
}

// Basic Damysus
bool msgNewviewDamysusFrom(std::set<MsgNewviewDamysus> msgNewviews, std::set<ReplicaID> signers)
{
	for (std::set<MsgNewviewDamysus>::iterator itMsg = msgNewviews.begin(); itMsg != msgNewviews.end(); itMsg++)
	{
		MsgNewviewDamysus msgNewview = *itMsg;
		std::set<ReplicaID> allSigners = msgNewview.signs.getSigners();
		for (std::set<ReplicaID>::iterator itSigners = allSigners.begin(); itSigners != allSigners.end(); itSigners++)
		{
			signers.erase(*itSigners);
			if (signers.empty())
			{
				return true;
			}
		}
	}
	return false;
}

unsigned int Log::storeMsgNewviewDamysus(MsgNewviewDamysus msgNewview)
{
	RoundData roundData_MsgNewview = msgNewview.roundData;
	View proposeView_MsgNewview = roundData_MsgNewview.getProposeView();
	std::set<ReplicaID> signers = msgNewview.signs.getSigners();

	std::map<View, std::set<MsgNewviewDamysus>>::iterator itView = this->newviewsDamysus.find(proposeView_MsgNewview);
	if (itView != this->newviewsDamysus.end())
	{
		std::set<MsgNewviewDamysus> msgNewviews = itView->second;
		if (!msgNewviewDamysusFrom(msgNewviews, signers))
		{
			msgNewviews.insert(msgNewview);
			this->newviewsDamysus[proposeView_MsgNewview] = msgNewviews;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Updated entry for MsgNewview in view " << proposeView_MsgNewview << " and the number of MsgNewview is: " << msgNewviews.size() << COLOUR_NORMAL << std::endl;
			}
			return msgNewviews.size();
		}
	}
	else
	{
		std::set<MsgNewviewDamysus> msgNewviews = {msgNewview};
		this->newviewsDamysus[proposeView_MsgNewview] = msgNewviews;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "No entry for MsgNewview in view " << proposeView_MsgNewview << " and the number of MsgNewview is: 1" << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
	return 0;
}

bool msgLdrprepareDamysusFrom(std::set<MsgLdrprepareDamysus> msgLdrprepares, std::set<ReplicaID> signers)
{
	for (std::set<MsgLdrprepareDamysus>::iterator itMsg = msgLdrprepares.begin(); itMsg != msgLdrprepares.end(); itMsg++)
	{
		MsgLdrprepareDamysus msgLdrprepare = *itMsg;
		std::set<ReplicaID> allSigners = msgLdrprepare.signs.getSigners();
		for (std::set<ReplicaID>::iterator itSigners = allSigners.begin(); itSigners != allSigners.end(); itSigners++)
		{
			signers.erase(*itSigners);
			if (signers.empty())
			{
				return true;
			}
		}
	}
	return false;
}

unsigned int Log::storeMsgLdrprepareDamysus(MsgLdrprepareDamysus msgLdrprepare)
{
	Accumulator accumulator_MsgLdrprepare = msgLdrprepare.proposal.getCertification();
	View proposeView_MsgLdrprepare = accumulator_MsgLdrprepare.getProposeView();
	std::set<ReplicaID> signers = msgLdrprepare.signs.getSigners();

	std::map<View, std::set<MsgLdrprepareDamysus>>::iterator itView = this->ldrpreparesDamysus.find(proposeView_MsgLdrprepare);
	if (itView != this->ldrpreparesDamysus.end())
	{
		std::set<MsgLdrprepareDamysus> msgLdrprepares = itView->second;
		if (!msgLdrprepareDamysusFrom(msgLdrprepares, signers))
		{
			msgLdrprepares.insert(msgLdrprepare);
			this->ldrpreparesDamysus[proposeView_MsgLdrprepare] = msgLdrprepares;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Updated entry for MsgLdrprepare in view " << proposeView_MsgLdrprepare << " and the number of MsgLdrprepare is: " << msgLdrprepares.size() << COLOUR_NORMAL << std::endl;
			}
			return msgLdrprepares.size();
		}
	}
	else
	{
		std::set<MsgLdrprepareDamysus> msgLdrprepares = {msgLdrprepare};
		this->ldrpreparesDamysus[proposeView_MsgLdrprepare] = msgLdrprepares;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "No entry for MsgLdrprepare in view " << proposeView_MsgLdrprepare << " and the number of MsgLdrprepare is: 1" << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
	return 0;
}

bool msgPrepareDamysusFrom(std::set<MsgPrepareDamysus> msgPrepares, std::set<ReplicaID> signers)
{
	for (std::set<MsgPrepareDamysus>::iterator itMsg = msgPrepares.begin(); itMsg != msgPrepares.end(); itMsg++)
	{
		MsgPrepareDamysus msgPrepare = *itMsg;
		std::set<ReplicaID> allSigners = msgPrepare.signs.getSigners();
		for (std::set<ReplicaID>::iterator itSigners = allSigners.begin(); itSigners != allSigners.end(); itSigners++)
		{
			signers.erase(*itSigners);
			if (signers.empty())
			{
				return true;
			}
		}
	}
	return false;
}

unsigned int Log::storeMsgPrepareDamysus(MsgPrepareDamysus msgPrepare)
{
	RoundData roundData_MsgPrepare = msgPrepare.roundData;
	View proposeView_MsgPrepare = roundData_MsgPrepare.getProposeView();
	std::set<ReplicaID> signers = msgPrepare.signs.getSigners();

	std::map<View, std::set<MsgPrepareDamysus>>::iterator itView = this->preparesDamysus.find(proposeView_MsgPrepare);
	if (itView != this->preparesDamysus.end())
	{
		std::set<MsgPrepareDamysus> msgPrepares = itView->second;
		if (!msgPrepareDamysusFrom(msgPrepares, signers))
		{
			msgPrepares.insert(msgPrepare);
			this->preparesDamysus[proposeView_MsgPrepare] = msgPrepares;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Updated entry for MsgPrepare in view " << proposeView_MsgPrepare << " and the number of MsgPrepare is: " << msgPrepares.size() << COLOUR_NORMAL << std::endl;
			}
			return msgPrepares.size();
		}
	}
	else
	{
		std::set<MsgPrepareDamysus> msgPrepares = {msgPrepare};
		this->preparesDamysus[proposeView_MsgPrepare] = msgPrepares;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "No entry for MsgPrepare in view " << proposeView_MsgPrepare << " and the number of MsgPrepare is: 1" << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
	return 0;
}

bool msgPrecommitDamysusFrom(std::set<MsgPrecommitDamysus> msgPrecommits, std::set<ReplicaID> signers)
{
	for (std::set<MsgPrecommitDamysus>::iterator itMsg = msgPrecommits.begin(); itMsg != msgPrecommits.end(); itMsg++)
	{
		MsgPrecommitDamysus msgPrecommit = *itMsg;
		std::set<ReplicaID> allSigners = msgPrecommit.signs.getSigners();
		for (std::set<ReplicaID>::iterator itSigners = allSigners.begin(); itSigners != allSigners.end(); itSigners++)
		{
			signers.erase(*itSigners);
			if (signers.empty())
			{
				return true;
			}
		}
	}
	return false;
}

unsigned int Log::storeMsgPrecommitDamysus(MsgPrecommitDamysus msgPrecommit)
{
	RoundData roundData_MsgPrecommit = msgPrecommit.roundData;
	View proposeView_MsgPrecommit = roundData_MsgPrecommit.getProposeView();
	std::set<ReplicaID> signers = msgPrecommit.signs.getSigners();

	std::map<View, std::set<MsgPrecommitDamysus>>::iterator itView = this->precommitsDamysus.find(proposeView_MsgPrecommit);
	if (itView != this->precommitsDamysus.end())
	{
		std::set<MsgPrecommitDamysus> msgPrecommits = itView->second;
		if (!msgPrecommitDamysusFrom(msgPrecommits, signers))
		{
			msgPrecommits.insert(msgPrecommit);
			this->precommitsDamysus[proposeView_MsgPrecommit] = msgPrecommits;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Updated entry for MsgPrecommit in view " << proposeView_MsgPrecommit << " and the number of MsgPrecommit is: " << msgPrecommits.size() << COLOUR_NORMAL << std::endl;
			}
			return msgPrecommits.size();
		}
	}
	else
	{
		std::set<MsgPrecommitDamysus> msgPrecommits = {msgPrecommit};
		this->precommitsDamysus[proposeView_MsgPrecommit] = {msgPrecommit};
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "No entry for MsgPrecommit in view " << proposeView_MsgPrecommit << " and the number of MsgPrecommit is: 1" << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
	return 0;
}

std::set<MsgNewviewDamysus> Log::getMsgNewviewDamysus(View view, unsigned int n)
{
	std::set<MsgNewviewDamysus> msgNewview;
	std::map<View, std::set<MsgNewviewDamysus>>::iterator itView = this->newviewsDamysus.find(view);
	if (itView != this->newviewsDamysus.end())
	{
		std::set<MsgNewviewDamysus> msgNewviews = itView->second;
		for (std::set<MsgNewviewDamysus>::iterator itMsg = msgNewviews.begin(); msgNewview.size() < n && itMsg != msgNewviews.end(); itMsg++)
		{
			MsgNewviewDamysus msg = *itMsg;
			msgNewview.insert(msg);
		}
	}
	return msgNewview;
}

Signs Log::getMsgPrepareDamysus(View view, unsigned int n)
{
	Signs signs;
	std::map<View, std::set<MsgPrepareDamysus>>::iterator itView = this->preparesDamysus.find(view);
	if (itView != this->preparesDamysus.end())
	{
		std::set<MsgPrepareDamysus> msgPrepares = itView->second;
		for (std::set<MsgPrepareDamysus>::iterator itMsg = msgPrepares.begin(); signs.getSize() < n && itMsg != msgPrepares.end(); itMsg++)
		{
			MsgPrepareDamysus msgPrepare = *itMsg;
			Signs signs_MsgPrepare = msgPrepare.signs;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Adding signatures: " << signs_MsgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
			}
			signs.addUpto(signs_MsgPrepare, n);
		}
	}
	if (DEBUG_LOG)
	{
		std::cout << COLOUR_GREEN << "Log signatures: " << signs.toPrint() << COLOUR_NORMAL << std::endl;
	}
	return signs;
}

Signs Log::getMsgPrecommitDamysus(View view, unsigned int n)
{
	Signs signs;
	std::map<View, std::set<MsgPrecommitDamysus>>::iterator itView = this->precommitsDamysus.find(view);
	if (itView != this->precommitsDamysus.end())
	{
		std::set<MsgPrecommitDamysus> msgPrecommits = itView->second;
		for (std::set<MsgPrecommitDamysus>::iterator itMsg = msgPrecommits.begin(); signs.getSize() < n && itMsg != msgPrecommits.end(); itMsg++)
		{
			MsgPrecommitDamysus msgPrecommit = *itMsg;
			Signs signs_MsgPrecommit = msgPrecommit.signs;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Adding signatures: " << signs_MsgPrecommit.toPrint() << COLOUR_NORMAL << std::endl;
			}
			signs.addUpto(signs_MsgPrecommit, n);
		}
	}
	if (DEBUG_LOG)
	{
		std::cout << COLOUR_GREEN << "Log signatures: " << signs.toPrint() << COLOUR_NORMAL << std::endl;
	}
	return signs;
}

MsgLdrprepareDamysus Log::firstMsgLdrprepareDamysus(View view)
{
	std::map<View, std::set<MsgLdrprepareDamysus>>::iterator itView = this->ldrpreparesDamysus.find(view);
	if (itView != this->ldrpreparesDamysus.end())
	{
		std::set<MsgLdrprepareDamysus> msgLdrprepares = itView->second;
		if (msgLdrprepares.size() > 0)
		{
			std::set<MsgLdrprepareDamysus>::iterator itMsg = msgLdrprepares.begin();
			MsgLdrprepareDamysus msgLdrprepare = *itMsg;
			return msgLdrprepare;
		}
	}
	Proposal<Accumulator> proposal;
	Signs signs;
	MsgLdrprepareDamysus msgLdrprepare = MsgLdrprepareDamysus(proposal, signs);
	return msgLdrprepare;
}

Justification Log::firstMsgPrepareDamysus(View view)
{
	std::map<View, std::set<MsgPrepareDamysus>>::iterator itView = this->preparesDamysus.find(view);
	if (itView != this->preparesDamysus.end())
	{
		std::set<MsgPrepareDamysus> msgPrepares = itView->second;
		if (msgPrepares.size() > 0)
		{
			std::set<MsgPrepareDamysus>::iterator itMsg = msgPrepares.begin();
			MsgPrepareDamysus msgPrepare = *itMsg;
			RoundData roundData_MsgPrepare = msgPrepare.roundData;
			Signs signs_MsgPrepare = msgPrepare.signs;
			Justification justification_MsgPrepare = Justification(roundData_MsgPrepare, signs_MsgPrepare);
			return justification_MsgPrepare;
		}
	}
	Justification justification = Justification();
	return justification;
}

Justification Log::firstMsgPrecommitDamysus(View view)
{
	std::map<View, std::set<MsgPrecommitDamysus>>::iterator itView = this->precommitsDamysus.find(view);
	if (itView != this->precommitsDamysus.end())
	{
		std::set<MsgPrecommitDamysus> msgPrecommits = itView->second;
		if (msgPrecommits.size() > 0)
		{
			std::set<MsgPrecommitDamysus>::iterator itMsg = msgPrecommits.begin();
			MsgPrecommitDamysus msgPrecommit = *itMsg;
			RoundData roundData_MsgPrecommit = msgPrecommit.roundData;
			Signs signs_MsgPrecommit = msgPrecommit.signs;
			Justification justification_MsgPrecommit = Justification(roundData_MsgPrecommit, signs_MsgPrecommit);
			return justification_MsgPrecommit;
		}
	}
	Justification justification = Justification();
	return justification;
}

// Basic Hotsus
bool msgNewviewHotsusFrom(std::set<MsgNewviewHotsus> msgNewviews, std::set<ReplicaID> signers)
{
	for (std::set<MsgNewviewHotsus>::iterator itMsg = msgNewviews.begin(); itMsg != msgNewviews.end(); itMsg++)
	{
		MsgNewviewHotsus msgNewview = *itMsg;
		std::set<ReplicaID> allSigners = msgNewview.signs.getSigners();
		for (std::set<ReplicaID>::iterator itSigner = allSigners.begin(); itSigner != allSigners.end(); itSigner++)
		{
			signers.erase(*itSigner);
			if (signers.empty())
			{
				return true;
			}
		}
	}
	return false;
}

unsigned int Log::storeMsgNewviewHotsus(MsgNewviewHotsus msgNewview)
{
	RoundData roundData_MsgNewview = msgNewview.roundData;
	View proposeView_MsgNewview = roundData_MsgNewview.getProposeView();
	std::set<ReplicaID> signers = msgNewview.signs.getSigners();
	std::map<View, std::set<MsgNewviewHotsus>>::iterator itView = this->newviewsHotsus.find(proposeView_MsgNewview);
	if (itView != this->newviewsHotsus.end())
	{
		std::set<MsgNewviewHotsus> msgNewviews = itView->second;
		if (!msgNewviewHotsusFrom(msgNewviews, signers))
		{
			msgNewviews.insert(msgNewview);
			this->newviewsHotsus[proposeView_MsgNewview] = msgNewviews;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Updated entry for MsgNewview in view " << proposeView_MsgNewview << " and the number of MsgNewview is: " << msgNewviews.size() << COLOUR_NORMAL << std::endl;
			}
			return msgNewviews.size();
		}
	}
	else
	{
		std::set<MsgNewviewHotsus> msgNewviews = {msgNewview};
		this->newviewsHotsus[proposeView_MsgNewview] = msgNewviews;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "No entry for MsgNewview in view " << proposeView_MsgNewview << " and the number of MsgNewview is: 1" << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
	return 0;
}

bool msgLdrprepareHotsusFrom(std::set<MsgLdrprepareHotsus> msgLdrprepares, std::set<ReplicaID> signers)
{
	for (std::set<MsgLdrprepareHotsus>::iterator itMsg = msgLdrprepares.begin(); itMsg != msgLdrprepares.end(); itMsg++)
	{
		MsgLdrprepareHotsus msgLdrprepare = *itMsg;
		std::set<ReplicaID> allSigners = msgLdrprepare.signs.getSigners();
		for (std::set<ReplicaID>::iterator itSigners = allSigners.begin(); itSigners != allSigners.end(); itSigners++)
		{
			signers.erase(*itSigners);
			if (signers.empty())
			{
				return true;
			}
		}
	}
	return false;
}

unsigned int Log::storeMsgLdrprepareHotsus(MsgLdrprepareHotsus msgLdrprepare)
{
	Accumulator accumulator_MsgLdrprepare = msgLdrprepare.proposal.getCertification();
	View proposeView_MsgLdrprepare = accumulator_MsgLdrprepare.getProposeView();
	std::set<ReplicaID> signers = msgLdrprepare.signs.getSigners();

	std::map<View, std::set<MsgLdrprepareHotsus>>::iterator itView = this->ldrpreparesHotsus.find(proposeView_MsgLdrprepare);
	if (itView != this->ldrpreparesHotsus.end())
	{
		std::set<MsgLdrprepareHotsus> msgLdrprepares = itView->second;
		if (!msgLdrprepareHotsusFrom(msgLdrprepares, signers))
		{
			msgLdrprepares.insert(msgLdrprepare);
			this->ldrpreparesHotsus[proposeView_MsgLdrprepare] = msgLdrprepares;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Updated entry for MsgLdrprepare in view " << proposeView_MsgLdrprepare << " and the number of MsgLdrprepare is: " << msgLdrprepares.size() << COLOUR_NORMAL << std::endl;
			}
			return msgLdrprepares.size();
		}
	}
	else
	{
		std::set<MsgLdrprepareHotsus> msgLdrprepares = {msgLdrprepare};
		this->ldrpreparesHotsus[proposeView_MsgLdrprepare] = msgLdrprepares;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "No entry for MsgLdrprepare in view " << proposeView_MsgLdrprepare << " and the number of MsgLdrprepare is: 1" << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
	return 0;
}

bool msgPrepareHotsusFrom(std::set<MsgPrepareHotsus> msgPrepares, std::set<ReplicaID> signers)
{
	for (std::set<MsgPrepareHotsus>::iterator itMsg = msgPrepares.begin(); itMsg != msgPrepares.end(); itMsg++)
	{
		MsgPrepareHotsus msgPrepare = *itMsg;
		std::set<ReplicaID> allSigners = msgPrepare.signs.getSigners();
		for (std::set<ReplicaID>::iterator itSigners = allSigners.begin(); itSigners != allSigners.end(); itSigners++)
		{
			signers.erase(*itSigners);
			if (signers.empty())
			{
				return true;
			}
		}
	}
	return false;
}

unsigned int Log::storeMsgPrepareHotsus(MsgPrepareHotsus msgPrepare)
{
	RoundData roundData_MsgPrepare = msgPrepare.roundData;
	View proposeView_MsgPrepare = roundData_MsgPrepare.getProposeView();
	std::set<ReplicaID> signers = msgPrepare.signs.getSigners();

	std::map<View, std::set<MsgPrepareHotsus>>::iterator itView = this->preparesHotsus.find(proposeView_MsgPrepare);
	if (itView != this->preparesHotsus.end())
	{
		std::set<MsgPrepareHotsus> msgPrepares = itView->second;
		if (!msgPrepareHotsusFrom(msgPrepares, signers))
		{
			msgPrepares.insert(msgPrepare);
			this->preparesHotsus[proposeView_MsgPrepare] = msgPrepares;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Updated entry for MsgPrepare in view " << proposeView_MsgPrepare << " and the number of MsgPrepare is: " << msgPrepares.size() << COLOUR_NORMAL << std::endl;
			}
			return msgPrepares.size();
		}
	}
	else
	{
		std::set<MsgPrepareHotsus> msgPrepares = {msgPrepare};
		this->preparesHotsus[proposeView_MsgPrepare] = msgPrepares;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "No entry for MsgPrepare in view " << proposeView_MsgPrepare << " and the number of MsgPrepare is: 1" << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
	return 0;
}

bool msgPrecommitHotsusFrom(std::set<MsgPrecommitHotsus> msgPrecommits, std::set<ReplicaID> signers)
{
	for (std::set<MsgPrecommitHotsus>::iterator itMsg = msgPrecommits.begin(); itMsg != msgPrecommits.end(); itMsg++)
	{
		MsgPrecommitHotsus msgPrecommit = *itMsg;
		std::set<ReplicaID> allSigners = msgPrecommit.signs.getSigners();
		for (std::set<ReplicaID>::iterator itSigners = allSigners.begin(); itSigners != allSigners.end(); itSigners++)
		{
			signers.erase(*itSigners);
			if (signers.empty())
			{
				return true;
			}
		}
	}
	return false;
}

unsigned int Log::storeMsgPrecommitHotsus(MsgPrecommitHotsus msgPrecommit)
{
	RoundData roundData_MsgPrecommit = msgPrecommit.roundData;
	View proposeView_MsgPrecommit = roundData_MsgPrecommit.getProposeView();
	std::set<ReplicaID> signers = msgPrecommit.signs.getSigners();

	std::map<View, std::set<MsgPrecommitHotsus>>::iterator itView = this->precommitsHotsus.find(proposeView_MsgPrecommit);
	if (itView != this->precommitsHotsus.end())
	{
		std::set<MsgPrecommitHotsus> msgPrecommits = itView->second;
		if (!msgPrecommitHotsusFrom(msgPrecommits, signers))
		{
			msgPrecommits.insert(msgPrecommit);
			this->precommitsHotsus[proposeView_MsgPrecommit] = msgPrecommits;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Updated entry for MsgPrecommit in view " << proposeView_MsgPrecommit << " and the number of MsgPrecommit is: " << msgPrecommits.size() << COLOUR_NORMAL << std::endl;
			}
			return msgPrecommits.size();
		}
	}
	else
	{
		std::set<MsgPrecommitHotsus> msgPrecommits = {msgPrecommit};
		this->precommitsHotsus[proposeView_MsgPrecommit] = {msgPrecommit};
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "No entry for MsgPrecommit in view " << proposeView_MsgPrecommit << " and the number of MsgPrecommit is: 1" << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
	return 0;
}

bool msgExnewviewHotsusFrom(std::set<MsgExnewviewHotsus> msgExnewviews, std::set<ReplicaID> signers)
{
	for (std::set<MsgExnewviewHotsus>::iterator itMsg = msgExnewviews.begin(); itMsg != msgExnewviews.end(); itMsg++)
	{
		MsgExnewviewHotsus msgExnewview = *itMsg;
		std::set<ReplicaID> allSigners = msgExnewview.signs.getSigners();
		for (std::set<ReplicaID>::iterator itSigner = allSigners.begin(); itSigner != allSigners.end(); itSigner++)
		{
			signers.erase(*itSigner);
			if (signers.empty())
			{
				return true;
			}
		}
	}
	return false;
}

unsigned int Log::storeMsgExnewviewHotsus(MsgExnewviewHotsus msgExnewview)
{
	RoundData roundData_MsgExnewview = msgExnewview.roundData;
	View proposeView_MsgExnewview = roundData_MsgExnewview.getProposeView();
	std::set<ReplicaID> signers = msgExnewview.signs.getSigners();
	std::map<View, std::set<MsgExnewviewHotsus>>::iterator itView = this->exnewviewsHotsus.find(proposeView_MsgExnewview);
	if (itView != this->exnewviewsHotsus.end())
	{
		std::set<MsgExnewviewHotsus> msgExnewviews = itView->second;
		if (!msgExnewviewHotsusFrom(msgExnewviews, signers))
		{
			msgExnewviews.insert(msgExnewview);
			this->exnewviewsHotsus[proposeView_MsgExnewview] = msgExnewviews;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Updated entry for MsgExnewview in view " << proposeView_MsgExnewview << " and the number of MsgExnewview is: " << msgExnewviews.size() << COLOUR_NORMAL << std::endl;
			}
			return msgExnewviews.size();
		}
	}
	else
	{
		std::set<MsgExnewviewHotsus> msgExnewviews = {msgExnewview};
		this->exnewviewsHotsus[proposeView_MsgExnewview] = msgExnewviews;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "No entry for MsgExnewview in view " << proposeView_MsgExnewview << " and the number of MsgExnewview is: 1" << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
	return 0;
}

bool msgExldrprepareHotsusFrom(std::set<MsgExldrprepareHotsus> msgExldrprepares, std::set<ReplicaID> signers)
{
	for (std::set<MsgExldrprepareHotsus>::iterator itMsg = msgExldrprepares.begin(); itMsg != msgExldrprepares.end(); itMsg++)
	{
		MsgExldrprepareHotsus msgExldrprepare = *itMsg;
		std::set<ReplicaID> allSigners = msgExldrprepare.signs.getSigners();
		for (std::set<ReplicaID>::iterator itSigner = allSigners.begin(); itSigner != allSigners.end(); itSigner++)
		{
			signers.erase(*itSigner);
			if (signers.empty())
			{
				return true;
			}
		}
	}
	return false;
}

unsigned int Log::storeMsgExldrprepareHotsus(MsgExldrprepareHotsus msgExldrprepare)
{
	Proposal<Justification> proposal_MsgExldrprepare = msgExldrprepare.proposal;
	View proposeView_MsgExldrprepare = proposal_MsgExldrprepare.getCertification().getRoundData().getProposeView();
	std::set<ReplicaID> signers = msgExldrprepare.signs.getSigners();
	std::map<View, std::set<MsgExldrprepareHotsus>>::iterator itView = this->exldrpreparesHotsus.find(proposeView_MsgExldrprepare);
	if (itView != this->exldrpreparesHotsus.end())
	{
		std::set<MsgExldrprepareHotsus> msgExldrprepares = itView->second;
		if (!msgExldrprepareHotsusFrom(msgExldrprepares, signers))
		{
			msgExldrprepares.insert(msgExldrprepare);
			this->exldrpreparesHotsus[proposeView_MsgExldrprepare] = msgExldrprepares;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Updated entry for MsgExldrprepare in view " << proposeView_MsgExldrprepare << " and the number of MsgExldrprepare is: " << msgExldrprepares.size() << COLOUR_NORMAL << std::endl;
			}
			return msgExldrprepares.size();
		}
	}
	else
	{
		std::set<MsgExldrprepareHotsus> msgExldrprepares = {msgExldrprepare};
		this->exldrpreparesHotsus[proposeView_MsgExldrprepare] = msgExldrprepares;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "No entry for MsgExldrprepare in view " << proposeView_MsgExldrprepare << " and the number of MsgExldrprepare is: 1" << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
	return 0;
}

bool msgExprepareHotsusFrom(std::set<MsgExprepareHotsus> msgExprepares, std::set<ReplicaID> signers)
{
	for (std::set<MsgExprepareHotsus>::iterator itMsg = msgExprepares.begin(); itMsg != msgExprepares.end(); itMsg++)
	{
		MsgExprepareHotsus msgExprepare = *itMsg;
		std::set<ReplicaID> allSigners = msgExprepare.signs.getSigners();
		for (std::set<ReplicaID>::iterator itSigner = allSigners.begin(); itSigner != allSigners.end(); itSigner++)
		{
			signers.erase(*itSigner);
			if (signers.empty())
			{
				return true;
			}
		}
	}
	return false;
}

unsigned int Log::storeMsgExprepareHotsus(MsgExprepareHotsus msgExprepare)
{
	RoundData roundData_MsgExprepare = msgExprepare.roundData;
	View proposeView_MsgExprepare = roundData_MsgExprepare.getProposeView();
	std::set<ReplicaID> signers = msgExprepare.signs.getSigners();
	std::map<View, std::set<MsgExprepareHotsus>>::iterator itView = this->expreparesHotsus.find(proposeView_MsgExprepare);
	if (itView != this->expreparesHotsus.end())
	{
		std::set<MsgExprepareHotsus> msgExprepares = itView->second;
		if (!msgExprepareHotsusFrom(msgExprepares, signers))
		{
			msgExprepares.insert(msgExprepare);
			this->expreparesHotsus[proposeView_MsgExprepare] = msgExprepares;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Updated entry for MsgExprepare in view " << proposeView_MsgExprepare << " and the number of MsgExprepare is: " << msgExprepares.size() << COLOUR_NORMAL << std::endl;
			}
			return msgExprepares.size();
		}
	}
	else
	{
		std::set<MsgExprepareHotsus> msgExprepares = {msgExprepare};
		this->expreparesHotsus[proposeView_MsgExprepare] = msgExprepares;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "No entry for MsgExprepare in view " << proposeView_MsgExprepare << " and the number of MsgExprepare is: 1" << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
	return 0;
}

bool msgExprecommitHotsusFrom(std::set<MsgExprecommitHotsus> msgExprecommits, std::set<ReplicaID> signers)
{
	for (std::set<MsgExprecommitHotsus>::iterator itMsg = msgExprecommits.begin(); itMsg != msgExprecommits.end(); itMsg++)
	{
		MsgExprecommitHotsus msgExprecommit = *itMsg;
		std::set<ReplicaID> allSigners = msgExprecommit.signs.getSigners();
		for (std::set<ReplicaID>::iterator itSigner = allSigners.begin(); itSigner != allSigners.end(); itSigner++)
		{
			signers.erase(*itSigner);
			if (signers.empty())
			{
				return true;
			}
		}
	}
	return false;
}

unsigned int Log::storeMsgExprecommitHotsus(MsgExprecommitHotsus msgExprecommit)
{
	RoundData roundData_MsgExprecommit = msgExprecommit.roundData;
	View proposeView_MsgExprecommit = roundData_MsgExprecommit.getProposeView();
	std::set<ReplicaID> signers = msgExprecommit.signs.getSigners();
	std::map<View, std::set<MsgExprecommitHotsus>>::iterator itView = this->exprecommitsHotsus.find(proposeView_MsgExprecommit);
	if (itView != this->exprecommitsHotsus.end())
	{
		std::set<MsgExprecommitHotsus> msgExprecommits = itView->second;
		if (!msgExprecommitHotsusFrom(msgExprecommits, signers))
		{
			msgExprecommits.insert(msgExprecommit);
			this->exprecommitsHotsus[proposeView_MsgExprecommit] = msgExprecommits;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Updated entry for MsgExprecommit in view " << proposeView_MsgExprecommit << " and the number of MsgExprecommit is: " << msgExprecommits.size() << COLOUR_NORMAL << std::endl;
			}
			return msgExprecommits.size();
		}
	}
	else
	{
		std::set<MsgExprecommitHotsus> msgExprecommits = {msgExprecommit};
		this->exprecommitsHotsus[proposeView_MsgExprecommit] = msgExprecommits;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "No entry for MsgExprecommit in view " << proposeView_MsgExprecommit << " and the number of MsgExprecommit is: 1" << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
	return 0;
}

bool msgExcommitHotsusFrom(std::set<MsgExcommitHotsus> msgExcommits, std::set<ReplicaID> signers)
{
	for (std::set<MsgExcommitHotsus>::iterator itMsg = msgExcommits.begin(); itMsg != msgExcommits.end(); itMsg++)
	{
		MsgExcommitHotsus msgExcommit = *itMsg;
		std::set<ReplicaID> allSigners = msgExcommit.signs.getSigners();
		for (std::set<ReplicaID>::iterator itSigner = allSigners.begin(); itSigner != allSigners.end(); itSigner++)
		{
			signers.erase(*itSigner);
			if (signers.empty())
			{
				return true;
			}
		}
	}
	return false;
}

unsigned int Log::storeMsgExcommitHotsus(MsgExcommitHotsus msgExcommit)
{
	RoundData roundData_MsgExcommit = msgExcommit.roundData;
	View proposeView_MsgExcommit = roundData_MsgExcommit.getProposeView();
	std::set<ReplicaID> signers = msgExcommit.signs.getSigners();
	std::map<View, std::set<MsgExcommitHotsus>>::iterator itView = this->excommitsHotsus.find(proposeView_MsgExcommit);
	if (itView != this->excommitsHotsus.end())
	{
		std::set<MsgExcommitHotsus> msgExcommits = itView->second;
		if (!msgExcommitHotsusFrom(msgExcommits, signers))
		{
			msgExcommits.insert(msgExcommit);
			this->excommitsHotsus[proposeView_MsgExcommit] = msgExcommits;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Updated entry for MsgExcommit in view " << proposeView_MsgExcommit << " and the number of MsgExcommit is: " << msgExcommits.size() << COLOUR_NORMAL << std::endl;
			}
			return msgExcommits.size();
		}
	}
	else
	{
		std::set<MsgExcommitHotsus> msgExcommits = {msgExcommit};
		this->excommitsHotsus[proposeView_MsgExcommit] = msgExcommits;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "No entry for MsgExcommit in view " << proposeView_MsgExcommit << " and the number of MsgExcommit is: 1" << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
	return 0;
}

std::set<MsgNewviewHotsus> Log::getMsgNewviewHotsus(View view, unsigned int n)
{
	std::set<MsgNewviewHotsus> msgNewview;
	std::map<View, std::set<MsgNewviewHotsus>>::iterator itView = this->newviewsHotsus.find(view);
	if (itView != this->newviewsHotsus.end())
	{
		std::set<MsgNewviewHotsus> msgNewviews = itView->second;
		for (std::set<MsgNewviewHotsus>::iterator itMsg = msgNewviews.begin(); msgNewview.size() < n && itMsg != msgNewviews.end(); itMsg++)
		{
			MsgNewviewHotsus msg = *itMsg;
			msgNewview.insert(msg);
		}
	}
	return msgNewview;
}

Signs Log::getMsgPrepareHotsus(View view, unsigned int n)
{
	Signs signs;
	std::map<View, std::set<MsgPrepareHotsus>>::iterator itView = this->preparesHotsus.find(view);
	if (itView != this->preparesHotsus.end())
	{
		std::set<MsgPrepareHotsus> msgPrepares = itView->second;
		for (std::set<MsgPrepareHotsus>::iterator itMsg = msgPrepares.begin(); signs.getSize() < n && itMsg != msgPrepares.end(); itMsg++)
		{
			MsgPrepareHotsus msgPrepare = *itMsg;
			Signs signs_MsgPrepare = msgPrepare.signs;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Adding signatures: " << signs_MsgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
			}
			signs.addUpto(signs_MsgPrepare, n);
		}
	}
	if (DEBUG_LOG)
	{
		std::cout << COLOUR_GREEN << "Log signatures: " << signs.toPrint() << COLOUR_NORMAL << std::endl;
	}
	return signs;
}

Signs Log::getMsgPrecommitHotsus(View view, unsigned int n)
{
	Signs signs;
	std::map<View, std::set<MsgPrecommitHotsus>>::iterator itView = this->precommitsHotsus.find(view);
	if (itView != this->precommitsHotsus.end())
	{
		std::set<MsgPrecommitHotsus> msgPrecommits = itView->second;
		for (std::set<MsgPrecommitHotsus>::iterator itMsg = msgPrecommits.begin(); signs.getSize() < n && itMsg != msgPrecommits.end(); itMsg++)
		{
			MsgPrecommitHotsus msgPrecommit = *itMsg;
			Signs signs_MsgPrecommit = msgPrecommit.signs;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Adding signatures: " << signs_MsgPrecommit.toPrint() << COLOUR_NORMAL << std::endl;
			}
			signs.addUpto(signs_MsgPrecommit, n);
		}
	}
	if (DEBUG_LOG)
	{
		std::cout << COLOUR_GREEN << "Log signatures: " << signs.toPrint() << COLOUR_NORMAL << std::endl;
	}
	return signs;
}

Signs Log::getMsgExnewviewHotsus(View view, unsigned int n)
{
	Signs signs;
	std::map<View, std::set<MsgExnewviewHotsus>>::iterator itView = this->exnewviewsHotsus.find(view);
	if (itView != this->exnewviewsHotsus.end())
	{
		std::set<MsgExnewviewHotsus> msgExnewviews = itView->second;
		for (std::set<MsgExnewviewHotsus>::iterator itMsg = msgExnewviews.begin(); signs.getSize() < n && itMsg != msgExnewviews.end(); itMsg++)
		{
			MsgExnewviewHotsus msgExnewview = *itMsg;
			Signs signs_MsgExnewview = msgExnewview.signs;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Adding signatures: " << signs_MsgExnewview.toPrint() << COLOUR_NORMAL << std::endl;
			}
			signs.addUpto(signs_MsgExnewview, n);
		}
	}
	if (DEBUG_LOG)
	{
		std::cout << COLOUR_GREEN << "Log signatures: " << signs.toPrint() << COLOUR_NORMAL << std::endl;
	}
	return signs;
}

Signs Log::getMsgExprepareHotsus(View view, unsigned int n)
{
	Signs signs;
	std::map<View, std::set<MsgExprepareHotsus>>::iterator itView = this->expreparesHotsus.find(view);
	if (itView != this->expreparesHotsus.end())
	{
		std::set<MsgExprepareHotsus> msgExprepares = itView->second;
		for (std::set<MsgExprepareHotsus>::iterator itMsg = msgExprepares.begin(); signs.getSize() < n && itMsg != msgExprepares.end(); itMsg++)
		{
			MsgExprepareHotsus msgExprepare = *itMsg;
			Signs signs_MsgExprepare = msgExprepare.signs;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Adding signatures: " << signs_MsgExprepare.toPrint() << COLOUR_NORMAL << std::endl;
			}
			signs.addUpto(signs_MsgExprepare, n);
		}
	}
	if (DEBUG_LOG)
	{
		std::cout << COLOUR_GREEN << "Log signatures: " << signs.toPrint() << COLOUR_NORMAL << std::endl;
	}
	return signs;
}

Signs Log::getMsgExprecommitHotsus(View view, unsigned int n)
{
	Signs signs;
	std::map<View, std::set<MsgExprecommitHotsus>>::iterator itView = this->exprecommitsHotsus.find(view);
	if (itView != this->exprecommitsHotsus.end())
	{
		std::set<MsgExprecommitHotsus> msgExprecommits = itView->second;
		for (std::set<MsgExprecommitHotsus>::iterator itMsg = msgExprecommits.begin(); signs.getSize() < n && itMsg != msgExprecommits.end(); itMsg++)
		{
			MsgExprecommitHotsus msgExprecommit = *itMsg;
			Signs signs_MsgExprecommit = msgExprecommit.signs;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Adding signatures: " << signs_MsgExprecommit.toPrint() << COLOUR_NORMAL << std::endl;
			}
			signs.addUpto(signs_MsgExprecommit, n);
		}
	}
	if (DEBUG_LOG)
	{
		std::cout << COLOUR_GREEN << "Log signatures: " << signs.toPrint() << COLOUR_NORMAL << std::endl;
	}
	return signs;
}

Signs Log::getMsgExcommitHotsus(View view, unsigned int n)
{
	Signs signs;
	std::map<View, std::set<MsgExcommitHotsus>>::iterator itView = this->excommitsHotsus.find(view);
	if (itView != this->excommitsHotsus.end())
	{
		std::set<MsgExcommitHotsus> msgExcommits = itView->second;
		for (std::set<MsgExcommitHotsus>::iterator itMsg = msgExcommits.begin(); signs.getSize() < n && itMsg != msgExcommits.end(); itMsg++)
		{
			MsgExcommitHotsus msgExcommit = *itMsg;
			Signs signs_MsgExcommit = msgExcommit.signs;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Adding signatures: " << signs_MsgExcommit.toPrint() << COLOUR_NORMAL << std::endl;
			}
			signs.addUpto(signs_MsgExcommit, n);
		}
	}
	if (DEBUG_LOG)
	{
		std::cout << COLOUR_GREEN << "Log signatures: " << signs.toPrint() << COLOUR_NORMAL << std::endl;
	}
	return signs;
}

std::vector<ReplicaID> Log::getTrustedMsgExnewviewHotsus(View view, unsigned int n)
{
	std::vector<ReplicaID> senders;
	std::map<View, std::set<MsgExnewviewHotsus>>::iterator itView = this->exnewviewsHotsus.find(view);
	if (itView != this->exnewviewsHotsus.end())
	{
		std::set<MsgExnewviewHotsus> msgExnewviews = itView->second;
		unsigned int numMsg = 0;
		for (std::set<MsgExnewviewHotsus>::iterator itMsg = msgExnewviews.begin(); itMsg != msgExnewviews.end() && numMsg < n; itMsg++, numMsg++)
		{
			MsgExnewviewHotsus msgExnewview = *itMsg;
			Signs signs_MsgExnewview = msgExnewview.signs;
			std::set<ReplicaID> signer = signs_MsgExnewview.getSigners();
			ReplicaID sender;
			for (std::set<ReplicaID>::iterator itSigner = signer.begin(); itSigner != signer.end(); itSigner++)
			{
				sender = *itSigner;
				break;
			}
			senders.push_back(sender);
		}
	}
	if (DEBUG_LOG)
	{
		std::cout << COLOUR_GREEN << "Log trusted senders size: " << std::to_string(senders.size()) << COLOUR_NORMAL << std::endl;
	}
	return senders;
}

Justification Log::findHighestMsgExnewviewHotsus(View view)
{
	std::map<View, std::set<MsgExnewviewHotsus>>::iterator itView = this->exnewviewsHotsus.find(view);
	Justification justification_MsgExnewview = Justification();
	if (itView != this->exnewviewsHotsus.end())
	{
		std::set<MsgExnewviewHotsus> msgExnewviews = itView->second;
		View highView = 0;
		for (std::set<MsgExnewviewHotsus>::iterator itMsg = msgExnewviews.begin(); itMsg != msgExnewviews.end(); itMsg++)
		{
			MsgExnewviewHotsus msgExnewview = *itMsg;
			RoundData roundData_MsgExnewview = msgExnewview.roundData;
			Signs signs_MsgExnewview = msgExnewview.signs;
			View justifyView_MsgExnewview = roundData_MsgExnewview.getJustifyView();
			if (justifyView_MsgExnewview >= highView)
			{
				highView = justifyView_MsgExnewview;
				justification_MsgExnewview = Justification(roundData_MsgExnewview, signs_MsgExnewview);
			}
		}
	}
	return justification_MsgExnewview;
}

MsgLdrprepareHotsus Log::firstMsgLdrprepareHotsus(View view)
{
	std::map<View, std::set<MsgLdrprepareHotsus>>::iterator itView = this->ldrpreparesHotsus.find(view);
	if (itView != this->ldrpreparesHotsus.end())
	{
		std::set<MsgLdrprepareHotsus> msgLdrprepares = itView->second;
		if (msgLdrprepares.size() > 0)
		{
			std::set<MsgLdrprepareHotsus>::iterator itMsg = msgLdrprepares.begin();
			MsgLdrprepareHotsus msgLdrprepare = *itMsg;
			return msgLdrprepare;
		}
	}
	Proposal<Accumulator> proposal;
	Signs signs;
	MsgLdrprepareHotsus msgLdrprepare = MsgLdrprepareHotsus(proposal, signs);
	return msgLdrprepare;
}

Justification Log::firstMsgPrepareHotsus(View view)
{
	std::map<View, std::set<MsgPrepareHotsus>>::iterator itView = this->preparesHotsus.find(view);
	if (itView != this->preparesHotsus.end())
	{
		std::set<MsgPrepareHotsus> msgPrepares = itView->second;
		if (msgPrepares.size() > 0)
		{
			std::set<MsgPrepareHotsus>::iterator itMsg = msgPrepares.begin();
			MsgPrepareHotsus msgPrepare = *itMsg;
			RoundData roundData_MsgPrepare = msgPrepare.roundData;
			Signs signs_MsgPrepare = msgPrepare.signs;
			Justification justification_MsgPrepare = Justification(roundData_MsgPrepare, signs_MsgPrepare);
			return justification_MsgPrepare;
		}
	}
	Justification justification = Justification();
	return justification;
}

Justification Log::firstMsgPrecommitHotsus(View view)
{
	std::map<View, std::set<MsgPrecommitHotsus>>::iterator itView = this->precommitsHotsus.find(view);
	if (itView != this->precommitsHotsus.end())
	{
		std::set<MsgPrecommitHotsus> msgPrecommits = itView->second;
		if (msgPrecommits.size() > 0)
		{
			std::set<MsgPrecommitHotsus>::iterator itMsg = msgPrecommits.begin();
			MsgPrecommitHotsus msgPrecommit = *itMsg;
			RoundData roundData_MsgPrecommit = msgPrecommit.roundData;
			Signs signs_MsgPrecommit = msgPrecommit.signs;
			Justification justification_MsgPrecommit = Justification(roundData_MsgPrecommit, signs_MsgPrecommit);
			return justification_MsgPrecommit;
		}
	}
	Justification justification = Justification();
	return justification;
}

MsgExldrprepareHotsus Log::firstMsgExldrprepareHotsus(View view)
{
	std::map<View, std::set<MsgExldrprepareHotsus>>::iterator itView = this->exldrpreparesHotsus.find(view);
	if (itView != this->exldrpreparesHotsus.end())
	{
		std::set<MsgExldrprepareHotsus> msgExldrprepares = itView->second;
		if (msgExldrprepares.size() > 0)
		{
			std::set<MsgExldrprepareHotsus>::iterator itMsg = msgExldrprepares.begin();
			MsgExldrprepareHotsus msgExldrprepare = *itMsg;
			return msgExldrprepare;
		}
	}
	Proposal<Justification> proposal;
	Group group;
	Signs signs;
	MsgExldrprepareHotsus msgExldrprepare = MsgExldrprepareHotsus(proposal, group, signs);
	return msgExldrprepare;
}

Justification Log::firstMsgExprepareHotsus(View view)
{
	std::map<View, std::set<MsgExprepareHotsus>>::iterator itView = this->expreparesHotsus.find(view);
	if (itView != this->expreparesHotsus.end())
	{
		std::set<MsgExprepareHotsus> msgExprepares = itView->second;
		if (msgExprepares.size() > 0)
		{
			std::set<MsgExprepareHotsus>::iterator itMsg = msgExprepares.begin();
			MsgExprepareHotsus msgExprepare = *itMsg;
			RoundData roundData_MsgExprepare = msgExprepare.roundData;
			Signs signs_MsgExprepare = msgExprepare.signs;
			Justification justification_MsgExprepare = Justification(roundData_MsgExprepare, signs_MsgExprepare);
			return justification_MsgExprepare;
		}
	}
	Justification justification = Justification();
	return justification;
}

Justification Log::firstMsgExprecommitHotsus(View view)
{
	std::map<View, std::set<MsgExprecommitHotsus>>::iterator itView = this->exprecommitsHotsus.find(view);
	if (itView != this->exprecommitsHotsus.end())
	{
		std::set<MsgExprecommitHotsus> msgExprecommits = itView->second;
		if (msgExprecommits.size() > 0)
		{
			std::set<MsgExprecommitHotsus>::iterator itMsg = msgExprecommits.begin();
			MsgExprecommitHotsus msgExprecommit = *itMsg;
			RoundData roundData_MsgExprecommit = msgExprecommit.roundData;
			Signs signs_MsgExprecommit = msgExprecommit.signs;
			Justification justification_MsgExprecommit = Justification(roundData_MsgExprecommit, signs_MsgExprecommit);
			return justification_MsgExprecommit;
		}
	}
	Justification justification = Justification();
	return justification;
}

Justification Log::firstMsgExcommitHotsus(View view)
{
	std::map<View, std::set<MsgExcommitHotsus>>::iterator itView = this->excommitsHotsus.find(view);
	if (itView != this->excommitsHotsus.end())
	{
		std::set<MsgExcommitHotsus> msgExcommits = itView->second;
		if (msgExcommits.size() > 0)
		{
			std::set<MsgExcommitHotsus>::iterator itMsg = msgExcommits.begin();
			MsgExcommitHotsus msgExcommit = *itMsg;
			RoundData roundData_MsgExcommit = msgExcommit.roundData;
			Signs signs_MsgExcommit = msgExcommit.signs;
			Justification justification_MsgExcommit = Justification(roundData_MsgExcommit, signs_MsgExcommit);
			return justification_MsgExcommit;
		}
	}
	Justification justification = Justification();
	return justification;
}

// Basic ResiBFT
bool msgNewviewResiBFTFrom(std::set<MsgNewviewResiBFT> msgNewviews, std::set<ReplicaID> signers)
{
	for (std::set<MsgNewviewResiBFT>::iterator itMsg = msgNewviews.begin(); itMsg != msgNewviews.end(); itMsg++)
	{
		MsgNewviewResiBFT msgNewview = *itMsg;
		std::set<ReplicaID> allSigners = msgNewview.signs.getSigners();
		for (std::set<ReplicaID>::iterator itSigner = allSigners.begin(); itSigner != allSigners.end(); itSigner++)
		{
			signers.erase(*itSigner);
			if (signers.empty())
			{
				return true;
			}
		}
	}
	return false;
}

unsigned int Log::storeMsgNewviewResiBFT(MsgNewviewResiBFT msgNewview)
{
	RoundData roundData_MsgNewview = msgNewview.roundData;
	View proposeView_MsgNewview = roundData_MsgNewview.getProposeView();
	std::set<ReplicaID> signers = msgNewview.signs.getSigners();
	std::map<View, std::set<MsgNewviewResiBFT>>::iterator itView = this->newviewsResiBFT.find(proposeView_MsgNewview);
	if (itView != this->newviewsResiBFT.end())
	{
		std::set<MsgNewviewResiBFT> msgNewviews = itView->second;
		if (!msgNewviewResiBFTFrom(msgNewviews, signers))
		{
			msgNewviews.insert(msgNewview);
			this->newviewsResiBFT[proposeView_MsgNewview] = msgNewviews;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Updated entry for MsgNewviewResiBFT in view " << proposeView_MsgNewview << " and the number of MsgNewview is: " << msgNewviews.size() << COLOUR_NORMAL << std::endl;
			}
			return msgNewviews.size();
		}
	}
	else
	{
		std::set<MsgNewviewResiBFT> msgNewviews = {msgNewview};
		this->newviewsResiBFT[proposeView_MsgNewview] = msgNewviews;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "No entry for MsgNewviewResiBFT in view " << proposeView_MsgNewview << " and the number of MsgNewview is: 1" << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
	return 0;
}

bool msgLdrprepareResiBFTFrom(std::set<MsgLdrprepareResiBFT> msgLdrprepares, std::set<ReplicaID> signers)
{
	for (std::set<MsgLdrprepareResiBFT>::iterator itMsg = msgLdrprepares.begin(); itMsg != msgLdrprepares.end(); itMsg++)
	{
		MsgLdrprepareResiBFT msgLdrprepare = *itMsg;
		std::set<ReplicaID> allSigners = msgLdrprepare.signs.getSigners();
		for (std::set<ReplicaID>::iterator itSigners = allSigners.begin(); itSigners != allSigners.end(); itSigners++)
		{
			signers.erase(*itSigners);
			if (signers.empty())
			{
				return true;
			}
		}
	}
	return false;
}

unsigned int Log::storeMsgLdrprepareResiBFT(MsgLdrprepareResiBFT msgLdrprepare)
{
	Accumulator accumulator_MsgLdrprepare = msgLdrprepare.proposal.getCertification();
	View proposeView_MsgLdrprepare = accumulator_MsgLdrprepare.getProposeView();
	std::set<ReplicaID> signers = msgLdrprepare.signs.getSigners();

	std::map<View, std::set<MsgLdrprepareResiBFT>>::iterator itView = this->ldrpreparesResiBFT.find(proposeView_MsgLdrprepare);
	if (itView != this->ldrpreparesResiBFT.end())
	{
		std::set<MsgLdrprepareResiBFT> msgLdrprepares = itView->second;
		if (!msgLdrprepareResiBFTFrom(msgLdrprepares, signers))
		{
			msgLdrprepares.insert(msgLdrprepare);
			this->ldrpreparesResiBFT[proposeView_MsgLdrprepare] = msgLdrprepares;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Updated entry for MsgLdrprepareResiBFT in view " << proposeView_MsgLdrprepare << " and the number of MsgLdrprepare is: " << msgLdrprepares.size() << COLOUR_NORMAL << std::endl;
			}
			return msgLdrprepares.size();
		}
	}
	else
	{
		std::set<MsgLdrprepareResiBFT> msgLdrprepares = {msgLdrprepare};
		this->ldrpreparesResiBFT[proposeView_MsgLdrprepare] = msgLdrprepares;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "No entry for MsgLdrprepareResiBFT in view " << proposeView_MsgLdrprepare << " and the number of MsgLdrprepare is: 1" << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
	return 0;
}

bool msgPrepareResiBFTFrom(std::set<MsgPrepareResiBFT> msgPrepares, std::set<ReplicaID> signers)
{
	for (std::set<MsgPrepareResiBFT>::iterator itMsg = msgPrepares.begin(); itMsg != msgPrepares.end(); itMsg++)
	{
		MsgPrepareResiBFT msgPrepare = *itMsg;
		std::set<ReplicaID> allSigners = msgPrepare.signs.getSigners();
		for (std::set<ReplicaID>::iterator itSigners = allSigners.begin(); itSigners != allSigners.end(); itSigners++)
		{
			signers.erase(*itSigners);
			if (signers.empty())
			{
				return true;
			}
		}
	}
	return false;
}

unsigned int Log::storeMsgPrepareResiBFT(MsgPrepareResiBFT msgPrepare)
{
	RoundData roundData_MsgPrepare = msgPrepare.roundData;
	View proposeView_MsgPrepare = roundData_MsgPrepare.getProposeView();
	std::set<ReplicaID> signers = msgPrepare.signs.getSigners();

	std::map<View, std::set<MsgPrepareResiBFT>>::iterator itView = this->preparesResiBFT.find(proposeView_MsgPrepare);
	if (itView != this->preparesResiBFT.end())
	{
		std::set<MsgPrepareResiBFT> msgPrepares = itView->second;
		if (!msgPrepareResiBFTFrom(msgPrepares, signers))
		{
			msgPrepares.insert(msgPrepare);
			this->preparesResiBFT[proposeView_MsgPrepare] = msgPrepares;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Updated entry for MsgPrepareResiBFT in view " << proposeView_MsgPrepare << " and the number of MsgPrepare is: " << msgPrepares.size() << COLOUR_NORMAL << std::endl;
			}
			return msgPrepares.size();
		}
	}
	else
	{
		std::set<MsgPrepareResiBFT> msgPrepares = {msgPrepare};
		this->preparesResiBFT[proposeView_MsgPrepare] = msgPrepares;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "No entry for MsgPrepareResiBFT in view " << proposeView_MsgPrepare << " and the number of MsgPrepare is: 1" << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
	return 0;
}

bool msgPrecommitResiBFTFrom(std::set<MsgPrecommitResiBFT> msgPrecommits, std::set<ReplicaID> signers)
{
	for (std::set<MsgPrecommitResiBFT>::iterator itMsg = msgPrecommits.begin(); itMsg != msgPrecommits.end(); itMsg++)
	{
		MsgPrecommitResiBFT msgPrecommit = *itMsg;
		std::set<ReplicaID> allSigners = msgPrecommit.signs.getSigners();
		for (std::set<ReplicaID>::iterator itSigners = allSigners.begin(); itSigners != allSigners.end(); itSigners++)
		{
			signers.erase(*itSigners);
			if (signers.empty())
			{
				return true;
			}
		}
	}
	return false;
}

unsigned int Log::storeMsgPrecommitResiBFT(MsgPrecommitResiBFT msgPrecommit)
{
	RoundData roundData_MsgPrecommit = msgPrecommit.roundData;
	View proposeView_MsgPrecommit = roundData_MsgPrecommit.getProposeView();
	std::set<ReplicaID> signers = msgPrecommit.signs.getSigners();

	std::map<View, std::set<MsgPrecommitResiBFT>>::iterator itView = this->precommitsResiBFT.find(proposeView_MsgPrecommit);
	if (itView != this->precommitsResiBFT.end())
	{
		std::set<MsgPrecommitResiBFT> msgPrecommits = itView->second;
		if (!msgPrecommitResiBFTFrom(msgPrecommits, signers))
		{
			msgPrecommits.insert(msgPrecommit);
			this->precommitsResiBFT[proposeView_MsgPrecommit] = msgPrecommits;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Updated entry for MsgPrecommitResiBFT in view " << proposeView_MsgPrecommit << " and the number of MsgPrecommit is: " << msgPrecommits.size() << COLOUR_NORMAL << std::endl;
			}
			return msgPrecommits.size();
		}
	}
	else
	{
		std::set<MsgPrecommitResiBFT> msgPrecommits = {msgPrecommit};
		this->precommitsResiBFT[proposeView_MsgPrecommit] = {msgPrecommit};
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "No entry for MsgPrecommitResiBFT in view " << proposeView_MsgPrecommit << " and the number of MsgPrecommit is: 1" << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
	return 0;
}

bool msgDecideResiBFTFrom(std::set<MsgDecideResiBFT> msgDecides, std::set<ReplicaID> signers)
{
	for (std::set<MsgDecideResiBFT>::iterator itMsg = msgDecides.begin(); itMsg != msgDecides.end(); itMsg++)
	{
		MsgDecideResiBFT msgDecide = *itMsg;
		std::set<ReplicaID> allSigners = msgDecide.signs.getSigners();
		for (std::set<ReplicaID>::iterator itSigners = allSigners.begin(); itSigners != allSigners.end(); itSigners++)
		{
			signers.erase(*itSigners);
			if (signers.empty())
			{
				return true;
			}
		}
	}
	return false;
}

unsigned int Log::storeMsgDecideResiBFT(MsgDecideResiBFT msgDecide)
{
	RoundData roundData_MsgDecide = msgDecide.qcPrecommit.getRoundData();
	View proposeView_MsgDecide = roundData_MsgDecide.getProposeView();
	std::set<ReplicaID> signers = msgDecide.signs.getSigners();

	std::map<View, std::set<MsgDecideResiBFT>>::iterator itView = this->decidesResiBFT.find(proposeView_MsgDecide);
	if (itView != this->decidesResiBFT.end())
	{
		std::set<MsgDecideResiBFT> msgDecides = itView->second;
		if (!msgDecideResiBFTFrom(msgDecides, signers))
		{
			msgDecides.insert(msgDecide);
			this->decidesResiBFT[proposeView_MsgDecide] = msgDecides;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Updated entry for MsgDecideResiBFT in view " << proposeView_MsgDecide << " and the number of MsgDecide is: " << msgDecides.size() << COLOUR_NORMAL << std::endl;
			}
			return msgDecides.size();
		}
	}
	else
	{
		std::set<MsgDecideResiBFT> msgDecides = {msgDecide};
		this->decidesResiBFT[proposeView_MsgDecide] = {msgDecide};
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "No entry for MsgDecideResiBFT in view " << proposeView_MsgDecide << " and the number of MsgDecide is: 1" << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
	return 0;
}

std::set<MsgNewviewResiBFT> Log::getMsgNewviewResiBFT(View view, unsigned int n)
{
	std::set<MsgNewviewResiBFT> msgNewview;
	std::map<View, std::set<MsgNewviewResiBFT>>::iterator itView = this->newviewsResiBFT.find(view);
	if (itView != this->newviewsResiBFT.end())
	{
		std::set<MsgNewviewResiBFT> msgNewviews = itView->second;
		for (std::set<MsgNewviewResiBFT>::iterator itMsg = msgNewviews.begin(); msgNewview.size() < n && itMsg != msgNewviews.end(); itMsg++)
		{
			MsgNewviewResiBFT msg = *itMsg;
			msgNewview.insert(msg);
		}
	}
	return msgNewview;
}

Signs Log::getMsgPrepareResiBFT(View view, unsigned int n)
{
	Signs signs;
	std::map<View, std::set<MsgPrepareResiBFT>>::iterator itView = this->preparesResiBFT.find(view);
	if (itView != this->preparesResiBFT.end())
	{
		std::set<MsgPrepareResiBFT> msgPrepares = itView->second;
		for (std::set<MsgPrepareResiBFT>::iterator itMsg = msgPrepares.begin(); signs.getSize() < n && itMsg != msgPrepares.end(); itMsg++)
		{
			MsgPrepareResiBFT msgPrepare = *itMsg;
			Signs signs_MsgPrepare = msgPrepare.signs;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Adding signatures: " << signs_MsgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
			}
			signs.addUpto(signs_MsgPrepare, n);
		}
	}
	if (DEBUG_LOG)
	{
		std::cout << COLOUR_GREEN << "Log signatures: " << signs.toPrint() << COLOUR_NORMAL << std::endl;
	}
	return signs;
}

Signs Log::getMsgPrecommitResiBFT(View view, unsigned int n)
{
	Signs signs;
	std::map<View, std::set<MsgPrecommitResiBFT>>::iterator itView = this->precommitsResiBFT.find(view);
	if (itView != this->precommitsResiBFT.end())
	{
		std::set<MsgPrecommitResiBFT> msgPrecommits = itView->second;
		for (std::set<MsgPrecommitResiBFT>::iterator itMsg = msgPrecommits.begin(); signs.getSize() < n && itMsg != msgPrecommits.end(); itMsg++)
		{
			MsgPrecommitResiBFT msgPrecommit = *itMsg;
			Signs signs_MsgPrecommit = msgPrecommit.signs;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Adding signatures: " << signs_MsgPrecommit.toPrint() << COLOUR_NORMAL << std::endl;
			}
			signs.addUpto(signs_MsgPrecommit, n);
		}
	}
	if (DEBUG_LOG)
	{
		std::cout << COLOUR_GREEN << "Log signatures: " << signs.toPrint() << COLOUR_NORMAL << std::endl;
	}
	return signs;
}

Signs Log::getMsgDecideResiBFT(View view, unsigned int n)
{
	Signs signs;
	std::map<View, std::set<MsgDecideResiBFT>>::iterator itView = this->decidesResiBFT.find(view);
	if (itView != this->decidesResiBFT.end())
	{
		std::set<MsgDecideResiBFT> msgDecides = itView->second;
		for (std::set<MsgDecideResiBFT>::iterator itMsg = msgDecides.begin(); signs.getSize() < n && itMsg != msgDecides.end(); itMsg++)
		{
			MsgDecideResiBFT msgDecide = *itMsg;
			Signs signs_MsgDecide = msgDecide.signs;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Adding signatures: " << signs_MsgDecide.toPrint() << COLOUR_NORMAL << std::endl;
			}
			signs.addUpto(signs_MsgDecide, n);
		}
	}
	if (DEBUG_LOG)
	{
		std::cout << COLOUR_GREEN << "Log signatures: " << signs.toPrint() << COLOUR_NORMAL << std::endl;
	}
	return signs;
}

// =====================================================
// HotStuff ResiBFT: PrepareProposal and Commit methods
// =====================================================

unsigned int Log::storeMsgPrepareProposalResiBFT(MsgPrepareProposalResiBFT msgPrepareProposal)
{
	// Block 类没有 getView() 方法，从 Justification 的 RoundData 中获取 view
	View view = msgPrepareProposal.proposal.getCertification().getRoundData().getProposeView();
	std::map<View, std::set<MsgPrepareProposalResiBFT>>::iterator itView = this->prepareProposalsResiBFT.find(view);
	if (itView != this->prepareProposalsResiBFT.end())
	{
		std::set<MsgPrepareProposalResiBFT> proposals = itView->second;
		proposals.insert(msgPrepareProposal);
		this->prepareProposalsResiBFT[view] = proposals;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "Stored MsgPrepareProposalResiBFT for view " << view
					  << " count=" << proposals.size() << COLOUR_NORMAL << std::endl;
		}
		return proposals.size();
	}
	else
	{
		std::set<MsgPrepareProposalResiBFT> proposals = {msgPrepareProposal};
		this->prepareProposalsResiBFT[view] = proposals;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "Created new MsgPrepareProposalResiBFT entry for view " << view << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
}

unsigned int Log::storeMsgCommitResiBFT(MsgCommitResiBFT msgCommit)
{
	View view = msgCommit.qcPrecommit.getRoundData().getProposeView();
	std::map<View, std::set<MsgCommitResiBFT>>::iterator itView = this->commitsResiBFT.find(view);
	if (itView != this->commitsResiBFT.end())
	{
		std::set<MsgCommitResiBFT> commits = itView->second;
		commits.insert(msgCommit);
		this->commitsResiBFT[view] = commits;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "Stored MsgCommitResiBFT for view " << view
					  << " count=" << commits.size() << COLOUR_NORMAL << std::endl;
		}
		return commits.size();
	}
	else
	{
		std::set<MsgCommitResiBFT> commits = {msgCommit};
		this->commitsResiBFT[view] = commits;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "Created new MsgCommitResiBFT entry for view " << view << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
}

Signs Log::getMsgCommitResiBFT(View view, unsigned int n)
{
	Signs signs;
	std::map<View, std::set<MsgCommitResiBFT>>::iterator itView = this->commitsResiBFT.find(view);
	if (itView != this->commitsResiBFT.end())
	{
		std::set<MsgCommitResiBFT> msgCommits = itView->second;
		for (std::set<MsgCommitResiBFT>::iterator itMsg = msgCommits.begin(); signs.getSize() < n && itMsg != msgCommits.end(); itMsg++)
		{
			MsgCommitResiBFT msgCommit = *itMsg;
			Signs signs_MsgCommit = msgCommit.signs;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Adding signatures: " << signs_MsgCommit.toPrint() << COLOUR_NORMAL << std::endl;
			}
			signs.addUpto(signs_MsgCommit, n);
		}
	}
	if (DEBUG_LOG)
	{
		std::cout << COLOUR_GREEN << "Log signatures: " << signs.toPrint() << COLOUR_NORMAL << std::endl;
	}
	return signs;
}

// =====================================================
// ResiBFT-specific: VC (Verification Certificate) methods
// =====================================================

unsigned int Log::storeMsgVCResiBFT(MsgVCResiBFT msgVC)
{
	View vcView = msgVC.vc.getView();
	ReplicaID signer = msgVC.vc.getSigner();

	std::map<View, std::set<MsgVCResiBFT>>::iterator itView = this->vcsResiBFT.find(vcView);
	if (itView != this->vcsResiBFT.end())
	{
		std::set<MsgVCResiBFT> vcs = itView->second;
		// Check if already have VC from this signer
		bool exists = false;
		for (std::set<MsgVCResiBFT>::iterator itVC = vcs.begin(); itVC != vcs.end(); itVC++)
		{
			if ((*itVC).vc.getSigner() == signer && (*itVC).vc.getType() == msgVC.vc.getType())
			{
				exists = true;
				break;
			}
		}
		if (!exists)
		{
			vcs.insert(msgVC);
			this->vcsResiBFT[vcView] = vcs;
			if (DEBUG_LOG)
			{
				std::cout << COLOUR_GREEN << "Stored MsgVCResiBFT for view " << vcView
						  << " type " << printVCType(msgVC.vc.getType())
						  << " from signer " << signer
						  << " count=" << vcs.size() << COLOUR_NORMAL << std::endl;
			}
			return vcs.size();
		}
		return vcs.size();
	}
	else
	{
		std::set<MsgVCResiBFT> vcs = {msgVC};
		this->vcsResiBFT[vcView] = vcs;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "Created new MsgVCResiBFT entry for view " << vcView << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
}

std::set<MsgVCResiBFT> Log::getMsgVCResiBFT(View view)
{
	std::map<View, std::set<MsgVCResiBFT>>::iterator itView = this->vcsResiBFT.find(view);
	if (itView != this->vcsResiBFT.end())
	{
		return itView->second;
	}
	return std::set<MsgVCResiBFT>();
}

std::set<MsgVCResiBFT> Log::getMsgVCResiBFTByType(View view, VCType type)
{
	std::set<MsgVCResiBFT> result;
	std::set<MsgVCResiBFT> allVcs = getMsgVCResiBFT(view);
	for (std::set<MsgVCResiBFT>::iterator itVC = allVcs.begin(); itVC != allVcs.end(); itVC++)
	{
		if ((*itVC).vc.getType() == type)
		{
			result.insert(*itVC);
		}
	}
	return result;
}

bool Log::hasVCQuorum(View view, VCType type, unsigned int quorumSize)
{
	std::set<MsgVCResiBFT> vcs = getMsgVCResiBFTByType(view, type);
	return vcs.size() >= quorumSize;
}

// =====================================================
// ResiBFT-specific: BC (Block Certificate) methods
// =====================================================

unsigned int Log::storeMsgBCResiBFT(MsgBCResiBFT msgBC)
{
	View bcView = msgBC.blockCertificate.getView();

	std::map<View, std::set<MsgBCResiBFT>>::iterator itView = this->bcsResiBFT.find(bcView);
	if (itView != this->bcsResiBFT.end())
	{
		std::set<MsgBCResiBFT> bcs = itView->second;
		bcs.insert(msgBC);
		this->bcsResiBFT[bcView] = bcs;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "Stored MsgBCResiBFT for view " << bcView
					  << " count=" << bcs.size() << COLOUR_NORMAL << std::endl;
		}
		return bcs.size();
	}
	else
	{
		std::set<MsgBCResiBFT> bcs = {msgBC};
		this->bcsResiBFT[bcView] = bcs;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "Created new MsgBCResiBFT entry for view " << bcView << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
}

std::set<MsgBCResiBFT> Log::getMsgBCResiBFT(View view)
{
	std::map<View, std::set<MsgBCResiBFT>>::iterator itView = this->bcsResiBFT.find(view);
	if (itView != this->bcsResiBFT.end())
	{
		return itView->second;
	}
	return std::set<MsgBCResiBFT>();
}

bool Log::hasBCQuorum(View view, unsigned int quorumSize)
{
	std::set<MsgBCResiBFT> bcs = getMsgBCResiBFT(view);
	return bcs.size() >= quorumSize;
}

// =====================================================
// ResiBFT-specific: Committee methods
// =====================================================

unsigned int Log::storeMsgCommitteeResiBFT(MsgCommitteeResiBFT msgCommittee)
{
	View committeeView = msgCommittee.view;

	std::map<View, std::set<MsgCommitteeResiBFT>>::iterator itView = this->committeesResiBFT.find(committeeView);
	if (itView != this->committeesResiBFT.end())
	{
		std::set<MsgCommitteeResiBFT> committees = itView->second;
		committees.insert(msgCommittee);
		this->committeesResiBFT[committeeView] = committees;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "Stored MsgCommitteeResiBFT for view " << committeeView
					  << " count=" << committees.size() << COLOUR_NORMAL << std::endl;
		}
		return committees.size();
	}
	else
	{
		std::set<MsgCommitteeResiBFT> committees = {msgCommittee};
		this->committeesResiBFT[committeeView] = committees;
		if (DEBUG_LOG)
		{
			std::cout << COLOUR_GREEN << "Created new MsgCommitteeResiBFT entry for view " << committeeView << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
}

MsgCommitteeResiBFT Log::getMsgCommitteeResiBFT(View view)
{
	std::map<View, std::set<MsgCommitteeResiBFT>>::iterator itView = this->committeesResiBFT.find(view);
	if (itView != this->committeesResiBFT.end())
	{
		std::set<MsgCommitteeResiBFT> committees = itView->second;
		if (!committees.empty())
		{
			// Return the first MsgCommittee message
			return *committees.begin();
		}
	}
	// Return empty MsgCommittee if not found
	MsgCommitteeResiBFT emptyMsg;
	emptyMsg.committee = Group();
	emptyMsg.view = view;
	emptyMsg.signs = Signs();
	return emptyMsg;
}

Group Log::getCommitteeResiBFT(View view)
{
	std::map<View, std::set<MsgCommitteeResiBFT>>::iterator itView = this->committeesResiBFT.find(view);
	if (itView != this->committeesResiBFT.end())
	{
		std::set<MsgCommitteeResiBFT> committees = itView->second;
		if (!committees.empty())
		{
			// Return the committee from the first message (all should agree)
			return (*committees.begin()).committee;
		}
	}
	return Group();
}

bool Log::hasCommitteeQuorum(View view, unsigned int quorumSize)
{
	std::map<View, std::set<MsgCommitteeResiBFT>>::iterator itView = this->committeesResiBFT.find(view);
	if (itView != this->committeesResiBFT.end())
	{
		return itView->second.size() >= quorumSize;
	}
	return false;
}

// =====================================================
// ResiBFT-specific: Checkpoint methods
// =====================================================

unsigned int Log::storeCheckpoint(Checkpoint checkpoint)
{
	this->checkpoints.insert(checkpoint);
	if (DEBUG_LOG)
	{
		std::cout << COLOUR_GREEN << "Stored checkpoint for view " << checkpoint.getView()
				  << " count=" << this->checkpoints.size() << COLOUR_NORMAL << std::endl;
	}
	return this->checkpoints.size();
}

std::set<Checkpoint> Log::getCheckpoints()
{
	return this->checkpoints;
}

Checkpoint Log::getHighestCheckpoint()
{
	Checkpoint highest;
	for (std::set<Checkpoint>::iterator it = this->checkpoints.begin(); it != this->checkpoints.end(); it++)
	{
		if ((*it).getView() > highest.getView())
		{
			highest = *it;
		}
	}
	return highest;
}

std::string Log::toPrint()
{
	std::string text = "";

#if defined(BASIC_HOTSTUFF)
	// MsgNewviewHotstuff
	for (std::map<View, std::set<MsgNewviewHotstuff>>::iterator itView = this->newviewsHotstuff.begin(); itView != this->newviewsHotstuff.end(); itView++)
	{
		View view = itView->first;
		std::set<MsgNewviewHotstuff> msgs = itView->second;
		text += "MsgNewviewHotstuff: View = " + std::to_string(view) + "; The number of MsgNewviewHotstuff is: " + std::to_string(msgs.size()) + "\n";
	}
	// MsgLdrprepareHotstuff
	for (std::map<View, std::set<MsgLdrprepareHotstuff>>::iterator itView = this->ldrpreparesHotstuff.begin(); itView != this->ldrpreparesHotstuff.end(); itView++)
	{
		View view = itView->first;
		std::set<MsgLdrprepareHotstuff> msgs = itView->second;
		text += "MsgLdrprepareHotstuff: View = " + std::to_string(view) + "; The number of MsgLdrprepareHotstuff is: " + std::to_string(msgs.size()) + "\n";
	}
	// MsgPrepareHotstuff
	for (std::map<View, std::set<MsgPrepareHotstuff>>::iterator itView = this->preparesHotstuff.begin(); itView != this->preparesHotstuff.end(); itView++)
	{
		View view = itView->first;
		std::set<MsgPrepareHotstuff> msgs = itView->second;
		text += "MsgPrepareHotstuff: View = " + std::to_string(view) + "; The number of MsgPrepareHotstuff is: " + std::to_string(msgs.size()) + "\n";
	}
	// MsgPrecommitHotstuff
	for (std::map<View, std::set<MsgPrecommitHotstuff>>::iterator itView = this->precommitsHotstuff.begin(); itView != this->precommitsHotstuff.end(); itView++)
	{
		View view = itView->first;
		std::set<MsgPrecommitHotstuff> msgs = itView->second;
		text += "MsgPrecommitHotstuff: View = " + std::to_string(view) + "; The number of MsgPrecommitHotstuff is: " + std::to_string(msgs.size()) + "\n";
	}
	// MsgCommitHotstuff
	for (std::map<View, std::set<MsgCommitHotstuff>>::iterator itView = this->commitsHotstuff.begin(); itView != this->commitsHotstuff.end(); itView++)
	{
		View view = itView->first;
		std::set<MsgCommitHotstuff> msgs = itView->second;
		text += "MsgCommitHotstuff: View = " + std::to_string(view) + "; The number of MsgCommitHotstuff is: " + std::to_string(msgs.size()) + "\n";
	}
#elif defined(BASIC_DAMYSUS)
	// MsgNewviewDamysus
	for (std::map<View, std::set<MsgNewviewDamysus>>::iterator itView = this->newviewsDamysus.begin(); itView != this->newviewsDamysus.end(); itView++)
	{
		View view = itView->first;
		std::set<MsgNewviewDamysus> msgs = itView->second;
		text += "MsgNewviewDamysus: View = " + std::to_string(view) + "; The number of MsgNewviewDamysus is: " + std::to_string(msgs.size()) + "\n";
	}
	// MsgLdrprepareDamysus
	for (std::map<View, std::set<MsgLdrprepareDamysus>>::iterator itView = this->ldrpreparesDamysus.begin(); itView != this->ldrpreparesDamysus.end(); itView++)
	{
		View view = itView->first;
		std::set<MsgLdrprepareDamysus> msgs = itView->second;
		text += "MsgLdrprepareDamysus: View = " + std::to_string(view) + "; The number of MsgLdrprepareDamysus is: " + std::to_string(msgs.size()) + "\n";
	}
	// MsgPrepareDamysus
	for (std::map<View, std::set<MsgPrepareDamysus>>::iterator itView = this->preparesDamysus.begin(); itView != this->preparesDamysus.end(); itView++)
	{
		View view = itView->first;
		std::set<MsgPrepareDamysus> msgs = itView->second;
		text += "MsgPrepareDamysus: View = " + std::to_string(view) + "; The number of MsgPrepareDamysus is: " + std::to_string(msgs.size()) + "\n";
	}
	// MsgPrecommitDamysus
	for (std::map<View, std::set<MsgPrecommitDamysus>>::iterator itView = this->precommitsDamysus.begin(); itView != this->precommitsDamysus.end(); itView++)
	{
		View view = itView->first;
		std::set<MsgPrecommitDamysus> msgs = itView->second;
		text += "MsgPrecommitDamysus: View = " + std::to_string(view) + "; The number of MsgPrecommitDamysus is: " + std::to_string(msgs.size()) + "\n";
	}
#elif defined(BASIC_HOTSUS)
	// MsgNewviewHotsus
	for (std::map<View, std::set<MsgNewviewHotsus>>::iterator itView = this->newviewsHotsus.begin(); itView != this->newviewsHotsus.end(); itView++)
	{
		View view = itView->first;
		std::set<MsgNewviewHotsus> msgs = itView->second;
		text += "MsgNewviewHotsus: View = " + std::to_string(view) + "; The number of MsgNewviewHotsus is: " + std::to_string(msgs.size()) + "\n";
	}
	// MsgLdrprepareHotsus
	for (std::map<View, std::set<MsgLdrprepareHotsus>>::iterator itView = this->ldrpreparesHotsus.begin(); itView != this->ldrpreparesHotsus.end(); itView++)
	{
		View view = itView->first;
		std::set<MsgLdrprepareHotsus> msgs = itView->second;
		text += "MsgLdrprepareHotsus: View = " + std::to_string(view) + "; The number of MsgLdrprepareHotsus is: " + std::to_string(msgs.size()) + "\n";
	}
	// MsgPrepareHotsus
	for (std::map<View, std::set<MsgPrepareHotsus>>::iterator itView = this->preparesHotsus.begin(); itView != this->preparesHotsus.end(); itView++)
	{
		View view = itView->first;
		std::set<MsgPrepareHotsus> msgs = itView->second;
		text += "MsgPrepareHotsus: View = " + std::to_string(view) + "; The number of MsgPrepareHotsus is: " + std::to_string(msgs.size()) + "\n";
	}
	// MsgPrecommitHotsus
	for (std::map<View, std::set<MsgPrecommitHotsus>>::iterator itView = this->precommitsHotsus.begin(); itView != this->precommitsHotsus.end(); itView++)
	{
		View view = itView->first;
		std::set<MsgPrecommitHotsus> msgs = itView->second;
		text += "MsgPrecommitHotsus: View = " + std::to_string(view) + "; The number of MsgPrecommitHotsus is: " + std::to_string(msgs.size()) + "\n";
	}
	// MsgExnewviewHotsus
	for (std::map<View, std::set<MsgExnewviewHotsus>>::iterator itView = this->exnewviewsHotsus.begin(); itView != this->exnewviewsHotsus.end(); itView++)
	{
		View view = itView->first;
		std::set<MsgExnewviewHotsus> msgs = itView->second;
		text += "MsgExnewviewHotsus: View = " + std::to_string(view) + "; The number of MsgExnewviewHotsus is: " + std::to_string(msgs.size()) + "\n";
	}
	// MsgExldrprepareHotsus
	for (std::map<View, std::set<MsgExldrprepareHotsus>>::iterator itView = this->exldrpreparesHotsus.begin(); itView != this->exldrpreparesHotsus.end(); itView++)
	{
		View view = itView->first;
		std::set<MsgExldrprepareHotsus> msgs = itView->second;
		text += "MsgExldrprepareHotsus: View = " + std::to_string(view) + "; The number of MsgExldrprepareHotsus is: " + std::to_string(msgs.size()) + "\n";
	}
	// MsgExprepareHotsus
	for (std::map<View, std::set<MsgExprepareHotsus>>::iterator itView = this->expreparesHotsus.begin(); itView != this->expreparesHotsus.end(); itView++)
	{
		View view = itView->first;
		std::set<MsgExprepareHotsus> msgs = itView->second;
		text += "MsgExprepareHotsus: View = " + std::to_string(view) + "; The number of MsgExprepareHotsus is: " + std::to_string(msgs.size()) + "\n";
	}
	// MsgExprecommitHotsus
	for (std::map<View, std::set<MsgExprecommitHotsus>>::iterator itView = this->exprecommitsHotsus.begin(); itView != this->exprecommitsHotsus.end(); itView++)
	{
		View view = itView->first;
		std::set<MsgExprecommitHotsus> msgs = itView->second;
		text += "MsgExprecommitHotsus: View = " + std::to_string(view) + "; The number of MsgExprecommitHotsus is: " + std::to_string(msgs.size()) + "\n";
	}
	// MsgExcommitHotsus
	for (std::map<View, std::set<MsgExcommitHotsus>>::iterator itView = this->excommitsHotsus.begin(); itView != this->excommitsHotsus.end(); itView++)
	{
		View view = itView->first;
		std::set<MsgExcommitHotsus> msgs = itView->second;
		text += "MsgExcommitHotsus: View = " + std::to_string(view) + "; The number of MsgExcommitHotsus is: " + std::to_string(msgs.size()) + "\n";
	}
#elif defined(BASIC_RESIBFT)
	// MsgNewviewResiBFT
	for (std::map<View, std::set<MsgNewviewResiBFT>>::iterator itView = this->newviewsResiBFT.begin(); itView != this->newviewsResiBFT.end(); itView++)
	{
		View view = itView->first;
		std::set<MsgNewviewResiBFT> msgs = itView->second;
		text += "MsgNewviewResiBFT: View = " + std::to_string(view) + "; The number of MsgNewviewResiBFT is: " + std::to_string(msgs.size()) + "\n";
	}
	// MsgLdrprepareResiBFT
	for (std::map<View, std::set<MsgLdrprepareResiBFT>>::iterator itView = this->ldrpreparesResiBFT.begin(); itView != this->ldrpreparesResiBFT.end(); itView++)
	{
		View view = itView->first;
		std::set<MsgLdrprepareResiBFT> msgs = itView->second;
		text += "MsgLdrprepareResiBFT: View = " + std::to_string(view) + "; The number of MsgLdrprepareResiBFT is: " + std::to_string(msgs.size()) + "\n";
	}
	// MsgPrepareResiBFT
	for (std::map<View, std::set<MsgPrepareResiBFT>>::iterator itView = this->preparesResiBFT.begin(); itView != this->preparesResiBFT.end(); itView++)
	{
		View view = itView->first;
		std::set<MsgPrepareResiBFT> msgs = itView->second;
		text += "MsgPrepareResiBFT: View = " + std::to_string(view) + "; The number of MsgPrepareResiBFT is: " + std::to_string(msgs.size()) + "\n";
	}
	// MsgPrecommitResiBFT
	for (std::map<View, std::set<MsgPrecommitResiBFT>>::iterator itView = this->precommitsResiBFT.begin(); itView != this->precommitsResiBFT.end(); itView++)
	{
		View view = itView->first;
		std::set<MsgPrecommitResiBFT> msgs = itView->second;
		text += "MsgPrecommitResiBFT: View = " + std::to_string(view) + "; The number of MsgPrecommitResiBFT is: " + std::to_string(msgs.size()) + "\n";
	}
	// MsgDecideResiBFT
	for (std::map<View, std::set<MsgDecideResiBFT>>::iterator itView = this->decidesResiBFT.begin(); itView != this->decidesResiBFT.end(); itView++)
	{
		View view = itView->first;
		std::set<MsgDecideResiBFT> msgs = itView->second;
		text += "MsgDecideResiBFT: View = " + std::to_string(view) + "; The number of MsgDecideResiBFT is: " + std::to_string(msgs.size()) + "\n";
	}
	// MsgVCResiBFT
	for (std::map<View, std::set<MsgVCResiBFT>>::iterator itView = this->vcsResiBFT.begin(); itView != this->vcsResiBFT.end(); itView++)
	{
		View view = itView->first;
		std::set<MsgVCResiBFT> msgs = itView->second;
		text += "MsgVCResiBFT: View = " + std::to_string(view) + "; The number of MsgVCResiBFT is: " + std::to_string(msgs.size()) + "\n";
	}
	// MsgBCResiBFT
	for (std::map<View, std::set<MsgBCResiBFT>>::iterator itView = this->bcsResiBFT.begin(); itView != this->bcsResiBFT.end(); itView++)
	{
		View view = itView->first;
		std::set<MsgBCResiBFT> msgs = itView->second;
		text += "MsgBCResiBFT: View = " + std::to_string(view) + "; The number of MsgBCResiBFT is: " + std::to_string(msgs.size()) + "\n";
	}
	// MsgCommitteeResiBFT
	for (std::map<View, std::set<MsgCommitteeResiBFT>>::iterator itView = this->committeesResiBFT.begin(); itView != this->committeesResiBFT.end(); itView++)
	{
		View view = itView->first;
		std::set<MsgCommitteeResiBFT> msgs = itView->second;
		text += "MsgCommitteeResiBFT: View = " + std::to_string(view) + "; The number of MsgCommitteeResiBFT is: " + std::to_string(msgs.size()) + "\n";
	}
	// Checkpoints
	text += "Checkpoints: The number of Checkpoints is: " + std::to_string(this->checkpoints.size()) + "\n";
#endif

	return text;
}