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
        return remSpaces(m_name) == remSpaces(other.m_name) && m_born == other.m_born && m_enrolled == other.m_enrolled;
    }

    bool operator!=(const CStudent &other) const {
        return !(*this == other);
    }

    bool operator<(const CStudent& other) const {
        if (m_name != other.m_name) return m_name < other.m_name;
        if (auto res = m_born <=> other.m_born; res != 0) return res < 0;
        return m_enrolled < other.m_enrolled;
    }



    std::string m_name;
    std::string m_normalized;
    CDate m_born;
    int m_enrolled;

  private:
    static std::string remSpaces(const std::string& name) {
        std::istringstream iss(name);
        std::vector<std::string> words;
        std::string token, normalized;

        while (iss >> token) {
            std::transform(token.begin(), token.end(), token.begin(), ::tolower);
            words.push_back(token);
        }
        std::sort(words.begin(), words.end());

        for (const auto& word : words) {
            if (!normalized.empty()) normalized += " ";
            normalized += word;
        }

        return normalized;
    }
};

class CFilter 
{
 public:
    CFilter() = default;
    CFilter(const CFilter& other) = delete; // Disallow copy to simplify memory management.
    CFilter& operator=(const CFilter& other) = delete; // Disallow assignment for the same reason.

    CFilter& name(const std::string &name) {
        std::string normalized = normalizeName(name);
        m_names.insert(normalized);
        return *this;
    }

    CFilter& bornBefore(const CDate &date) {
        m_bornBefore = std::make_unique<CDate>(date);
        return *this;
    }

    CFilter& bornAfter(const CDate &date) {
        m_bornAfter = std::make_unique<CDate>(date);
        return *this;
    }

    CFilter& enrolledBefore(int year) {
        m_enrolledBefore = year;
        m_enrolledBeforeSet = true;
        return *this;
    }

    CFilter& enrolledAfter(int year) {
        m_enrolledAfter = year;
        m_enrolledAfterSet = true;
        return *this;
    }

    bool PassesFilter(const CStudent &student) const {
        if (!m_names.empty() && !matchesName(student.m_normalized)) return false;
        if (m_bornBefore && !(student.m_born < *m_bornBefore)) return false;
        if (m_bornAfter && !(student.m_born > *m_bornAfter)) return false;
        if (m_enrolledBeforeSet && !(student.m_enrolled < m_enrolledBefore)) return false;
        if (m_enrolledAfterSet && !(student.m_enrolled > m_enrolledAfter)) return false;
        return true;
    }

private:
    std::set<std::string> m_names;
    std::unique_ptr<CDate> m_bornBefore, m_bornAfter;
    int m_enrolledBefore{}, m_enrolledAfter{};
    bool m_bornBeforeSet = false, m_bornAfterSet = false, m_enrolledBeforeSet = false, m_enrolledAfterSet = false;

    /*static std::string normalizeName(const std::string &name) {
        std::string result = name;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        std::sort(result.begin(), result.end());
        return result;
    }*/

    bool matchesName(const std::string &name) const {
        return m_names.find(name) != m_names.end();
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
                             CStudyDept                    () = default;
    bool                     addStudent                    (const CStudent& x) {
        if (m_studentsIndex.find(x) != m_studentsIndex.end()) {
            return false;
        }
        m_studentsOrder.push_back(x);
        m_studentsIndex[x] = m_studentsOrder.size() - 1;
        return true;
    }
    bool                     delStudent                    ( const CStudent  & x ) {
        auto it = m_studentsIndex.find(x);
        if (it != m_studentsIndex.end()) {
            // Get index of student to remove.
            size_t index = it->second;

            // Remove the student from the order vector.
            // To maintain order, we have to move all elements after the removed student.
            m_studentsOrder.erase(m_studentsOrder.begin() + index);

            // Update the indices in the map since the vector has been altered.
            // Every student that came after the removed one will have its index decremented by one.
            for (auto &pair : m_studentsIndex) {
                if (pair.second > index) {
                    pair.second--;
                }
            }

            // Remove the student from the index map.
            m_studentsIndex.erase(it);
            return true;
        }
        return false;
    }
    std::list<CStudent>      search                        ( const CFilter   & flt,
                                                             const CSort     & sortOpt ) const {
        // Filter students
        std::vector<CStudent> filtered;
        for (const auto &student : m_studentsOrder) {
            if (flt.PassesFilter(student)) {
                filtered.push_back(student);
            }
        }

        // Sort students
        auto comp = [&sortOpt](const CStudent &a, const CStudent &b) {
            for (const auto &[key, ascending] : sortOpt.getSortKeys()) {
                if (key == ESortKey::NAME) {
                    int cmp = a.m_name.compare(b.m_name);
                    if (cmp != 0) return ascending ? cmp < 0 : cmp > 0;
                } else if (key == ESortKey::BIRTH_DATE) {
                    auto cmp = a.m_born <=> b.m_born;
                    if (cmp != 0) return ascending ? cmp < 0 : cmp > 0;
                } else if (key == ESortKey::ENROLL_YEAR) {
                    if (a.m_enrolled != b.m_enrolled) return ascending ? a.m_enrolled < b.m_enrolled : a.m_enrolled > b.m_enrolled;
                }
            }
            return false;  // In case all sort keys are equal, maintain original order.
        };
        std::sort(filtered.begin(), filtered.end(), comp);

        return std::list<CStudent>(filtered.begin(), filtered.end());
    }
    std::set<std::string>    suggest                       ( const std::string & name ) const {
        std::set<std::string> result;
        std::set<std::string> inputWords = splitWords(normalizeName(name));

        for (const auto& student : m_studentsOrder) {
            std::set<std::string> nameWords = splitWords(normalizeName(student.m_name));
            if (std::includes(nameWords.begin(), nameWords.end(), inputWords.begin(), inputWords.end())) {
                result.insert(student.m_name);
            }
        }
        return result;
    }
  private:
    std::vector<CStudent> m_studentsOrder;                 // Stores students in insertion order
    std::map<CStudent, size_t> m_studentsIndex;

    static std::string normalizeName(const std::string& str) {
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return lower;
    }

    static std::set<std::string> splitWords(const std::string& str) {
        std::set<std::string> words;
        std::istringstream iss(str);
        std::string word;
        while (iss >> word) {
            words.insert(word);
        }
        return words;
    }
};



#ifndef __PROGTEST__
#include <chrono>
void fizzBuzz() {
    CStudyDept dept;
    CDate edgeBirthBefore(2000, 1, 1);
    CDate edgeBirthAfter(1990, 1, 1);
    CFilter filterEdge;
    filterEdge.bornBefore(edgeBirthBefore).bornAfter(edgeBirthAfter);

// Přidání studentů na hranici filtru
    assert(dept.addStudent(CStudent("Edge Case", CDate(2000, 1, 1), 2018)) == true); // Tento student by neměl projít filtrem bornBefore
    assert(dept.addStudent(CStudent("Edge Case", CDate(1990, 1, 1), 2018)) == true); // Tento student by neměl projít filtrem bornAfter

    auto resultsEdge = dept.search(filterEdge, CSort());
    assert(std::find_if(resultsEdge.begin(), resultsEdge.end(), [](const CStudent& s) {
        return s.m_born == CDate(2000, 1, 1) || s.m_born == CDate(1990, 1, 1);
    }) == resultsEdge.end()); // Žádný z těchto studentů by neměl být ve výsledcích

    assert(dept.addStudent(CStudent("Future Student", CDate(1999, 12, 31), 2025)) == true); // Student s nástupem v budoucnosti
    assert(dept.addStudent(CStudent("Ancient Student", CDate(1999, 12, 31), 1900)) == true); // Velmi starý ročník nástupu

    CFilter filterEnroll;
    filterEnroll.enrolledAfter(2000).enrolledBefore(2020);
    auto resultsEnroll = dept.search(filterEnroll, CSort());
    assert(std::find_if(resultsEnroll.begin(), resultsEnroll.end(), [](const CStudent& s) {
        return s.m_enrolled == 2025 || s.m_enrolled == 1900;
    }) == resultsEnroll.end()); // Tyto extrémní roky by neměly projít filtrem

    CStudent first("John Doe", CDate(1995, 5, 15), 2015);
    CStudent second("Jane Doe", CDate(1996, 6, 16), 2016);
    dept.addStudent(first);
    dept.addStudent(second);

    /*CSort sortStable;
    auto resultsStable = dept.search(CFilter(), sortStable);
    assert(resultsStable.front() == first && resultsStable.back() == second); // Kontrola, zda je pořadí zachováno*/

    dept.addStudent(CStudent("Jan Jakub Ryba", CDate(1995, 1, 1), 2014));
    dept.addStudent(CStudent("Jakub Jan Ryba", CDate(1995, 1, 2), 2014));

    assert(dept.suggest("ryba").size() == 2);  // Očekáváme dvě jména
    assert(dept.suggest("jan").size() == 2);   // Očekáváme dvě jména, i přes různé pořadí slov
    assert(dept.suggest("Jakub Ryba").size() == 2); // Očekáváme obě jména, i přes různé pořadí slov

    assert(dept.suggest("Jan Jan").size() == 0);  // Duplicitní slovo, žádné jméno by nemělo odpovídat
}
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

  //fizzBuzz();

  return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
