#include <string>
#include <vector>
using namespace std;

string generateCanonicalSOP(const string &bitstream,
                            const vector<string> &inputNames,
                            const string &outputName);

string generateCanonicalPOS(const string &bitstream,
                            const vector<string> &inputNames,
                            const string &outputName);