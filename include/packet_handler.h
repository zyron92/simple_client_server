#ifndef __PACKET_HANDLER_H__
#define __PACKET_HANDLER_H__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ERR -1
#define OK 0

#define VERSION 0x04
#define USER_ID 0x08

#define CLIENT_HELLO 0x0001
#define SERVER_HELLO 0x0002
#define DATA_DELIVERY 0x0003
#define DATA_STORE 0x0004
#define ERROR 0x0005

//Data structure of Header
typedef struct _header{
	unsigned char version; //1byte
	unsigned char userId;  //1byte
	unsigned int sequence; //2bytes
	unsigned int length;   //2bytes
	unsigned int command;  //2bytes
} Header;

//Data structure of the whole Packet
typedef struct _packet{
        Header *packet_header;
	unsigned char *packet_data;
} Packet;


/*
 * Initialization of a new packet data structure with data and 
 * a header based on sequence number and command number
 */
Packet * init_packet(int sequence, int command, unsigned char *packetData, 
		     int packetDataLength);

/*
 * Initialization of only a header data structure from the read bytes
 */
Header * read_header(const unsigned char *readHeader);

/*
 * Free-ing header data structure
 */
void free_header(Header *headerToFree);

/*
 * Initialization of a packet data structure including header data structure 
 * and data part from the bytes read
 */
Packet * read_packet(unsigned char *readPacket, unsigned int packetLength);

/*
 * Free-ing packet data structure including header
 */
void free_packet(Packet *packetToFree);

/*
 * Free-ing packet data structure including header and data part
 */
void free_packet_for_read(Packet *packetToFree);

/*
 * Converting a packet data structure to bytes in order to send them over socket
 */
unsigned char * packetToBytes(Packet *ptrPacket);

/*
 * In case of invalid command according to the current state or
 * an error code, we send an error packet to start over automatically
 * the server but manually the client
 * 
 * Server goes back to initial state
 */
Packet *send_error_packet(unsigned int seq_num, int **current_state, 
			  int init_code);

#endif
