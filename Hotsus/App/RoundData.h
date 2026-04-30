#ifndef ROUNDDATA_H
#define ROUNDDATA_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "salticidae/stream.h"
#include "types.h"
#include "Hash.h"

class RoundData
{
private:
	Hash proposeHash;
	View proposeView;
	Hash justifyHash;
	View justifyView;
	Phase phase;

public:
	void serialize(salticidae::DataStream &data) const;
	void unserialize(salticidae::DataStream &data);

	RoundData();
	RoundData(Hash proposeHash, View proposeView, Hash justifyHash, View justifyView, Phase phase);
	RoundData(salticidae::DataStream &data);

	Hash getProposeHash();
	View getProposeView();
	Hash getJustifyHash();
	View getJustifyView();
	Phase getPhase();

	std::string toPrint();
	std::string toString();

	bool operator==(const RoundData &data) const;
};

#endif
