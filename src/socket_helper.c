#include "socket_helper.h"

/*
 * Creating a socket, binding it to localhost:12345 and preparing it
 */
int server_listening(void)
{	//STEP 1 : Creating a socket descriptor for the server
	int resultSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(resultSocket < 0){
		fprintf(stderr, "Error of establishing a server socket \
[socket()]\n");
		return ERR;
	}

	//Removing the binding error for a faster debugging
	int optValue = 1;
	if(setsockopt(resultSocket, SOL_SOCKET, SO_REUSEADDR, 
		      (const void *)(&optValue), sizeof(int)) < 0){
		fprintf(stderr, "Error of establishing a server socket \
[setsockopt()]\n");
		return ERR;
	}

	//STEP 2 : Binding process
	//we use calloc to initialize the structure with zeros
	struct sockaddr_in *serverAddress = calloc(1, 
						   sizeof(struct sockaddr_in));
	serverAddress->sin_family = AF_INET;
	serverAddress->sin_port = htons(12345);
	inet_pton(AF_INET, "127.0.0.1", &(serverAddress->sin_addr));
	if(bind(resultSocket, (struct sockaddr *)(serverAddress), 
		sizeof(struct sockaddr_in)) < 0){
		free(serverAddress);
		fprintf(stderr, "Error of establishing a server socket \
[bind()]\n");
		return ERR;
	}

	//STEP 3 : Making the socket ready to accept incomming connection
	if(listen(resultSocket, 1) < 0){
		free(serverAddress);
		fprintf(stderr, "Error of establishing a server socket \
[listen()]\n");
		return ERR;
	}

	free(serverAddress);
	return resultSocket;
}

/*
 * Creating a socket, and connecting it to localhost:12345
 */
int client_connecting(void)
{
	//STEP 1 : Creating a socket descriptor for the client
	int resultSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(resultSocket < 0){
		fprintf(stderr, "Error of establishing a client socket \
[socket()]\n");
		return ERR;
	}

	//STEP 2 : Connecting process to the server
	//we use calloc to initialize the structure with zeros
	struct sockaddr_in *serverAddress = calloc(1, 
						   sizeof(struct sockaddr_in));
	serverAddress->sin_family = AF_INET;
	serverAddress->sin_port = htons(12345);
	inet_pton(AF_INET, "127.0.0.1", &(serverAddress->sin_addr));
	if(connect(resultSocket, (struct sockaddr *)(serverAddress), 
		   sizeof(struct sockaddr_in)) < 0){
		free(serverAddress);
		fprintf(stderr, "Error of establishing a client socket \
[bind()]\n");
		return ERR;
	}

	free(serverAddress);
	return resultSocket;
}

/*
 * Reading bytes from a file descriptor (server-side or client-side) 
 * for the whole packet
 */
int read_check_packet(int input_fd, Packet **readPacket)
{
	//Values by default
	*readPacket = NULL;

	//Buffer containing the whole packet
	char *buffer = calloc(8, sizeof(char));
	size_t numReadBytes;

	//Read the header first
	numReadBytes = Rio_readn(input_fd, buffer, 8);
	if(numReadBytes != 8){
		free(buffer);
		fprintf(stderr, "Error of reading packet header\n");
		return ERR;
	}

	//Accessing the length of packet found in the header
	//for the length of data
	Header *readHeader = read_header((unsigned char *)(buffer));
	unsigned int dataLength = readHeader->length - 8;

	free_header(readHeader);

	if(dataLength > 0){
		char *buffer_data = calloc(dataLength, sizeof(char));
		numReadBytes += Rio_readn(input_fd, buffer_data, dataLength);  	
		if(numReadBytes-8 != dataLength){
			free(buffer_data);
			fprintf(stderr, "Error of reading packet data\n");
			return ERR;
		}

		buffer = realloc(buffer, 
				 numReadBytes*sizeof(char));
		memcpy(buffer+8, buffer_data, numReadBytes-8);
		free(buffer_data);
	}
	
	//Interpretating the read bytes into a packet data structure
	*readPacket = read_packet((unsigned char *)buffer, numReadBytes);
	free(buffer);
	if(readPacket == NULL){
		fprintf(stderr, "Error of reading the paket\n");
		return ERR;
	}
	return OK;
}
