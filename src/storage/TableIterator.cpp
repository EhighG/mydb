#include "mydb/storage/TableIterator.hpp"
#include "mydb/storage/TableHeap.hpp"
#include "mydb/storage/TablePage.hpp"

namespace mydb {

    TableIterator::TableIterator(TableHeap* table_heap, RID rid)
        : table_heap_(table_heap), rid_(rid) {}

    TableIterator::TableIterator(const TableIterator& other)
        : table_heap_(other.table_heap_), rid_(other.rid_) {
        // 튜플 데이터가 있으면 복사 (Deep copy 비용 고랴)
        if (other.tuple_.GetSize() > 0) {
            tuple_ = Tuple(other.tuple_);
        }
    }

    bool TableIterator::operator==(const TableIterator& itr) const {
        // 순회 대상, 현재 커서(위치)가 같으면 같음
        return rid_ == itr.rid_ && table_heap_ == itr.table_heap_;
    }

    bool TableIterator::operator!=(const TableIterator& itr) const {
        return !(*this == itr);
    }

    const Tuple& TableIterator::operator*() {
        // 캐시된 튜플이 없으면 로드 (Lazy Loading)
        if (tuple_.GetSize() == 0 && rid_.GetPageId() != INVALID_PAGE_ID) {
            // TableHeap의 friend 권한으로 bpm_ 접근
            auto bpm = table_heap_->bpm_;

            Page* page = bpm->FetchPage(rid_.GetPageId());
            auto* table_page = reinterpret_cast<TablePage*>(page);

            // 데이터를 읽어서 tuple_에 캐싱
            table_page->GetTuple(rid_.GetSlotId(), &tuple_);

            // 읽기 전용이므로 dirty = false
            bpm->UnpinPage(rid_.GetPageId(), false);
        }
        return tuple_;
    }

    // 화살표 연산자(->)
    const Tuple* TableIterator::operator->() {
        return &this->operator*();
    }

    // 전위 증가(++it)
    TableIterator& TableIterator::operator++() {
        auto bpm = table_heap_->bpm_;
        PageId page_id = rid_.GetPageId();

        Page* page = bpm->FetchPage(page_id);
        auto* table_page = reinterpret_cast<TablePage*>(page);

        RID next_rid;
        // 1. 현재 페이지 안에서 다음 슬롯 찾기
        if (table_page->GetNextTupleRid(rid_, &next_rid)) {
            rid_ = next_rid;
            bpm->UnpinPage(page_id, false);
        }
        // 2. 없으면 다음 페이지들 탐색
        else {
            PageId next_page_id = table_page->GetNextPageId();
            bpm->UnpinPage(page_id, false);

            // 다음 유효한 튜플을 찾을 때까지 페이지 이동
            while (next_page_id != INVALID_PAGE_ID) {
                Page* next_page = bpm->FetchPage(next_page_id);
                auto* next_table_page = reinterpret_cast<TablePage*>(next_page);

                RID first_rid;
                // 페이지에 유효한 튜플이 하나라도 있으면 stop
                if (next_table_page->GetFirstTupleRid(&first_rid)) {
                    rid_ = first_rid;
                    bpm->UnpinPage(next_page_id, false);
                    break;
                }

                // 현재 페이지도 비었으면, 페이지 이동
                next_page_id = next_table_page->GetNextPageId();
                bpm->UnpinPage(next_page_id, false);
            }

            // 끝까지 갔는데도 없으면, End() 상태로 설정
            if (next_page_id == INVALID_PAGE_ID) {
                rid_.Set(INVALID_PAGE_ID, 0);
            }
        }

        // 이동했으니 캐시 초기화 (다음 * 호출 시 새로 읽도록)
        tuple_ = Tuple();

        return *this;
    }

    // 후위 증가(it++)
    TableIterator TableIterator::operator++(int) {
        TableIterator clone(*this);
        ++(*this);
        return clone;
    }

}