#include "analyzer.h"

#include <algorithm>
#include <climits>
#include <functional>
#include <limits>
#include <map>
#include <stdexcept>
#include <utility>

bool inputBitForRow(size_t row, size_t inputIndex, size_t inputCount)
{
    return ((row >> (inputCount - 1 - inputIndex)) & 1U) != 0;
}

struct Implicant
{
    string pattern;
    vector<size_t> minterms;
    bool combined = false;
};

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

void validateCountBitstream(const string &bitstream)
{
    if (bitstream.empty() || (bitstream.size() & (bitstream.size() - 1)) != 0)
        throw invalid_argument("bitstream length must be a nonzero power of two");
    if (any_of(bitstream.begin(), bitstream.end(),
               [](char bit) { return bit != '0' && bit != '1'; }))
        throw invalid_argument("bitstream may contain only '0' and '1'");
}

size_t countPrimeImplicants(const string &bitstream)
{
    validateCountBitstream(bitstream);
    return findPrimeImplicants(bitstream, '1').size();
}

size_t countEssentialPrimeImplicants(const string &bitstream)
{
    validateCountBitstream(bitstream);
    const vector<Implicant> primes = findPrimeImplicants(bitstream, '1');
    vector<bool> essential(primes.size(), false);

    for (size_t minterm = 0; minterm < bitstream.size(); ++minterm)
    {
        if (bitstream[minterm] != '1')
            continue;

        size_t coverCount = 0;
        size_t soleCover = primes.size();
        for (size_t prime = 0; prime < primes.size(); ++prime)
        {
            if (binary_search(primes[prime].minterms.begin(),
                              primes[prime].minterms.end(), minterm))
            {
                ++coverCount;
                soleCover = prime;
                if (coverCount > 1)
                    break;
            }
        }
        if (coverCount == 1)
            essential[soleCover] = true;
    }
    return static_cast<size_t>(count(essential.begin(), essential.end(), true));
}

size_t countOnSetMinterms(const string &bitstream)
{
    validateCountBitstream(bitstream);
    return static_cast<size_t>(count(bitstream.begin(), bitstream.end(), '1'));
}

size_t countOffSetMaxterms(const string &bitstream)
{
    validateCountBitstream(bitstream);
    return static_cast<size_t>(count(bitstream.begin(), bitstream.end(), '0'));
}

string generateHazardFreeSOP(const string &bitstream,
                             const vector<string> &inputNames,
                             const string &outputName,
                             bool &hazardExists)
{
    validateCountBitstream(bitstream);
    if (inputNames.size() >= numeric_limits<size_t>::digits ||
        bitstream.size() != (size_t{1} << inputNames.size()))
        throw invalid_argument("bitstream length must equal 2^number-of-inputs");
    if (outputName.empty() ||
        any_of(inputNames.begin(), inputNames.end(),
               [](const string &name) { return name.empty(); }))
        throw invalid_argument("input and output names must not be empty");

    hazardExists = false;
    vector<Implicant> cover = findMinimumCover(bitstream, '1');
    const vector<Implicant> primes = findPrimeImplicants(bitstream, '1');
    const size_t inputCount = inputNames.size();

    for (size_t row = 0; row < bitstream.size(); ++row)
    {
        if (bitstream[row] != '1')
            continue;

        for (size_t input = 0; input < inputCount; ++input)
        {
            const size_t neighbor = row ^ (size_t{1} << (inputCount - 1 - input));
            if (neighbor <= row || bitstream[neighbor] != '1')
                continue;

            const bool alreadyBridged = any_of(
                cover.begin(), cover.end(),
                [row, neighbor](const Implicant &implicant)
                {
                    return binary_search(implicant.minterms.begin(),
                                         implicant.minterms.end(), row) &&
                           binary_search(implicant.minterms.begin(),
                                         implicant.minterms.end(), neighbor);
                });
            if (alreadyBridged)
                continue;

            hazardExists = true;
            const auto bridge = find_if(
                primes.begin(), primes.end(),
                [row, neighbor](const Implicant &implicant)
                {
                    return binary_search(implicant.minterms.begin(),
                                         implicant.minterms.end(), row) &&
                           binary_search(implicant.minterms.begin(),
                                         implicant.minterms.end(), neighbor);
                });
            if (bridge == primes.end())
                throw runtime_error("could not find an implicant to cover adjacent on-set minterms");

            const bool alreadyAdded = any_of(
                cover.begin(), cover.end(),
                [&bridge](const Implicant &implicant)
                {
                    return implicant.pattern == bridge->pattern;
                });
            if (!alreadyAdded)
                cover.push_back(*bridge);
        }
    }

    if (cover.empty())
        return outputName + " = 0";
    sort(cover.begin(), cover.end(),
         [](const Implicant &left, const Implicant &right)
         {
             return left.pattern < right.pattern;
         });

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
                            const string &outputName,
                            int &literalsSaved)
{
    const size_t oneCount = count(bitstream.begin(), bitstream.end(), '1');
    if (!inputNames.empty() && oneCount > static_cast<size_t>(INT_MAX) / inputNames.size())
        throw overflow_error("SOP literal savings exceed the supported integer range");
    const int canonicalLiteralCount =
        static_cast<int>(oneCount * inputNames.size());
    const vector<Implicant> cover = findMinimumCover(bitstream, '1');
    size_t minimizedLiteralCount = 0;
    for (const Implicant &implicant : cover)
    {
        const size_t implicantLiteralCount = static_cast<size_t>(
            count(implicant.pattern.begin(), implicant.pattern.end(), '0') +
            count(implicant.pattern.begin(), implicant.pattern.end(), '1'));
        if (implicantLiteralCount > static_cast<size_t>(INT_MAX) - minimizedLiteralCount)
            throw overflow_error("minimized SOP literal count exceeds the supported integer range");
        minimizedLiteralCount += implicantLiteralCount;
    }
    literalsSaved = canonicalLiteralCount - static_cast<int>(minimizedLiteralCount);

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
                            const string &outputName,
                            int &literalsSaved)
{
    const size_t zeroCount = count(bitstream.begin(), bitstream.end(), '0');
    if (!inputNames.empty() && zeroCount > static_cast<size_t>(INT_MAX) / inputNames.size())
        throw overflow_error("POS literal savings exceed the supported integer range");
    const int canonicalLiteralCount =
        static_cast<int>(zeroCount * inputNames.size());
    const vector<Implicant> cover = findMinimumCover(bitstream, '0');
    size_t minimizedLiteralCount = 0;
    for (const Implicant &implicant : cover)
    {
        const size_t implicantLiteralCount = static_cast<size_t>(
            count(implicant.pattern.begin(), implicant.pattern.end(), '0') +
            count(implicant.pattern.begin(), implicant.pattern.end(), '1'));
        if (implicantLiteralCount > static_cast<size_t>(INT_MAX) - minimizedLiteralCount)
            throw overflow_error("minimized POS literal count exceeds the supported integer range");
        minimizedLiteralCount += implicantLiteralCount;
    }
    literalsSaved = canonicalLiteralCount - static_cast<int>(minimizedLiteralCount);

    if (all_of(bitstream.begin(), bitstream.end(),
               [](char bit)
               { return bit == '0'; }))
        return outputName + " = 0";
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