#pragma once
#include <vector>
#include <string>
#include <array>
#include <string.h>

using namespace std;

using Sha1Hash = std::array<std::uint8_t, 20>;

/*
    inline多个 .cpp 文件 各自生成一份函数代码 → 链接时合并
    字典序比较
*/
inline bool operator<(const Sha1Hash& a, const Sha1Hash& b) {

    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
}

// std::unordered_set 在编译时会自动查找 std::hash<T>
namespace std {
    template<>
    struct hash<Sha1Hash> {
        size_t operator()(const Sha1Hash& h) const noexcept {
            uint64_t val;
            memcpy(&val, h.data(), 8);
            return std::hash<uint64_t>{}(val);
        }
    };
}

struct Chunk{
    uint8_t* data;
    int size;
    Sha1Hash hash;
};

struct SuperChunk{
    vector<Chunk> chunk_list;
    Sha1Hash hash;
    int total_size;

    SuperChunk(){
        total_size = 0;
        hash.fill(0);
    }
};