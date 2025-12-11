#pragma once

#include <cstring>
#include <optional>
#include "mydb/storage/Page.hpp"
#include "mydb/storage/Tuple.hpp"

namespace mydb {

    struct SlottedPageHeader {
        PageId next_page_id_ = INVALID_PAGE_ID; // Table scan 용도
        PageId prev_page_id_ = INVALID_PAGE_ID;
        uint16_t num_slots_ = 0;
        uint16_t free_space_pointer_;           // 빈 공간의 시작점(데이터는 역순으로 쌓이므로, 이 지점 앞은 빈 공간, 뒤는 데이터)
    };

    /**
     * @brief 슬롯 (메타데이터 저장)
     * Deleted된 상태로 존재할 수 있다.(데이터 삭제 시 슬롯은 soft delete함)
     */
    struct Slot {
        uint16_t offset_; // 페이지 시작점으로부터의 거리 (Byte)
        uint16_t length_; // 데이터 크기 (삭제된 경우 0)
    };

    /**
     * @brief Page객체를 래핑해서, Slotted Page처럼 씀
     */
    class TablePage : public Page { // 상속
    public:
        // 페이지 생성 시 헤더 초기화
        void Init(PageId page_id, PageId prev_id = INVALID_PAGE_ID, PageId next_id = INVALID_PAGE_ID) {
            // 헤더 객체 가져오기 (메모리 재해석?)
            auto* header = GetHeader();

            header->next_page_id_ = next_id;
            header->prev_page_id_ = prev_id;
            header->num_slots_ = 0;
            header->free_space_pointer_ = PAGE_SIZE; // 데이터 영역은 맨 끝부터
        }

        // 헤더 영역의 포인터 반환
        SlottedPageHeader* GetHeader() {
            return reinterpret_cast<SlottedPageHeader*>(get_data());
        }

        // 읽기 전용 헤더
        const SlottedPageHeader* GetHeader() const {
            return reinterpret_cast<const SlottedPageHeader*>(get_data());
        }

        // 슬롯 배열(페이지 내 데이터 위치, 길이정보를 담는 메타데이터들의 배열) 포인터 반환
        Slot* GetSlotArray() {
            auto* ptr = get_data() + sizeof(SlottedPageHeader);
            return reinterpret_cast<Slot*>(ptr);
        }

        // 남은 빈 데이터 영역 크기 계산
        // = 현재 마지막 데이터 영역 지점 - 슬롯 배열 끝 지점
        uint32_t GetFreeSpaceRemaining() {
            auto* header = GetHeader();
            // 슬롯 배열 끝 위치 = 헤더 크기 + (슬롯 개수 * 개별 슬롯 크기)
            uint32_t slot_array_end = sizeof(SlottedPageHeader) + (header->num_slots_ * sizeof(Slot));

            return header->free_space_pointer_ - slot_array_end;
        }

        /**
         * @brief 튜플 삽입 (row의 데이터 메모리에 추가) -> 핵심 로직
         * @return 성공여부
         */
        bool InsertTuple(const Tuple& tuple, uint16_t* slot_id);

    private:
        // 자체 멤버변수 없음. Page의 data_만 해석해서 사용.
    };
}