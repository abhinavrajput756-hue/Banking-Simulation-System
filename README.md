# Banking Simulation System

A console-based banking simulation mini project written in C.

## Features
- Create customer accounts with unique account numbers
- Login using account number and a four-digit PIN
- Lock an account after three incorrect PIN attempts
- Deposit and withdraw money
- Transfer funds to another account
- Check account balance and change PIN
- View timestamped transaction history
- Save account records in the binary file `accounts.dat`

## Requirements
- GCC or another C compiler
- Windows, Linux, or macOS terminal

## Compile
```bash
gcc -std=c11 -Wall -Wextra -pedantic src/banking_simulation.c -o banking
```

On Windows with MinGW, use `-o banking.exe`.

## Run
- Linux/macOS: `./banking`
- Windows: `banking.exe`

The program creates `accounts.dat` in the current working directory.

## Project structure
```text
Banking-Simulation-System/
├── README.md
├── .gitignore
└── src/
    └── banking_simulation.c
```

## Note
This is an educational simulation, not real banking software. It does not provide production-grade security, encryption, or banking integrations. Do not use real personal or financial information.
