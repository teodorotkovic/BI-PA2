//
// Created by HP on 5/6/2024.
//

#ifndef VELKA_ULOHA_EXPRESSIONBUILDERAST_H
#define VELKA_ULOHA_EXPRESSIONBUILDERAST_H


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

#endif //VELKA_ULOHA_EXPRESSIONBUILDERAST_Hq
