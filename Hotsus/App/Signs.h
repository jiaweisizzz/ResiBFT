#ifndef SIGNS_H
#define SIGNS_H

#include <iostream>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "salticidae/stream.h"
#include "parameters.h"
#include "Nodes.h"
#include "Sign.h"
#include "Statistics.h"

class Signs
{
private:
	unsigned int size;
	Sign signs[MAX_NUM_SIGNATURES];

public:
	void serialize(salticidae::DataStream &data) const;
	void unserialize(salticidae::DataStream &data);

	Signs();
	Signs(Sign sign);
	Signs(unsigned int size, Sign signs[MAX_NUM_SIGNATURES]);
	Signs(salticidae::DataStream &data);

	unsigned int getSize();
	Sign get(unsigned int n);

	void add(Sign sign);
	void addUpto(Signs signs, unsigned int n);
	bool hasSigned(ReplicaID replicaId);
	bool verify(ReplicaID id, Nodes nodes, std::string text);
	std::set<ReplicaID> getSigners();
	std::string printSigners();

	std::string toPrint();
	std::string toString();

	bool operator==(const Signs &data) const;
	bool operator<(const Signs &data) const;
};

#endif
