# Embedded Linux + Yocto

## Overview

This document contains beginner to advanced Embedded Linux and Yocto interview preparation topics.

---

# 1. Embedded Linux Fundamentals

## Topics

- Linux boot process
- Cross compilation
- Toolchains
- BusyBox
- Init systems
- Root filesystem
- Kernel architecture

## Interview Questions

- Explain Embedded Linux boot flow.
- What is cross-compilation?
- Difference between kernel space and user space?

---

# 2. Bootloader and Boot Process

## Topics

- ROM code
- SPL
- U-Boot
- Kernel loading
- Device Tree
- initramfs

## Interview Questions

- Explain U-Boot boot sequence.
- Difference between zImage and uImage?

---

# 3. Linux Kernel

## Topics

- Kernel modules
- Scheduling
- Interrupts
- Memory management
- Synchronization
- IPC

## Interview Questions

- Difference between mutex and semaphore?
- What is context switching?

---

# 4. Device Drivers

## Topics

- Character drivers
- GPIO
- I2C/SPI/UART
- Interrupt handling
- DMA

## Interview Questions

- Explain top-half and bottom-half.
- What is ioctl?

---

# 5. Device Tree

## Topics

- DTS/DTB
- Compatible strings
- GPIO bindings
- Clock bindings

## Interview Questions

- Why is Device Tree needed?
- How does kernel match DT node to driver?

---

# 6. Yocto Fundamentals

## Topics

- Poky
- BitBake
- Metadata
- Layers
- Recipes

## Interview Questions

- Difference between Yocto and Buildroot?
- What is BitBake?

---

# 7. Yocto Recipes

## Topics

- .bb files
- .bbappend
- DEPENDS
- RDEPENDS
- PACKAGECONFIG

## Interview Questions

- Explain recipe lifecycle.
- Difference between DEPENDS and RDEPENDS?

---

# 8. Yocto Layers

## Topics

- Layer priority
- Custom layers
- Layer dependencies

## Interview Questions

- What is bbappend?
- Why use separate layers?

---

# 9. Image Customization

## Topics

- Custom images
- IMAGE_INSTALL
- packagegroups
- systemd services

---

# 10. OTA Systems

## Topics

- RAUC
- Mender
- OSTree
- A/B updates

---

# 11. Security

## Topics

- Secure boot
- OP-TEE
- dm-verity
- TPM
- SELinux

---

# 12. Debugging

## Topics

- dmesg
- strace
- perf
- ftrace
- trace-cmd

---

# 13. CI/CD

## Topics

- kas
- GitHub Actions
- Docker builds
- Shared sstate cache

---

# 14. Advanced Topics

## Topics

- Real-time Linux
- Virtualization
- Containers
- Edge AI
- FPGA acceleration
- Multi-core optimization

---

# 15. Hands-On Practice

Recommended exercises:

- Build custom Yocto image
- Create recipe
- Create layer
- Add kernel patch
- Modify Device Tree
- Add systemd service
- Implement OTA workflow
