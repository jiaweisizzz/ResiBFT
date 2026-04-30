#include "Accumulator.h"

void Accumulator::serialize(salticidae::DataStream &data) const
{
	data << this->set << this->proposeView << this->prepareHash << this->prepareView << this->size;
}

void Accumulator::unserialize(salticidae::DataStream &data)
{
	data >> this->set >> this->proposeView >> this->prepareHash >> this->prepareView >> this->size;
}

Accumulator::Accumulator()
{
	this->set = false;
}

Accumulator::Accumulator(View proposeView, Hash prepareHash, View prepareView, unsigned int size)
{
	this->set = true;
	this->proposeView = proposeView;
	this->prepareHash = prepareHash;
	this->prepareView = prepareView;
	this->size = size;
}

Accumulator::Accumulator(bool set, View proposeView, Hash prepareHash, View prepareView, unsigned int size)
{
	this->set = set;
	this->proposeView = proposeView;
	this->prepareHash = prepareHash;
	this->prepareView = prepareView;
	this->size = size;
}

Accumulator::Accumulator(salticidae::DataStream &data)
{
	unserialize(data);
}

bool Accumulator::isSet()
{
	return this->set;
}

View Accumulator::getProposeView()
{
	return this->proposeView;
}

Hash Accumulator::getPrepareHash()
{
	return this->prepareHash;
}

View Accumulator::getPrepareView()
{
	return this->prepareView;
}

unsigned int Accumulator::getSize()
{
	return this->size;
}

std::string Accumulator::toPrint()
{
	std::string text = "";
	text += "ACCUMULATOR[";
	text += std::to_string(this->set);
	text += ",";
	text += std::to_string(this->proposeView);
	text += ",";
	text += this->prepareHash.toPrint();
	text += ",";
	text += std::to_string(this->prepareView);
	text += ",";
	text += std::to_string(this->size);
	text += "]";
	return text;
}

std::string Accumulator::toString()
{
	std::string text = "";
	text += std::to_string(this->set);
	text += std::to_string(this->proposeView);
	text += this->prepareHash.toString();
	text += std::to_string(this->prepareView);
	text += std::to_string(this->size);
	return text;
}

bool Accumulator::operator==(const Accumulator &data) const
{
	if (this->set == data.set && this->proposeView == data.proposeView && this->prepareHash == data.prepareHash && this->prepareView == data.prepareView && this->size == data.size)
	{
		return true;
	}
	else
	{
		return false;
	}
}
