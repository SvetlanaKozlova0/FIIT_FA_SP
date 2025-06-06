#include "../include/NTT.h"
#include "../include/big_int.h"
#include <algorithm>
#include <cmath>
#include <cassert>
#include <iostream>

namespace schonhage_strassen {

    big_int multiply_schonhage_strassen(const big_int& left, const big_int& right) {

        constexpr size_t SIZE_BLOCK = 12;
        uint64_t block_mask = (1ULL << SIZE_BLOCK) - 1;

        bool mult_sign = (left._sign == right._sign);

        auto left_blocks = number_to_blocks(left, SIZE_BLOCK);
        auto right_blocks = number_to_blocks(right, SIZE_BLOCK);

        size_t result_size = left_blocks.size() + right_blocks.size() - 1;
        size_t size_ntt = next_power_of_2(result_size);

        std::vector<uint64_t> result_blocks;

        for (const auto& info : NTT_MODULES) {

            if (size_ntt <= info.max_ntt_size) {

                result_blocks = NTT_functional::convolute(left_blocks, right_blocks, info);

                if (!result_blocks.empty()) {
                    break;
                }
            }
        }

        uint64_t carry = 0;

        for (unsigned long long & elem : result_blocks) {
            uint64_t curr = elem + carry;
            elem = curr & block_mask;
            carry = curr >> SIZE_BLOCK;
        }

        while (carry > 0) {
            result_blocks.push_back(carry & block_mask);
            carry >>= SIZE_BLOCK;
        }

        while (result_blocks.size() > 1 && result_blocks.back() == 0) {
            result_blocks.pop_back();
        }

        return blocks_to_number(result_blocks, SIZE_BLOCK, mult_sign);
    }

    void NTT_functional::forward_ntt(std::vector<uint64_t>& coefficients, const NTT_info& mod_info) {
        size_t length = coefficients.size();

        reverse_number_bits(coefficients);

        for (size_t len = 2; len <= length; len <<= 1) {

            uint64_t w_len = NTT_info::mod_pow(mod_info.primitive_root,
                                                 (mod_info.modulus - 1) / len,
                                                 mod_info.modulus);

            for (size_t i = 0; i < length; i += len) {
                uint64_t w = 1;
                for (size_t j = 0; j < len / 2; ++j) {

                    uint64_t u = coefficients[i + j];
                    uint64_t v = NTT_info::mod_mul(coefficients[i + j + len / 2], w, mod_info.modulus);

                    coefficients[i + j] = NTT_info::mod_add(u, v, mod_info.modulus);
                    coefficients[i + j + len / 2] = NTT_info::mod_sub(u, v, mod_info.modulus);

                    w = NTT_info::mod_mul(w, w_len, mod_info.modulus);
                }
            }
        }
    }

    void NTT_functional::inverse_ntt(std::vector<uint64_t>& coefficients, const NTT_info& mod_info) {

        size_t n = coefficients.size();

        uint64_t inverted_root = NTT_info::mod_inverse(mod_info.primitive_root, mod_info.modulus);

        reverse_number_bits(coefficients);

        for (size_t len = 2; len <= n; len <<= 1) {
            uint64_t w_len = NTT_info::mod_pow(inverted_root,
                                                 (mod_info.modulus - 1) / len,
                                                 mod_info.modulus);
            for (size_t i = 0; i < n; i += len) {
                uint64_t w = 1;
                for (size_t j = 0; j < len / 2; ++j) {

                    uint64_t u = coefficients[i + j];
                    uint64_t v = NTT_info::mod_mul(coefficients[i + j + len / 2], w, mod_info.modulus);

                    coefficients[i + j] = NTT_info::mod_add(u, v, mod_info.modulus);
                    coefficients[i + j + len / 2] = NTT_info::mod_sub(u, v, mod_info.modulus);

                    w = NTT_info::mod_mul(w, w_len, mod_info.modulus);
                }
            }
        }

        uint64_t inverted_n = NTT_info::mod_inverse(n, mod_info.modulus);

        for (auto& elem : coefficients) {
            elem = NTT_info::mod_mul(elem, inverted_n, mod_info.modulus);
        }
    }

    std::vector<uint64_t> NTT_functional::convolute(const std::vector<uint64_t>& first_number,
                                                    const std::vector<uint64_t>& second_number,
                                                    const NTT_info& info) {

        size_t result_size = first_number.size() + second_number.size() - 1;
        size_t n = next_power_of_2(result_size);

        std::vector<uint64_t> copy_first(first_number.begin(), first_number.end());
        std::vector<uint64_t> copy_second(second_number.begin(), second_number.end());
        copy_first.resize(n, 0);
        copy_second.resize(n, 0);

        forward_ntt(copy_first, info);
        forward_ntt(copy_second, info);

        for (size_t i = 0; i < n; ++i) {
            copy_first[i] = NTT_info::mod_mul(copy_first[i], copy_second[i], info.modulus);
        }

        inverse_ntt(copy_first, info);
        copy_first.resize(result_size);

        return copy_first;
    }

    uint64_t NTT_info::mod_add(uint64_t first_number, uint64_t second_number, uint64_t mod) {

        first_number %= mod;
        second_number %= mod;

        if (first_number <= mod - second_number) {
            return first_number + second_number;
        }

        return first_number + second_number - mod;
    }

    uint64_t NTT_info::mod_sub(uint64_t first_number, uint64_t second_number, uint64_t mod) {

        first_number %= mod;
        second_number %= mod;

        if (first_number >= second_number) {
            return first_number - second_number;
        }

        return first_number + mod - second_number;
    }

    uint64_t NTT_info::mod_mul(uint64_t first_number, uint64_t second_number, uint64_t mod) {

        uint64_t high_number, low_number;

        low_number = _umul128(first_number, second_number, &high_number);
        uint64_t remainder;
        _udiv128(high_number, low_number, mod, &remainder);

        return remainder;
    }

    uint64_t NTT_info::mod_pow(uint64_t base, uint64_t exp, uint64_t mod) {

        uint64_t result = 1;

        base %= mod;

        while (exp > 0) {

            if (exp & 1) {
                result = mod_mul(result, base, mod);
            }

            exp >>= 1;
            base = mod_mul(base, base, mod);

        }
        return result;
    }

    uint64_t NTT_info::mod_inverse(uint64_t a, uint64_t mod) {
        return mod_pow(a, mod - 2, mod);
    }

    void NTT_functional::reverse_number_bits(std::vector<uint64_t>& coefficients) {

        size_t length = coefficients.size();

        for (size_t i = 1, j = 0; i < length; ++i) {

            size_t bit = length >> 1;

            for (; j & bit; bit >>= 1) {
                j ^= bit;
            }

            j ^= bit;

            if (i < j) {
                std::swap(coefficients[i], coefficients[j]);
            }
        }
    }

    size_t next_power_of_2(size_t n) {
        if (n <= 1) {
            return 1;
        }
        --n;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        n |= n >> 32;
        return ++n;
    }

    std::vector<uint64_t> number_to_blocks(const big_int& number, size_t block_bits) {

        if (number._digits.size() == 1 && number._digits[0] == 0) {
            return {0};
        }

        big_int temp_number = number;
        temp_number._sign = true;
        std::vector<uint64_t> blocks;

        big_int base(1ULL << block_bits);

        while (!(temp_number._digits.size() == 1 && temp_number._digits[0] == 0)) {

            big_int remainder = temp_number % base;
            uint64_t new_elem = 0;

            if (!(remainder._digits.size() == 1 && remainder._digits[0] == 0) && !remainder._digits.empty()) {

                new_elem = remainder._digits[0];

                if (remainder._digits.size() > 1) {
                    new_elem |= (uint64_t(remainder._digits[1]) << 32);
                }
            }

            blocks.push_back(new_elem);
            temp_number /= base;

        }
        return blocks;
    }

    big_int blocks_to_number(const std::vector<uint64_t>& blocks, size_t block_bits, bool sign) {

        if (blocks.empty() || (blocks.size() == 1 && blocks[0] == 0)) {
            return {0};
        }

        big_int result(0);
        big_int base(1ULL << block_bits);

        big_int n(1);

        for (unsigned long long block : blocks) {
            if (block != 0) {
                big_int temp(block);
                result += temp * n;
            }
            n *= base;
        }

        if (!sign && result._sign) {
            result._sign = false;
        } else if (!sign) {
            result._sign = true;
        }

        return result;
    }

    big_int reconstructor::reconstruct(const std::array<std::vector<uint64_t>, NUM_MODULES>& blocks,
                                          size_t size_block, bool sign) {
        if (blocks[0].empty()) {
            return {0};
        }

        return blocks_to_number(blocks[0], size_block, sign);
    }

}
