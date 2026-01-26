#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <random>
#include <cstring>

#include "mydb/common/config.hpp"
#include "mydb/storage/TableHeap.hpp"
#include "mydb/buffer/BufferPoolManager.hpp"
#include "mydb/buffer/LRUReplacer.hpp"
#include "mydb/storage/DiskManager.hpp"

namespace mydb {

    // 테스트용 헬퍼 함수 : 문자열로 튜플 생성
    Tuple ConstructTuple(const std::string& data) {
        // 테스트용이므로, 스키마 신경쓰지 않고 문자열 그대로 바이트 배열로 저장
        std::vector<char> val(data.begin(), data.end());
        return Tuple(val);
    }

    TEST(TableHeapTest, InsertAndGetTupleTest) {
        const std::string db_name = "test.db";

        // 0. 기존 파일 제거
        remove(db_name.c_str());

        // 1. 시스템 컴포넌트 초기화
        auto disk_manager = std::make_unique<DiskManager>(db_name);
        auto bpm = std::make_unique<BufferPoolManager>(10, disk_manager.get());

        // 2. TableHeap 생성
        auto table_heap = std::make_unique<TableHeap>(bpm.get());

        // 3. 단일 데이터 테스트
        std::string test_data = "Hello, MySQL Clone!";
        Tuple tuple = ConstructTuple(test_data);
        RID rid;

        // 삽입
        EXPECT_TRUE(table_heap->InsertTuple(tuple, &rid));
        std::cout << "First Tuple Inserted at: " << rid.ToString() << std::endl;

        // 조회
        Tuple result_tuple;
        EXPECT_TRUE(table_heap->GetTuple(rid, &result_tuple));

        // 검증
        // Size 확인
        EXPECT_EQ(tuple.GetSize(), result_tuple.GetSize());

        // 내용 비교
        EXPECT_EQ(std::memcmp(tuple.GetData(), result_tuple.GetData(), tuple.GetSize()), 0);

        // 4. 대량 삽입 테스트 (페이지 확장 테스트)
        // 단일 페이지 메모리(16KB)보다 크게 넣어서, 새 페이지가 잘 추가/연결되는지 확인
        int num_tuples = 1000;
        std::vector<RID> rids;
        std::vector<std::string> raw_data;

        for (int i = 0; i < num_tuples; ++i) {
            std::string s = "Tuple Data Number " + std::to_string(i);
            Tuple t = ConstructTuple(s);
            RID r;

            EXPECT_TRUE(table_heap->InsertTuple(t, &r));

            rids.push_back(r);
            raw_data.push_back(s);
        }

        std::cout << "Bulk Insert Finished. Total: " << num_tuples << "tuples." << std::endl;

        // 5. 대량 데이터 검증(데이터 일치여부 확인)
        for (int i = 0; i < num_tuples; ++i) {
            Tuple res;
            if (!table_heap->GetTuple(rids[i], &res)) {
                FAIL() << "GetTuple failed at index " << i << ", RID: " << rids[i].ToString();
            }

            // 데이터 비교
            EXPECT_EQ(res.GetSize(), raw_data[i].size());
            EXPECT_EQ(std::memcmp(res.GetData(), raw_data[i].c_str(), res.GetSize()), 0);
        }

        // 6. 삭제 시 마킹 테스트
        // 튜플 삭제
        EXPECT_TRUE(table_heap->MarkDelete(rids[0]));

        // 삭제 후 조회가 불가능해야 함
        Tuple result_tuple2;
        EXPECT_FALSE(table_heap->GetTuple(rids[0], &result_tuple2));

        // 7. 파일 정리
        remove(db_name.c_str());
    }
}