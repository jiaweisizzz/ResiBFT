#include "Justification.h"

void Justification::serialize(salticidae::DataStream &data) const
{
	data << this->set << this->roundData << this->signs;
}

void Justification::unserialize(salticidae::DataStream &data)
{
	data >> this->set >> this->roundData >> this->signs;
}

Justification::Justification()
{
	this->set = false;
	this->roundData = RoundData();
	this->signs = Signs();
}

Justification::Justification(RoundData roundData, Sign sign)
{
	this->set = true;
	this->roundData = roundData;
	this->signs = Signs(sign);
}

Justification::Justification(RoundData roundData, Signs signs)
{
	this->set = true;
	this->roundData = roundData;
	this->signs = signs;
}

Justification::Justification(bool b, RoundData roundData, Signs signs)
{
	this->set = b;
	this->roundData = roundData;
	this->signs = signs;
}

bool Justification::isSet()
{
	return this->set;
}

RoundData Justification::getRoundData()
{
	return this->roundData;
}

Signs Justification::getSigns()
{
	return this->signs;
}

bool Justification::wellFormedInitialize()
{
	if (this->roundData.getProposeView() == 0 && this->roundData.getJustifyView() == 0 && this->roundData.getPhase() == PHASE_NEWVIEW && this->signs.getSize() == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool Justification::wellFormedNewview()
{
	if (this->roundData.getJustifyHash().getSet() && this->roundData.getPhase() == PHASE_NEWVIEW && this->signs.getSize() == 1)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool Justification::wellFormedPrepare(unsigned int quorumSize)
{
	if (this->roundData.getProposeHash().getSet() && this->roundData.getPhase() == PHASE_PREPARE && this->signs.getSize() == quorumSize)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool Justification::wellFormed(unsigned int quorumSize)
{
	if (wellFormedInitialize() || wellFormedNewview() || wellFormedPrepare(quorumSize))
	{
		return true;
	}
	else
	{
		return false;
	}
}

View Justification::getCertificationView()
{
	if (wellFormedInitialize() || wellFormedNewview())
	{
		return roundData.getJustifyView();
	}
	else
	{
		return roundData.getProposeView();
	}
}

Hash Justification::getCertificationHash()
{
	if (wellFormedInitialize() || wellFormedNewview())
	{
		return roundData.getJustifyHash();
	}
	else
	{
		return roundData.getProposeHash();
	}
}

std::string Justification::toPrint()
{
	std::string text = "";
	text += "JUSTIFICATION[";
	text += std::to_string(this->set);
	text += ",";
	text += this->roundData.toPrint();
	text += ",";
	text += this->signs.toPrint();
	text += "]";
	return text;
}

std::string Justification::toString()
{
	std::string text = "";
	text += std::to_string(this->set);
	text += this->roundData.toString();
	text += this->signs.toString();
	return text;
}
