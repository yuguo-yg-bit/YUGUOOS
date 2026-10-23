CC       := clang
LD       := ld.lld
AS       := clang

TARGET   := x86_64-unknown-none-elf

CFLAGS   := -target $(TARGET) \
            -ffreestanding \
            -nostdlib \
            -nostdinc \
            -fno-stack-protector \
            -fno-pic \
            -fno-pie \
            -mno-red-zone \
            -mno-mmx \
            -mno-sse \
            -mno-sse2 \
            -Wall \
            -Wextra \
            -O2 \
            -g \
            -I src

ASFLAGS  := -target $(TARGET) \
            -ffreestanding \
            -nostdinc \
            -fno-pic

LDFLAGS  := -nostdlib \
            -static \
            -z max-page-size=0x1000 \
            -T linker.ld

SRC_DIR  := src
BUILD_DIR:= build
ISO_DIR  := iso

C_SRCS   := $(wildcard $(SRC_DIR)/*.c)
ASM_SRCS := $(wildcard $(SRC_DIR)/*.S)

C_OBJS   := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.c.o, $(C_SRCS))
ASM_OBJS := $(patsubst $(SRC_DIR)/%.S, $(BUILD_DIR)/%.S.o, $(ASM_SRCS))

OBJS     := $(ASM_OBJS) $(C_OBJS)

KERNEL   := $(BUILD_DIR)/kernel.elf
ISO      := $(BUILD_DIR)/yuguocom.iso
GRUB_MKRESCUE := i686-elf-grub-mkrescue

.PHONY: all clean iso run debug

all: $(KERNEL)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.S.o: $(SRC_DIR)/%.S | $(BUILD_DIR)
	$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/%.c.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

iso: $(KERNEL)
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL) $(ISO_DIR)/boot/kernel.elf
	@echo 'set timeout=0' > $(ISO_DIR)/boot/grub/grub.cfg
	@echo 'menuentry "YuguoOS" {' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '    multiboot2 /boot/kernel.elf' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '    boot' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '}' >> $(ISO_DIR)/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o $(ISO) $(ISO_DIR)

run: $(ISO)
	qemu-system-x86_64 -cdrom $(ISO) -m 256M -vga vmware -serial stdio -no-reboot -no-shutdown

clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR)