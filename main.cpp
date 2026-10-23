#include "parser.h"
#include "analyzer.h"
#include "verilog_generation.h"

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
    if (argc < 3)
    {
        cout << "please provide arguments\n";
        return 0;
    }
    string filename = argv[1];
    ifstream inFile(filename);

    string out_file = argv[2];
    ofstream outFile(out_file);

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
        cout << "Information for " << outputName << endl;
        outFile << "Information for " << outputName << endl;

        cout << "canonical SOP: "
             << generateCanonicalSOP(bitstream, inputNames, outputName) << endl;
        cout << "canonical POS: "
             << generateCanonicalPOS(bitstream, inputNames, outputName) << endl;

        outFile << "canonical SOP: "
                << generateCanonicalSOP(bitstream, inputNames, outputName) << endl;
        outFile << "canonical POS: "
                << generateCanonicalPOS(bitstream, inputNames, outputName) << endl;

        // Find the inverse function by inverting the bitstream.
        const string bitstreamFlip = flipBinaryString(bitstream);
        cout << "canonical SOP: "
             << generateCanonicalSOP(bitstreamFlip, inputNames, outputName + "-bar") << endl;
        cout << "canonical POS: "
             << generateCanonicalPOS(bitstreamFlip, inputNames, outputName + "-bar") << endl;

        outFile << "canonical SOP: "
                << generateCanonicalSOP(bitstreamFlip, inputNames, outputName + "-bar") << endl;
        outFile << "canonical POS: "
                << generateCanonicalPOS(bitstreamFlip, inputNames, outputName + "-bar") << endl;

        // minimized SOP/POS
        int sopLiteralsSaved = 0;
        int posLiteralsSaved = 0;
        string minimizedSOP = generateMinimizedSOP(bitstream, inputNames, outputName, posLiteralsSaved);
        cout << "minimized SOP: "
             << minimizedSOP
             << " (literals saved: " << sopLiteralsSaved << ")" << endl;
        cout << "minimized POS: "
             << generateMinimizedPOS(bitstream, inputNames, outputName, posLiteralsSaved)
             << " (literals saved: " << posLiteralsSaved << ")" << endl;

        cout << "# prime implicants: " << countPrimeImplicants(bitstream) << endl;
        cout << "# essential prime implicants: " << countEssentialPrimeImplicants(bitstream) << endl;
        cout << "# on-set minterms: " << countOnSetMinterms(bitstream) << endl;
        cout << "# off-set minterms: " << countOffSetMaxterms(bitstream) << endl;

        outFile << "minimized SOP: "
                << minimizedSOP
                << " (literals saved: " << sopLiteralsSaved << ")" << endl;
        outFile << "minimized POS: "
                << generateMinimizedPOS(bitstream, inputNames, outputName, posLiteralsSaved)
                << " (literals saved: " << posLiteralsSaved << ")" << endl;

        outFile << "# prime implicants: " << countPrimeImplicants(bitstream) << endl;
        outFile << "# essential prime implicants: " << countEssentialPrimeImplicants(bitstream) << endl;
        outFile << "# on-set minterms: " << countOnSetMinterms(bitstream) << endl;
        outFile << "# off-set minterms: " << countOffSetMaxterms(bitstream) << endl;

        bool hazardExists = false;
        string hazardFree = generateHazardFreeSOP(bitstream, inputNames, outputName, hazardExists);
        if (hazardExists)
        {
            cout << "there is hazard in original circuit, hazard-free version: " << hazardFree << endl;
            outFile << "there is hazard in original circuit, hazard-free version: " << hazardFree << endl;
        }
        else
        {
            cout << "no hazard exists" << endl;
            outFile << "no hazard exists" << endl;
        }

        outFile << endl;

        string truthTablePath = outputName + ".txt";
        exportTruthTable(bitstream, inputNames, outputName, truthTablePath);

        // verilog file creation
        string verilog_path = outputName + ".v";
        ofstream verilogFile(verilog_path);
        cout << generateVerilogModule(minimizedSOP) << endl;
        verilogFile << generateVerilogModule(minimizedSOP) << endl;
    }

    return 0;
}