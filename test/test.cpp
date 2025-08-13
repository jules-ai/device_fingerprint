#include "../sdk/include/fingerprint.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <functional>
#include <algorithm>

// --- Test Utilities ---
bool test_passed = true;
#define CHECK(condition, message)                                                                         \
    if (!(condition))                                                                                     \
    {                                                                                                     \
        std::cerr << "FAILED: " << message << " (at " << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
        test_passed = false;                                                                              \
    }                                                                                                     \
    else                                                                                                  \
    {                                                                                                     \
        std::cout << "PASSED: " << message << std::endl;                                                  \
    }

// --- Helper functions copied from fingerprint.cpp for testing ---

// Helper function to trim strings
static std::string trim(const std::string &str)
{
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (std::string::npos == first)
    {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(first, (last - first + 1));
}

// Hashing function
std::string generate_hash(const std::string &input)
{
    std::hash<std::string> hasher;
    std::stringstream ss;
    ss << std::hex << hasher(input);
    return ss.str();
}

// --- Test Cases ---

void test_trim_function()
{
    std::cout << "\n--- Testing trim() ---" << std::endl;
    CHECK(trim("  hello  ") == "hello", "Trim with leading/trailing spaces");
    CHECK(trim("\tworld\t") == "world", "Trim with tabs");
    CHECK(trim("\n\r a b c \n\r") == "a b c", "Trim with newlines and other whitespace");
    CHECK(trim("no_spaces") == "no_spaces", "Trim with no spaces");
    CHECK(trim("") == "", "Trim empty string");
    CHECK(trim("   ") == "", "Trim string with only spaces");
}

void test_hash_function()
{
    std::cout << "\n--- Testing generate_hash() ---" << std::endl;
    // The exact hash value is implementation-defined, but for a given STL implementation, it should be consistent.
    // We'll just check that it produces a non-empty, hex-like string.
    std::string hash_result = generate_hash("test_string");
    CHECK(!hash_result.empty(), "Hash result is not empty");

    // Check if the hash is a plausible hex string
    bool is_hex = true;
    for (char c : hash_result)
    {
        if (!isxdigit(c))
        {
            is_hex = false;
            break;
        }
    }
    CHECK(is_hex, "Hash result is a hex string");
}

void test_fingerprint_api()
{
    std::cout << "\n--- Testing Fingerprint API ---" << std::endl;
    std::string fp1 = get_machine_fingerprint();
    CHECK(!fp1.empty(), "Fingerprint should not be empty");

    std::string fp2 = get_machine_fingerprint();
    CHECK(fp1 == fp2, "Fingerprint should be consistent across multiple calls");
}

int main()
{
    test_trim_function();
    test_hash_function();
    test_fingerprint_api();

    if (!test_passed)
    {
        std::cerr << "\n*** Some tests failed. ***" << std::endl;
        return 1;
    }

    std::cout << "\n*** All tests passed! ***" << std::endl;
    return 0;
}
