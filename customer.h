// customer.h - Authentication + Account Management + customer menu
#ifndef CUSTOMER_H
#define CUSTOMER_H

#include "bank.h"

class CustomerModule {
public:
    CustomerModule(Database& db, Bank& bank) : db_(db), bank_(bank) {}

    // ---------------------------------------------------------------- account creation
    void createAccount() {
        util::title("OPEN A NEW BANK ACCOUNT");
        std::string name = util::readNonEmpty("Account holder name : ");
        std::string phone;
        while (true) {
            phone = util::readLine("Phone (10 digits)    : ");
            if (util::isDigits(phone) && phone.size() == 10) break;
            std::cout << "  ! Phone number must be exactly 10 digits.\n";
        }
        int t = util::readInt("Account type  1) Savings (4% p.a. interest)  2) Current : ", 1, 2);
        std::string type = t == 1 ? "Savings" : "Current";
        long long opening = 0;
        while (true) {
            std::string s = util::readLine("Opening deposit (Rs. 500 or more): ");
            opening = util::parseAmount(s);
            if (opening >= 50000 && opening <= 10000000000LL) break;
            std::cout << "  ! Minimum opening deposit is Rs. 500.\n";
        }
        std::string pw = readPassword("Set login password (min 6 chars): ");
        std::string pin = readPin("Set 4-digit PIN: ");
        Account& a = bank_.createAccount(name, phone, type, opening, pw, pin);
        std::cout << "\n  Account created successfully!\n  Your account number is " << a.accNo << " - please remember it.\n";
        bank_.printReceipt(db_.txns.back().id);
    }

    // ---------------------------------------------------------------- login
    Account* login() {
        util::title("CUSTOMER LOGIN");
        int no = util::readInt("Account number : ", 0, 2000000000);
        std::string pw = util::readLine("Password       : ");
        Account* a = db_.findAccount(no);
        if (!a) { std::cout << "  ! Account not found.\n"; return nullptr; }
        if (!bank_.verifyPassword(*a, pw)) return nullptr;
        std::cout << "\n  Welcome, " << a->name << "!\n";
        return a;
    }

    // ---------------------------------------------------------------- main customer menu
    void menu(Account& a) {
        while (true) {
            util::title("ACCOUNT " + std::to_string(a.accNo) + " - " + a.name);
            std::cout << "  1. View account details      7. Interest information\n"
                         "  2. Check balance             8. Loans (apply / EMI)\n"
                         "  3. Deposit money             9. Change password\n"
                         "  4. Withdraw money           10. Change PIN\n"
                         "  5. Transfer money           11. View receipt\n"
                         "  6. Transaction history      12. Logout\n";
            int c = util::readInt("\n  Choose: ", 1, 12);
            switch (c) {
                case 1: details(a); break;
                case 2: std::cout << "\n  Available balance: " << util::rs(a.balance) << "\n"; break;
                case 3: depositUi(a); break;
                case 4: withdrawUi(a); break;
                case 5: transferUi(a); break;
                case 6: historyMenu(a); break;
                case 7: interestInfo(a); break;
                case 8: loanMenu(a); break;
                case 9: changePassword(a); break;
                case 10: changePin(a); break;
                case 11: receiptUi(a); break;
                case 12: std::cout << "  Logged out. Thank you for banking with us!\n"; return;
            }
            if (a.blocked) { std::cout << "  ! Your account has been blocked. Logging out.\n"; return; }
        }
    }

private:
    Database& db_;
    Bank& bank_;

    static std::string readPassword(const std::string& prompt) {
        while (true) {
            std::string p = util::readLine(prompt);
            if (p.size() >= 6 && p.find(',') == std::string::npos) return p;
            std::cout << "  ! Password needs at least 6 characters (no commas).\n";
        }
    }
    static std::string readPin(const std::string& prompt) {
        while (true) {
            std::string p = util::readLine(prompt);
            if (util::isDigits(p) && p.size() == 4) return p;
            std::cout << "  ! PIN must be exactly 4 digits.\n";
        }
    }

    // Asks for the PIN; returns true only if it is correct (3 failures block the account)
    bool askPin(Account& a) {
        std::string pin = util::readLine("  Enter your 4-digit PIN: ");
        return bank_.verifyPin(a, pin);
    }

    void details(Account& a) {
        util::title("ACCOUNT DETAILS");
        std::cout << "  Account number : " << a.accNo << "\n  Holder name    : " << a.name << "\n  Phone          : " << a.phone
                  << "\n  Account type   : " << a.type << "\n  Balance        : " << util::rs(a.balance)
                  << "\n  Opened on      : " << a.createdAt << "\n  Status         : " << (a.blocked ? "BLOCKED" : "Active") << "\n";
    }

    void depositUi(Account& a) {
        util::title("DEPOSIT");
        long long amt = util::readAmount("Amount to deposit: ");
        Result r = bank_.deposit(a, amt);
        report(r, a);
    }

    void withdrawUi(Account& a) {
        util::title("WITHDRAW");
        std::cout << "  Available balance: " << util::rs(a.balance) << "\n";
        long long amt = util::readAmount("Amount to withdraw: ");
        if (amt > a.balance) { std::cout << "  ! Insufficient balance. Available: " << util::rs(a.balance) << "\n"; return; }
        if (!askPin(a)) return;
        report(bank_.withdraw(a, amt), a);
    }

    void transferUi(Account& a) {
        util::title("TRANSFER MONEY");
        int to = util::readInt("Destination account number: ", 0, 2000000000);
        Account* dest = db_.findAccount(to);
        if (!dest) { std::cout << "  ! Account " << to << " does not exist.\n"; return; }
        if (to == a.accNo) { std::cout << "  ! You cannot transfer to your own account.\n"; return; }
        std::cout << "  Beneficiary: " << dest->name << " (" << dest->type << ")\n";
        long long amt = util::readAmount("Amount to transfer: ");
        if (amt > a.balance) { std::cout << "  ! Insufficient balance. Available: " << util::rs(a.balance) << "\n"; return; }
        if (!util::confirm("  Send " + util::rs(amt) + " to " + dest->name + "?")) { std::cout << "  Transfer cancelled.\n"; return; }
        if (!askPin(a)) return;
        report(bank_.transfer(a, to, amt), a);
    }

    void report(const Result& r, Account&) {
        if (!r.ok) { std::cout << "  ! " << r.msg << "\n"; return; }
        std::cout << "  " << r.msg << "\n";
        bank_.printReceipt(r.txnId);
    }

    // ---------------------------------------------------------------- history
    void printTxns(const std::vector<const Transaction*>& list) {
        if (list.empty()) { std::cout << "  No transactions found.\n"; return; }
        std::cout << "  " << std::left << std::setw(7) << "ID" << std::setw(18) << "Date" << std::setw(14) << "Type" << std::setw(18)
                  << "Amount" << std::setw(18) << "Balance" << "Details\n";
        util::line('-', 100);
        for (auto* t : list)
            std::cout << "  " << std::setw(7) << t->id << std::setw(18) << t->timestamp << std::setw(14) << t->type << std::setw(18)
                      << util::rs(t->amount) << std::setw(18) << util::rs(t->balanceAfter) << t->note << "\n";
        std::cout << std::right;
    }

    void historyMenu(Account& a) {
        util::title("TRANSACTION HISTORY");
        std::cout << "  1. All transactions\n  2. Deposits & withdrawals\n  3. Transfers (with totals)\n  4. Back\n";
        int c = util::readInt("  Choose: ", 1, 4);
        if (c == 4) return;
        std::vector<const Transaction*> list;
        long long sent = 0, received = 0;
        for (auto& t : db_.txns) {
            if (t.accNo != a.accNo) continue;
            bool pick = c == 1 || (c == 2 && (t.type == TxType::Deposit || t.type == TxType::Withdrawal)) ||
                        (c == 3 && (t.type == TxType::TransferOut || t.type == TxType::TransferIn));
            if (!pick) continue;
            list.push_back(&t);
            if (t.type == TxType::TransferOut) sent += t.amount;
            if (t.type == TxType::TransferIn) received += t.amount;
        }
        printTxns(list);
        if (c == 3) std::cout << "\n  Total transferred out: " << util::rs(sent) << "\n  Total received       : " << util::rs(received) << "\n";
        if (c == 2) {
            long long dep = 0, wd = 0;
            for (auto* t : list) (t->type == TxType::Deposit ? dep : wd) += t->amount;
            std::cout << "\n  Total deposited: " << util::rs(dep) << "\n  Total withdrawn: " << util::rs(wd) << "\n";
        }
    }

    // ---------------------------------------------------------------- interest
    void interestInfo(Account& a) {
        util::title("INTEREST INFORMATION");
        if (a.type != "Savings") { std::cout << "  Current accounts do not earn interest. Open a Savings account to earn 4% p.a.\n"; return; }
        long long m = Bank::monthlyInterest(a);
        std::cout << "  Rate                     : 4.0% per annum (credited monthly by the bank)\n"
                  << "  Interest on current balance this month : " << util::rs(m) << "\n"
                  << "  Estimated for 1 year (no deposits)     : " << util::rs(static_cast<long long>(std::llround(a.balance * 0.04))) << "\n"
                  << "  Last credited for month  : " << (a.lastInterestMonth.empty() ? "not yet" : a.lastInterestMonth) << "\n";
    }

    // ---------------------------------------------------------------- loans
    void loanMenu(Account& a) {
        util::title("LOANS");
        std::cout << "  1. Apply for a loan (" << Bank::LOAN_RATE << "% p.a.)\n  2. My loans\n  3. Pay EMI\n  4. Back\n";
        int c = util::readInt("  Choose: ", 1, 4);
        if (c == 1) {
            std::cout << "  Loan limit: up to 10x your balance, between " << util::rs(Bank::MIN_LOAN) << " and " << util::rs(Bank::MAX_LOAN) << "\n";
            long long p = util::readAmount("  Loan amount: ");
            int m = util::readInt("  Tenure in months (3-60): ", 1, 600);
            Result r = bank_.applyLoan(a, p, m);
            std::cout << (r.ok ? "  " : "  ! ") << r.msg << "\n";
        } else if (c == 2) {
            bool any = false;
            for (auto& l : db_.loans) {
                if (l.accNo != a.accNo) continue;
                any = true;
                printLoan(l);
            }
            if (!any) std::cout << "  You have no loans.\n";
        } else if (c == 3) {
            Loan* active = nullptr;
            for (auto& l : db_.loans)
                if (l.accNo == a.accNo && l.status == "Active") active = &l;
            if (!active) { std::cout << "  You have no active loan.\n"; return; }
            printLoan(*active);
            if (!util::confirm("  Pay the next EMI now?")) return;
            if (!askPin(a)) return;
            Result r = bank_.payEmi(a, *active);
            std::cout << (r.ok ? "  " : "  ! ") << r.msg << "\n";
            if (r.ok) bank_.printReceipt(r.txnId);
        }
    }

public:
    static void printLoan(const Loan& l) {
        std::cout << "  Loan #" << l.id << " | A/C " << l.accNo << " | " << l.status << " | Principal " << util::rs(l.principal) << " | "
                  << l.annualRate << "% | " << l.months << " months | EMI " << util::rs(l.emi) << " | Outstanding " << util::rs(l.outstanding)
                  << " | EMIs left " << l.monthsLeft << "\n";
    }

private:
    void changePassword(Account& a) {
        util::title("CHANGE PASSWORD");
        std::string old = util::readLine("Current password: ");
        if (!bank_.verifyPassword(a, old)) return;
        a.passHash = util::hashPassword(readPassword("New password (min 6 chars): "));
        db_.save();
        std::cout << "  Password changed successfully.\n";
    }

    void changePin(Account& a) {
        util::title("CHANGE PIN");
        if (!askPin(a)) return;
        a.pinHash = Bank::hashPin(readPin("New 4-digit PIN: "));
        db_.save();
        std::cout << "  PIN changed successfully.\n";
    }

    void receiptUi(Account& a) {
        util::title("VIEW RECEIPT");
        int id = util::readInt("Transaction ID: ", 1, 2000000000);
        for (auto& t : db_.txns)
            if (t.id == id) {
                if (t.accNo != a.accNo) break;
                bank_.printReceipt(id);
                return;
            }
        std::cout << "  ! Transaction not found in your account.\n";
    }
};

#endif
