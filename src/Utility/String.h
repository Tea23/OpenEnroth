#pragma once

#include <cstring>
#include <string>
#include <vector>

std::string StringPrintf(const char *fmt, ...);
std::string StringFromInt(int value);
std::string ToLower(std::string_view text);
std::string ToUpper(std::string_view text);
std::vector<char*> Tokenize(char* input, const char separator);

//----- (00452C30) --------------------------------------------------------
inline char *RemoveQuotes(char *str) {
    if (*str == '"') {
        str[strlen(str) - 1] = 0;
        return str + 1;
    }
    return str;
}

inline std::string TrimRemoveQuotes(std::string str) {
    while (str.length() > 0 && (str.at(0) == ' ' || str.at(0) == '"')) {
        str.erase(0, 1);
    }
    while (str.length() > 0 && (str.at(str.length() - 1) == ' ' || str.at(str.length() - 1) == '"')) {
        str.pop_back();
    }

    return str;
}

inline std::string_view trim(std::string_view s) {
    size_t l = 0;
    size_t r = s.size();
    while (l < r && s[l] == ' ')
        l++;
    while (l < r && s[r - 1] == ' ')
        r--;
    return s.substr(l, r - l);
}

bool istarts_with(std::string_view s, std::string_view prefix);
bool iequals(std::string_view a, std::string_view b);
bool iless(std::string_view a, std::string_view b);
bool iequalsAscii(std::u8string_view a, std::u8string_view b);
bool ilessAscii(std::u8string_view a, std::u8string_view b);

struct ILess {
    bool operator()(std::string_view a, std::string_view b) const {
        return iless(a, b);
    }
};

