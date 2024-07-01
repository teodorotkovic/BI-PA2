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


struct StringEntry {
    std::shared_ptr<char> str;
    size_t length;

    // Constructor
    StringEntry(const char* s) : str(new char[std::strlen(s) + 1], std::default_delete<char[]>()), length(std::strlen(s)) {
        std::strcpy(str.get(), s);
    }
};



class CPatchStr
{
  public:
    CPatchStr() : m_segments(nullptr), m_segmentCount(0) {}

    CPatchStr(const char * str) {
        m_segmentCount = 1;
        m_segments = new StringList[m_segmentCount];
        m_segments[0].ptr = std::make_shared<StringEntry>(str);
        m_segments[0].ofs = 0;
        m_segments[0].len = static_cast<int>(strlen(str));
    }

    CPatchStr(const CPatchStr& other) : m_segments(new StringList[other.m_segmentCount]), m_segmentCount(other.m_segmentCount) {
        for (size_t i = 0; i < m_segmentCount; ++i) {
            m_segments[i].ptr = other.m_segments[i].ptr;
            m_segments[i].ofs = other.m_segments[i].ofs;
            m_segments[i].len = other.m_segments[i].len;
        }
    }

    // Destructor
    ~CPatchStr() {
        delete[] m_segments;
    }

    // Copy assignment operator
    CPatchStr& operator=(const CPatchStr& other) {
        if (this != &other) {
            delete[] m_segments;

            m_segmentCount = other.m_segmentCount;
            m_segments = new StringList[other.m_segmentCount];
            for (size_t i = 0; i < m_segmentCount; ++i) {
                m_segments[i].ptr = other.m_segments[i].ptr;
                m_segments[i].ofs = other.m_segments[i].ofs;
                m_segments[i].len = other.m_segments[i].len;
            }
        }
        return *this;
    }

    CPatchStr   subStr    ( size_t            from,
                            size_t            len ) const;
    CPatchStr & append    ( const CPatchStr & src );

    CPatchStr & insert    ( size_t            pos,
                            const CPatchStr & src );
    CPatchStr & remove    ( size_t            from,
                            size_t            len );
    char      * toStr     () const;
  public:
    struct StringList {
        int ofs;
        int len;
        std::shared_ptr<StringEntry> ptr;
    };
    StringList * m_segments;
    size_t m_segmentCount;
  private:
    size_t totalLength() const {
        size_t total = 0;
        for (size_t i = 0; i < m_segmentCount; ++i) {
            total += m_segments[i].len;
        }
        return total;
    }
};


char * CPatchStr::toStr() const {
    std::ostringstream oss;
    for (size_t i = 0; i < m_segmentCount; ++i) {
        StringList &seg = m_segments[i];
        if (seg.ptr) {  // Ensure the pointer is valid
            // Append the substring to the output stream
            oss.write(seg.ptr->str.get() + seg.ofs, seg.len);
        }
    }

    // Convert stream to string and then to C-style string
    std::string temp = oss.str();
    char* result = new char[temp.length() + 1];
    std::strcpy(result, temp.c_str());
    return result;
}

CPatchStr & CPatchStr::append(const CPatchStr & src) {
    if (this == &src) {  // Handle self-assignment
        CPatchStr temp(src);  // Make a copy to work with
        return append(temp);  // Append the copy
    }

    size_t newSegmentCount = m_segmentCount + src.m_segmentCount;
    StringList* newSegments = new StringList[newSegmentCount];

    // Copy existing segments to the new array
    for (size_t i = 0; i < m_segmentCount; ++i) {
        newSegments[i] = m_segments[i];
    }

    // Append new segments from src
    for (size_t i = 0; i < src.m_segmentCount; ++i) {
        newSegments[m_segmentCount + i] = src.m_segments[i];
    }

    delete[] m_segments;  // Delete old segment array
    m_segments = newSegments;  // Assign the new segment array
    m_segmentCount = newSegmentCount;

    return *this;
}

CPatchStr CPatchStr::subStr(size_t from, size_t len) const {
    if (from + len > totalLength()) {
        throw std::out_of_range("Starting position out of range");
    }

    CPatchStr result;
    size_t endPos = from + len;
    size_t currentPos = 0;

    // Find and allocate only necessary segments for the substring
    for (size_t i = 0; i < m_segmentCount && currentPos < endPos; ++i) {
        size_t segmentFullStart = currentPos;
        size_t segmentFullEnd = currentPos + m_segments[i].len;
        currentPos = segmentFullEnd;  // Prepare currentPos for the next loop iteration

        // Check if the segment falls within the substring range
        if (segmentFullEnd > from && segmentFullStart < endPos) {
            size_t segmentStart = (from > segmentFullStart) ? from - segmentFullStart : 0;
            size_t segmentEnd = (endPos < segmentFullEnd) ? endPos - segmentFullStart : m_segments[i].len;

            if (result.m_segmentCount == 0) {
                // Allocate with an estimate of necessary segment count to avoid frequent reallocations
                result.m_segments = new StringList[m_segmentCount];
            }

            result.m_segments[result.m_segmentCount].ptr = m_segments[i].ptr;
            result.m_segments[result.m_segmentCount].ofs = m_segments[i].ofs + segmentStart;
            result.m_segments[result.m_segmentCount].len = segmentEnd - segmentStart;
            result.m_segmentCount++;
        }
    }

    return result;
}

CPatchStr & CPatchStr::insert(size_t pos, const CPatchStr & src) {
    if (pos > totalLength()) {
        throw std::out_of_range("Position out of range");
    }

    // Calculate new total segment count considering possible split of existing segments
    size_t newSegmentCount = m_segmentCount + src.m_segmentCount;
    for (size_t i = 0, len = 0; i < m_segmentCount; ++i) {
        len += m_segments[i].len;
        if (pos < len) {
            // If we're inserting in the middle of a segment, it will be split into two
            newSegmentCount++;
            break;
        }
    }

    StringList* newSegments = new StringList[newSegmentCount];

    size_t newIndex = 0, originalIndex = 0, currentLength = 0;
    bool inserted = false;

    while (originalIndex < m_segmentCount) {
        if (!inserted && currentLength + m_segments[originalIndex].len > pos) {
            // Handle the case where we need to split the segment for insertion
            if (pos > currentLength) {
                // Insert part of the original segment before the insertion point
                newSegments[newIndex++] = {m_segments[originalIndex].ofs, static_cast<int>(pos - currentLength), m_segments[originalIndex].ptr};
            }

            // Insert the entire src string
            for (size_t i = 0; i < src.m_segmentCount; i++) {
                newSegments[newIndex++] = src.m_segments[i];
            }

            // Adjust and insert the remaining part of the split original segment, if necessary
            if (currentLength + m_segments[originalIndex].len > pos) {
                int remainingLength = currentLength + m_segments[originalIndex].len - pos;
                newSegments[newIndex++] = {static_cast<int>(m_segments[originalIndex].ofs) + static_cast<int>(pos) - static_cast<int>(currentLength), remainingLength, m_segments[originalIndex].ptr};
            }

            inserted = true;
        } else {
            // Insert the original segment as is
            newSegments[newIndex++] = m_segments[originalIndex];
        }

        currentLength += m_segments[originalIndex].len;
        originalIndex++;
    }

    // In case the insertion point is at the end of the string
    if (!inserted) {
        for (size_t i = 0; i < src.m_segmentCount; i++) {
            newSegments[newIndex++] = src.m_segments[i];
        }
    }

    delete[] m_segments;
    m_segments = newSegments;
    m_segmentCount = newIndex;

    return *this;
}

CPatchStr & CPatchStr::remove(size_t from, size_t len) {
    if (from > totalLength()) {
        throw std::out_of_range("Starting position out of range");
    }
    if (from + len > totalLength()) {
        len = totalLength() - from;  // Adjust len to avoid going out of bounds
    }

    size_t newSegmentCount = 0;
    for (size_t i = 0, currentPos = 0; i < m_segmentCount; ++i) {
        size_t segStart = currentPos;
        size_t segEnd = currentPos + m_segments[i].len;
        currentPos = segEnd;

        if (segEnd <= from || segStart >= from + len) {
            newSegmentCount++;
        } else if (segStart < from && segEnd > from + len) {
            newSegmentCount += 2;
        } else {
            newSegmentCount++;
        }
    }

    StringList * newSegments = new StringList[newSegmentCount];
    size_t newIndex = 0;
    for (size_t i = 0, currentPos = 0; i < m_segmentCount; ++i) {
        size_t segStart = currentPos;
        size_t segEnd = currentPos + m_segments[i].len;
        currentPos = segEnd;

        if (segEnd <= from) {
            newSegments[newIndex++] = m_segments[i];
        } else if (segStart >= from + len) {
            newSegments[newIndex++] = {static_cast<int>(segStart - len), m_segments[i].len, m_segments[i].ptr};
        } else {
            if (segStart < from) {
                int newLen = static_cast<int>(from - segStart);
                newSegments[newIndex++] = {m_segments[i].ofs, newLen, m_segments[i].ptr};
            }
            if (segEnd > from + len) {
                int newOfs = m_segments[i].ofs + (from + len - segStart);
                int newLen = static_cast<int>(segEnd - (from + len));
                newSegments[newIndex++] = {newOfs, newLen, m_segments[i].ptr};
            }
        }
    }

    delete[] m_segments;
    m_segments = newSegments;
    m_segmentCount = newSegmentCount;

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

void edgeCases() {
    CPatchStr emptyStr;
    CPatchStr testStr("hello");

    // Appending to an empty string
    assert(stringMatch(emptyStr.append("world").toStr(), "world"));

    // Inserting into an empty string (should be like append)
    assert(stringMatch(emptyStr.insert(0, "hello").toStr(), "helloworld"));

    // Substring of length zero
    CPatchStr zeroSub = testStr.subStr(1, 0);
    assert(stringMatch(zeroSub.toStr(), ""));

    // Substring of full length
    CPatchStr fullSub = testStr.subStr(0, strlen(testStr.toStr()));
    assert(stringMatch(fullSub.toStr(), "hello"));

    // Out-of-range access
    try {
        testStr.subStr(0, 100);
        assert(false); // Should not reach here
    } catch (const std::out_of_range&) {
        // Expected behavior
    }

    // Inserting at the beginning and end
    testStr.insert(0, "start ");
    testStr.append(" end");
    assert(stringMatch(testStr.toStr(), "start hello end"));

    // Removing everything
    testStr.remove(0, strlen(testStr.toStr()));
    assert(stringMatch(testStr.toStr(), ""));

    // Self-append
    CPatchStr selfStr("abc");
    selfStr.append(selfStr);
    assert(stringMatch(selfStr.toStr(), "abcabc"));

    // Self-insert
    selfStr.insert(3, selfStr);
    assert(stringMatch(selfStr.toStr(), "abcabcabcabc"));

    CPatchStr largeStr;
    std::string largeTestStr(1e6, 'x'); // 1 million 'x' characters
    largeStr.append(largeTestStr.c_str());
    assert(stringMatch(largeStr.toStr(), largeTestStr.c_str()));

    // Accessing after deletion
    CPatchStr* dynamicStr = new CPatchStr("temporary");
    delete dynamicStr;
    // Access after delete is undefined behavior, so don't actually do: assert(dynamicStr->toStr()); // This is unsafe!

    // Rapid creation and destruction
    for (int i = 0; i < 100000; i++) {
        CPatchStr tempStr("temp");
        for (int j = 0; j < 10; j++) {
            tempStr.append("temp");
        }
        // No explicit check, just ensuring no memory or resource leaks
    }

    // Recursive operations - assuming there are any recursive method calls in CPatchStr (illustrative purposes)
    // CPatchStr recursiveStr;
    // auto recursiveAppend = [](CPatchStr& str, int depth) {
    //     if (depth > 0) {
    //         str.append("recursion ");
    //         recursiveAppend(str, depth - 1);
    //     }
    // };
    // recursiveAppend(recursiveStr, 10000); // Very deep recursion, could exceed stack limit if implemented recursively
}

void deepCopyTest(int depth) {
    if (depth <= 0) return;

    CPatchStr baseStr("base");
    CPatchStr copyStr = baseStr;  // If the copy constructor is incorrectly implementing deep copy, this can cause stack overflow.

    deepCopyTest(depth - 1);  // Recursive call without proper base condition or too deep recursion.
}

void bimBimBamBam() {
    {
        CPatchStr str1("hello");
        {
            CPatchStr str2 = str1;
            // str2 goes out of scope and its destructor is called.
        }
        // Accessing str1 after str2 has been destructed, if refCount and memory management are incorrect, might access freed memory.
        assert(strcmp(str1.toStr(), "hello") == 0);  // Potential use-after-free if refCount handling is incorrect.
    }
    for (int i = 0; i < 1000000; i++) {
        CPatchStr str("test");
        str.append("data");  // If append does not manage memory correctly, it could leak.
        // No deletion of internal data, assuming destructor should handle, but if it's incorrect, memory leaks.
    }

    deepCopyTest(10000);  // Adjust the depth based on your system's stack size to test the limit.

    CPatchStr str1("initial");
    CPatchStr str2 = str1.subStr(0, 3);  // Assuming subStr might alter internal state improperly.
    str1.remove(0, 3);  // If remove doesn't handle internal pointers correctly, it might corrupt str2's data.
    assert(strcmp(str2.toStr(), "ini") == 0);  // Potential access to corrupted memory.


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

  edgeCases();
  bimBimBamBam();


  return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
