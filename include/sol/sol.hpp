#pragma once
#include <string>
#include <functional>
#include <vector>
#include <map>
#include <memory>

// This is a stub for the sol2 library
// Only includes minimal necessary declarations to allow compilation
// For production use, download and include the actual sol2 library

namespace sol {
    class state {
    public:
        state() = default;
        
        void open_libraries() {}
        
        template<typename T>
        void set_function(const std::string& name, T func) {}
        
        void script(const std::string& code) {}
        
        bool operator_import() { return true; }
    };
    
    class object {
    public:
        bool is_function() const { return false; }
        
        template<typename... Args>
        void operator()(Args&&... args) {}
    };
    
    class table {
    public:
        template<typename K, typename V>
        void set(K&& key, V&& value) {}
        
        template<typename K>
        object get(K&& key) { return object(); }
    };
    
    namespace lib {
        struct base_t {};
        struct package_t {};
        struct coroutine_t {};
        struct string_t {};
        struct table_t {};
        struct math_t {};
        struct bit32_t {};
        struct io_t {};
        struct os_t {};
        struct debug_t {};
        
        constexpr base_t base{};
        constexpr package_t package{};
        constexpr coroutine_t coroutine{};
        constexpr string_t string{};
        constexpr table_t table{};
        constexpr math_t math{};
        constexpr bit32_t bit32{};
        constexpr io_t io{};
        constexpr os_t os{};
        constexpr debug_t debug{};
    }
    
    template<typename T>
    struct usertype {
        template<typename... Args>
        static auto make(Args&&... args) {
            return usertype<T>{};
        }
    };
    
    template<typename T>
    auto make_usertype(const std::string& name, T&& t) {
        return table();
    }
    
    namespace types {
        enum type : int {
            none = 0,
            lua_nil = 1,
            string = 2,
            number = 3,
            thread = 4,
            boolean = 5,
            function = 6,
            userdata = 7,
            lightuserdata = 8,
            table = 9,
            poly = 10,
            optional = 11,
            any = 12
        };
    }
}

// Add fmt::format compatibility
namespace fmt {
    template<typename... Args>
    std::string format(const std::string& format_str, Args&&... args) {
        return format_str;  // Stub implementation
    }
} 