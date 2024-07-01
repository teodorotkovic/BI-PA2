#ifndef __PROGTEST__
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <memory>
#include <stdexcept>
#endif /* __PROGTEST__ */

class CPatchStr
{

  public:
    CPatchStr() : m_string(nullptr) {
        m_string = new char[1];
        m_string[0] = '\0';
    }

    CPatchStr(const char* str) {
        m_string = new char[std::strlen(str) + 1];
        std::strcpy(m_string, str);
    }

    CPatchStr(const CPatchStr& other) {
        m_string = new char[std::strlen(other.m_string) + 1];
        std::strcpy(m_string, other.m_string);
    }

    CPatchStr& operator=(const CPatchStr& other) {
        if (this != &other) {
            delete[] m_string;
            m_string = new char[std::strlen(other.m_string) + 1];
            std::strcpy(m_string, other.m_string);
        }
        return *this;
    }

    ~CPatchStr() {
        delete[] m_string;
    }
    CPatchStr   subStr    ( size_t            from,
                            size_t            len ) const;
    CPatchStr & append    ( const CPatchStr & src );

    CPatchStr & insert    ( size_t            pos,
                            const CPatchStr & src );
    CPatchStr & remove    ( size_t            from,
                            size_t            len );
    char      * toStr     () const;
  private:
    char * m_string;
};

char* CPatchStr::toStr() const {
    // Zkontrolovat, zda m_string není nullptr
    if (m_string == nullptr) {
        // Vrátit prázdný řetězec, pokud je interní řetězec prázdný
        char* result = new char[1];
        result[0] = '\0';
        return result;
    }

    // Vytvořit kopii interního řetězce
    size_t len = std::strlen(m_string);
    char* result = new char[len + 1];
    std::strcpy(result, m_string);
    return result;
}

CPatchStr& CPatchStr::append(const CPatchStr& src) {
    if (src.m_string) {
        // Spočítat novou délku řetězce
        size_t newLength = std::strlen(m_string) + std::strlen(src.m_string);
        // Vytvořit nový řetězec s potřebnou délkou
        char* newStr = new char[newLength + 1];
        // Zkopírovat stávající obsah a přidat nový
        std::strcpy(newStr, m_string);
        std::strcat(newStr, src.m_string);
        // Uvolnit starý řetězec a přiřadit nový
        delete[] m_string;
        m_string = newStr;
    }
    // Vrátit referenci na tento objekt
    return *this;
}

CPatchStr CPatchStr::subStr(size_t from, size_t len) const {
    size_t originalLength = std::strlen(m_string);

    // Check if the substring range is valid
    if (from + len > originalLength || from > originalLength) {
        throw std::out_of_range("Substring range is out of the string's bounds");
    }

    CPatchStr result;
    if (len == 0) {
        return result;
    }

    // Allocate memory for the internal string of the result
    char* newString = new char[len + 1];

    try {
        // Copy the substring into the internal string of the result
        std::strncpy(newString, m_string + from, len);
        newString[len] = '\0';

        // Deallocate memory for result's internal string if it was previously allocated
        delete[] result.m_string;

        // Assign the newly allocated string to result
        result.m_string = newString;
    } catch (...) {
        // Clean up allocated memory if an exception occurs
        delete[] newString;
        throw;
    }

    return result;
}

CPatchStr& CPatchStr::insert(size_t pos, const CPatchStr& src) {
    size_t originalLength = std::strlen(m_string);
    // Kontrola, zda je pozice v platném rozsahu
    if (pos > originalLength) {
        throw std::out_of_range("Position out of range");
    }

    size_t insertLength = std::strlen(src.m_string);
    // Vytvoření nového řetězce s potřebnou délkou
    char* newStr = new char[originalLength + insertLength + 1];

    // Kopírování části původního řetězce před pozici vkládání
    std::strncpy(newStr, m_string, pos);
    newStr[pos] = '\0';

    // Přidání vkládaného řetězce
    std::strcat(newStr, src.m_string);

    // Přidání zbytku původního řetězce
    std::strcat(newStr, m_string + pos);

    // Uvolnění původního řetězce a nahrazení novým
    delete[] m_string;
    m_string = newStr;

    return *this;
}

CPatchStr& CPatchStr::remove(size_t from, size_t len) {
    size_t originalLength = std::strlen(m_string);

    // Kontrola rozsahu
    if (from + len > originalLength) {
        throw std::out_of_range("Range for remove is out of the string bounds");
    }

    // Pokud je len 0, pak nic nemazat
    if (len == 0) {
        return *this;
    }

    // Vytvoření nového řetězce po odstranění
    size_t newLength = originalLength - len;
    char* newStr = new char[newLength + 1];

    // Kopírování části řetězce před from
    std::strncpy(newStr, m_string, from);
    newStr[from] = '\0';

    // Přidání části řetězce za odstraněným úsekem
    std::strcat(newStr, m_string + from + len);

    // Uvolnění starého řetězce a přiřazení nového
    delete[] m_string;
    m_string = newStr;

    return *this;
}

#ifndef __PROGTEST__
bool stringMatch ( char       * str,
                   const char * expected )
{
  bool res = std::strcmp ( str, expected ) == 0;
  delete [] str;
  return res;
}

int main ()
{
  char tmpStr[100];

  CPatchStr a ( "test" );
  assert ( stringMatch ( a . toStr (), "test" ) );
  std::strncpy ( tmpStr, " da", sizeof ( tmpStr ) - 1 );
  a . append ( tmpStr );
  assert ( stringMatch ( a . toStr (), "test da" ) );
  std::strncpy ( tmpStr, "ta", sizeof ( tmpStr ) - 1 );
  a . append ( tmpStr );
  assert ( stringMatch ( a . toStr (), "test data" ) );
  std::strncpy ( tmpStr, "foo text", sizeof ( tmpStr ) - 1 );
  CPatchStr b ( tmpStr );
  assert ( stringMatch ( b . toStr (), "foo text" ) );
  CPatchStr c ( a );
  assert ( stringMatch ( c . toStr (), "test data" ) );
  CPatchStr d ( a . subStr ( 3, 5 ) );
  assert ( stringMatch ( d . toStr (), "t dat" ) );
  d . append ( b );
  assert ( stringMatch ( d . toStr (), "t datfoo text" ) );
  d . append ( b . subStr ( 3, 4 ) );
  assert ( stringMatch ( d . toStr (), "t datfoo text tex" ) );
  c . append ( d );
  assert ( stringMatch ( c . toStr (), "test datat datfoo text tex" ) );
  c . append ( c );
  assert ( stringMatch ( c . toStr (), "test datat datfoo text textest datat datfoo text tex" ) );
  d . insert ( 2, c . subStr ( 6, 9 ) );
  assert ( stringMatch ( d . toStr (), "t atat datfdatfoo text tex" ) );
  b = "abcdefgh";
  assert ( stringMatch ( b . toStr (), "abcdefgh" ) );
  assert ( stringMatch ( d . toStr (), "t atat datfdatfoo text tex" ) );
  assert ( stringMatch ( d . subStr ( 4, 8 ) . toStr (), "at datfd" ) );
  assert ( stringMatch ( b . subStr ( 2, 6 ) . toStr (), "cdefgh" ) );
  try
  {
    b . subStr ( 2, 7 ) . toStr ();
    assert ( "Exception not thrown" == nullptr );
  }
  catch ( const std::out_of_range & e )
  {
  }
  catch ( ... )
  {
    assert ( "Invalid exception thrown" == nullptr );
  }
  a . remove ( 3, 5 );
  assert ( stringMatch ( a . toStr (), "tesa" ) );


// Assertion for subStr with out-of-range parameters in the first implementation
try {
    CPatchStr("hello").subStr(10, 10);
    assert(false); // Should not reach here
} catch (const std::out_of_range&) {
    // Expected behavior
}

// Assertion for remove with out-of-range parameters in the first implementation
try {
    CPatchStr("hello").remove(10, 10);
    assert(false); // Should not reach here
} catch (const std::out_of_range&) {
    // Expected behavior
}
  return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
