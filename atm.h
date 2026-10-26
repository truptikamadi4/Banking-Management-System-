// atm.h - ATM simulation (card-less: account number + PIN verification)
#ifndef ATM_H
#define ATM_H

#include "bank.h"

class AtmModule {
public:
    AtmModule(Database& db, Bank& bank) : db_(db), bank_(bank) {}

    void run() {
        util::title("ATM - WELCOME");
        int no = util::readInt("Insert card (enter account number): ", 0, 2000000000);
        Account* a = db_.findAccount(no);
        if (!a) { std::cout << "  ! Card not recognised.\n"; return; }
        std::string pin = util::readLine("Enter your 4-digit PIN: ");
        if (!bank_.verifyPin(*a, pin)) return;

        while (true) {
            util::title("ATM MENU - " + a->name);
            std::cout << "  1. Balance enquiry\n  2. Cash withdrawal (multiples of Rs. 100)\n  3. Cash deposit\n"
                         "  4. Mini statement (last 5)\n  5. Change PIN\n  6. Eject card\n";
            int c = util::readInt("\n  Choose: ", 1, 6);
            if (c == 1) {
                std::cout << "  Available balance: " << util::rs(a->balance) << "\n";
            } else if (c == 2) {
                long long amt = util::readAmount("  Amount: ");
                long long used = bank_.atmWithdrawnToday(a->accNo);
                if (amt % 10000 != 0) { std::cout << "  ! ATM dispenses notes in multiples of Rs. 100 only.\n"; continue; }
                if (amt > Bank::ATM_PER_TXN) { std::cout << "  ! Per-transaction ATM limit is " << util::rs(Bank::ATM_PER_TXN) << ".\n"; continue; }
                if (used + amt > Bank::ATM_PER_DAY) {
                    std::cout << "  ! Daily ATM limit of " << util::rs(Bank::ATM_PER_DAY) << " would be exceeded (already withdrawn today: " << util::rs(used) << ").\n";
                    continue;
                }
                Result r = bank_.withdraw(*a, amt, "ATM cash withdrawal");
                if (!r.ok) { std::cout << "  ! " << r.msg << "\n"; continue; }
                std::cout << "  Please collect your cash: " << util::rs(amt) << "\n";
                bank_.printReceipt(r.txnId);
            } else if (c == 3) {
                long long amt = util::readAmount("  Amount to deposit: ");
                Result r = bank_.deposit(*a, amt, "ATM cash deposit");
                std::cout << (r.ok ? "  " : "  ! ") << r.msg << "\n";
                if (r.ok) bank_.printReceipt(r.txnId);
            } else if (c == 4) {
                std::vector<const Transaction*> mine;
                for (auto& t : db_.txns)
                    if (t.accNo == a->accNo) mine.push_back(&t);
                size_t from = mine.size() > 5 ? mine.size() - 5 : 0;
                if (mine.empty()) std::cout << "  No transactions yet.\n";
                for (size_t i = from; i < mine.size(); i++)
                    std::cout << "  " << mine[i]->timestamp << "  " << std::left << std::setw(13) << mine[i]->type << std::setw(16)
                              << util::rs(mine[i]->amount) << " Bal " << util::rs(mine[i]->balanceAfter) << "\n" << std::right;
                std::cout << "  Balance: " << util::rs(a->balance) << "\n";
            } else if (c == 5) {
                std::string np = util::readLine("  New 4-digit PIN: ");
                if (!util::isDigits(np) || np.size() != 4) { std::cout << "  ! PIN must be exactly 4 digits.\n"; continue; }
                a->pinHash = Bank::hashPin(np);
                db_.save();
                std::cout << "  PIN changed.\n";
            } else {
                std::cout << "  Card ejected. Thank you!\n";
                return;
            }
        }
    }

private:
    Database& db_;
    Bank& bank_;
};

#endif
