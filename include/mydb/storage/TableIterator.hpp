#pragma once

#include <cassert>
#include "mydb/common/config.hpp"
#include "mydb/storage/RID.hpp"
#include "mydb/storage/Tuple.hpp"

namespace mydb {

    class TableHeap; // 전방 선언; TableHeap과 TableIterator가 서로 참조하는 구조일 때, 순환참조 방지

    class TableIterator {
    public:
        // 생성자 : TableHeap과 시작 위치(RID)를 받음
        TableIterator(TableHeap* table_heap, RID rid);

        TableIterator(const TableIterator& other);

        ~TableIterator() = default;

        // C++ Iterator 표준 연산자들
        const Tuple& operator*();       // 현재 가리키는 Tuple의 참조를 반환 (Tuple t = *it;)
        Tuple* operator->();            // ->는 포인터 변수에만 쓸 수 있다.
        TableIterator& operator++();    // 전위 (++it) -> Iterator 복사본을 줄 필요가 없음
        TableIterator operator++(int);  // 후위 (it++) -> 현재 위치를 가진 Iterator를 주고, 가리키는 위치는 옮겨야 하므로.

        bool operator==(const TableIterator& itr) const;
        bool operator!=(const TableIterator& itr) const;

    private:
        TableHeap* table_heap_;
        RID rid_;                   // 현재 커서 위치
        Tuple tuple_;               // 현재 데이터 캐싱 (Lazy Loading용)
    };
}