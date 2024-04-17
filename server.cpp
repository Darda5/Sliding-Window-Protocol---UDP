#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include "utils.cpp"
#include <fstream>

int totalPackets;
std::vector<data_packet> receivedData;
void SortData();

//compiled using "gcc -o server server.cpp -lm -lstdc++"

int main()
{
	// Create a UDP socket
    int serverSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP); 
    if (serverSocket < 0)
    {
        printf("Error in creating server socket! \n");
        exit(0);
    }
    printf("Server socket created successfully! \n");

	// Set up server address structure
    struct sockaddr_in server, client;
    server.sin_family = AF_INET;                
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);              

    socklen_t client_addr_length = sizeof(client);

	// Bind the socket to the server address
    int bindServer = bind(serverSocket, (struct sockaddr *)&server, sizeof(server));
    if (bindServer < 0)
    {
        printf("Error in binding socket to port and local addr! \n");
        exit(0);
    }
    printf("Server socket bound to port and local addr successfully! \n");

    int windowSize;
    // Receive the window size from the client
    int recvWindiowSize = recvfrom(serverSocket, &windowSize, sizeof(windowSize), 0, (struct sockaddr *)&client, &client_addr_length);

    if (recvWindiowSize < 0)
    {
        printf("Error in receiving window size from client! \n");
        exit(0);
    }
    printf("Received window size - \"%d\" from client successfully! \n", windowSize);

    // Receive total number of packets expected from client
    int recvTotalPack = recvfrom(serverSocket, (int *)&totalPackets, sizeof(totalPackets), 0, (struct sockaddr *)&client, &client_addr_length);
    if (recvTotalPack < 0)
    {
        printf("Error in receiving total packet count!\n");
        exit(0);
    }

    printf("Total packets to be received from client - %d packets\n", totalPackets);

    int num = 0;
    // Random seed
    srand(time(NULL));

    int dropChance;
    printf("###################################\n");

    while(1){


        struct data_packet pack;
        pack.packetId = 0;
        pack.retransmitted = 0;
        pack.data[CHUNKSIZE] = '\0';

        // Receive the data packet from the client
        int recvPack = recvfrom(serverSocket, &pack, sizeof(struct data_packet), 0, (struct sockaddr *)&client, &client_addr_length);

        if (recvPack < 0)
        {
            printf("Error in receiving data packet\n");
            exit(0);
        }   

            
            // Simulation of lost packages
        	// Generates a random number between 0 and 100
        	// If the number is greater than 30, the packet is successfully received
        	// Retransmitted packages are guaranteed to be received
            dropChance = rand() % 101;

            if(pack.retransmitted == 1){
            printf("***********************************\n");
            printf("Packet with ID %d was retransmitted\n", pack.packetId);
            printf("***********************************\n");
            }

        if(dropChance > 30 || pack.retransmitted == 1)
        {
            
            printf("-----------------------------------\n");
            printf("Packet with id %d received.\n", pack.packetId);
            printf("-----------------------------------\n");

            for (int del = 0; del < 10000; del++){}
            
            // Send acknowledgment for the received packet
            int ackSent = sendto(serverSocket, (void *)&pack.packetId, sizeof(pack.packetId), 0, (struct sockaddr *)&client, client_addr_length);
            
            if(ackSent < 0)
            {
                printf("Error in sending ack for packet - %d\n", pack.packetId);
                exit(0);
            }
            
            // Add the received packet to the packet list and increase num
            // Once num equals the amount of expected packages, the server stops receiving packets
            printf("ACK sent for packet with id - %d. \n", pack.packetId);
            receivedData.push_back(pack);
            num++;

            if(num == totalPackets) break;
        }
        else
        {
            printf("Packet %d dropped.\n", pack.packetId);
            
        }
  
        
    }


    // Sorts packets based on their id
    // Ensuring correct writing into the output file
    SortData();

	// Open a file stream for writing received data to a file
    std::fstream strm;
    strm.open("out.txt", std::ios_base::out);

	// Write each received packet's data to the output file
    for (int k = 0; k < totalPackets; k++)
    {
        char write[CHUNKSIZE + 1];
        write[CHUNKSIZE] = '\0';
        strcpy(write, receivedData[k].data);
        strm << write;
    }
    strm.close();

    printf("Data written to out.txt successfully\n");

    strm.close();
    close(serverSocket);
}

// Function to sort received data packets based on their id
void SortData()
{
    data_packet temp;
    for(int i = 0; i < totalPackets; i++){
        for(int k = i + 1; k < totalPackets; k++){
            if(receivedData[i].packetId > receivedData[k].packetId){
                temp = receivedData[i];
                receivedData[i] = receivedData[k];
                receivedData[k] = temp;

            }
        }
        
    }
}
