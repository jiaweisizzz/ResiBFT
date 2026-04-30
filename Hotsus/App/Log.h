#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <map>
#include <set>
#include <stdint.h>
#include <stdio.h>
#include "config.h"
#include "message.h"
#include "Justification.h"
#include "Proposal.h"

class Log
{
private:
	// Basic Hotstuff
	std::map<View, std::set<MsgNewviewHotstuff>> newviewsHotstuff;
	std::map<View, std::set<MsgLdrprepareHotstuff>> ldrpreparesHotstuff;
	std::map<View, std::set<MsgPrepareHotstuff>> preparesHotstuff;
	std::map<View, std::set<MsgPrecommitHotstuff>> precommitsHotstuff;
	std::map<View, std::set<MsgCommitHotstuff>> commitsHotstuff;

	// Basic Damysus
	std::map<View, std::set<MsgNewviewDamysus>> newviewsDamysus;
	std::map<View, std::set<MsgLdrprepareDamysus>> ldrpreparesDamysus;
	std::map<View, std::set<MsgPrepareDamysus>> preparesDamysus;
	std::map<View, std::set<MsgPrecommitDamysus>> precommitsDamysus;

	// Basic Hotsus
	std::map<View, std::set<MsgNewviewHotsus>> newviewsHotsus;
	std::map<View, std::set<MsgLdrprepareHotsus>> ldrpreparesHotsus;
	std::map<View, std::set<MsgPrepareHotsus>> preparesHotsus;
	std::map<View, std::set<MsgPrecommitHotsus>> precommitsHotsus;
	std::map<View, std::set<MsgExnewviewHotsus>> exnewviewsHotsus;
	std::map<View, std::set<MsgExldrprepareHotsus>> exldrpreparesHotsus;
	std::map<View, std::set<MsgExprepareHotsus>> expreparesHotsus;
	std::map<View, std::set<MsgExprecommitHotsus>> exprecommitsHotsus;
	std::map<View, std::set<MsgExcommitHotsus>> excommitsHotsus;

public:
	Log();

	// Basic Hotstuff
	// Return the number of signatures
	unsigned int storeMsgNewviewHotstuff(MsgNewviewHotstuff msgNewview);
	unsigned int storeMsgLdrprepareHotstuff(MsgLdrprepareHotstuff msgLdrprepare);
	unsigned int storeMsgPrepareHotstuff(MsgPrepareHotstuff msgPrepare);
	unsigned int storeMsgPrecommitHotstuff(MsgPrecommitHotstuff msgPrecommit);
	unsigned int storeMsgCommitHotstuff(MsgCommitHotstuff msgCommit);

	// Collect [n] signatures of the messages
	Signs getMsgNewviewHotstuff(View view, unsigned int n);
	Signs getMsgPrepareHotstuff(View view, unsigned int n);
	Signs getMsgPrecommitHotstuff(View view, unsigned int n);
	Signs getMsgCommitHotstuff(View view, unsigned int n);

	// Find the justification of the highest message
	Justification findHighestMsgNewviewHotstuff(View view);

	// Find the first justification of the all messages
	MsgLdrprepareHotstuff firstMsgLdrprepareHotstuff(View view);
	Justification firstMsgPrepareHotstuff(View view);
	Justification firstMsgPrecommitHotstuff(View view);
	Justification firstMsgCommitHotstuff(View view);

	// Basic Damysus
	// Return the number of signatures
	unsigned int storeMsgNewviewDamysus(MsgNewviewDamysus msgNewview);
	unsigned int storeMsgLdrprepareDamysus(MsgLdrprepareDamysus msgLdrprepare);
	unsigned int storeMsgPrepareDamysus(MsgPrepareDamysus msgPrepare);
	unsigned int storeMsgPrecommitDamysus(MsgPrecommitDamysus msgPrecommit);

	// Collect [n] signatures of the messages
	std::set<MsgNewviewDamysus> getMsgNewviewDamysus(View view, unsigned int n);
	Signs getMsgPrepareDamysus(View view, unsigned int n);
	Signs getMsgPrecommitDamysus(View view, unsigned int n);

	// Find the first message
	MsgLdrprepareDamysus firstMsgLdrprepareDamysus(View view);
	Justification firstMsgPrepareDamysus(View view);
	Justification firstMsgPrecommitDamysus(View view);

	// Basic Hotsus
	// Return the number of signatures
	unsigned int storeMsgNewviewHotsus(MsgNewviewHotsus msgNewview);
	unsigned int storeMsgLdrprepareHotsus(MsgLdrprepareHotsus msgLdrprepare);
	unsigned int storeMsgPrepareHotsus(MsgPrepareHotsus msgPrepare);
	unsigned int storeMsgPrecommitHotsus(MsgPrecommitHotsus msgPrecommit);
	unsigned int storeMsgExnewviewHotsus(MsgExnewviewHotsus msgExnewview);
	unsigned int storeMsgExldrprepareHotsus(MsgExldrprepareHotsus msgExldrprepare);
	unsigned int storeMsgExprepareHotsus(MsgExprepareHotsus msgExprepare);
	unsigned int storeMsgExprecommitHotsus(MsgExprecommitHotsus msgExprecommit);
	unsigned int storeMsgExcommitHotsus(MsgExcommitHotsus msgExcommit);

	// Collect [n] signatures of the messages
	std::set<MsgNewviewHotsus> getMsgNewviewHotsus(View view, unsigned int n);
	Signs getMsgPrepareHotsus(View view, unsigned int n);
	Signs getMsgPrecommitHotsus(View view, unsigned int n);
	Signs getMsgExnewviewHotsus(View view, unsigned int n);
	Signs getMsgExprepareHotsus(View view, unsigned int n);
	Signs getMsgExprecommitHotsus(View view, unsigned int n);
	Signs getMsgExcommitHotsus(View view, unsigned int n);

    // Get the trusted sender of the messages
	std::vector<ReplicaID> getTrustedMsgExnewviewHotsus(View view, unsigned int n);

	// Find the justification of the highest message
	Justification findHighestMsgExnewviewHotsus(View view);

	// Find the first message
	MsgLdrprepareHotsus firstMsgLdrprepareHotsus(View view);
	Justification firstMsgPrepareHotsus(View view);
	Justification firstMsgPrecommitHotsus(View view);
	MsgExldrprepareHotsus firstMsgExldrprepareHotsus(View view);
	Justification firstMsgExprepareHotsus(View view);
	Justification firstMsgExprecommitHotsus(View view);
	Justification firstMsgExcommitHotsus(View view);

	// Print
	std::string toPrint();
};

#endif
