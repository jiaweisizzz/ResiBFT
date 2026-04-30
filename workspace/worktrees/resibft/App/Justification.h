#ifndef JUSTIFICATION_H
#define JUSTIFICATION_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "salticidae/stream.h"
#include "Signs.h"
#include "RoundData.h"

class Justification
{
private:
	bool set;
	RoundData roundData;
	Signs signs;

public:
	void serialize(salticidae::DataStream &data) const;
	void unserialize(salticidae::DataStream &data);

	Justification();
	Justification(RoundData roundData, Sign sign);
	Justification(RoundData roundData, Signs signs);
	Justification(bool b, RoundData roundData, Signs signs);

	bool isSet() const;
	RoundData getRoundData() const;
	Signs getSigns() const;

	bool wellFormedInitialize() const;
	bool wellFormedNewview() const;
	bool wellFormedPrepare(unsigned int quorumSize) const;
	bool wellFormed(unsigned int quorumSize) const;
	View getCertificationView() const; // The view at which the certificate was generated depends on the kind of certificate we have
	Hash getCertificationHash() const;

	std::string toPrint() const;
	std::string toString() const;
};

#endif
