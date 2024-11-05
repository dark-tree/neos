## NEOS

```bash
# First install QEMU
sudo apt install qemu-system

# Build all and generate the image with filesystem
make all

# Start the system in VM
make run

# Start the system in VM with attached GDB
make debug

# Generate the floppy disk image with filesystem
make image
```
