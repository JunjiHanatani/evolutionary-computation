#include <vector>
#include <random>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <string>
#include "utility.h"
using std::vector;
using std::string;

double PI=acos(-1);

std::random_device seed_gen;
//std::mt19937 mt_engine(seed_gen());
std::mt19937 mt_engine(1);
int get_rand_range_int(int min_val, int max_val) {
    std::uniform_int_distribution<int> gen_rand_uni_int( min_val, max_val );
    return gen_rand_uni_int(mt_engine);
}

double get_rand_range_dbl(double min_val, double max_val) {
    std::uniform_real_distribution<double> gen_rand_uni_real( min_val, max_val );
    return gen_rand_uni_real(mt_engine);
}

int findIndex( vector<int> vec, int value ){
    vector<int>::iterator iter = std::find( vec.begin(), vec.end(), value);
    size_t index = std::distance( vec.begin(), iter );
    if(index == vec.size())
        {
            return -1;
        }
    return index;
}

vector<string> split(string& input, char delimiter)
{
    std::istringstream stream(input);
    string field;
    vector<string> result;
    while (getline(stream, field, delimiter)) {
        result.push_back(field);
    }
    return result;
}

vector<vector<double>> read_csv(string filename, int start_line, int end_line)
{
    std::ifstream ifs(filename);
    vector<vector<double>> result;
    string line;
    int i=0;

    while (getline(ifs, line)) {

        if (i>=start_line){

            vector<double> vec;
            vector<string> strvec = split(line, ',');
            int num=strvec.size();
            for (int j=0; j<num;j++){
                vec.push_back(stod(strvec.at(j)));
            }
            result.push_back(vec);
        }
        if(i == end_line) break;

        i++;
    }

    return result;
}

