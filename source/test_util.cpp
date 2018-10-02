//
// Created by Ernir Erlingsson on 1.10.2018.
//

#include <iomanip>
#include <iostream>
#include "test_util.h"

void do_unit_test(char *string, const std::function<bool()> &test_case) {
    std::cout << std::left << std::setfill('.') << std::setw (50) << string;
    test_case() ? std::cout << "PASSED" : std::cout << "FAILED";
    std::cout << std::endl;
}