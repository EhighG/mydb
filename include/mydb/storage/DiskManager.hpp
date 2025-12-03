#pragma once // 중복 포함 방지

#include <string>
#include <fstream>
#include <mutex> // 스레드 동기화
#include "mydb/storage/Page.hpp"

namespace mydb {
    /**
     * 디스크상의 파일에 Page read/write
     */
    class DiskManager {
    public:
        // 생성자: DB파일 열기
        // db_file: 파일 경로
        explicit DiskManager(const std::string& db_file);

        // 소멸자: 파일 닫기
        ~DiskManager();

        // 특정 페이지 ID의 데이터를 읽어서 page객체에 적재
        void ReadPage(PageId page_id, Page& page);

        // page객체의 데이터를 디스크의 해당 ID 위치에 write
        void WritePage(PageId page_id, const Page& page);

        // 현재 파일 크기를 기준으로, 다음에 할당할 Page ID를 반환
        PageId AllocatePage();

        // 파일 닫기 (소멸자에서 호출되지만, 명시적으로 닫기도 가능)
        void ShutDown();

    private:
        std::string file_name_;
        std::fstream db_io_; // 파일 입출력용 통로(?) 자바의 입출력 Stream처럼

        // 동시성 제어용 Lock 객체
        // 여러 스레드의 동시 write 방지
        std::mutex db_io_mutex_;
    };
}
