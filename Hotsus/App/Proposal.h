#ifndef PROPOSAL_H
#define PROPOSAL_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "salticidae/stream.h"
#include "Accumulator.h"
#include "Block.h"
#include "Justification.h"
#include "Signs.h"

template <class Certification>

class Proposal
{
private:
	Certification certification;
	Block block;

public:
	void serialize(salticidae::DataStream &data) const;
	void unserialize(salticidae::DataStream &data);

	Proposal();
	Proposal(Certification certification, Block block);

	Certification getCertification();
	Block getBlock();

	std::string toPrint();
	std::string toString();
};

#endif
