#ifndef __PROGTEST__
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <climits>
#include <cstdint>
#include <cassert>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <memory>
#include <functional>
#include <compare>
#include <stdexcept>
#endif /* __PROGTEST__ */

std::string removeZero(std::string str)
{
    // Count leading zeros
    int i = 0;
    while (str[i] == '0')
        i++;

    // The erase function removes i characters
    // from given index (0 here)
    str.erase(0, i);
    if (str == "") {str = "0";}

    return str;
}

class CBigInt
{
  private:
    // Helper function to add two positive numbers
    std::string addPositiveStrings(std::string a, std::string b) const { // TODO
        a.erase(0, std::min(a.find_first_not_of('0'), a.size()-1));
        b.erase(0, std::min(b.find_first_not_of('0'), b.size()-1));

        // If both strings become empty, return "0"
        if (a.empty() && b.empty())
            return "0";

        // Before proceeding further, make sure length
        // of b is larger.
        if (a.length() > b.length())
            swap(a, b);

        // Take an empty string for storing result
        std::string str = "";

        // Calculate length of both string
        int n1 = a.length(), n2 = b.length();
        int diff = n2 - n1;

        // Initially take carry zero
        int carry = 0;

        // Traverse from end of both strings
        for (int i=n1-1; i>=0; i--)
        {
            // Do school mathematics, compute sum of
            // current digits and carry
            int sum = ((a[i]-'0') +
                       (b[i+diff]-'0') +
                       carry);
            str.push_back(sum%10 + '0');
            carry = sum/10;
        }

        // Add remaining digits of b[]
        for (int i=n2-n1-1; i>=0; i--)
        {
            int sum = ((b[i]-'0')+carry);
            str.push_back(sum%10 + '0');
            carry = sum/10;
        }

        // Add remaining carry
        if (carry)
            str.push_back(carry+'0');

        // reverse resultant string
        reverse(str.begin(), str.end());

        return str;
    }

    // Helper function to subtract smaller from larger (both positive, larger >= smaller)
    std::string subtractPositiveStrings(const std::string& larger, const std::string& smaller) const {
        // Assuming larger and smaller are positive and larger >= smaller in value
        size_t n1 = larger.length(), n2 = smaller.length();
        size_t diff = n1 - n2;
        std::string result = "";

        int carry = 0;
        for (int i = n2 - 1; i >= 0; i--) {
            int sub = (larger[i + diff] - '0') - (smaller[i] - '0') - carry;
            if (sub < 0) {
                sub += 10;
                carry = 1;
            } else {
                carry = 0;
            }
            result.insert(0, 1, sub + '0');
        }

        for (int i = diff - 1; i >= 0; i--) {
            if (larger[i] == '0' && carry) {
                result.insert(0, 1, '9');
                continue;
            }
            int sub = (larger[i] - '0') - carry;
            if (i > 0 || sub > 0) {  // Prevent leading zeros
                result.insert(0, 1, sub + '0');
            }
            carry = 0;
        }

        // Removing leading zeros from the result
        size_t startpos = result.find_first_not_of('0');
        if (std::string::npos != startpos) {
            return result.substr(startpos);
        }
        return "0";  // Return "0" if all digits were zero
    }

    CBigInt addDigit(const CBigInt& val1, const CBigInt& val2) const {
        // Simplified logic for adding two CBigInt numbers
        std::string resultDigits;
        bool resultNegative = false;

        if (val1.m_negative == val2.m_negative) {
            resultDigits = addPositiveStrings(val1.m_digits, val2.m_digits);
            resultNegative = val1.m_negative; // Both have same sign
        } else {
            // Determine which is larger and subtract smaller from larger
            if (val1.m_digits.length() > val2.m_digits.length() || (val1.m_digits.length() == val2.m_digits.length() && val1.m_digits >= val2.m_digits)) {
                resultDigits = subtractPositiveStrings(val1.m_digits, val2.m_digits);
                resultNegative = val1.m_negative;
            } else {
                resultDigits = subtractPositiveStrings(val2.m_digits, val1.m_digits);
                resultNegative = val2.m_negative;
            }
        }
        CBigInt temp;
        temp.m_digits = resultDigits;
        temp.m_negative = resultNegative;

        return temp;
    }

    std::string multiplyPositiveStrings(const std::string& a, const std::string& b) const {
        size_t n1 = a.size();
        size_t n2 = b.size();
        std::string result(n1 + n2, '0');

        for (int i = n1 - 1; i >= 0; i--) {
            for (int j = n2 - 1; j >= 0; j--) {
                int sum = (a[i] - '0') * (b[j] - '0') + (result[i + j + 1] - '0');
                result[i + j + 1] = sum % 10 + '0';
                result[i + j] += sum / 10;
            }
        }

        // Remove leading zeros
        size_t start = result.find_first_not_of('0');
        if (start != std::string::npos) {
            return result.substr(start);
        }
        return "0"; // Return "0" if result is all zeros
    }
     int compareNums(const CBigInt& a, const CBigInt& b) const {
        // If one number is negative and the other is positive
        if (a.m_negative && !b.m_negative) return -1; // a is smaller
        if (!a.m_negative && b.m_negative) return 1;  // a is larger

        // If both are positive or both are negative
        if (a.m_digits.length() > b.m_digits.length()) return a.m_negative ? -1 : 1; // If negative, a is smaller
        if (a.m_digits.length() < b.m_digits.length()) return a.m_negative ? 1 : -1; // If negative, a is larger

        // Same length, need to compare digit by digit
        for (size_t i = 0; i < a.m_digits.length(); ++i) {
            if (a.m_digits[i] > b.m_digits[i]) return a.m_negative ? -1 : 1; // If negative, a is smaller
            if (a.m_digits[i] < b.m_digits[i]) return a.m_negative ? 1 : -1; // If negative, a is larger
        }

        // Numbers are equal
        return 0;
    }
    bool isANum( const std::string& s ) const {
        int start;
        if( s[0] == '-' && isdigit( s[1] ) ) {
            start = 1;
        }
        else {
            start = 0;
        }

        for( size_t i = start; i < s.length(); i++ ) {
            if( !isdigit( s[i] )) {
                return false;
            }
        }
        return true;
    }
  public:
    // Default constructor
    CBigInt() : m_digits("0"), m_negative(false) {}

    // Copy constructor
    CBigInt(const CBigInt & value) = default;
    CBigInt(const char* value) {
        if (isANum(value) == false) {
            throw std::invalid_argument("Invalid syntax.");
        }
        if(value[0] == '-') {
            m_negative = true;
            m_digits = value;
            m_digits.erase(0, 1);
        } else {
            m_digits = value;
            m_negative = false;
        }
        m_digits = removeZero(m_digits);
    }
    CBigInt(const int value) {
        if (value < 0) {
            m_negative = true;
            m_digits = std::to_string(abs(value));
        } else {
            m_digits = std::to_string(value);
            m_negative = false;
        }
    }

    CBigInt& operator=(const CBigInt & other) = default;

    CBigInt& operator+=(const CBigInt& other);

    friend CBigInt operator+(const CBigInt& l, const CBigInt& r);

    CBigInt& operator*=(const CBigInt& other);

    friend CBigInt operator*(const CBigInt& lhs, const CBigInt& rhs);

    friend bool operator<(const CBigInt& lhs, const CBigInt& rhs);
    friend bool operator<=(const CBigInt& lhs, const CBigInt& rhs);
    friend bool operator>(const CBigInt& lhs, const CBigInt& rhs);
    friend bool operator>=(const CBigInt& lhs, const CBigInt& rhs);
    friend bool operator==(const CBigInt& lhs, const CBigInt& rhs);
    friend bool operator!=(const CBigInt& lhs, const CBigInt& rhs);

    friend std::ostream& operator<<(std::ostream& os, const CBigInt& b);
    friend std::istream& operator>>(std::istream& is, CBigInt& b);

  private:
    std::string m_digits; // Least significant digit first
    bool m_negative; // Sign of the number
};

std::ostream& operator<<(std::ostream& os, const CBigInt& b) {
    if (b.m_negative) {
        os << '-';
    }
    os << b.m_digits;
    return os;
}

/*std::istream& operator>>(std::istream& is, CBigInt& b) {
    std::string input;
    b.m_digits = "";
    b.m_negative = false;
    if (is >> input) {  // Read a string from the input stream
        // Initialize as positive by default
        b.m_negative = false;
        bool negative = false;
        bool nonZero = false;

        // Find the first character that is either a minus or a non-zero digit
        std::size_t startIndex = input.find_first_of("-123456789");

        // If string is non-empty and contains valid characters
        if (startIndex != std::string::npos) {
            if (input[startIndex] == '-') {
                negative = true;
                // Find the first non-zero digit after the minus sign
                startIndex++;
                if (std::isdigit(input[startIndex]) == false) {
                    is.setstate(std::ios::failbit);
                }
            }

            for (size_t i = startIndex; i < input.length(); ++i) {
                if (std::isdigit(input[i])) {
                    if (input[i] != '0') {
                        nonZero = true;
                    }
                    b.m_digits.push_back(input[i]);
                } else {
                    break;
                }
            }
            b.m_negative = negative;
            b.m_digits = removeZero(b.m_digits);
            if (nonZero == false) {
                b.m_digits = "0";
                b.m_negative = false;
            }

        } else {
            // Input doesn't have any digits or minus (should probably handle as an error)
            is.setstate(std::ios::failbit);
        }
    }
    return is;  // Return the input stream to allow chain calls
}*/

std::istream& operator>>(std::istream& is, CBigInt& b) {
    std::string input;
    char ch;
    bool numberStarted = false;
    bool isNegative = false;

    // Skip leading whitespaces
    while (isspace(is.peek())) {
        is.get();
    }

    // Check for negative sign
    if (is.peek() == '-') {
        is.get(ch);
        isNegative = true;
        if (!isdigit(is.peek())) { // After '-' there must be a digit
            is.setstate(std::ios::failbit);
            return is;
        }
    }

    // Read digits
    while (isdigit(is.peek())) {
        numberStarted = true;
        is.get(ch);
        input += ch;
    }

    // If no digit was read, set failbit
    if (!numberStarted) {
        is.setstate(std::ios::failbit);
        return is;
    }

    // Set the values only if the whole reading operation was successful
    b.m_negative = isNegative;
    b.m_digits = numberStarted ? removeZero(input) : "0";

    return is;
}

CBigInt operator+(const CBigInt& l, const CBigInt& r) {
    CBigInt result = l.addDigit(l, r);
    return result;
}

CBigInt operator*(const CBigInt& lhs, const CBigInt& rhs) {
    CBigInt result;
    // Call a private member function for the actual multiplication operation
    result.m_digits = lhs.multiplyPositiveStrings(lhs.m_digits, rhs.m_digits);
    result.m_negative = lhs.m_negative != rhs.m_negative; // Determine the sign
    if (lhs.m_digits == "0" || rhs.m_digits == "0" ) {
        result.m_negative = false;
    }
    return result;
}

CBigInt& CBigInt::operator+=(const CBigInt& other) {
    return *this = *this + other;
}

CBigInt& CBigInt::operator*=(const CBigInt& other) {
    return *this = *this * other;
}

bool operator<(const CBigInt& l, const CBigInt& r) {
    return l.compareNums(l, r) < 0;
}
bool operator<=(const CBigInt& l, const CBigInt& r) {
    return l.compareNums(l, r) <= 0;
}
bool operator>(const CBigInt& l, const CBigInt& r) {
    return l.compareNums(l, r) > 0;
}
bool operator>=(const CBigInt& l, const CBigInt& r) {
    return l.compareNums(l, r) >= 0;
}
bool operator==(const CBigInt& l, const CBigInt& r) {
    return l.compareNums(l, r) == 0;
}
bool operator!=(const CBigInt& l, const CBigInt& r) {
    return l.compareNums(l, r) != 0;
}

#ifndef __PROGTEST__
static bool equal ( const CBigInt & x, const char val [] )
{
  std::ostringstream oss;
  oss << x;
  return oss . str () == val;
}
static bool equalHex ( const CBigInt & x, const char val [] )
{
  return true; // hex output is needed for bonus tests only
  // std::ostringstream oss;
  // oss << std::hex << x;
  // return oss . str () == val;
}

void testAddition() {
    CBigInt a("1234567890");
    CBigInt b("9876543210");
    CBigInt c = a + b;
    assert(c == "11111111100");

    CBigInt d("-100");
    CBigInt e("50");
    assert((d + e) == "-50");

    assert(equal((CBigInt("0") + CBigInt("0")), "0"));
}

void testMultiplication() {
    CBigInt a("12345");
    CBigInt b("6789");
    assert((a * b) == "83810205");

    CBigInt c("-12345");
    assert((a * c) == "-152399025");

    assert(equal((CBigInt("123") * CBigInt("0")), "0"));
}

void testComparison() {
    CBigInt a("12345");
    CBigInt b("54321");
    assert(a < b);
    assert(a <= b);
    assert(b > a);
    assert(b >= a);
    assert(a != b);
    assert(a == CBigInt("12345"));
}

void testExceptions() {
    // Test for basic arithmetic correctness
    assert(CBigInt("5") + CBigInt("3") == CBigInt("8"));
    assert(CBigInt("6") * CBigInt("7") == CBigInt("42"));

    // Test for large number handling
    assert(CBigInt("12345678901234567890") + CBigInt("98765432109876543210") == CBigInt("111111111011111111100"));

    // Test with leading zeros
    assert(CBigInt("0000123") + CBigInt("00077") == CBigInt("200"));

    // Test negative number arithmetic
    assert(CBigInt("-5") * CBigInt("-4") == CBigInt("20"));

    // Edge cases and input validation
    // Assuming that CBigInt throws an exception for invalid input
    try {
        CBigInt a = "abc"; // should throw an exception
        assert("missing an exception" == nullptr); // should not reach this point
    } catch (const std::invalid_argument& e) {
        // expected
    }

    // Zero handling
    assert(CBigInt("0") + CBigInt("0") == CBigInt("0"));
    assert(CBigInt("0") * CBigInt("1234") == CBigInt("0"));

    // Testing comparison operators
   assert(CBigInt("500") > CBigInt("400"));
    assert(CBigInt("-500") <  CBigInt("500"));
    assert(CBigInt("123") == CBigInt("123"));


    // Arithmetic with Zero
    assert(CBigInt("0") * CBigInt("1234") == CBigInt("0"));
    // Division by zero would normally throw an exception, but since it's not defined here, it's a placeholder
    /*try {
        CBigInt result = CBigInt("1234") / CBigInt("0");
        assert(false); // should not reach here
    } catch (const std::exception& e) {
        // Expected exception for division by zero
    }*/

    // Negative Number Handling
    assert(CBigInt("-100") + CBigInt("-200") == CBigInt("-300"));
    assert(CBigInt("-100") * CBigInt("-200") == CBigInt("20000"));

    // Comparison Logic
    assert(CBigInt("-100") < CBigInt("100"));
    assert(CBigInt("100") > CBigInt("-100"));
    assert(CBigInt("100") != CBigInt("-100"));

    // Memory Management and Performance cannot be asserted this way, they require
    // profiling and analysis tools to monitor memory usage and execution time.

    // Robustness to Bad Input
    try {
        CBigInt invalid("not a number");
        assert(false); // should not reach here if exception is thrown properly
    } catch (const std::invalid_argument& e) {
        // Expected path for invalid input
    }

    // Immutable Behavior
    CBigInt a("100");
    CBigInt b("200");
    CBigInt c = a + b;
    assert(a == CBigInt("100")); // Ensure 'a' remains unchanged after addition
    assert(b == CBigInt("200")); // Ensure 'b' remains unchanged after addition
}
void fuzz() {
    CBigInt a{};
    CBigInt b{};
    a = "10104";
    b = "-6253327645483";
    assert(equal(a + b, "-6253327635379"));
    a = "-759775";
    b = "98648408";
    assert(equal(a + b, "97888633"));
    a = "3294767281734";
    b = "30327367095257363";
    assert(equal(a + b, "30330661862539097"));
    a = "-136890576";
    b = "1014676387988518";
    assert(equal(a + b, "1014676251097942"));
    a = "-1969686";
    b = "-1494590";
    assert(equal(a + b, "-3464276"));
    a = "-3041284552";
    b = "2010897";
    assert(equal(a + b, "-3039273655"));
    a = "-829114490593";
    b = "808264853492152592";
    assert(equal(a + b, "808264024377661999"));
    a = "-54135227";
    b = "-84";
    assert(equal(a + b, "-54135311"));
    a = "-105133903366057304";
    b = "37578177449";
    assert(equal(a + b, "-105133865787879855"));
    a = "3016837";
    b = "484276041400770";
    assert(equal(a + b, "484276044417607"));
    a = "4636";
    b = "-9925366454300613";
    assert(equal(a + b, "-9925366454295977"));
    a = "1044779256638";
    b = "-3991098465479";
    assert(equal(a + b, "-2946319208841"));
    a = "572465684894206097";
    b = "-43186456482";
    assert(equal(a + b, "572465641707749615"));
    a = "-11";
    b = "-3220269164175";
    assert(equal(a + b, "-3220269164186"));
    a = "203775669725";
    b = "-38";
    assert(equal(a + b, "203775669687"));
    a = "-8919056590077257125";
    b = "6413833";
    assert(equal(a + b, "-8919056590070843292"));
    a = "-3118";
    b = "234388895528913";
    assert(equal(a + b, "234388895525795"));
    a = "287408981";
    b = "11331598617512231";
    assert(equal(a + b, "11331598904921212"));
    a = "-815249001254788";
    b = "-406";
    assert(equal(a + b, "-815249001255194"));
    a = "1031935";
    b = "3960937";
    assert(equal(a + b, "4992872"));
    a = "-112757334354";
    b = "-602707943";
    assert(equal(a + b, "-113360042297"));
    a = "-5372";
    b = "10236051495132";
    assert(equal(a + b, "10236051489760"));
    a = "-470971166";
    b = "-44020985251251";
    assert(equal(a + b, "-44021456222417"));
    a = "35772";
    b = "-373122643721380924";
    assert(equal(a + b, "-373122643721345152"));
    a = "15796927447";
    b = "1748571402134";
    assert(equal(a + b, "1764368329581"));
    a = "-124043947";
    b = "-587464";
    assert(equal(a + b, "-124631411"));
    a = "234784235";
    b = "114492083908333";
    assert(equal(a + b, "114492318692568"));
    a = "-32361729864777";
    b = "375221476932531082";
    assert(equal(a + b, "375189115202666305"));
    a = "-1393056603726884239";
    b = "-1094345639";
    assert(equal(a + b, "-1393056604821229878"));
    a = "-1";
    b = "2116894016612070";
    assert(equal(a + b, "2116894016612069"));
    a = "-5919343558344";
    b = "-1075732764009";
    assert(equal(a + b, "-6995076322353"));
    a = "-106170998";
    b = "19765111890024";
    assert(equal(a + b, "19765005719026"));
    a = "-558237258873314857";
    b = "-99553308098568";
    assert(equal(a + b, "-558336812181413425"));
    a = "-509107";
    b = "-9";
    assert(equal(a + b, "-509116"));
    a = "-106498393396310181";
    b = "-5208123226001352";
    assert(equal(a + b, "-111706516622311533"));
    a = "-902813678743221094";
    b = "72328717424782";
    assert(equal(a + b, "-902741350025796312"));
    a = "6605839460";
    b = "19674";
    assert(equal(a + b, "6605859134"));
    a = "-5202771";
    b = "2908442767";
    assert(equal(a + b, "2903239996"));
    a = "-101710131";
    b = "437984";
    assert(equal(a + b, "-101272147"));
    a = "50234002";
    b = "1743681075";
    assert(equal(a + b, "1793915077"));
    a = "-27572663";
    b = "-394634083";
    assert(equal(a + b, "-422206746"));
    a = "-225470071593";
    b = "1";
    assert(equal(a + b, "-225470071592"));
    a = "505681254";
    b = "193470606859696071";
    assert(equal(a + b, "193470607365377325"));
    a = "155810";
    b = "-43920";
    assert(equal(a + b, "111890"));
    a = "898739202593";
    b = "1221147";
    assert(equal(a + b, "898740423740"));
    a = "3";
    b = "-3258297433685918396";
    assert(equal(a + b, "-3258297433685918393"));
    a = "-8174";
    b = "37538120385054";
    assert(equal(a + b, "37538120376880"));
    a = "360392953396";
    b = "7544";
    assert(equal(a + b, "360392960940"));
    a = "-33338882875551630";
    b = "1405367614";
    assert(equal(a + b, "-33338881470184016"));
    a = "2678";
    b = "1244141532218507";
    assert(equal(a + b, "1244141532221185"));
    a = "374407290916";
    b = "30208008022";
    assert(equal(a + b, "404615298938"));
    a = "1259984341882";
    b = "137106456";
    assert(equal(a + b, "1260121448338"));
    a = "137109270297807493";
    b = "-52277015415377";
    assert(equal(a + b, "137056993282392116"));
    a = "585";
    b = "-345";
    assert(equal(a + b, "240"));
    a = "657721050518715";
    b = "-522974";
    assert(equal(a + b, "657721049995741"));
    a = "-33720665094975";
    b = "6922520074125";
    assert(equal(a + b, "-26798145020850"));
    a = "10043";
    b = "-17485346182925042";
    assert(equal(a + b, "-17485346182914999"));
    a = "-6414571541358";
    b = "-2175574716410990922";
    assert(equal(a + b, "-2175581130982532280"));
    a = "-17483848575516";
    b = "-33568423369574";
    assert(equal(a + b, "-51052271945090"));
    a = "-101638910856725";
    b = "-591";
    assert(equal(a + b, "-101638910857316"));
    a = "1";
    b = "911728";
    assert(equal(a + b, "911729"));
    a = "-40";
    b = "270227592212906603";
    assert(equal(a + b, "270227592212906563"));
    a = "46469808";
    b = "-254138456";
    assert(equal(a + b, "-207668648"));
    a = "-14600541234211";
    b = "59507";
    assert(equal(a + b, "-14600541174704"));
    a = "-163847";
    b = "-382999697777133138";
    assert(equal(a * b, "62753351481689933261886"));
    a = "-69";
    b = "-47094764228";
    assert(equal(a * b, "3249538731732"));
    a = "696791";
    b = "-928110804906556";
    assert(equal(a * b, "-646699255861644061796"));
    a = "2204831429837934";
    b = "47700528045";
    assert(equal(a * b, "105171623453481820571859030"));
    a = "-613";
    b = "-2";
    assert(equal(a * b, "1226"));
    a = "3664";
    b = "-531245980795755";
    assert(equal(a * b, "-1946485273635646320"));
    a = "-451109";
    b = "219188027693";
    assert(equal(a * b, "-98877691984561537"));
    a = "154641488498903";
    b = "120544";
    assert(equal(a * b, "18641103589611763232"));
    a = "1145593652843";
    b = "301990828866";
    assert(equal(a * b, "345958776765686227366038"));
    a = "-1081992569";
    b = "-39483493";
    assert(equal(a * b, "42720846024163517"));
    a = "-6803";
    b = "275308592236447082";
    assert(equal(a * b, "-1872924352984549498846"));
    a = "2387548924228571245";
    b = "512881182817218";
    assert(equal(a * b, "1224528916292326015067036125696410"));
    a = "678637036975519";
    b = "57435011509";
    assert(equal(a * b, "38977526029122592316248171"));
    a = "35150572154021";
    b = "-412923";
    assert(equal(a * b, "-14514479705554813383"));
    a = "89495";
    b = "-25";
    assert(equal(a * b, "-2237375"));
    a = "45692";
    b = "-154698730200265";
    assert(equal(a * b, "-7068494380310508380"));
    a = "206972393";
    b = "-3055330471960516";
    assert(equal(a * b, "-632369059187487398034788"));
    a = "119730794";
    b = "-7116206";
    assert(equal(a * b, "-852028994647564"));
    a = "668711705264216";
    b = "48278302151164738";
    assert(equal(a * b, "32284265758766439565030432415408"));
    a = "413029051419";
    b = "-58303";
    assert(equal(a * b, "-24080832784881957"));
    a = "84799";
    b = "182850853";
    assert(equal(a * b, "15505569483547"));
    a = "-39779";
    b = "430016193";
    assert(equal(a * b, "-17105614141347"));
    a = "257867443877";
    b = "-412136396901";
    assert(equal(a * b, "-106276559197537614225177"));
    a = "9892023161";
    b = "-211771961";
    assert(equal(a * b, "-2094853143062388721"));
    a = "95011372";
    b = "51085738453";
    assert(equal(a * b, "4853726100052687516"));
    a = "-4082061229";
    b = "-11";
    assert(equal(a * b, "44902673519"));
    a = "-3323279002";
    b = "239892508854";
    assert(equal(a * b, "-797229737411597283708"));
    a = "2942547305274622";
    b = "61972983523";
    assert(equal(a * b, "182358435665432200196053306"));
    a = "-81033";
    b = "180442147365";
    assert(equal(a * b, "-14621768527428045"));
    a = "410854";
    b = "-2202732247879818184";
    assert(equal(a * b, "-905001354970414820169136"));
    a = "-726909506186569353";
    b = "-4551553035107418407";
    assert(equal(a * b, "3308567169131914476072854279694280671"));
    a = "13017980";
    b = "-85646";
    assert(equal(a * b, "-1114937915080"));
    a = "-44135888";
    b = "-250041012";
    assert(equal(a * b, "11035782101038656"));
    a = "25806971713567";
    b = "-2265";
    assert(equal(a * b, "-58452790931229255"));
    a = "7762036583440";
    b = "38507023";
    assert(equal(a * b, "298892921245365499120"));
    a = "39";
    b = "-20282845796928108";
    assert(equal(a * b, "-791030986080196212"));
    a = "43660047072689";
    b = "-100";
    assert(equal(a * b, "-4366004707268900"));
    a = "2041575979554092147";
    b = "78671967746136";
    assert(equal(a * b, "160614799614765547184315247193992"));
    a = "-337547";
    b = "13647451108";
    assert(equal(a * b, "-4606656179152076"));
    a = "487280";
    b = "-40";
    assert(equal(a * b, "-19491200"));
    a = "26251187056981";
    b = "127011592";
    assert(equal(a * b, "3334205059996951523752"));
    a = "-56581201";
    b = "-1720961";
    assert(equal(a * b, "97374040254161"));
    a = "-27442671692";
    b = "14734884";
    assert(equal(a * b, "-404364584031703728"));
    a = "2058504129649";
    b = "-9852592912";
    assert(equal(a * b, "-20281603197102466447888"));
    a = "-13067741491028774";
    b = "93465138925";
    assert(equal(a * b, "-1221378273894991003082427950"));
    a = "-8922767022897058939";
    b = "-5911";
    assert(equal(a * b, "52742475872344515388429"));
    a = "-8060";
    b = "60220477";
    assert(equal(a * b, "-485377044620"));
    a = "132030237620628246";
    b = "65124789821833";
    assert(equal(a * b, "8598441475170082842663867294918"));
    a = "-25";
    b = "98062000977";
    assert(equal(a * b, "-2451550024425"));
    a = "498001051978";
    b = "-3627311006904604881";
    assert(equal(a * b, "-1806404697289871652230433504618"));
    a = "864939123915229991";
    b = "-6114196203932982";
    assert(equal(a * b, "-5288407508075618338552603280463162"));
    a = "-405116722360392693";
    b = "-10327466";
    assert(equal(a * b, "4183829176208395283605938"));
    a = "-7775173";
    b = "4981278709";
    assert(equal(a * b, "-38730303723691657"));
    a = "-3212";
    b = "42048";
    assert(equal(a * b, "-135058176"));
    a = "1084065";
    b = "49615446378919632";
    assert(equal(a * b, "53786368878763510864080"));
    a = "3000";
    b = "4847616";
    assert(equal(a * b, "14542848000"));
    a = "1004221196";
    b = "-10215922446678";
    assert(equal(a * b, "-10259045857646227386888"));
    a = "353035062644333531";
    b = "27611";
    assert(equal(a * b, "9747651114672693124441"));
    a = "-3902775984";
    b = "265725300";
    assert(equal(a * b, "-1037066319181195200"));
    a = "26893366347889";
    b = "-136634250954";
    assert(equal(a * b, "-3674554966575324094136106"));
    a = "59777057726363696";
    b = "-398997447";
    assert(equal(a * b, "-23850893421990739297484112"));
    a = "-33104749400280";
    b = "403";
    assert(equal(a * b, "-13341214008312840"));
    a = "-2840600";
    b = "-1899916350453224809";
    assert(equal(a * b, "5396902385097430392445400"));
    a = "2107860216180327600";
    b = "-2808615";
    assert(equal(a * b, "-5920167821067310802274000"));
}
int main ()
{
   fuzz();
    testAddition();
    testMultiplication();
    testComparison();
    testExceptions();
  CBigInt a, b;
  std::istringstream is;
  a = 10;
  assert ( equal ( a, "10" ) );
    a = "010";
    assert ( equal ( a, "10" ) );

  a += 20;
  assert ( equal ( a, "30" ) );
  a *= 5;
  assert ( equal ( a, "150" ) );
  b = a + 3;
  assert ( equal ( b, "153" ) );
  b = a * 7;
  assert ( equal ( b, "1050" ) );
  assert ( equal ( a, "150" ) );
  assert ( equalHex ( a, "96" ) );

    a = 10 + a;
    assert ( equal ( a, "160" ) );
    a = 10 * a;
    assert ( equal ( a, "1600" ) );

  a = 10;
  a += -20;
  assert ( equal ( a, "-10" ) );
  a *= 5;
  assert ( equal ( a, "-50" ) );
  b = a + 73;
  assert ( equal ( b, "23" ) );
  b = a * -7;
  assert ( equal ( b, "350" ) );
  assert ( equal ( a, "-50" ) );
  assert ( equalHex ( a, "-32" ) );
  a = "0000345";
  assert ( equal ( a, "345" ) );

  a = "12345678901234567890";
  a += "-99999999999999999999";
  assert ( equal ( a, "-87654321098765432109" ) );
  a *= "54321987654321987654";
  assert ( equal ( a, "-4761556948575111126880627366067073182286" ) );
  a *= 0;
  assert ( equal ( a, "0" ) );
  a = 10;
  b = a + "400";
  assert ( equal ( b, "410" ) );
  b = a * "15";
  assert ( equal ( b, "150" ) );
  assert ( equal ( a, "10" ) );
  assert ( equalHex ( a, "a" ) );

  is . clear ();
  is . str ( " 1234" );
  assert ( is >> b );
  assert ( equal ( b, "1234" ) );
  is . clear ();
  is . str ( " 12 34" );
  assert ( is >> b );
  assert ( equal ( b, "12" ) );
  is . clear ();
  is . str ( "999z" );
  assert ( is >> b );
  assert ( equal ( b, "999" ) );
  is . clear ();
  is . str ( "abcd" );
  assert ( ! ( is >> b ) );
  is . clear ();
  is . str ( "- 758" );
  assert ( ! ( is >> b ) );
  a = 42;
  try
  {
    a = "-xyz";
    assert ( "missing an exception" == nullptr );
  }
  catch ( const std::invalid_argument & e )
  {
    assert ( equal ( a, "42" ) );
  }

  a = "73786976294838206464";
  assert ( equal ( a, "73786976294838206464" ) );
  assert ( equalHex ( a, "40000000000000000" ) );
  assert ( a < "1361129467683753853853498429727072845824" );
  assert ( a <= "1361129467683753853853498429727072845824" );
  assert ( ! ( a > "1361129467683753853853498429727072845824" ) );
  assert ( ! ( a >= "1361129467683753853853498429727072845824" ) );
  assert ( ! ( a == "1361129467683753853853498429727072845824" ) );
  assert ( a != "1361129467683753853853498429727072845824" );
  assert ( ! ( a < "73786976294838206464" ) );
  assert ( a <= "73786976294838206464" );
  assert ( ! ( a > "73786976294838206464" ) );
  assert ( a >= "73786976294838206464" );
  assert ( a == "73786976294838206464" );
  assert ( ! ( a != "73786976294838206464" ) );
  assert ( a < "73786976294838206465" );
  assert ( a <= "73786976294838206465" );
  assert ( ! ( a > "73786976294838206465" ) );
  assert ( ! ( a >= "73786976294838206465" ) );
  assert ( ! ( a == "73786976294838206465" ) );
  assert ( a != "73786976294838206465" );
  a = "2147483648";
  assert ( ! ( a < -2147483648 ) );
  assert ( ! ( a <= -2147483648 ) );
  assert ( a > -2147483648 );
    assert ( -2147483648 < a );
  assert ( a >= -2147483648 );
  assert ( ! ( a == -2147483648 ) );
  assert ( a != -2147483648 );
  a = "-12345678";
  assert ( ! ( a < -87654321 ) );
  assert ( ! ( a <= -87654321 ) );
  assert ( a > -87654321 );
  assert ( a >= -87654321 );
  assert ( ! ( a == -87654321 ) );
  assert ( a != -87654321 );

    assert(equal(CBigInt("99999999999999999999") + CBigInt("99999999999999999999"), "199999999999999999998"));
    assert(equal(CBigInt("123456789") * CBigInt("-123456789"), "-15241578750190521"));
    assert(equal(CBigInt("0000012345"), "12345"));

    assert(equal(CBigInt("1") + INT_MAX, "2147483648")); // Assuming INT_MAX is 2147483647
    assert(equal(CBigInt("1234567890") * -1, "-1234567890"));
    assert(equal(CBigInt("987654321") + 0, "987654321"));

    assert(equal(CBigInt("999999999999999999999999999999") + "1", "1000000000000000000000000000000"));
    try {
        CBigInt invalid("abcd123");
        assert(false); // This line should not be executed
    } catch (const std::invalid_argument&) {
        assert(true); // Exception caught as expected
    }
    assert(equal(CBigInt("-00001234"), "-1234"));
    assert(equal(CBigInt("00001234"), "1234"));

    assert(equal(INT_MAX + CBigInt("1"), "2147483648"));
    assert(100 > CBigInt("99"));
    assert(-100 < CBigInt("0"));

    try {
        CBigInt result = CBigInt("123abc") + CBigInt("456");
        assert(false); // This line should not be executed
    } catch (const std::invalid_argument&) {
        assert(true); // Exception caught as expected
    }
    assert(equal("10000000000000000000" * CBigInt("2"), "20000000000000000000"));
    assert("-0000100" < CBigInt("100"));
    const CBigInt constVal("100");
    CBigInt notConst = constVal + 3;
    assert(equal(notConst, "103"));


  return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
