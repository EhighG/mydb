#include "mydb/storage/TableHeap.hpp"
#include <stdexcept>

namespace mydb {

    TableHeap::TableHeap(BufferPoolManager* bpm) : bpm_(bpm) {
        // 1. 첫 페이지 할당
        PageId first_page_id;
        Page* page = bpm_->NewPage(&first_page_id); // &와 * 복습: first_page_id는 NewPage() 내부에서 변경될 수 있고, 생성된 페이지 id가 담겨 나올 것

        if (page == nullptr) {
            throw std::runtime_error("TableHeap: Failed to allocate first page.");
        }

        // 2. 초기화
        auto* table_page = reinterpret_cast<TablePage*>(page);
        table_page->Init(first_page_id, INVALID_PAGE_ID, INVALID_PAGE_ID);

        first_page_id_ = first_page_id;

        // 3. 반납
        bpm_->UnpinPage(first_page_id, true);
    }

    bool TableHeap::InsertTuple(const Tuple& tuple, RID* rid) {
        PageId current_page_id = first_page_id_;

        while (current_page_id != INVALID_PAGE_ID) {
            Page* page = bpm_->FetchPage(current_page_id);
            if (page == nullptr) return false;

            auto* table_page = reinterpret_cast<TablePage*>(page);

            // 1. 삽입 시도
            uint16_t slot_id;
            if (table_page->InsertTuple(tuple, &slot_id)) {
                rid->Set(current_page_id, slot_id);
                bpm_->UnpinPage(current_page_id, true); // dirty=true
                return true;
            }

            // 2. 실패 (꽉 참) -> 다음 페이지 확인
            PageId next_page_id = table_page->GetHeader()->next_page_id_;

            // 다음 페이지가 없으면, 새로 생성해서 연결
            if (next_page_id == INVALID_PAGE_ID) {
                PageId new_page_id;
                Page* new_page = bpm_->NewPage(&new_page_id);
                if (new_page == nullptr) {
                    bpm_->UnpinPage(current_page_id, false);
                    return false;
                }

                // 새 페이지 초기화 & 연결
                auto* new_table_page = reinterpret_cast<TablePage*>(new_page);
                new_table_page->Init(new_page_id, current_page_id, INVALID_PAGE_ID);

                // 현재 페이지의 next 포인터 업데이트
                table_page->GetHeader()->next_page_id_ = new_page_id;

                // 현재 페이지 반납 (링크 수정했으므로 dirty = true)
                bpm_->UnpinPage(new_page_id, true);

                // 페이지 생성 성공한 경우에도, 코드 단순화를 위해 튜플 삽입은 다음번 루프에서 처리
                bpm_->UnpinPage(current_page_id, true);
                current_page_id = new_page_id;
            } else {
                // 다음 페이지가 있으면, 이동
                bpm_->UnpinPage(current_page_id, false);
                current_page_id = next_page_id;
            }
        }
        return false;
    }

    bool TableHeap::GetTuple(const RID& rid, Tuple* tuple) {
        PageId page_id = rid.GetPageId();
        Page* page = bpm_->FetchPage(page_id);
        if (page == nullptr) return false;

        auto* table_page = reinterpret_cast<TablePage*>(page);

        // 조회
        bool result = table_page->GetTuple(rid.GetSlotId(), tuple);

        // 반납
        bpm_->UnpinPage(page_id, false);

        return result;
    }

    bool TableHeap::MarkDelete(const RID& rid) {
        PageId page_id = rid.GetPageId();
        Page* page = bpm_->FetchPage(page_id);
        if (page == nullptr) return false;

        auto* table_page = reinterpret_cast<TablePage*>(page);

        // 삭제 마킹
        bool result = table_page->MarkDelete(rid.GetSlotId());

        // 반납
        bpm_->UnpinPage(page_id, true);

        return result;
    }
}