CPPFLAGS = -std=c++20 -O2

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
	$(CXX) $(CXXFLAGS) -o $@ $^ -lpthread

# Dependecies
%.o: %.cc %.d
	$(CC) -MT $@ -MMD -MP -MF $*.d $(CFLAGS) $(CPPFLAGS) -c $(OUTPUT_OPTION) $<
$(DEPS):
include $(wildcard $(DEPS))

clean:
	rm -f *.o *.d tftp-client tftp-server xkucha28.tar

run_client: tftp-client
	./tftp-client -p 1234 -h 127.0.0.1 -t test.txt -f tftp-client.cc

run_server: tftp-server
	./tftp-server -p 1234 files

tar: clean
	tar cvf xkucha28.tar *
