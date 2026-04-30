#ifndef NODES_H
#define NODES_H

#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "config.h"
#include "types.h"
#include "Node.h"

class Nodes
{
private:
	std::map<ReplicaID, Node> nodes;

public:
	Nodes();
	Nodes(std::string filename, unsigned int numReplicas);

	void addNode(ReplicaID replicaId, Host host, PortID replicaPort, PortID clientPort);
	void addNode(ReplicaID replicaId, Key publicKey, Host host, PortID replicaPort, PortID clientPort);
	void setPublicKey(ReplicaID replicaId, Key publicKey);
	Node *find(ReplicaID replicaId);
	std::set<ReplicaID> getReplicaIds();
	void printNodes();
	int printNumReplicas();
};

#endif
