#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <unistd.h>

#include "packet_handler.h"
#include "socket_helper.h"

#define STATE_INIT 1 //initial state before connection setup or
                     //end of the current connection
#define STATE_HELLO 2 //state after receiving a Hello command
#define STATE_DELIVERY 3 //state after receiving a Data Delivery command
#define STATE_STORE 4 //state after receiving a Data Store command

/*
 * Creating packets to be sent back to client according to actual state of
 * the server
 */
void reply_from_server(int client_fd, int status_read, int *current_state, 
		       Packet *readPacket);

/*
 * Handling the received data, (DELIVERY and STORE)
 */
void data_handler(int *current_state, Packet *readPacket, 
		  unsigned char **bytesToSave, int *sizeBytesToSave);

/*
 * Writing the whole contents into a file
 */
void write_whole_file(const char *filename, char *file_contents, 
		      int filesize);

int main()
{
	//Preparing the server
	int server_fd = server_listening();
	if(server_fd == ERR){
		fprintf(stderr, "Error of establishing a server socket\n");
		return ERR;
	}

	int client_fd;
	struct sockaddr_in clientAddress;
	socklen_t addLength = (socklen_t)(sizeof(struct sockaddr_in));
	Packet *readPacket = NULL;
	int current_state = STATE_INIT;
	int status_read = ERR;

	//Information regarding file to store
	unsigned char *bytesToSave = NULL;
	int sizeBytesToSave = 0;
	
        while(1){
		//Renew the acceptation of connection
		if(current_state == STATE_INIT)
		{
			status_read = ERR;
			bytesToSave = NULL;

			//Accepting a new request from the client
			client_fd = accept(server_fd, 
					   (struct sockaddr *)(&clientAddress), 
					   &addLength);
			if(client_fd < 0){
				fprintf(stderr, "Error of accepting a new \
connection request. Retrying!\n");
				break;
			}
		}

		//At each iteration, we ensure the server
		//to receive a packet from the client, but we may not
		//get any packet or incurate packet
		status_read = read_check_packet(client_fd, 
						&readPacket);
		
		//We sent packets to client, or we skip it incase of
		//no packet received before this sending process
		reply_from_server(client_fd, status_read, &current_state, 
				  readPacket);

		//Handling the data if the reading process passed
		if(status_read == OK)
		{
			data_handler(&current_state, readPacket, 
				     &bytesToSave, &sizeBytesToSave);
			if(readPacket != NULL)
			{
				free_packet_for_read(readPacket);
			}
		}

		//Close the current connection after finishing
		//the job for one client or if there is any error
		if(current_state == STATE_INIT)
		{
			status_read = ERR;
			bytesToSave = NULL;
			close(client_fd);
		}
	}

	return OK;
}

/*
 * ACTIONS ACCORDING TO STATE OF SERVER
 *
 * Creating packets to be sent back to client according to actual state of
 * the server
 */
void reply_from_server(int client_fd, int status_read, int *current_state, 
		       Packet *readPacket)
{
	if(readPacket == NULL){
		return;
	}

	Header *readPacketHeader = readPacket->packet_header;

	Packet *packetToSend;

	//if process of reading bytes from the client failed
	if(status_read == ERR){
		packetToSend = send_error_packet(
			readPacketHeader->sequence, &current_state,
			(int)(STATE_INIT));
	}
	//if process of reading bytes from the client passed
	else{
		//We test the current command according to the 
		//current state of server
		switch (*current_state) {
		case STATE_INIT:
			//INITIAL STATE
			switch (readPacketHeader->command) {
			case CLIENT_HELLO:
				packetToSend = init_packet(
					readPacketHeader->sequence, 
					SERVER_HELLO, NULL, 0);
				*current_state = STATE_HELLO;
				break;
			default:
				packetToSend = send_error_packet(
					readPacketHeader->sequence, 
					&current_state, (int)(STATE_INIT));
				break;
			}
			break;
		case STATE_HELLO:
			//STATE after receiving CLIENT HELLO
			switch (readPacketHeader->command) {
			case DATA_DELIVERY:
				packetToSend = NULL;
				*current_state = STATE_DELIVERY;
				break;
			default:
				packetToSend = send_error_packet(
					readPacketHeader->sequence, 
					&current_state,(int)(STATE_INIT));
				break;
			}
			break;
		case STATE_DELIVERY:
			//STATE after receiving DATA DELIVERY
			switch (readPacketHeader->command) {
			case DATA_DELIVERY: //If there is still other packet
				packetToSend = NULL;
				*current_state = STATE_DELIVERY;
				break;
			case DATA_STORE:
				fprintf(stderr, "DATA DELIVERY DONE\n");
				packetToSend = NULL;
				*current_state = STATE_STORE;
				break;
			default:
				packetToSend = send_error_packet(
					readPacketHeader->sequence, 
					&current_state,(int)(STATE_INIT));
				break;
			}
			break;
		default:
			//unrecognised error
			packetToSend = send_error_packet(
				readPacketHeader->sequence, &current_state,
				(int)(STATE_INIT));
			break;
		}
	}

	//If there is packet to send (ERROR OR SERVER HELLO), we sent them
	if(packetToSend != NULL){
		//8 bytes data sent
		char *bytesToSend = (char *)(packetToBytes(packetToSend));
		Rio_writen(client_fd, bytesToSend, 8);
		free_packet(packetToSend);
		free(bytesToSend);
	}
}

/*
 * Handling the received data, (DELIVERY and STORE)
 */
void data_handler(int *current_state, Packet *readPacket, 
		  unsigned char **bytesToSave, int *sizeBytesToSave)
{
	/*
	 * Extra task related to saving DATA according to state
	 */
	//If current state is STATE_DELIVERY, we memorise the fragments of data
	if(*current_state == STATE_DELIVERY){
		int sizeActualPacketData = readPacket->packet_header->length-8;
		*sizeBytesToSave += sizeActualPacketData;
		if(*sizeBytesToSave > 0){
			*bytesToSave = realloc(*bytesToSave, 
						 (*sizeBytesToSave)*
						 sizeof(unsigned char));
			memcpy(*bytesToSave + (*sizeBytesToSave) - 
			       sizeActualPacketData, 
			       readPacket->packet_data,
			       sizeActualPacketData);
		}else{
			fprintf(stderr, "DATA DELIVERY DONE, BUT ZERO DATA\n");
		}
	}
	//OTHER STATES
	else{
		//If current state is state_store, meaning
		//we will save the delivered data into "server.out"
		if(*current_state == STATE_STORE){
			write_whole_file("server.out", 
					 (char *)(*bytesToSave), 
					 *sizeBytesToSave);
			*current_state = STATE_INIT;
			fprintf(stderr, "SAVING DATA DONE into server.out\n");
		}
		
		//In any case or state, we try to free the delivered data
		//as we are assured to have the data stored or
		//in case of error, client needs to restart the whole
		//programme
		if(*bytesToSave != NULL){
			*sizeBytesToSave = 0;
			free(*bytesToSave);
		}
	}
}

/*
 * Writing the whole contents into a file
 */
void write_whole_file(const char *filename, char *file_contents, 
		      int filesize)
{
	FILE *new_file = fopen(filename, "wb");

	fwrite(file_contents, sizeof(char), filesize, new_file);
	fclose(new_file);
}
