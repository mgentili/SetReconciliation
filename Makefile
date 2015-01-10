CXX=g++
RM=-rm -f
CPPFLAGS=-std=c++11 -Wall -ggdb
LDFLAGS=-lprotobuf -lz

COMMON_SRCS=MurmurHash2.cpp IBLT_helpers.cpp
BASIC_IBLT_SRCS=basicIBLT_testing.cpp
MULTI_IBLT_SRCS=multiIBLT_testing.cpp
TABULATION_SRCS=tabulation_testing.cpp
BASIC_FIELD_SRCS=field_testing.cpp
FINGERPRINT_SRCS=fingerprint_testing.cpp
FILE_SYNC_SRCS=file_sync_testing.cpp file_sync.pb.cpp
SRCS=$(COMMON_SRCS) $(BASIC_IBLT_SRCS) $(MULTI_IBLT_SRCS) $(TABULATION_SRCS) $(BASIC_FIELD_SRCS) $(FINGERPRINT_SRCS) $(FILE_SYNC_SRCS)
OBJS=$(SRCS:%.cpp=obj/%.o)

BASIC_IBLT=bin/basic_testing
MULTI_IBLT=bin/multi_testing
TABULATION=bin/tabulation_testing
BASIC_FIELD=bin/field_testing
FINGERPRINT=bin/fingerprint_testing
SYNC=bin/file_sync_testing
PROGRAMS=$(BASIC_IBLT) $(MULTI_IBLT) $(TABULATION) $(BASIC_FIELD) $(FINGERPRINT) $(SYNC)

.PHONY: default all tabulation basic_iblt multi_iblt field fingerprint sync
default: all
all: $(PROGRAMS)
tabulation: $(TABULATION)
basic_iblt: $(BASIC_IBLT)
multi_iblt: $(MULTI_IBLT)
field: $(BASIC_FIELD)
fingerprint: $(FINGERPRINT)
sync: $(SYNC)

obj/%.o: src/%.cpp
	$(CXX) $(CPPFLAGS) -c -MMD -MP $< -o $@

DEPS=$(OBJS:%.o=%.d)

#lala:;echo $(FINGERPRINT_SRCS:%.cpp=obj/%.o)

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

clean:
	$(RM) $(OBJS) $(PROGRAMS) $(DEPS)

-include $(DEPS)