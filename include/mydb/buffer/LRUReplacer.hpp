#pragma once

#include <list>             // Linked List
#include <unordered_map>    // HashMap
#include <mutex>            // 동시성 제어
#include <optional>

namespace mydb {

    using FrameId = uint32_t;

    /**
     * @brief 캐시? 공간이 다 찼을 때, 마지막 사용 시점이 가장 오래된 데이터를 버림
     * (Least Recently Used)
     */
    class LRUReplacer {
    public:
        explicit LRUReplacer(size_t num_pages);

        ~LRUReplacer() = default;

        bool Victim(FrameId* frame_id);

        void Unpin(FrameId frame_id);

        void Pin(FrameId frame_id);

        /**
         * 현재 관리대상인(비워질 가능성이 있는) 프레임 수
         */
        size_t Size();

    private:
        std::mutex mutex_;

        // 마지막 사용한지 오래된 순으로 정렬
        std::list<FrameId> lru_list_;

        // FrameId를 빨리 찾기 위해, 리스트 상에서 FrameId의 위치를 기억
        std::unordered_map<FrameId, std::list<FrameId>::iterator> lru_map_;

        size_t num_pages_;
    };
}