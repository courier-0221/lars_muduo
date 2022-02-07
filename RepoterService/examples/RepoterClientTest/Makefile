TARGET := RepoterClientDemo

MUDUOSRC := ../../../SimplifyMuduo/src/
PROTOSRC := ../../../Proto/
CLIENTSRC := ./
MUDUOINC := ../../../SimplifyMuduo/include/
PROTOINC := ../../../Proto/
CLIENTINC := ./
LIB := -lpthread -lprotobuf

CXX = g++
CPPFLAGS = -g -O2 -Wall -std=c++11
CPPFLAGS += $(addprefix -I,$(MUDUOINC))
CPPFLAGS += $(addprefix -I,$(CLIENTINC))
CPPFLAGS += $(addprefix -I,$(PROTOINC))

FILETYPE = cc
OBJS := $(MUDUOSRC)*.$(FILETYPE)
OBJS += $(PROTOSRC)*.$(FILETYPE)
OBJS += $(CLIENTSRC)*.$(FILETYPE)
#OBJS := $(addsuffix .o, $(basename $(wildcard $(SRC)*.$(FILETYPE))))
#objs : $(OBJS)

all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CXX) $(CPPFLAGS) -o $(TARGET) $(OBJS) $(LIB)

clean:
	-rm -f $(TARGET)
