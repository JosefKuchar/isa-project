CPPFLAGS = -std=c++2a -O2

# Get all .c files
SRCS = $(wildcard *.cc)
# Get corresponding .o files
OBJS := $(SRCS:%.cc=%.o)
# Exclude tfpt-client.cc and tftp-server.cc
OBJS := $(filter-out tftp-client.o tftp-server.o, $(OBJS))
# Get corresponding .d files
DEPS := $(SRCS:%.cc=%.d)

# These will run every time (not just when the files are newer)
.PHONY: run_client run_server clean zip test

all: tftp-client tftp-server

tftp-client: $(OBJS) tftp-client.o
	$(CXX) $(CXXFLAGS) -o $@ $^

tftp-server: $(OBJS) tftp-server.o
	$(CXX) $(CXXFLAGS) -o $@ $^

# Dependecies
%.o: %.cc %.d
	$(CC) -MT $@ -MMD -MP -MF $*.d $(CFLAGS) $(CPPFLAGS) -c $(OUTPUT_OPTION) $<
$(DEPS):
include $(wildcard $(DEPS))

clean:
	rm -f *.o *.d tftp-client tftp-server xkucha28.zip

run_client: tftp-client
	./tftp-client -h 127.0.0.1 -t test.txt

run_server: tftp-server
	./tftp-server files

zip: clean
	zip -r xkucha28.zip *
