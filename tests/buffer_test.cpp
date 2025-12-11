#include <gtest/gtest.h>
#include <filesystem>
#include <string>
#include <random>

#include "mydb/buffer/BufferPoolManager.hpp"
#include "mydb/buffer/LRUReplacer.hpp"

namespace mydb {

    // 1. LRU Replacer 단위테스트
    TEST(BufferPoolTest, LRUReplacerTest) {
        // 크기 3짜리 LRU 생성
        LRUReplacer lru(3);

        FrameId victim;

        // 초기엔 비어있으니, 쫓아낼 게 없어야 함
        EXPECT_EQ(lru.Size(), 0);
        EXPECT_FALSE(lru.Victim(&victim));

        // 1,2,3번 프레임 Unpin -> LRU 목록(=삭제 가능 대상 리스트)에 추가
        lru.Unpin(1);
        lru.Unpin(2);
        lru.Unpin(3);

        EXPECT_EQ(lru.Size(), 3);

        //1번 프레임을 사용함 -> 삭제 가능 대상에서 제외
        lru.Pin(1);
        EXPECT_EQ(lru.Size(), 2);

        // 순서 맞게, 가장 오래 안 쓰인 프레임이 victim으로 선택되는지
        EXPECT_TRUE(lru.Victim(&victim));
        EXPECT_EQ(victim, 2);

        EXPECT_TRUE(lru.Victim(&victim));
        EXPECT_EQ(victim, 3);

        // 1번 프레임만 남았지만 현재 사용중이므로, 이젠 삭제대상 선택 실패해야 함
        EXPECT_FALSE(lru.Victim(&victim));
    }

    // BufferPoolManager 부분 전체 테스트(페이지 생성, 데이터 쓰기, 쫓아내기, 다시 불러오기
    TEST(BufferPoolTest, BufferPoolIntegrationTest) {
        const std::string db_name = "test.db";

        // 해당 db_name을 가진 파일이 있으면 비우기
        if (std::filesystem::exists(db_name)) {
            std::filesystem::remove(db_name);
        }

        DiskManager disk_manager(db_name);
        BufferPoolManager bpm(5, &disk_manager);

        // 1. 페이지 하나 생성해서 데이터 쓰기
        PageId page_id_0;
        Page* page0 = bpm.NewPage(&page_id_0);
        /*
         * EXPECT_~와 ASSERT_~의 차이
         * EXPECT~는, 실패하면 기록은 남기고, 테스트는 계속 진행
         * ASSERT~는, 실패하면 기록 남기고, 테스트 중단
         */
        ASSERT_NE(page0, nullptr);
        EXPECT_EQ(page_id_0, 0);


        char data[] = "Hello World";
        std::memcpy(page0->get_data(), data, sizeof(data));

        bpm.UnpinPage(page_id_0, true);

        for (int i = 1; i < 5; i++) {
            PageId temp_page_id;
            Page* p = bpm.NewPage(&temp_page_id);
            ASSERT_NE(p, nullptr);
            bpm.UnpinPage(temp_page_id, false);
        }

        // 현재 버퍼 풀: [0, 1, 2, 3, 4] (모두 Unpin 상태)

        // 새 페이지를 생성하면, 0번 페이지가 쫓겨남
        // 페이지를 메모리에서 삭제할 때 수정사항 있으면 Flush하기로 했으므로,
        // 이 때 디스크의 0번 페이지 공간에 위의 Hello World가 기록되어야 함
        PageId page_id_5;
        Page* page5 = bpm.NewPage(&page_id_5);
        ASSERT_NE(page5, nullptr);
        bpm.UnpinPage(page_id_5, true);

        // 0번 페이지를 다시 불러왔을 때, Hello World가 불러와져야 함
        Page* page0_2 = bpm.FetchPage(0);
        ASSERT_NE(page0_2, nullptr);

        EXPECT_EQ(std::strcmp(page0_2->get_data(), "Hello World"), 0);

        bpm.UnpinPage(0, false);

        // 테스트 종료 후 파일 삭제
        disk_manager.ShutDown();
        std::filesystem::remove(db_name);
    }
}