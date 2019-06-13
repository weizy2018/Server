/*
 * server.h
 *
 *  Created on: May 29, 2019
 *      Author: weizy
 */

#ifndef SERVER_H_
#define SERVER_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <time.h>

#include "utils.h"

#include <map>
using namespace std;

class Server {
public:
	Server();
	virtual ~Server();
	static void release();

private:
	static map<string, int> users;
	int server_sockfd;
	struct sockaddr_in server_sockaddr;

private:
	static void * startThread(void *);

private:
	static int query_user(const char * id, const char * passwd);
	static void query_friends_list(string userid, int client_sockfd);
	static void query_unreceive_message(string userid, int client_sockfd);
	static void saveMessage(const SendContent  content);

public:
	void initServer();
	void acceptUsers();
};

#endif /* SERVER_H_ */
