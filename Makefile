CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

bank: main.cpp utils.h models.h database.h bank.h customer.h atm.h admin.h
	$(CXX) $(CXXFLAGS) -o bank main.cpp

run: bank
	./bank

clean:
	rm -f bank
