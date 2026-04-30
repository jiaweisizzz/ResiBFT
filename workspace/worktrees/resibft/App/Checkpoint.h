#ifndef CHECKPOINT_H
#define CHECKPOINT_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "salticidae/stream.h"
#include "types.h"
#include "Block.h"
#include "Justification.h"
#include "Signs.h"

class Checkpoint
{
private:
	Block block;
	View view;
	Justification qcPrecommit;
	Signs qcVc;

public:
	void serialize(salticidae::DataStream &data) const;
	void unserialize(salticidae::DataStream &data);

	Checkpoint();
	Checkpoint(Block block, View view, Justification qcPrecommit, Signs qcVc);
	Checkpoint(salticidae::DataStream &data);

	Block getBlock() const;
	View getView() const;
	Justification getQcPrecommit() const;
	Signs getQcVc() const;

	Hash hash() const;

	bool wellFormed(unsigned int quorumSize) const;

	std::string toPrint() const;
	std::string toString() const;

	bool operator==(const Checkpoint &data) const;
	bool operator<(const Checkpoint &data) const;
};

#endif