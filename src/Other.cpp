#include "types.hpp"
#define UNLEN 256

class UserIdentifier {
public:
  // Get the current user ID
  static str GetUser() {
    #ifdef _WIN32
    // Windows specific implementation
    char username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    if (GetUserNameA(username, &username_len)) {
      return str(username);
    } else {
      // Handle error (e.g., set uid to "unknown")
      return "unknown";
    }
    #else
    // Linux/Unix specific implementation
    uid_t user_id = getuid();
    passwd* user_info = getpwuid(user_id);
    if (user_info != nullptr) {
      return str(user_info->pw_name);
    } else {
      // Handle error (e.g., set uid to "unknown")
      return "unknown";
    }
    #endif
  }
};