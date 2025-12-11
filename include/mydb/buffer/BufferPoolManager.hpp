#pragma once

#include <list>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "mydb/buffer/LRUReplacer.hpp"
#include "mydb/storage/DiskManager.hpp"
#include "mydb/storage/Page.hpp"

namespace mydb {

    class BufferPoolManager {
    public:
        /**
         * @param pool_size 버퍼 풀에 동시에 둘 수 있는 페이지 수
         */
        BufferPoolManager(size_t pool_size, DiskManager *disk_manager);

        ~BufferPoolManager();

        /**
         * @brief 디스크에서 페이지를 가져옴
         * 메모리에 있으면, 포인터 반환
         * 없으면 디스크에서 메모리로 로드한 후, 포인터 반환
         */
        Page* FetchPage(PageId page_id);

        /**
         * @brief 특정 페이지 사용이 끝났음(언제든 치워도 됨)을 알림
         * @param is_dirty 데이터가 디스크에서 읽은 값과 다른지(수정됐는지) 여부
         */
        bool UnpinPage(PageId page_id, bool is_dirty);

        /**
         * @brief 디스크의 파일 크기를 해당 페이지 크기만큼 늘리고, 메모리에 올림 + 새 ID 생성해서 리턴
         */
        Page* NewPage(PageId* page_id);

        /**
         * @brief 특정 페이지를 디스크에 강제로 쓰기
         */
        bool FlushPage(PageId page_id);

        /**
         * @brief <b>메모리 상의</b> 특정 페이지 데이터 삭제
         */
        bool DeletePage(PageId page_id);

    private:
        /**
         * @brief 빈 프레임 id 가져옴
         * 1. free_list_에 빈 게 있으면 쓰고,
         * 2. 없으면 replacer를 통해 다른 페이지를 내보내고 새 공간 확보
         */
        bool FindFreeFrameFromVictim(FrameId* frame_id);

        size_t pool_size_;

        Page* pages_;

        // 외부 주입받거나, 내부에서 생성
        DiskManager* disk_manager_;
        LRUReplacer* replacer_;

        // PageId -> FrameId 매핑 테이블
        std::unordered_map<PageId, FrameId> page_table_;

        std::list<FrameId> free_list_;

        std::mutex mutex_;
    };
}