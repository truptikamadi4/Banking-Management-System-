// utils.h - small helper functions shared by every module
#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Thrown when the input stream ends (Ctrl+D / Ctrl+Z or piped input finished)
struct EndOfInput {};

namespace util {

inline std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) a++;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) b--;
    return s.substr(a, b - a);
}

inline std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> out;
    std::string item;
    std::stringstream ss(s);
    while (std::getline(ss, item, delim)) out.push_back(item);
    if (!s.empty() && s.back() == delim) out.push_back("");
    return out;
}

inline std::string join(const std::vector<std::string>& v, char delim) {
    std::string out;
    for (size_t i = 0; i < v.size(); i++) {
        if (i) out += delim;
        out += v[i];
    }
    return out;
}

// CSV files use ',' as separator, so user text must not contain it
inline std::string sanitize(std::string s) {
    for (char& c : s)
        if (c == ',' || c == '\n' || c == '\r' || c == ';') c = ' ';
    return trim(s);
}

inline std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

// Simple (NOT cryptographically secure) password hash - fine for a learning project
inline std::string hashPassword(const std::string& pw) {
    std::hash<std::string> h;
    std::ostringstream os;
    os << std::hex << h("bank#salt#" + pw);
    return os.str();
}

inline std::string money(double v) {
    std::ostringstream os;
    os << "Rs. " << std::fixed << std::setprecision(2) << v;
    return os.str();
}

inline std::string fixed1(double v) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(1) << v;
    return os.str();
}

inline std::string nowDateTime() {
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", std::localtime(&t));
    return buf;
}

inline std::string nowHM() {
    std::time_t t = std::time(nullptr);
    char buf[8];
    std::strftime(buf, sizeof(buf), "%H:%M", std::localtime(&t));
    return buf;
}

// "HH:MM" -> true if it is a valid 24h time
inline bool validTime(const std::string& s, int* hour = nullptr) {
    if (s.size() != 5 || s[2] != ':') return false;
    if (!std::isdigit(static_cast<unsigned char>(s[0])) || !std::isdigit(static_cast<unsigned char>(s[1])) ||
        !std::isdigit(static_cast<unsigned char>(s[3])) || !std::isdigit(static_cast<unsigned char>(s[4])))
        return false;
    int h = std::stoi(s.substr(0, 2)), m = std::stoi(s.substr(3, 2));
    if (h > 23 || m > 59) return false;
    if (hour) *hour = h;
    return true;
}

inline bool isDigits(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c); });
}

// ---------- money helpers: all amounts are stored as integer paise (no floating point errors) ----------

// "1234.5" -> 123450 ; returns -1 when the text is not a valid amount
inline long long parseAmount(const std::string& s) {
    if (s.empty() || s.size() > 12) return -1;
    size_t dot = s.find('.');
    std::string whole = dot == std::string::npos ? s : s.substr(0, dot);
    std::string frac = dot == std::string::npos ? "" : s.substr(dot + 1);
    if (!isDigits(whole)) return -1;
    if (dot != std::string::npos && (frac.empty() || frac.size() > 2 || !isDigits(frac))) return -1;
    while (frac.size() < 2) frac += '0';
    return std::stoll(whole) * 100 + std::stoll(frac);
}

// 123456789 paise -> "Rs. 1,234,567.89"
inline std::string rs(long long paise) {
    bool neg = paise < 0;
    if (neg) paise = -paise;
    std::string whole = std::to_string(paise / 100);
    for (int i = static_cast<int>(whole.size()) - 3; i > 0; i -= 3) whole.insert(i, ",");
    std::ostringstream os;
    os << (neg ? "-" : "") << "Rs. " << whole << "." << std::setw(2) << std::setfill('0') << (paise % 100);
    return os.str();
}

inline std::string currentMonth() {
    std::time_t t = std::time(nullptr);
    char buf[16];
    std::strftime(buf, sizeof(buf), "%Y-%m", std::localtime(&t));
    return buf;
}

inline std::string today() {
    std::time_t t = std::time(nullptr);
    char buf[16];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", std::localtime(&t));
    return buf;
}

// ---------- input helpers (all throw EndOfInput when stdin closes) ----------

inline std::string readLine(const std::string& prompt) {
    std::cout << prompt << std::flush;
    std::string s;
    if (!std::getline(std::cin, s)) throw EndOfInput();
    return trim(s);
}

inline std::string readNonEmpty(const std::string& prompt) {
    while (true) {
        std::string s = sanitize(readLine(prompt));
        if (!s.empty()) return s;
        std::cout << "  ! This field cannot be empty.\n";
    }
}

inline int readInt(const std::string& prompt, int lo, int hi) {
    while (true) {
        std::string s = readLine(prompt);
        bool ok = !s.empty();
        size_t start = (ok && s[0] == '-') ? 1 : 0;
        if (start == s.size()) ok = false;
        for (size_t i = start; ok && i < s.size(); i++)
            if (!std::isdigit(static_cast<unsigned char>(s[i]))) ok = false;
        if (ok && s.size() < 9) {
            int v = std::stoi(s);
            if (v >= lo && v <= hi) return v;
        }
        std::cout << "  ! Please enter a number between " << lo << " and " << hi << ".\n";
    }
}

// Reads a positive money amount (max 10 crore); returns paise
inline long long readAmount(const std::string& prompt) {
    while (true) {
        long long v = parseAmount(readLine(prompt));
        if (v > 0 && v <= 10000000000LL) return v;
        std::cout << "  ! Enter a valid positive amount, e.g. 2500 or 2500.50 (max 10 crore).\n";
    }
}

inline bool confirm(const std::string& prompt) {
    std::string a = toLower(readLine(prompt + " (y/n): "));
    return a == "y" || a == "yes";
}

inline void line(char c = '-', int n = 60) { std::cout << std::string(n, c) << "\n"; }

inline void title(const std::string& t) {
    std::cout << "\n";
    line('=');
    std::cout << "  " << t << "\n";
    line('=');
}

}  // namespace util

#endif
