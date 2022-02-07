TARGET := DnsClientDemo

MUDUOSRC := ../../../SimplifyMuduo/src/
PROTOSRC := ../../proto/
DNSCLIENTSRC := ./
MUDUOINC := ../../../SimplifyMuduo/include/
PROTOINC := ../../proto/
DNSCLIENTINC := ./
LIB := -lpthread -lprotobuf

CXX = g++
CPPFLAGS = -g -O2 -Wall -std=c++11
CPPFLAGS += $(addprefix -I,$(MUDUOINC))
CPPFLAGS += $(addprefix -I,$(DNSCLIENTINC))
CPPFLAGS += $(addprefix -I,$(PROTOINC))

FILETYPE = cc
OBJS := $(MUDUOSRC)*.$(FILETYPE)
OBJS += $(PROTOSRC)*.$(FILETYPE)
OBJS += $(DNSCLIENTSRC)*.$(FILETYPE)
#OBJS := $(addsuffix .o, $(basename $(wildcard $(SRC)*.$(FILETYPE))))
#objs : $(OBJS)

all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CXX) $(CPPFLAGS) -o $(TARGET) $(OBJS) $(LIB)

clean:
	-rm -f $(TARGET)
