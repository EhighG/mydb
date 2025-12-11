#include "mydb/buffer/BufferPoolManager.hpp"
#include <spdlog/spdlog.h>

namespace mydb {

    BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager* disk_manager)
        : pool_size_(pool_size), disk_manager_(disk_manager) {

        /* 컴파일타임에 크기가 결정되지 않는 배열을 만들려면, 동적할당해야 함
         * (스택에 할당할 메모리 크기를 결정할 수 없으므로, 힙에 할당하고 스택에는 포인터만 둬야 함.
         * ((참고) 스택 프레임 오프셋 결정)
         */
        pages_ = new Page[pool_size_];

        replacer_ = new LRUReplacer(pool_size);

        // 처음 생성하면 모든 프레임이 비어있음.
        for (size_t i = 0; i < pool_size_; i++) {
            free_list_.push_back(static_cast<FrameId>(i));
        }
    }

    BufferPoolManager::~BufferPoolManager() {
        delete[] pages_;
        delete replacer_;
    }

    // 페이지 요청
    Page* BufferPoolManager::FetchPage(PageId page_id) {
        std::scoped_lock lock(mutex_);

        // 이미 메모리에 있는 경우(Cache hit)
        if (page_table_.find(page_id) != page_table_.end()) {
            FrameId frame_id = page_table_[page_id];

            // pin count 증가 (unpin -> pin으로 바뀌는 경우 포함)
            pages_[frame_id].pin_count_++;

            // 사용중이므로 LRU List (삭제가능대상 리스트)에서 제거
            replacer_->Pin(frame_id);

            return &pages_[frame_id];
        }

        // 메모리에 없는 경우 -> 빈자리 찾고, 디스크에서 읽어온다
        FrameId frame_id;
        if (!FindFreeFrameFromVictim(&frame_id)) {
            return nullptr; // 버퍼 풀에 빈자리가 없음(pin상태인 프레임으로 꽉 참)
        }

        // 찾았다면, frame_id = 빈 프레임 id
        Page& page = pages_[frame_id];

        // 매핑 테이블에 기존 정보가 남아있으면, 삭제
        if (page.page_id_ != INVALID_PAGE_ID) {
            page_table_.erase(page.page_id_);
        }

        // 디스크에서 읽어오기
        page.page_id_ = page_id;
        page.pin_count_ = 1;
        page.is_dirty_ = false;

        disk_manager_->ReadPage(page_id, page);

        // 메타데이터 업데이트
        replacer_->Pin(frame_id);
        page_table_[page_id] = frame_id;

        return &page;
    }

    bool BufferPoolManager::UnpinPage(PageId page_id, bool is_dirty) {
        std::scoped_lock lock(mutex_);

        // 메모리에 없으면 실패
        if (page_table_.find(page_id) == page_table_.end()) {
            return false;
        }

        FrameId frame_id = page_table_[page_id];
        Page& page = pages_[frame_id];

        // 사용중인 곳이 없는데 unpin 시도 -> 로직 오류
        if (page.pin_count_ <= 0) {
            return false;
        }

        // 수정여부 반영
        page.is_dirty_ |= is_dirty;

        // 핀 카운트 감소
        page.pin_count_--;

        // 사용중인 곳이 없으면, 삭제 가능 리스트에 등록
        if (page.pin_count_ == 0) {
            replacer_->Unpin(frame_id);
        }

        return true;
    }

    bool BufferPoolManager::FlushPage(PageId page_id) {
        std::scoped_lock lock(mutex_);

        if (page_table_.find(page_id) == page_table_.end()) {
            return false;
        }

        FrameId frame_id = page_table_[page_id];
        Page& page = pages_[frame_id];

        // 디스크 쓰기
        disk_manager_->WritePage(page_id, page);
        page.is_dirty_ = false;

        return true;
    }

    Page* BufferPoolManager::NewPage(PageId* page_id) {
        std::scoped_lock lock(mutex_);

        FrameId frame_id;
        if (!FindFreeFrameFromVictim(&frame_id)) {
            return nullptr;
        }

        // 새 페이지 할당 = 디스크 관련 작업이므로, 디스크 매니저에게
        PageId new_page_id = disk_manager_->AllocatePage();
        *page_id = new_page_id;

        // 새 페이지를 만들고, 버퍼 풀에 저장
        // 메모리 프레임 세팅
        Page& page = pages_[frame_id];

        if (page.page_id_ != INVALID_PAGE_ID) {
            page_table_.erase(page.page_id_);
        }

        // 새 페이지 세팅
        page.reset(); // 데이터 0으로 초기화
        page.page_id_ = new_page_id;
        page.pin_count_ = 1;
        page.is_dirty_ = false;

        // 테이블 등록
        page_table_[new_page_id] = frame_id;
        replacer_->Pin(frame_id);

        return &page;
    }

    // 헬퍼 함수: 빈 프레임 찾기 (FreeList - LRU list 순으로 탐색)
    bool BufferPoolManager::FindFreeFrameFromVictim(FrameId* frame_id) {
        // Free List에 빈 공간 있는지 체크
        if (!free_list_.empty()) {
            *frame_id = free_list_.front();
            free_list_.pop_front();
            return true;
        }

        // 없으면 LRU Replacer에게 victim 결정 요청
        if (replacer_->Victim(frame_id)) {
            Page& victim_page = pages_[*frame_id];
            // victim이 디스크에 저장하지 않은 수정사항을 갖고 있으면, 기록
            if (victim_page.is_dirty_) {
                disk_manager_->WritePage(victim_page.page_id_, victim_page);
                victim_page.is_dirty_ = false;
            }
            // 테이블에서 제거는 호출한 쪽에서 처리 (여기선 프레임 확보만)
            return true;
        }

        // 쫓아낼 페이지도 없으면(pin상태인 것들로 꽉 차있으면) 실패
        return false;
    }
}