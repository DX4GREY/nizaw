#include <iostream>

int run_core_result_tests();
int run_core_env_tests();
int run_core_platform_tests();
int run_core_version_tests();
int run_core_log_tests();
int run_system_info_tests();

int main() {
    int failures = 0;
    failures += run_core_result_tests();
    failures += run_core_env_tests();
    failures += run_core_platform_tests();
    failures += run_core_version_tests();
    failures += run_core_log_tests();
    failures += run_system_info_tests();

    if (failures != 0) {
        std::cerr << failures << " test(s) failed" << std::endl;
        return 1;
    }

    std::cout << "All Nizaw core/system tests passed" << std::endl;
    return 0;
}
