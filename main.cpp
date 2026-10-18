#include "parser.h"
#include "analyzer.h"

#include <bitset>

// Function that takes a const string and returns a new flipped string
std::string flipBinaryString(const std::string &input)
{
    std::string result;
    result.reserve(input.size()); // Optimize memory allocation

    std::transform(input.begin(), input.end(), std::back_inserter(result), [](char c)
                   { return (c == '0') ? '1' : '0'; });

    return result;
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        cout << "please provide arguments\n";
        return 0;
    }
    string filename = argv[1];
    ifstream inFile(filename);

    if (!inFile)
    {
        cerr << "Error: Could not open file '" << filename << "'\n";
        return 1;
    }

    vector<string> lines;
    string line;
    while (getline(inFile, line))
        lines.push_back(line);
    inFile.close();

    vector<string> bitstreams;
    vector<string> inputNames;
    vector<string> outputNames;

    if (filename.length() >= 5 && filename.compare(filename.length() - 5, 5, ".blif") == 0)
    {
        const BlifCircuit circuit = parseBlif(lines);
        unordered_map<string, const BlifNode *> nodes;
        for (const BlifNode &node : circuit.nodes)
            nodes.emplace(node.output, &node);

        bitstreams = generateBitstreams(
            circuit.inputs.size(), circuit.outputs.size(),
            [&circuit, &nodes](const vector<bool> &inputValues)
            {
                unordered_map<string, bool> values;
                for (size_t i = 0; i < circuit.inputs.size(); ++i)
                    values.emplace(circuit.inputs[i], inputValues[i]);

                set<string> active;
                vector<bool> outputValues;
                outputValues.reserve(circuit.outputs.size());
                for (const string &output : circuit.outputs)
                    outputValues.push_back(evaluateBlifSignal(output, nodes, values, active));
                return outputValues;
            });
        inputNames = circuit.inputs;
        outputNames = circuit.outputs;
    }
    else
    {
        vector<vector<ProductTerm>> outputTerms;
        set<char> allInputs;

        // Parse every expression so outputs share one input ordering and row enumeration.
        for (string &expression : lines)
        {
            // Clear out trailing or leading spaces/returns
            expression.erase(expression.find_last_not_of(" \r\n\t") + 1);

            set<char> expressionInputs;
            string outputName;
            vector<ProductTerm> terms = parseSOP(expression, expressionInputs, outputName);

            if (!terms.empty())
            {
                outputTerms.push_back(move(terms));
                outputNames.push_back(outputName.empty() ? "Out" : outputName);
                allInputs.insert(expressionInputs.begin(), expressionInputs.end());
            }
            else
            {
                cout << "Invalid or empty expression found.\n\n";
            }
        }

        bitstreams = generateSOPBitstream(outputTerms, allInputs);
        for (char input : allInputs)
            inputNames.push_back(string(1, input));
    }

    for (size_t output = 0; output < bitstreams.size(); ++output)
    {
        const string &bitstream = bitstreams[output];
        const string &outputName = outputNames[output];
        cout << "canonical SOP: "
             << generateCanonicalSOP(bitstream, inputNames, outputName) << "\n";
        cout << "canonical POS: "
             << generateCanonicalPOS(bitstream, inputNames, outputName) << "\n";

        // Find the inverse function by inverting the bitstream.
        const string bitstreamFlip = flipBinaryString(bitstream);
        cout << "canonical SOP: "
             << generateCanonicalSOP(bitstreamFlip, inputNames, outputName + "-bar") << "\n";
        cout << "canonical POS: "
             << generateCanonicalPOS(bitstreamFlip, inputNames, outputName + "-bar") << "\n";
    }

    return 0;
}