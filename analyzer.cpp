#include "analyzer.h"

bool inputBitForRow(size_t row, size_t inputIndex, size_t inputCount)
{
    return ((row >> (inputCount - 1 - inputIndex)) & 1U) != 0;
}

string generateCanonicalSOP(const string &bitstream,
                            const vector<string> &inputNames,
                            const string &outputName)
{

    string expression;
    for (size_t row = 0; row < bitstream.size(); ++row)
    {
        if (bitstream[row] != '1')
            continue;

        if (!expression.empty())
            expression += " + ";
        if (inputNames.empty())
        {
            expression += "1";
            continue;
        }

        for (size_t input = 0; input < inputNames.size(); ++input)
        {
            if (input != 0)
                expression += " ";
            expression += inputNames[input];
            if (!inputBitForRow(row, input, inputNames.size()))
                expression += "'";
        }
    }

    if (expression.empty())
        expression = "0";
    return outputName + " = " + expression;
}

string generateCanonicalPOS(const string &bitstream,
                            const vector<string> &inputNames,
                            const string &outputName)
{

    string expression;
    for (size_t row = 0; row < bitstream.size(); ++row)
    {
        if (bitstream[row] != '0')
            continue;

        if (!expression.empty())
            expression += " ";
        if (inputNames.empty())
        {
            expression += "(0)";
            continue;
        }

        expression += "(";
        for (size_t input = 0; input < inputNames.size(); ++input)
        {
            if (input != 0)
                expression += " + ";
            expression += inputNames[input];
            if (inputBitForRow(row, input, inputNames.size()))
                expression += "'";
        }
        expression += ")";
    }

    if (expression.empty())
        expression = "1";
    return outputName + " = " + expression;
}