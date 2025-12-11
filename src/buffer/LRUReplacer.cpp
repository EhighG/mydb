#include "mydb/buffer/LRUReplacer.hpp"

namespace mydb {

    LRUReplacer::LRUReplacer(size_t num_pages) : num_pages_(num_pages) {}

    bool LRUReplacer::Victim(FrameId* frame_id) {
        std::scoped_lock lock(mutex_);

        if (lru_list_.empty()) {
            return false;
        }

        // 맨 앞 요소(가장 오랫동안 안 쓰인 요소) 선택
        FrameId evicted_frame = lru_list_.front();

        // 리스트 & 맵에서 제거
        lru_list_.pop_front();

        lru_map_.erase(evicted_frame);

        // 삭제한 frame의 ID, 삭제여부 전달
        // param으로 받은 주소의 메모리에 frameID 기록
        *frame_id = evicted_frame;

        return true;
    }

    void LRUReplacer::Pin(FrameId frame_id) {
        std::scoped_lock lock(mutex_);

        // Unpin상태가 아닌 프레임일 때(Pin상태거나, 없거나)
        if (lru_map_.find(frame_id) == lru_map_.end()) {
            return;
        }

        // Unpin -> pin상태로 전환
        auto iter = lru_map_[frame_id];

        lru_list_.erase(iter);

        lru_map_.erase(frame_id);
    }

    void LRUReplacer::Unpin(FrameId frame_id) {
        std::scoped_lock lock(mutex_);

        // 이미 관리되는 상태(lru_list_와 map에 있는 상태)인 경우
        if (lru_map_.find(frame_id) != lru_map_.end()) {
            return;
        }

        // 목록이 다 찼을 때(드뭄)
        if (lru_list_.size() >= num_pages_) {
            return;
        }

        lru_list_.push_back(frame_id); // 가장 최근에 쓰였으므로, 맨 뒤에 추가

        lru_map_[frame_id] = std::prev(lru_list_.end());
    }

    size_t LRUReplacer::Size() {
        std::scoped_lock lock(mutex_);
        return lru_list_.size();
    }
}