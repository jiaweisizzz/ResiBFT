#include "Nodes.h"

Nodes::Nodes() {}

Nodes::Nodes(std::string filename, unsigned int numReplicas)
{
	std::ifstream inFile(filename);
	char oneline[MAXLINE];
	char delim[] = " ";
	char *token;
	unsigned int added = 0;

	if (DEBUG_MODULES)
	{
		std::cout << COLOUR_CYAN << "Parsing configuration file" << COLOUR_NORMAL << std::endl;
	}
	while (inFile && added < numReplicas)
	{
		inFile.getline(oneline, MAXLINE);
		token = strtok(oneline, delim);

		if (token)
		{
			// replicaId
			ReplicaID replicaId = atoi(token + 3);

			// host
			token = strtok(NULL, delim);
			Host host = token + 5;

			// replica port
			token = strtok(NULL, delim);
			PortID replicaPort = atoi(token + 5);

			// client port
			token = strtok(NULL, delim);
			PortID clientPort = atoi(token + 5);

			if (DEBUG_MODULES)
			{
				std::cout << COLOUR_CYAN << "replicaId: " << replicaId << "; host: " << host << "; replicaPort: " << replicaPort << "; clientPort: " << clientPort << COLOUR_NORMAL << std::endl;
			}

			this->addNode(replicaId, host, replicaPort, clientPort);
			added++;
		}
	}
	if (DEBUG_MODULES)
	{
		std::cout << COLOUR_CYAN << "Closing configuration file" << COLOUR_NORMAL << std::endl;
	}
	inFile.close();
	if (DEBUG_MODULES)
	{
		std::cout << COLOUR_CYAN << "Done parsing the configuration file" << COLOUR_NORMAL << std::endl;
	}
}

void Nodes::addNode(ReplicaID replicaId, Host host, PortID replicaPort, PortID clientPort)
{
	if (DEBUG_MODULES)
	{
		std::cout << COLOUR_CYAN << "Adding " << replicaId << " to list of nodes" << COLOUR_NORMAL << std::endl;
	}
	Node node = Node(replicaId, host, replicaPort, clientPort);
	this->nodes[replicaId] = node;
}

void Nodes::addNode(ReplicaID replicaId, Key publicKey, Host host, PortID replicaPort, PortID clientPort)
{
	if (DEBUG_MODULES)
	{
		std::cout << COLOUR_CYAN << "Adding " << replicaId << " to list of nodes" << COLOUR_NORMAL << std::endl;
	}
	Node node = Node(replicaId, publicKey, host, replicaPort, clientPort);
	this->nodes[replicaId] = node;
}

void Nodes::setPublicKey(ReplicaID replicaId, Key publicKey)
{
	std::map<ReplicaID, Node>::iterator it = this->nodes.find(replicaId);
	if (it != this->nodes.end())
	{
		if (DEBUG_MODULES)
		{
			std::cout << COLOUR_CYAN << "Found a corresponding Node entry" << COLOUR_NORMAL << std::endl;
		}
		Node node = it->second;
		node.setPublicKey(publicKey);
		this->nodes[replicaId] = node;
	}
}

Node *Nodes::find(ReplicaID replicaId)
{
	std::map<ReplicaID, Node>::iterator it = this->nodes.find(replicaId);
	if (it != this->nodes.end())
	{
		if (DEBUG_MODULES)
		{
			std::cout << COLOUR_CYAN << "Found a corresponding Node entry" << COLOUR_NORMAL << std::endl;
		}
		return &(it->second);
	}
	else
	{
		return NULL;
	}
}

std::set<ReplicaID> Nodes::getReplicaIds()
{
	std::map<ReplicaID, Node>::iterator it = this->nodes.begin();
	std::set<ReplicaID> replicaIds = {};
	while (it != this->nodes.end())
	{
		replicaIds.insert(it->first);
		it++;
	}
	return replicaIds;
}

void Nodes::printNodes()
{
	std::map<ReplicaID, Node>::iterator it = this->nodes.begin();
	while (it != this->nodes.end())
	{
		ReplicaID replicaId = it->first;
		Node node = it->second;
		if (DEBUG_MODULES)
		{
			std::cout << COLOUR_CYAN << "replicaId: " << replicaId << COLOUR_NORMAL << std::endl;
		}
		it++;
	}
}

int Nodes::printNumReplicas()
{
	std::map<ReplicaID, Node>::iterator it = this->nodes.begin();
	int count = 0;
	while (it != this->nodes.end())
	{
		count++;
		it++;
	}
	return count;
}
