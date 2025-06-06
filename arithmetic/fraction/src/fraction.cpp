#include "../include/fraction.h"
#include <sstream>

//region help functions

big_int big_int_gcd(big_int first, big_int second) {
    if (first < 0) first = - first;
    if (second < 0) second = - second;
    while (first != second) {
        if (first > second) {
            first -= second;
        } else {
            second -= first;
        }
    }
    return first;
}

fraction fraction::abs() const {
    fraction temp = *this;
    if (_denominator < 0) {
        temp._denominator = - _denominator;
    }
    if (_numerator < 0) {
        temp._numerator = - _numerator;
    }
    return temp;
}

void fraction::optimise()
{
    if (_denominator == 0) {
        throw std::logic_error("division by zero (denominator = 0)");
    }
    if (_numerator == 0) {
        _denominator = 1;
        return;
    }
    if (_denominator < 0) {
        _numerator = - _numerator;
        _denominator = - _denominator;
    }
    big_int temp = big_int_gcd(_numerator, _denominator);
    _numerator /= temp;
    _denominator /= temp;
}

//endregion

fraction::fraction(pp_allocator<big_int::value_type> alloc): _numerator(0, alloc), _denominator(1, alloc)
{
}

//region standart operations

fraction &fraction::operator+=(fraction const &other) &
{
    _numerator = _numerator * other._denominator + _denominator * other._numerator;
    _denominator *= other._denominator;
    optimise();
    return *this;
}

fraction fraction::operator+(fraction const &other) const
{
    fraction temp = *this;
    temp += other;
    return temp;
}

fraction &fraction::operator-=(fraction const &other) &
{
    _numerator = _numerator * other._denominator - _denominator * other._numerator;
    _denominator *= other._denominator;
    optimise();
    return *this;
}

fraction fraction::operator-(fraction const &other) const
{
    fraction temp = *this;
    temp -= other;
    return temp;
}

fraction &fraction::operator*=(fraction const &other) &
{
    _numerator *= other._numerator;
    _denominator *= other._denominator;
    optimise();
    return *this;
}

fraction fraction::operator*(fraction const &other) const
{
    fraction temp = *this;
    temp *= other;
    return temp;
}

fraction &fraction::operator/=(fraction const &other) &
{
    if (other._numerator == 0) {
        throw std::logic_error("division by zero");
    }
    _numerator *= other._denominator;
    _denominator *= other._numerator;
    optimise();
    return *this;
}

fraction fraction::operator/(fraction const &other) const
{
    fraction temp = *this;
    temp /= other;
    return temp;
}

bool fraction::operator==(fraction const &other) const noexcept
{
    return _numerator == other._numerator && _denominator == other._denominator;
}

std::partial_ordering fraction::operator<=>(const fraction& other) const noexcept
{
    big_int first = _numerator * other._denominator;
    big_int second = other._numerator * _denominator;
    return first <=> second;
}

std::ostream &operator<<(std::ostream &stream, fraction const &obj)
{
    stream << obj.to_string();
    return stream;
}

std::istream &operator>>(std::istream &stream, fraction &obj)
{
    char symbol;
    stream >> obj._numerator >> symbol >> obj._denominator;
    if (symbol != '/' || obj._denominator == 0_bi) stream.setstate(std::ios::failbit);
    obj.optimise();
    return stream;
}

std::string fraction::to_string() const
{
    std::stringstream stream;
    if (_denominator < 0) stream << "-";
    stream << _numerator << "/" << ((_denominator > 0) ? _denominator : - _denominator);
    return stream.str();
}

//endregion

//region trigonometry standard functions

fraction fraction::sin(fraction const &epsilon) const {
    fraction result(0, 1);
    fraction x = *this;
    fraction term = x;
    big_int factorial = 1;
    int n = 1;
    while (term.abs() > epsilon) {
        result += term;
        term *= x * x * fraction(-1, 1);
        factorial *= (2 * n) * (2 * n + 1);
        term /= fraction(factorial, 1);
        n++;
    }
    return result;
}

fraction fraction::cos(fraction const &epsilon) const {
    fraction result(0, 1);
    fraction x = *this;
    fraction term(1, 1);
    big_int denominator_factor = 1;
    int n = 1;
    while (term.abs() > epsilon) {
        result += term;
        term *= x * x * fraction(-1, 1);
        denominator_factor *= (2 * n - 1) * (2 * n);
        term /= fraction(denominator_factor, 1);
        n++;
    }
    return result;
}

fraction fraction::tg(fraction const &epsilon) const
{
    fraction denominator = cos(epsilon);
    if (denominator._numerator == 0) {
        throw std::logic_error("tg not exist");
    }
    return sin(epsilon) / denominator;
}

fraction fraction::ctg(fraction const &epsilon) const
{
    fraction denominator = sin(epsilon);
    if (denominator._numerator == 0) {
        throw std::logic_error("division by zero");
    }
    return cos(epsilon) / denominator;
}

fraction fraction::sec(fraction const &epsilon) const
{
    fraction denominator = cos(epsilon);
    if (denominator._numerator == 0) {
        throw std::logic_error("division by zero");
    }
    return fraction(1, 1) / denominator;
}

fraction fraction::cosec(fraction const &epsilon) const
{
    fraction denominator = sin(epsilon);
    if (denominator._numerator == 0) {
        throw std::logic_error("division by zero");
    }
    return fraction(1, 1) / denominator;
}

//endregion

//region arc-functions
fraction fraction::arcsin(const fraction &epsilon) const {
    if (*this > fraction(1, 1) || *this < fraction(-1, 1)) {
        throw std::logic_error("error while calculating arcsin");
    }
    fraction x = *this;
    fraction term = x;
    fraction result(0, 1);
    int n = 1;
    while (term.abs() > epsilon) {
        result += term;
        term *= (x * x) * fraction((2 * n - 1) * (2 * n - 1), (2 * n) * (2 * n + 1));
        n++;
    }
    return result;
}

fraction fraction::arccos(const fraction &epsilon) const {
    if (*this > fraction(1, 1) || *this < fraction(-1, 1)) {
        throw std::logic_error("error while calculation arccos");
    }
    fraction pi = compute_pi(epsilon);
    return (pi / fraction(2, 1)) - this->arcsin(epsilon);
}

fraction fraction::arctg(const fraction &epsilon) const {
    if (*this > fraction(1, 1) || *this < fraction(-1, 1)) {
        throw std::logic_error("error while calculating arctg");
    }
    fraction x = *this;
    fraction term = x;
    fraction result(0, 1);
    int n = 1;
    while (term.abs() > epsilon) {
        result += term;
        term = term * (x * x) * fraction(- 1, 1) * fraction(2 * n - 1, 2 * n + 1);
        n++;
    }
    return result;
}

fraction fraction::arcctg(const fraction &epsilon) const {
    if (*this > fraction(1, 1) || *this < fraction(-1, 1)) {
        throw std::logic_error("error while calculating arcctg");
    }
    if (*this == fraction(0, 1)) {
        return compute_pi(epsilon) / fraction(2, 1);
    }
    fraction pi_half = compute_pi(epsilon) / fraction(2, 1);
    return pi_half - this->arctg(epsilon);
}

fraction fraction::arcsec(const fraction &epsilon) const {
    if (*this < fraction(1, 1) || *this < fraction(-1, 1)) {
        throw std::logic_error("error while calculating arcsec");
    }
    fraction temp = fraction(1, 1) / (*this);
    return temp.arccos(epsilon);
}

fraction fraction::arccosec(const fraction &epsilon) const {
    if (*this < fraction(1, 1) || *this < fraction(-1, 1)) {
        throw std::logic_error("error while calculating arccosec");
    }
    fraction temp = fraction(1, 1) / (*this);
    return temp.arcsin(epsilon);
}
//endregion

fraction fraction::pow(size_t degree) const
{
    if (degree == 0) {
        return {1, 1};
    }
    fraction temp = *this;
    fraction result(1, 1);
    while (degree > 0) {
        if (degree % 2 == 1) {
            result *= temp;
        }
        temp *= temp;
        degree /= 2;
    }
    return result;
}

fraction fraction::root(size_t degree, fraction const &epsilon) const {
    if (degree == 0) {
        throw std::invalid_argument("error while calculating root");
    }
    if (degree == 1) {
        return *this;
    }
    if (*this < fraction(0, 1) && degree % 2 == 0) {
        throw std::logic_error("error while calculating root");
    }
    if (*this == fraction(0, 1)) {
        return {0, 1};
    }
    fraction previous = (*this > fraction(0, 1)) ? fraction(1, 1) : fraction(-1, 1);
    fraction current = previous;
    fraction approach = *this;
    do {
        previous = current;
        fraction x_in_degree = previous.pow(degree - 1);
        fraction term = previous * fraction(degree - 1, 1) + (approach / x_in_degree);
        current = term / fraction(degree, 1);
    } while ((current - previous).abs() > epsilon);
    return current;
}

fraction fraction::compute_pi(const fraction &epsilon) const {
    fraction first = fraction(1, 5).arctg(epsilon) * fraction(4, 1);
    fraction second = fraction(1, 239).arctg(epsilon);
    return fraction(4, 1) * (first - second);
}

//region logarithms

fraction fraction::log2(fraction const &epsilon) const
{
    if (_denominator < 0 || *this >= fraction(2, 1)) {
        throw std::logic_error("error while calculation log2");
    }
    fraction ln2 = fraction(2, 1).ln(epsilon);
    return ln(epsilon) / ln2;
}

fraction fraction::ln(fraction const &epsilon) const {
    if (_denominator < 0) {
        throw std::logic_error("error while calculation ln");
    }
    if (*this > fraction(2, 1)) {
        fraction ln2 = fraction(2, 1).ln(epsilon);
        return ln2 + ((*this) / fraction(2, 1)).ln(epsilon);
    }
    if (*this < fraction(1, 2)) {
        return fraction(0, 1) - (fraction(1, 1) / (*this)).ln(epsilon);
    }
    fraction y = (*this) - fraction(1, 1);
    fraction result(0, 1);
    fraction term = y;
    int n = 1;
    while (term.abs() > epsilon) {
        result += term;
        term *= y * fraction(n - 1, n) * fraction(-1, 1);
        n++;
    }
    return result;
}

fraction fraction::lg(fraction const &epsilon) const
{
    if (_denominator < 0) {
        throw std::logic_error("error while calculating lg");
    }
    fraction ln10 = fraction(10, 1).ln(epsilon);
    return ln(epsilon) / ln10;
}

//endregion
