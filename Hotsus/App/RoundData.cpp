#include "RoundData.h"

void RoundData::serialize(salticidae::DataStream &data) const
{
	data << this->proposeHash << this->proposeView << this->justifyHash << this->justifyView << this->phase;
}

void RoundData::unserialize(salticidae::DataStream &data)
{
	data >> this->proposeHash >> this->proposeView >> this->justifyHash >> this->justifyView >> this->phase;
}

RoundData::RoundData()
{
	this->proposeHash = Hash();
	this->proposeView = 0;
	this->justifyHash = Hash();
	this->justifyView = 0;
	this->phase = PHASE_NEWVIEW;
}

RoundData::RoundData(Hash proposeHash, View proposeView, Hash justifyHash, View justifyView, Phase phase)
{
	this->proposeHash = proposeHash;
	this->proposeView = proposeView;
	this->justifyHash = justifyHash;
	this->justifyView = justifyView;
	this->phase = phase;
}

RoundData::RoundData(salticidae::DataStream &data)
{
	unserialize(data);
}

Hash RoundData::getProposeHash()
{
	return this->proposeHash;
}

View RoundData::getProposeView()
{
	return this->proposeView;
}

Hash RoundData::getJustifyHash()
{
	return this->justifyHash;
}

View RoundData::getJustifyView()
{
	return this->justifyView;
}

Phase RoundData::getPhase()
{
	return this->phase;
}

std::string phase2string(Phase phase)
{
	if (phase == PHASE_NEWVIEW)
	{
		return "PHASE_NEWVIEW";
	}
	else if (phase == PHASE_PREPARE)
	{
		return "PHASE_PREPARE";
	}
	else if (phase == PHASE_PRECOMMIT)
	{
		return "PHASE_PRECOMMIT";
	}
	else if (phase == PHASE_COMMIT)
	{
		return "PHASE_COMMIT";
	}
	else if (phase == PHASE_EXNEWVIEW)
	{
		return "PHASE_EXNEWVIEW";
	}
	else if (phase == PHASE_EXPREPARE)
	{
		return "PHASE_EXPREPARE";
	}
	else if (phase == PHASE_EXPRECOMMIT)
	{
		return "PHASE_EXPRECOMMIT";
	}
	else if (phase == PHASE_EXCOMMIT)
	{
		return "PHASE_EXCOMMIT";
	}
	else
	{
		return "";
	}
	return "";
}

std::string RoundData::toPrint()
{
	std::string text = "";
	text += "ROUNDDATA[";
	text += this->proposeHash.toPrint();
	text += ",";
	text += std::to_string(this->proposeView);
	text += ",";
	text += this->justifyHash.toPrint();
	text += ",";
	text += std::to_string(this->justifyView);
	text += ",";
	text += phase2string(this->phase);
	text += "]";
	return text;
}

std::string RoundData::toString()
{
	std::string text = "";
	text += this->proposeHash.toString();
	text += std::to_string(this->proposeView);
	text += this->justifyHash.toString();
	text += std::to_string(this->justifyView);
	text += std::to_string(this->phase);
	return text;
}

bool RoundData::operator==(const RoundData &data) const
{
	if (this->proposeHash == data.proposeHash && this->proposeView == data.proposeView && this->justifyHash == data.justifyHash && this->justifyView == data.justifyView && this->phase == data.phase)
	{
		return true;
	}
	else
	{
		return false;
	}
}
