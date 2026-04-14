DEVICE=/dev/disk/by-label/RP2350
MOUNTPOINT=/mnt/pico

PROGRAM=wlanrelais

ifeq ($(OS),Windows_NT)
    # Windows (Git Bash / MSYS)
    DETECTED_OS := Windows
else
    # Linux
    DETECTED_OS := Linux
endif


all: configure compile copy

configure:
	mkdir -p build
	#cmake -DPICO_SDK_PATH=/git/pico-sdk -S . -B build
	cmake -S . -B build

compile:
	cmake --build build

copy:
ifeq ($(DETECTED_OS),Windows)
	if [ -d /f/ ] ; then cp build/$(PROGRAM).uf2 /f/; fi
else
	# Linux: Mount, Copy, Unmount
	# Benötigt ggf. sudo-Rechte für mount/umount
	@echo "Mounting $(DEVICE) to $(MOUNTPOINT)..."
	sudo mkdir -p $(MOUNTPOINT)
	sudo mount $(DEVICE) $(MOUNTPOINT)
	@echo "Copying firmware..."
	sudo cp build/$(PROGRAM).uf2 $(MOUNTPOINT)
	sudo sync
	@echo "Unmounting $(MOUNTPOINT)..."
	sudo umount $(MOUNTPOINT)
	@echo "Done."
endif



clean:
	rm -rf $(PROGRAM).* build build.ninja CMakeCache.txt .ninja_deps .ninja_log cmake_install.cmake pico-sdk
	mkdir -p build

fonts:
	cd resources; ./makefonts.sh
