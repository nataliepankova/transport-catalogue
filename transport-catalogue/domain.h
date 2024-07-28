#pragma once
#include <string>
#include <vector>

#include "geo.h"

struct DistanceToStop {
	std::string stop_name;
	int distance;
};

struct Stop {
	std::string name;
	geo::Coordinates coordinates;
};

struct Bus {
	std::string name;
	std::vector<Stop*> route;
	bool is_roundtrip;
};

struct BusSetCmp {
	bool operator() (const Bus* rhs, const Bus* lhs) const {
		return rhs->name < lhs->name;
	}
};