#ifndef LEADER_ELECTION_H
#define LEADER_ELECTION_H

#include <vector>
#include "Node.h"
#include <thread>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>   
#include <bits/stdc++.h> 
#include <ctime>

#define P(s) semop(s, &pop, 1) 
#define V(s) semop(s, &vop, 1) 

using json = nlohmann::json;

void handleAppendEntries(Node &node, const struct sockaddr_in &clientAddr, const char *msg, int sockfd);
void sendAppendEntries(Node &node, const struct sockaddr_in &clientAddr, const char *msg, int sockfd);
void parseAppendRequest(const string &msg, int &term, int &prevLogIndex, vector<LogEntry> &tempLog, int &commitIndex);
int storeEntries(Node &node, const char *msg);
void handleAppendRequest(Node &node, const struct sockaddr_in &clientAddr, const char *msg, int sockfd);
void handleAppendReply(Node &node, const struct sockaddr_in &clientAddr, const char *msg, int sockfd);
void handleHeartbeat(Node &node, const struct sockaddr_in &clientAddr, const char *msg, int sockfd);
void handleRequestVote(Node &node, const struct sockaddr_in &clientAddr, const char *msg, int sockfd);
void handleVote(Node &node, const struct sockaddr_in &clientAddr, const char *msg, int sockfd);
void handleAck(Node &node, const struct sockaddr_in &clientAddr, const char *msg, int sockfd);
void receiveThreadFunction(Node &node, int sockfd);
void startElection(Node &node, int &sockfd);
void assignType(Node &node);
void processLogEntry(Node &node, const LogEntry &entry);



#endif