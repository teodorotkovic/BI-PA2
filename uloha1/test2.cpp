#ifndef __PROGTEST__
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <cmath>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <functional>
#include <memory>
#endif /* __PROGTEST__ */

struct LandRecord {

    LandRecord() {}

    std::string city, addr, region, owner;
    unsigned id;

    LandRecord(const std::string& c, const std::string& a, const std::string& r, unsigned i, const std::string& o = "")
        : city(c), addr(a), region(r), id(i), owner(o) {}

    bool operator==(const LandRecord& other) const {
        return city == other.city && addr == other.addr && region == other.region && id == other.id;
    }
};

class CIterator
{
 private:
    const std::vector<LandRecord>& records; // Reference to the records vector
    int currentIndex; // Current position in the vector
  public:
    CIterator(const std::vector<LandRecord>& recs)
            : records(recs), currentIndex(0) {}
    bool                     atEnd                         () const {
        return currentIndex >= records.size();
    }
    void                     next                          () {
        if (!atEnd()) {
            currentIndex++;
        }
    }
    std::string              city                          () const {
        return records[currentIndex].city;
    }
    std::string              addr                          () const {
        return records[currentIndex].addr;
    }
    std::string              region                        () const {
        return records[currentIndex].region;
    }
    unsigned                 id                            () const {
        return records[currentIndex].id;
    }
    std::string              owner                         () const {
        return records[currentIndex].owner;
    }

};

class CLandRegister
{
  private:
    std::vector<LandRecord> records;
    std::vector<LandRecord> recordsRegIdSort;
    std::vector<std::string> owners;
    std::vector<std::vector<const LandRecord*>> ownersRecords;

    int findOwnerIndex(const std::string& owner) {
        auto it = std::find(owners.begin(), owners.end(), owner);
        if (it == owners.end()) {
            owners.push_back(owner);
            ownersRecords.push_back({});
            return owners.size() - 1;
        }
        return std::distance(owners.begin(), it);
    }

    auto findByCityAddr(const std::string& city, const std::string& addr) {

        /*return std::find_if(records.begin(), records.end(), [&](const LandRecord& rec) {
            return rec.city == city && rec.addr == addr;
        });*/
        LandRecord key;
        key.city = city;
        key.addr = addr;

        auto it = std::lower_bound(records.begin(), records.end(), key,
                                   [](const LandRecord& rec, const LandRecord& key) {
                                       return std::tie(rec.city, rec.addr) < std::tie(key.city, key.addr);
                                   }
        );

        if (it != records.end() && it->city == city && it->addr == addr) {
            // Found the record
            return it;
        } else {
            // Record not found
            return records.end();
        }
    }

    auto findByRegionId(const std::string& region, unsigned id) {
        /*return std::find_if(records.begin(), records.end(), [&](const LandRecord& rec) {
            return rec.region == region && rec.id == id;
        });*/
        LandRecord key;
        key.region = region;
        key.id = id;

        auto it = std::lower_bound(recordsRegIdSort.begin(), recordsRegIdSort.end(), key,
                                   [](const LandRecord& rec, const LandRecord& key) {
                                       return std::tie(rec.region, rec.id) < std::tie(key.region, key.id);
                                   }
        );

        if (it != recordsRegIdSort.end() && it->region == region && it->id == id) {
            // Found the record
            return it;
        } else {
            // Record not found
            return recordsRegIdSort.end();
        }
    }

    /*auto insertSorted(const LandRecord& record) {
        auto it1 = std::lower_bound(records.begin(), records.end(), record,
                                   [](const LandRecord& a, const LandRecord& b) {
                                       return a.city < b.city || (a.city == b.city && a.addr < b.addr);
                                   });
        records.insert(it1, record);
        auto it2 = std::lower_bound(recordsRegIdSort.begin(), recordsRegIdSort.end(), record,
                                   [](const LandRecord& a, const LandRecord& b) {
                                       return a.region < b.region || (a.region == b.region && a.id < b.id);
                                   });
        recordsRegIdSort.insert(it2, record);
        return it1;
    }*/
  public:
    CLandRegister() {
        owners.push_back(""); // Initialize for unowned records
        ownersRecords.push_back({});
    }
    bool                     add                           ( const std::string    & city,
                                                             const std::string    & addr,
                                                             const std::string    & region,
                                                             unsigned int           id ) {
        if (findByCityAddr(city, addr) != records.end() || findByRegionId(region, id) != recordsRegIdSort.end()) {
            return false; // Record already exists
        }
        LandRecord newRecord(city, addr, region, id, "");
        // Insert the new record into the main records vector and get the iterator
        auto it1 = std::lower_bound(records.begin(), records.end(), newRecord,
                                    [](const LandRecord& a, const LandRecord& b) {
                                        return a.city < b.city || (a.city == b.city && a.addr < b.addr);
                                    });
        it1 = records.insert(it1, newRecord);

        // Insert the new record into the sorted by region and id vector
        auto it2 = std::lower_bound(recordsRegIdSort.begin(), recordsRegIdSort.end(), newRecord,
                                    [](const LandRecord& a, const LandRecord& b) {
                                        return a.region < b.region || (a.region == b.region && a.id < b.id);
                                    });
        recordsRegIdSort.insert(it2, newRecord);

        // Now that the record is inserted, update the owner information
        int index = findOwnerIndex(""); // Assuming new records have no owner initially
        ownersRecords[index].push_back(&*it1); // Use the iterator to get the pointer to the newly added record

        return true;
    }

    bool                     del                           ( const std::string    & city,
                                                             const std::string    & addr ) {
        auto it = findByCityAddr(city, addr);
        if (it != records.end()) {
            int index = findOwnerIndex(it->owner);
            auto it2 = findByRegionId(it->region, it->id);
            ownersRecords[index].erase(std::remove(ownersRecords[index].begin(), ownersRecords[index].end(), &*it), ownersRecords[index].end());
            records.erase(it);
            recordsRegIdSort.erase(it2);
            return true;
        }
        return false;
    }

    bool                     del                           ( const std::string    & region,
                                                             unsigned int           id ) {
        auto it = findByRegionId(region, id);
        if (it != records.end()) {
            int index = findOwnerIndex(it->owner);
            auto it2 = findByCityAddr(it->city, it->addr);
            ownersRecords[index].erase(std::remove(ownersRecords[index].begin(), ownersRecords[index].end(), &*it), ownersRecords[index].end());
            records.erase(it);
            recordsRegIdSort.erase(it2);
            return true;
        }
        return false;
    }

    bool                     getOwner                      ( const std::string    & city,
                                                             const std::string    & addr,
                                                             std::string          & owner ) const {
        auto it = std::find_if(records.begin(), records.end(), [&](const LandRecord& rec) {
            return rec.city == city && rec.addr == addr;
        });
        if (it != records.end()) {
            owner = it->owner;
            return true;
        }
        return false;
    }

    bool                     getOwner                      ( const std::string    & region,
                                                             unsigned int           id,
                                                             std::string          & owner ) const {
        auto it = std::find_if(records.begin(), records.end(), [&](const LandRecord& rec) {
            return rec.region == region && rec.id == id;
        });
        if (it != records.end()) {
            owner = it->owner;
            return true;
        }
        return false;
    }

    bool                     newOwner                      ( const std::string    & city,
                                                             const std::string    & addr,
                                                             const std::string    & owner ) {
        for (auto& record : records) {
            if (record.city == city && record.addr == addr) {
                // Remove from old owner list
                int oldIndex = findOwnerIndex(record.owner);
                ownersRecords[oldIndex].erase(std::remove(ownersRecords[oldIndex].begin(), ownersRecords[oldIndex].end(), &record), ownersRecords[oldIndex].end());

                // Update owner in the record
                record.owner = owner;

                // Add to new owner list
                int newIndex = findOwnerIndex(owner);
                ownersRecords[newIndex].push_back(&record);
                return true;
            }
        }
        return false;
    }

    bool                     newOwner                      ( const std::string    & region,
                                                             unsigned int           id,
                                                             const std::string    & owner ) {
        for (auto& record : records) {
            if (record.region == region && record.id == id) {
                // Remove from old owner list
                int oldIndex = findOwnerIndex(record.owner);
                ownersRecords[oldIndex].erase(std::remove(ownersRecords[oldIndex].begin(), ownersRecords[oldIndex].end(), &record), ownersRecords[oldIndex].end());

                // Update owner in the record
                record.owner = owner;

                // Add to new owner list
                int newIndex = findOwnerIndex(owner);
                ownersRecords[newIndex].push_back(&record);
                return true;
            }
        }
        return false;
    }

    size_t                   count                         ( const std::string    & owner ) const {
        /*return std::count_if(records.begin(), records.end(), [&](const LandRecord& rec) {
            return rec.owner == owner;
        });*/
        auto it = std::find(owners.begin(), owners.end(), owner);
        if (it != owners.end()) {
            int index = std::distance(owners.begin(), it);
            size_t size = ownersRecords[index].size();
            return size;
        }
        return 0; // Owner not found
    }

    CIterator                listByAddr                    () const {
        return CIterator(records);
    }


    CIterator                listByOwner                   ( const std::string    & owner ) const {
        auto it = std::find(owners.begin(), owners.end(), owner);
        if (it == owners.end()) {
            return CIterator(std::vector<LandRecord>()); // Return an empty CIterator if owner is not found
        }

        int index = std::distance(owners.begin(), it);
        std::vector<LandRecord> recordsCopy;
        for (const LandRecord* recordPtr : ownersRecords[index]) {
            recordsCopy.push_back(*recordPtr);
        }

        return CIterator(recordsCopy);
    }

};

#ifndef __PROGTEST__
static void test0 ()
{
  CLandRegister x;
  std::string owner;

  assert ( x . add ( "Prague", "Thakurova", "Dejvice", 12345 ) );
  assert ( x . add ( "Prague", "Evropska", "Vokovice", 12345 ) );
  assert ( x . add ( "Prague", "Technicka", "Dejvice", 9873 ) );
  assert ( x . add ( "Plzen", "Evropska", "Plzen mesto", 78901 ) );
  assert ( x . add ( "Liberec", "Evropska", "Librec", 4552 ) );/*
  CIterator i0 = x . listByAddr ();

  assert ( ! i0 . atEnd ()
           && i0 . city () == "Liberec"
           && i0 . addr () == "Evropska"
           && i0 . region () == "Librec"
           && i0 . id () == 4552
           && i0 . owner () == "" );
  i0 . next ();
  assert ( ! i0 . atEnd ()
           && i0 . city () == "Plzen"
           && i0 . addr () == "Evropska"
           && i0 . region () == "Plzen mesto"
           && i0 . id () == 78901
           && i0 . owner () == "" );
  i0 . next ();
  assert ( ! i0 . atEnd ()
           && i0 . city () == "Prague"
           && i0 . addr () == "Evropska"
           && i0 . region () == "Vokovice"
           && i0 . id () == 12345
           && i0 . owner () == "" );
  i0 . next ();
  assert ( ! i0 . atEnd ()
           && i0 . city () == "Prague"
           && i0 . addr () == "Technicka"
           && i0 . region () == "Dejvice"
           && i0 . id () == 9873
           && i0 . owner () == "" );
  i0 . next ();
  assert ( ! i0 . atEnd ()
           && i0 . city () == "Prague"
           && i0 . addr () == "Thakurova"
           && i0 . region () == "Dejvice"
           && i0 . id () == 12345
           && i0 . owner () == "" );
  i0 . next ();
  assert ( i0 . atEnd () );
*/
  assert ( x . count ( "" ) == 5 );
  CIterator i1 = x . listByOwner ( "" );
  assert ( ! i1 . atEnd ()
           && i1 . city () == "Prague"
           && i1 . addr () == "Thakurova"
           && i1 . region () == "Dejvice"
           && i1 . id () == 12345
           && i1 . owner () == "" );
  i1 . next ();
  assert ( ! i1 . atEnd ()
           && i1 . city () == "Prague"
           && i1 . addr () == "Evropska"
           && i1 . region () == "Vokovice"
           && i1 . id () == 12345
           && i1 . owner () == "" );
  i1 . next ();
  assert ( ! i1 . atEnd ()
           && i1 . city () == "Prague"
           && i1 . addr () == "Technicka"
           && i1 . region () == "Dejvice"
           && i1 . id () == 9873
           && i1 . owner () == "" );
  i1 . next ();
  assert ( ! i1 . atEnd ()
           && i1 . city () == "Plzen"
           && i1 . addr () == "Evropska"
           && i1 . region () == "Plzen mesto"
           && i1 . id () == 78901
           && i1 . owner () == "" );
  i1 . next ();
  assert ( ! i1 . atEnd ()
           && i1 . city () == "Liberec"
           && i1 . addr () == "Evropska"
           && i1 . region () == "Librec"
           && i1 . id () == 4552
           && i1 . owner () == "" );
  i1 . next ();
  assert ( i1 . atEnd () );
/*
  assert ( x . count ( "CVUT" ) == 0 );
  CIterator i2 = x . listByOwner ( "CVUT" );
  assert ( i2 . atEnd () );

  assert ( x . newOwner ( "Prague", "Thakurova", "CVUT" ) );
  assert ( x . newOwner ( "Dejvice", 9873, "CVUT" ) );
  assert ( x . newOwner ( "Plzen", "Evropska", "Anton Hrabis" ) );
  assert ( x . newOwner ( "Librec", 4552, "Cvut" ) );
  assert ( x . getOwner ( "Prague", "Thakurova", owner ) && owner == "CVUT" );
  assert ( x . getOwner ( "Dejvice", 12345, owner ) && owner == "CVUT" );
  assert ( x . getOwner ( "Prague", "Evropska", owner ) && owner == "" );
  assert ( x . getOwner ( "Vokovice", 12345, owner ) && owner == "" );
  assert ( x . getOwner ( "Prague", "Technicka", owner ) && owner == "CVUT" );
  assert ( x . getOwner ( "Dejvice", 9873, owner ) && owner == "CVUT" );
  assert ( x . getOwner ( "Plzen", "Evropska", owner ) && owner == "Anton Hrabis" );
  assert ( x . getOwner ( "Plzen mesto", 78901, owner ) && owner == "Anton Hrabis" );
  assert ( x . getOwner ( "Liberec", "Evropska", owner ) && owner == "Cvut" );
  assert ( x . getOwner ( "Librec", 4552, owner ) && owner == "Cvut" );
  CIterator i3 = x . listByAddr ();
  assert ( ! i3 . atEnd ()
           && i3 . city () == "Liberec"
           && i3 . addr () == "Evropska"
           && i3 . region () == "Librec"
           && i3 . id () == 4552
           && i3 . owner () == "Cvut" );
  i3 . next ();
  assert ( ! i3 . atEnd ()
           && i3 . city () == "Plzen"
           && i3 . addr () == "Evropska"
           && i3 . region () == "Plzen mesto"
           && i3 . id () == 78901
           && i3 . owner () == "Anton Hrabis" );
  i3 . next ();
  assert ( ! i3 . atEnd ()
           && i3 . city () == "Prague"
           && i3 . addr () == "Evropska"
           && i3 . region () == "Vokovice"
           && i3 . id () == 12345
           && i3 . owner () == "" );
  i3 . next ();
  assert ( ! i3 . atEnd ()
           && i3 . city () == "Prague"
           && i3 . addr () == "Technicka"
           && i3 . region () == "Dejvice"
           && i3 . id () == 9873
           && i3 . owner () == "CVUT" );
  i3 . next ();
  assert ( ! i3 . atEnd ()
           && i3 . city () == "Prague"
           && i3 . addr () == "Thakurova"
           && i3 . region () == "Dejvice"
           && i3 . id () == 12345
           && i3 . owner () == "CVUT" );
  i3 . next ();
  assert ( i3 . atEnd () );
/*
  assert ( x . count ( "cvut" ) == 3 );
  CIterator i4 = x . listByOwner ( "cVuT" );
  assert ( ! i4 . atEnd ()
           && i4 . city () == "Prague"
           && i4 . addr () == "Thakurova"
           && i4 . region () == "Dejvice"
           && i4 . id () == 12345
           && i4 . owner () == "CVUT" );
  i4 . next ();
  assert ( ! i4 . atEnd ()
           && i4 . city () == "Prague"
           && i4 . addr () == "Technicka"
           && i4 . region () == "Dejvice"
           && i4 . id () == 9873
           && i4 . owner () == "CVUT" );
  i4 . next ();
  assert ( ! i4 . atEnd ()
           && i4 . city () == "Liberec"
           && i4 . addr () == "Evropska"
           && i4 . region () == "Librec"
           && i4 . id () == 4552
           && i4 . owner () == "Cvut" );
  i4 . next ();
  assert ( i4 . atEnd () );

  assert ( x . newOwner ( "Plzen mesto", 78901, "CVut" ) );
  assert ( x . count ( "CVUT" ) == 4 );
  CIterator i5 = x . listByOwner ( "CVUT" );
  assert ( ! i5 . atEnd ()
           && i5 . city () == "Prague"
           && i5 . addr () == "Thakurova"
           && i5 . region () == "Dejvice"
           && i5 . id () == 12345
           && i5 . owner () == "CVUT" );
  i5 . next ();
  assert ( ! i5 . atEnd ()
           && i5 . city () == "Prague"
           && i5 . addr () == "Technicka"
           && i5 . region () == "Dejvice"
           && i5 . id () == 9873
           && i5 . owner () == "CVUT" );
  i5 . next ();
  assert ( ! i5 . atEnd ()
           && i5 . city () == "Liberec"
           && i5 . addr () == "Evropska"
           && i5 . region () == "Librec"
           && i5 . id () == 4552
           && i5 . owner () == "Cvut" );
  i5 . next ();
  assert ( ! i5 . atEnd ()
           && i5 . city () == "Plzen"
           && i5 . addr () == "Evropska"
           && i5 . region () == "Plzen mesto"
           && i5 . id () == 78901
           && i5 . owner () == "CVut" );
  i5 . next ();
  assert ( i5 . atEnd () );

  assert ( x . del ( "Liberec", "Evropska" ) );
  assert ( x . del ( "Plzen mesto", 78901 ) );
  assert ( x . count ( "cvut" ) == 2 );
  CIterator i6 = x . listByOwner ( "cVuT" );
  assert ( ! i6 . atEnd ()
           && i6 . city () == "Prague"
           && i6 . addr () == "Thakurova"
           && i6 . region () == "Dejvice"
           && i6 . id () == 12345
           && i6 . owner () == "CVUT" );
  i6 . next ();
  assert ( ! i6 . atEnd ()
           && i6 . city () == "Prague"
           && i6 . addr () == "Technicka"
           && i6 . region () == "Dejvice"
           && i6 . id () == 9873
           && i6 . owner () == "CVUT" );
  i6 . next ();
  assert ( i6 . atEnd () );

  assert ( x . add ( "Liberec", "Evropska", "Librec", 4552 ) );*/
}
/*
static void test1 ()
{
  CLandRegister x;
  std::string owner;

  assert ( x . add ( "Prague", "Thakurova", "Dejvice", 12345 ) );
  assert ( x . add ( "Prague", "Evropska", "Vokovice", 12345 ) );
  assert ( x . add ( "Prague", "Technicka", "Dejvice", 9873 ) );
  assert ( ! x . add ( "Prague", "Technicka", "Hradcany", 7344 ) );
  assert ( ! x . add ( "Brno", "Bozetechova", "Dejvice", 9873 ) );
  assert ( !x . getOwner ( "Prague", "THAKUROVA", owner ) );
  assert ( !x . getOwner ( "Hradcany", 7343, owner ) );
  CIterator i0 = x . listByAddr ();
  assert ( ! i0 . atEnd ()
           && i0 . city () == "Prague"
           && i0 . addr () == "Evropska"
           && i0 . region () == "Vokovice"
           && i0 . id () == 12345
           && i0 . owner () == "" );
  i0 . next ();
  assert ( ! i0 . atEnd ()
           && i0 . city () == "Prague"
           && i0 . addr () == "Technicka"
           && i0 . region () == "Dejvice"
           && i0 . id () == 9873
           && i0 . owner () == "" );
  i0 . next ();
  assert ( ! i0 . atEnd ()
           && i0 . city () == "Prague"
           && i0 . addr () == "Thakurova"
           && i0 . region () == "Dejvice"
           && i0 . id () == 12345
           && i0 . owner () == "" );
  i0 . next ();
  assert ( i0 . atEnd () );

  assert ( x . newOwner ( "Prague", "Thakurova", "CVUT" ) );
  assert ( ! x . newOwner ( "Prague", "technicka", "CVUT" ) );
  assert ( ! x . newOwner ( "prague", "Technicka", "CVUT" ) );
  assert ( ! x . newOwner ( "dejvice", 9873, "CVUT" ) );
  assert ( ! x . newOwner ( "Dejvice", 9973, "CVUT" ) );
  assert ( ! x . newOwner ( "Dejvice", 12345, "CVUT" ) );
  assert ( x . count ( "CVUT" ) == 1 );
  CIterator i1 = x . listByOwner ( "CVUT" );
  assert ( ! i1 . atEnd ()
           && i1 . city () == "Prague"
           && i1 . addr () == "Thakurova"
           && i1 . region () == "Dejvice"
           && i1 . id () == 12345
           && i1 . owner () == "CVUT" );
  i1 . next ();
  assert ( i1 . atEnd () );

  assert ( ! x . del ( "Brno", "Technicka" ) );
  assert ( ! x . del ( "Karlin", 9873 ) );
  assert ( x . del ( "Prague", "Technicka" ) );
  assert ( ! x . del ( "Prague", "Technicka" ) );
  assert ( ! x . del ( "Dejvice", 9873 ) );
}
*/
int main ( void )
{
  test0 ();
  //test1 ();
  return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
