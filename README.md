# RobloxRandom

A powerful CLI tool designed for generating random data (integers, floats, vectors) using seeds, as well as testing or cracking pseudorandom number generator (PRNG) outputs, with native support for Roblox environments.

## Features
* **Generation Mode (`gen`):** Generate reproducible random integers, floating-point numbers, and vectors with or without a specific seed.
* **Single Mode (`single`):** Fast, single-value generation, tailored for explicit environment testing (e.g., Roblox).
* **Crack Mode (`crack`):** Analyze and reverse-engineer PRNG states based on generated outputs.

---

## Command Line Interface (CLI)

![CLI Usage](https://github.com/user-attachments/assets/8623388d-7e93-4485-b1f7-16a8b516ab71)

---

## Usage Examples

### 1. Generation Mode (`gen`)

#### Generating Integers (Without Seed)
![Generating integers without seed](https://github.com/user-attachments/assets/1990c796-6ff2-4fc4-90f0-a6cb95519d42)

#### Generating Integers (With Seed)
![Generating integers with seed](https://github.com/user-attachments/assets/062105da-2b6b-42ab-8cb2-b4b898707605)

#### Generating Floating-Point Numbers
![Generating numbers](https://github.com/user-attachments/assets/a22b8047-12ce-41ca-a6ec-b4260f122329)

#### Generating Vectors
![Generating vectors](https://github.com/user-attachments/assets/ff510837-ec08-4fa4-a5fa-9c27f14195f5)

### 2. Single Mode (`single`)

Running the command:
![Single mode command](https://github.com/user-attachments/assets/a44a085a-4ba2-4691-b937-53875aeb936a)

Corresponding Roblox output:
![Roblox output](https://github.com/user-attachments/assets/83f8d40e-1b02-4f24-acda-a3718df1ebe6)

### 3. Crack Mode (`crack`)

To reverse or crack the generator state:
![Cracking mode](https://github.com/user-attachments/assets/e87135c0-1dca-401f-953b-fb0d03dd05c7)
