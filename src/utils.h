/*
 * utils.h
 *
 *  Created on: May 29, 2019
 *      Author: weizy
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <string>
#include <ctime>

#define LOGIN_ACTION            1
#define SEND_MESSAGE_ACTION     2
#define EXIT_ACTION             3

#define TYPE_LENGTH             6
#define ACCOUNT_LENGTH          16
#define PASSWORD_LENGTH         16
#define MESSAGE_LENGTH          205
#define TIME_LENGTH				22

#define PORT                    6688
#define SERVER_ADDR             "127.0.0.1"

#define LOGIN_SUCCESS           "success"
#define LOGIN_FAIL              "fail"

#define USER 					"root"
#define PASSWD 					"123456"
#define DATABASE 				"chat"

#define END_FLAG                1


struct SendContent {
    int type;
    char receiver[ACCOUNT_LENGTH];      //如果type==LOGIN_ACTION 则receiver的内容为password， sender的内容为account
    char sender[ACCOUNT_LENGTH];
    char message[MESSAGE_LENGTH];
    char sendTime[TIME_LENGTH];
};

struct friends {
    char friendid[ACCOUNT_LENGTH];
    int flag;
};

class Utils {
public:
	Utils();
	virtual ~Utils();
};

#endif /* UTILS_H_ */
