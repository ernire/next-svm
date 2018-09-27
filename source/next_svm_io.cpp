//
// Created by Ernir Erlingsson on 27.9.2018.
//

#include <cstring>
#include <sstream>
#include <fstream>
#include "next_svm_io.h"

void parse_svm_light_data(std::istream &is, const std::function<void(int, int, float*)> &sample_callback) {
    std::string line;
    int c, i, m = 0;
    float f;
    float sample_features[512];

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
        sample_callback(m, c, sample_features);
    }
}

std::streampos get_file_size(const char *filePath){

    std::streampos fsize = 0;
    std::ifstream file( filePath, std::ios::in | std::ios::binary );

    fsize = file.tellg();
    file.seekg(0, std::ios::end );
    fsize = file.tellg() - fsize;
    file.close();

    return fsize;
}



void convert_light_to_bin(std::istream &is, std::ostream &os, const std::function<void(int, int, int)> &meta_callback) {
    std::vector<float *> features;
    std::vector<int> classes;
    int max_features = 0;
    int total_samples = 0;
    int zero_features = 0;

//    std::ifstream infile(in_file, std::ios::in | std::ifstream::binary);
//    std::ofstream outfile(out_file, std::ios::in | std::ofstream::binary);
//    if (infile.is_open() && outfile.is_open()) {

    parse_svm_light_data(is, [&max_features](int m, int c, float* f) ->
    void {
        if (m > max_features) max_features = m;
    });
    parse_svm_light_data(is, [max_features, &total_samples, &zero_features](int m, int c, const float* f) ->
    void {
        total_samples++;
        for (int i = 0; i < max_features; i++) {
            if (f[i] == 0)
                zero_features++;
        }
    });
    // callback the number of features and the data sparsity
    meta_callback(total_samples, max_features, zero_features);

    os.write(reinterpret_cast<const char *>(&total_samples), sizeof(int));
    os.write(reinterpret_cast<const char *>(&max_features), sizeof(int));
    parse_svm_light_data(is, [max_features, &os](int m, int c, const float* f) ->
    void {
        os.write(reinterpret_cast<const char *>(&c), sizeof(int));
        for (int i = 0; i < max_features; i++) {
            os.write(reinterpret_cast<const char *>(&i), sizeof(int));
            os.write(reinterpret_cast<const char *>(&f[i]), sizeof(float));
        }
        os << std::endl;
    });
    os.flush();
//        outfile.flush();
//        outfile.close();
//        infile.close();
//    } else {
//        std::cout << "Unable to open input or output files: " << in_file << ", " << out_file << std::endl;
//    }
}