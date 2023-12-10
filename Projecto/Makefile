CC = gcc
CFLAGS = -lzmq -lncurses -g -L /opt/homebrew/Cellar/zeromq/4.3.5_1/lib -I /opt/homebrew/Cellar/zeromq/4.3.5_1/include

# Include the current directory for header files
INCLUDE_DIR = -I.

SERVER_SRC = server_folder/LizardServer.c
LIZARD_SRC = lizard_clients_folder/LizardClient.c
ROACHES_SRC = roach_clients_folder/RoachesClient.c
DISPLAY_SRC = display_folder/Display.c
JSON_SRC = display_folder/json.c

#output directory is the current directory
OUTPUT_DIR = .

all: server lizard roaches display

server: $(SERVER_SRC)   
	$(CC) $(SERVER_SRC) $(JSON_SRC) -o server $(CFLAGS) $(INCLUDE_DIR)

lizard: $(LIZARD_SRC)   
	$(CC) $(LIZARD_SRC)  $(JSON_SRC)  -o lizard $(CFLAGS) $(INCLUDE_DIR)

roaches: $(ROACHES_SRC)   
	$(CC) $(ROACHES_SRC) $(JSON_SRC)   -o roaches $(CFLAGS) $(INCLUDE_DIR)

display: $(DISPLAY_SRC)   
	$(CC) $(DISPLAY_SRC) $(JSON_SRC)   -o display $(CFLAGS) $(INCLUDE_DIR)

clean:
	rm -f $(OUTPUT_DIR)/server $(OUTPUT_DIR)/lizard $(OUTPUT_DIR)/roaches $(OUTPUT_DIR)/display 

re: clean all