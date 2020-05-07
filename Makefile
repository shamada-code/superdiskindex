
VERSION1D=$(shell cat VERSION_L0).$(shell cat VERSION_L1)
VERSION2D=$(shell cat VERSION_L0).$(shell printf "%02d" `cat VERSION_L1`)

TGT=superdiskindex
OBJS=main.o 
OBJS+=BitStream.o Buffer.o Config.o CRC.o DiskMap.o FluxData.o
OBJS+=Format.o FormatDiskIBM.o FormatDiskAmiga.o
OBJS+=FormatDiskC64_1541.o FormatDiskC64_1581.o
OBJS+=MFM.o Helpers.o VirtualDisk.o
OBJS+=JsonState.o
OBJS+=GCR.o
OBJS+=CBM.o
OBJS+=DiskLayout.o
OBJS+=TextureTGA.o
OBJS+=G64.o

CXXFLAGS=-Wall

all: folders $(TGT)

folders:
	mkdir -p tmp

install: all
	install -D --mode=755 --preserve-timestamps --strip $(TGT) /usr/local/bin/$(TGT)

dist_deb: clean all
	mkdir -p releases
	mkdir -p tmp.deb/superdiskindex_$(VERSION1D)-1/DEBIAN
	cat contrib/DEBIAN/control | sed "s/@@V@@/$(VERSION1D)/" > tmp.deb/superdiskindex_$(VERSION1D)-1/DEBIAN/control
	mkdir -p tmp.deb/superdiskindex_$(VERSION1D)-1/usr/local/bin
	cp $(TGT) tmp.deb/superdiskindex_$(VERSION1D)-1/usr/local/bin/
	cd tmp.deb && dpkg-deb --build superdiskindex_$(VERSION1D)-1
	cd ..
	cp tmp.deb/superdiskindex_$(VERSION1D)-1.deb releases/
	rm -rf tmp.deb

dist_upload: dist_deb
	./upload.sh

clean:
	rm -f $(OBJS:%.o=tmp/%.o)
	rm -f $(OBJS:%.o=tmp/%.d)
	rm -f $(TGT)

$(TGT): $(OBJS:%.o=tmp/%.o)
	$(CXX) -o $@ $(OBJS:%.o=tmp/%.o)

tmp/%.o: %.cc Makefile
	$(CXX) -MMD -Wall -Wextra -O3 -D_TOOLVERSION=$(VERSION2D) -o $@ -c $<

-include $(OBJS:%.o=tmp/%.d)
