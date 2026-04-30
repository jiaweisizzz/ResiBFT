#include "VerificationCertificate.h"
#include "ResiBFTBasic.h"

VerificationCertificate::VerificationCertificate()
{
	this->type = VC_ACCEPTED;
	this->view = 0;
	this->blockHash = Hash();
	this->signer = 0;
	this->sign = Sign();
}

VerificationCertificate::VerificationCertificate(VCType type, View view, Hash blockHash, ReplicaID signer, Sign sign)
{
	this->type = type;
	this->view = view;
	this->blockHash = blockHash;
	this->signer = signer;
	this->sign = sign;
}

VerificationCertificate::VerificationCertificate(salticidae::DataStream &data)
{
	unserialize(data);
}

void VerificationCertificate::serialize(salticidae::DataStream &data) const
{
	data << this->type << this->view << this->blockHash << this->signer << this->sign;
}

void VerificationCertificate::unserialize(salticidae::DataStream &data)
{
	data >> this->type >> this->view >> this->blockHash >> this->signer >> this->sign;
}

VCType VerificationCertificate::getType() const
{
	return this->type;
}

View VerificationCertificate::getView() const
{
	return this->view;
}

Hash VerificationCertificate::getBlockHash() const
{
	return this->blockHash;
}

ReplicaID VerificationCertificate::getSigner() const
{
	return this->signer;
}

Sign VerificationCertificate::getSign() const
{
	return this->sign;
}

std::string VerificationCertificate::toPrint() const
{
	std::string text = "";
	text += "VC[";
	text += printVCType(this->type);
	text += ",v=";
	text += std::to_string(this->view);
	text += ",";
	text += this->blockHash.toPrint();
	text += ",signer=";
	text += std::to_string(this->signer);
	text += ",";
	text += this->sign.toPrint();
	text += "]";
	return text;
}

std::string VerificationCertificate::toString() const
{
	std::string text = "";
	text += std::to_string(this->type);
	text += std::to_string(this->view);
	text += this->blockHash.toString();
	text += std::to_string(this->signer);
	text += this->sign.toString();
	return text;
}

bool VerificationCertificate::operator==(const VerificationCertificate &data) const
{
	return (this->type == data.type && this->view == data.view && this->blockHash == data.blockHash && this->signer == data.signer && this->sign == data.sign);
}

bool VerificationCertificate::operator<(const VerificationCertificate &data) const
{
	if (this->view < data.view)
	{
		return true;
	}
	if (this->view == data.view && this->signer < data.signer)
	{
		return true;
	}
	return false;
}