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
                            const string &outputName);
string generateMinimizedPOS(const string &bitstream,
                            const vector<string> &inputNames,
                            const string &outputName);