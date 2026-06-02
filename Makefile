.PHONY: all client server clean deb

all: client server

client:
	$(MAKE) -C client

server:
	$(MAKE) -C server

clean:
	$(MAKE) -C client clean
	$(MAKE) -C server clean
	$(MAKE) -C libmysyslog clean

deb:
	@echo "Debian package build target is prepared for Astra Linux"