#include "logger.h"
 #include "Printer.h"

// Variadic template function for printing with a delimiter
template <typename... Args>
void Printer::print(const std::string& format, const std::string& delim, Args... args) {
    std::ostringstream oss;
    formatToStream(oss, format, delim, args...);
    str out = oss.str();
    lo.log(out);  // Print with newline at the end
}

// Helper function to format the output
template <typename T>
void Printer::formatToStream(std::ostringstream& oss, const std::string& format, const std::string& delim, T value) {
    replaceFormatSpecifiers(oss, format, value);
}

template <typename T, typename... Args>
void Printer::formatToStream(std::ostringstream& oss, const std::string& format, const std::string& delim, T value, Args... args) {
    std::size_t pos = format.find('%');
    if (pos != std::string::npos) {
        oss << format.substr(0, pos);
        oss << value;
        formatToStream(oss, format.substr(pos + 1), delim, args...);
    } else {
        oss << format; // No more format specifiers, print the remaining format
    }
    if constexpr (sizeof...(args) > 0) {
        oss << delim;
    }
}

// Replace format specifiers for various types
void Printer::replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, int value) {
    oss << value;
}

void Printer::replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, double value) {
    oss << value;
}

void Printer::replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, float value) {
    oss << value;
}

void Printer::replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, long value) {
    oss << value;
}

void Printer::replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, short value) {
    oss << value;
}

void Printer::replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, unsigned int value) {
    oss << value;
}

void Printer::replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, char value) {
    oss << value;
}

void Printer::replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, const char* value) {
    oss << value;
}

void Printer::replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, std::string value) {
    oss << value;
}

void Printer::replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, bool value) {
    oss << (value ? "true" : "false");
}

void Printer::replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, wchar_t value) {
    oss << static_cast<char>(value); // Basic conversion; can be extended for better handling
}

// Pointer types
template <typename T>
void Printer::replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, T* value) {
    oss << value; // Print pointer address
}

// Handling std::vector
template <typename T>
void Printer::replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, const std::vector<T>& vec) {
    oss << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        oss << vec[i];
        if (i < vec.size() - 1) {
            oss << ", ";
        }
    }
    oss << "]";
}

// Handling std::unordered_map
template <typename K, typename V>
void Printer::replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, const std::unordered_map<K, V>& map) {
    oss << "{";
    for (auto it = map.begin(); it != map.end(); ++it) {
        oss << it->first << ": " << it->second;
        if (std::next(it) != map.end()) {
            oss << ", ";
        }
    }
    oss << "}";
}

// Handling arrays
template <typename T, std::size_t N>
void Printer::replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, const T (&arr)[N]) {
    oss << "[";
    for (size_t i = 0; i < N; ++i) {
        oss << arr[i];
        if (i < N - 1) {
            oss << ", ";
        }
    }
    oss << "]";
}

void Printer::replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, byte value) {
    oss << static_cast<int>(value);
}

void Printer::replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, word value) {
    oss << value;
}

void Printer::replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, dword value) {
    oss << value;
}

#ifdef WINDOWS
// HWND type simulation (using void pointer for demonstration)
void Printer::replaceFormatSpecifiers(std::ostringstream& oss, const std::string& format, HWND hwnd) {
    oss << hwnd; // Print HWND as pointer address
}
#endif