#include "analyzer.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <map>
#include <stdexcept>
#include <utility>

bool inputBitForRow(size_t row, size_t inputIndex, size_t inputCount)
{
    return ((row >> (inputCount - 1 - inputIndex)) & 1U) != 0;
}

namespace
{
struct Implicant
{
    string pattern;
    vector<size_t> minterms;
    bool combined = false;
};

void validateMinimizerInputs(const string &bitstream,
                             const vector<string> &inputNames,
                             const string &outputName)
{
    if (inputNames.size() >= numeric_limits<size_t>::digits)
        throw invalid_argument("too many input names for a truth-table bitstream");
    if (bitstream.size() != (size_t{1} << inputNames.size()))
        throw invalid_argument("bitstream length must equal 2^number-of-inputs");
    if (outputName.empty())
        throw invalid_argument("output name must not be empty");
    if (any_of(bitstream.begin(), bitstream.end(),
               [](char bit) { return bit != '0' && bit != '1'; }))
        throw invalid_argument("bitstream may contain only '0' and '1'");
    if (any_of(inputNames.begin(), inputNames.end(),
               [](const string &name) { return name.empty(); }))
        throw invalid_argument("input names must not be empty");
}

vector<Implicant> findPrimeImplicants(const string &bitstream, char target)
{
    vector<Implicant> current;
    for (size_t row = 0; row < bitstream.size(); ++row)
    {
        if (bitstream[row] == target)
        {
            size_t inputCount = 0;
            for (size_t rows = bitstream.size(); rows > 1; rows >>= 1)
                ++inputCount;
            string pattern;
            pattern.reserve(inputCount);
            for (size_t bit = 0; bit < inputCount; ++bit)
                pattern += ((row >> (inputCount - 1 - bit)) & 1U) != 0 ? '1' : '0';
            current.push_back({move(pattern), {row}, false});
        }
    }

    vector<Implicant> primes;
    while (!current.empty())
    {
        vector<Implicant> next;
        map<string, size_t> nextByPattern;

        for (size_t i = 0; i < current.size(); ++i)
        {
            for (size_t j = i + 1; j < current.size(); ++j)
            {
                size_t differingPosition = current[i].pattern.size();
                bool canCombine = true;
                for (size_t bit = 0; bit < current[i].pattern.size(); ++bit)
                {
                    if (current[i].pattern[bit] == current[j].pattern[bit])
                        continue;
                    if (current[i].pattern[bit] == '-' || current[j].pattern[bit] == '-' ||
                        differingPosition != current[i].pattern.size())
                    {
                        canCombine = false;
                        break;
                    }
                    differingPosition = bit;
                }
                if (!canCombine || differingPosition == current[i].pattern.size())
                    continue;

                current[i].combined = true;
                current[j].combined = true;
                string pattern = current[i].pattern;
                pattern[differingPosition] = '-';
                vector<size_t> minterms = current[i].minterms;
                minterms.insert(minterms.end(), current[j].minterms.begin(),
                                current[j].minterms.end());
                sort(minterms.begin(), minterms.end());
                minterms.erase(unique(minterms.begin(), minterms.end()), minterms.end());

                const auto found = nextByPattern.find(pattern);
                if (found == nextByPattern.end())
                {
                    nextByPattern.emplace(pattern, next.size());
                    next.push_back({move(pattern), move(minterms), false});
                }
                else
                {
                    vector<size_t> &existing = next[found->second].minterms;
                    existing.insert(existing.end(), minterms.begin(), minterms.end());
                    sort(existing.begin(), existing.end());
                    existing.erase(unique(existing.begin(), existing.end()), existing.end());
                }
            }
        }

        for (Implicant &implicant : current)
        {
            if (!implicant.combined)
                primes.push_back(move(implicant));
        }
        current = move(next);
    }
    return primes;
}

vector<Implicant> findMinimumCover(const string &bitstream, char target)
{
    vector<size_t> targetMinterms;
    for (size_t row = 0; row < bitstream.size(); ++row)
    {
        if (bitstream[row] == target)
            targetMinterms.push_back(row);
    }
    if (targetMinterms.empty())
        return {};

    vector<Implicant> primes = findPrimeImplicants(bitstream, target);
    vector<size_t> literalCounts(primes.size(), 0);
    vector<vector<size_t>> coveringPrimes(bitstream.size());
    for (size_t prime = 0; prime < primes.size(); ++prime)
    {
        literalCounts[prime] = count(primes[prime].pattern.begin(),
                                     primes[prime].pattern.end(), '0') +
                              count(primes[prime].pattern.begin(),
                                     primes[prime].pattern.end(), '1');
        for (size_t minterm : primes[prime].minterms)
            coveringPrimes[minterm].push_back(prime);
    }

    vector<bool> selected(primes.size(), false);
    vector<size_t> currentCover;
    vector<size_t> bestCover;
    size_t bestLiteralCount = numeric_limits<size_t>::max();

    function<void(size_t)> search = [&](size_t currentLiteralCount)
    {
        if (currentLiteralCount > bestLiteralCount ||
            (currentLiteralCount == bestLiteralCount &&
             currentCover.size() >= bestCover.size() && !bestCover.empty()))
            return;

        size_t uncovered = bitstream.size();
        size_t fewestChoices = numeric_limits<size_t>::max();
        for (size_t minterm : targetMinterms)
        {
            const bool covered = any_of(currentCover.begin(), currentCover.end(),
                                        [&](size_t prime)
                                        {
                                            return binary_search(primes[prime].minterms.begin(),
                                                                 primes[prime].minterms.end(),
                                                                 minterm);
                                        });
            if (covered)
                continue;

            size_t choices = 0;
            for (size_t prime : coveringPrimes[minterm])
            {
                if (!selected[prime])
                    ++choices;
            }
            if (choices < fewestChoices)
            {
                fewestChoices = choices;
                uncovered = minterm;
            }
        }

        if (uncovered == bitstream.size())
        {
            bestCover = currentCover;
            bestLiteralCount = currentLiteralCount;
            return;
        }
        if (fewestChoices == 0)
            return;

        for (size_t prime : coveringPrimes[uncovered])
        {
            if (selected[prime])
                continue;
            selected[prime] = true;
            currentCover.push_back(prime);
            search(currentLiteralCount + literalCounts[prime]);
            currentCover.pop_back();
            selected[prime] = false;
        }
    };
    search(0);

    vector<Implicant> cover;
    cover.reserve(bestCover.size());
    for (size_t prime : bestCover)
        cover.push_back(move(primes[prime]));
    sort(cover.begin(), cover.end(),
         [](const Implicant &left, const Implicant &right)
         {
             return left.pattern < right.pattern;
         });
    return cover;
}
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

string generateMinimizedSOP(const string &bitstream,
                            const vector<string> &inputNames,
                            const string &outputName)
{
    validateMinimizerInputs(bitstream, inputNames, outputName);
    const vector<Implicant> cover = findMinimumCover(bitstream, '1');
    if (cover.empty())
        return outputName + " = 0";

    string expression;
    for (const Implicant &implicant : cover)
    {
        if (!expression.empty())
            expression += " + ";
        if (inputNames.empty())
        {
            expression += "1";
            continue;
        }

        bool hasLiteral = false;
        for (size_t input = 0; input < inputNames.size(); ++input)
        {
            if (implicant.pattern[input] == '-')
                continue;
            if (hasLiteral)
                expression += " ";
            expression += inputNames[input];
            if (implicant.pattern[input] == '0')
                expression += "'";
            hasLiteral = true;
        }
        if (!hasLiteral)
            expression += "1";
    }
    return outputName + " = " + expression;
}

string generateMinimizedPOS(const string &bitstream,
                            const vector<string> &inputNames,
                            const string &outputName)
{
    validateMinimizerInputs(bitstream, inputNames, outputName);
    if (all_of(bitstream.begin(), bitstream.end(),
               [](char bit) { return bit == '0'; }))
        return outputName + " = 0";
    const vector<Implicant> cover = findMinimumCover(bitstream, '0');
    if (cover.empty())
        return outputName + " = 1";

    string expression;
    for (const Implicant &implicant : cover)
    {
        if (!expression.empty())
            expression += " ";
        if (inputNames.empty())
        {
            expression += "(0)";
            continue;
        }

        expression += "(";
        bool hasLiteral = false;
        for (size_t input = 0; input < inputNames.size(); ++input)
        {
            if (implicant.pattern[input] == '-')
                continue;
            if (hasLiteral)
                expression += " + ";
            expression += inputNames[input];
            if (implicant.pattern[input] == '1')
                expression += "'";
            hasLiteral = true;
        }
        if (!hasLiteral)
            expression += "0";
        expression += ")";
    }
    return outputName + " = " + expression;
}