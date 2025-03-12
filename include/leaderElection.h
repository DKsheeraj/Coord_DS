#ifndef LEADER_ELECTION_H
#define LEADER_ELECTION_H

#include <vector>
#include "Node.h"
#include <thread>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void sendHeartbeat(Node &node);
void receiveHeartbeat(Node &node);
#endif