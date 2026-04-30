#include "Statistics.h"

Statistics::Statistics()
{
	this->replicaId = 0;
	this->totalHandleTime = 0.0;
	this->totalViewTime = 0.0;
}

void Statistics::setReplicaId(ReplicaID replicaId)
{
	this->replicaId = replicaId;
}

ReplicaID Statistics::getReplicaId()
{
	return this->replicaId;
}

Times Statistics::getTotalHandleTime()
{
	std::multiset<double> allHandleTimes = this->handleTimes;
	unsigned int totalHandleTimes = 0;
	unsigned int numHandleTimes = 0;
	for (std::multiset<double>::iterator it = allHandleTimes.begin(); it != allHandleTimes.end(); it++)
	{
		totalHandleTimes += *it;
		numHandleTimes++;
	}
	return Times(numHandleTimes, totalHandleTimes);
}

Times Statistics::getTotalViewTime()
{
	std::multiset<double> allViewTimes = this->viewTimes;
	unsigned int totalViewTimes = 0;
	unsigned int numViewTimes = 0;
	for (std::multiset<double>::iterator it = allViewTimes.begin(); it != allViewTimes.end(); it++)
	{
		totalViewTimes += *it;
		numViewTimes++;
	}
	return Times(numViewTimes, totalViewTimes);
}

void Statistics::addHandleTime(double value)
{
	this->totalHandleTime += value;
	this->handleTimes.insert(value);
}

void Statistics::addViewTime(double value)
{
	this->totalViewTime += value;
	this->viewTimes.insert(value);
}

void Statistics::increaseExecuteViews()
{
	this->executeViews++;
}

std::string Statistics::toString()
{
	std::ostringstream os;
	os << "STATISTICS"
	   << "[replicaId = " << this->replicaId
	   << ", executeViews = " << this->executeViews
	   << ", totalHandleTime = " << this->totalHandleTime / this->executeViews
	   << ", totalViewTime = " << this->totalViewTime / this->executeViews
	   << "]";
	return os.str();
}

std::ostream &operator<<(std::ostream &os, const Statistics &data)
{
	os << std::to_string(data.replicaId);
	return os;
}
