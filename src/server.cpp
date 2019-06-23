/*
 * server.cpp
 *
 *  Created on: May 29, 2019
 *      Author: weizy
 */

#include "server.h"
#include "utils.h"
#include <stdio.h>
#include <string>
#include <iostream>
#include <pthread.h>
#include <mysql.h>

using namespace std;

map<string, int>  Server::users;
Server::Server() {
	server_sockfd = 0;
}

Server::~Server() {

}
void Server::release() {

}

//服务器端初始化socket
void Server::initServer() {
	//新建socket对象
	server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_port = htons(PORT);
	server_sockaddr.sin_addr.s_addr = INADDR_ANY;

	int server_len = sizeof(server_sockaddr);
	//服务器端绑定socket
	if (bind(server_sockfd, (struct sockaddr *) &server_sockaddr, server_len) == -1) {
		perror("bind fail !!!");
		exit(1);
	}
	//监听
	if (listen(server_sockfd, 5) == -1) {
		perror("listen fail !!!");
		exit(1);
	}
	printf("Listening...\n");
}

void Server::acceptUsers() {
	struct sockaddr_in client_sockaddr;
	unsigned int client_len = sizeof(client_sockaddr);
	int client_sockfd;
	cout << "accepting..." << endl;
	//死循环接收客户端的连接
	while (true) {
		if ((client_sockfd = accept(server_sockfd, (struct sockaddr*)&client_sockaddr, &client_len)) == -1) {
			perror("accept error");
			exit(1);
		}
		printf("%s login service\n", inet_ntoa(client_sockaddr.sin_addr));
		//每当一个客户端连接就开启一个线程对该客户端进行处理
		pthread_t userId;
		pthread_create(&userId, NULL, startThread, &client_sockfd);
	}
}
void * Server::startThread(void * sockfd) {
	int client_sockfd = *(int*)sockfd;
	struct SendContent content;
	memset(&content, 0, sizeof(content));
	//接收客户端发来的消息
	while (content.type != EXIT_ACTION) {
		if (read(client_sockfd, &content, sizeof(content)) == 0) {
			cout << "read client_sockfd failed" << endl;
			break;
		} else {
			//根据content.type来判断客户端发来的是什么消息（登录消息/聊天消息）
			//以便做出不同的处理
			if (content.type == LOGIN_ACTION) {
				cout << "======================login======================" << endl;
				char * account = content.sender;
				char * passwd = content.receiver;

				cout << "account: " << account << " passwd = " << passwd << endl;

				int result = query_user(account, passwd);
				if (result != 0) {
					write(client_sockfd, LOGIN_FAIL, 10);
				} else {
					write(client_sockfd, LOGIN_SUCCESS, 10);
					//获取将该用户的好友列表并发送
					users.insert(pair<string, int>(string(account), client_sockfd));
					query_friends_list(account, client_sockfd);
					//获取离线时未接收的消息
					query_unreceive_message(account, client_sockfd);
				}


			} else if (content.type == EXIT_ACTION) {
				cout << "======================exit======================" << endl;
				break;
			} else if (content.type == SEND_MESSAGE_ACTION) {
				cout << "======================send message======================" << endl;
				cout << content.sender << " " << content.receiver << " " << content.message << " " << content.sendTime << endl;

				auto it = users.find(content.receiver);
				if (it != users.end()) {				//发送的客户端在线，直接发送给客户端
					write(users[content.receiver], &content, sizeof(content));
				} else {								//用户不在线, 将数据保存到数据库中
					saveMessage(content);
				}
			} else if (content.type == REGISTER_ACTION) {
				cout << "========================register==========================" << endl;
				char * account = content.sender;
				char * passwd = content.receiver;
				cout << "account: " << account << "  password: " << passwd << endl;
				int result = userRegister(account, passwd);
				if (result != 0) {
					//失败
					write(client_sockfd, REGISTER_FAIL, 10);
				} else {
					//成功
					write(client_sockfd, REGISTER_SUCCESS, 10);
				}
			}
		}
		memset(&content, 0, sizeof(content));
	}
	//将client_sockfd从users中删除
	for (auto it = users.begin(); it != users.end(); it++) {
		if (it->second == client_sockfd) {
			users.erase(it->first);
			break;
		}
	}
	close(client_sockfd);
	return NULL;
}

int Server::query_user(const char * id, const char * passwd) {
	MYSQL conn;
	mysql_init(&conn);
	if(mysql_real_connect(&conn,"localhost",USER,PASSWD,DATABASE,0,NULL,CLIENT_FOUND_ROWS) == NULL) {
		cout << "mysql connect fail!!!" << endl;
		return -1;
	}
	string query = "select * from user where id = '";
	query.append(id);
	query.append("' and passwd = '");
	query.append(passwd);
	query.append("'");

	if (mysql_query(&conn, query.c_str()) != 0) {
		cout << "query error" << endl;
		return -1;
	}
	MYSQL_RES * result = mysql_store_result(&conn);
	MYSQL_ROW row;
	if ((row = mysql_fetch_row(result)) == NULL) {
		cout << "no such user" << endl;
		return -1;
	}
	cout << "query OK" << endl;

	mysql_free_result(result);
	mysql_close(&conn);

	return 0;
}

void Server::query_friends_list(string userid, int client_sockfd) {
	cout << "query friends list: " << userid << endl;
	MYSQL conn;
	mysql_init(&conn);
	if(mysql_real_connect(&conn,"localhost",USER,PASSWD,DATABASE,0,NULL,CLIENT_FOUND_ROWS) == NULL) {
		cout << "mysql connect fail!!!" << endl;
		return;
	}

	string sql = "select friendid from friends where userid = '";
	sql.append(userid);
	sql.append("'");

	if (mysql_query(&conn, sql.c_str()) != 0) {
		cout << "query error" << endl;
		return;
	}

	MYSQL_RES * result = mysql_store_result(&conn);
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)) != NULL) {
		friends f;
		strcpy(f.friendid, row[0]);
		f.flag = 0;
		cout << row[0] << endl;
		write(client_sockfd, &f, sizeof(f));
	}
	//发送作为结束的标志
	friends f_end;
	f_end.flag = END_FLAG;
	write(client_sockfd, &f_end, sizeof(f_end));

	cout << "friend list query OK!" << endl;

	mysql_free_result(result);
	mysql_close(&conn);
}

void Server::query_unreceive_message(string userid, int client_sockfd) {
	cout << "query unreceive message" << endl;
	MYSQL conn;
	mysql_init(&conn);
	if (mysql_real_connect(&conn, "localhost", USER, PASSWD, DATABASE, 0, NULL, CLIENT_FOUND_ROWS) == NULL) {
		cout << "mysql connect fail!!!" << endl;
		return;
	}
	//select * from receive where receiver = '123' order by receive_date
	string sql = "select * from receive where receiver = '";
	sql.append(userid);
	sql.append("' ");
	sql.append("order by receive_date");

	if (mysql_query(&conn, sql.c_str()) != 0) {
		cout << "query error" << endl;
		return;
	}

	MYSQL_RES * result = mysql_store_result(&conn);
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)) != NULL) {
		SendContent content;
		strcpy(content.receiver, userid.c_str());
		strcpy(content.sender, row[2]);
		strcpy(content.message, row[3]);
		strcpy(content.sendTime, row[4]);
		cout << row[2] << " " << row[3] << " " << row[4] << endl;
		content.type = 0;
		write(client_sockfd, &content, sizeof(content));
	}
	//发送个结束标志
	SendContent cont;
	cont.type = END_FLAG;
	write(client_sockfd, &cont, sizeof(cont));
	cout << "query OK" << endl;

	//删除数据库中的聊天记录，服务器不再保留这些聊天记录
	//测试阶段不做删除，注释掉

	sql = "delete from receive where receiver='";
	sql.append(userid);
	sql.append("'");

	if (mysql_query(&conn, sql.c_str()) != 0) {
		cout << "message delete error" << endl;
		return;
	}


	mysql_free_result(result);
	mysql_close(&conn);
}

void Server::saveMessage(const SendContent  content) {
	cout << "save message" << endl;
	MYSQL conn;
	mysql_init(&conn);
	if (mysql_real_connect(&conn, "localhost", USER, PASSWD, DATABASE, 0, NULL, CLIENT_FOUND_ROWS) == NULL) {
		cout << "mysql connect fail!!!" << endl;
		return;
	}
	//INSERT INTO `chat`.`receive` (`receiver`, `sender`, `content`, `receive_date`) VALUES ('zhangsan', '123', 'I\'m 123', '2019-6-13 12:23:30');

	string sql = "INSERT INTO receive (receiver, sender, content, receive_date) VALUES ('";
	sql += content.receiver;
	sql += "', '";
	sql += content.sender;
	sql += "', '";
	sql += content.message;
	sql += "', '";
	sql += content.sendTime;
	sql += "')";

	cout << sql << endl;

	if (mysql_query(&conn, sql.c_str()) != 0) {
		cout << "query error" << endl;
		return;
	}
	cout << "save OK" << endl;
	mysql_close(&conn);
}

int Server::userRegister(const char * id, const char * passwd) {
	MYSQL conn;
	mysql_init(&conn);
	if (mysql_real_connect(&conn, "localhost", USER, PASSWD, DATABASE, 0, NULL, CLIENT_FOUND_ROWS) == NULL) {
		cout << "mysql connect fail!!!" << endl;
		return -1;
	}
	//先查找该账号是否已经存在
	string query = "select * from user where id = '";
	query.append(id).append("'");

	if (mysql_query(&conn, query.c_str()) != 0) {
		cout << "query error" << endl;
		return -1;
	}
	MYSQL_RES * result = mysql_store_result(&conn);
	MYSQL_ROW row;
	//结果集不是空表明用户该账号已经存在
	if ((row = mysql_fetch_row(result)) != NULL) {
		cout << "账号已经存在" << endl;
		return -1;
	}

	//进行注册
	//INSERT INTO `chat`.`user` (`id`, `passwd`) VALUES ('hello', '123');
	string sql = "insert into user (id, passwd) values ('";
	sql.append(id).append("', '");
	sql.append(passwd).append("')");
	if (mysql_query(&conn, sql.c_str()) != 0) {
		cout << "插入失败" << endl;
		return -1;
	}
	return 0;
}









