#ifndef __PROGTEST__
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <climits>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <string>
#include <array>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <functional>
#include <iterator>
#include <compare>

class CDate
{
public:
    CDate                         ( int               y,
                                    int               m,
                                    int               d )
            : m_Y ( y ),
              m_M ( m ),
              m_D ( d )
    {
    }

    std::strong_ordering     operator <=>                  ( const CDate     & other ) const = default;

    friend std::ostream    & operator <<                   ( std::ostream    & os,
                                                             const CDate     & d )
    {
        return os << d . m_Y << '-' << d . m_M << '-' << d . m_D;
    }

private:
    int                      m_Y;
    int                      m_M;
    int                      m_D;
};
enum class ESortKey
{
    NAME,
    BIRTH_DATE,
    ENROLL_YEAR
};
#endif /* __PROGTEST__ */

std::string toLowerCase(const std::string& str) {
    std::string lowerStr = str;
    for (char& c : lowerStr) {
        c = std::tolower(c);
    }
    return lowerStr;
}

std::string normalizeName(const std::string& input) {
    // Using stringstream to extract words from the input string
    std::stringstream ss(input);
    std::string word;
    std::vector<std::string> words;

    // Extracting words and adding them to the vector
    while (ss >> word) {
        words.push_back(toLowerCase(word));
    }

    // Sorting the words alphabetically
    std::sort(words.begin(), words.end());

    // Building the output string with a space after each word
    std::string output;
    for (const auto& sortedWord : words) {
        output += sortedWord + " ";
    }

    return output;
}

class CStudent
{
public:
    CStudent                      ( const std::string & name,
                                    const CDate     & born,
                                    int               enrolled )
    : m_name(name), m_normalized(normalizeName(name)), m_born(born), m_enrolled(enrolled){}

    bool operator==(const CStudent &other) const {
        return m_name == other.m_name && m_born == other.m_born && m_enrolled == other.m_enrolled;
    }

    bool operator!=(const CStudent &other) const {
        return !(*this == other);
    }

    bool operator<(const CStudent& other) const {
        if (m_enrolled != other.m_enrolled) return m_enrolled < other.m_enrolled;
        if (auto res = m_born <=> other.m_born; res != 0) return res < 0;
        return m_name < other.m_name;
    }



    std::string m_name;
    std::string m_normalized;
    CDate m_born;
    int m_enrolled;
    int m_position;
};

class CFilter
{
public:
    CFilter                       () : m_enrolledBefore(INT_MAX),
                                       m_enrolledAfter(INT_MIN),
                                       m_bornBefore(INT_MAX, 12, 31),  // Using December 31 of INT_MAX year as a max placeholder
                                       m_bornAfter(INT_MIN, 1, 1)      // Using January 1 of INT_MIN year as a min placeholder
    {
        m_checkNames = false;
    }
    CFilter                & name                          ( const std::string & name ) {
        std::string newName = normalizeName(name);
        m_names.push_back(newName);
        m_checkNames = true;
        return *this;
    }
    CFilter                & bornBefore                    ( const CDate     & date ) {
        m_bornBefore = date;
        return *this;
    }
    CFilter                & bornAfter                     ( const CDate     & date ) {
        m_bornAfter = date;
        return *this;
    }
    CFilter                & enrolledBefore                ( int               year ) {
        m_enrolledBefore = year;
        return *this;
    }
    CFilter                & enrolledAfter                 ( int               year ) {
        m_enrolledAfter = year;
        return *this;
    }
    /*bool PassesFilter(const CStudent &student) const {
        if (m_checkNames && !matchesName(student.m_normalized)) return false;
        if (student.m_born >= m_bornBefore) return false;
        if (student.m_born <= m_bornAfter) return false;
        if (student.m_enrolled >= m_enrolledBefore) return false;
        if (student.m_enrolled <= m_enrolledAfter) return false;
        return true;
    }*/

    bool m_checkNames;
    std::vector<std::string> m_names;
    int m_enrolledBefore;
    int m_enrolledAfter;
    CDate m_bornBefore;
    CDate m_bornAfter;
private:
    bool matchesName(const std::string &name) const {
        if (std::find(m_names.begin(), m_names.end(), name) != m_names.end()) {
            return true;
        }
        return false;
    }
};

class CSort
{
public:
    CSort                         () = default;
    CSort                  & addKey                        ( ESortKey          key,
                                                             bool              ascending ) {
        m_SortKeys.emplace_back(key, ascending);
        return *this;
    }

    std::vector<std::pair<ESortKey, bool>> getSortKeys() const {
        return m_SortKeys;
    }
private:
    std::vector<std::pair<ESortKey, bool>> m_SortKeys;
};

class CStudyDept
{
public:
    CStudyDept                   () {m_studentCount = 0;}
    bool                     addStudent                    ( const CStudent  & x ) {
        auto it = std::lower_bound(m_students.begin(), m_students.end(), x);

        // Check if the student already exists
        if (it != m_students.end() && !(*it < x) && !(x < *it)) {
            if(*it == x) {
                return false;
            }
        }

        CStudent newStudent = x;
        newStudent.m_position = m_studentCount++;
        // Insert the new student at the position returned by lower_bound
        m_students.insert(newStudent);
        return true;
    }
    bool                     delStudent                    ( const CStudent  & x ) {
        auto it = std::lower_bound(m_students.begin(), m_students.end(), x);

        // Check if the student already exists
        if (it != m_students.end() && !(*it < x) && !(x < *it)) {
            if(*it == x) {
                m_students.erase(it);
                return true;
            }
        }
        return false;
    }
    std::list<CStudent>      search                        ( const CFilter   & flt,
                                                             const CSort     & sortOpt ) const {
        std::list<CStudent> filteredStudents;

        // Linear scan through all students
        for (const auto& student : m_students)
        {
            // Check enrollment and birth date filters
            if (student.m_enrolled >= flt.m_enrolledBefore ||
                student.m_enrolled <= flt.m_enrolledAfter ||
                student.m_born >= flt.m_bornBefore ||
                student.m_born <= flt.m_bornAfter)
            {
                continue;  // Does not pass the filter, skip to next student
            }

            // Check name filters, if any
            if (flt.m_checkNames)
            {
                bool nameMatch = false;
                for (const auto& name : flt.m_names)
                {
                    if (student.m_normalized == name)
                    {
                        nameMatch = true;
                        break;
                    }
                }
                if (!nameMatch)
                {
                    continue;  // No name matched, skip to next student
                }
            }

            // If it passed all filters, add to result
            filteredStudents.push_back(student);
        }

        auto compare = [&sortOpt](const CStudent& a, const CStudent& b) {
            for (const auto& [key, ascending] : sortOpt.getSortKeys()) {
                if (key == ESortKey::NAME) {
                    if (a.m_name != b.m_name) {
                        return ascending ? a.m_name < b.m_name : b.m_name < a.m_name;
                    }
                } else if (key == ESortKey::BIRTH_DATE) {
                    if (auto cmp = a.m_born <=> b.m_born; cmp != 0) {
                        return ascending ? cmp < 0 : cmp > 0;
                    }
                } else if (key == ESortKey::ENROLL_YEAR) {
                    if (a.m_enrolled != b.m_enrolled) {
                        return ascending ? a.m_enrolled < b.m_enrolled : b.m_enrolled < a.m_enrolled;
                    }
                }
            }
            // Default case if no keys specified or all comparisons are equal
            return a.m_position < b.m_position;  // Preserve original insertion order as a fallback
        };

        // Apply the sorting using the lambda
        filteredStudents.sort(compare);

        return filteredStudents;
    }
    std::set<std::string>    suggest                       ( const std::string & name ) const {
        std::set<std::string> result;
        std::vector<std::string> inputWords;
        std::stringstream ss(normalizeName(name));  // Normalize and prepare for word extraction
        std::string word;

        // Extract words and add to the vector
        while (ss >> word) {
            inputWords.push_back(word + " ");
        }

        // Check each student against the input words
        for (const auto& student : m_students) {
            bool allWordsMatch = true;

            // Check if all words are in the student's normalized name
            for (const auto& inputWord : inputWords) {
                if (student.m_normalized.find(inputWord) == std::string::npos) {
                    allWordsMatch = false;
                    break;
                }
            }

            // If all words match, add the student's original name to the result
            if (allWordsMatch) {
                result.insert(student.m_name);
            }
        }

        return result;
    }
private:
    std::set<CStudent> m_students;
    int m_studentCount;
};


#ifndef __PROGTEST__
int main ( void )
{
    CStudyDept x0;
    assert ( CStudent ( "James Bond", CDate ( 1980, 4, 11), 2010 ) == CStudent ( "James Bond", CDate ( 1980, 4, 11), 2010 ) );
    assert ( ! ( CStudent ( "James Bond", CDate ( 1980, 4, 11), 2010 ) != CStudent ( "James Bond", CDate ( 1980, 4, 11), 2010 ) ) );
    assert ( CStudent ( "James Bond", CDate ( 1980, 4, 11), 2010 ) != CStudent ( "Peter Peterson", CDate ( 1980, 4, 11), 2010 ) );
    assert ( ! ( CStudent ( "James Bond", CDate ( 1980, 4, 11), 2010 ) == CStudent ( "Peter Peterson", CDate ( 1980, 4, 11), 2010 ) ) );
    assert ( CStudent ( "James Bond", CDate ( 1980, 4, 11), 2010 ) != CStudent ( "James Bond", CDate ( 1997, 6, 17), 2010 ) );
    assert ( ! ( CStudent ( "James Bond", CDate ( 1980, 4, 11), 2010 ) == CStudent ( "James Bond", CDate ( 1997, 6, 17), 2010 ) ) );
    assert ( CStudent ( "James Bond", CDate ( 1980, 4, 11), 2010 ) != CStudent ( "James Bond", CDate ( 1980, 4, 11), 2016 ) );
    assert ( ! ( CStudent ( "James Bond", CDate ( 1980, 4, 11), 2010 ) == CStudent ( "James Bond", CDate ( 1980, 4, 11), 2016 ) ) );
    assert ( CStudent ( "James Bond", CDate ( 1980, 4, 11), 2010 ) != CStudent ( "Peter Peterson", CDate ( 1980, 4, 11), 2016 ) );
    assert ( ! ( CStudent ( "James Bond", CDate ( 1980, 4, 11), 2010 ) == CStudent ( "Peter Peterson", CDate ( 1980, 4, 11), 2016 ) ) );
    assert ( CStudent ( "James Bond", CDate ( 1980, 4, 11), 2010 ) != CStudent ( "Peter Peterson", CDate ( 1997, 6, 17), 2010 ) );
    assert ( ! ( CStudent ( "James Bond", CDate ( 1980, 4, 11), 2010 ) == CStudent ( "Peter Peterson", CDate ( 1997, 6, 17), 2010 ) ) );
    assert ( CStudent ( "James Bond", CDate ( 1980, 4, 11), 2010 ) != CStudent ( "James Bond", CDate ( 1997, 6, 17), 2016 ) );
    assert ( ! ( CStudent ( "James Bond", CDate ( 1980, 4, 11), 2010 ) == CStudent ( "James Bond", CDate ( 1997, 6, 17), 2016 ) ) );
    assert ( CStudent ( "James Bond", CDate ( 1980, 4, 11), 2010 ) != CStudent ( "Peter Peterson", CDate ( 1997, 6, 17), 2016 ) );
    assert ( ! ( CStudent ( "James Bond", CDate ( 1980, 4, 11), 2010 ) == CStudent ( "Peter Peterson", CDate ( 1997, 6, 17), 2016 ) ) );
    assert ( x0 . addStudent ( CStudent ( "John Peter Taylor", CDate ( 1983, 7, 13), 2014 ) ) );
    assert ( x0 . addStudent ( CStudent ( "John Taylor", CDate ( 1981, 6, 30), 2012 ) ) );
    assert ( x0 . addStudent ( CStudent ( "Peter Taylor", CDate ( 1982, 2, 23), 2011 ) ) );
    assert ( x0 . addStudent ( CStudent ( "Peter John Taylor", CDate ( 1984, 1, 17), 2017 ) ) );
    assert ( x0 . addStudent ( CStudent ( "James Bond", CDate ( 1981, 7, 16), 2013 ) ) );
    assert ( x0 . addStudent ( CStudent ( "James Bond", CDate ( 1982, 7, 16), 2013 ) ) );
    assert ( x0 . addStudent ( CStudent ( "James Bond", CDate ( 1981, 8, 16), 2013 ) ) );
    assert ( x0 . addStudent ( CStudent ( "James Bond", CDate ( 1981, 7, 17), 2013 ) ) );
    assert ( x0 . addStudent ( CStudent ( "James Bond", CDate ( 1981, 7, 16), 2012 ) ) );
    assert ( x0 . addStudent ( CStudent ( "Bond James", CDate ( 1981, 7, 16), 2013 ) ) );
    assert ( x0 . search ( CFilter (), CSort () ) == (std::list<CStudent>
            {
                    CStudent ( "John Peter Taylor", CDate ( 1983, 7, 13), 2014 ),
                    CStudent ( "John Taylor", CDate ( 1981, 6, 30), 2012 ),
                    CStudent ( "Peter Taylor", CDate ( 1982, 2, 23), 2011 ),
                    CStudent ( "Peter John Taylor", CDate ( 1984, 1, 17), 2017 ),
                    CStudent ( "James Bond", CDate ( 1981, 7, 16), 2013 ),
                    CStudent ( "James Bond", CDate ( 1982, 7, 16), 2013 ),
                    CStudent ( "James Bond", CDate ( 1981, 8, 16), 2013 ),
                    CStudent ( "James Bond", CDate ( 1981, 7, 17), 2013 ),
                    CStudent ( "James Bond", CDate ( 1981, 7, 16), 2012 ),
                    CStudent ( "Bond James", CDate ( 1981, 7, 16), 2013 )
            }) );
    assert ( x0 . search ( CFilter (), CSort () . addKey ( ESortKey::NAME, true ) ) == (std::list<CStudent>
            {
                    CStudent ( "Bond James", CDate ( 1981, 7, 16), 2013 ),
                    CStudent ( "James Bond", CDate ( 1981, 7, 16), 2013 ),
                    CStudent ( "James Bond", CDate ( 1982, 7, 16), 2013 ),
                    CStudent ( "James Bond", CDate ( 1981, 8, 16), 2013 ),
                    CStudent ( "James Bond", CDate ( 1981, 7, 17), 2013 ),
                    CStudent ( "James Bond", CDate ( 1981, 7, 16), 2012 ),
                    CStudent ( "John Peter Taylor", CDate ( 1983, 7, 13), 2014 ),
                    CStudent ( "John Taylor", CDate ( 1981, 6, 30), 2012 ),
                    CStudent ( "Peter John Taylor", CDate ( 1984, 1, 17), 2017 ),
                    CStudent ( "Peter Taylor", CDate ( 1982, 2, 23), 2011 )
            }) );
    assert ( x0 . search ( CFilter (), CSort () . addKey ( ESortKey::NAME, false ) ) == (std::list<CStudent>
            {
                    CStudent ( "Peter Taylor", CDate ( 1982, 2, 23), 2011 ),
                    CStudent ( "Peter John Taylor", CDate ( 1984, 1, 17), 2017 ),
                    CStudent ( "John Taylor", CDate ( 1981, 6, 30), 2012 ),
                    CStudent ( "John Peter Taylor", CDate ( 1983, 7, 13), 2014 ),
                    CStudent ( "James Bond", CDate ( 1981, 7, 16), 2013 ),
                    CStudent ( "James Bond", CDate ( 1982, 7, 16), 2013 ),
                    CStudent ( "James Bond", CDate ( 1981, 8, 16), 2013 ),
                    CStudent ( "James Bond", CDate ( 1981, 7, 17), 2013 ),
                    CStudent ( "James Bond", CDate ( 1981, 7, 16), 2012 ),
                    CStudent ( "Bond James", CDate ( 1981, 7, 16), 2013 )
            }) );
    assert ( x0 . search ( CFilter (), CSort () . addKey ( ESortKey::ENROLL_YEAR, false ) . addKey ( ESortKey::BIRTH_DATE, false ) . addKey ( ESortKey::NAME, true ) ) == (std::list<CStudent>
            {
                    CStudent ( "Peter John Taylor", CDate ( 1984, 1, 17), 2017 ),
                    CStudent ( "John Peter Taylor", CDate ( 1983, 7, 13), 2014 ),
                    CStudent ( "James Bond", CDate ( 1982, 7, 16), 2013 ),
                    CStudent ( "James Bond", CDate ( 1981, 8, 16), 2013 ),
                    CStudent ( "James Bond", CDate ( 1981, 7, 17), 2013 ),
                    CStudent ( "Bond James", CDate ( 1981, 7, 16), 2013 ),
                    CStudent ( "James Bond", CDate ( 1981, 7, 16), 2013 ),
                    CStudent ( "James Bond", CDate ( 1981, 7, 16), 2012 ),
                    CStudent ( "John Taylor", CDate ( 1981, 6, 30), 2012 ),
                    CStudent ( "Peter Taylor", CDate ( 1982, 2, 23), 2011 )
            }) );
    assert ( x0 . search ( CFilter () . name ( "james bond" ), CSort () . addKey ( ESortKey::ENROLL_YEAR, false ) . addKey ( ESortKey::BIRTH_DATE, false ) . addKey ( ESortKey::NAME, true ) ) == (std::list<CStudent>
            {
                    CStudent ( "James Bond", CDate ( 1982, 7, 16), 2013 ),
                    CStudent ( "James Bond", CDate ( 1981, 8, 16), 2013 ),
                    CStudent ( "James Bond", CDate ( 1981, 7, 17), 2013 ),
                    CStudent ( "Bond James", CDate ( 1981, 7, 16), 2013 ),
                    CStudent ( "James Bond", CDate ( 1981, 7, 16), 2013 ),
                    CStudent ( "James Bond", CDate ( 1981, 7, 16), 2012 )
            }) );
    assert ( x0 . search ( CFilter () . bornAfter ( CDate ( 1980, 4, 11) ) . bornBefore ( CDate ( 1983, 7, 13) ) . name ( "John Taylor" ) . name ( "james BOND" ), CSort () . addKey ( ESortKey::ENROLL_YEAR, false ) . addKey ( ESortKey::BIRTH_DATE, false ) . addKey ( ESortKey::NAME, true ) ) == (std::list<CStudent>
            {
                    CStudent ( "James Bond", CDate ( 1982, 7, 16), 2013 ),
                    CStudent ( "James Bond", CDate ( 1981, 8, 16), 2013 ),
                    CStudent ( "James Bond", CDate ( 1981, 7, 17), 2013 ),
                    CStudent ( "Bond James", CDate ( 1981, 7, 16), 2013 ),
                    CStudent ( "James Bond", CDate ( 1981, 7, 16), 2013 ),
                    CStudent ( "James Bond", CDate ( 1981, 7, 16), 2012 ),
                    CStudent ( "John Taylor", CDate ( 1981, 6, 30), 2012 )
            }) );
    assert ( x0 . search ( CFilter () . name ( "james" ), CSort () . addKey ( ESortKey::NAME, true ) ) == (std::list<CStudent>
            {
            }) );
    assert ( x0 . suggest ( "peter" ) == (std::set<std::string>
            {
                    "John Peter Taylor",
                    "Peter John Taylor",
                    "Peter Taylor"
            }) );
    assert ( x0 . suggest ( "bond" ) == (std::set<std::string>
            {
                    "Bond James",
                    "James Bond"
            }) );
    assert ( x0 . suggest ( "peter joHn" ) == (std::set<std::string>
            {
                    "John Peter Taylor",
                    "Peter John Taylor"
            }) );
    assert ( x0 . suggest ( "peter joHn bond" ) == (std::set<std::string>
            {
            }) );
    assert ( x0 . suggest ( "pete" ) == (std::set<std::string>
            {
            }) );
    assert ( x0 . suggest ( "peter joHn PETER" ) == (std::set<std::string>
            {
                    "John Peter Taylor",
                    "Peter John Taylor"
            }) );
    assert ( ! x0 . addStudent ( CStudent ( "James Bond", CDate ( 1981, 7, 16), 2013 ) ) );
    assert ( x0 . delStudent ( CStudent ( "James Bond", CDate ( 1981, 7, 16), 2013 ) ) );
    assert ( x0 . search ( CFilter () . bornAfter ( CDate ( 1980, 4, 11) ) . bornBefore ( CDate ( 1983, 7, 13) ) . name ( "John Taylor" ) . name ( "james BOND" ), CSort () . addKey ( ESortKey::ENROLL_YEAR, false ) . addKey ( ESortKey::BIRTH_DATE, false ) . addKey ( ESortKey::NAME, true ) ) == (std::list<CStudent>
            {
                    CStudent ( "James Bond", CDate ( 1982, 7, 16), 2013 ),
                    CStudent ( "James Bond", CDate ( 1981, 8, 16), 2013 ),
                    CStudent ( "James Bond", CDate ( 1981, 7, 17), 2013 ),
                    CStudent ( "Bond James", CDate ( 1981, 7, 16), 2013 ),
                    CStudent ( "James Bond", CDate ( 1981, 7, 16), 2012 ),
                    CStudent ( "John Taylor", CDate ( 1981, 6, 30), 2012 )
            }) );
    assert ( ! x0 . delStudent ( CStudent ( "James Bond", CDate ( 1981, 7, 16), 2013 ) ) );
    return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
