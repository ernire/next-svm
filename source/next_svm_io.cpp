//
// Created by Ernir Erlingsson on 27.9.2018.
//

#include <cstring>
#include <sstream>
#include <fstream>
#include <iostream>
#include "next_svm_io.h"

int get_part_size(const int part_index, const int number_of_samples, const int number_of_parts) {
    int part = (number_of_samples / number_of_parts);
    int reserve = number_of_samples % number_of_parts;
    // Some processes will need one more sample if the data size does not fit completely with the number of processes
    if (reserve > 0 && reserve < part_index - 1) {
        return part + 1;
    }
    return part;
}

int get_block_start_offset(const int part_index, const int number_of_samples, const int number_of_blocks) {
    int offset = 0;
    for (int i = 0; i < part_index; i++) {
        offset += get_part_size(part_index, number_of_samples, number_of_blocks);
    }
    return offset;
}

// Single thread assumed
void parse_svm_light_data(std::istream &is, const std::function<void(int, int, float *)> &sample_callback) {
    std::string line;
    int c, i, m = 0;
    float f;
    float sample_features[512];

    is.clear();
    is.seekg(0, std::istream::beg);
    while (std::getline(is, line)) {
        memset(sample_features, 0, sizeof(sample_features));
        std::istringstream iss(line);
        // class
        iss >> c;
        while (!iss.eof()) {
            iss >> i;
            // index starts at 1, therefore we decrement by one
            i--;
            if (i > m) m = i;
            iss.ignore(2, ':');
            iss >> f;
            sample_features[i] = f;
        }
        sample_callback(m, c, sample_features);
    }
}

std::streampos get_file_size(const char *filePath) {

    std::streampos fsize = 0;
    std::ifstream file(filePath, std::ios::in | std::ios::binary);

    fsize = file.tellg();
    file.seekg(0, std::ios::end);
    fsize = file.tellg() - fsize;
    file.close();

    return fsize;
}

bool convert_light_to_bin(char *in_file, char *out_file, const std::function<void(int, int, int)> &meta_callback) {
    std::vector<float *> features;
    std::vector<int> classes;
    int max_features = 0;
    int total_samples = 0;
    int zero_features = 0;
    std::ifstream is(in_file, std::ios::in | std::ifstream::binary);
    std::ofstream os(out_file, std::ios::out | std::ofstream::binary);
    if (!is.is_open() || !os.is_open()) {
        return false;
    }
    parse_svm_light_data(is, [&max_features](int m, int c, float *f) -> void {
        if (m > max_features) max_features = m;
    });
    parse_svm_light_data(is, [max_features, &total_samples, &zero_features](int m, int c, const float *f) -> void {
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
    parse_svm_light_data(is, [max_features, &os](int m, int c, const float *f) -> void {
        os.write(reinterpret_cast<const char *>(&c), sizeof(int));
    });
    parse_svm_light_data(is, [max_features, &os](int m, int c, const float *f) -> void {
        for (int i = 0; i <= max_features; i++) {
            os.write(reinterpret_cast<const char *>(&f[i]), sizeof(float));
        }
    });
    os.flush();
    os.close();
    is.close();
    return true;
}


/*
 * unsigned int count5(std::istream &infile, char c) {
    static char buffer[8192];
    unsigned int count = 0;

    while (infile.read(buffer, sizeof(buffer)))
        count += std::count(buffer, buffer+infile.gcount(), c);
    count += std::count(buffer, buffer+infile.gcount(), c);
    return count;
}
 */

next_svm_data::next_svm_data(char *in_file, int number_of_blocks, int block_index)
        : in_file(in_file),
          number_of_blocks(number_of_blocks),
          block_index(block_index) {
}


//next_svm_data::next_svm_data(char *in_file, int number_of_blocks, int block_index, int sample_size)
//        : in_file(in_file),
//          number_of_blocks(number_of_blocks),
//          block_index(block_index),
//          buffer_sample_size(sample_size) {
//
//}

void next_svm_data::init_meta_data(std::istream &is) {
    is.read((char *) &number_of_samples, sizeof(int));
    is.read((char *) &number_of_features, sizeof(int));
    // calculate this parts data offsets
//    current_offset = get_block_start_offset(block_index, number_of_samples, number_of_blocks) + 2 * sizeof(int);
    int block_start_offset = get_block_start_offset(block_index, number_of_samples, number_of_blocks);
    class_offset = (block_start_offset + 2) * sizeof(int);
    feature_offset = (number_of_samples+2) * sizeof(int) + block_start_offset * sizeof(float);
//    char* in_buffer; = new char[buffer_sample_size * (number_of_features * sizeof(float))];
}

bool
next_svm_data::read_next_samples(int max_samples, const std::function<void(int*, float**, int, int)> &read_callback) {
    std::ifstream ifs(in_file, std::ios::in | std::ifstream::binary);
    if (class_offset == -1) {
        init_meta_data(ifs);
    }
    auto *features = new float[max_samples];
    auto *classes = new int[max_samples];
    char* in_buffer = new char[max_samples * (number_of_features * sizeof(float))];
    unsigned int read_count = 0;
    // read the classifications
    ifs.seekg(class_offset, std::istream::beg);
    if (!ifs.read(in_buffer, std::min(sizeof(in_buffer), max_samples* sizeof(int)))) {
        return false;
    }
    class_offset += ifs.gcount();
    read_count += ifs.gcount();
    // read the features
    ifs.seekg(feature_offset, std::istream::beg);
    if (!ifs.read(in_buffer, std::min(sizeof(in_buffer), max_samples*number_of_features* sizeof(float)))) {
        return false;
    }
    feature_offset += ifs.gcount();
    read_count += ifs.gcount();



    ifs.close();
    delete[] features;
    delete[] classes;
    return true;
}

//        double a[9] = { 1, 2, 3, 4, 5, 6, 7, 8, 9};
//        double (*b)[3] = reinterpret_cast<double (*)[3]>(a);
//        memcpy(data, in_buffer, static_cast<size_t>(ifs.gcount()));
