#include <string>

static std::string oFAdd(const std::string & a, const std::string & b) {
    return a + b;
}
typedef std::string (*opString2)(const std::string&, const std::string&);

double oFSub(const double & a, const double & b) {
    return a - b;
}
double oFMul(const double & a, const double & b) {
    return a * b;
}
double oFDiv(const double & a, const double & b) {
    return a / b;
}
typedef double (*opDouble2)(const double&, const double&);

double oFPow(const double & a) {
    return a * a;
}
double oFNeg(const double & a) {
    return -a;
}
typedef double (*opDouble1)(const double&);

double oFEq(const double & a, const double & b) {
    return a == b ? 1 : 0;
}
double oFNe(const double & a, const double & b) {
    return a != b ? 1 : 0;
}
double oFLt(const double & a, const double & b) {
    return a < b ? 1 : 0;
}
double oFLe(const double & a, const double & b) {
    return a <= b ? 1 : 0;
}
double oFGt(const double & a, const double & b) {
    return a > b ? 1 : 0;
}
double oFGe(const double & a, const double & b) {
    return a >= b ? 1 : 0;
}
typedef int (*opInt2)(const double&, const double&);