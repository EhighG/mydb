#include "mydb/storage/DiskManager.hpp"
#include <spdlog/spdlog.h> // 로깅
#include <stdexcept>       // 예외처리
#include <filesystem>      // 파일 존재여부 확인용

namespace mydb {

    // 생성자 구현
    DiskManager::DiskManager(const std::string& db_file) : file_name_(db_file) {
        bool exists = std::filesystem::exists(file_name_);

        if (!exists) {
            // 파일이 없으면: 출력 전용(ofstream)으로 열어서 "생성"만 하고 바로 닫음
            std::ofstream out(file_name_, std::ios::binary);
            if (!out.is_open()) {
                throw std::runtime_error("Failed to create new file: " + file_name_ +
                                       " | Error: " + std::strerror(errno));
            }

            out.close();
            spdlog::info("Created new database file: {}", file_name_);
        }

        db_io_.open(file_name_, std::ios::binary | std::ios::in | std::ios::out);

        if (!db_io_.is_open()) {
            throw std::runtime_error("Failed to open file: " + file_name_ +
                                   " | Error: " + std::strerror(errno));
        }
    }

    //소멸자: 객체가 메모리에서 사라질 때 자동 호출
    DiskManager::~DiskManager() {
        ShutDown();
    }

    void DiskManager::ShutDown() {
        if (db_io_.is_open()) {
            db_io_.close();
        }
    }

    void DiskManager::WritePage(PageId page_id, const Page& page) {
        // Lock: 이 블록이 끝날때까지 다른 스레드는 대기
        std::scoped_lock lock(db_io_mutex_);

        // 1. 파일 내 위치 계산 (Offset)
        // 오,,, 파일도 결국 binary 모음. 여러 페이지 데이터가 붙어있고, 거기서 현재 페이지의 시작점을 찾아야 함.
        // 텍스트 파일 떠올려도 될듯.
        size_t offset = static_cast<size_t>(page_id) * PAGE_SIZE;

        // 2. 커서 이동 (Seek)
        // seekp = seek put = 쓸 위치로 이동
        db_io_.seekp(offset);

        // 3. 데이터 쓰기 (혹시 이미 데이터가 있던 페이지라면, overwrite)
        // reinterpret_cast? 이해 안 갔음
        db_io_.write(page.get_data(), PAGE_SIZE);

        // 4. 즉시 디스크 반영(Flush) (영속화)
        db_io_.flush();

        // 아 락은 저렇게 얻어오면, 블록 끝나면 자동해제인가봄.
    }

    void DiskManager::ReadPage(PageId page_id, Page& page) {
        std::scoped_lock lock(db_io_mutex_); //읽기 쓰기 구분없는 락인가봄.

        size_t offset = static_cast<size_t>(page_id) * PAGE_SIZE;

        // 파일 크기 체크 (읽으려는 위치가 파일 끝보다 뒤면 안되니까)
        db_io_.seekg(0, std::ios::end); // 끝으로 이동
        size_t file_size = db_io_.tellg(); // 현재 위치(크기)

        if (offset >= file_size) {
            throw std::runtime_error("ReadPage: PageId out of bound");
        }

        // seekg = seek get = 읽을 위치로 이동
        db_io_.seekg(offset);

        // 데이터 읽어서 page 변수에 채워넣기
        // 읽어서 수정해야 하므로 const_cast 하지 않음
        db_io_.read(page.get_data(), PAGE_SIZE);

        // 읽은 바이트 수 확인
        if (db_io_.bad() || db_io_.fail()) {
            spdlog::error("I/O error while reading page {}", page_id);
        }
    }

    // 다음 페이지 ID 할당 (단순히 파일 크기 늘리는 역할)
    PageId DiskManager::AllocatePage() {
        std::scoped_lock lock(db_io_mutex_);

        // 현재 파일 끝으로 이동
        db_io_.seekp(0, std::ios::end);
        size_t file_size = db_io_.tellp();

        // 현재 몇 번째 페이지까지 있는지 계산
        PageId next_page_id = file_size / PAGE_SIZE;

        // 모던 C++ 방식 : vector, array 사용(스택 오버플로우 방지)
        std::vector<char> buf(PAGE_SIZE, 0);

        db_io_.write(buf.data(), PAGE_SIZE);
        // // 파일 크기를 16KB만큼 늘리기 위해 0으로 채운 데이터 사용
        // // 임시 버퍼(모두 0)
        // char buf[PAGE_SIZE] = {0};
        // db_io_.write(buf, PAGE_SIZE);
        db_io_.flush();

        return next_page_id;
    }
}