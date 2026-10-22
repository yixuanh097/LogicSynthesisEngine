#include "verilog_generation.h"
#include "parser.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;

bool isVerilogIdentifier(const string &name)
{
    if (name.empty() ||
        !(isalpha(static_cast<unsigned char>(name[0])) || name[0] == '_'))
        return false;

    const bool validCharacters = all_of(name.begin() + 1, name.end(), [](char ch)
                                        {
                                                 const unsigned char value =
                                                     static_cast<unsigned char>(ch);
                                                 return isalnum(value) || ch == '_' || ch == '$'; });
    if (!validCharacters)
        return false;

    static const set<string> keywords{
        "always", "and", "assign", "begin", "buf", "case", "casex", "casez",
        "else", "end", "endcase", "endmodule", "input", "inout", "integer",
        "logic", "module", "nand", "negedge", "nor", "not", "or", "output",
        "parameter", "posedge", "reg", "wire", "xnor", "xor"};
    return keywords.count(name) == 0;
}

string generateVerilogModule(const string &sop)
{
    const size_t equals = sop.find('=');

    const string output = trim(sop.substr(0, equals));
    const string expression = trim(sop.substr(equals + 1));

    set<string> inputs;
    vector<vector<pair<string, bool>>> terms;
    bool functionIsOne = false;
    istringstream termStream(expression);
    string term;
    while (getline(termStream, term, '+'))
    {
        term = trim(term);

        istringstream literalStream(term);
        string literal;
        vector<pair<string, bool>> literals;
        bool termIsZero = false;
        bool termIsOne = false;
        while (literalStream >> literal)
        {
            bool complemented = false;
            if (!literal.empty() && literal.back() == '\'')
            {
                complemented = true;
                literal.pop_back();
            }

            if (literal == "0" || literal == "1")
            {
                if (complemented)
                    literal = literal == "0" ? "1" : "0";
                termIsZero = termIsZero || literal == "0";
                termIsOne = termIsOne || literal == "1";
                continue;
            }

            if (!isVerilogIdentifier(literal) || literal == output)
                throw invalid_argument("invalid SOP input literal '" + literal + "'");
            inputs.insert(literal);
            literals.emplace_back(literal, complemented);
        }

        if (termIsZero)
            continue;
        if (termIsOne && literals.empty())
        {
            functionIsOne = true;
            break;
        }
        terms.push_back(move(literals));
    }

    if (functionIsOne)
        terms.clear();

    set<string> usedNames = inputs;
    usedNames.insert(output);
    size_t internalIndex = 0;
    auto nextInternalName = [&usedNames, &internalIndex]()
    {
        string name;
        do
        {
            name = "w" + to_string(internalIndex++);
        } while (usedNames.count(name) != 0);
        usedNames.insert(name);
        return name;
    };

    map<string, string> invertedInputs;
    for (const vector<pair<string, bool>> &product : terms)
    {
        for (const auto &literal : product)
        {
            if (literal.second && invertedInputs.count(literal.first) == 0)
                invertedInputs.emplace(literal.first, nextInternalName());
        }
    }

    vector<string> termWires;
    for (size_t i = 0; i < terms.size(); ++i)
        termWires.push_back(nextInternalName());

    string verilog;
    verilog += "module " + output + " (\n";
    for (const string &input : inputs)
        verilog += "    input wire " + input + ",\n";
    verilog += "    output wire " + output + "\n";
    verilog += ");\n\n";

    for (const string &internalName : usedNames)
    {
        if (internalName != output && inputs.count(internalName) == 0)
            verilog += "    wire " + internalName + ";\n";
    }
    if (!invertedInputs.empty() || !termWires.empty())
        verilog += "\n";

    for (const auto &inverted : invertedInputs)
        verilog += "    not (" + inverted.second + ", " + inverted.first + ");\n";

    for (size_t i = 0; i < terms.size(); ++i)
    {
        if (terms[i].empty())
        {
            verilog += "    and (" + termWires[i] + ", 1'b1);\n";
            continue;
        }
        verilog += "    and (" + termWires[i];
        for (const auto &literal : terms[i])
        {
            verilog += ", ";
            verilog += literal.second ? invertedInputs.at(literal.first) : literal.first;
        }
        verilog += ");\n";
    }

    if (functionIsOne)
        verilog += "    or (" + output + ", 1'b1);\n";
    else if (termWires.empty())
        verilog += "    and (" + output + ", 1'b0);\n";
    else
    {
        verilog += "    or (" + output;
        for (const string &termWire : termWires)
            verilog += ", " + termWire;
        verilog += ");\n";
    }
    verilog += "\nendmodule\n";
    return verilog;
}