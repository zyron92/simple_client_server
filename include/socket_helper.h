#ifndef __SOCKET_HELPER_H__
#define __SOCKET_HELPER_H__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "packet_handler.h"
#include "csapp.h"

/*
 * Creating a socket, binding it to localhost:12345 and preparing it
 */
int server_listening(void);

/*
 * Creating a socket, and connecting it to localhost:12345
 */
int client_connecting(void);

/*
 * Reading bytes from a file descriptor (server-side or client-side) 
 * for the whole packet
 */
int read_check_packet(int input_fd, Packet **readPacket);

#endif
