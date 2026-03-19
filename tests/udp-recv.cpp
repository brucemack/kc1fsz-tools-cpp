#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <iostream>

#include "arpa/inet.h"
#include "netinet/in.h"

using namespace std;

int main(int, const char**) {

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return -1;
    }
    struct sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(514);
    inet_pton(AF_INET, "0.0.0.0", &localAddr.sin_addr); 
    int rc = bind(fd, (const sockaddr*)&localAddr, sizeof(localAddr));
    if (rc < 0) {
        cout << "Bind error" << endl;
        return -1;
    }

    while (true) {
        char msg[1500] = { 0 };
        struct sockaddr_in peerAddr;
        socklen_t peerAddrLen = sizeof(peerAddr);
        rc = recvfrom(fd, msg, sizeof(msg), 0, (struct sockaddr *)&peerAddr, &peerAddrLen); 
        if (rc > 0) {
            cout << msg;
        }
    }
}

