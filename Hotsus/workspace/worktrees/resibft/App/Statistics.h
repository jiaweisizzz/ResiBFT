#ifndef STATISTICS_H
#define STATISTICS_H

#include <chrono>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "types.h"

using Time = std::chrono::time_point<std::chrono::steady_clock>;

struct Times
{
	unsigned int num;
	double total;
	Times()
	{
		num = 0;
		total = 0.0;
	}
	Times(unsigned int num, double total) : num(num), total(total) {}
};

class Statistics
{
private:
	ReplicaID replicaId;

	std::multiset<double> handleTimes;
	std::multiset<double> viewTimes;
	double totalHandleTime = 0.0; // Total time of spending on handling messages
	double totalViewTime = 0.0;	  // Total time of spending on views
	unsigned int executeViews = 0;

public:
	Statistics();

	void setReplicaId(ReplicaID replicaId);
	ReplicaID getReplicaId();
	Times getTotalHandleTime();
	Times getTotalViewTime();

	void addHandleTime(double value);
	void addViewTime(double value);
	void increaseExecuteViews();

	std::string toString();

	friend std::ostream &operator<<(std::ostream &os, const Statistics &data);
};

#endif
