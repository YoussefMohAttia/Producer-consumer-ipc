<div align="center">

# Producer-Consumer IPC

### Systems Programming Exercise with Shared Memory and Semaphores

[![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)](https://isocpp.org/)
[![Linux](https://img.shields.io/badge/Platform-Linux-FCC624?logo=linux&logoColor=black)](https://www.kernel.org/)

</div>

## Overview

This repository demonstrates the producer-consumer synchronization problem using inter-process communication primitives on Linux. Separate producer and consumer programs coordinate access to a shared buffer using System V shared memory and semaphores.

## Contents

```text
.
├── producer.cpp       # Produces items into the shared buffer
├── consumer.cpp       # Consumes items from the shared buffer
├── Game.c             # Additional coursework source
├── Makefile           # Builds producer and consumer binaries
└── lab5.pdf           # Assignment brief
```

## Build

Run the following on a Linux system with `g++` and `make` installed:

```bash
make
```

This produces `producer` and `consumer` executables. Review the assignment PDF and the source comments for the expected launch arguments and exercise flow.

## Concepts Demonstrated

- Shared-memory setup and attachment
- Semaphore-controlled access to a bounded buffer
- Producer and consumer coordination across processes
- C/C++ systems programming on Linux

## Author

**Youssef Mohamed Attia**  
[GitHub](https://github.com/YoussefMohAttia) · [LinkedIn](https://www.linkedin.com/in/youssefmohattia/)
