CC      = gcc
CFLAGS  = -Wall -Wextra -pthread -g
TARGETS = sensor_server sensor_client
 
all: $(TARGETS)
 
sensor_server: sensor_server.c
	$(CC) $(CFLAGS) -o $@ $< -lm
 
sensor_client: sensor_client.c
	$(CC) $(CFLAGS) -o $@ $<
 
server: sensor_server
 
client: sensor_client
 
clean:
	rm -f $(TARGETS) *.o
 
.PHONY: all server client clean
 