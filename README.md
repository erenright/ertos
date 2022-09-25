# Eric's Real-Time Operating System

This was a self-learning endeavor I embarked on in 2012, with the primary goals of better understanding embedded programming and implementation of task context switching.

Development was done on a Technologic Systems (now embeddedTS) TS-7250 with Cirrus Logic EP9302 ARM processor. Other than the NAND flash interface, this code is likely portable to other SBC's with the same processor family.

## Features

This OS implements the following features:

* Kernel/userspace separation
* System calls
* Basic process management including timers, sleeps, wait states, and context switching
* Serial-port console abstraction
* NAND flash interface
* Work-in-progress IP stack (TODO: merge enet fork)
* Work-in-progress USB stack (very incomplete, IIRC)

## Toolchain

Cross-compilers are available from embeddedTS [here](https://files.embeddedts.com//ts-arm-sbc/ts-7250-linux/cross-toolchains/). The version used was gcc-3.3.4-glibc-2.3.2, though any compiler supporting this ARM target may work, modulo warnings from newer compilers.

## To run on TS-7250

Terminal settings: 115,200 8n1

```
RedBoot> load -r -b 0x00100000 -m xmodem
<send file via xmodem>
RedBoot> go
```

Copy ertos.ld.xip overtop of ertos.ld and enable XIP in config.h to
build an image which can run directly from NOR flash on the TS-7200.

## Todo

* Merge enet fork
* Merge TS-ETH2 drivers fork
* Implement cons_write/read as syscalls.
* Returning values from syscalls does not work (returns syscall number) - likely just need to avoid clobbering `r0` in `lib/syscall_arm.s:__syscall` when restoring registers
* MMU support has issues under certain configurations (I no longer recall what those configurations were)
* Remove hard-coded include path in `Makefile.inc`
