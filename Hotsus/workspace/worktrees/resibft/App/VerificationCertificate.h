#ifndef VERIFICATIONCERTIFICATE_H
#define VERIFICATIONCERTIFICATE_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "salticidae/stream.h"
#include "types.h"
#include "Hash.h"
#include "Sign.h"

class VerificationCertificate
{
private:
	VCType type;
	View view;
	Hash blockHash;
	ReplicaID signer;
	Sign sign;

public:
	void serialize(salticidae::DataStream &data) const;
	void unserialize(salticidae::DataStream &data);

	VerificationCertificate();
	VerificationCertificate(VCType type, View view, Hash blockHash, ReplicaID signer, Sign sign);
	VerificationCertificate(salticidae::DataStream &data);

	VCType getType() const;
	View getView() const;
	Hash getBlockHash() const;
	ReplicaID getSigner() const;
	Sign getSign() const;

	std::string toPrint() const;
	std::string toString() const;

	bool operator==(const VerificationCertificate &data) const;
	bool operator<(const VerificationCertificate &data) const;
};

#endif