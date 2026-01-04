#include "mydb/storage/TablePage.hpp"

namespace mydb {
    /**
     *
     * @param tuple 페이지에 삽입할 row 단위의 실제 데이터
     * @param slot_id 할당된 슬롯 번호를 저장해서 돌려줌.
     * @return 성공여부
     */
    bool TablePage::InsertTuple(const Tuple& tuple, uint16_t* slot_id) {
        // 필요한 공간 계산(데이터 공간 크기 + 추가될 슬롯 하나 크기)
        uint32_t needed_space = tuple.GetSize() + sizeof(Slot);

        // 페이지에 남은 공간 체크
        if (GetFreeSpaceRemaining() < needed_space) {
            return false;
        }

        // 페이지 헤더, 슬롯 배열 가져오기
        auto* header = GetHeader();
        auto* slots = GetSlotArray();

        // 빈 공간과 데이터 영역의 경계선을 새로 추가될 튜플을 반영해서, 더 위(앞? 더 낮은 주소값)로 옮김
        header->free_space_pointer_ -= tuple.GetSize();

        uint32_t offset = header->free_space_pointer_;

        // 실제 데이터 write
        // get_data(): 페이지의 시작 주소
        // offset: 페이지 안에서, write작업을 시작할 위치
        std::memcpy(get_data() + offset, tuple.GetData(), tuple.GetSize());

        // 슬롯 추가
        uint16_t index = header->num_slots_;

        slots[index].offset_ = static_cast<uint16_t>(offset);
        slots[index].length_ = tuple.GetSize();

        // 메타데이터 갱신
        header->num_slots_++;

        // 저장된 데이터의 slot id도 기록(반환)
        *slot_id = index;
        return true;
    }

    bool TablePage::GetTuple(uint16_t slot_id, Tuple* tuple) {
        auto* header = GetHeader();

        // 1. 범위체크
        if (slot_id >= header->num_slots_) {
            return false;
        }

        // 2. 삭제여부 체크
        const Slot& slot = GetSlotArray()[slot_id];
        if (slot.length_ == 0) {
            return false;
        }

        // 3. 조회 (복사)
        *tuple = Tuple(get_data() + slot.offset_, slot.length_);

        return true;
    }

    bool TablePage::MarkDelete(uint16_t slot_id) {
        auto* header = GetHeader();

        // 1, 범위 체크
        if (slot_id >= header->num_slots_) {
            return false;
        }

        // 2. 삭제여부 체크
        Slot& slot = GetSlotArray()[slot_id];
        if (slot.length_ == 0) {
            return false;
        }

        // 3. 마킹(soft delete)
        slot.length_ = 0;
        slot.offset_ = 0;

        // free_space_pointer를 옮기는 등, 데이터 영역 범위를 변경하지 않음
        // (B+Tree 인덱스 참조 유지 + 성능 이유라는데, 뭘지?)

        return true;
    }


}