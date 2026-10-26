// Banking Management System - Sqrock IT Solutions Internship, Project Phase 1, Task 2
// Concepts: OOP (classes, encapsulation, separation of data / logic / UI), STL, file handling (CSV),
// integer money arithmetic, input validation, hashed credentials.

#include "admin.h"
#include "atm.h"
#include "bank.h"
#include "customer.h"
#include "database.h"

int main() {
    Database db("data");
    db.load();
    Bank bank(db);
    CustomerModule customers(db, bank);
    AtmModule atm(db, bank);
    AdminModule admin(db, bank);

    try {
        while (true) {
            util::title("BANKING MANAGEMENT SYSTEM");
            std::cout << "  1. Open a new account\n  2. Customer login\n  3. ATM\n  4. Admin login\n  5. Exit\n";
            int c = util::readInt("\n  Choose: ", 1, 5);
            switch (c) {
                case 1: customers.createAccount(); break;
                case 2:
                    if (Account* a = customers.login()) customers.menu(*a);
                    break;
                case 3: atm.run(); break;
                case 4:
                    if (AdminModule::login()) admin.menu();
                    break;
                case 5:
                    db.save();
                    std::cout << "\n  Thank you for using the Banking Management System. Goodbye!\n";
                    return 0;
            }
        }
    } catch (const EndOfInput&) {
        db.save();
        std::cout << "\n  (Input closed - data saved. Goodbye!)\n";
    }
    return 0;
}
