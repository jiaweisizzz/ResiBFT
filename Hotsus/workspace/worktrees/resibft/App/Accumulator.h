#ifndef ACCUMULATOR_H
#define ACCUMULATOR_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "salticidae/stream.h"
#include "Hash.h"
#include "Signs.h"

class Accumulator
{
private:
	bool set;
	View proposeView;
	Hash prepareHash;
	View prepareView;
	unsigned int size;

public:
	void serialize(salticidae::DataStream &data) const;
	void unserialize(salticidae::DataStream &data);

	Accumulator();
	Accumulator(View proposeView, Hash prepareHash, View prepareView, unsigned int size);
	Accumulator(bool set, View proposeView, Hash prepareHash, View prepareView, unsigned int size);
	Accumulator(salticidae::DataStream &data);

	bool isSet();
	View getProposeView();
	Hash getPrepareHash();
	View getPrepareView();
	unsigned int getSize();

	std::string toPrint();
	std::string toString();

	bool operator==(const Accumulator &data) const;
};

#endif
