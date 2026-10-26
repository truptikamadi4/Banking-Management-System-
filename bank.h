// bank.h - Business rules: transactions, validation, interest, loans, receipts, PIN/password security
#ifndef BANK_H
#define BANK_H

#include <cmath>
#include <fstream>

#include "database.h"
#include "utils.h"

struct Result {
    bool ok = false;
    std::string msg;
    int txnId = 0;
};

class Bank {
public:
    static constexpr double LOAN_RATE = 10.5;          // % per year
    static constexpr long long MIN_LOAN = 100000;      // Rs. 1,000 in paise
    static constexpr long long MAX_LOAN = 50000000;    // Rs. 5,00,000 in paise
    static constexpr long long ATM_PER_TXN = 2000000;  // Rs. 20,000
    static constexpr long long ATM_PER_DAY = 5000000;  // Rs. 50,000
    static constexpr int MAX_ATTEMPTS = 3;

    explicit Bank(Database& db) : db_(db) {}

    // ------------------------------------------------------------------ security
    bool verifyPassword(Account& a, const std::string& pw) { return check(a, a.passHash, util::hashPassword(pw)); }
    bool verifyPin(Account& a, const std::string& pin) { return check(a, a.pinHash, util::hashPassword("pin:" + pin)); }
    static std::string hashPin(const std::string& pin) { return util::hashPassword("pin:" + pin); }

    // ------------------------------------------------------------------ account creation
    Account& createAccount(const std::string& name, const std::string& phone, const std::string& type, long long opening,
                           const std::string& password, const std::string& pin) {
        Account a;
        a.accNo = db_.nextAccNo++;
        a.name = name;
        a.phone = phone;
        a.type = type;
        a.passHash = util::hashPassword(password);
        a.pinHash = hashPin(pin);
        a.createdAt = util::nowDateTime();
        db_.accounts.push_back(a);
        Account& ref = db_.accounts.back();
        if (opening > 0) {
            ref.balance = opening;
            record(ref, TxType::Deposit, opening, 0, "Opening deposit");
        }
        db_.save();
        return ref;
    }

    // ------------------------------------------------------------------ core operations
    Result deposit(Account& a, long long amt, const std::string& note = "Cash deposit") {
        if (a.blocked) return fail("Account is blocked.");
        if (amt <= 0) return fail("Amount must be greater than zero.");
        a.balance += amt;
        Transaction& t = record(a, TxType::Deposit, amt, 0, note);
        db_.save();
        return success("Deposit successful.", t.id);
    }

    Result withdraw(Account& a, long long amt, const std::string& note = "Cash withdrawal") {
        if (a.blocked) return fail("Account is blocked.");
        if (amt <= 0) return fail("Amount must be greater than zero.");
        if (amt > a.balance) return fail("Insufficient balance. Available: " + util::rs(a.balance));
        a.balance -= amt;
        Transaction& t = record(a, TxType::Withdrawal, amt, 0, note);
        db_.save();
        return success("Withdrawal successful.", t.id);
    }

    Result transfer(Account& from, int toAccNo, long long amt) {
        if (from.blocked) return fail("Your account is blocked.");
        if (amt <= 0) return fail("Amount must be greater than zero.");
        if (toAccNo == from.accNo) return fail("You cannot transfer money to your own account.");
        Account* to = db_.findAccount(toAccNo);
        if (!to) return fail("Destination account " + std::to_string(toAccNo) + " does not exist.");
        if (to->blocked) return fail("Destination account is blocked and cannot receive money.");
        if (amt > from.balance) return fail("Insufficient balance. Available: " + util::rs(from.balance));
        from.balance -= amt;
        to->balance += amt;
        Transaction& out = record(from, TxType::TransferOut, amt, to->accNo, "Transfer to " + to->name);
        int outId = out.id;
        record(*to, TxType::TransferIn, amt, from.accNo, "Transfer from " + from.name);
        db_.save();
        return success("Transferred " + util::rs(amt) + " to " + to->name + " (A/C " + std::to_string(to->accNo) + ").", outId);
    }

    // ------------------------------------------------------------------ interest
    static long long monthlyInterest(const Account& a) {
        return static_cast<long long>(std::llround(a.balance * a.interestRatePercent() / 100.0 / 12.0));
    }

    // Credits one month of interest to every eligible savings account. Returns accounts credited.
    int applyMonthlyInterest(const std::string& month, long long& totalPaid) {
        int n = 0;
        totalPaid = 0;
        for (auto& a : db_.accounts) {
            if (a.blocked || a.type != "Savings" || a.lastInterestMonth == month) continue;
            long long i = monthlyInterest(a);
            if (i <= 0) continue;
            a.balance += i;
            a.lastInterestMonth = month;
            record(a, TxType::Interest, i, 0, "Interest credit for " + month + " @ 4% p.a.");
            totalPaid += i;
            n++;
        }
        db_.save();
        return n;
    }

    // ------------------------------------------------------------------ loans (simulation)
    static long long computeEmi(long long principal, double annualRate, int months) {
        double r = annualRate / 1200.0;
        double p = static_cast<double>(principal);
        double emi = p * r * std::pow(1 + r, months) / (std::pow(1 + r, months) - 1);
        return static_cast<long long>(std::llround(emi));
    }

    Result applyLoan(Account& a, long long principal, int months) {
        if (a.blocked) return fail("Account is blocked.");
        if (db_.hasActiveLoan(a.accNo)) return fail("You already have an active or pending loan.");
        if (principal < MIN_LOAN || principal > MAX_LOAN)
            return fail("Loan amount must be between " + util::rs(MIN_LOAN) + " and " + util::rs(MAX_LOAN) + ".");
        if (principal > a.balance * 10) return fail("Loan limit is 10 times your current balance (" + util::rs(a.balance * 10) + ").");
        if (months < 3 || months > 60) return fail("Tenure must be between 3 and 60 months.");
        Loan l;
        l.id = db_.nextLoanId++;
        l.accNo = a.accNo;
        l.principal = principal;
        l.annualRate = LOAN_RATE;
        l.months = months;
        l.emi = computeEmi(principal, LOAN_RATE, months);
        l.outstanding = principal;
        l.monthsLeft = months;
        l.appliedAt = util::nowDateTime();
        db_.loans.push_back(l);
        db_.save();
        return success("Loan application #" + std::to_string(l.id) + " submitted. EMI would be " + util::rs(l.emi) +
                           " for " + std::to_string(months) + " months. Waiting for admin approval.",
                       0);
    }

    Result approveLoan(int id) {
        Loan* l = db_.findLoan(id);
        if (!l || l->status != "Pending") return fail("No pending loan with that ID.");
        Account* a = db_.findAccount(l->accNo);
        if (!a || a->blocked) return fail("Borrower account is missing or blocked.");
        l->status = "Active";
        a->balance += l->principal;
        Transaction& t = record(*a, TxType::LoanCredit, l->principal, 0, "Loan #" + std::to_string(l->id) + " disbursed");
        db_.save();
        return success("Loan #" + std::to_string(id) + " approved and " + util::rs(l->principal) + " credited to A/C " +
                           std::to_string(a->accNo) + ".",
                       t.id);
    }

    Result rejectLoan(int id) {
        Loan* l = db_.findLoan(id);
        if (!l || l->status != "Pending") return fail("No pending loan with that ID.");
        l->status = "Rejected";
        db_.save();
        return success("Loan #" + std::to_string(id) + " rejected.", 0);
    }

    Result payEmi(Account& a, Loan& l) {
        if (l.status != "Active") return fail("This loan is not active.");
        long long interest = static_cast<long long>(std::llround(l.outstanding * l.annualRate / 1200.0));
        long long principalPart = l.emi - interest;
        long long pay = l.emi;
        if (l.monthsLeft == 1 || principalPart >= l.outstanding) {  // final instalment clears the loan
            principalPart = l.outstanding;
            pay = l.outstanding + interest;
        }
        if (pay > a.balance) return fail("Insufficient balance for the EMI of " + util::rs(pay) + ".");
        a.balance -= pay;
        l.outstanding -= principalPart;
        l.monthsLeft--;
        int number = l.months - l.monthsLeft;
        Transaction& t = record(a, TxType::LoanRepay, pay, 0,
                                "EMI " + std::to_string(number) + " of " + std::to_string(l.months) + " for loan #" + std::to_string(l.id));
        if (l.outstanding == 0 || l.monthsLeft == 0) {
            l.status = "Closed";
            l.outstanding = 0;
        }
        db_.save();
        return success("EMI of " + util::rs(pay) + " paid (interest " + util::rs(interest) + ", principal " + util::rs(principalPart) +
                           "). Outstanding: " + util::rs(l.outstanding) + (l.status == "Closed" ? "  ** LOAN CLOSED **" : ""),
                       t.id);
    }

    // ------------------------------------------------------------------ ATM limits
    long long atmWithdrawnToday(int accNo) const {
        long long sum = 0;
        std::string d = util::today();
        for (auto& t : db_.txns)
            if (t.accNo == accNo && t.type == TxType::Withdrawal && t.note.rfind("ATM", 0) == 0 && t.timestamp.rfind(d, 0) == 0)
                sum += t.amount;
        return sum;
    }

    // ------------------------------------------------------------------ receipts
    static std::string buildReceipt(const Transaction& t, const Account& a) {
        std::ostringstream os;
        os << "==========================================\n";
        os << "        SQROCK BANK - TRANSACTION RECEIPT\n";
        os << "==========================================\n";
        os << "Receipt No   : TXN" << std::setw(6) << std::setfill('0') << t.id << std::setfill(' ') << "\n";
        os << "Date & time  : " << t.timestamp << "\n";
        os << "Account      : " << a.accNo << " (" << a.name << ", " << a.type << ")\n";
        os << "Type         : " << t.type << "\n";
        if (t.counterparty) os << "Other account: " << t.counterparty << "\n";
        os << "Amount       : " << util::rs(t.amount) << "\n";
        os << "Balance after: " << util::rs(t.balanceAfter) << "\n";
        os << "Remarks      : " << t.note << "\n";
        os << "==========================================\n";
        os << "      Thank you for banking with us!\n";
        return os.str();
    }

    void printReceipt(int txnId, bool print = true) {
        const Transaction* t = nullptr;
        for (auto& x : db_.txns)
            if (x.id == txnId) t = &x;
        if (!t) return;
        Account* a = db_.findAccount(t->accNo);
        if (!a) return;
        std::string text = buildReceipt(*t, *a);
        std::string file = "receipts/TXN" + std::to_string(t->id) + ".txt";
        std::ofstream(file) << text;
        if (print) std::cout << "\n" << text;
        std::cout << "  (Receipt saved to " << file << ")\n";
    }

private:
    Database& db_;

    static Result fail(const std::string& m) { return {false, m, 0}; }
    static Result success(const std::string& m, int id) { return {true, m, id}; }

    Transaction& record(Account& a, const std::string& type, long long amt, int other, const std::string& note) {
        Transaction t;
        t.id = db_.nextTxnId++;
        t.accNo = a.accNo;
        t.type = type;
        t.amount = amt;
        t.balanceAfter = a.balance;
        t.counterparty = other;
        t.timestamp = util::nowDateTime();
        t.note = util::sanitize(note);
        db_.txns.push_back(t);
        return db_.txns.back();
    }

    // Shared credential check: 3 consecutive failures block the account
    bool check(Account& a, const std::string& stored, const std::string& given) {
        if (a.blocked) { std::cout << "  ! This account is blocked. Please contact the bank admin.\n"; return false; }
        if (stored == given) {
            if (a.failedAttempts) { a.failedAttempts = 0; db_.save(); }
            return true;
        }
        a.failedAttempts++;
        if (a.failedAttempts >= MAX_ATTEMPTS) {
            a.blocked = true;
            std::cout << "  ! Too many wrong attempts. Account " << a.accNo << " has been BLOCKED for security.\n";
        } else {
            std::cout << "  ! Incorrect credentials. " << (MAX_ATTEMPTS - a.failedAttempts) << " attempt(s) left before the account is blocked.\n";
        }
        db_.save();
        return false;
    }
};

#endif
