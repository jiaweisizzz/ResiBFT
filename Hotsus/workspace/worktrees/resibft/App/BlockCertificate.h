#ifndef BLOCKCERTIFICATE_H
#define BLOCKCERTIFICATE_H

#include <string>
#include "salticidae/stream.h"
#include "types.h"
#include "Block.h"
#include "Justification.h"
#include "Signs.h"
#include "Nodes.h"

class BlockCertificate
{
private:
	Block block;
	View view;
	Justification qcPrecommit;
	Signs qcVc;

public:
	void serialize(salticidae::DataStream &data) const;
	void unserialize(salticidae::DataStream &data);

	BlockCertificate();
	BlockCertificate(Block block, View view, Justification qcPrecommit, Signs qcVc);
	BlockCertificate(salticidae::DataStream &data);

	Block getBlock();
	View getView();
	Justification getQcPrecommit();
	Signs getQcVc();

	bool verify(Nodes nodes);

	std::string toPrint();
	std::string toString();

	bool operator==(const BlockCertificate &data) const;
	bool operator<(const BlockCertificate &data) const;
};

#endif