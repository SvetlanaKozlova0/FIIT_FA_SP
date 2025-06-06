#include "../include/big_int.h"
#include "strassen.cpp"
#include <exception>
#include <string>
#include <sstream>
#include <cmath>


big_int big_int::operator+(const big_int &other) const {
    big_int temp = *this;
    temp += other;
    return std::move(temp);
}

big_int big_int::operator-(const big_int &other) const {
    big_int temp = *this;
    temp -= other;
    return std::move(temp);
}

big_int big_int::operator*(const big_int &other) const {
    big_int temp = *this;
    temp *= other;
    return std::move(temp);
}

big_int big_int::operator/(const big_int &other) const {
    big_int temp = *this;
    temp /= other;
    return std::move(temp);
}


big_int big_int::operator%(const big_int &other) const {
    big_int temp = *this;
    temp %= other;
    return std::move(temp);
}

big_int big_int::operator&(const big_int &other) const {
    big_int temp = *this;
    temp &= other;
    return std::move(temp);
}

big_int big_int::operator|(const big_int &other) const {
    big_int temp = *this;
    temp |= other;
    return std::move(temp);
}

big_int big_int::operator^(const big_int &other) const {
    big_int temp = *this;
    temp ^= other;
    return std::move(temp);
}

big_int big_int::operator<<(size_t shift) const {
    big_int temp = *this;
    temp <<= shift;
    return std::move(temp);
}

big_int big_int::operator>>(size_t shift) const {
    big_int temp = *this;
    temp >>= shift;
    return std::move(temp);
}

big_int big_int::operator~() const {
    big_int temp(*this);
    for (auto& num: temp._digits) {
        num = ~num;
    }
    temp.optimise();
    return std::move(temp);
}

std::ostream& operator<<(std::ostream& stream, const big_int& value) {
    stream << value.to_string();
    return stream;
}

std::istream& operator>>(std::istream& stream, big_int& value) {
    std::string number;
    stream >> number;
    value = big_int(number, 10, value._digits.get_allocator());
    return stream;
}

bool big_int::operator==(const big_int &other) const noexcept {
    return (*this <=> other) == std::strong_ordering::equal;
}

big_int::operator bool() const noexcept {
    return !is_zero();
}

big_int &big_int::operator+=(const big_int &other) &{
    return plus_assign(other, 0);
}

big_int &big_int::operator%=(const big_int &other) &{
    return modulo_assign(other, big_int::division_rule::trivial);
}

big_int &big_int::operator&=(const big_int &other) &{
    _digits.resize(std::max(_digits.size(), other._digits.size()), 0);
    for (size_t i = 0; i < _digits.size(); ++i) {
        _digits[i] &= (i < other._digits.size()) ? other._digits[i] : 0;
    }
    optimise();
    return *this;
}

big_int &big_int::operator-=(const big_int &other) &{
    return minus_assign(other, 0);
}

big_int &big_int::operator|=(const big_int &other) &{
    _digits.resize(std::max(_digits.size(), other._digits.size()), 0);
    for (size_t i = 0; i < _digits.size(); ++i) {
        _digits[i] |= (i < other._digits.size()) ? other._digits[i] : 0;
    }
    optimise();
    return *this;
}

big_int &big_int::operator^=(const big_int &other) &{
    _digits.resize(std::max(_digits.size(), other._digits.size()), 0);
    for (size_t i = 0; i < _digits.size(); ++i) {
        _digits[i] ^= (i < other._digits.size()) ? other._digits[i] : 0;
    }
    optimise();
    return *this;
}

big_int &big_int::operator*=(const big_int &other) &{
    return multiply_assign(other, decide_mult(other._digits.size()));
}

big_int &big_int::operator/=(const big_int &other) &{
    return divide_assign(other, decide_div(other._digits.size()));
}

big_int &big_int::operator<<=(size_t shift) &{
    if (shift == 0 || is_zero()) {
        return *this;
    }
    size_t big_shift = shift / BITS_PER_DIGIT;
    if (big_shift) {
        _digits.reserve(_digits.size() + big_shift);
        _digits.insert(_digits.begin(), big_shift, 0);
    }
    size_t little_shift = shift % BITS_PER_DIGIT;
    if (little_shift) {
        unsigned long long shifted, total, carry = 0;
        for (auto& digit: _digits) {
            shifted = static_cast<unsigned long long>(digit) << little_shift;
            total = shifted | carry;
            digit = static_cast<unsigned int>(total & mask);
            carry = total >> BITS_PER_DIGIT;
        }
        if (carry) _digits.push_back(static_cast<unsigned int>(carry));
    }
    optimise();
    return *this;
}

big_int &big_int::operator>>=(size_t shift) &{
    if (shift == 0 || is_zero()) {
        return *this;
    }
    size_t big_shift = shift / BITS_PER_DIGIT;
    if (big_shift >= _digits.size()) {
        clear_big_int();
        return *this;
    }
    if (big_shift) {
        _digits.erase(_digits.begin(), _digits.begin() + static_cast<unsigned int>(big_shift));
    }
    size_t little_shift = shift % BITS_PER_DIGIT;
    if (little_shift) {
        auto little_mask = (1ULL << little_shift) - 1;
        unsigned long long shifted, carry = 0;
        for (auto it = _digits.rbegin(); it != _digits.rend(); ++it) {
            shifted = (carry << BITS_PER_DIGIT) | *it;
            carry = shifted & little_mask;
            *it = static_cast<unsigned int>(shifted >> little_shift);
        }
    }
    optimise();
    return *this;
}

big_int &big_int::operator++() &{
    *this += big_int(1, _digits.get_allocator());
    return *this;
}

big_int big_int::operator++(int) {
    auto temp = *this;
    ++(*this);
    return temp;
}

big_int &big_int::operator--() &{
    *this -= big_int(1, _digits.get_allocator());
    return *this;
}

big_int big_int::operator--(int) {
    auto temp = *this;
    --(*this);
    return temp;
}

big_int operator""_bi(unsigned long long n) {
    return {n};
}

big_int::big_int(const std::vector<unsigned int, pp_allocator<unsigned int>> &digits, bool sign) : _digits(digits), _sign(sign) {
    optimise();
}

big_int::big_int(std::vector<unsigned int, pp_allocator<unsigned int>> &&digits, bool sign) noexcept : _digits(std::move(digits)), _sign(sign) {
    optimise();
}

big_int::big_int(const std::string &num, unsigned int radix, pp_allocator<unsigned int> allocator) : _sign(true), _digits(allocator) {
    if (num.empty()) {
        _digits.push_back(0);
        return;
    }
    std::string copy_number = num;
    bool flag_is_negative = false;
    if (copy_number[0] == '-') {
        flag_is_negative = true;
        copy_number = copy_number.substr(1);
    } else if (copy_number[0] == '+') {
        copy_number = copy_number.substr(1);
    }
    while (copy_number.size() > 1 && copy_number[0] == '0') {
        copy_number = copy_number.substr(1);
    }
    if (copy_number.empty()) {
        _digits.push_back(0);
        return;
    }
    _digits.push_back(0);
    for (char c: copy_number) {
        if (!std::isdigit(c)) {
            throw std::invalid_argument("invalid symbol");
        }
        unsigned int digit = isdigit(c) ? c - '0' : isalpha(c) ? tolower(c) - 'a' + 10: radix + 1;
        if (digit == radix + 1) {
            throw std::invalid_argument("invalid symbol");
        }
        *this *= radix;
        *this += big_int(static_cast<long long>(digit), allocator);
    }
    _sign = !flag_is_negative;
    optimise();
}

big_int::big_int(pp_allocator<unsigned int> allocator) : _digits(allocator), _sign(true) {
    _digits.push_back(0);
}

bool big_int::is_zero() const {
    return _digits.size() == 1 && _digits[0] == 0;
}

void big_int::optimise() {
    if (_digits.empty()) {
        _digits.push_back(0);
    }
    while (_digits.size() > 1 && _digits.back() == 0) {
        _digits.pop_back();
    }
    if (is_zero()) {
        _sign = true;
    }
}

void big_int::clear_big_int(){
    _digits.clear();
    _digits.push_back(0);
    _sign = true;
}

big_int::division_rule big_int::decide_div(size_t rhs) const noexcept {
    return big_int::division_rule::trivial;
}

big_int::multiplication_rule big_int::decide_mult(size_t rhs) const noexcept {
    if (rhs > 32) {
        return big_int::multiplication_rule::Karatsuba;
    }
    return big_int::multiplication_rule::trivial;
}


std::strong_ordering big_int::operator<=>(const big_int &other) const noexcept {
    if (_sign != other._sign) {
        if (_sign) {
            return std::strong_ordering::greater;
        } else {
            return std::strong_ordering::less;
        }
    }
    if (_digits.size() != other._digits.size()) {
        if (_sign) {
            return _digits.size() <=> other._digits.size();
        } else {
            return other._digits.size() <=> _digits.size();
        }
    }
    for (int i = static_cast<int>(_digits.size()) - 1; i >= 0; --i) {
        if (_digits[i] != other._digits[i]) {
            if (_sign) {
                return _digits[i] <=> other._digits[i];
            } else {
                return other._digits[i] <=> _digits[i];
            }
        }
    }
    return std::strong_ordering::equal;
}


std::string big_int::to_string() const {
    if (is_zero()) {
        return "0";
    }
    std::string result;
    big_int temp = *this;
    temp._sign = true;
    while (temp) {
        auto value = temp % 10;
        int digit = static_cast<int>(value._digits[0]);
        result += static_cast<char>('0' + digit);
        temp /= 10;
    }
    if (!_sign) {
        result += '-';
    }
    std::reverse(result.begin(), result.end());
    return result;
}


big_int &big_int::plus_assign(const big_int &other, size_t shift) &{
    if (other.is_zero()) {
        return *this;
    }
    if (_sign != other._sign) {
        big_int temp(other);
        temp._sign = _sign;
        minus_assign(temp, shift);
        optimise();
        return *this;
    }
    unsigned long long carry = 0, sum;
    _digits.resize(std::max(_digits.size(), other._digits.size() + shift), 0);
    for (size_t i = 0; i < _digits.size(); i++) {
        sum = carry + _digits[i];
        sum += (i >= shift && (i - shift) < other._digits.size()) ? other._digits[i - shift] : 0;
        carry = sum >> BITS_PER_DIGIT;
        _digits[i] = static_cast<unsigned int>(sum & mask);
    }
    if (carry) _digits.push_back(static_cast<unsigned int>(carry));
    optimise();
    return *this;
}


big_int& big_int::minus_assign(const big_int& other, size_t shift) & {
    if (other.is_zero()) return *this;
    if (_sign != other._sign) {
        big_int temp = other;
        temp._sign = _sign;
        return plus_assign(temp, shift);
    }
    bool this_larger = compare_abs_numbers(other, shift);
    _sign = this_larger ? _sign : !_sign;
    const big_int& big_number = this_larger ? *this : other;
    const big_int& little_number = this_larger ? other : *this;
    _digits.resize(big_number._digits.size(), 0);
    unsigned long long borrow = 0, big_digit, little_digit;
    long long difference;
    for (size_t i = 0; i < _digits.size(); ++i) {
        big_digit = big_number._digits[i];
        little_digit = (i >= shift && (i - shift) < little_number._digits.size()) ? little_number._digits[i - shift] : 0;
        difference = static_cast<long long>(big_digit - little_digit - borrow);
        borrow = 0;
        if (difference < 0) {
            difference += BASE;
            borrow = 1;
        }
        _digits[i] = static_cast<unsigned>(difference);
    }
    optimise();
    return *this;
}

bool big_int::compare_abs_numbers(const big_int& other, size_t shift) {
    size_t this_size = _digits.size();
    size_t other_size = other._digits.size() + shift;
    bool this_larger = (this_size > other_size);
    unsigned int this_digit, other_digit;
    if (this_size == other_size) {
        for (int i = static_cast<int>(this_size) - 1; i >= 0; --i) {
            this_digit = _digits[i];
            other_digit = (i >= shift && (i - shift) < other._digits.size()) ? other._digits[i - shift] : 0;
            if (this_digit != other_digit) {
                this_larger = (this_digit > other_digit);
                break;
            }
        }
    }
    return this_larger;
}


big_int &big_int::multiply_assign(const big_int &other, big_int::multiplication_rule rule) &{
    if (is_zero()) {
        return *this;
    }
    if (other.is_zero()) {
        clear_big_int();
        return *this;
    }
    if (rule == big_int::multiplication_rule::trivial) {
        return trivial_multiply(other);
    } else if (rule == big_int::multiplication_rule::Karatsuba) {
        bool sign = determine_the_sign(other);
        *this = karatsuba_multiply(*this, other);
        _sign = sign;
        return *this;
    } else if (rule == big_int::multiplication_rule::SchonhageStrassen) {
        big_int result = multiply_strassen(*this, other);
        _digits = std::move(result._digits);
        _sign = (_sign == other._sign);
        optimise();
        return *this;
    }
    return *this;
}


big_int &big_int::trivial_multiply(const big_int &other) &{
    big_int result(_digits.get_allocator());
    result._digits.resize(_digits.size() + other._digits.size(), 0);
    unsigned long long carry = 0, temp_product;
    for (size_t i = 0; i < _digits.size(); i++) {
        carry = 0;
        for (size_t j = 0; j < other._digits.size() || carry; j++) {
            temp_product = carry + result._digits[i + j];
            if (j < other._digits.size()) {
                temp_product += static_cast<unsigned long long>(_digits[i]) * other._digits[j];
            }
            result._digits[i + j] = static_cast<unsigned int>(temp_product & mask);
            carry = temp_product >> BITS_PER_DIGIT;
        }
    }
    _sign = determine_the_sign(other);
    std::swap(_digits, result._digits);
    optimise();
    return *this;
}

big_int &big_int::divide_assign(const big_int &other, big_int::division_rule rule) &{
    if (other.is_zero()) {
        throw std::logic_error("division by zero");
    }
    big_int temp1 = *this;
    temp1._sign = true;
    big_int temp2 = other;
    temp2._sign = true;
    if (temp1 != temp2 && (is_zero() || !compare_abs_numbers(other, 0))) {
        clear_big_int();
        return *this;
    }
    std::vector<unsigned int, pp_allocator<unsigned int>> remainder;
    trivial_divide_help(other, _digits, remainder);
    _sign = determine_the_sign(other);
    optimise();
    return *this;
}

bool big_int::determine_the_sign(const big_int &other) const {
    if (_sign == other._sign) {
        return true;
    } else {
        return false;
    }
}

big_int &big_int::modulo_assign(const big_int &other, big_int::division_rule rule) &{
    if (other.is_zero()) {
        throw std::logic_error("division by zero");
    }
//    if (is_zero() || !compare_abs_numbers(other, 0)) {
//        _sign = true;
//        return *this;
//    }
    std::vector<unsigned int, pp_allocator<unsigned int>> quotient;
    trivial_divide_help(other, quotient, _digits);
    optimise();
    return *this;
}

void big_int::trivial_divide_help(const big_int &abs_other, std::vector<unsigned int, pp_allocator<unsigned int>>  &quotient_result, std::vector<unsigned int, pp_allocator<unsigned int>>  &remainder_result) &{
    big_int remainder(_digits.get_allocator());
    remainder._digits.reserve(_digits.size());
    std::vector<unsigned int, pp_allocator<unsigned int>> quotient(_digits.size(), 0, _digits.get_allocator());
    for (int i = static_cast<int>(_digits.size()) - 1; i >= 0; i--) {
        remainder._digits.insert(remainder._digits.begin(), _digits[i]);
        remainder.optimise();
        unsigned long long current_quot = find_divider(abs_other, remainder);
        if (current_quot) {
            big_int temp = abs_other * big_int(static_cast<long long>(current_quot), _digits.get_allocator());
            if (temp < 0) temp._sign = true;
            remainder -= temp;
        }
        quotient[i] = static_cast<unsigned int>(current_quot);
    }
    remainder_result = std::move(remainder._digits);
    quotient_result = std::move(quotient);
}


unsigned long long big_int::find_divider(const big_int& other, const big_int& remainder) {
    unsigned long long left = 0, current_quot = 0, right = BASE, middle;
    while (left <= right) {
        middle = left + (right - left) / 2;
        big_int temp = other * big_int(static_cast<long long>(middle), _digits.get_allocator());
        if (temp < 0) temp._sign = true;
        if (remainder >= temp) {
            current_quot = middle;
            left = middle + 1;
        } else {
            right = middle - 1;
        }
    }
    return current_quot;
}


big_int big_int::karatsuba_multiply(const big_int &x, const big_int &y) {
    if (x._digits.size() <= 10 || y._digits.size() <= 10) {
        big_int result = x;
        result.trivial_multiply(y);
        return std::move(result);
    }
    
    size_t half_length = std::max(x._digits.size(), y._digits.size()) / 2;
    big_int a = x >> (half_length * BITS_PER_DIGIT);
    if (a < 0) a._sign = true;
    big_int b = x - (a << (half_length * BITS_PER_DIGIT));
    big_int c = y >> (half_length * BITS_PER_DIGIT);
    if (c < 0) c._sign = true;
    big_int d = y - (c << (half_length * BITS_PER_DIGIT));

    big_int z0 = karatsuba_multiply(a, c);
    big_int z1 = karatsuba_multiply(b, d);
    big_int z2 = karatsuba_multiply(a + b, c + d) - z0 - z1;
    big_int result = big_int(z0 << (2 * half_length * BITS_PER_DIGIT)) + big_int(z2 << (half_length * BITS_PER_DIGIT)) + z1;
    result.optimise();
    return result;
}

big_int big_int::operator-() const {
    big_int temp = *this;
    temp._sign = !temp._sign;
    return temp;
}

big_int multiply_strassen(const big_int &a, const big_int &b){
    return schonhage_strassen::multiply_schonhage_strassen(a, b);
}