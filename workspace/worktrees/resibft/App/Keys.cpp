#include <iostream>

#include "config.h"
#include "KeysFunctions.h"

int main(int argc, char const *argv[])
{
	// Geting inputs
	unsigned int replicaId = 0;
	KeysFunctions keysFunction;

	if (argc > 1)
	{
		sscanf(argv[1], "%d", &replicaId);
	}
	std::cout << COLOUR_CYAN << "[my id is: " << replicaId << "]" << COLOUR_NORMAL << std::endl;
	std::cout << COLOUR_CYAN << "Generating EC_256 keys" << COLOUR_NORMAL << std::endl;
	keysFunction.generateEc256Keys(replicaId);
	return 0;
}
