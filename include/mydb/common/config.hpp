#pragma once

#include <cstdint>
#include <atomic>

namespace mydb {
    /**
     * 타입 alias - 나중에 타입 바꿀 때 여기만 고치면 됨
     */

    /** 페이지 ID (디스크상의 위치) */
    using PageId = int32_t;

    /** 프레임 ID (버퍼 풀 메모리상의 인덱스) */
    using frame_id_t = int32_t;

    /** 로그 시퀀스 번호 (Recovery나 Transaction에서 사용 예정) */
    using lsn_t = int32_t;

    /** 슬롯 오프셋/길이 (Page 내부 위치 */
    using offset_t = uint16_t;

    /**
     * 시스템 상수 설정
     */

    /** 페이지 크기: 16KB (InnoDB 기본값, 일반적인 OS 페이지 크기 * 4) */
    static constexpr uint32_t PAGE_SIZE = 16384;

    /** 유효하지 않은 페이지 ID */
    static constexpr PageId INVALID_PAGE_ID = -1;
}
