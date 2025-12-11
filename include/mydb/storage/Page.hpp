#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <string>

namespace mydb {

    // MySQL 호환 페이지 크기: 16KB
    constexpr size_t PAGE_SIZE = 16384;

    // 페이지 ID 타입 정의: 4바이트 정수(unsigned)
    using PageId = uint32_t;

    // 유효하지 않은 페이지 ID 상수
    constexpr PageId INVALID_PAGE_ID = UINT32_MAX;

    /**
     * @brief DB의 가장 기본 저장 단위
     * 디스크상의 16KB 블록 하나와 1:1 매핑
     */
    class Page {
    public:
        Page() {reset();}
        ~Page() = default;

        // 페이지 데이터를 0으로 초기화
        void reset() {
            data_.fill(0);
            page_id_ = INVALID_PAGE_ID;
            is_dirty_ = false;
            pin_count_ = 0;
        }

        // getter
        inline char* get_data() {
            return reinterpret_cast<char*>(data_.data());
        }
        inline const char* get_data() const {
            return reinterpret_cast<const char*>(data_.data());
        }

        inline PageId get_page_id() const {
            return page_id_;
        }

        // 나중에 Buffer Manager에서 사용할 메타데이터 Getter/Setter
        inline int get_pin_count() const {return pin_count_;}
        inline bool is_dirty() const { return is_dirty_;}

    private:
        // 실제 16KB 데이터가 저장되는 공간
        // alignas: 메모리 정렬 최적화(CPU 캐시 히트율 증가)
        alignas(16) std::array<uint8_t, PAGE_SIZE> data_;

        // 메모리 상에서만 관리되는 메타데이터
        PageId page_id_ = INVALID_PAGE_ID;
        int pin_count_ = 0; // 현재 이 페이지를 보고 있는 스레드 수(atomic인듯)
        bool is_dirty_ = false; // (마지막 디스크에서 쓴/읽은 시점 이후) 데이터 변경 여부. (true면 디스크에 다시 써야함)

        // BufferPoolManager가 이 private 멤버들을 관리할 수 있게 허용
        friend class BufferPoolManager;
        // [추가] SlottedPage 구현 시
        friend class TablePage;
    };
}