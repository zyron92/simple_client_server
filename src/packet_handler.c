#include "packet_handler.h"

/*
 * Converting some bytes in an array of char to a value in integer
 */
static unsigned int bytesToInt(const unsigned char *readBytes, 
			       int positionStartByte, unsigned int numBytes)
{
	//If number of bytes is more than the size of 'int', 
	//we send an error code
	if(numBytes > sizeof(int)){
		fprintf(stderr, "Number of bytes to be converted to Int is \
too large\n");
		return ERR;
	}

	int currentInt = 0;
	unsigned int i;
	//we read one by one byte to produce a final value of int
	for(i=0; i<numBytes; i++){
	        currentInt = (currentInt * 256) + (unsigned char)
			(readBytes[(positionStartByte+i)]);
	}

	return currentInt;
}

/*
 * Initialization of a new header data structure
 * based on sequence number and command number
 */
static Header * init_header(int sequence, int command)
{

	//if the sequence number is invalid (negative seq. num. 
	//or seq. num. too big), we return a null pointer
	if(sequence < 0 || sequence > 65535){ //maximum seq number is 65535
		fprintf(stderr, "Sequence number is invalid\n");
		return NULL;
	}

	//if the command number is incorrect,  we return a null pointer
	if(command < 1 || command > 5){
		fprintf(stderr, "Command number is invalid\n");
		return NULL;
	}

	Header *new_header = calloc(1, sizeof(Header));
	
        new_header->version = VERSION;
        new_header->userId = USER_ID;
        new_header->sequence = sequence;

	//by default, the minimum size packet is 8bytes (size of header)
        new_header->length = 8;
        new_header->command = command;
	
	return new_header;
}

/*
 * Initialization of a header data structure from the read bytes
 */
Header * read_header(const unsigned char *readHeader)
{
	//if we dont have any bytes from the input,  we return a null pointer
	if(readHeader == NULL){
		fprintf(stderr, "No byte can be read\n");
		return NULL;
	}

	//if the version is incompatible, we return a null pointer
	if(readHeader[0]!=VERSION){
		fprintf(stderr, "Version number is invalid\n");
		return NULL;
	}
	
	//if the user id is incorrect, we return a null pointer
	if(readHeader[1]!=USER_ID){
		fprintf(stderr, "User Id is invalid\n");
		return NULL;
	}

	//if the sequence number is invalid (negative seq. num. or seq. num. too big), 
	//we return a null pointer
	int sequence = bytesToInt(readHeader,2,2);
	if(sequence < 0 || sequence > 65535){ //maximum seq number is 65535 because 2bytes
		fprintf(stderr, "Sequence number is invalid\n");
		return NULL;
	}

	//if the length is invalid (negative length or length too big), 
	//we return a null pointer
	int length = bytesToInt(readHeader,4,2);
	if(length < 0 || length > 65535){ //maximum length is 65535 because 2bytes
		fprintf(stderr, "Length is invalid\n");
		return NULL;
	}

	//if the command number is incorrect,  we return a null pointer
	int command = bytesToInt(readHeader,6,2);
	if(command < 1 || command > 5){
		fprintf(stderr, "Command number is invalid\n");
		return NULL;
	}

	Header *new_header = calloc(1,sizeof(Header));
	
        new_header->version = readHeader[0];
        new_header->userId = readHeader[1];
        new_header->sequence = sequence;
        new_header->length = length;
        new_header->command = command;
	
	return new_header;
}

/*
 * Free-ing header data structure
 */
void free_header(Header *headerToFree)
{
	free(headerToFree);
}

/*
 * Initialization of a new packet data structure with data and 
 * a header based on sequence number and command number
 */
Packet * init_packet(int sequence, int command, unsigned char *packetData, 
		     int packetDataLength)
{
	Header *new_header = init_header(sequence, command);
	//If the creation of header is already failed, we return a null pointer
	if(new_header == NULL){
		fprintf(stderr, "Creation of header failed\n");
		return NULL;
	}

	Packet *new_packet = calloc(1,sizeof(Packet));

	new_packet->packet_header = new_header;
	new_packet->packet_header->length += packetDataLength;

	new_packet->packet_data = packetData;

	return new_packet;
}

/*
 * Initialization of a packet data structure including header data structure 
 * and data part from the bytes read
 *
 * If we have more than 8bytes for the commands other than Data Delivery and
 * if its packet length information indicates the same number of bytes read,
 * this is not considered as an error and we will read only the header
 */
Packet * read_packet(unsigned char *readPacket, unsigned int packetLength)
{
	//if we dont have any bytes from the input,  we return a null pointer
	if(readPacket == NULL)
	{
		fprintf(stderr, "No byte can be read\n");
		return NULL;
	}

	//if we dont have min 8 bytes from the input,  we return a null pointer
	if(packetLength < 8)
	{
		fprintf(stderr, "No enough bytes to be read; Minimum 8bytes\n");
		return NULL;
	}

	Header *new_header = read_header(readPacket);
	//If the creation of header is already failed, we return a null pointer
	if(new_header == NULL){
		fprintf(stderr, "Creation of header failed\n");
		return NULL;
	}

	//If the read packet length is invalid, we return a null pointer
	if(packetLength != new_header->length){
		fprintf(stderr, "Error of read packet length\n");
		free_header(new_header);
		return NULL;
	}

	Packet *new_packet = calloc(1,sizeof(Packet));

	new_packet->packet_header = new_header;
	new_packet->packet_data = NULL;

	//Make a new copy of packet data and store them in the
	//structure
	if(packetLength > 8)
	{
		unsigned char *new_packet_data = calloc(packetLength-8,
							sizeof(unsigned char));
		memcpy(new_packet_data, readPacket+8, packetLength-8);
		new_packet->packet_data = new_packet_data;
	}

	return new_packet;
}

/*
 * Free-ing packet data structure including header
 * we won't free the packet data here because it is a reference to data
 * allocated outside the function
 */
void free_packet(Packet *packetToFree)
{
	free_header(packetToFree->packet_header);
	free(packetToFree);
}

/*
 * Free-ing packet data structure including header and data part
 */
void free_packet_for_read(Packet *packetToFree)
{
	free_header(packetToFree->packet_header);
	if(packetToFree->packet_data != NULL){
		free(packetToFree->packet_data);
	}
	free(packetToFree);
}

/*
 * Converting a packet data structure to bytes in order to send them over socket
 */
unsigned char * packetToBytes(Packet *ptrPacket)
{
	if(ptrPacket == NULL)
	{
		return NULL;
	}

	Header *ptrHeader = ptrPacket->packet_header;
		
	unsigned char *new_bytes = calloc(ptrHeader->length, 
					  sizeof(unsigned char));
	new_bytes[0] = ptrHeader->version;
	new_bytes[1] = ptrHeader->userId;
	new_bytes[2] = (unsigned char)((ptrHeader->sequence >> 8) & (0xFF));
	new_bytes[3] = (unsigned char)((ptrHeader->sequence) & (0xFF));
	new_bytes[4] = (unsigned char)((ptrHeader->length >> 8) & (0xFF));
	new_bytes[5] = (unsigned char)((ptrHeader->length) & (0xFF));
	new_bytes[6] = (unsigned char)((ptrHeader->command >> 8) & (0xFF));
	new_bytes[7] = (unsigned char)((ptrHeader->command) & (0xFF));

	memcpy(new_bytes + 8, ptrPacket->packet_data, ptrHeader->length - 8);
	return new_bytes;
}

/*
 * In case of invalid command according to the current state or
 * an error code, we send an error packet to start over automatically
 * the server but manually the client
 * 
 * Server goes back to initial state
 */
Packet *send_error_packet(unsigned int seq_num, int **current_state, 
			  int init_code)
{
	Packet *error_packet = init_packet(seq_num, ERROR, NULL, 0);
	**current_state = init_code;
	fprintf(stderr, "ERROR PACKET SENT, RESTART CLIENT\n");
	return error_packet;
}
