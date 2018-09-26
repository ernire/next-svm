//
// Created by Ernir Erlingsson on 19.9.2018.
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <functional>
#include <cstring>
#include <iomanip>


void parse_svm_light_data(std::ifstream &is, const std::function<void(int, int, float*)> &f1) {

    std::string line;
    int c, i, m = 0;
    float f;
    float sample_features[256];

    is.clear();
    is.seekg (0, std::ifstream::beg);
    while (std::getline(is, line)) {
        memset(sample_features, 0, sizeof(sample_features));
        std::istringstream iss(line);
        // class
        iss >> c;
        while (!iss.eof()) {
            iss >> i;
            // index starts at 1, we therefore decrement by one
            i--;
            if (i > m) m = i;
            iss.ignore(2, ':');
            iss >> f;
            sample_features[i] = f;
        }
        f1(m, c, sample_features);
    }
}


std::streampos fileSize( const char* filePath ){

    std::streampos fsize = 0;
    std::ifstream file( filePath, std::ios::in | std::ios::binary );

    fsize = file.tellg();
    file.seekg(0, std::ios::end );
    fsize = file.tellg() - fsize;
    file.close();

    return fsize;
}


int main(int argc, char** argv) {

    if (argc != 3) {
        std::cout << "Wrong number of arguments, should be 2" << std::endl << "1: The input file to process"
                  << std::endl << "2: The Output file" << std::endl;
        return -1;
    }

    char* in_file = argv[1];
    char* out_file = argv[2];
    std::vector<float *> features;
    std::vector<int> classes;
    int max_features = 0;
    int total_samples = 0;
    int zero_features = 0;

    std::cout << "Parsing size: " << fileSize(in_file) << std::endl;
    std::ifstream infile(in_file, std::ios::in | std::ifstream::binary);
    std::ofstream outfile(out_file, std::ios::in | std::ofstream::binary);
    if (infile.is_open() && outfile.is_open()) {

        std::cout << "Parsing the features" << std::endl;
        parse_svm_light_data(infile, [&max_features](int m, int c, float* f) ->
        void {
            if (m > max_features) max_features = m;
        });
        std::cout << "Max feature length: " << max_features << std::endl;
        parse_svm_light_data(infile, [max_features, &total_samples, &zero_features](int m, int c, const float* f) ->
        void {
            total_samples++;
            for (int i = 0; i < max_features; i++) {
                if (f[i] == 0)
                    zero_features++;
            }

        });
        std::cout << "Sparsity ratio: " << std::fixed << std::setprecision(4) <<
            ((double)zero_features / (double)(total_samples * max_features)) * 100 << "%" << std::endl;
        outfile.write(reinterpret_cast<const char *>(&total_samples), sizeof(int));
        outfile.write(reinterpret_cast<const char *>(&max_features), sizeof(int));
        parse_svm_light_data(infile, [max_features, &outfile](int m, int c, const float* f) ->
        void {
            outfile.write(reinterpret_cast<const char *>(&c), sizeof(int));
            for (int i = 0; i < max_features; i++) {
                outfile.write(reinterpret_cast<const char *>(&i), sizeof(int));
                outfile.write(reinterpret_cast<const char *>(&f[i]), sizeof(float));
            }
            outfile << std::endl;
        });
        outfile.flush();
        outfile.close();
        infile.close();
    } else {
        std::cout << "Unable to open input or output files: " << in_file << ", " << out_file << std::endl;
    }
    return 0;
}
