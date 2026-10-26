// database.h - File Handling Module (CSV persistence)
#ifndef DATABASE_H
#define DATABASE_H

#include <fstream>
#include <sys/stat.h>

#include "models.h"
#include "utils.h"

class Database {
public:
    std::vector<Account> accounts;
    std::vector<Transaction> txns;
    std::vector<Loan> loans;
    int nextAccNo = 100001;
    int nextTxnId = 1;
    int nextLoanId = 1;

    explicit Database(const std::string& dir = "data") : dir_(dir) {}

    void load() {
        ::mkdir(dir_.c_str(), 0755);
        ::mkdir("receipts", 0755);
        loadAccounts();
        loadTxns();
        loadLoans();
    }

    void save() {
        saveAccounts();
        saveTxns();
        saveLoans();
    }

    Account* findAccount(int accNo) {
        for (auto& a : accounts)
            if (a.accNo == accNo) return &a;
        return nullptr;
    }
    Loan* findLoan(int id) {
        for (auto& l : loans)
            if (l.id == id) return &l;
        return nullptr;
    }
    bool hasActiveLoan(int accNo) const {
        for (auto& l : loans)
            if (l.accNo == accNo && (l.status == "Active" || l.status == "Pending")) return true;
        return false;
    }

private:
    std::string dir_;
    std::string path(const std::string& f) const { return dir_ + "/" + f; }

    static std::string f(const std::vector<std::string>& r, size_t i) { return i < r.size() ? r[i] : ""; }
    static long long ll(const std::string& s) {
        try { return s.empty() ? 0 : std::stoll(s); } catch (...) { return 0; }
    }
    static double dbl(const std::string& s) {
        try { return s.empty() ? 0 : std::stod(s); } catch (...) { return 0; }
    }

    static bool rows(const std::string& p, std::vector<std::vector<std::string>>& out) {
        std::ifstream in(p);
        if (!in) return false;
        std::string ln;
        bool first = true;
        while (std::getline(in, ln)) {
            if (first) { first = false; continue; }
            if (util::trim(ln).empty()) continue;
            out.push_back(util::split(ln, ','));
        }
        return true;
    }

    void loadAccounts() {
        accounts.clear();
        std::vector<std::vector<std::string>> r;
        if (!rows(path("accounts.csv"), r)) return;
        for (auto& x : r) {
            Account a;
            a.accNo = static_cast<int>(ll(f(x, 0))); a.name = f(x, 1); a.phone = f(x, 2); a.type = f(x, 3);
            a.balance = ll(f(x, 4)); a.passHash = f(x, 5); a.pinHash = f(x, 6); a.blocked = f(x, 7) == "1";
            a.createdAt = f(x, 8); a.failedAttempts = static_cast<int>(ll(f(x, 9))); a.lastInterestMonth = f(x, 10);
            accounts.push_back(a);
            nextAccNo = std::max(nextAccNo, a.accNo + 1);
        }
    }
    void saveAccounts() {
        std::ofstream o(path("accounts.csv"));
        o << "accNo,name,phone,type,balance,passHash,pinHash,blocked,createdAt,failedAttempts,lastInterestMonth\n";
        for (auto& a : accounts)
            o << a.accNo << ',' << a.name << ',' << a.phone << ',' << a.type << ',' << a.balance << ',' << a.passHash << ','
              << a.pinHash << ',' << (a.blocked ? 1 : 0) << ',' << a.createdAt << ',' << a.failedAttempts << ','
              << a.lastInterestMonth << '\n';
    }

    void loadTxns() {
        txns.clear();
        std::vector<std::vector<std::string>> r;
        if (!rows(path("transactions.csv"), r)) return;
        for (auto& x : r) {
            Transaction t;
            t.id = static_cast<int>(ll(f(x, 0))); t.accNo = static_cast<int>(ll(f(x, 1))); t.type = f(x, 2);
            t.amount = ll(f(x, 3)); t.balanceAfter = ll(f(x, 4)); t.counterparty = static_cast<int>(ll(f(x, 5)));
            t.timestamp = f(x, 6); t.note = f(x, 7);
            txns.push_back(t);
            nextTxnId = std::max(nextTxnId, t.id + 1);
        }
    }
    void saveTxns() {
        std::ofstream o(path("transactions.csv"));
        o << "id,accNo,type,amount,balanceAfter,counterparty,timestamp,note\n";
        for (auto& t : txns)
            o << t.id << ',' << t.accNo << ',' << t.type << ',' << t.amount << ',' << t.balanceAfter << ',' << t.counterparty << ','
              << t.timestamp << ',' << t.note << '\n';
    }

    void loadLoans() {
        loans.clear();
        std::vector<std::vector<std::string>> r;
        if (!rows(path("loans.csv"), r)) return;
        for (auto& x : r) {
            Loan l;
            l.id = static_cast<int>(ll(f(x, 0))); l.accNo = static_cast<int>(ll(f(x, 1))); l.principal = ll(f(x, 2));
            l.annualRate = dbl(f(x, 3)); l.months = static_cast<int>(ll(f(x, 4))); l.emi = ll(f(x, 5));
            l.outstanding = ll(f(x, 6)); l.monthsLeft = static_cast<int>(ll(f(x, 7))); l.status = f(x, 8);
            l.appliedAt = f(x, 9);
            loans.push_back(l);
            nextLoanId = std::max(nextLoanId, l.id + 1);
        }
    }
    void saveLoans() {
        std::ofstream o(path("loans.csv"));
        o << "id,accNo,principal,annualRate,months,emi,outstanding,monthsLeft,status,appliedAt\n";
        for (auto& l : loans)
            o << l.id << ',' << l.accNo << ',' << l.principal << ',' << l.annualRate << ',' << l.months << ',' << l.emi << ','
              << l.outstanding << ',' << l.monthsLeft << ',' << l.status << ',' << l.appliedAt << '\n';
    }
};

#endif
