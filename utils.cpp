#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <thread>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <vector>

#define PORT 12345
#define CHUNKSIZE 64

struct data_packet{

    int packetId;
    int retransmitted;
    char data[CHUNKSIZE + 1];

};

struct ack_packet{
    int packetId;
    int ack;
};

void Substring(char *str, char *substr, int pos, int size)
{
    int k = 0;
    while (k < size)
    {
        *(substr + k) = str[k + pos];
        k++;
    }
    *(substr + size) = '\0';
    printf("Substring - %s\n", substr);
    printf("---------------------------------------\n");
}
