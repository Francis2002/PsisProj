CXX = gcc
CXXFLAGS = -Wall -Wno-unused-function
LIBS = -lncurses -lzmq 

.PHONY: clean run

TARGETS = client server roaches display

run: $(TARGETS)
	./human    # Assuming 'human' is your main target

clean:
	rm -f $(TARGETS)

client: LizardClient.c
	$(CXX) $(CXXFLAGS) -o client LizardClient.c json.c $(LIBS)

roaches: RoachesClient.c
	$(CXX) $(CXXFLAGS) -o roaches RoachesClient.c json.c $(LIBS)

server: LizardServer.c
	$(CXX) $(CXXFLAGS) -o server LizardServer.c json.c $(LIBS)

display: Display.c
	$(CXX) $(CXXFLAGS) -o display Display.c $(LIBS)

all: $(TARGETS)
