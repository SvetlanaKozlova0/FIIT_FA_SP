#ifndef SCHONHAGE_STRASSEN_IMPL_H
#define SCHONHAGE_STRASSEN_IMPL_H

#include <array>
#include <cstdint>
#include <vector>

#include "big_int.h"

#include <cstdint>

#include <intrin.h>

class big_int;

namespace schonhage_strassen {

    big_int multiply_schonhage_strassen(const big_int& left, const big_int& right);

    std::vector<uint64_t> number_to_blocks(const big_int& num, size_t block_bits);

    big_int blocks_to_number(const std::vector<uint64_t>& blocks, size_t block_bits, bool sign);

    size_t next_power_of_2(size_t n);

    struct NTT_info {
        uint64_t modulus;
        uint64_t primitive_root;
        uint32_t max_ntt_size;

        static uint64_t mod_add(uint64_t a, uint64_t b, uint64_t mod);
        static uint64_t mod_sub(uint64_t a, uint64_t b, uint64_t mod);
        static uint64_t mod_mul(uint64_t a, uint64_t b, uint64_t mod);
        static uint64_t mod_pow(uint64_t base, uint64_t exp, uint64_t mod);
        static uint64_t mod_inverse(uint64_t a, uint64_t mod);
    };

    constexpr std::array<NTT_info, 3> NTT_MODULES = {{
                                                              {998244353ULL, 3ULL, 1ULL << 23},
                                                              {469762049ULL, 3ULL, 1ULL << 26},
                                                              {167772161ULL, 3ULL, 1ULL << 25}
                                                      }};

    class NTT_functional {
    private:

        static void reverse_number_bits(std::vector<uint64_t>& coeffs);

    public:
        static void forward_ntt(std::vector<uint64_t>& coeffs, const NTT_info& mod_info);

        static void inverse_ntt(std::vector<uint64_t>& coeffs, const NTT_info& mod_info);

        static std::vector<uint64_t> convolute(const std::vector<uint64_t>& a,
                                                 const std::vector<uint64_t>& b,
                                                 const NTT_info& mod_info);


    };
    class reconstructor {
        static constexpr size_t NUM_MODULES = 3;

    public:
        static big_int reconstruct(const std::array<std::vector<uint64_t>, NUM_MODULES>& residues,
                                   size_t block_bits, bool sign);
    };

}

#endif