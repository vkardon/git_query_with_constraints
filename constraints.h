//
// constraints.h
//
#ifndef __CONSTRAINTS_H__
#define __CONSTRAINTS_H__

#include <iostream>         // std::cout
#include <memory>           // std::unique_ptr
#include <string>
#include <variant>

//
// Class Value
//
class Value
{
public:
    Value(const std::string& valueStr) { *this = valueStr; }
    Value() = default;
    ~Value() = default;

    Value& operator=(const std::string& valueStr)
    {
        // If the _value is numeric, then treat is a number
        bool isNumeric = false;
        for(const char c : valueStr)
        {
            if(!(isNumeric = isdigit(c)))
                break;
        }

        if(isNumeric)
            value = std::stoi(valueStr);
        else
            value = valueStr;

        return *this;
    }

    bool operator==(const Value& valueIn) const { return value == valueIn.value; }
    bool operator!=(const Value& valueIn) const { return value != valueIn.value; }
    bool operator<(const Value& valueIn) const { return value < valueIn.value; }
    bool operator<=(const Value& valueIn) const { return value <= valueIn.value; }
    bool operator>(const Value& valueIn) const { return value > valueIn.value; }
    bool operator>=(const Value& valueIn) const { return value >= valueIn.value; }

    // Diagnostic
    bool IsString() const { return std::holds_alternative<std::string>(value); }

    std::ostream& Dump(std::ostream& os) const
    {
        if(IsString())
            return os << "'" << std::get<std::string>(value) << "'";
        else
            return os << std::get<int>(value);
    }

private:
    std::variant<std::string, int> value;

    friend std::ostream& operator<<(std::ostream& os, const Value& val);
};

inline std::ostream& operator<<(std::ostream& os, const Value& val) { return val.Dump(os); }

//
// Class Constraints
//
class Constraints
{
    class Node
    {
    public:
        enum Type : char
        {
            UNKNOWN=0, NODE, ELEMENT, GROUP
        };

        enum Operator : char
        {
            NOOP=0,
            EQ,        // ==
            NE,        // !=
            LT,        // <
            LE,        // <=
            GT,        // >
            GE,        // >=
            ISNOTNULL, // TODO
            ISNULL,    // TODO
            AND,       // AND
            OR         // OR
        };

        Node(Type typeIn, Node::Operator operIn) : type(typeIn), oper(operIn) {}
        virtual ~Node() {}

        Type GetType() const { return type; }
        Operator GetOperator() const { return oper; }

        // Diagnostic
        const std::string& GetConstraints() const { return constraintsStr; }
        virtual void SetConstraints(const char* ptr, size_t size) { /*For child to override*/ }

    protected:
        Type type{UNKNOWN};
        Operator oper{NOOP};
        std::string constraintsStr; // Only used for Diagnostic
    };
    // End of class Node

    class Element : public Node
    {
    public:
        Element(const std::string& nameIn, const std::string& valueIn, Node::Operator operIn)
            : Node(ELEMENT, operIn), name(nameIn), value(valueIn) { refCount++; }
        virtual ~Element() { refCount--; }

        const std::string& GetName() const { return name; }
        const Value& GetValue() const { return value; }

        // Diagnostic
        inline static int refCount{0};
        static int GetRefCount() { return refCount; }

        virtual void SetConstraints(const char* ptr, size_t size) override
        {
            constraintsStr = '{' + std::string(ptr, size) + '}';
        }

        std::ostream& Dump(std::ostream& os)
        {
            return os << "Element: '" << name << "' " << GetOperatorStr(GetOperator()) << " " << value << " " << GetConstraints();
        }

    private:
        std::string name;
        Value value;
    };
    // End of class Element

    class Group : public Node
    {
    public:
        Group(Node* lChildIn, Node* rChildIn, Node::Operator operIn)
            : Node(GROUP, operIn), lChild(lChildIn), rChild(rChildIn)
        {
            refCount++;
            constraintsStr = '[' + lChild->GetConstraints() + ' ' + GetOperatorStr(GetOperator()) + ' ' + rChild->GetConstraints() + ']';
        }
        virtual ~Group() { refCount--; }

        const Node* GetLChild() const { return lChild.get(); }
        const Node* GetRChild() const { return rChild.get(); }

        // Diagnostic
        inline static int refCount{0};
        static int GetRefCount() { return refCount; }

        std::ostream& Dump(std::ostream& os)
        {
            return os << "Group " << GetOperatorStr(GetOperator()) << ": " << GetConstraints();
        }

    private:
        std::unique_ptr<Node> lChild;
        std::unique_ptr<Node> rChild;
    };
    // End of class Group

public:
    Constraints() = default;
    ~Constraints() = default;

    bool Parse(const char* constraintsStr);
    bool Parse(const std::string& constraintsStr) { return Parse(constraintsStr.c_str()); }
    bool IsValid() { return (bool)constraintsTree; }
    const std::string& GetError() { return err; }

    template<class OBJECT>
    bool Evaluate(const OBJECT& object, bool& result)
    {
        if(constraintsTree)
            return EvaluateImpl(*constraintsTree, object, result);
        err = "Invalid (null) root logical node";
        return false;
    }

    // Diagnostic
    std::ostream& Dump(std::ostream& os) { return Dump(os, constraintsTree.get()); }

private:
    Node* Parse(const char* constraintsStr, std::unique_ptr<Node>& node);
    Node* ParseOperand(const char* constraintsStr, size_t& len);
    Node::Operator ParseLogicalOperator(const char* constraintsStr, size_t& len);

    bool ParseOperandName(const char* constraintsStr, size_t& len, std::string& name);
    bool ParseOperandValue(const char* constraintsStr, size_t& len,
            std::string& value, const char* terminators=nullptr);
    Node::Operator ParseOperandOperator(const char* constraintsStr, size_t& len);
    bool BuildValuesForOperatorIN(const char* constraintsStr, size_t& len,
            const std::string& name, std::string& subConstraints);

    std::ostream& Dump(std::ostream& msg, const Node* node);
    static const std::string& GetOperatorStr(Node::Operator operIn);

    // Evaluates a logical expression for OBJECT
    // Note: OBJECT must provide GetValue() method with a follow signature:
    // const Value* Object::GetValue(const std::string& name) const;
    //
    // Note: We must be able to handle different object types. One way would
    // be to make Object::GetValue() method virtual, but that will affect
    // performance with a large volume of objects. Template implementation
    // allows to avoid using virtual functions and hence perform better.
    template<class OBJECT>
    bool EvaluateImpl(const Node& node, const OBJECT& object, bool& result);

    // Class data
    std::unique_ptr<Node> constraintsTree;
    std::string err;
    std::string constraintsStrIn;   // Only used for Diagnostic
    int depth = 0;                  // Only used for Diagnostic

    // Omit implementation of the copy constructor and assignment operator
    Constraints(const Constraints&) = delete;
    Constraints& operator=(const Constraints&) = delete;
};

template<class OBJECT>
bool Constraints::EvaluateImpl(const Node& node, const OBJECT& object, bool& result)
{
    // Create an iterator for the child nodes of the current group.
    Node::Type type = node.GetType();

    if(type == Node::GROUP)
    {
        const Group& group = (const Group&)node;
        Node::Operator logicalOperator = group.GetOperator();
        bool resultA = false;
        bool resultB = false;

        const Node* pLChild = group.GetLChild();
        const Node* pBChild = group.GetRChild();

        if(!pLChild || !pBChild)
        {
            err = "Badly formed logical expression";
            return false;
        }

        // Recurse on the L child
        if(!EvaluateImpl(*pLChild, object, resultA))
            return false;

        // Short circuit OR  eval if A is TRUE.
        // Short circuit AND eval if A is FALSE.
        if(logicalOperator == Node::OR && resultA)
        {
            result = true;
            return true;
        }
        else if(logicalOperator == Node::AND && !resultA)
        {
            result = false;
            return true;
        }

        // Recurse on the R child
        if(!EvaluateImpl(*pBChild, object, resultB))
            return false;

        switch(logicalOperator)
        {
            case Node::OR:
                result = (resultA || resultB);
                break;

            case Node::AND:
                result = (resultA && resultB);
                break;

            default:
                err = "Invalid group operand " +  GetOperatorStr(logicalOperator) + " in logical expression evaluation.";
                return false;
        }
    }
    else if(type == Node::ELEMENT)
    {
        const Element& element = (const Element&)node;
        Node::Operator logicalOperator = element.GetOperator();

        const Value* valueA = object.GetValue(element.GetName());
        if(!valueA)
        {
            // TODO: If we don't have a value then we have nothing to evaluate.
            // We should consider returning "true" with result set to "false".
            err = "Evaluated object doesn't have a value for a name '" + element.GetName() + "'";
            return false;
        }

        const Value& valueB = element.GetValue();

        switch(logicalOperator)
        {
            case Node::EQ:  // ==
                result = (*valueA == valueB);
                break;

            case Node::NE:  // !=
                result = (*valueA != valueB);
                break;

            case Node::LT:  // <
                result = (*valueA < valueB);
                break;

            case Node::LE:  // <=
                result = (*valueA <= valueB);
                break;

            case Node::GT:  // >
                result = (*valueA > valueB);
                break;

            case Node::GE:  // >=
                result = (*valueA >= valueB);
                break;

//            case Node::ISNOTNULL:   // TODO
//                result = // TODO
//                break;
//
//            case Node::ISNULL:      // TODO
//                result = // TODO
//                break;

            default:
                err = "Invalid element operand " + GetOperatorStr(logicalOperator) + " in logical expression evaluation.";
                return false;
        }
    }
    else
    {
        err = "Invalid logical node type " + type;
        return false;
    }

    return true;
}

#endif // __CONSTRAINTS_H__
