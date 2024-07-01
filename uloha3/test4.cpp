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
    std::shared_ptr<char[]> str;
    size_t length;

    // Constructor
    StringEntry(const char* s) : str(new char[std::strlen(s) + 1]), length(std::strlen(s)) {
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
            m_segments[i] = other.m_segments[i];
        }
    }

    // Destructor
    ~CPatchStr() {
        delete[] m_segments;
    }

    // Copy assignment operator
    CPatchStr& operator=(const CPatchStr& other) {
        if (this != &other) {
            // Directly allocate new memory for the segments with the size of the other's segment count
            StringList * newSegments = new StringList[other.m_segmentCount];

            // Copy the segments from 'other' to this object, using shared_ptr's assignment
            // which will handle reference counting automatically
            for (size_t i = 0; i < other.m_segmentCount; ++i) {
                newSegments[i].ptr = other.m_segments[i].ptr;  // Shared_ptr assignment
                newSegments[i].ofs = other.m_segments[i].ofs;
                newSegments[i].len = other.m_segments[i].len;
            }

            // Now, safely delete the old segments array and assign the new one
            delete[] m_segments;
            m_segments = newSegments;
            m_segmentCount = other.m_segmentCount;
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

};
char* CPatchStr::toStr() const {
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
    if (from + len > strlen(toStr())) {  // Checking the range
        throw std::out_of_range("Substring out of range");
    }

    CPatchStr result;
    size_t processedLength = 0;
    size_t start = from;

    // Initialize the result's segments with enough space.
    result.m_segments = new StringList[m_segmentCount];  // Might need to adjust based on actual needed segments
    result.m_segmentCount = 0;

    for (size_t i = 0; i < m_segmentCount && processedLength < len; ++i) {
        if (start >= static_cast<size_t>(m_segments[i].len)) {
            start -= static_cast<size_t>(m_segments[i].len);  // Skip this segment if the start is beyond its length
            continue;
        }
        size_t segmentLength = std::min(len - processedLength, m_segments[i].len - start);

        result.m_segments[result.m_segmentCount].ptr = m_segments[i].ptr;
        result.m_segments[result.m_segmentCount].ofs = m_segments[i].ofs + static_cast<int>(start);
        result.m_segments[result.m_segmentCount].len = static_cast<int>(segmentLength);

        processedLength += segmentLength;
        start = 0;  // Reset start for next segments
        result.m_segmentCount++;
    }

    return result;
}

CPatchStr & CPatchStr::insert(size_t pos, const CPatchStr & src) {
    size_t origLength = strlen(this->toStr());
    if (pos > origLength) {
        throw std::out_of_range("Range out of bounds");
    }

    // Calculate the new segment count
    size_t newSegmentCount = m_segmentCount + src.m_segmentCount;
    bool splitSegment = false;
    size_t splitIndex = 0;
    size_t splitOffset = 0;
    size_t currentPos = 0;

    // Find if we need to split a segment and where
    for (size_t i = 0; i < m_segmentCount; ++i) {
        if (currentPos <= pos && pos < currentPos + m_segments[i].len) {
            splitSegment = true;
            splitIndex = i;
            splitOffset = pos - currentPos;
            break;
        }
        currentPos += m_segments[i].len;
    }
    if (splitSegment) {
        newSegmentCount++;  // We need an extra segment for the split
    }

    // Create the new segment array
    StringList * newSegments = new StringList[newSegmentCount];

    // Copy the original segments up to the split index
    size_t newSegIndex = 0;
    for (size_t i = 0; i < splitIndex; ++i) {
        newSegments[newSegIndex++] = m_segments[i];
    }

    // If we split a segment, handle the first part of the split
    if (splitSegment) {
        newSegments[newSegIndex].ptr = m_segments[splitIndex].ptr;
        newSegments[newSegIndex].ofs = m_segments[splitIndex].ofs;
        newSegments[newSegIndex].len = static_cast<int>(splitOffset);
        newSegIndex++;
    }

    // Insert the new segments from 'src'
    for (size_t i = 0; i < src.m_segmentCount; ++i) {
        newSegments[newSegIndex++] = {src.m_segments[i].ofs, src.m_segments[i].len, src.m_segments[i].ptr};
    }

    // Handle the second part of the split, if necessary
    if (splitSegment) {
        newSegments[newSegIndex].ptr = m_segments[splitIndex].ptr;
        newSegments[newSegIndex].ofs = m_segments[splitIndex].ofs + static_cast<int>(splitOffset);
        newSegments[newSegIndex].len = m_segments[splitIndex].len - static_cast<int>(splitOffset);
        newSegIndex++;
        splitIndex++;
    }

    // Copy the remaining segments
    for (size_t i = splitIndex; i < m_segmentCount; ++i) {
        newSegments[newSegIndex++] = m_segments[i];
    }

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
        } else if (segStart < from && segEnd > endPos) {
            // This segment is split
            // Add the first part of the segment
            newSegments[newSegIndex].ptr = m_segments[i].ptr;
            newSegments[newSegIndex].ofs = m_segments[i].ofs;
            newSegments[newSegIndex].len = from - segStart;
            newSegIndex++;

            // Add the second part of the segment
            newSegments[newSegIndex].ptr = m_segments[i].ptr;
            newSegments[newSegIndex].ofs = m_segments[i].ofs + (endPos - segStart);
            newSegments[newSegIndex].len = segEnd - endPos;
            newSegIndex++;
        } else if (segStart < from && segEnd > from) {
            // Keep the beginning of the segment
            newSegments[newSegIndex].ptr = m_segments[i].ptr;
            newSegments[newSegIndex].ofs = m_segments[i].ofs;
            newSegments[newSegIndex].len = from - segStart;
            newSegIndex++;
        } else if (segStart < endPos && segEnd > endPos) {
            // Keep the end of the segment
            newSegments[newSegIndex].ptr = m_segments[i].ptr;
            newSegments[newSegIndex].ofs = m_segments[i].ofs + (endPos - segStart);
            newSegments[newSegIndex].len = segEnd - endPos;
            newSegIndex++;
        }
        currentPos = segEnd;
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
    emptyStr.append("world");
    assert(stringMatch(emptyStr.toStr(), "world"));

    // Inserting into an empty string (should be like append)
    emptyStr.insert(0, "hello");
    assert(stringMatch(emptyStr.toStr(), "helloworld"));

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


// Assertion for subStr with out-of-range parameters in the first implementation
try {
    CPatchStr().subStr(100, 10);
    assert(false); // Should not reach here
} catch (const std::out_of_range&) {
    // Expected behavior
}

// Assertion for remove with out-of-range parameters in the first implementation
try {
    CPatchStr().remove(100, 10);
    assert(false); // Should not reach here
} catch (const std::out_of_range&) {
    // Expected behavior
}


  return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
