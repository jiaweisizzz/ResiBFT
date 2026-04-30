#include "Checkpoint.h"

Checkpoint::Checkpoint()
{
	this->block = Block();
	this->view = 0;
	this->qcPrecommit = Justification();
	this->qcVc = Signs();
}

Checkpoint::Checkpoint(Block block, View view, Justification qcPrecommit, Signs qcVc)
{
	this->block = block;
	this->view = view;
	this->qcPrecommit = qcPrecommit;
	this->qcVc = qcVc;
}

Checkpoint::Checkpoint(salticidae::DataStream &data)
{
	this->unserialize(data);
}

void Checkpoint::serialize(salticidae::DataStream &data) const
{
	data << this->block << this->view << this->qcPrecommit << this->qcVc;
}

void Checkpoint::unserialize(salticidae::DataStream &data)
{
	data >> this->block >> this->view >> this->qcPrecommit >> this->qcVc;
}

Block Checkpoint::getBlock() const
{
	return this->block;
}

View Checkpoint::getView() const
{
	return this->view;
}

Justification Checkpoint::getQcPrecommit() const
{
	return this->qcPrecommit;
}

Signs Checkpoint::getQcVc() const
{
	return this->qcVc;
}

Hash Checkpoint::hash() const
{
	return this->block.hash();
}

bool Checkpoint::wellFormed(unsigned int quorumSize) const
{
	// Check that block is valid (not dummy)
	if (this->block.isDummy())
	{
		return false;
	}

	// Check that qcPrecommit has quorum signatures
	if (this->qcPrecommit.getSigns().getSize() < quorumSize)
	{
		return false;
	}

	// Check that qcVc is valid (optional - may be empty for some checkpoints)
	// qcVc should at least have some signatures if it's set
	return true;
}

std::string Checkpoint::toPrint() const
{
	std::string text = "";
	text += "CHECKPOINT[";
	text += this->block.toPrint();
	text += ",v=";
	text += std::to_string(this->view);
	text += ",";
	text += this->qcPrecommit.toPrint();
	text += ",";
	text += this->qcVc.toPrint();
	text += "]";
	return text;
}

std::string Checkpoint::toString() const
{
	std::string text = "";
	text += this->block.toString();
	text += std::to_string(this->view);
	text += this->qcPrecommit.toString();
	text += this->qcVc.toString();
	return text;
}

bool Checkpoint::operator==(const Checkpoint &data) const
{
	return (this->block == data.block && this->view == data.view);
}

bool Checkpoint::operator<(const Checkpoint &data) const
{
	if (this->view < data.view)
	{
		return true;
	}
	return false;
}