#pragma once
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>

class Printer {
public:
    template<typename... Args>
    static void print(const std::string& format, const std::string& prefix, Args... args);
    
private:
    template<typename T, typename... Rest>
    static void processArgs(std::ostringstream& oss, const std::string& format, T value, Rest... rest);
    
    template<typename T>
    static void processArgs(std::ostringstream& oss, const std::string& format, T value);
    
    static void replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, int value);
    static void replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, double value);
    static void replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, float value);
    static void replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, long value);
    static void replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, short value);
    static void replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, unsigned int value);
    static void replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, char value);
    static void replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, const char* value);
    static void replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, std::string value);
    static void replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, bool value);
    static void replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, wchar_t value);
    static void replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, unsigned char value);

    template <typename T>
    static void replaceFormatSpecifiers(std::ostringstream& oss, const std::string&, T* value);

    template <typename T>
    static void replaceFormatSpecifiers(std::ostringstream& oss, const std::string&, const std::vector<T>& vec);

    template <typename K, typename V>
    static void replaceFormatSpecifiers(std::ostringstream& oss, const std::string&, const std::unordered_map<K, V>& map);

    template <typename T, std::size_t N>
    static void replaceFormatSpecifiers(std::ostringstream& oss, const std::string&, const T (&arr)[N]);
}; 

template <typename T>
static void replaceFormatSpecifiers(std::ostringstream& oss, const std::string&, T* value) {
    oss << value; // Print pointer address
}

template <typename T>
static void replaceFormatSpecifiers(std::ostringstream& oss, const std::string&, const std::vector<T>& vec) {
    oss << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        oss << vec[i];
        if (i < vec.size() - 1) {
            oss << ", ";
        }
    }
    oss << "]";
}

template <typename K, typename V>
static void replaceFormatSpecifiers(std::ostringstream& oss, const std::string&, const std::unordered_map<K, V>& map) {
    oss << "{";
    for (auto it = map.begin(); it != map.end(); ++it) {
        oss << it->first << ": " << it->second;
        if (std::next(it) != map.end()) {
            oss << ", ";
        }
    }
    oss << "}";
}

template <typename T, std::size_t N>
static void replaceFormatSpecifiers(std::ostringstream& oss, const std::string&, const T (&arr)[N]) {
    oss << "[";
    for (size_t i = 0; i < N; ++i) {
        oss << arr[i];
        if (i < N - 1) {
            oss << ", ";
        }
    }
    oss << "]";
} 