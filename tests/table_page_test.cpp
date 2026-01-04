#include <gtest/gtest.h>
#include "mydb/storage/TablePage.hpp"

namespace mydb {

    TEST(TablePageTest, InsertTupleTest) {
        // 페이지 생성, 초기화
        TablePage page;
        // BufferPool 없이 테스트함. 실제와 달리 페이지 데이터를 직접 0으로 밀어버림
        page.Init(100, INVALID_PAGE_ID, INVALID_PAGE_ID);

        // 튜플 생성
        char raw_data[] = "Hello World!";
        Tuple tuple(raw_data, sizeof(raw_data));

        // 삽입 시도
        uint16_t slot_id;
        bool result = page.InsertTuple(tuple, &slot_id); // slot_id에 삽입된 데이터의 슬롯 id 받아옴

        // 검증
        // 1. 성공여부
        EXPECT_TRUE(result);
        EXPECT_EQ(slot_id, 0);
        EXPECT_EQ(page.GetHeader()->num_slots_, 1);

        // 2. 슬롯 정보 유효여부
        Slot* slots = page.GetSlotArray();
        EXPECT_EQ(slots[0].length_, sizeof(raw_data));
        // 현재 offset위치 검증
        EXPECT_EQ(slots[0].offset_, PAGE_SIZE - sizeof(raw_data));

        // 3. 실제로 데이터가 해당 위치에 있는지?
        char* data_ptr = page.get_data() + slots[0].offset_;
        EXPECT_EQ(std::memcmp(data_ptr, raw_data, sizeof(raw_data)), 0); //0: 일치

        // 4. 하나 더 넣어보기
        char raw_data2[] = "Second Tuple";
        Tuple tuple2(raw_data2, sizeof(raw_data2));

        result = page.InsertTuple(tuple2, &slot_id);

        // 검증
        EXPECT_TRUE(result);
        EXPECT_EQ(slot_id, 1);
        EXPECT_EQ(page.GetHeader()->num_slots_, 2);

        EXPECT_EQ(slots[1].offset_, PAGE_SIZE - sizeof(raw_data) - sizeof(raw_data2));

        data_ptr = page.get_data() + slots[1].offset_;
        EXPECT_EQ(std::memcmp(data_ptr, raw_data2, sizeof(raw_data2)), 0);
    }

    TEST(TablePageTest, DeleteAndGetTupleTest) {
        TablePage page;
        page.Init(100);

        // 1. 데이터 준비
        char data1[] = "Data 1";
        char data2[] = "Data 222"; // 길이 다르게
        char data3[] = "Data 33333";

        uint16_t slot1, slot2, slot3;

        // 2. 삽입
        page.InsertTuple(Tuple(data1, sizeof(data1)), &slot1);
        page.InsertTuple(Tuple(data2, sizeof(data2)), &slot2);
        page.InsertTuple(Tuple(data3, sizeof(data3)), &slot3);

        EXPECT_EQ(page.GetHeader()->num_slots_, 3);

        // 3. 조회 테스트
        Tuple result_tuple;

        // slot 1 조회 -> 성공
        EXPECT_TRUE(page.GetTuple(slot1, &result_tuple));
        EXPECT_EQ(result_tuple.GetSize(), sizeof(data1));
        EXPECT_EQ(std::memcmp(result_tuple.GetData(), data1, sizeof(data1)), 0);

        // 4. 삭제 테스트 (Slot 2 삭제)
        EXPECT_TRUE(page.MarkDelete(slot2));

        // 5. 삭제한걸 조회 -> 실패
        EXPECT_FALSE(page.GetTuple(slot2, &result_tuple));

        // slot 1, 3은 정상이어야 함
        EXPECT_TRUE(page.GetTuple(slot1, &result_tuple));
        EXPECT_EQ(std::memcmp(result_tuple.GetData(), data1, sizeof(data1)), 0);

        EXPECT_TRUE(page.GetTuple(slot3, &result_tuple));
        EXPECT_EQ(std::memcmp(result_tuple.GetData(), data3, sizeof(data3)), 0);

        // 6. 삭제된걸 다시 삭제 시도 시, 실패
        EXPECT_FALSE(page.MarkDelete(slot2));
    }
}