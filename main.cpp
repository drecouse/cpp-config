#include <iostream>

void example1();
void example2();

void init_tests();
void create_test();
void run_test();

int main() {
 /*   std::cout << "Running Example1: " << std::endl;
    example1();
    std::cout << std::endl;

    std::cout << "Running Example2: " << std::endl;
    example2();
    std::cout << std::endl;*/

    init_tests();
    create_test();
    run_test();
    return 0;
}
