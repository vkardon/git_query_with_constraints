//
// constraints.cpp
//
#include <iostream>         // std::cout
#include <strings.h>        // strncasecmp
#include <string.h>         // strchr
#include "constraints.h"
#include "logger.h"


bool Constraints::Parse(const char* constraintsStr)
{
    constraintsTree.reset();
    err.clear();
    constraintsStrIn = constraintsStr;

    std::unique_ptr<Node> empty;
    constraintsTree.reset(Parse(constraintsStr, empty));
    return (bool)constraintsTree;
}

Constraints::Node* Constraints::ParseOperand(const char* constraintsStr, size_t& len)
{
    std::string prefix = std::string(__func__) + "[" + std::to_string(depth) + "]: ";

    DEBUGMSG(prefix << "Constraints is '" << (constraintsStr ? constraintsStr : "null") << "'");

    if(constraintsStr == nullptr || *constraintsStr == '\0')
    {
        err = prefix + "Invalid (empty) constraintsStr";
        return nullptr;
    }

    // Skip trailing whitespaces
    const char* ptr = constraintsStr;
    while(isspace(*ptr))
        ptr++;

    // Single operand example:
    //   Name == Value
    //   Name != Value
    //   "Name AAA" == "Value BBB"
    //
    // Sub-constraintsStr example:
    //   (NameSort == 000Test AND IsGroup != 1)

    Node* operand = nullptr;

    // Parse singe operand or sub-constraintsStr.
    if(*ptr == '(')
    {
        // Find the end of sub-constraintsStr
        ptr++;
        const char* end = strchr(ptr, ')');
        if(end == nullptr)
        {
            err = prefix + "Misformed sub-constraintsStr - missing closing ')'";
            return nullptr;
        }
        else if(ptr == end)
        {
            err = prefix + "Misformed (empty) sub-constraintsStr";
            return nullptr;
        }

        std::string subConstraints(ptr, end - ptr);
        ptr = ++end; // The next character after ')'

        DEBUGMSG(prefix << "Begin parsing sub-constraintsStr '" << subConstraints << "'");

        // Parse sub-constraintsStr
        std::unique_ptr<Node> empty;
        operand = Parse(subConstraints.c_str(), empty);
        if(operand == nullptr)
            return nullptr; // Error is reported by above Parse() call

        DEBUGMSG(prefix << "End parsing sub-constraintsStr");
    }
    else
    {
        // Parse single operand
        size_t parsedLen = 0;

        // Read Name argument
        std::string name;
        if(!ParseOperandName(ptr, parsedLen, name))
            return nullptr; // Error is reported by above ParseOperandName() call
        ptr += parsedLen;

        // Check if this is "IN (aaa, bbb, ccc)" Value operator
        if(strncasecmp(ptr, "IN", 2) == 0)
        {
            ptr += 2;
            std::string subConstraints;
            if(!BuildValuesForOperatorIN(ptr, parsedLen, name, subConstraints))
                return nullptr; // Error is reported by above BuildValuesForOperatorIN() call
            ptr += parsedLen;

            DEBUGMSG(prefix << "Begin parsing operator IN '" << subConstraints << "'");

            // Parse sub-constraintsStr
            std::unique_ptr<Node> empty;
            operand = Parse(subConstraints.c_str(), empty);
            if(operand == nullptr)
                return nullptr; // Error is reported by above Parse() call

            DEBUGMSG(prefix << "End parsing operator IN");
        }
        else
        {
            // Parse operator: ==, !=, <=, >=, <, >
            Node::Operator oper = ParseOperandOperator(ptr, parsedLen);
            if(oper == Node::NOOP)
                return nullptr; // Error is reported by above ParseLogicalOperator() call
            ptr += parsedLen;

            // Read Value argument
            parsedLen = 0;
            std::string value;
            if(!ParseOperandValue(ptr, parsedLen, value))
                return nullptr; // Error is reported by above ParseOperandValue() call
            ptr += parsedLen;

            // Create Element operand
            Element* elem = new Element(name, value, oper);
            operand = elem;

            DEBUGMSG(prefix << "Operand is '" << name << "' " << elem->GetOperatorStr()  << " '" << value << "'");
        }
    }

    // Skip remaining whitespaces
    while(isspace(*ptr))
        ptr++;

    // Set size of parsed constraintsStr
    len = (ptr - constraintsStr);
    return operand;
}

bool Constraints::ParseOperandName(const char* constraintsStr, size_t& len, std::string& name)
{
    std::string prefix = std::string(__func__) + "[" + std::to_string(depth) + "]: ";

    DEBUGMSG(prefix << "Constraints is '" << (constraintsStr ? constraintsStr : "null") << "'");

    if(constraintsStr == nullptr || *constraintsStr == '\0')
    {
        err = prefix + "Invalid (empty) constraintsStr";
        return false;
    }

    // Skip trailing whitespaces
    const char* ptr = constraintsStr;
    while(isspace(*ptr))
        ptr++;

    // Read quated or unquated Name argument (until first =, !, <, > or space)
    bool isQuated = (*ptr == '"');
    const char* begin = (isQuated ? ++ptr : ptr);

    while(*ptr != '\0' &&
          *ptr != '=' && *ptr != '!' && *ptr != '<' && *ptr != '>' &&
          (isQuated ? *ptr != '"' : isspace(*ptr) == 0))
    {
        ptr++;
    }

    if(isQuated && *ptr != '"')
    {
        err = prefix + "Invalid Name argument (no closing quote) in '" + (begin - 1) + "'";
        return false;
    }

    if(begin == ptr)
    {
        err = prefix + "Invalid (missing) Name argument in '" + constraintsStr + "'";
        return false;
    }

    name.assign(begin, ptr - begin);

    if(isQuated)
        ptr++;

    // Skip remaining whitespaces
    while(isspace(*ptr))
        ptr++;

    // Set size of parsed constraintsStr
    len = (ptr - constraintsStr);
    return true;
}

bool Constraints::ParseOperandValue(const char* constraintsStr, size_t& len,
        std::string& value, const char* terminators/*=nullptr*/)
{
    std::string prefix = std::string(__func__) + "[" + std::to_string(depth) + "]: ";

    DEBUGMSG(prefix << "Constraints is '" << (constraintsStr ? constraintsStr : "null") << "'");

    if(constraintsStr == nullptr || *constraintsStr == '\0')
    {
        err = prefix + "Invalid (empty) constraintsStr";
        return false;
    }

    // Skip trailing whitespaces
    const char* ptr = constraintsStr;
    while(isspace(*ptr))
        ptr++;

    // Read quated or unquated Value argument
    bool isQuated = (*ptr == '"');
    const char* begin = (isQuated ? ++ptr : ptr);

    // Quated value can have any characters.
    // Unquated terminates on space or any of terminators characters.
    if(isQuated)
    {
        // Parse quated value
        while(*ptr != '\0' && *ptr != '"')
            ptr++;

        if(*ptr != '"')
        {
            //err = prefix + "Invalid Value argument (no close quote) in '" + constraintsStr + "'";
            err = prefix + "Invalid Value argument (no closing quote) in '" + (begin - 1) + "'";
            return false;
        }
    }
    else
    {
        // Parse unquated value
        while(*ptr != '\0' && isspace(*ptr) == 0 &&
                (terminators == nullptr || strchr(terminators, *ptr) == 0))
        {
            ptr++;
        }
    }

    if(begin == ptr)
    {
        err = prefix + "Invalid (empty) Value argument in '" + constraintsStr + "'";
        return false;
    }

    value.assign(begin, ptr - begin);
    begin = nullptr;

    if(isQuated)
        ptr++;

    // Skip remaining whitespaces
    while(isspace(*ptr))
        ptr++;

    // Set size of parsed constraintsStr
    len = (ptr - constraintsStr);
    return true;
}

Constraints::Node::Operator Constraints::ParseOperandOperator(const char* constraintsStr, size_t& len)
{
    std::string prefix = std::string(__func__) + "[" + std::to_string(depth) + "]: ";

    DEBUGMSG(prefix << "Constraints is '" << (constraintsStr ? constraintsStr : "null") << "'");

    if(constraintsStr == nullptr || *constraintsStr == '\0')
    {
        err = prefix + "Invalid (empty) constraintsStr";
        return Node::NOOP;
    }

    // Skip whitespaces
    const char* ptr = constraintsStr;
    while(isspace(*ptr))
        ptr++;

    // Supported operators: ==, !=, <=, >=, <, >
    Node::Operator oper = Node::NOOP;

    if(*(ptr + 1) == '=')
    {
        if(*ptr == '=') // ==
        {
            oper = Node::EQ;
            ptr += 2;
        }
        else if(*ptr == '!') // !=
        {
            oper = Node::NE;
            ptr += 2;
        }
        else if(*ptr == '<') // <=
        {
            oper = Node::LE;
            ptr += 2;
        }
        else if(*ptr == '>') // >=
        {
            oper = Node::GE;
            ptr += 2;
        }
    }
    else if(*ptr == '<')
    {
        oper = Node::LT;
        ptr++;
    }
    else if(*ptr == '>')
    {
        oper = Node::GT;
        ptr++;
    }
    else
    {
        //err = prefix + "Invalid Operator in '" + constraintsStr + "'";
        err = prefix + "Invalid Operator in '" + ptr + "'";
        return Node::NOOP;
    }

    // Skip remaining whitespaces
    while(isspace(*ptr))
        ptr++;

    // Set size of parsed constraintsStr
    len = (ptr - constraintsStr);
    return oper;
}

bool Constraints::BuildValuesForOperatorIN(const char* constraintsStr, size_t& len,
        const std::string& name, std::string& subConstraints)
{
    std::string prefix = std::string(__func__) + "[" + std::to_string(depth) + "]: ";

    DEBUGMSG(prefix << "Constraints is '" << (constraintsStr ? constraintsStr : "null") << "'");

    if(constraintsStr == nullptr || *constraintsStr == '\0')
    {
        err = prefix + "Invalid (empty) constraintsStr";
        return false;
    }

    // Skip trailing whitespaces
    const char* ptr = constraintsStr;
    while(isspace(*ptr))
        ptr++;

    if(*ptr != '(')
    {
        err = prefix + "Misformed IN operator - missing beginning ')'";
        return false;
    }

    // Find the end of sub-constraintsStr
    ptr++;
    const char* end = strchr(ptr, ')');
    if(end == nullptr)
    {
        err = prefix + "Misformed IN operator - missing closing ')'";
        return false;
    }
    else if(ptr == end)
    {
        err = prefix + "Misformed (empty) IN operator";
        return false;
    }

    size_t parsedLen = 0;
    std::string value;

    while(ptr < end)
    {
        // Stop reading unquated value on comma or ')'
        if(!ParseOperandValue(ptr, parsedLen, value, ",)"))
            return false; // Error is reported by above ParseOperandValue() call

        ptr += parsedLen;
        subConstraints += ('"' + name + "\" == \"" + value + '"');

        // We should point to either comma or ')'
        if(*ptr == ',')
        {
            subConstraints += " OR ";
        }
        else if(*ptr != ')')
        {
            err = prefix + "Misformed IN operator around '" + (ptr - parsedLen) + "'";
            return false;
        }

        ptr++;
    }

    // Skip remaining whitespaces
    while(isspace(*ptr))
        ptr++;

    // Set size of parsed constraintsStr
    len = (ptr - constraintsStr);
    return true;
}

Constraints::Node::Operator Constraints::ParseLogicalOperator(const char* constraintsStr, size_t& len)
{
    std::string prefix = std::string(__func__) + "[" + std::to_string(depth) + "]: ";

    DEBUGMSG(prefix << "Constraints is '" << (constraintsStr ? constraintsStr : "null") << "'");

    if(constraintsStr == nullptr || *constraintsStr == '\0')
    {
        err = prefix + "Invalid (empty) constraintsStr";
        return Node::NOOP;
    }

    // Skip whitespaces
    const char* ptr = constraintsStr;
    while(isspace(*ptr))
        ptr++;

    // Supported operators: AND, OR
    Node::Operator oper = Node::NOOP;

    if(strncasecmp(ptr, "AND", 3) == 0 && (isspace(ptr[3]) || ptr[3] == '\0'))
    {
        ptr += 3;
        oper = Node::AND;
        DEBUGMSG(prefix << "Operator is 'AND'");
    }
    else if(strncasecmp(ptr, "OR", 2) == 0 && (isspace(ptr[2]) || ptr[2] == '\0'))
    {
        ptr += 2;
        oper = Node::OR;
        DEBUGMSG(prefix << "Operator is 'OR'");
    }
    else
    {
        //err = prefix + "Invalid Operator in '" + constraintsStr + "'";
        err = prefix + "Invalid Operator in '" + ptr + "'";
        return Node::NOOP;
    }

    // Skip remaining whitespaces
    while(isspace(*ptr))
        ptr++;

    // Set size of parsed constraintsStr
    len = (ptr - constraintsStr);
    return oper;
}

Constraints::Node* Constraints::Parse(const char* constraintsStr, std::unique_ptr<Node>& node)
{
    depth++;
    std::string prefix = std::string(__func__) + "[" + std::to_string(depth) + "]: ";

    DEBUGMSG(prefix << "Constraints is '" << (constraintsStrStr ? constraintsStrStr : "null") << "'");

    if(constraintsStr == nullptr)
    {
        err = "Invalid (empty) constraintsStr";
        return nullptr;
    }

    // Skip whitespaces
    const char* ptr = constraintsStr;
    while(isspace(*ptr))
        ptr++;

    // Do we have first operand?
    std::unique_ptr<Node> operand1;
    size_t len = 0;

    if(node.get() != nullptr)
    {
        operand1 = std::move(node);
    }
    else if(*ptr != '\0')
    {
        // Parse first operand. It could be singe operand or sub-constraintsStr.
        operand1.reset(ParseOperand(ptr, len));
        if(operand1.get() == nullptr)
        {
            err.insert(0, prefix + "Failed to parse first operand, ");
            return nullptr;
        }
        operand1->SetConstraints(ptr, len);
        ptr += len;
    }

    // Do we have more constraintsStr to parse?
    Node* root = nullptr;

    if(*ptr == '\0')
    {
        // We are done
        root = operand1.release();
    }
    else
    {
        // Parse AND or OR operator
        Node::Operator oper = ParseLogicalOperator(ptr, len);
        if(oper == Node::NOOP)
        {
            err.insert(0, prefix + "Failed to parse operator, ");
            return nullptr;
        }
        ptr += len;

        // Parse second operand. It could be single operand or sub-constraintsStr.
        Node* operand2 = ParseOperand(ptr, len);
        if(operand2 == nullptr)
        {
            err.insert(0, prefix + "Failed to parse operand, ");
            return nullptr;
        }
        operand2->SetConstraints(ptr, len);
        ptr += len;

        Group* group = new Group(operand1.release(), operand2, oper);

        std::unique_ptr<Node> newNode(group);
        root = Parse(ptr, newNode);
    }

    // We are done
    DEBUGMSG(prefix << "Parsing complete");
    depth--;
    return root;
}

std::ostream& Constraints::Dump(std::ostream& os, const Node* node)
{
    if(node == nullptr)
        return os;

    if(node == constraintsTree.get())
    {
        depth = 0;
        os << __func__ << ": Constraints: '" << constraintsStrIn << "'" << std::endl;
    }

    depth++;
    os << __func__ << "[" << depth << "]: " << node << ' ' << std::string((depth-1) * 3, '.');

    Node::Type type = node->GetType();

    if(type == Node::GROUP)
    {
        Group* group = (Group*)node;
        group->Dump(os) << std::endl;

        // Recurse on the first child
        Dump(os, group->GetLChild());

        // Recurse on the second child
        Dump(os, group->GetRChild());
    }
    else if(type == Node::ELEMENT)
    {
        Element* elem = (Element*)node;
        elem->Dump(os) << std::endl;
    }
    else
    {
        os << "Invalid logical node (type " << type << ')' << std::endl;
    }

    depth--;
    return os;
}

const std::string& Constraints::GetOperatorStr(Node::Operator operIn)
{
    thread_local static std::string operStr;

    operStr = (operIn == Node::EQ        ? "=="        :
               operIn == Node::NE        ? "!="        :
               operIn == Node::LT        ? "<"         :
               operIn == Node::LE        ? "<="        :
               operIn == Node::GT        ? ">"         :
               operIn == Node::GE        ? ">="        :
               operIn == Node::AND       ? "AND"       :
               operIn == Node::OR        ? "OR"        :
            /* operIn == Node::ISNOTNULL ? "ISNOTNULL" : */
            /* operIn == Node::ISNULL    ? "ISNULL"    : */ "UNKNOWN (" + std::to_string(operIn) + ")");

    return operStr;
}

