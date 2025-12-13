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
}