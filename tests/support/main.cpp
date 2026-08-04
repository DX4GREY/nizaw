#include <iostream>

int run_core_result_tests();
int run_core_env_tests();
int run_core_platform_tests();
int run_core_version_tests();
int run_core_log_tests();
int run_system_info_tests();
int run_process_tests();
int run_filesystem_tests();
int run_storage_tests();
int run_network_tests();
int run_service_tests();
int run_security_tests();
int run_plugin_tests();
int run_agent_tests();

int main() {
    int failures = 0;
    failures += run_core_result_tests();
    failures += run_core_env_tests();
    failures += run_core_platform_tests();
    failures += run_core_version_tests();
    failures += run_core_log_tests();
    failures += run_system_info_tests();
    failures += run_process_tests();
    failures += run_filesystem_tests();
    failures += run_storage_tests();
    failures += run_network_tests();
    failures += run_service_tests();
    failures += run_security_tests();
    failures += run_plugin_tests();
    failures += run_agent_tests();

    if (failures != 0) {
        std::cerr << failures << " test(s) failed" << std::endl;
        return 1;
    }

    std::cout << "All Nizaw core/system/agent tests passed" << std::endl;
    return 0;
}
