#include "Signs.h"

void Signs::serialize(salticidae::DataStream &data) const
{
	data << this->size;
	for (int i = 0; i < MAX_NUM_SIGNATURES; i++)
	{
		data << this->signs[i];
	}
}

void Signs::unserialize(salticidae::DataStream &data)
{
	data >> this->size;
	for (int i = 0; i < MAX_NUM_SIGNATURES; i++)
	{
		data >> this->signs[i];
	}
}

Signs::Signs()
{
	this->size = 0;
	for (int i = 0; i < MAX_NUM_SIGNATURES; i++)
	{
		this->signs[i] = Sign();
	}
}

Signs::Signs(Sign sign)
{
	this->size = 1;
	this->signs[0] = sign;
}

Signs::Signs(unsigned int size, Sign signs[MAX_NUM_SIGNATURES])
{
	this->size = size;
	for (int i = 0; i < MAX_NUM_SIGNATURES; i++)
	{
		this->signs[i] = signs[i];
	}
}

Signs::Signs(salticidae::DataStream &data)
{
	unserialize(data);
}

unsigned int Signs::getSize()
{
	return this->size;
}

Sign Signs::get(unsigned int n)
{
	return this->signs[n];
}

void Signs::add(Sign sign)
{
	this->signs[this->size] = sign;
	this->size++;
}

void Signs::addUpto(Signs signs, unsigned int n)
{
	for (int i = 0; i < signs.getSize() && this->size < n; i++)
	{
		this->add(signs.get(i));
	}
}

bool Signs::hasSigned(ReplicaID replicaId)
{
	for (int i = 0; i < this->size; i++)
	{
		if (this->signs[i].getSigner() == replicaId)
		{
			return true;
		}
	}
	return false;
}

bool Signs::verify(ReplicaID id, Nodes nodes, std::string text)
{
	for (int i = 0; i < this->size; i++)
	{
		Sign sign = this->signs[i];
		ReplicaID replicaId = sign.getSigner();
		Node *node = nodes.find(replicaId);
		if (node)
		{
			// The id of the signer should also be added to the string
			bool b = sign.verify(node->getPublicKey(), text);
			if (b)
			{
				if (DEBUG_MODULES)
				{
					std::cout << COLOUR_CYAN << "Successfully verified signature from " << node->getReplicaId() << COLOUR_NORMAL << std::endl;
				}
			}
			else
			{
				if (DEBUG_MODULES)
				{
					std::cout << COLOUR_CYAN << "Failed to verify signature from " << node->getReplicaId() << COLOUR_NORMAL << std::endl;
				}
				return false;
			}
		}
		else
		{
			if (DEBUG_MODULES)
			{
				std::cout << COLOUR_CYAN << "Couldn't get information for " << replicaId << COLOUR_NORMAL << std::endl;
			}
			return false;
		}
	}
	return true;
}

std::set<ReplicaID> Signs::getSigners()
{
	std::set<ReplicaID> replicaIds;
	for (int i = 0; i < this->size; i++)
	{
		replicaIds.insert(this->signs[i].getSigner());
	}
	return replicaIds;
}

std::string Signs::printSigners()
{
	std::string text = "-";
	for (int i = 0; i < this->size; i++)
	{
		text += this->signs[i].getSigner() + "-";
	}
	return text;
}

std::string Signs::toPrint()
{
	std::string textSigns = "";
	for (int i = 0; i < this->size; i++)
	{
		textSigns += signs[i].toPrint();
	}
	std::string text = "";
	text += "SIGNS[";
	text += std::to_string(this->size);
	text += "-";
	text += textSigns;
	text += "]";
	return text;
}

std::string Signs::toString()
{
	std::string text = "";
	text += std::to_string(this->size);
	for (int i = 0; i < this->size; i++)
	{
		text += signs[i].toString();
	}
	return text;
}

bool Signs::operator==(const Signs &data) const
{
	if (this->size != data.size)
	{
		return false;
	}
	for (int i = 0; i < MAX_NUM_SIGNATURES && i < this->size; i++)
	{
		if (!(signs[i] == data.signs[i]))
		{
			return false;
		}
	}
	return true;
}

bool Signs::operator<(const Signs &data) const
{
	if (size < data.size)
	{
		return true;
	}
	if (data.size < size)
	{
		return false;
	}
	// They must have the same size
	for (int i = 0; i < size; i++)
	{
		if (signs[i] < data.signs[i])
		{
			return true;
		}
		if (data.signs[i] < signs[i])
		{
			return false;
		}
	}
	return false;
}
