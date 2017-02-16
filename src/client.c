#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "packet_handler.h"
#include "socket_helper.h"

#define STATE_INIT 1 //initial state before connection setup
#define STATE_HELLO 2 //state after sending a Hello command or Delivery command
#define STATE_DELIVERY 3 //state after sending THE WHOLE DATA
#define STATE_STORE 4 //state after sending a Data Store command

#define MAX_DATA_SIZE 21880 //65527 //not including 8bytes of header

/*
 * Reading the whole file and send them as strings
 */
char *read_whole_file(const char *filename, int *filesize);

/*
 * Creating packets to be sent to server according to actual state of
 * the client
 */
void reply_from_client(int client_fd, int status_read, int *current_state, 
		       int *current_sequence, Packet *readPacket, 
		       unsigned char *dataToSend, int packetDataLength,
		       int numberDeliveriesRemaining);

int main(int argc, char **argv)
{
	
	//Test if we have argument of filename
	if(argc != 2){
		fprintf(stderr, 
			"#Error: require only one argument - \
the filename\n");
		return ERR;
	}

	//Reading the input file
	int file_size = 0;
	char *fileContents = read_whole_file(argv[1], &file_size);
	
	//New client socket
	int client_fd = client_connecting();
	if(client_fd == ERR){
		fprintf(stderr, "Error of establishing a client socket\n");
		free(fileContents);
		return ERR;
	}

	Packet *readPacket = NULL;
	int current_state = STATE_INIT;
	int status_read = OK;

	//Generation of a random sequence number
	srand(time(NULL));
	int current_sequence = rand()%(65535/2);
	
	//CLIENT HELLO
	reply_from_client(client_fd, status_read, &current_state, 
			  &current_sequence, readPacket, 
			  (unsigned char *)(fileContents), 0, 0);

	//WAITING FOR SERVER HELLO
	status_read = read_check_packet(client_fd, &readPacket);
	
	//--------------- DATA DELIVERY ------------------------//
        int i;

	int numberSending = (int)(file_size/MAX_DATA_SIZE);
	int numberDeliveriesRemaining = numberSending;
	if((file_size%MAX_DATA_SIZE) != 0 ){
		numberDeliveriesRemaining++;
	}

	//Delivery of all the fragments of file except the last one
	for(i=0; i<numberSending; i++){
		reply_from_client(client_fd, status_read, &current_state, 
				  &current_sequence, readPacket, 
				  (unsigned char *)(fileContents + 
						    i*MAX_DATA_SIZE),
				  MAX_DATA_SIZE, numberDeliveriesRemaining);
		numberDeliveriesRemaining--;
	}

	//Delivery of the last fragment
	if(numberDeliveriesRemaining == 1)
	{
		reply_from_client(client_fd, status_read, &current_state, 
				  &current_sequence, readPacket, 
				  (unsigned char *)(fileContents + 
						    numberSending
						    *MAX_DATA_SIZE),
				  file_size%MAX_DATA_SIZE, 
				  numberDeliveriesRemaining);	
	}
	//-------------- END OF DATA DELIVERY ------------------//

	free_packet(readPacket);
	readPacket = NULL;

	//DATA STORE
	reply_from_client(client_fd, status_read, &current_state, 
			  &current_sequence, readPacket, 
			  (unsigned char *)(fileContents), 0, 0);
		
	free(fileContents);
	return OK;
}

/*
 * Reading the whole file and send them as strings
 */
char *read_whole_file(const char *filename, int *filesize)
{
	*filesize = 0;

	FILE *new_file = fopen(filename, "rb");
	if(new_file == NULL){
		fprintf(stderr, "ERROR OF OPENING FILE\n");
		exit(1);
	}

	//Looking for the size of file
	fseek(new_file, 0, SEEK_END);
	long file_size = ftell(new_file);
	*filesize = file_size;
	fseek(new_file, 0, SEEK_SET);

	//Reading the whole file and put them onto a string
	char *output_string = calloc(file_size, sizeof(char));
	fread(output_string, file_size, 1, new_file);
	
	fclose(new_file);

	return output_string;
}

/*
 * ACTIONS ACCORDING TO STATE OF CLIENT
 *
 * Creating packets to be sent to server according to actual state of
 * the client
 */
void reply_from_client(int client_fd, int status_read, int *current_state, 
		       int *current_sequence, Packet *readPacket, 
		       unsigned char *dataToSend, int packetDataLength,
		       int numberDeliveriesRemaining)
{
	//Increasing sequence number
	(*current_sequence) += 1;

	Header *readPacketHeader;
	if(readPacket == NULL){
		readPacketHeader = NULL;
	}else{
		readPacketHeader = readPacket->packet_header;
	}

	Packet *packetToSend;

	if(status_read == ERR){
		packetToSend = send_error_packet(
			readPacketHeader->sequence, 
			&current_state,(int)(STATE_INIT));
	}else{
		//We test the current command according to the 
		//current state of client
		switch (*current_state) {
		case STATE_INIT:
			//INITIAL STATE
			packetToSend = init_packet(*current_sequence, 
						   CLIENT_HELLO, NULL, 
						   0);
			*current_state = STATE_HELLO;
			break;
		case STATE_HELLO:
			//STATE after sending CLIENT HELLO
			switch (readPacketHeader->command) {
			case SERVER_HELLO:
				packetToSend = init_packet(*current_sequence, 
							   DATA_DELIVERY, 
							   dataToSend,
							   packetDataLength);
				
				if(numberDeliveriesRemaining == 1){
					*current_state = STATE_DELIVERY;
				}
				break;
			default:
				packetToSend = send_error_packet(
					readPacketHeader->sequence, 
					&current_state,(int)(STATE_INIT));
				break;
			}
			break;
		case STATE_DELIVERY:
			//STATE after sending THE WHOLE DATA
			if(readPacketHeader == NULL){
				packetToSend = init_packet(*current_sequence, 
							   DATA_STORE, 
							   NULL,
							   0);
				*current_state = STATE_STORE;
				break;
			}else{
				packetToSend = send_error_packet(
					readPacketHeader->sequence, 
					&current_state,(int)(STATE_INIT));
			}
		default:
			packetToSend = send_error_packet(
				readPacketHeader->sequence, 
				&current_state,(int)(STATE_INIT));
			break;
		}
	}

	if(packetToSend != NULL){
		char *bytesToBeSent = (char *)(packetToBytes(packetToSend));
                Rio_writen(client_fd, bytesToBeSent, 8+packetDataLength);
		free_packet(packetToSend);
		free(bytesToBeSent);
	}
}
