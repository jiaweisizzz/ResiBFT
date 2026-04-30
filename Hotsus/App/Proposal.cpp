#include "Proposal.h"

template <class Certification>
void Proposal<Certification>::serialize(salticidae::DataStream &data) const
{
	data << this->certification << this->block;
}

template <class Certification>
void Proposal<Certification>::unserialize(salticidae::DataStream &data)
{
	data >> this->certification >> this->block;
}

template <class Certification>
Proposal<Certification>::Proposal() {}

template <class Certification>
Proposal<Certification>::Proposal(Certification certification, Block block)
{
	this->certification = certification;
	this->block = block;
}

template <class Certification>
Certification Proposal<Certification>::getCertification()
{
	return this->certification;
}

template <class Certification>
Block Proposal<Certification>::getBlock()
{
	return this->block;
}

template <class Certification>
std::string Proposal<Certification>::toPrint()
{
	std::string text = "";
	text += "PROPOSAL[";
	text += this->certification.toPrint();
	text += ",";
	text += this->block.toPrint();
	text += "]";
	return text;
}

template <class Certification>
std::string Proposal<Certification>::toString()
{
	std::string text = "";
	text += this->certification.toString();
	text += this->block.toString();
	return text;
}

template class Proposal<Justification>;
template class Proposal<Accumulator>;