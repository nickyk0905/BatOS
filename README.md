# BatOS

**BatOS** is a custom **32-bit x86 operating system built from scratch using C and Assembly**.

The project focuses on understanding the fundamentals of operating-system development by implementing a bootable kernel, terminal, command-line shell, hardware interaction, and an in-memory hierarchical file system.

The system is cross-compiled using a Make-based build process and can be booted and tested using **QEMU**.

---

## 🚀 Features

* Custom 32-bit x86 kernel
* Bootloader and kernel initialization
* Interactive command-line shell
* Terminal with scrolling support
* Keyboard driver
* Mouse driver
* In-memory hierarchical file system
* File and directory operations
* Bootable ISO generation
* QEMU-based execution and testing

### Supported File System Commands

| Command | Description                  |
| ------- | ---------------------------- |
| `ls`    | List files and directories   |
| `cd`    | Change the current directory |
| `mkdir` | Create a directory           |
| `touch` | Create a file                |
| `cat`   | Display file contents        |
| `rm`    | Remove a file or directory   |
| `cp`    | Copy files                   |
| `mv`    | Move files                   |

---

## 🏗️ Architecture

The project is structured around several core OS components:

```text
                    BatOS
                      │
              ┌───────┴───────┐
              │                │
          Bootloader         Kernel
                               │
                ┌──────────────┼──────────────┐
                │              │              │
             Terminal        Shell         Drivers
                │              │          ┌────┴────┐
                │              │       Keyboard   Mouse
                │              │
                └──────────────┼──────────────
                               │
                         File System
                               │
                    In-Memory Hierarchy
```

---

## 📁 Project Structure

```text
BatOS/
│
├── boot/
│   └── Bootloader / boot-related source
│
├── kernel/
│   ├── Kernel source
│   ├── Shell
│   ├── Terminal
│   ├── File System
│   ├── Keyboard Driver
│   └── Mouse Driver
│
├── iso/
│   └── boot/
│
├── Makefile
├── linker.ld
└── BatOS.iso
```

---

## 🛠️ Technologies Used

* **C** — Kernel and system-level functionality
* **Assembly** — Low-level boot and hardware initialization
* **Make** — Build automation
* **x86 Architecture** — Target architecture
* **QEMU** — Virtual machine for testing
* **ISO** — Bootable operating-system image

---

## ⚙️ Build and Run

### 1. Clone the repository

```bash
git clone https://github.com/nickyk0905/BatOS.git
cd BatOS
```

### 2. Build the operating system

```bash
make
```

The Makefile handles the compilation and linking process required to generate the OS image.

### 3. Run using QEMU

If QEMU is installed and configured:

```bash
qemu-system-i386 -cdrom BatOS.iso
```

You can then interact with BatOS through the terminal.

---

## 💻 Example Shell Usage

Once BatOS is running:

```text
> mkdir projects
> cd projects
> touch hello.txt
> ls
> cat hello.txt
> cd ..
> ls
```

These commands demonstrate interaction with the in-memory hierarchical file system.

---

## 🧠 Concepts Demonstrated

This project was developed to explore fundamental operating-system concepts, including:

* Boot process
* Kernel development
* x86 architecture
* Low-level programming
* C and Assembly interoperability
* Memory management concepts
* Device drivers
* Keyboard and mouse input
* Terminal handling
* Command-line interfaces
* File-system design
* Directory hierarchies
* Cross-compilation
* Linking and linker scripts
* Build automation
* Virtual machine testing

---

## 🎯 Project Goals

The main goals of BatOS are:

1. Understand how an operating system boots.
2. Learn how a kernel interacts with hardware.
3. Understand low-level C and Assembly programming.
4. Implement a basic command-line environment.
5. Build a simple hierarchical file system.
6. Understand the relationship between bootloader, kernel, drivers, and user interaction.
7. Create a minimal operating system that can be booted and tested independently.

---

## 🔮 Future Improvements

Potential future improvements include:

* [ ] Improved memory management
* [ ] Process management
* [ ] Basic multitasking
* [ ] More hardware drivers
* [ ] Persistent file-system storage
* [ ] Improved shell functionality
* [ ] System calls
* [ ] User/kernel mode separation
* [ ] More robust error handling
* [ ] Improved terminal features
* [ ] Networking support

---

## 👨‍💻 Author

**Nicky Kumari**

---

## 📜 License

This project is intended primarily as an educational operating-system development project.

See the repository for licensing information.

```
```
