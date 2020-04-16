
TGT=superdiskindex
OBJS=main3.o 
OBJS+=BitStream.o Buffer.o Config.o CRC.o DiskMap.o FluxData.o
OBJS+=Format.o FormatDiskIBM.o FormatDiskAmiga.o
OBJS+=MFM.o Helpers.o VirtualDisk.o

CXXFLAGS=-Wall

all: $(TGT)

clean:
	rm -f $(OBJS)
	rm -f $(TGT)

$(TGT): $(OBJS)
	$(CXX) -o $@ $(OBJS)

%.o: %.cc Makefile
	$(CXX) -MMD -Wall -O3 -g -o $@ -c $<

-include $(OBJS:%.o=%.d)
