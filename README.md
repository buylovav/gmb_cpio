# gmb_cpio
prebuild:
	sudo apt install libarchive-dev

build:
	mkdir -p build && cd build && cmake .. && make

launch:
	./cpio_demo

