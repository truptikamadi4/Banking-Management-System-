// models.h - entity classes: Account, Transaction, Loan (money is stored in paise)
#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <vector>

namespace TxType {
const std::string Deposit = "Deposit";
const std::string Withdrawal = "Withdrawal";
const std::string TransferOut = "Transfer-Out";
const std::string TransferIn = "Transfer-In";
const std::string Interest = "Interest";
const std::string LoanCredit = "Loan-Credit";
const std::string LoanRepay = "Loan-EMI";
}  // namespace TxType

class Account {
public:
    int accNo = 0;
    std::string name;
    std::string phone;
    std::string type;  // Savings / Current
    long long balance = 0;
    std::string passHash;
    std::string pinHash;
    bool blocked = false;
    std::string createdAt;
    int failedAttempts = 0;
    std::string lastInterestMonth;  // YYYY-MM when interest was last credited

    double interestRatePercent() const { return type == "Savings" ? 4.0 : 0.0; }
};

struct Transaction {
    int id = 0;
    int accNo = 0;
    std::string type;
    long long amount = 0;
    long long balanceAfter = 0;
    int counterparty = 0;  // other account for transfers (0 = none)
    std::string timestamp;
    std::string note;
};

struct Loan {
    int id = 0;
    int accNo = 0;
    long long principal = 0;
    double annualRate = 0;  // percent
    int months = 0;
    long long emi = 0;
    long long outstanding = 0;  // remaining principal
    int monthsLeft = 0;
    std::string status = "Pending";  // Pending / Active / Closed / Rejected
    std::string appliedAt;
};

#endif
