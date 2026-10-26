# Banking Management System (C++)

Console-based banking application built for **Sqrock IT Solutions - Internship Project Phase 1, Task 2**.
Customers open accounts, deposit, withdraw and transfer money, view their transaction history, use an ATM,
earn interest and apply for loans. An admin monitors accounts and transactions.

## Build and run

Requires a C++17 compiler (g++ / clang++ / MinGW).

```bash
make run          # builds and starts the app
# or manually
g++ -std=c++17 -O2 -o bank main.cpp
./bank
```

Data is created automatically in `data/` (CSV files) and `receipts/`. Delete those folders to reset.

### Logins

| Role | Login | Password |
|------|-------|----------|
| Customer | account number shown after opening an account (starts at 100001) | password chosen at sign-up |
| ATM | account number | 4-digit PIN chosen at sign-up |
| Admin | `admin` | `admin123` |

## Features

**Required**
- Create account (Savings / Current), login, view details, check balance
- Deposit, withdraw, transfer, balance inquiry
- Validations: no negative balance, destination account must exist and not be blocked, amount must be a valid positive number (max 2 decimals), no self-transfer
- Transaction history: all transactions, deposit/withdraw history with totals, transfers with total sent / received
- **Admin**: view all accounts, monitor all transactions (or one account), block / unblock, delete accounts (only with zero balance and no loan), generate report (`report.txt`)

**Bonus (all implemented)**
- **ATM simulation**: balance, withdrawal in multiples of Rs. 100 (Rs. 20,000 per transaction, Rs. 50,000 per day), deposit, mini statement, PIN change
- **PIN verification** for withdrawals, transfers, EMI payments and ATM; **3 wrong attempts block the account** (admin can unblock)
- **Interest calculation**: Savings earn 4% p.a., credited monthly by the admin (cannot be credited twice for the same month); customers see their expected interest
- **Receipt generation** after every transaction (printed and saved to `receipts/TXN<id>.txt`)
- **Loan management simulation**: apply (3-60 months, up to 10x balance, 10.5% p.a.), admin approves / rejects, EMI by reducing-balance formula, EMI payment with interest / principal split, loan closes automatically
- **Password security**: passwords and PINs are stored as salted hashes, never in plain text

## Project structure

| File | Module |
|------|--------|
| `main.cpp` | Top-level menu |
| `utils.h` | Input validation, money parsing / formatting, hashing |
| `models.h` | Entities: `Account`, `Transaction`, `Loan`, transaction type constants |
| `database.h` | **File Handling Module** (CSV load / save) |
| `bank.h` | **Transaction Module / business rules** (`Bank`: deposit, withdraw, transfer, interest, loans, receipts, PIN checks) |
| `customer.h` | **Authentication + Account Management Module** and customer menu |
| `atm.h` | ATM simulation |
| `admin.h` | Admin features |

## Design notes

- **Money is stored as integer paise** (`long long`), never `double`, so there are no rounding errors in balances.
- The business rules live in the `Bank` class; the menu classes only read input and print results, which keeps the code easy to test and extend.
- A transfer creates two linked records (`Transfer-Out` for the sender, `Transfer-In` for the receiver).

## C++ concepts used

OOP (classes, encapsulation, static members, constants), STL (`vector`, `map`, `string`, `stringstream`), file handling (`ifstream` / `ofstream`, CSV), references and pointers, exception for end-of-input, input validation, `<cmath>` for the EMI formula, lambdas.

## Data files

`data/accounts.csv`, `data/transactions.csv`, `data/loans.csv`, `receipts/TXN<id>.txt`, `report.txt`.
Hashing uses `std::hash` with a salt: fine for learning, not production-grade security (a real bank would use bcrypt / Argon2 and a database).

## Sample run

`sample_output.txt` shows two accounts being opened, deposits, withdrawals, transfers, history, a loan request, admin approval, interest, EMI payments, ATM usage and an account being blocked after three wrong PINs and unblocked by the admin.

## Possible improvements

SQL database, OTP verification, scheduled interest, statement export to PDF, GUI.
