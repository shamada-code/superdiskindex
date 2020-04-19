
TGT=superdiskindex
OBJS=main3.o 
OBJS+=BitStream.o Buffer.o Config.o CRC.o DiskMap.o FluxData.o
OBJS+=Format.o FormatDiskIBM.o FormatDiskAmiga.o
OBJS+=MFM.o Helpers.o VirtualDisk.o

CXXFLAGS=-Wall

all: folders $(TGT)

folders:
	mkdir -p tmp

dist_deb: clean all
	mkdir -p releases
	mkdir -p tmp.deb/superdiskindex_0.2-1
	cp -rv contrib/DEBIAN tmp.deb/superdiskindex_0.2-1
	mkdir -p tmp.deb/superdiskindex_0.2-1/usr/local/bin
	cp $(TGT) tmp.deb/superdiskindex_0.2-1/usr/local/bin/
	cd tmp.deb && dpkg-deb --build superdiskindex_0.2-1
	cd ..
	cp tmp.deb/superdiskindex_0.2-1.deb releases/
	rm -rf tmp.deb

clean:
	rm -f $(OBJS:%.o=tmp/%.o)
	rm -f $(OBJS:%.o=tmp/%.d)
	rm -f $(TGT)

$(TGT): $(OBJS:%.o=tmp/%.o)
	$(CXX) -o $@ $(OBJS:%.o=tmp/%.o)

tmp/%.o: %.cc Makefile
	$(CXX) -MMD -Wall -O3 -o $@ -c $<

-include $(OBJS:%.o=tmp/%.d)
