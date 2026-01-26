#pragma once

#include "mydb/buffer/BufferPoolManager.hpp"
#include "mydb/storage/TablePage.hpp"
#include "mydb/storage/Tuple.hpp"
#include "mydb/storage/RID.hpp"

namespace mydb {
    /**
     * @brief 디스크상의 테이블 데이터를 관리하는 메인 클래스
     * TablePage들을 LinkedList 형태로 관리
     */
    class TableHeap {
    public:
        /**
         * @brief 테이블 힙 생성
         * @param bpm 버퍼 풀 매니저(페이지 관리용)
         */
        TableHeap(BufferPoolManager* bpm);

        /*
         * out 변수에 & 말고 * 붙이는 이유
         * 1. 사용하는 쪽 코드에 명시
         * 2. null값 사용 편의성
         * *을 붙이는 경우, 함수 호출 시 InsertTuple(tuple, &rid) 처럼 주소값임을 명시해야 하고, InsertTuple(tuple, nullptr) 처럼 사용 가능.
         * &을 붙이는 경우, 함수 호출 시 InsertTuple(tuple, rid) 처럼 사용함. 그리고 null값을 넣고 싶으면, 따로 더미 객체를 생성해야 함.
         */

        /**
         * @brief 튜플 삽입
         * @param tuple 저장할 데이터
         * @param rid 저장된 위치를 반환받을 포인터
         * @return 성공 여부
         */
        bool InsertTuple(const Tuple& tuple, RID* rid);

        /**
         * @brief 튜플 조회
         * @param rid 읽을
         * @param tuple
         * @return
         */
        bool GetTuple(const RID& rid, Tuple* tuple);

        /**
         * @brief 튜플 삭제 (tombstone 마킹)
         */
        bool MarkDelete(const RID& rid);

        /*
         * 함수에 붙는 const의 의미 = 해당 함수에선 멤버변수 변경 안함
         * const는 왼쪽에 있는 요소를, 맨 앞에 오는 const라면(= 왼쪽 요소가 없다면) 오른쪽 요소를 불변으로 만듦
         * 컴파일러가 멤버 함수를 해석할 때, this가 추가됨
         * PageId GetFirstPageId() {...} -> PageId GetFirstPageId(TableHeap* const this) {...}
         * -> 주소값은 바꿀 수 없고, 가리키는 객체(= 자신)는 바꿀 수 있는 포인터
         * 여기에 const를 붙이면,
         * PageId GetFirstPageId() const {...} -> PageId GetFirstPageId(const TableHeap* const this) {...} 가 됨.
         * -> 주소값도 바꿀 수 없고, 가리키는 객체도 바꿀 수 없는 포인터가 됨.
         */

        /**
         * @brief Iterator 사용 목적
         * @return
         */
        PageId GetFirstPageId() const { return first_page_id_; }

    private:
        BufferPoolManager* bpm_;
        PageId first_page_id_;
    };
}