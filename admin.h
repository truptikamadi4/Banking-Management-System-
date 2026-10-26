// admin.h - Admin features: view accounts, monitor transactions, block/delete, reports, interest, loan approval
#ifndef ADMIN_H
#define ADMIN_H

#include <map>

#include "customer.h"

class AdminModule {
public:
    AdminModule(Database& db, Bank& bank) : db_(db), bank_(bank) {}

    static bool login() {
        util::title("ADMIN LOGIN");
        std::string u = util::readLine("Username : ");
        std::string p = util::readLine("Password : ");
        if (u == "admin" && p == "admin123") return true;
        std::cout << "  ! Invalid admin credentials.\n";
        return false;
    }

    void menu() {
        while (true) {
            util::title("ADMIN PANEL");
            std::cout << "  1. View all customer accounts   6. Apply monthly interest\n"
                         "  2. Monitor transactions         7. Pending loan requests\n"
                         "  3. Block / unblock account      8. All loans\n"
                         "  4. Delete account               9. Logout\n"
                         "  5. Generate report\n";
            int c = util::readInt("\n  Choose: ", 1, 9);
            switch (c) {
                case 1: accounts(); break;
                case 2: monitor(); break;
                case 3: blockToggle(); break;
                case 4: deleteAccount(); break;
                case 5: report(); break;
                case 6: interest(); break;
                case 7: pendingLoans(); break;
                case 8: allLoans(); break;
                case 9: std::cout << "  Admin logged out.\n"; return;
            }
        }
    }

private:
    Database& db_;
    Bank& bank_;

    void accounts() {
        util::title("ALL ACCOUNTS");
        if (db_.accounts.empty()) { std::cout << "  No accounts yet.\n"; return; }
        std::cout << "  " << std::left << std::setw(9) << "A/C No" << std::setw(20) << "Name" << std::setw(13) << "Phone" << std::setw(9)
                  << "Type" << std::setw(20) << "Balance" << "Status\n";
        util::line('-', 78);
        for (auto& a : db_.accounts)
            std::cout << "  " << std::setw(9) << a.accNo << std::setw(20) << a.name << std::setw(13) << a.phone << std::setw(9) << a.type
                      << std::setw(20) << util::rs(a.balance) << (a.blocked ? "BLOCKED" : "Active") << "\n";
        std::cout << std::right;
    }

    void monitor() {
        util::title("MONITOR TRANSACTIONS");
        std::cout << "  1. All transactions   2. Transactions of one account\n";
        int c = util::readInt("  Choose: ", 1, 2);
        int acc = c == 2 ? util::readInt("  Account number: ", 0, 2000000000) : 0;
        int n = 0;
        std::cout << "  " << std::left << std::setw(7) << "ID" << std::setw(18) << "Date" << std::setw(9) << "A/C" << std::setw(14) << "Type"
                  << std::setw(18) << "Amount" << "Details\n";
        util::line('-', 90);
        for (auto& t : db_.txns) {
            if (acc && t.accNo != acc) continue;
            std::cout << "  " << std::setw(7) << t.id << std::setw(18) << t.timestamp << std::setw(9) << t.accNo << std::setw(14) << t.type
                      << std::setw(18) << util::rs(t.amount) << t.note << "\n";
            n++;
        }
        std::cout << std::right;
        if (!n) std::cout << "  No transactions found.\n";
        else std::cout << "  " << n << " transaction(s) listed.\n";
    }

    void blockToggle() {
        util::title("BLOCK / UNBLOCK ACCOUNT");
        Account* a = db_.findAccount(util::readInt("Account number: ", 0, 2000000000));
        if (!a) { std::cout << "  ! Account not found.\n"; return; }
        std::cout << "  " << a->name << " - currently " << (a->blocked ? "BLOCKED" : "Active") << "\n";
        if (!util::confirm(a->blocked ? "  Unblock this account?" : "  Block this account?")) return;
        a->blocked = !a->blocked;
        if (!a->blocked) a->failedAttempts = 0;
        db_.save();
        std::cout << "  Account " << a->accNo << " is now " << (a->blocked ? "BLOCKED" : "Active") << ".\n";
    }

    void deleteAccount() {
        util::title("DELETE ACCOUNT");
        int no = util::readInt("Account number: ", 0, 2000000000);
        Account* a = db_.findAccount(no);
        if (!a) { std::cout << "  ! Account not found.\n"; return; }
        if (a->balance != 0) { std::cout << "  ! Balance is " << util::rs(a->balance) << ". Withdraw/settle it first (or block the account instead).\n"; return; }
        if (db_.hasActiveLoan(no)) { std::cout << "  ! Account has an active or pending loan and cannot be deleted.\n"; return; }
        if (!util::confirm("  Permanently delete account " + std::to_string(no) + " (" + a->name + ")?")) return;
        for (size_t i = 0; i < db_.accounts.size(); i++)
            if (db_.accounts[i].accNo == no) { db_.accounts.erase(db_.accounts.begin() + i); break; }
        db_.save();
        std::cout << "  Account deleted. (Transaction history is retained for auditing.)\n";
    }

    void report() {
        util::title("BANK REPORT");
        long long total = 0, deposits = 0, withdrawals = 0, interest = 0, loanOut = 0;
        int savings = 0, current = 0, blocked = 0;
        for (auto& a : db_.accounts) {
            total += a.balance;
            (a.type == "Savings" ? savings : current)++;
            if (a.blocked) blocked++;
        }
        std::map<std::string, int> counts;
        for (auto& t : db_.txns) {
            counts[t.type]++;
            if (t.type == TxType::Deposit) deposits += t.amount;
            if (t.type == TxType::Withdrawal) withdrawals += t.amount;
            if (t.type == TxType::Interest) interest += t.amount;
        }
        for (auto& l : db_.loans)
            if (l.status == "Active") loanOut += l.outstanding;
        std::ostringstream os;
        os << "  Accounts          : " << db_.accounts.size() << " (Savings " << savings << ", Current " << current << ", Blocked " << blocked << ")\n";
        os << "  Total deposits held: " << util::rs(total) << "\n";
        os << "  Total deposited    : " << util::rs(deposits) << "\n";
        os << "  Total withdrawn    : " << util::rs(withdrawals) << "\n";
        os << "  Interest paid      : " << util::rs(interest) << "\n";
        os << "  Loans outstanding  : " << util::rs(loanOut) << "\n";
        os << "  Transactions       : " << db_.txns.size() << "\n";
        for (auto& kv : counts) os << "    " << std::left << std::setw(14) << kv.first << kv.second << "\n";
        std::cout << os.str() << std::right;
        std::ofstream("report.txt") << "BANK REPORT (" << util::nowDateTime() << ")\n" << os.str();
        std::cout << "  (Report saved to report.txt)\n";
    }

    void interest() {
        util::title("APPLY MONTHLY INTEREST");
        std::string month = util::currentMonth();
        if (!util::confirm("  Credit " + month + " interest (4% p.a.) to all Savings accounts?")) return;
        long long paid = 0;
        int n = bank_.applyMonthlyInterest(month, paid);
        if (n == 0) std::cout << "  No account was eligible (interest already credited for " << month << " or balances are zero).\n";
        else std::cout << "  Interest credited to " << n << " account(s). Total paid: " << util::rs(paid) << "\n";
    }

    void pendingLoans() {
        util::title("PENDING LOAN REQUESTS");
        bool any = false;
        for (auto& l : db_.loans)
            if (l.status == "Pending") { any = true; CustomerModule::printLoan(l); }
        if (!any) { std::cout << "  No pending loan requests.\n"; return; }
        int id = util::readInt("  Loan ID to review (0 = back): ", 0, 2000000000);
        if (id == 0) return;
        int d = util::readInt("  1) Approve   2) Reject: ", 1, 2);
        Result r = d == 1 ? bank_.approveLoan(id) : bank_.rejectLoan(id);
        std::cout << (r.ok ? "  " : "  ! ") << r.msg << "\n";
    }

    void allLoans() {
        util::title("ALL LOANS");
        if (db_.loans.empty()) { std::cout << "  No loans.\n"; return; }
        for (auto& l : db_.loans) CustomerModule::printLoan(l);
    }
};

#endif
