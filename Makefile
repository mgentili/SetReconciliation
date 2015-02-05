CXX=g++
RM=-rm -f

# Uncomment one of the following to switch between optimized and debug mode
#OPT= -DNDEBUG
OPT= -g -ggdb
CPPFLAGS=-std=c++11 -Wall $(OPT) 
LDFLAGS=-lprotobuf -lz -lboost_system -lboost_filesystem -lboost_program_options -lssl -lcrypto

COMMON_SRCS=hash_util.cpp IBLT_helpers.cpp jsoncpp.cpp
BASIC_IBLT_SRCS=basicIBLT_testing.cpp
MULTI_IBLT_SRCS=multiIBLT_testing.cpp
TABULATION_SRCS=tabulation_testing.cpp
BASIC_FIELD_SRCS=field_testing.cpp
FINGERPRINT_SRCS=fingerprint_testing.cpp
FILE_SYNC_SRCS=file_sync_testing.cpp file_sync.pb.cpp
STRATA_SRCS=StrataEstimator_testing.cpp
DIR_SYNC_SRCS=dir_sync_testing.cpp
NETWORK_SRCS=network_testing.cpp
HASH_SRCS=hash_testing.cpp
SRCS=$(COMMON_SRCS) $(BASIC_IBLT_SRCS) $(MULTI_IBLT_SRCS) $(TABULATION_SRCS) $(BASIC_FIELD_SRCS) $(FINGERPRINT_SRCS) $(FILE_SYNC_SRCS) $(STRATA_SRCS) $(DIR_SYNC_SRCS) $(NETWORK_SRCS) $(HASH_SRCS)
OBJS=$(SRCS:%.cpp=obj/%.o)

BASIC_IBLT=bin/basicIBLT_testing
MULTI_IBLT=bin/multiIBLT_testing
TABULATION=bin/tabulation_testing
BASIC_FIELD=bin/field_testing
FINGERPRINT=bin/fingerprint_testing
SYNC=bin/file_sync_testing
STRATA=bin/strata_testing
DIR_SYNC=bin/dir_sync_testing
NETWORK=bin/network_testing
HASH=bin/hash_testing
PROGRAMS=$(BASIC_IBLT) $(MULTI_IBLT) $(TABULATION) $(BASIC_FIELD) $(FINGERPRINT) $(SYNC) $(STRATA) $(DIR_SYNC) $(NETWORK) $(HASH)

.PHONY: default all tabulation basic_iblt multi_iblt field fingerprint sync strata network dir_sync hash
default: all
all: $(PROGRAMS)
tabulation: $(TABULATION)
basic_iblt: $(BASIC_IBLT)
multi_iblt: $(MULTI_IBLT)
field: $(BASIC_FIELD)
fingerprint: $(FINGERPRINT)
sync: $(SYNC)
strata: $(STRATA)
dir_sync: $(DIR_SYNC)
network: $(NETWORK)
hash: $(HASH)
obj/%.o: src/%.cpp
	$(CXX) $(CPPFLAGS) -c -MMD -MP $< -o $@

DEPS=$(OBJS:%.o=%.d)

#lala:;echo $(FINGERPRINT_SRCS:%.cpp=obj/%.o)
#protoc --proto_path=./src --cpp_out=./src src/file_sync.proto && mv src/file_sync.pb.cc src/file_sync.pb.cpp
#obj/%.d: src/%.cpp
#	$(CC) $(CFLAGS) -M -MF $@ -MT $(@:%.d=%.o) $^


$(BASIC_IBLT): $(COMMON_SRCS:%.cpp=obj/%.o) $(BASIC_IBLT_SRCS:%.cpp=obj/%.o)
	$(CXX) $^ $(LDFLAGS) -o $@

$(MULTI_IBLT): $(COMMON_SRCS:%.cpp=obj/%.o) $(MULTI_IBLT_SRCS:%.cpp=obj/%.o)
	$(CXX) $^ $(LDFLAGS) -o $@

$(TABULATION): $(COMMON_SRCS:%.cpp=obj/%.o) $(TABULATION_SRCS:%.cpp=obj/%.o)
	$(CXX) $^ $(LDFLAGS) -o $@

$(BASIC_FIELD): $(COMMON_SRCS:%.cpp=obj/%.o) $(BASIC_FIELD_SRCS:%.cpp=obj/%.o)
	$(CXX) $^ $(LDFLAGS) -o $@

$(FINGERPRINT): $(COMMON_SRCS:%.cpp=obj/%.o) $(FINGERPRINT_SRCS:%.cpp=obj/%.o)
	$(CXX) $^ $(LDFLAGS) -o $@

$(SYNC): $(COMMON_SRCS:%.cpp=obj/%.o) $(FILE_SYNC_SRCS:%.cpp=obj/%.o)
	$(CXX) $^ $(LDFLAGS) -o $@

$(STRATA): $(COMMON_SRCS:%.cpp=obj/%.o) $(STRATA_SRCS:%.cpp=obj/%.o)
	$(CXX) $^ $(LDFLAGS) -o $@

$(DIR_SYNC): $(COMMON_SRCS:%.cpp=obj/%.o) $(DIR_SYNC_SRCS:%.cpp=obj/%.o)
	$(CXX) $^ $(LDFLAGS) -o $@

$(NETWORK): $(COMMON_SRCS:%.cpp=obj/%.o) $(NETWORK_SRCS:%.cpp=obj/%.o)
	$(CXX) $^ $(LDFLAGS) -o $@

$(HASH): $(COMMON_SRCS:%.cpp=obj/%.o) $(HASH_SRCS:%.cpp=obj/%.o)
	$(CXX) $^ $(LDFLAGS) -o $@



clean:
	$(RM) $(OBJS) $(PROGRAMS) $(DEPS)

-include $(DEPS)
