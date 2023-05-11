//
// main.cpp
//
#include <iostream>         // std::cout
#include <fstream>          // std::ifstream
#include <sstream>          // std::stringstream
#include <map>
#include <unistd.h>         // access()
#include <string.h>         // strerror()
#include "constraints.h"
#include "logger.h"

//
// Test Object class to evaluate
//
class Object : public std::map<std::string, Value>
{
public:
    const Value* GetValue(const std::string& name) const
    {
        auto it = find(name);
        return (it == end() ? nullptr : &it->second);
    }

    void SetValue(const std::string& name, const std::string& valueStr)
    {
        Value& value = operator[](name); // Add a new value
        value = valueStr;
    }

    std::ostream& Dump(std::ostream& os = std::cout) const
    {
        for(auto itr = begin(); itr != end(); ++itr)
        {
            if(itr != begin())
                os << ", ";
            os << "'" << itr->first << "'=" << itr->second;
        }
        return (empty() ? os : os << std::endl);
    }
};

std::string& TrimString(std::string& str)
{
    const char* whiteSpace = " \t\v\r\n";
    size_t start = str.find_first_not_of(whiteSpace);
    size_t end = str.find_last_not_of(whiteSpace);
    if(start == end)
        str.clear();
    else
        str = str.substr(start, end - start + 1);
    return str;
}

int main(int argc, const char** argv)
{
    const char* inputFileName = "";
    const char* constraintsStr = "";

    if(argc > 1)
    {
        // Read imput file
        inputFileName = argv[1];
        if(access(inputFileName, F_OK) != 0)
        {
            ERRORMSG("Failed to access '" << inputFileName << "': " <<  strerror(errno));
            return 1;
        }
    }
    else
    {
        // Use some hard-coded file name
        inputFileName = "books.txt";
    }

    if(argc > 2)
    {
        constraintsStr  = argv[2];
    }
    else
    {
        // Use some hard-coded constraints
//        constraintsStr = "(Language == French OR Language == Spanish) AND BookNumber >= 120";
//        constraintsStr = "(Language == French OR Language == Spanish) AND BookNumber < 120";
        constraintsStr = "(Language == French OR Language == Spanish)";
//        constraintsStr = "Nationality IN (French, American, Russian)";
    }

    // Build constraints from a string
    Constraints constraints;
    if(!constraints.Parse(constraintsStr))
    {
        ERRORMSG(constraints.GetError());
        return 1;
    }
    constraints.Dump(std::cout);
    std::cout << std::endl;

    // Open input file
    std::ifstream in(inputFileName);
    if(!in)
    {
        ERRORMSG("Cannot open input file '" << inputFileName << "'");
        return 1;
    }

    // Go through input file and select lines that matches constraints
    std::string line;
    int matchCount = 0;

    while(std::getline(in, line))
    {
        std::stringstream ss(line) ;
        std::string token;
        Object obj;

        // Construct object from a line
        while(getline(ss, token, ','))
        {
            // Trim leading/trailing while spaces
            TrimString(token);

            // Spit token into name and value
            size_t pos = token.find('=');
            if(pos != std::string::npos)
            {
                std::string name = token.substr(0, pos);
                std::string value = token.substr(pos + 1);
                obj.SetValue(TrimString(name), TrimString(value));
            }
        }

        // Evaluate object for constraints matching
        bool result = false;
        if(!constraints.Evaluate(obj, result))
        {
            ERRORMSG(constraints.GetError());
        }
        else if(result)
        {
            matchCount++;
            std::cout << matchCount << ": ";
            obj.Dump(std::cout);
        }
    }

    if(matchCount == 0)
        std::cout << "No matches found" << std::endl;

    return 0;
}

//    const char* constraints[] =
//    {
//        "  Genre == Detective  ",
//        "  Genre == \" Romance \"  ",
//        //"  Genre == \" aa \"  AND  ", // Failure test
//        //"  Genre == \" 000_! Test \"  AND  ", // Failure est
//        "  Genre == Detective AND Language == English ",
//        "  Genre == \"Detective\" AND Language == English  OR  BookNumber==200 ",
//
//        "  ( Genre == Detective )  ",
//        "  Genre == \"Detective\" AND (Language == English  OR  BookNumber==200 )",
//        "  Genre == \"Detective\" AND (Language == English  OR  BookNumber>10 ) OR  Nationality == American",
//
//        "(\"Genre\" == \"Detective\" AND \"BookNumber\" == \"0\") AND "
//        "(\"Name\" == \"000Test\" OR \"Language\" == \"English\")",
//
//        "(Genre == \"Romance\" AND BookNumber == \"0\") AND "
//        "(\"Genre\" == \"Romance\" OR \"Language\" == \"English\")",
//
//        // Test for operator "IN(aaa, bbb, ccc)"
//        "Genre == \"Romance\" AND Language IN ( \"AAA & BBB\"  ,  CCC ,  \"DD&EE\"  ) ",
//
//        "Genre == Detective AND (Language == Belgian OR Language == French) AND BookNumber>=5",
//
//        nullptr
//    };

