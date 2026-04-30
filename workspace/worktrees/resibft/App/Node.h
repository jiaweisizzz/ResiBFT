#ifndef NODE_H
#define NODE_H

#include <assert.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include "types.h"
#include "KeysFunctions.h"

typedef std::string Host;

class Node
{
private:
	ReplicaID replicaId;
	Key publicKey;
	Host host;
	PortID replicaPort; // Replica port to listen to replicas
	PortID clientPort;	// Client port to listen to clients

public:
	Node();
	Node(ReplicaID replicaId, Host host, PortID replicaPort, PortID clientPort);
	Node(ReplicaID replicaId, Key publicKey, Host host, PortID replicaPort, PortID clientPort);

	ReplicaID getReplicaId();
	Key getPublicKey();
	Host getHost();
	PortID getReplicaPort();
	PortID getClientPort();

	void setPublicKey(Key publicKey);

	std::string toString();

	bool operator<(const Node &data) const;
};

#endif
