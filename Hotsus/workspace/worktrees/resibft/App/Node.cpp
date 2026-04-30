#include "Node.h"

Node::Node()
{
	this->replicaId = 0;
	this->publicKey = NULL;
	this->host = "";
	this->replicaPort = 0;
	this->clientPort = 0;
}

Node::Node(ReplicaID replicaId, Host host, PortID replicaPort, PortID clientPort)
{
	this->replicaId = replicaId;
	this->publicKey = NULL;
	this->host = host;
	this->replicaPort = replicaPort;
	this->clientPort = clientPort;
}

Node::Node(ReplicaID replicaId, Key publicKey, Host host, PortID replicaPort, PortID clientPort)
{
	this->replicaId = replicaId;
	this->publicKey = publicKey;
	this->host = host;
	this->replicaPort = replicaPort;
	this->clientPort = clientPort;
}

ReplicaID Node::getReplicaId()
{
	return this->replicaId;
}

Key Node::getPublicKey()
{
	return this->publicKey;
}

Host Node::getHost()
{
	return this->host;
}

PortID Node::getReplicaPort()
{
	return this->replicaPort;
}

PortID Node::getClientPort()
{
	return this->clientPort;
}

void Node::setPublicKey(Key publicKey)
{
	this->publicKey = publicKey;
}

std::string Node::toString()
{
	std::string text = "";
	text += "NODE[replicaId=";
	text += std::to_string(this->replicaId);
	text += ";host=";
	text += this->host;
	text += ";replicaPort=";
	text += std::to_string(this->replicaPort);
	text += ";clientPort=";
	text += std::to_string(this->clientPort);
	text += "]";
	return text;
}

bool Node::operator<(const Node &data) const
{
	if (replicaId < data.replicaId)
	{
		return true;
	}
	else
	{
		return false;
	}
}
