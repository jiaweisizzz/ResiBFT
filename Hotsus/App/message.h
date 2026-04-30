#ifndef MESSAGE_H
#define MESSAGE_H

#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "salticidae/msg.h"
#include "salticidae/stream.h"
#include "Accumulator.h"
#include "Group.h"
#include "Proposal.h"
#include "RoundData.h"
#include "Sign.h"
#include "Signs.h"
#include "Transaction.h"

// Client messages
struct MsgTransaction
{
	static const uint8_t opcode = HEADER_TRANSACTION;
	salticidae::DataStream serialized;
	Transaction transaction;
	Sign sign;

	MsgTransaction(const Transaction &transaction, const Sign &sign) : transaction(transaction), sign(sign) { serialized << transaction << sign; }
	MsgTransaction(salticidae::DataStream &&data) { data >> transaction >> sign; }

	void serialize(salticidae::DataStream &data) const
	{
		data << transaction << sign;
	}

	unsigned int sizeMsg()
	{
		return (sizeof(Transaction) + sizeof(Sign));
	}

	std::string toPrint()
	{
		std::string text = "";
		text += "MSGTRANSACTION[";
		text += transaction.toPrint();
		text += ",";
		text += sign.toPrint();
		text += "]";
		return text;
	}

	bool operator<(const MsgTransaction &data) const
	{
		if (sign < data.sign)
		{
			return true;
		}
		return false;
	}
};

struct MsgStart
{
	static const uint8_t opcode = HEADER_START;
	salticidae::DataStream serialized;
	ClientID clientId;
	Sign sign;

	MsgStart(const ClientID &clientId, const Sign &sign) : clientId(clientId), sign(sign) { serialized << clientId << sign; }
	MsgStart(salticidae::DataStream &&data) { data >> clientId >> sign; }

	void serialize(salticidae::DataStream &data) const
	{
		data << clientId << sign;
	}

	unsigned int sizeMsg()
	{
		return (sizeof(ClientID) + sizeof(Sign));
	}

	std::string toPrint()
	{
		std::string text = "";
		text += "MSGSTART[";
		text += std::to_string(clientId);
		text += ",";
		text += sign.toPrint();
		text += "]";
		return text;
	}

	bool operator<(const MsgStart &data) const
	{
		if (clientId < data.clientId)
		{
			return true;
		}
		return false;
	}
};

struct MsgReply
{
	static const uint8_t opcode = HEADER_REPLY;
	salticidae::DataStream serialized;
	unsigned int reply;

	MsgReply(const unsigned int &reply) : reply(reply) { serialized << reply; }
	MsgReply(salticidae::DataStream &&data) { data >> reply; }

	void serialize(salticidae::DataStream &data) const
	{
		data << reply;
	}

	unsigned int sizeMsg()
	{
		return (sizeof(unsigned int));
	}

	std::string toPrint()
	{
		std::string text = "";
		text += "MSGREPLY[";
		text += std::to_string(reply);
		text += "]";
		return text;
	}

	bool operator<(const MsgReply &data) const
	{
		if (reply < data.reply)
		{
			return true;
		}
		return false;
	}
};

// Basic Hotstuff
struct MsgNewviewHotstuff
{
	static const uint8_t opcode = HEADER_NEWVIEW_HOTSTUFF;
	salticidae::DataStream serialized;
	RoundData roundData;
	Signs signs;

	MsgNewviewHotstuff(const RoundData &roundData, const Signs &signs) : roundData(roundData), signs(signs) { serialized << roundData << signs; }
	MsgNewviewHotstuff(salticidae::DataStream &&data) { data >> roundData >> signs; }

	void serialize(salticidae::DataStream &data) const
	{
		data << roundData << signs;
	}

	unsigned int sizeMsg()
	{
		return (sizeof(RoundData) + sizeof(Signs));
	}

	std::string toPrint()
	{
		std::string text = "";
		text += "HOTSTUFF_MSGNEWVIEW[";
		text += roundData.toPrint();
		text += ",";
		text += signs.toPrint();
		text += "]";
		return text;
	}

	bool operator<(const MsgNewviewHotstuff &data) const
	{
		if (signs < data.signs)
		{
			return true;
		}
		return false;
	}
};

struct MsgLdrprepareHotstuff
{
	static const uint8_t opcode = HEADER_LDRPREPARE_HOTSTUFF;
	salticidae::DataStream serialized;
	Proposal<Justification> proposal;
	Signs signs;

	MsgLdrprepareHotstuff(const Proposal<Justification> &proposal, const Signs &signs) : proposal(proposal), signs(signs) { serialized << proposal << signs; }
	MsgLdrprepareHotstuff(salticidae::DataStream &&data) { data >> proposal >> signs; }

	void serialize(salticidae::DataStream &data) const
	{
		data << proposal << signs;
	}

	unsigned int sizeMsg()
	{
		return (sizeof(Proposal<Justification>) + sizeof(Signs));
	}

	std::string toPrint()
	{
		std::string text = "";
		text += "HOTSTUFF_MSGLDRPREPARE[";
		text += proposal.toPrint();
		text += ",";
		text += signs.toPrint();
		text += "]";
		return text;
	}

	bool operator<(const MsgLdrprepareHotstuff &data) const
	{
		if (signs < data.signs)
		{
			return true;
		}
		return false;
	}
};

struct MsgPrepareHotstuff
{
	static const uint8_t opcode = HEADER_PREPARE_HOTSTUFF;
	salticidae::DataStream serialized;
	RoundData roundData;
	Signs signs;

	MsgPrepareHotstuff(const RoundData &roundData, const Signs &signs) : roundData(roundData), signs(signs) { serialized << roundData << signs; }
	MsgPrepareHotstuff(salticidae::DataStream &&data) { data >> roundData >> signs; }

	void serialize(salticidae::DataStream &data) const
	{
		data << roundData << signs;
	}

	unsigned int sizeMsg()
	{
		return (sizeof(RoundData) + sizeof(Signs));
	}

	std::string toPrint()
	{
		std::string text = "";
		text += "HOTSTUFF_MSGPREPARE[";
		text += roundData.toPrint();
		text += ",";
		text += signs.toPrint();
		text += "]";
		return text;
	}

	bool operator<(const MsgPrepareHotstuff &data) const
	{
		if (signs < data.signs)
		{
			return true;
		}
		return false;
	}
};

struct MsgPrecommitHotstuff
{
	static const uint8_t opcode = HEADER_PRECOMMIT_HOTSTUFF;
	salticidae::DataStream serialized;
	RoundData roundData;
	Signs signs;

	MsgPrecommitHotstuff(const RoundData &roundData, const Signs &signs) : roundData(roundData), signs(signs) { serialized << roundData << signs; }
	MsgPrecommitHotstuff(salticidae::DataStream &&data) { data >> roundData >> signs; }

	void serialize(salticidae::DataStream &data) const
	{
		data << roundData << signs;
	}

	unsigned int sizeMsg()
	{
		return (sizeof(RoundData) + sizeof(Signs));
	}

	std::string toPrint()
	{
		std::string text = "";
		text += "HOTSTUFF_MSGPRECOMMIT[";
		text += roundData.toPrint();
		text += ",";
		text += signs.toPrint();
		text += "]";
		return text;
	}

	bool operator<(const MsgPrecommitHotstuff &data) const
	{
		if (signs < data.signs)
		{
			return true;
		}
		return false;
	}
};

struct MsgCommitHotstuff
{
	static const uint8_t opcode = HEADER_COMMIT_HOTSTUFF;
	salticidae::DataStream serialized;
	RoundData roundData;
	Signs signs;

	MsgCommitHotstuff(const RoundData &roundData, const Signs &signs) : roundData(roundData), signs(signs) { serialized << roundData << signs; }
	MsgCommitHotstuff(salticidae::DataStream &&data) { data >> roundData >> signs; }

	void serialize(salticidae::DataStream &data) const
	{
		data << roundData << signs;
	}

	unsigned int sizeMsg()
	{
		return (sizeof(RoundData) + sizeof(Signs));
	}

	std::string toPrint()
	{
		std::string text = "";
		text += "HOTSTUFF_MSGCOMMIT[";
		text += roundData.toPrint();
		text += ",";
		text += signs.toPrint();
		text += "]";
		return text;
	}

	bool operator<(const MsgCommitHotstuff &data) const
	{
		if (signs < data.signs)
		{
			return true;
		}
		return false;
	}
};

// Basic Damysus
struct MsgNewviewDamysus
{
	static const uint8_t opcode = HEADER_NEWVIEW_DAMYSUS;
	salticidae::DataStream serialized;
	RoundData roundData;
	Signs signs;

	MsgNewviewDamysus(const RoundData &roundData, const Signs &signs) : roundData(roundData), signs(signs) { serialized << roundData << signs; }
	MsgNewviewDamysus(salticidae::DataStream &&data) { data >> roundData >> signs; }

	void serialize(salticidae::DataStream &data) const
	{
		data << roundData << signs;
	}

	unsigned int sizeMsg()
	{
		return (sizeof(RoundData) + sizeof(Signs));
	}

	std::string toPrint()
	{
		std::string text = "";
		text += "DAMYSUS_MSGNEWVIEW[";
		text += roundData.toPrint();
		text += ",";
		text += signs.toPrint();
		text += "]";
		return text;
	}

	bool operator<(const MsgNewviewDamysus &data) const
	{
		if (signs < data.signs)
		{
			return true;
		}
		return false;
	}
};

struct MsgLdrprepareDamysus
{
	static const uint8_t opcode = HEADER_LDRPREPARE_DAMYSUS;
	salticidae::DataStream serialized;
	Proposal<Accumulator> proposal;
	Signs signs;

	MsgLdrprepareDamysus(const Proposal<Accumulator> &proposal, const Signs &signs) : proposal(proposal), signs(signs) { serialized << proposal << signs; }
	MsgLdrprepareDamysus(salticidae::DataStream &&data) { data >> proposal >> signs; }

	void serialize(salticidae::DataStream &data) const
	{
		data << proposal << signs;
	}

	unsigned int sizeMsg()
	{
		return (sizeof(Proposal<Accumulator>) + sizeof(Signs));
	}

	std::string toPrint()
	{
		std::string text = "";
		text += "DAMYSUS_MSGLDRPREPARE[";
		text += proposal.toPrint();
		text += ",";
		text += signs.toPrint();
		text += "]";
		return text;
	}

	bool operator<(const MsgLdrprepareDamysus &data) const
	{
		if (signs < data.signs)
		{
			return true;
		}
		return false;
	}
};

struct MsgPrepareDamysus
{
	static const uint8_t opcode = HEADER_PREPARE_DAMYSUS;
	salticidae::DataStream serialized;
	RoundData roundData;
	Signs signs;

	MsgPrepareDamysus(const RoundData &roundData, const Signs &signs) : roundData(roundData), signs(signs) { serialized << roundData << signs; }
	MsgPrepareDamysus(salticidae::DataStream &&data) { data >> roundData >> signs; }

	void serialize(salticidae::DataStream &data) const
	{
		data << roundData << signs;
	}

	unsigned int sizeMsg()
	{
		return (sizeof(RoundData) + sizeof(Signs));
	}

	std::string toPrint()
	{
		std::string text = "";
		text += "DAMYSUS_MSGPREPARE[";
		text += roundData.toPrint();
		text += ",";
		text += signs.toPrint();
		text += "]";
		return text;
	}

	bool operator<(const MsgPrepareDamysus &data) const
	{
		if (signs < data.signs)
		{
			return true;
		}
		return false;
	}
};

struct MsgPrecommitDamysus
{
	static const uint8_t opcode = HEADER_PRECOMMIT_DAMYSUS;
	salticidae::DataStream serialized;
	RoundData roundData;
	Signs signs;

	MsgPrecommitDamysus(const RoundData &roundData, const Signs &signs) : roundData(roundData), signs(signs) { serialized << roundData << signs; }
	MsgPrecommitDamysus(salticidae::DataStream &&data) { data >> roundData >> signs; }

	void serialize(salticidae::DataStream &data) const
	{
		data << roundData << signs;
	}

	unsigned int sizeMsg()
	{
		return (sizeof(RoundData) + sizeof(Signs));
	}

	std::string toPrint()
	{
		std::string text = "";
		text += "DAMYSUS_MSGPRECOMMIT[";
		text += roundData.toPrint();
		text += ",";
		text += signs.toPrint();
		text += "]";
		return text;
	}

	bool operator<(const MsgPrecommitDamysus &data) const
	{
		if (signs < data.signs)
		{
			return true;
		}
		return false;
	}
};

// Basic Hotsus
struct MsgNewviewHotsus
{
	static const uint8_t opcode = HEADER_NEWVIEW_HOTSUS;
	salticidae::DataStream serialized;
	RoundData roundData;
	Signs signs;

	MsgNewviewHotsus(const RoundData &roundData, const Signs &signs) : roundData(roundData), signs(signs) { serialized << roundData << signs; }
	MsgNewviewHotsus(salticidae::DataStream &&data) { data >> roundData >> signs; }

	void serialize(salticidae::DataStream &data) const
	{
		data << roundData << signs;
	}

	unsigned int sizeMsg()
	{
		return (sizeof(RoundData) + sizeof(Signs));
	}

	std::string toPrint()
	{
		std::string text = "";
		text += "HOTSUS_MSGNEWVIEW[";
		text += roundData.toPrint();
		text += ",";
		text += signs.toPrint();
		text += "]";
		return text;
	}

	bool operator<(const MsgNewviewHotsus &data) const
	{
		if (signs < data.signs)
		{
			return true;
		}
		return false;
	}
};

struct MsgLdrprepareHotsus
{
	static const uint8_t opcode = HEADER_LDRPREPARE_HOTSUS;
	salticidae::DataStream serialized;
	Proposal<Accumulator> proposal;
	Signs signs;

	MsgLdrprepareHotsus(const Proposal<Accumulator> &proposal, const Signs &signs) : proposal(proposal), signs(signs) { serialized << proposal << signs; }
	MsgLdrprepareHotsus(salticidae::DataStream &&data) { data >> proposal >> signs; }

	void serialize(salticidae::DataStream &data) const
	{
		data << proposal << signs;
	}

	unsigned int sizeMsg()
	{
		return (sizeof(Proposal<Accumulator>) + sizeof(Signs));
	}

	std::string toPrint()
	{
		std::string text = "";
		text += "HOTSUS_MSGLDRPREPARE[";
		text += proposal.toPrint();
		text += ",";
		text += signs.toPrint();
		text += "]";
		return text;
	}

	bool operator<(const MsgLdrprepareHotsus &data) const
	{
		if (signs < data.signs)
		{
			return true;
		}
		return false;
	}
};

struct MsgPrepareHotsus
{
	static const uint8_t opcode = HEADER_PREPARE_HOTSUS;
	salticidae::DataStream serialized;
	RoundData roundData;
	Signs signs;

	MsgPrepareHotsus(const RoundData &roundData, const Signs &signs) : roundData(roundData), signs(signs) { serialized << roundData << signs; }
	MsgPrepareHotsus(salticidae::DataStream &&data) { data >> roundData >> signs; }

	void serialize(salticidae::DataStream &data) const
	{
		data << roundData << signs;
	}

	unsigned int sizeMsg()
	{
		return (sizeof(RoundData) + sizeof(Signs));
	}

	std::string toPrint()
	{
		std::string text = "";
		text += "HOTSUS_MSGPREPARE[";
		text += roundData.toPrint();
		text += ",";
		text += signs.toPrint();
		text += "]";
		return text;
	}

	bool operator<(const MsgPrepareHotsus &data) const
	{
		if (signs < data.signs)
		{
			return true;
		}
		return false;
	}
};

struct MsgPrecommitHotsus
{
	static const uint8_t opcode = HEADER_PRECOMMIT_HOTSUS;
	salticidae::DataStream serialized;
	RoundData roundData;
	Signs signs;

	MsgPrecommitHotsus(const RoundData &roundData, const Signs &signs) : roundData(roundData), signs(signs) { serialized << roundData << signs; }
	MsgPrecommitHotsus(salticidae::DataStream &&data) { data >> roundData >> signs; }

	void serialize(salticidae::DataStream &data) const
	{
		data << roundData << signs;
	}

	unsigned int sizeMsg()
	{
		return (sizeof(RoundData) + sizeof(Signs));
	}

	std::string toPrint()
	{
		std::string text = "";
		text += "HOTSUS_MSGPRECOMMIT[";
		text += roundData.toPrint();
		text += ",";
		text += signs.toPrint();
		text += "]";
		return text;
	}

	bool operator<(const MsgPrecommitHotsus &data) const
	{
		if (signs < data.signs)
		{
			return true;
		}
		return false;
	}
};

struct MsgExnewviewHotsus
{
	static const uint8_t opcode = HEADER_EXNEWVIEW_HOTSUS;
	salticidae::DataStream serialized;
	RoundData roundData;
	Signs signs;

	MsgExnewviewHotsus(const RoundData &roundData, const Signs &signs) : roundData(roundData), signs(signs) { serialized << roundData << signs; }
	MsgExnewviewHotsus(salticidae::DataStream &&data) { data >> roundData >> signs; }

	void serialize(salticidae::DataStream &data) const
	{
		data << roundData << signs;
	}

	unsigned int sizeMsg()
	{
		return (sizeof(RoundData) + sizeof(Signs));
	}

	std::string toPrint()
	{
		std::string text = "";
		text += "HOTSUS_MSGEXNEWVIEW[";
		text += roundData.toPrint();
		text += ",";
		text += signs.toPrint();
		text += "]";
		return text;
	}

	bool operator<(const MsgExnewviewHotsus &data) const
	{
		if (signs < data.signs)
		{
			return true;
		}
		return false;
	}
};

struct MsgExldrprepareHotsus
{
	static const uint8_t opcode = HEADER_EXLDRPREPARE_HOTSUS;
	salticidae::DataStream serialized;
	Proposal<Justification> proposal;
	Group group;
	Signs signs;

	MsgExldrprepareHotsus(const Proposal<Justification> &proposal, const Group &group, const Signs &signs) : proposal(proposal), group(group), signs(signs) { serialized << proposal << group << signs; }
	MsgExldrprepareHotsus(salticidae::DataStream &&data) { data >> proposal >> group >> signs; }

	void serialize(salticidae::DataStream &data) const
	{
		data << proposal << group << signs;
	}

	unsigned int sizeMsg()
	{
		return (sizeof(Proposal<Justification>) + sizeof(Group) + sizeof(Signs));
	}

	std::string toPrint()
	{
		std::string text = "";
		text += "HOTSUS_MSGEXLDRPREPARE[";
		text += proposal.toPrint();
		text += ",";
		text += group.toPrint();
		text += ",";
		text += signs.toPrint();
		text += "]";
		return text;
	}

	bool operator<(const MsgExldrprepareHotsus &data) const
	{
		if (signs < data.signs)
		{
			return true;
		}
		return false;
	}
};

struct MsgExprepareHotsus
{
	static const uint8_t opcode = HEADER_EXPREPARE_HOTSUS;
	salticidae::DataStream serialized;
	RoundData roundData;
	Signs signs;

	MsgExprepareHotsus(const RoundData &roundData, const Signs &signs) : roundData(roundData), signs(signs) { serialized << roundData << signs; }
	MsgExprepareHotsus(salticidae::DataStream &&data) { data >> roundData >> signs; }

	void serialize(salticidae::DataStream &data) const
	{
		data << roundData << signs;
	}

	unsigned int sizeMsg()
	{
		return (sizeof(RoundData) + sizeof(Signs));
	}

	std::string toPrint()
	{
		std::string text = "";
		text += "HOTSUS_MSGEXPREPARE[";
		text += roundData.toPrint();
		text += ",";
		text += signs.toPrint();
		text += "]";
		return text;
	}

	bool operator<(const MsgExprepareHotsus &data) const
	{
		if (signs < data.signs)
		{
			return true;
		}
		return false;
	}
};

struct MsgExprecommitHotsus
{
	static const uint8_t opcode = HEADER_EXPRECOMMIT_HOTSUS;
	salticidae::DataStream serialized;
	RoundData roundData;
	Signs signs;

	MsgExprecommitHotsus(const RoundData &roundData, const Signs &signs) : roundData(roundData), signs(signs) { serialized << roundData << signs; }
	MsgExprecommitHotsus(salticidae::DataStream &&data) { data >> roundData >> signs; }

	void serialize(salticidae::DataStream &data) const
	{
		data << roundData << signs;
	}

	unsigned int sizeMsg()
	{
		return (sizeof(RoundData) + sizeof(Signs));
	}

	std::string toPrint()
	{
		std::string text = "";
		text += "HOTSUS_MSGEXPRECOMMIT[";
		text += roundData.toPrint();
		text += ",";
		text += signs.toPrint();
		text += "]";
		return text;
	}

	bool operator<(const MsgExprecommitHotsus &data) const
	{
		if (signs < data.signs)
		{
			return true;
		}
		return false;
	}
};

struct MsgExcommitHotsus
{
	static const uint8_t opcode = HEADER_EXCOMMIT_HOTSUS;
	salticidae::DataStream serialized;
	RoundData roundData;
	Signs signs;

	MsgExcommitHotsus(const RoundData &roundData, const Signs &signs) : roundData(roundData), signs(signs) { serialized << roundData << signs; }
	MsgExcommitHotsus(salticidae::DataStream &&data) { data >> roundData >> signs; }

	void serialize(salticidae::DataStream &data) const
	{
		data << roundData << signs;
	}

	unsigned int sizeMsg()
	{
		return (sizeof(RoundData) + sizeof(Signs));
	}

	std::string toPrint()
	{
		std::string text = "";
		text += "HOTSUS_MSGEXCOMMIT[";
		text += roundData.toPrint();
		text += ",";
		text += signs.toPrint();
		text += "]";
		return text;
	}

	bool operator<(const MsgExcommitHotsus &data) const
	{
		if (signs < data.signs)
		{
			return true;
		}
		return false;
	}
};

#endif