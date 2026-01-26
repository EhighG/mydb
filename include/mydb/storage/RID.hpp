#pragma once

#include <cstdint>
#include <string>
#include <sstream> // for string stream
#include "mydb/common/config.hpp" // PAGE_ID 등을 위해

namespace mydb {
    /**
     * @brief Record ID (PageId + SlotId)
     * 테이블 내에서 튜플의 위치 식별
     */
    class RID {
    public:
        // 기본 생성자: 유효하지 않은 값으로 초기화
        RID() : page_id_(INVALID_PAGE_ID), slot_id_(0) {}

        RID(PageId page_id, uint16_t slot_id) : page_id_(page_id), slot_id_(slot_id) {}

        // Getter
        inline PageId GetPageId() const { return page_id_; } // const는, 반환 값이 아닌 식만 const이면 되는건가?
        inline uint16_t GetSlotId() const { return slot_id_; }

        inline void Set(PageId page_id, uint16_t slot_id) {
            page_id_ = page_id;
            slot_id_ = slot_id;
        }

        // 디버깅용 출력
        std::string ToString() const {
            std::stringstream ss;
            ss << "RID(" << page_id_ << ", " << slot_id_ << ")";
            return ss.str();
        }

        // 비교 연산자 오버라이딩(테스트, Map, Set 등에 필요)
        bool operator==(const RID& other) const {
            return page_id_ == other.page_id_ && slot_id_ == other.slot_id_;
        }

    private:
        PageId page_id_;
        uint16_t slot_id_;
    };
}