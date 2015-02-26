/*
 * Author: Yevgeniy Kiveisha <yevgeniy.kiveisha@intel.com>
 * Copyright (c) 2014 Intel Corporation.
 */

#pragma once

#include <iostream>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <sys/un.h>

using namespace std;

#define SUCCESS				0
#define ERROR_OPEN_SOCKET	-10
#define ERROR_BIND_SOCKET	-11
#define ERROR_LISTEN_SOCKET	-12

#define MESSAGE_SIZE        42

class WiseIPC {
    public:
        WiseIPC (string socket_path);
        ~WiseIPC ();

        void setBuffer (unsigned char * buff);
        int  listenIPC ();
        int  setServer ();
        int  setClient ();
        bool sendMsg (unsigned int len);
        void readMsg (int socket, unsigned int len);
		int  getUnreadDataLength ();

        unsigned char *     buff;

    private:
        int                 fd_sock;
        struct sockaddr_un  addr;
        string              socketPath;
};
