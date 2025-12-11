#pragma once

#include <vector>
#include <cstring>
#include <cstdint>

namespace mydb {
    /**
     * @brief 행(Row) 데이터가 실제로 db에 저장될 때, 그 데이터를 담는 컨테이너
     */
    class Tuple {
    public:
        Tuple() = default;

        // 데이터를 복사해서 생성
        // Tuple클래스는 data_라는 멤버변수를 가지고,
        // 생성자는 vector<char>& 타입의 매개변수를 받아 data_에 할당함
        Tuple(const std::vector<char>& data) : data_(data) {}

        // 포인터로부터 생성(복사)
        Tuple(const char* data, uint32_t size) {
            data_.resize(size);
            std::memcpy(data_.data(), data, size); // dest, src(start), size
        }

        inline uint32_t GetSize() const { return static_cast<uint32_t>(data_.size()); }
        inline const char* GetData() const { return data_.data(); }

    private:
        // 실제 데이터 바이트
        std::vector<char> data_;

        // 추후 다른 정보 추가
    };
}