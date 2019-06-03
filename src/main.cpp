//============================================================================
// Name        : Server.cpp
// Author      : weizy
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <pthread.h>
#include "server.h"

using namespace std;


int main() {
	Server s;
	s.initServer();
	s.acceptUsers();

	return 0;
}
