#include <cstddef>
#include <string>
#include <vector>
using namespace std;

string generateCanonicalSOP(const string &bitstream,
                            const vector<string> &inputNames,
                            const string &outputName);

string generateCanonicalPOS(const string &bitstream,
                            const vector<string> &inputNames,
                            const string &outputName);
string generateMinimizedSOP(const string &bitstream,
                            const vector<string> &inputNames,
                            const string &outputName,
                            int &literalsSaved);
string generateMinimizedPOS(const string &bitstream,
                            const vector<string> &inputNames,
                            const string &outputName,
                            int &literalsSaved);

size_t countPrimeImplicants(const string &bitstream);
size_t countEssentialPrimeImplicants(const string &bitstream);
size_t countOnSetMinterms(const string &bitstream);
size_t countOffSetMaxterms(const string &bitstream);

string generateHazardFreeSOP(const string &bitstream,
                             const vector<string> &inputNames,
                             const string &outputName,
                             bool &hazardExists);