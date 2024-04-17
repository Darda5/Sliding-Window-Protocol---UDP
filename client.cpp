#include "utils.cpp"
#include <string>
#include <thread>
// #include <boost/filesystem.hpp>  
#include <chrono>
#include <condition_variable>
#include <signal.h>

int windowSize = 10;

struct sockaddr_in server;
int client_socket;

int totalPackets, packetsLeft, currentPacket = 1;

std::vector<data_packet> packetList;
std::vector<ack_packet> ackQueue;

int inc = 0;

void receive_ack(int server_socket);
void packet_dropped(int server_socket);

std::thread ackThread;

//compiled using "gcc -o client client.cpp -lm -lstdc++ -pthread"

int main(){

	// Set up server address structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(PORT);

	// Create a UDP socket
    client_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (client_socket < 0)
    {
        perror("socket creation failed");
        exit(0);
    }


    // Notifying server of the window size
    int windowSizeSent = sendto(client_socket, &windowSize, sizeof(windowSize), 0, (struct sockaddr *)&server, sizeof(server));

    if(windowSizeSent < 0){
        printf("Failed to send window size to server.\n");
        exit(0);
    }
    printf("Window size successfully sent to server!\n");
	
	// Open the file for reading
    FILE *file = fopen("file1.txt", "r");

    // Seek to the end of the file to get the file size
    double packetSize = (double)CHUNKSIZE;   
    fseek(file, 0L, SEEK_END);                   
    double file_size = ftell(file);                   
    fseek(file, 0L, SEEK_SET);                          

	// Allocate memory for file data
    char *file_data = (char *)malloc(file_size);          
    fread(file_data, file_size, 1, file);                 

    // Calculate the total number of packets
    totalPackets = (int)ceil((file_size) / packetSize); 
    packetsLeft = totalPackets;

    int totalPacketSent = sendto(client_socket, &totalPackets, sizeof(totalPackets), 0, (struct sockaddr *)&server, sizeof(server));

    //Notifying server of the number of packets expected
    if(totalPacketSent < 0){
        printf("Failed to send number of packets to server.\n");
        exit(0);
    }

    printf("Number of packets successfully sent to server!\n");
    
    int pos = 0;
    int i = 0;
    // Prepare two lists, one that consists of packets to be sent
    // and the other list is used to check if ACKs for each packet were received
    while(i < totalPackets){
        
        char data[CHUNKSIZE + 1];
        data_packet packet;

        packet.packetId = i + 1;
        packet.retransmitted = 0;

        // Divide the txt file into chunks of 32 chars/bytes
        Substring(file_data, data, pos, CHUNKSIZE);
        strcpy(packet.data, data);   

        packetList.push_back(packet);

        ack_packet ack;
        ack.ack = 0;
        ack.packetId = packetList[i].packetId;
        ackQueue.push_back(ack);

        i++;
        pos = pos + CHUNKSIZE;
    }

    int localInc = 0;
    bool start = true;

	// Main loop for sending packets
    while(currentPacket < totalPackets){
        localInc = inc;


        // Sends the number of packages that fit the window size
        for(i = currentPacket - 1; i < windowSize + inc; i++){

            if(packetList[i].packetId == 0){
                break;
            }
            
            int sendPack = sendto(client_socket, &packetList[i], sizeof(packetList[i]), 0, (struct sockaddr *)&server, sizeof(server));
            
                    if (sendPack < 0)
                    {
                        printf("Error in sending data packet-%d\n", packetList[i].packetId);
                        exit(0);
                    }

                    printf("Packet with id %d sent.\n", packetList[i].packetId);


                    currentPacket ++;
        }

        // Starting the ACK thread
        if(start == true){
            ackThread = std::thread(receive_ack, client_socket);
            start = false;
            
        }

        // Loop that stops the parent while loop from sending more packets
        // Instead, waits until ACK has been received/inc has changed
        do{
                int a = 0;
        } while(localInc == inc);
    }
    
    ackThread.join();
    fclose(file);    
    close(client_socket);
    
    return 0;
}

// Function to receive ACKs from the server
void receive_ack(int server_socket){

    socklen_t client_addr_length = sizeof(server);
    int recvId = 0;

    while(packetsLeft > 0){

        // Schedules the packet_dropped function for one second after the thread has been activated
        signal(SIGALRM, packet_dropped);
        alarm(1);

        int pomerac = 0;

        //recieves ACK id from server
        int ack = recvfrom(server_socket, &recvId, sizeof(recvId), 0, (struct sockaddr *)&server, &client_addr_length);
        if (ack < 0)
        {
            printf("Error in receiving ack from server\n");
        }
        else{

            std::vector<ack_packet>::iterator iter;

            // Iterates through the ack list and looks for the element with the received id
            // Once it finds it, ack becomes 1, meaning ack was received for that specific element
            for(iter = ackQueue.begin(); iter < ackQueue.end(); iter++)
            {   
                if(iter->ack == 0 && iter->packetId == recvId){
                    iter->ack = 1;
                    printf("------------------------------------------\n");
                    printf("ACK for packet with id - %d received\n", recvId);
                    printf("------------------------------------------\n");
                    packetsLeft --;
                    
                }
                
            }


            // Check how many packets have been acknowledged starting with the first element
            for(int i = inc; i < inc + windowSize; i++){
                if(ackQueue[i].ack == 0) {

                    break;
                }

                pomerac++;
                
            }

            // Moves the window based on how many elements in a row were acknowledged
            // and at the same time breaks main from the do while loop and sends more packets
            inc += pomerac;

        }
    }

}

void packet_dropped(int server_socket){


   	// Loops through the ack list and looks for not acknowledged packets inside the window frame
    // Then changes transmitted to 1 and resends the package
    int resendId;
        for(int i = inc; i < inc + windowSize; i++){
            if(ackQueue[i].ack == 0){
                resendId = ackQueue[i].packetId;
                
                for(int k = 0; k < packetList.size(); k++){
                    if(packetList[k].packetId == resendId && packetList[k].retransmitted == 0){
                        packetList[k].retransmitted = 1;
                         int sendPack = sendto(client_socket, &packetList[k], sizeof(packetList[k]), 0, (struct sockaddr *)&server, sizeof(server));
                
                        if (sendPack < 0)
                        {
                            printf("Error in re-sending data packet-%d\n", packetList[k].packetId);
                            exit(0);
                        }

                        printf("Packet with id %d re-sent.\n", packetList[k].packetId);

                    }
                }
            }
        }
}
