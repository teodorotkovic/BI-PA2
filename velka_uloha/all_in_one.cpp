#ifndef __PROGTEST__
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <climits>
#include <cfloat>
#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <string>
#include <array>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <stack>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <variant>
#include <optional>
#include <compare>
#include <charconv>
#include <span>
#include <utility>
#include "expression.h"

using namespace std::literals;
using CValue = std::variant<std::monostate, double, std::string>;

constexpr unsigned                     SPREADSHEET_CYCLIC_DEPS                 = 0x01;
constexpr unsigned                     SPREADSHEET_FUNCTIONS                   = 0x02;
constexpr unsigned                     SPREADSHEET_FILE_IO                     = 0x04;
constexpr unsigned                     SPREADSHEET_SPEED                       = 0x08;
constexpr unsigned                     SPREADSHEET_PARSER                      = 0x10;
#endif /* __PROGTEST__ */

int globalCount = 0;


bool checkCPos(std::string_view & str) {
    size_t i = 0;
    while (i < static_cast<size_t>(str.size()) && std::isalpha(str[i])) {
        i++;
    }
    if (i == 0 || i == str.size()) { // No letters or no numbers
        return false;
    }
    size_t j = i;
    while (j < static_cast<size_t>(str.size()) && std::isdigit(str[j])) {
        j++;
    }
    if (j != static_cast<size_t>(str.size())) { // Characters after numbers or no digits at all
        return false;
    }
    return true;
}


int letterToNumber(std::string_view input) {
    int result = 0;
    for (char c : input) {
        if (std::isalpha(c)) {
            result *= 26;
            result += std::toupper(c) - 'A' + 1;
        }
    }
    return result - 1; // Excel's numbering starts from 0
}

void splitString(std::string_view input, std::string& letters, int& numbers) {
    letters.clear();
    for (char c : input) {
        if (std::isalpha(c)) {
            letters.push_back(c);
        } else {
            break;
        }
    }
    std::string numStr = std::string(input.substr(letters.size()));
    numbers = std::stoi(numStr);
}


class CPos {
public:
    CPos (std::string_view str) {
        std::string letters;
        int numbers;

        if (checkCPos(str) == false) {
            throw std::invalid_argument("Invalid cell position format.");
        }

        splitString(str, letters, numbers);

        cPosHW.first = numbers;
        cPosHW.second = letterToNumber(letters);

    }
    CPos(int h, int w) {
        cPosHW.first = h;
        cPosHW.second = w;
    }

    std::pair<int, int> cPosHW;
};

class ExprNode;

CPos globalCycleCheck = CPos("A1");

class CSpreadsheet {
public:
    static unsigned capabilities () {
        return SPREADSHEET_CYCLIC_DEPS;
    }
    CSpreadsheet () = default;
    bool load (std::istream & is);
    bool save ( std::ostream & os ) const;
    bool setCell (CPos pos, std::string contents);
    CValue getValue (CPos pos);
    void copyRect (CPos dst, CPos src, int w = 1, int h = 1);


    using cellValue = std::variant<std::monostate, double, std::string, std::shared_ptr<ExprNode>>;
    std::map<std::pair<int, int>, cellValue> m_table;
};

std::string numberToLetters(int index) {
    std::string result;
    index += 1;  // Convert from 0-based index to 1-based index for Excel columns.
    while (index > 0) {
        int mod = (index - 1) % 26; // Find remainder to determine the current letter (A=0, ..., Z=25)
        char current = 'A' + mod;   // Compute the corresponding character.
        result = current + result;  // Prepend to result.
        index = (index - mod) / 26; // Reduce index to process the next most significant letter.
    }
    return result;
}

std::string parseAndAdjustExpression(const std::string& expr, int deltaRow, int deltaCol) {
    std::string result;
    size_t i = 0;

    while (i < expr.length()) {
        if (std::isalpha(expr[i]) || (expr[i] == '$' && i + 1 < expr.length() && std::isalpha(expr[i + 1]))) {
            bool colAbsolute = false;
            bool rowAbsolute = false;
            std::string col;
            std::string rowStr;
            size_t j = i;

            // Check for absolute column reference
            if (expr[j] == '$') {
                colAbsolute = true;
                j++;
            }

            // Collect column letters
            while (j < expr.length() && std::isalpha(expr[j])) {
                col.push_back(expr[j]);
                ++j;
            }

            // Check for absolute row reference
            if (j < expr.length() && expr[j] == '$') {
                rowAbsolute = true;
                j++;
            }

            // Collect row number
            while (j < expr.length() && std::isdigit(expr[j])) {
                rowStr.push_back(expr[j]);
                ++j;
            }

            // Convert column to index and row to 0-based index
            int colIndex = letterToNumber(col);
            int row = std::stoi(rowStr) - 1;

            // Adjust row and column based on deltaRow and deltaCol if not absolute
            if (!rowAbsolute) {
                row += deltaRow;
            }
            if (!colAbsolute) {
                colIndex += deltaCol;
            }

            // Convert back to spreadsheet notation
            col = colAbsolute ? "$" + numberToLetters(colIndex) : numberToLetters(colIndex);
            rowStr = rowAbsolute ? "$" + std::to_string(row + 1) : std::to_string(row + 1);

            // Append adjusted reference to result
            result += col + rowStr;
            i = j; // Move index to the end of the number
        } else {
            // Append normal characters
            result.push_back(expr[i]);
            ++i;
        }
    }

    return result;
}

using ExpressionResult = std::variant<std::monostate, double, std::string>;

class ExprNode {
public:
    std::string strExpr;
    virtual ~ExprNode() = default;
    virtual ExpressionResult evaluate(const CSpreadsheet& context, int row, int col) const = 0;
    virtual std::shared_ptr<ExprNode> clone() const = 0;
};


class NumberNode : public ExprNode {
    ExpressionResult value;
public:
    explicit NumberNode(double val) : value(val) {}
    ExpressionResult evaluate(const CSpreadsheet& context, int row, int col) const override {
        return value;
    }
    std::shared_ptr<ExprNode> clone() const override {
        return std::make_shared<NumberNode>(*this);
    }
};

class StringNode : public ExprNode {
    ExpressionResult value;
public:
    explicit StringNode(std::string val) : value(val) {}
    ExpressionResult evaluate(const CSpreadsheet& context, int row, int col) const override {
        return value;
    }
    std::shared_ptr<ExprNode> clone() const override {
        return std::make_shared<StringNode>(*this);
    }
};

class ValReferenceNode : public ExprNode {
    bool hAbs;
    bool wAbs;
    int posH;
    int posW;
public:
    ValReferenceNode(bool hAbsolute, bool wAbsolute, int hPosition, int wPosition)
            : hAbs(hAbsolute), wAbs(wAbsolute), posH(hPosition), posW(wPosition) {}
    ExpressionResult evaluate(const CSpreadsheet& context, int row, int col) const override {
        globalCount++;
        if (globalCount > 60) {return CValue();}
        std::pair<int, int> pos;
        pos.first = hAbs ? posH : row + posH;
        pos.second = wAbs ? posW : col + posW;


        auto it = context.m_table.find(pos);
        if (it == context.m_table.end()) {
            return ExpressionResult(); // Return empty if no value found
        }

        // Handle different types of cell values
        const auto& cellValue = it->second;
        if (std::holds_alternative<double>(cellValue)) {
            return std::get<double>(cellValue);
        } else if (std::holds_alternative<std::string>(cellValue)) {
            return std::get<std::string>(cellValue);
        } else if (std::holds_alternative<std::shared_ptr<ExprNode>>(cellValue)) {
            // If the cell contains an expression, evaluate it recursively
            const std::shared_ptr<ExprNode> expr = std::get<std::shared_ptr<ExprNode>>(cellValue);
            return expr->evaluate(context, pos.first, pos.second);
        } else {
            return ExpressionResult(); // Return empty for unsupported or uninitialized types
        }

    }
    std::shared_ptr<ExprNode> clone() const override {
        return std::make_shared<ValReferenceNode>(*this);
    }

};

class BinaryOpNode : public ExprNode {
protected:
    std::shared_ptr<ExprNode> left, right;
public:
    BinaryOpNode(std::shared_ptr<ExprNode> l, std::shared_ptr<ExprNode> r)
            : left(std::move(l)), right(std::move(r)) {}
    virtual ~BinaryOpNode() = default;
};

class PowNode : public BinaryOpNode {
public:
    using BinaryOpNode::BinaryOpNode;
    ExpressionResult evaluate(const CSpreadsheet& context, int row, int col) const override {
        auto lval = left->evaluate(context, row, col);
        auto rval = right->evaluate(context, row, col);
        if (std::holds_alternative<double>(lval) && std::holds_alternative<double>(rval)) {
            return std::pow(std::get<double>(lval), std::get<double>(rval));
        }
        return ExpressionResult(); // Undefined if operands are not both doubles
    }
    std::shared_ptr<ExprNode> clone() const override {
        return std::make_shared<PowNode>(left->clone(), right->clone());
    }
};

class MulNode : public BinaryOpNode {
public:
    using BinaryOpNode::BinaryOpNode;
    ExpressionResult evaluate(const CSpreadsheet& context, int row, int col) const override {
        auto lval = left->evaluate(context, row, col);
        auto rval = right->evaluate(context, row, col);
        if (std::holds_alternative<double>(lval) && std::holds_alternative<double>(rval)) {
            return std::get<double>(lval) * std::get<double>(rval);
        }
        return ExpressionResult(); // Undefined if operands are not both doubles
    }
    std::shared_ptr<ExprNode> clone() const override {
        return std::make_shared<MulNode>(left->clone(), right->clone());
    }
};

class DivNode : public BinaryOpNode {
public:
    using BinaryOpNode::BinaryOpNode;
    ExpressionResult evaluate(const CSpreadsheet& context, int row, int col) const override {
        auto lval = left->evaluate(context, row, col);
        auto rval = right->evaluate(context, row, col);
        if (std::holds_alternative<double>(rval) && std::get<double>(rval) == 0) {
            return ExpressionResult(); // Division by zero yields undefined result
        }
        if (std::holds_alternative<double>(lval) && std::holds_alternative<double>(rval)) {
            return std::get<double>(lval) / std::get<double>(rval);
        }
        return ExpressionResult(); // Undefined if operands are not both doubles
    }
    std::shared_ptr<ExprNode> clone() const override {
        return std::make_shared<DivNode>(left->clone(), right->clone());
    }
};

class SubNode : public BinaryOpNode {
public:
    using BinaryOpNode::BinaryOpNode;
    ExpressionResult evaluate(const CSpreadsheet& context, int row, int col) const override {
        auto lval = left->evaluate(context, row, col);
        auto rval = right->evaluate(context, row, col);
        if (std::holds_alternative<double>(lval) && std::holds_alternative<double>(rval)) {
            return std::get<double>(lval) - std::get<double>(rval);
        }
        return ExpressionResult(); // Undefined if operands are not both doubles
    }
    std::shared_ptr<ExprNode> clone() const override {
        return std::make_shared<SubNode>(left->clone(), right->clone());
    }
};


class AddNode : public ExprNode {
    std::shared_ptr<ExprNode> left, right;
public:
    AddNode(std::shared_ptr<ExprNode> l, std::shared_ptr<ExprNode> r)
            : left(std::move(l)), right(std::move(r)) {}
    ExpressionResult evaluate(const CSpreadsheet& context, int row, int col) const override {
        auto lval = left->evaluate(context, row, col);
        auto rval = right->evaluate(context, row, col);
        if (std::holds_alternative<double>(lval) && std::holds_alternative<double>(rval)) {
            return std::get<double>(lval) + std::get<double>(rval);
        } else {
            std::string strRval;
            std::string strLval;
            if (std::holds_alternative<double>(lval)) {
                strLval = std::to_string(std::get<double>(lval));
            }
            if (std::holds_alternative<double>(rval)) {
                strRval = std::to_string(std::get<double>(rval));
            }
            if (std::holds_alternative<std::string>(lval) && std::holds_alternative<std::string>(rval)) {
                return std::get<std::string>(lval) + std::get<std::string>(rval);
            } else if (std::holds_alternative<std::string>(lval) && std::holds_alternative<double>(rval)) {
                return std::get<std::string>(lval) + strRval;
            }
            else if (std::holds_alternative<double>(lval) && std::holds_alternative<std::string>(rval)) {
                return strLval + std::get<std::string>(rval);
            } else {
                return ExpressionResult();
            }
        }
    }
    std::shared_ptr<ExprNode> clone() const override {
        return std::make_shared<AddNode>(left->clone(), right->clone());
    }
};


class NegNode : public ExprNode {
private:
    std::shared_ptr<ExprNode> operand;

public:
    NegNode(std::shared_ptr<ExprNode> op)
            : operand(std::move(op)) {}

    ExpressionResult evaluate(const CSpreadsheet& context, int row, int col) const override {
        auto val = operand->evaluate(context, row, col);
        if (std::holds_alternative<double>(val)) {
            return -std::get<double>(val);
        }
        return {}; // Undefined if operand is not a double
    }
    std::shared_ptr<ExprNode> clone() const override {
        return std::make_shared<NegNode>(operand->clone());
    }
};


class RelationalOpNode : public ExprNode {
public:
    std::shared_ptr<ExprNode> left, right;
    const CSpreadsheet& context; int row; int col;
public:
    RelationalOpNode(std::shared_ptr<ExprNode> l, std::shared_ptr<ExprNode> r, const CSpreadsheet& context, int row, int col)
            : left(std::move(l)), right(std::move(r)), context(context), row(row), col(col) {}
    template<typename Compare>
    ExpressionResult compareOperands(const std::shared_ptr<ExprNode>& left, const std::shared_ptr<ExprNode>& right, const CSpreadsheet& context, int row, int col, Compare comp) const;
    virtual ~RelationalOpNode() = default;
};

class EqNode : public RelationalOpNode {
public:
    using RelationalOpNode::RelationalOpNode;
    ExpressionResult evaluate(const CSpreadsheet& context, int row, int col) const override {
        return compareOperands(left, right, context, row, col, [](auto a, auto b) { return a == b; });
    }
    std::shared_ptr<ExprNode> clone() const override {
        return std::make_shared<EqNode>(left->clone(), right->clone(), context, row, col);
    }
};

class NeNode : public RelationalOpNode {
public:
    using RelationalOpNode::RelationalOpNode;
    ExpressionResult evaluate(const CSpreadsheet& context, int row, int col) const override {
        return compareOperands(left, right, context, row, col, [](auto a, auto b) { return a != b; });
    }
    std::shared_ptr<ExprNode> clone() const override {
        return std::make_shared<EqNode>(left->clone(), right->clone(), context, row, col);
    }
};

class LtNode : public RelationalOpNode {
public:
    using RelationalOpNode::RelationalOpNode;
    ExpressionResult evaluate(const CSpreadsheet& context, int row, int col) const override {
        return compareOperands(left, right, context, row, col, [](auto a, auto b) { return a < b; });
    }
    std::shared_ptr<ExprNode> clone() const override {
        return std::make_shared<EqNode>(left->clone(), right->clone(), context, row, col);
    }
};

class LeNode : public RelationalOpNode {
public:
    using RelationalOpNode::RelationalOpNode;
    ExpressionResult evaluate(const CSpreadsheet& context, int row, int col) const override {
        return compareOperands(left, right, context, row, col, [](auto a, auto b) { return a <= b; });
    }
    std::shared_ptr<ExprNode> clone() const override {
        return std::make_shared<EqNode>(left->clone(), right->clone(), context, row, col);
    }
};

class GtNode : public RelationalOpNode {
public:
    using RelationalOpNode::RelationalOpNode;
    ExpressionResult evaluate(const CSpreadsheet& context, int row, int col) const override {
        return compareOperands(left, right, context, row, col, [](auto a, auto b) { return a > b; });
    }
    std::shared_ptr<ExprNode> clone() const override {
        return std::make_shared<EqNode>(left->clone(), right->clone(), context, row, col);
    }
};

class GeNode : public RelationalOpNode {
public:
    using RelationalOpNode::RelationalOpNode;
    ExpressionResult evaluate(const CSpreadsheet& context, int row, int col) const override {
        return compareOperands(left, right, context, row, col, [](auto a, auto b) { return a >= b; });
    }
    std::shared_ptr<ExprNode> clone() const override {
        return std::make_shared<EqNode>(left->clone(), right->clone(), context, row, col);
    }
};

// Helper function to perform comparison and handle type checking
template<typename Compare>
ExpressionResult RelationalOpNode::compareOperands(const std::shared_ptr<ExprNode>& left, const std::shared_ptr<ExprNode>& right, const CSpreadsheet& context, int row, int col, Compare comp) const {
    auto lval = left->evaluate(context, row, col);
    auto rval = right->evaluate(context, row, col);

    if (std::holds_alternative<double>(lval) && std::holds_alternative<double>(rval)) {
        return comp(std::get<double>(lval), std::get<double>(rval)) ? 1.0 : 0.0;
    }
    if (std::holds_alternative<std::string>(lval) && std::holds_alternative<std::string>(rval)) {
        return comp(std::get<std::string>(lval), std::get<std::string>(rval)) ? 1.0 : 0.0;
    }
    return {}; // Return undefined if types do not match or operands are not comparable
}



class ASTBuilder : public CExprBuilder {
    int posH;
    int posW;
    const CSpreadsheet& context;

public:
    ASTBuilder(int r, int c, const CSpreadsheet& context) : posH(r), posW(c), context(context){}
    std::stack<std::shared_ptr<ExprNode>> stack;

    void opAdd() override {
        auto right = std::move(stack.top()); stack.pop();
        auto left = std::move(stack.top()); stack.pop();
        stack.push(std::make_shared<AddNode>(std::move(left), std::move(right)));
    }

    void opPow() override {
        auto right = std::move(stack.top()); stack.pop();
        auto left = std::move(stack.top()); stack.pop();
        stack.push(std::make_shared<PowNode>(std::move(left), std::move(right)));
    }

    void opMul() override {
        auto right = std::move(stack.top()); stack.pop();
        auto left = std::move(stack.top()); stack.pop();
        stack.push(std::make_shared<MulNode>(std::move(left), std::move(right)));
    }

    void opDiv() override {
        auto right = std::move(stack.top()); stack.pop();
        auto left = std::move(stack.top()); stack.pop();
        stack.push(std::make_shared<DivNode>(std::move(left), std::move(right)));
    }

    void opSub() override {
        auto right = std::move(stack.top()); stack.pop();
        auto left = std::move(stack.top()); stack.pop();
        stack.push(std::make_shared<SubNode>(std::move(left), std::move(right)));
    }

    void opNeg() override {
        auto operand = std::move(stack.top()); stack.pop();
        stack.push(std::make_shared<NegNode>(std::move(operand)));
    }


    void opEq() override {
        auto right = std::move(stack.top()); stack.pop();
        auto left = std::move(stack.top()); stack.pop();
        stack.push(std::make_shared<EqNode>(std::move(left), std::move(right), context, posH, posW));
    }

    void opNe() override {
        auto right = std::move(stack.top()); stack.pop();
        auto left = std::move(stack.top()); stack.pop();
        stack.push(std::make_shared<NeNode>(std::move(left), std::move(right), context, posH, posW));
    }

    void opLt() override {
        auto right = std::move(stack.top()); stack.pop();
        auto left = std::move(stack.top()); stack.pop();
        stack.push(std::make_shared<LtNode>(std::move(left), std::move(right), context, posH, posW));
    }

    void opLe() override {
        auto right = std::move(stack.top()); stack.pop();
        auto left = std::move(stack.top()); stack.pop();
        stack.push(std::make_shared<LeNode>(std::move(left), std::move(right), context, posH, posW));
    }

    void opGt() override {
        auto right = std::move(stack.top()); stack.pop();
        auto left = std::move(stack.top()); stack.pop();
        stack.push(std::make_shared<GtNode>(std::move(left), std::move(right), context, posH, posW));
    }

    void opGe() override {
        auto right = std::move(stack.top()); stack.pop();
        auto left = std::move(stack.top()); stack.pop();
        stack.push(std::make_shared<GeNode>(std::move(left), std::move(right), context, posH, posW));
    }


    void valNumber(double val) override {
        stack.push(std::make_shared<NumberNode>(val));
    }

    void valString(std::string val) override {
        stack.push(std::make_shared<StringNode>(val));
    }

    void valReference(std::string val) override {
        bool hAbs;
        bool wAbs;
        int posValH;
        int posValW;

        // Determine whether the reference is absolute or relative
        if (val[0] == '$') {
            wAbs = true;
            val.erase(0, 1); // Remove '$' from the beginning
        } else {
            wAbs = false;
        }

        // Check if there's a second '$' to determine absolute or relative in the vertical direction
        auto secondDollarPos = val.find('$', 1);
        if (secondDollarPos != std::string::npos) {
            hAbs = true;
            val.erase(secondDollarPos, 1); // Remove second '$'
        } else {
            hAbs = false;
        }

        // Parse row and column numbers
        std::string letters;
        int numbers;
        splitString(val, letters, numbers);
        if (hAbs) {
            posValH = numbers;
        } else {
            posValH = numbers - posH;
        }

        if (wAbs) {
            posValW = letterToNumber(letters);
        } else {
            posValW = letterToNumber(letters) - posW;
        }

        stack.push(std::make_shared<ValReferenceNode>(hAbs, wAbs, posValH, posValW)); // Implement ReferenceNode accordingly
    }

    void valRange ( std::string val ) override {}
    void funcCall ( std::string fnName, int paramCount ) override {}

    std::shared_ptr<ExprNode> getExpression() {
        return std::move(stack.top());
    }
};

void CSpreadsheet::copyRect(CPos dst, CPos src, int w, int h) {
    int srcRow = src.cPosHW.first;
    int srcCol = src.cPosHW.second;
    int dstRow = dst.cPosHW.first;
    int dstCol = dst.cPosHW.second;

    // Copy the source cells to a temporary map first to handle overlaps correctly
    std::map<std::pair<int, int>, cellValue> tempMap;

    for (int i = 0; i < h; ++i) {
        for (int j = 0; j < w; ++j) {
            std::pair<int, int> srcPos = {srcRow + i, srcCol + j};
            auto srcIt = m_table.find(srcPos);
            if (srcIt != m_table.end()) {
                const cellValue& srcVal = srcIt->second;
                // Check for expression and clone if necessary
                if (std::holds_alternative<std::shared_ptr<ExprNode>>(srcVal)) {
                    std::shared_ptr<ExprNode> expr = std::get<std::shared_ptr<ExprNode>>(srcVal);
                    std::string adjustedExpr = parseAndAdjustExpression(expr->strExpr, dstRow - srcRow, dstCol - srcCol);
                    // Clone and set the modified expression string
                    std::shared_ptr<ExprNode> clonedExpr = expr->clone();
                    clonedExpr->strExpr = adjustedExpr;
                    tempMap[{dstRow + i, dstCol + j}] = clonedExpr;
                } else {
                    tempMap[{dstRow + i, dstCol + j}] = srcVal; // Normal deep copy for non-expression types
                }
            }
        }
    }

    // Now write the copied content to the destination cells
    for (int i = 0; i < h; ++i) {
        for (int j = 0; j < w; ++j) {
            std::pair<int, int> dstPos = {dstRow + i, dstCol + j};
            auto tempIt = tempMap.find({dstRow + i, dstCol + j});
            if (tempIt != tempMap.end()) {
                m_table[dstPos] = tempIt->second;
            } else {
                m_table.erase(dstPos); // Clear cells in the destination that don't match the source rectangle
            }
        }
    }
}

bool CSpreadsheet::save(std::ostream &os) const {
    try {
        for (const auto& [key, val] : m_table) {
            // Write row and column
            os.write(reinterpret_cast<const char*>(&key.first), sizeof(key.first));
            os.write(reinterpret_cast<const char*>(&key.second), sizeof(key.second));

            // Write the type of the value (double, string, or expression)
            int type = val.index();
            os.write(reinterpret_cast<const char*>(&type), sizeof(type));

            if (type == 1) { // double
                double num = std::get<double>(val);
                os.write(reinterpret_cast<const char*>(&num), sizeof(num));
            } else if (type == 2) { // string
                const std::string& str = std::get<std::string>(val);
                size_t len = str.length();
                os.write(reinterpret_cast<const char*>(&len), sizeof(len)); // Write length of string
                os.write(str.data(), str.size()); // Write string data
            } else if (type == 3) { // expression (store as string)
                std::shared_ptr<ExprNode> expr = std::get<std::shared_ptr<ExprNode>>(val);
                const std::string& strExpr = expr->strExpr;
                size_t len = strExpr.length();
                os.write(reinterpret_cast<const char*>(&len), sizeof(len));
                os.write(strExpr.data(), strExpr.size());
            }
        }
        return true;
    } catch (...) {
        return false; // Handle any kind of write failure
    }
}

bool CSpreadsheet::load(std::istream &is) {
    try {
        m_table.clear();
        while (is.peek() != std::istream::traits_type::eof()) {
            std::pair<int, int> key;
            is.read(reinterpret_cast<char*>(&key.first), sizeof(key.first));
            is.read(reinterpret_cast<char*>(&key.second), sizeof(key.second));

            if (is.fail()) {
                m_table.clear(); // Clear the table to remove partial data
                return false; // Early exit on read failure
            }

            int type;
            is.read(reinterpret_cast<char*>(&type), sizeof(type));
            if (is.fail() || (type < 1 || type > 3)) {
                m_table.clear();
                return false; // Exit if the type is invalid
            }

            if (type == 1) { // double
                double num;
                is.read(reinterpret_cast<char*>(&num), sizeof(num));
                if (is.fail()) return false;
                m_table[key] = num;
            } else if (type == 2 || type == 3) { // string or expression
                size_t len;
                is.read(reinterpret_cast<char*>(&len), sizeof(len));
                if (is.fail() || len > 1000000) { // Arbitrary large length check
                    m_table.clear();
                    return false; // Prevent buffer overflow or invalid length
                }
                std::string str(len, '\0');
                is.read(str.data(), len);
                if (is.fail()) {
                    m_table.clear();
                    return false; // If read fails, cleanup and exit
                }
                if (type == 3) {
                    setCell(CPos(key.first, key.second), str); // Assumes setCell can handle expressions correctly
                } else {
                    m_table[key] = str;
                }
            }
        }
        return !is.fail();
    } catch (...) {
        m_table.clear(); // Clear any partial data on exceptions
        return false; // Return failure on exception
    }
}

bool CSpreadsheet::setCell (CPos pos, std::string contents) {
    int row = pos.cPosHW.first;
    int col = pos.cPosHW.second;

    try {
        size_t idx;
        double numValue = std::stod(contents, &idx);
        if (idx == contents.size()) { // Entire string was successfully converted to a number
            m_table[pos.cPosHW] = numValue;
            return true;
        }
    } catch (...) {
        // Ignore
    }

    if (contents[0] == '=') {
        ASTBuilder builder(row, col, *this);
        parseExpression(contents, builder); // Assume this parses and builds the AST
        auto expr = builder.getExpression();
        expr->strExpr = contents;
        m_table[pos.cPosHW] = expr;
    } else {
        m_table[pos.cPosHW] = contents;
    }

    return true;
}

CValue CSpreadsheet::getValue (CPos pos) {
    auto it = m_table.find(pos.cPosHW);
    CValue result;
    globalCount++;

    if (globalCount > 60) {return CValue();}

    if (it == m_table.end()) {
        result = {};
    } else if (std::holds_alternative<double>(m_table[pos.cPosHW])) {
        result = std::get<double>(it->second);
    } else if (std::holds_alternative<std::string>(m_table[pos.cPosHW])){
        result = std::get<std::string>(it->second);
    } else if (std::holds_alternative<std::shared_ptr<ExprNode>>(m_table[pos.cPosHW])) {
        auto& expr = std::get<std::shared_ptr<ExprNode>>(m_table[pos.cPosHW]);
        if (expr) { // Check if the shared_ptr actually points to an object
            ExpressionResult resultExp = expr->evaluate(*this, pos.cPosHW.first, pos.cPosHW.second);
            if (std::holds_alternative<double>(resultExp)) {
                result = std::get<double>(resultExp);
            } else if (std::holds_alternative<std::string>(resultExp)) {
                result = std::get<std::string>(resultExp);
            }
        }
    }
    globalCount = 0;
    return result;
}

#ifndef __PROGTEST__


bool                                   valueMatch                              ( const CValue                        & r,
                                                                                 const CValue                        & s )

{
    if ( r . index () != s . index () )
        return false;
    if ( r . index () == 0 )
        return true;
    if ( r . index () == 2 )
        return std::get<std::string> ( r ) == std::get<std::string> ( s );
    if ( std::isnan ( std::get<double> ( r ) ) && std::isnan ( std::get<double> ( s ) ) )
        return true;
    if ( std::isinf ( std::get<double> ( r ) ) && std::isinf ( std::get<double> ( s ) ) )
        return ( std::get<double> ( r ) < 0 && std::get<double> ( s ) < 0 )
               || ( std::get<double> ( r ) > 0 && std::get<double> ( s ) > 0 );
    return fabs ( std::get<double> ( r ) - std::get<double> ( s ) ) <= 1e8 * DBL_EPSILON * fabs ( std::get<double> ( r ) );
}
#include "tests.h"
int main ()
{
    CSpreadsheet ss;
    ss.setCell(CPos("A1"), "55e0");
    CValue value = ss.getValue(CPos("A1"));
    assert(valueMatch(value, CValue(55.0)));

    CSpreadsheet x;
    assert(x.setCell(CPos("A1"), "ahoj"));
    assert(x.setCell(CPos("A2"), " svete"));
    assert(x.setCell(CPos("A3"), "=A1+A2"));
    assert(valueMatch(x.getValue(CPos("A3")), CValue("ahoj svete")));
    assert(x.setCell(CPos("A1"), "ahoj"));
    assert(x.setCell(CPos("A2"), "3"));
    assert(valueMatch(x.getValue(CPos("A3")), CValue("ahoj3.000000")));

    ss.setCell(CPos("A1"), "=A2");
    ss.setCell(CPos("A2"), "=A1");
    CValue value1 = ss.getValue(CPos("A1"));
    assert(valueMatch(value1, CValue()));

    /*CSpreadsheet x0, x1;
    std::ostringstream oss;
    std::istringstream iss;
    std::string data;
    assert ( x0 . setCell ( CPos ( "A1" ), "10" ) );
    assert ( x0 . setCell ( CPos ( "A2" ), "20.5" ) );
    assert ( x0 . setCell ( CPos ( "A3" ), "3e1" ) );
    assert ( x0 . setCell ( CPos ( "A4" ), "=40" ) );
    assert ( x0 . setCell ( CPos ( "A5" ), "=5e+1" ) );
    assert ( x0 . setCell ( CPos ( "A6" ), "raw text with any characters, including a quote \" or a newline\n" ) );
    assert ( x0 . setCell ( CPos ( "A7" ), "=\"quoted string, quotes must be doubled: \"\". Moreover, backslashes are needed for C++.\"" ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "A1" ) ), CValue ( 10.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "A2" ) ), CValue ( 20.5 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "A3" ) ), CValue ( 30.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "A4" ) ), CValue ( 40.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "A5" ) ), CValue ( 50.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "A6" ) ), CValue ( "raw text with any characters, including a quote \" or a newline\n" ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "A7" ) ), CValue ( "quoted string, quotes must be doubled: \". Moreover, backslashes are needed for C++." ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "A8" ) ), CValue() ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "AAAA9999" ) ), CValue() ) );
    assert ( x0 . setCell ( CPos ( "B1" ), "=A1+A2*A3" ) );
    assert ( x0 . setCell ( CPos ( "B2" ), "= -A1 ^ 2 - A2 / 2   " ) );
    assert ( x0 . setCell ( CPos ( "B3" ), "= 2 ^ $A$1" ) );
    assert ( x0 . setCell ( CPos ( "B4" ), "=($A1+A$2)^2" ) );
    assert ( x0 . setCell ( CPos ( "B5" ), "=B1+B2+B3+B4" ) );
    assert ( x0 . setCell ( CPos ( "B6" ), "=B1+B2+B3+B4+B5" ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "B1" ) ), CValue ( 625.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "B2" ) ), CValue ( -110.25 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "B3" ) ), CValue ( 1024.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "B4" ) ), CValue ( 930.25 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "B5" ) ), CValue ( 2469.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "B6" ) ), CValue ( 4938.0 ) ) );
    assert ( x0 . setCell ( CPos ( "A1" ), "12" ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "B1" ) ), CValue ( 627.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "B2" ) ), CValue ( -154.25 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "B3" ) ), CValue ( 4096.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "B4" ) ), CValue ( 1056.25 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "B5" ) ), CValue ( 5625.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "B6" ) ), CValue ( 11250.0 ) ) );
    x1 = x0;
    assert ( x0 . setCell ( CPos ( "A2" ), "100" ) );
    assert ( x1 . setCell ( CPos ( "A2" ), "=A3+A5+A4" ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "B1" ) ), CValue ( 3012.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "B2" ) ), CValue ( -194.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "B3" ) ), CValue ( 4096.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "B4" ) ), CValue ( 12544.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "B5" ) ), CValue ( 19458.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "B6" ) ), CValue ( 38916.0 ) ) );
    assert ( valueMatch ( x1 . getValue ( CPos ( "B1" ) ), CValue ( 3612.0 ) ) );
    assert ( valueMatch ( x1 . getValue ( CPos ( "B2" ) ), CValue ( -204.0 ) ) );
    assert ( valueMatch ( x1 . getValue ( CPos ( "B3" ) ), CValue ( 4096.0 ) ) );
    assert ( valueMatch ( x1 . getValue ( CPos ( "B4" ) ), CValue ( 17424.0 ) ) );
    assert ( valueMatch ( x1 . getValue ( CPos ( "B5" ) ), CValue ( 24928.0 ) ) );
    assert ( valueMatch ( x1 . getValue ( CPos ( "B6" ) ), CValue ( 49856.0 ) ) );
    oss . clear ();
    oss . str ( "" );
    assert ( x0 . save ( oss ) );
    data = oss . str ();
    iss . clear ();
    iss . str ( data );
    assert ( x1 . load ( iss ) );
    assert ( valueMatch ( x1 . getValue ( CPos ( "B1" ) ), CValue ( 3012.0 ) ) );
    assert ( valueMatch ( x1 . getValue ( CPos ( "B2" ) ), CValue ( -194.0 ) ) );
    assert ( valueMatch ( x1 . getValue ( CPos ( "B3" ) ), CValue ( 4096.0 ) ) );
    assert ( valueMatch ( x1 . getValue ( CPos ( "B4" ) ), CValue ( 12544.0 ) ) );
    assert ( valueMatch ( x1 . getValue ( CPos ( "B5" ) ), CValue ( 19458.0 ) ) );
    assert ( valueMatch ( x1 . getValue ( CPos ( "B6" ) ), CValue ( 38916.0 ) ) );
    assert ( x0 . setCell ( CPos ( "A3" ), "4e1" ) );
    assert ( valueMatch ( x1 . getValue ( CPos ( "B1" ) ), CValue ( 3012.0 ) ) );
    assert ( valueMatch ( x1 . getValue ( CPos ( "B2" ) ), CValue ( -194.0 ) ) );
    assert ( valueMatch ( x1 . getValue ( CPos ( "B3" ) ), CValue ( 4096.0 ) ) );
    assert ( valueMatch ( x1 . getValue ( CPos ( "B4" ) ), CValue ( 12544.0 ) ) );
    assert ( valueMatch ( x1 . getValue ( CPos ( "B5" ) ), CValue ( 19458.0 ) ) );
    assert ( valueMatch ( x1 . getValue ( CPos ( "B6" ) ), CValue ( 38916.0 ) ) );
    oss . clear ();
    oss . str ( "" );
    assert ( x0 . save ( oss ) );
    data = oss . str ();
    for ( size_t i = 0; i < std::min<size_t> ( data . length (), 10 ); i ++ )
      data[i] ^=0x5a;
    iss . clear ();
    iss . str ( data );
    assert ( ! x1 . load ( iss ) );
    assert ( x0 . setCell ( CPos ( "D0" ), "10" ) );
    assert ( x0 . setCell ( CPos ( "D1" ), "20" ) );
    assert ( x0 . setCell ( CPos ( "D2" ), "30" ) );
    assert ( x0 . setCell ( CPos ( "D3" ), "40" ) );
    assert ( x0 . setCell ( CPos ( "D4" ), "50" ) );
    assert ( x0 . setCell ( CPos ( "E0" ), "60" ) );
    assert ( x0 . setCell ( CPos ( "E1" ), "70" ) );
    assert ( x0 . setCell ( CPos ( "E2" ), "80" ) );
    assert ( x0 . setCell ( CPos ( "E3" ), "90" ) );
    assert ( x0 . setCell ( CPos ( "E4" ), "100" ) );
    assert ( x0 . setCell ( CPos ( "F10" ), "=D0+5" ) );
    assert ( x0 . setCell ( CPos ( "F11" ), "=$D0+5" ) );
    assert ( x0 . setCell ( CPos ( "F12" ), "=D$0+5" ) );
    assert ( x0 . setCell ( CPos ( "F13" ), "=$D$0+5" ) );
      auto ahoj = x0 . getValue ( CPos ( "H12" ) );
    x0 . copyRect ( CPos ( "G11" ), CPos ( "F10" ), 1, 4 );
      ahoj = x0 . getValue ( CPos ( "H12" ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "F10" ) ), CValue ( 15.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "F11" ) ), CValue ( 15.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "F12" ) ), CValue ( 15.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "F13" ) ), CValue ( 15.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "F14" ) ), CValue() ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "G10" ) ), CValue() ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "G11" ) ), CValue ( 75.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "G12" ) ), CValue ( 25.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "G13" ) ), CValue ( 65.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "G14" ) ), CValue ( 15.0 ) ) );
    x0 . copyRect ( CPos ( "G11" ), CPos ( "F10" ), 2, 4 );
      ahoj = x0 . getValue ( CPos ( "H12" ) );//5
    assert ( valueMatch ( x0 . getValue ( CPos ( "F10" ) ), CValue ( 15.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "F11" ) ), CValue ( 15.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "F12" ) ), CValue ( 15.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "F13" ) ), CValue ( 15.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "F14" ) ), CValue() ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "G10" ) ), CValue() ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "G11" ) ), CValue ( 75.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "G12" ) ), CValue ( 25.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "G13" ) ), CValue ( 65.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "G14" ) ), CValue ( 15.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "H10" ) ), CValue() ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "H11" ) ), CValue() ) );
      ahoj = x0 . getValue ( CPos ( "H12" ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "H12" ) ), CValue() ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "H13" ) ), CValue ( 35.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "H14" ) ), CValue() ) );
    assert ( x0 . setCell ( CPos ( "F0" ), "-27" ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "H14" ) ), CValue ( -22.0 ) ) );
    x0 . copyRect ( CPos ( "H12" ), CPos ( "H13" ), 1, 2 );
    assert ( valueMatch ( x0 . getValue ( CPos ( "H12" ) ), CValue ( 25.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "H13" ) ), CValue ( -22.0 ) ) );
    assert ( valueMatch ( x0 . getValue ( CPos ( "H14" ) ), CValue ( -22.0 ) ) );*/
    runTests();
    return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
