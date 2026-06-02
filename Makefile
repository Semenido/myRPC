.PHONY: all client server lib clean deb

all: lib client server

lib:
	$(MAKE) -C libmysyslog

client:
	$(MAKE) -C client

server:
	$(MAKE) -C server

clean:
	$(MAKE) -C client clean
	$(MAKE) -C server clean
	$(MAKE) -C libmysyslog clean
	rm -rf build
	rm -f *.deb

deb: all
	mkdir -p build/myrpc-client/usr/bin
	mkdir -p build/myrpc-client/DEBIAN
	cp client/myRPC-client build/myrpc-client/usr/bin/
	printf "Package: myrpc-client\nVersion: 1.0\nSection: utils\nPriority: optional\nArchitecture: amd64\nMaintainer: Semenido\nDescription: myRPC client\n" > build/myrpc-client/DEBIAN/control
	dpkg-deb --build build/myrpc-client

	mkdir -p build/myrpc-server/usr/bin
	mkdir -p build/myrpc-server/etc/myRPC
	mkdir -p build/myrpc-server/DEBIAN
	cp server/myRPC-server build/myrpc-server/usr/bin/
	cp configs/myRPC.conf build/myrpc-server/etc/myRPC/
	cp configs/users.conf build/myrpc-server/etc/myRPC/
	printf "Package: myrpc-server\nVersion: 1.0\nSection: utils\nPriority: optional\nArchitecture: amd64\nMaintainer: Semenido\nDescription: myRPC server\n" > build/myrpc-server/DEBIAN/control
	dpkg-deb --build build/myrpc-server