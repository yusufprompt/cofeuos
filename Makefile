# Araçlar
CC = i686-elf-gcc
LD = i686-elf-ld
AS = nasm
OBJCOPY = objcopy

# Dosya Yolları
BOOT_DIR = boot
KERNEL_DIR = kernel

# Çıktı Dosyaları
# kernel_entry.o'nun EN BAŞTA olması hayati önem taşır!
OBJ = $(BOOT_DIR)/kernel_entry.o $(KERNEL_DIR)/kernel.o font.o

# Bayraklar
CFLAGS = -ffreestanding -O2 -Wall -Wextra -fno-stack-protector -m32
LDFLAGS = -Ttext 0x1000 --oformat binary

all: os-image.bin run

os-image.bin: boot.bin kernel.bin
	cat $^ > $@
	truncate -s 1M $@

kernel.bin: boot/kernel_entry.o kernel/kernel.o font.o
	$(LD) -o $@ $(LDFLAGS) $^


boot.bin: $(BOOT_DIR)/boot.asm
	$(AS) -f bin $< -o $@

$(BOOT_DIR)/kernel_entry.o: $(BOOT_DIR)/kernel_entry.asm
	$(AS) -f elf32 $< -o $@

$(KERNEL_DIR)/kernel.o: $(KERNEL_DIR)/kernel.c
	$(CC) $(CFLAGS) -c $< -o $@

font.o: font.psf
	$(OBJCOPY) -I binary -O elf32-i386 -B i386 $< $@


run: os-image.bin
	qemu-system-i386 -drive format=raw,file=$<

clean:
	rm -rf *.bin $(BOOT_DIR)/*.o $(KERNEL_DIR)/*.o os-image.bin font.o
