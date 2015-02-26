/*
 * Author: Yevgeniy Kiveisha <yevgeniy.kiveisha@intel.com>
 * Copyright (c) 2014 Intel Corporation.
 */

#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

#include "uipc.h"

using namespace std;

WiseIPC::WiseIPC (string socket_path) {
    this->socketPath = socket_path;
}

WiseIPC::~WiseIPC () {
    if (this->fd_sock != -1) {
        close (this->fd_sock);
    }
}

void 
WiseIPC::setBuffer (unsigned char * buff) {
    this->buff = buff;
}

int
WiseIPC::listenIPC () {
    int client = -1;

    if ( (client = accept(this->fd_sock, NULL, NULL)) == -1) {
        return -1;
    }

    return client;
}

bool
WiseIPC::sendMsg (unsigned int len) {
    if (this->fd_sock == -1) {
        return false;
    }

    write(this->fd_sock, this->buff, len);
    return true;
}

void
WiseIPC::readMsg (int socket, unsigned int len) {
    memset (this->buff, 0, len);
    read (socket, this->buff, len);
}

int
WiseIPC::setServer () {
    if ( (this->fd_sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        return ERROR_OPEN_SOCKET;
    }
    
    memset(&this->addr, 0, sizeof(this->addr));
    this->addr.sun_family = AF_UNIX;
    strncpy(this->addr.sun_path, this->socketPath.c_str(), sizeof(this->addr.sun_path) - 1);

    unlink(this->socketPath.c_str());

    if (bind(this->fd_sock, (struct sockaddr*)&this->addr, sizeof(this->addr)) == -1) {
        return ERROR_BIND_SOCKET;
    }

    if (listen(this->fd_sock, 5) == -1) {
        return ERROR_LISTEN_SOCKET;
    }

	return SUCCESS;
}

int
WiseIPC::setClient () {
    if ( (this->fd_sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        return ERROR_OPEN_SOCKET;
    }
    
    memset(&this->addr, 0, sizeof(this->addr));
    this->addr.sun_family = AF_UNIX;
    strncpy(this->addr.sun_path, this->socketPath.c_str(), sizeof(this->addr.sun_path) - 1);

    return connect(this->fd_sock, (struct sockaddr*)&this->addr, sizeof(this->addr));
}

int
WiseIPC::getUnreadDataLength () {
	int error;
	int value = 0;
	
	error = ioctl (this->fd_sock, SIOCINQ, &value);
	
	return value;
}
