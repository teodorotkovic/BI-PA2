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
    char* str;
    size_t length;
    int refCount;  // Add reference count

    // Constructor
    StringEntry(const char* s) : str(new char[std::strlen(s) + 1]), length(std::strlen(s)), refCount(1) {
        std::strcpy(str, s);
    }

    // Increment the reference count
    void addRef() {
        refCount++;
    }

    // Destructor
    /*~StringEntry() {
        delete[] str;
    }

// Release method
    void release() {
        if (--refCount == 0) {
            delete this;
        }
    }*/
};



class CPatchStr
{
  public:
    CPatchStr() : m_segments(nullptr), m_segmentCount(0) {}

    CPatchStr(const char * str) {
        m_segmentCount = 1;
        m_segments = new StringList[m_segmentCount];
        m_segments[0].ptr = new StringEntry(str);
        m_segments[0].ofs = 0;
        m_segments[0].len = static_cast<int>(strlen(str));
    }

    CPatchStr(const CPatchStr& other) : m_segments(new StringList[other.m_segmentCount]), m_segmentCount(other.m_segmentCount) {
        for (size_t i = 0; i < m_segmentCount; ++i) {
            m_segments[i].ptr = other.m_segments[i].ptr;
            m_segments[i].ptr->addRef();  // Increment reference count
            m_segments[i].ofs = other.m_segments[i].ofs;
            m_segments[i].len = other.m_segments[i].len;
        }
    }

    // Destructor
    ~CPatchStr() {
        for (size_t i = 0; i < m_segmentCount; ++i) {
            /*m_segments[i].ptr->release();  // Decrement reference count and delete if it's 0*/
        }
        delete[] m_segments;
    }

    // Copy assignment operator
    CPatchStr& operator=(const CPatchStr& other) {
        if (this != &other) {
            for (size_t i = 0; i < m_segmentCount; ++i) {
                if (m_segments[i].ptr) {
                    /*m_segments[i].ptr->release();*/
                }
            }
            delete[] m_segments;

            m_segmentCount = other.m_segmentCount;
            m_segments = new StringList[other.m_segmentCount];
            for (size_t i = 0; i < m_segmentCount; ++i) {
                m_segments[i].ptr = other.m_segments[i].ptr;
                m_segments[i].ptr->addRef();  // Increment reference count
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
        StringEntry * ptr;
    };
    StringList * m_segments;
    size_t m_segmentCount;
  private:

};
char * CPatchStr::toStr() const {
    // Calculate total length needed
    size_t totalLength = 0;
    for (size_t i = 0; i < m_segmentCount; ++i) {
        totalLength += m_segments[i].len;
    }

    // Allocate memory for the resulting string plus null terminator
    char * result = new char[totalLength + 1];
    size_t currentPos = 0;

    // Concatenate each segment into the result
    for (size_t i = 0; i < m_segmentCount; ++i) {
        std::memcpy(result + currentPos, m_segments[i].ptr->str + m_segments[i].ofs, m_segments[i].len);
        currentPos += m_segments[i].len;
    }

    // Null-terminate the string
    result[totalLength] = '\0';
    return result;
}

CPatchStr & CPatchStr::append(const CPatchStr & src) {
    // Reallocate the m_segments array to accommodate the new segments
    StringList * newSegments = new StringList[m_segmentCount + src.m_segmentCount];

    // Copy existing segments to the new array
    for (size_t i = 0; i < m_segmentCount; ++i) {
        newSegments[i] = m_segments[i];  // Shallow copy is sufficient here
    }

    // Copy segments from the source into the new array
    // Here we should not create a new StringEntry, but reuse the existing ones and increment the reference count
    for (size_t i = 0; i < src.m_segmentCount; ++i) {
        newSegments[m_segmentCount + i].ptr = src.m_segments[i].ptr;
        newSegments[m_segmentCount + i].ptr->addRef();  // Increment reference count for the reused StringEntry
        newSegments[m_segmentCount + i].ofs = src.m_segments[i].ofs;
        newSegments[m_segmentCount + i].len = src.m_segments[i].len;
    }

    // Delete the old segments array and update to the new one
    delete[] m_segments;
    m_segments = newSegments;
    m_segmentCount += src.m_segmentCount;

    return *this;
}

CPatchStr CPatchStr::subStr(size_t from, size_t len) const {
    // Ensure the requested substring is within bounds, adjusting len if necessary.
    size_t totalLength = 0;
    for (size_t i = 0; i < m_segmentCount; ++i) {
        totalLength += m_segments[i].len;
    }
    if (from + len > totalLength) throw std::out_of_range("Substring start is out of range");
    len = std::min(len, totalLength - from);  // Adjust len to not exceed total length

    CPatchStr result;
    size_t processedLength = 0;
    size_t currentPos = 0;

    // First, find out how many segments are needed for the new substring.
    for (size_t i = 0; i < m_segmentCount && processedLength < len; ++i) {
        if (currentPos + m_segments[i].len <= from) {
            currentPos += m_segments[i].len;  // Skip segments before the start of the substring
            continue;
        }
        if (currentPos >= from + len) break;  // Stop if we have processed all necessary segments

        // Now we know this segment contributes to the substring
        if (result.m_segmentCount == 0) {
            result.m_segments = new StringList[m_segmentCount];  // Allocate with maximum possible size, but this can be optimized
        }
        size_t segmentStart = (from > currentPos) ? from - currentPos : 0;
        size_t segmentEnd = std::min(currentPos + m_segments[i].len, from + len) - currentPos;
        result.m_segments[result.m_segmentCount].ptr = m_segments[i].ptr;
        result.m_segments[result.m_segmentCount].ptr->addRef();
        result.m_segments[result.m_segmentCount].ofs = m_segments[i].ofs + segmentStart;
        result.m_segments[result.m_segmentCount].len = segmentEnd - segmentStart;

        processedLength += result.m_segments[result.m_segmentCount].len;
        currentPos += m_segments[i].len;
        result.m_segmentCount++;
    }

    return result;
}

CPatchStr & CPatchStr::insert(size_t pos, const CPatchStr & src) {
    // Check for self-insertion and use a copy if necessary
    CPatchStr sourceCopy;
    const CPatchStr* insertSource = &src;
    if (this == &src) {
        sourceCopy = src;  // Make a copy to prevent modification issues
        insertSource = &sourceCopy;
    }

    size_t newSegmentCount = m_segmentCount + insertSource->m_segmentCount;
    bool needSplit = false;
    size_t insertIndex = 0, insertOffset = 0;

    // Find the position to insert
    for (size_t i = 0; i < m_segmentCount; ++i) {
        if (pos <= insertOffset + m_segments[i].len) {
            insertIndex = i;
            needSplit = pos < insertOffset + m_segments[i].len;
            break;
        }
        insertOffset += m_segments[i].len;
    }

    if (needSplit) {
        newSegmentCount++;
    }

    StringList * newSegments = new StringList[newSegmentCount];
    size_t newSegIndex = 0;

    // Copy the original segments before the insertion point
    for (size_t i = 0; i < insertIndex; ++i) {
        newSegments[newSegIndex++] = m_segments[i];
        m_segments[i].ptr->addRef();
    }

    // Handle the split segment
    if (needSplit) {
        size_t splitLen = pos - insertOffset;
        newSegments[newSegIndex++] = {m_segments[insertIndex].ofs, static_cast<int>(splitLen), m_segments[insertIndex].ptr};
        m_segments[insertIndex].ptr->addRef();

        // Insert the source segments
        for (size_t i = 0; i < insertSource->m_segmentCount; ++i) {
            newSegments[newSegIndex++] = {insertSource->m_segments[i].ofs, insertSource->m_segments[i].len, insertSource->m_segments[i].ptr};
            insertSource->m_segments[i].ptr->addRef();
        }

        // Finish the split segment
        newSegments[newSegIndex] = {m_segments[insertIndex].ofs + static_cast<int>(splitLen),
                                    m_segments[insertIndex].len - static_cast<int>(splitLen), m_segments[insertIndex].ptr};
        newSegments[newSegIndex++].ptr->addRef();
        insertIndex++;  // Skip the original segment that was split
    } else {
        // Insert the source segments directly if no split
        for (size_t i = 0; i < insertSource->m_segmentCount; ++i) {
            newSegments[newSegIndex++] = {insertSource->m_segments[i].ofs, insertSource->m_segments[i].len, insertSource->m_segments[i].ptr};
            insertSource->m_segments[i].ptr->addRef();
        }
    }

    // Copy the remaining original segments after the insertion point
    for (size_t i = insertIndex; i < m_segmentCount; ++i) {
        newSegments[newSegIndex++] = m_segments[i];
        m_segments[i].ptr->addRef();
    }

    // Release old segments
    /*for (size_t i = 0; i < m_segmentCount; ++i) {
        m_segments[i].ptr->release();  // Assuming release method decreases refCount and deletes if 0
    }*/
    delete[] m_segments;

    m_segments = newSegments;
    m_segmentCount = newSegmentCount;

    return *this;
}

CPatchStr & CPatchStr::remove(size_t from, size_t len) {
    size_t origLength = strlen(this->toStr()); // Calculate the original string length
    if (from + len > origLength || from > origLength) {
        throw std::out_of_range("Range out of bounds");
    }

    if (len == 0 || m_segmentCount == 0) {
        return *this; // No change if length to remove is 0 or no segments
    }

    size_t newSegmentCount = 0;
    size_t endPos = from + len;
    size_t currentPos = 0;

    // Count the necessary segments for the new array
    for (size_t i = 0; i < m_segmentCount; ++i) {
        size_t segStart = currentPos;
        size_t segEnd = currentPos + m_segments[i].len;
        if (segEnd <= from || segStart >= endPos) {
            // This segment is fully kept
            newSegmentCount++;
        } else if (segStart < from && segEnd > endPos) {
            // This segment is partially kept on both sides
            newSegmentCount += 2;
        } else if ((segStart < from && segEnd > from) || (segStart < endPos && segEnd > endPos)) {
            // This segment is partially kept on one side
            newSegmentCount++;
        }
        currentPos = segEnd;
    }

    StringList * newSegments = new StringList[newSegmentCount];
    size_t newSegIndex = 0;
    currentPos = 0;

    // Construct the new segments array
    for (size_t i = 0; i < m_segmentCount; ++i) {
        size_t segStart = currentPos;
        size_t segEnd = currentPos + m_segments[i].len;
        if (segEnd <= from || segStart >= endPos) {
            // This segment is fully kept
            newSegments[newSegIndex++] = m_segments[i];
            m_segments[i].ptr->addRef();
        } else if (segStart < from && segEnd > endPos) {
            // This segment is split
            // Add the first part of the segment
            newSegments[newSegIndex].ptr = m_segments[i].ptr;
            newSegments[newSegIndex].ptr->addRef();
            newSegments[newSegIndex].ofs = m_segments[i].ofs;
            newSegments[newSegIndex].len = from - segStart;
            newSegIndex++;

            // Add the second part of the segment
            newSegments[newSegIndex].ptr = m_segments[i].ptr;
            newSegments[newSegIndex].ptr->addRef();
            newSegments[newSegIndex].ofs = m_segments[i].ofs + (endPos - segStart);
            newSegments[newSegIndex].len = segEnd - endPos;
            newSegIndex++;
        } else if (segStart < from && segEnd > from) {
            // Keep the beginning of the segment
            newSegments[newSegIndex].ptr = m_segments[i].ptr;
            newSegments[newSegIndex].ptr->addRef();
            newSegments[newSegIndex].ofs = m_segments[i].ofs;
            newSegments[newSegIndex].len = from - segStart;
            newSegIndex++;
        } else if (segStart < endPos && segEnd > endPos) {
            // Keep the end of the segment
            newSegments[newSegIndex].ptr = m_segments[i].ptr;
            newSegments[newSegIndex].ptr->addRef();
            newSegments[newSegIndex].ofs = m_segments[i].ofs + (endPos - segStart);
            newSegments[newSegIndex].len = segEnd - endPos;
            newSegIndex++;
        }
        currentPos = segEnd;
    }

    // Release the old segments and assign the new segment array
    /*for (size_t i = 0; i < m_segmentCount; ++i) {
        m_segments[i].ptr->release();
    }*/
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

    CPatchStr * str1 = new CPatchStr("hello");
    CPatchStr str2 = *str1;
    delete str1;  // str1 is deleted, but str2 might still refer to its data.
// Accessing str2 after str1 has been deleted might cause a segmentation fault if they share data improperly.
    assert(strcmp(str2.toStr(), "hello") == 0);


  return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
