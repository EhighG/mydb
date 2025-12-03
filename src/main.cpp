#include <iostream>
#include <spdlog/spdlog.h> // spdlog 라이브러리 헤더

#include "mydb/storage/Page.hpp"

int main() {
    spdlog::info("Starting MyDB...");

    // Page 인스턴스 생성
    mydb::Page my_page;

    // 사이즈 검증
    if (sizeof(my_page) >= mydb::PAGE_SIZE) {
        spdlog::info("Page created successfully. Size: {} bytes", sizeof(my_page));
    } else {
        spdlog::error("Page size is suspicious! Expected around {}, but got {}", mydb::PAGE_SIZE, sizeof(my_page));
    }

    // 데이터 조작 테스트
    char* data = my_page.get_data();
    std::string message = "Hello, Database Page!";
    std::memcpy(data, message.c_str(), message.size() + 1);

    spdlog::info("Content in Page: {}", data);

    return 0;
}
