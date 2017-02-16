# Project directories
SRC_DIR = ./src
INC_DIR = ./include
OBJ_DIR = ./obj

# Compilation options
CC = gcc
CFLAGS = -I$(INC_DIR) -Wextra -Wall
LDFLAGS = -lm -lpthread

# List of object file for client and server
OBJ_FILES_CLIENT = $(OBJ_DIR)/csapp.o $(OBJ_DIR)/packet_handler.o $(OBJ_DIR)/socket_helper.o
OBJ_FILES_SERVER = $(OBJ_DIR)/csapp.o $(OBJ_DIR)/packet_handler.o $(OBJ_DIR)/socket_helper.o

################################################################################

all : client server

# Generation of main object file for server program
$(OBJ_DIR)/server.o: $(SRC_DIR)/server.c
	$(CC) -c $(CFLAGS) $< -o $@ $(LDFLAGS)

# Generation of main object file for client program
$(OBJ_DIR)/client.o: $(SRC_DIR)/client.c
	$(CC) -c $(CFLAGS) $< -o $@ $(LDFLAGS)

# Generation of an object file from a source file and its specification file
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INC_DIR)/%.h
	$(CC) -c $(CFLAGS) $< -o $@ $(LDFLAGS)

# Linking of all object files for server program
server: $(OBJ_DIR)/server.o $(OBJ_FILES_SERVER)
	$(CC) -o $@ $^ $(LDFLAGS)

# Linking of all object files for client program
client: $(OBJ_DIR)/client.o $(OBJ_FILES_CLIENT)
	$(CC) -o $@ $^ $(LDFLAGS)

################################################################################

# Clean Up
.PHONY: clean
clean: clean_temp
	rm -f client server $(OBJ_DIR)/*.o
	rm -f server.out

.PHONY: clean_temp
clean_temp:
	rm -f ./*/*~
	rm -f ./*~

################################################################################

# Help
.PHONY: help
help:
	@echo "Options :-"
	@echo "1) make / make all"
	@echo "2) make clean"
	@echo "3) make clean_temp"
