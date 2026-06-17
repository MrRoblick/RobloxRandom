# Roblox Random PCG CLI Tool

A lightweight C++ command-line utility designed to generate, and reverse-engineer (crack) the PCG-based pseudorandom number generator (PRNG) used in Roblox (`Random.new()`).

## Features
* **Crack Mode:** Reverse-engineer and find the initial 64-bit seed using a sequence of generated numbers from a file.
* **Single Mode:** Find a valid seed using just a single target value within a specific range.
* **Generation Mode:** Generate reproducible random integers, floating-point numbers, and unit vectors identical to Roblox's native `Random` object behavior.

---

## Command Line Interface (CLI)

![CLI Usage](https://github.com/user-attachments/assets/8623388d-7e93-4485-b1f7-16a8b516ab71)

### Usage Syntax
```bash
roblox_random crack <filename> <min> <max>   # Crack seed using a sequence from a file
roblox_random single <target> <min> <max>    # Find a seed using a single target value
roblox_random gen <method> [options]         # Generate random numbers/vectors
