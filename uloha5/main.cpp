#ifndef __PROGTEST__
#include <cassert>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <algorithm>
#include <memory>
#include <functional>
#endif /* __PROGTEST__ */

std::string outputW (bool last1, bool last2, int lvl) {
    std::string pref;
    if (lvl == 0) {
        pref = "";
    } else if (lvl == 1) {
        if (last1) {
            pref = "  ";
        } else {
            pref = "| ";
        }
    } else if (lvl == 2) {
        if (last1) {
            if (last2) {
                pref = "    ";
            } else {
                pref = "  | ";
            }
        } else {
            if (last2) {
                pref = "|   ";
            } else {
                pref = "| | ";
            }
        }
    }
    return pref;
}

class CComponent {
public:
    virtual ~CComponent() {}
    virtual CComponent* clone() const = 0;
    virtual void print(std::ostream& os, bool last1, bool last2, int lvl) const = 0;
};

class CComputer {
private:
    std::string name;
    std::vector<std::string> addresses;
    std::vector<std::shared_ptr<CComponent>> components;
public:
    CComputer(const std::string& name) : name(name) {}

    CComputer(const CComputer& other)
            : name(other.name), addresses(other.addresses) {
        for (const auto& component : other.components) {
            components.push_back(std::shared_ptr<CComponent>(component->clone()));
        }
    }

    CComputer& operator=(const CComputer& other) {
        if (this == &other) {
            return *this; // Handle self assignment
        }
        name = other.name;
        addresses = other.addresses;
        components.clear(); // Clear existing components
        for (const auto& component : other.components) {
            components.push_back(std::shared_ptr<CComponent>(component->clone()));
        }
        return *this;
    }

    CComputer& addAddress(const std::string& address) {
        addresses.push_back(address);
        return *this;
    }

    // Template for adding components to the computer.
    template<typename T>
    CComputer& addComponent(const T& component) {
        static_assert(std::is_base_of<CComponent, T>::value, "T must be a derived class of CComponent");
        components.push_back(std::make_shared<T>(component));
        return *this;
    }

    void print(std::ostream& os, bool last, int lvl) const {
        os << "Host: " << name << "\n";

        size_t lastAddressIndex = addresses.size() - 1;
        for (size_t i = 0; i < addresses.size(); ++i) {
            if (i == lastAddressIndex && components.empty()) {
                os << outputW(last, false, lvl) << "\\-" << addresses[i] << "\n";
            } else {
                os << outputW(last, false, lvl) << "+-" << addresses[i] << "\n";
            }
        }

        size_t lastComponentIndex = components.size() - 1;
        for (size_t i = 0; i < components.size(); ++i) {
            if (i == lastComponentIndex) {
                os << outputW(last, false, lvl) << "\\-";
                if (lvl == 0) last = true;
                components[i]->print(os, last, true, lvl+1);
            } else {
                os << outputW(last, false, lvl) << "+-";
                if (lvl == 0) last = false;
                components[i]->print(os, last, false, lvl+1);
            }
        }
    }

    const std::string& getName() const {
        return name;
    }

    friend std::ostream& operator<<(std::ostream& os, const CComputer& comp) {
        comp.print(os, false, 0);
        return os;
    }
};

class CNetwork {
private:
    std::string networkName;
    std::vector<std::shared_ptr<CComputer>> computers;

public:
    CNetwork(const std::string &name) : networkName(name) {}

    // Deep Copy Constructor
    CNetwork(const CNetwork &other) : networkName(other.networkName) {
        for (const auto &comp : other.computers) {
            computers.push_back(std::make_shared<CComputer>(*comp));
        }
    }

    // Assignment operator
    CNetwork& operator=(const CNetwork& other) {
        if (this != &other) {
            networkName = other.networkName;
            computers.clear();  // Clear existing computers
            for (const auto& comp : other.computers) {
                computers.push_back(std::make_shared<CComputer>(*comp));
            }
        }
        return *this;
    }

    // Add computer to the network
    CNetwork& addComputer(const CComputer &computer) {
        computers.push_back(std::make_shared<CComputer>(computer));
        return *this;
    }

    // Find a computer by name
    std::shared_ptr<CComputer> findComputer(const std::string &name) const {
        for (const auto &comp : computers) {
            if (comp->getName() == name) {
                return comp;
            }
        }
        return nullptr; // Return nullptr if no computer found
    }

    friend std::ostream& operator<<(std::ostream &os, const CNetwork &network) {
        os << "Network: " << network.networkName << "\n";
        for (size_t i = 0; i < network.computers.size(); ++i) {
            if (i == network.computers.size() - 1) {
                os << "\\-";
                network.computers[i]->print(os, true, 1);
            } else {
                os << "+-";
                network.computers[i]->print(os, false, 1);
            }
        }
        return os;
    }

};


class CCPU : public CComponent {
private:
    int cores;
    int frequency;
public:
    CCPU(int cores, int frequency) : cores(cores), frequency(frequency) {}
    CCPU* clone() const override {
        return new CCPU(*this);
    }
    void print(std::ostream& os, bool last1, bool last2, int lvl) const override {
        os << "CPU, " << cores << " cores @ " << frequency << "MHz\n";
    }
};

class CDisk : public CComponent {
public:
    enum Type { SSD, MAGNETIC };

private:
    Type type;
    int size;  // Disk size in GiB
    std::vector<std::pair<int, std::string>> partitions;  // Each partition is a pair of size and label

public:
    CDisk(Type type, int size) : type(type), size(size) {}

    CDisk& addPartition(int partitionSize, const std::string& label) {
        partitions.emplace_back(partitionSize, label);
        return *this;
    }

    virtual CDisk* clone() const override {
        CDisk* newDisk = new CDisk(*this);
        return newDisk;
    }

    void print(std::ostream& os, bool last1, bool last2, int lvl) const override {
        os << (type == SSD ? "SSD" : "HDD") << ", " << size << " GiB\n";
        for (size_t i = 0; i < partitions.size(); i++) {
            os << outputW(last1, last2, lvl) << (i < partitions.size() - 1 ? "+-" : "\\-") << "[" << i << "]: "
               << partitions[i].first << " GiB, " << partitions[i].second << "\n";
        }
    }
};

class CMemory : public CComponent {
public:
    CMemory (int mem) : memory(mem){}
    virtual CMemory* clone() const override {
        return new CMemory(*this);
    }
private:
    int memory;
public:
    void print(std::ostream& os, bool last1, bool last2, int lvl) const override {
        os << "Memory, " << memory << " MiB\n";
    }
};

#ifndef __PROGTEST__
template<typename T_>
std::string toString ( const T_ & x )
{
    std::ostringstream oss;
    oss << x;
    return oss . str ();
}

int main ()
{
    CNetwork n ( "FIT network" );
    n . addComputer (
            CComputer ( "progtest.fit.cvut.cz" ) .
                    addAddress ( "147.32.232.142" ) .
                    addComponent ( CCPU ( 8, 2400 ) ) .
                    addComponent ( CCPU ( 8, 1200 ) ) .
                    addComponent ( CDisk ( CDisk::MAGNETIC, 1500 ) .
                    addPartition ( 50, "/" ) .
                    addPartition ( 5, "/boot" ).
                    addPartition ( 1000, "/var" ) ) .
                    addComponent ( CDisk ( CDisk::SSD, 60 ) .
                    addPartition ( 60, "/data" )  ) .
                    addComponent ( CMemory ( 2000 ) ).
                    addComponent ( CMemory ( 2000 ) ) ).
            addComputer (
            CComputer ( "courses.fit.cvut.cz" ) .
                    addAddress ( "147.32.232.213" ) .
                    addComponent ( CCPU ( 4, 1600 ) ) .
                    addComponent ( CMemory ( 4000 ) ).
                    addComponent ( CDisk ( CDisk::MAGNETIC, 2000 ) .
                    addPartition ( 100, "/" )   .
                    addPartition ( 1900, "/data" ) ) ) .
            addComputer (
            CComputer ( "imap.fit.cvut.cz" ) .
                    addAddress ( "147.32.232.238" ) .
                    addComponent ( CCPU ( 4, 2500 ) ) .
                    addAddress ( "2001:718:2:2901::238" ) .
                    addComponent ( CMemory ( 8000 ) ) );
    std::cout << toString(n);
    assert ( toString ( n ) ==
             "Network: FIT network\n"
             "+-Host: progtest.fit.cvut.cz\n"
             "| +-147.32.232.142\n"
             "| +-CPU, 8 cores @ 2400MHz\n"
             "| +-CPU, 8 cores @ 1200MHz\n"
             "| +-HDD, 1500 GiB\n"
             "| | +-[0]: 50 GiB, /\n"
             "| | +-[1]: 5 GiB, /boot\n"
             "| | \\-[2]: 1000 GiB, /var\n"
             "| +-SSD, 60 GiB\n"
             "| | \\-[0]: 60 GiB, /data\n"
             "| +-Memory, 2000 MiB\n"
             "| \\-Memory, 2000 MiB\n"
             "+-Host: courses.fit.cvut.cz\n"
             "| +-147.32.232.213\n"
             "| +-CPU, 4 cores @ 1600MHz\n"
             "| +-Memory, 4000 MiB\n"
             "| \\-HDD, 2000 GiB\n"
             "|   +-[0]: 100 GiB, /\n"
             "|   \\-[1]: 1900 GiB, /data\n"
             "\\-Host: imap.fit.cvut.cz\n"
             "  +-147.32.232.238\n"
             "  +-2001:718:2:2901::238\n"
             "  +-CPU, 4 cores @ 2500MHz\n"
             "  \\-Memory, 8000 MiB\n" );
    CNetwork x = n;
    auto c = x . findComputer ( "imap.fit.cvut.cz" );
    std::cout << toString(*c);
    assert ( toString ( *c ) ==
             "Host: imap.fit.cvut.cz\n"
             "+-147.32.232.238\n"
             "+-2001:718:2:2901::238\n"
             "+-CPU, 4 cores @ 2500MHz\n"
             "\\-Memory, 8000 MiB\n" );
    c -> addComponent ( CDisk ( CDisk::MAGNETIC, 1000 ) .
            addPartition ( 100, "system" ) .
            addPartition ( 200, "WWW" ) .
            addPartition ( 700, "mail" ) );
    assert ( toString ( x ) ==
             "Network: FIT network\n"
             "+-Host: progtest.fit.cvut.cz\n"
             "| +-147.32.232.142\n"
             "| +-CPU, 8 cores @ 2400MHz\n"
             "| +-CPU, 8 cores @ 1200MHz\n"
             "| +-HDD, 1500 GiB\n"
             "| | +-[0]: 50 GiB, /\n"
             "| | +-[1]: 5 GiB, /boot\n"
             "| | \\-[2]: 1000 GiB, /var\n"
             "| +-SSD, 60 GiB\n"
             "| | \\-[0]: 60 GiB, /data\n"
             "| +-Memory, 2000 MiB\n"
             "| \\-Memory, 2000 MiB\n"
             "+-Host: courses.fit.cvut.cz\n"
             "| +-147.32.232.213\n"
             "| +-CPU, 4 cores @ 1600MHz\n"
             "| +-Memory, 4000 MiB\n"
             "| \\-HDD, 2000 GiB\n"
             "|   +-[0]: 100 GiB, /\n"
             "|   \\-[1]: 1900 GiB, /data\n"
             "\\-Host: imap.fit.cvut.cz\n"
             "  +-147.32.232.238\n"
             "  +-2001:718:2:2901::238\n"
             "  +-CPU, 4 cores @ 2500MHz\n"
             "  +-Memory, 8000 MiB\n"
             "  \\-HDD, 1000 GiB\n"
             "    +-[0]: 100 GiB, system\n"
             "    +-[1]: 200 GiB, WWW\n"
             "    \\-[2]: 700 GiB, mail\n" );
    std::cout << "\n\n" << toString(n);
    assert ( toString ( n ) ==
             "Network: FIT network\n"
             "+-Host: progtest.fit.cvut.cz\n"
             "| +-147.32.232.142\n"
             "| +-CPU, 8 cores @ 2400MHz\n"
             "| +-CPU, 8 cores @ 1200MHz\n"
             "| +-HDD, 1500 GiB\n"
             "| | +-[0]: 50 GiB, /\n"
             "| | +-[1]: 5 GiB, /boot\n"
             "| | \\-[2]: 1000 GiB, /var\n"
             "| +-SSD, 60 GiB\n"
             "| | \\-[0]: 60 GiB, /data\n"
             "| +-Memory, 2000 MiB\n"
             "| \\-Memory, 2000 MiB\n"
             "+-Host: courses.fit.cvut.cz\n"
             "| +-147.32.232.213\n"
             "| +-CPU, 4 cores @ 1600MHz\n"
             "| +-Memory, 4000 MiB\n"
             "| \\-HDD, 2000 GiB\n"
             "|   +-[0]: 100 GiB, /\n"
             "|   \\-[1]: 1900 GiB, /data\n"
             "\\-Host: imap.fit.cvut.cz\n"
             "  +-147.32.232.238\n"
             "  +-2001:718:2:2901::238\n"
             "  +-CPU, 4 cores @ 2500MHz\n"
             "  \\-Memory, 8000 MiB\n" );
    return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
