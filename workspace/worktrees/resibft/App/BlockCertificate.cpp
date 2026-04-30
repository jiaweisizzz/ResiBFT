#include "BlockCertificate.h"

BlockCertificate::BlockCertificate()
{
	this->block = Block();
	this->view = 0;
	this->qcPrecommit = Justification();
	this->qcVc = Signs();
}

BlockCertificate::BlockCertificate(Block block, View view, Justification qcPrecommit, Signs qcVc)
{
	this->block = block;
	this->view = view;
	this->qcPrecommit = qcPrecommit;
	this->qcVc = qcVc;
}

BlockCertificate::BlockCertificate(salticidae::DataStream &data)
{
	this->unserialize(data);
}

void BlockCertificate::serialize(salticidae::DataStream &data) const
{
	data << this->block << this->view << this->qcPrecommit << this->qcVc;
}

void BlockCertificate::unserialize(salticidae::DataStream &data)
{
	data >> this->block >> this->view >> this->qcPrecommit >> this->qcVc;
}

Block BlockCertificate::getBlock()
{
	return this->block;
}

View BlockCertificate::getView()
{
	return this->view;
}

Justification BlockCertificate::getQcPrecommit()
{
	return this->qcPrecommit;
}

Signs BlockCertificate::getQcVc()
{
	return this->qcVc;
}

bool BlockCertificate::verify(Nodes nodes)
{
	// Verify qcPrecommit is well-formed
	if (!this->qcPrecommit.wellFormed(0))
	{
		return false;
	}
	// TODO: Add proper signature verification based on quorum size
	return true;
}

std::string BlockCertificate::toPrint()
{
	std::string text = "";
	text += "BC[";
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

std::string BlockCertificate::toString()
{
	std::string text = "";
	text += this->block.toString();
	text += std::to_string(this->view);
	text += this->qcPrecommit.toString();
	text += this->qcVc.toString();
	return text;
}

bool BlockCertificate::operator==(const BlockCertificate &data) const
{
	return (this->block == data.block && this->view == data.view);
}

bool BlockCertificate::operator<(const BlockCertificate &data) const
{
	if (this->view < data.view)
	{
		return true;
	}
	return false;
}