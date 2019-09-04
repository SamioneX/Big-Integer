#ifndef BIGINT_H
#define BIGINT_H

#include <iostream>

namespace my {
    struct myExceptions : public std::exception {
        const char* msg;
        myExceptions(const char* s) {
            msg = s;
        }
        const char * what () const throw () {
            return msg;
        }
    };
    struct divisionByZero : public myExceptions {
        divisionByZero(const char* s): myExceptions(s) {}
    };
    struct indexOutOfRange : public myExceptions {
        indexOutOfRange(const char* s): myExceptions(s) {}
    };
    
    long long pow2[62] = {0};  //look up table for powers of 2 from 2^1 to 2^62
    long long lengths[19] = {0}; //look up table to find number lengths

    class BigInt {
        private:
        typedef unsigned short int s_type;
        s_type* arr;
        int allocated;
        int inUse;
        bool isNeg;

        void growArray(int n) {
            allocated += (n < 1)? 2: n;
            s_type* temp = new s_type[allocated];
            for (int i = 0; i < inUse; ++i)
                temp[i] = arr[i];
            delete [] arr;
            arr = temp;
        }
        //Find lower bound for a number
        template<typename T>
        int lower_bound(T* arr, T n, int size) const {
            int i = 0, j = size, mid;
            while (i < j) {
                mid = (i + j) / 2;

                if (arr[mid] == n)
                    return mid;

                if (n < arr[mid]) {
                    if (mid > 0 && n >= arr[mid-1])
                        return mid-1;
                    j = mid;
                }
                else {
                    if (mid < size-1 && n < arr[mid+1])
                        return mid;
                    i = mid+1;
                }
            }
            return -1; //this never executes because n is bound to be within the range.
        }
        int numLength(long long n) const {
            if (!lengths[0]) { //this executes only once
                lengths[0] = 10;
                for (int i = 1; i < 18; ++i)
                    lengths[i] = 10 * lengths[i-1];
                lengths[18] = 9223372036854775807;  //LLONG_MAX
            }
            if (n < 10) return 1;
            if (n == lengths[18]) return 19;
            return lower_bound(lengths, n, 19) + 2;
        }
        //used in all move operations
        void move(BigInt&& b) {
            delete [] arr;
            allocated = b.allocated; b.allocated = 1;
            inUse = b.inUse; b.inUse = 1;
            isNeg = b.isNeg; b.isNeg = false;
            arr = b.arr; b.arr = new s_type[1]; b.arr[0] = 0;
        }
        //Resets value to a unit number
        void reset(int val = 0, bool isNeg = false) {
            delete [] arr;
            allocated = 1; inUse = 1; this->isNeg = isNeg;
            arr = new s_type[allocated];
            arr[0] = val;
        }
        //compares two arrays of integers in reverse manner starting from the last element
        int compare_help(s_type* a, int a_size, s_type* b, int b_size) const {
            if (a_size > b_size)
                return 1;
            if (b_size > a_size)
                return -1;
            for (int i = a_size-1; i >= 0; --i) {
                if (a[i] != b[i])
                    return a[i] - b[i];
            }
            return 0;
        }
        /*--------------Increment and decrement operators helper functions-----------*/
        void increase() {
            int carry = 1, i = 0;
            while (carry) {
                if (i == allocated) 
                    growArray(4);
                carry += arr[i];
                arr[i++] = carry%10;
                carry = carry/10;
            }
            if (i > inUse) ++inUse;
            std::fill_n(arr+inUse, allocated-inUse, 0);
        }
        void decrease() {
            int sub, borrow = 1, i = 0;
            while (borrow) {
                sub = arr[i] - borrow;
                if (sub < 0) {
                    sub += 10;
                    borrow = 1;
                }
                else
                    borrow = 0;
                arr[i++] = sub;
            }
            if (i>1 && i == inUse && arr[i-1] == 0)
                --inUse;
        }
        /*------------------------------------------------------------------------------*/
        BigInt add(const BigInt& b, bool move = false) {
            BigInt result((inUse > b.inUse? inUse + 1 : b.inUse + 1), 0, isNeg);

            int carry = 0, i = 0, sum;
            for (; i < b.inUse; ++i) {
                sum = (i < inUse? arr[i] : 0) + b.arr[i] + carry;
                result.arr[i] = sum % 10;
                carry = sum/10;
            }
            while (carry) {
                sum = arr[i] + carry;
                result.arr[i] = sum % 10;
                carry = sum/10;
                ++i;
            }
            for (; i < inUse; ++i)
                result.arr[i] = arr[i];
            result.inUse = i;
            std::fill_n(result.arr+i, result.allocated-i, 0);
            if (move) this->move(std::forward<BigInt&&>(result));
            return result;
        }
        int doSubtract(s_type* a, int a_size, s_type* b, int b_size, s_type* res) {
            int borrow = 0, sub, i = 0;
            for (; i < b_size; ++i) {
                sub = a[i] - b[i] - borrow;
                if (sub < 0) {
                    sub += 10;
                    borrow = 1;
                }
                else
                    borrow = 0;
                res[i] = sub;
            }
            for (; i < a_size; ++i) {
                sub = a[i] - borrow;
                if (sub < 0) {
                    sub += 10;
                    borrow = 1;
                }
                else
                    borrow = 0;
                res[i] = sub;
            }
            if (res[i-1] == 0) --i;
            return i;
        }
        BigInt subtract(const BigInt& b, bool move = false) {
            BigInt result((inUse > b.inUse? inUse : b.inUse), 0, isNeg);

            //find which of the numbers is smaller and let 'l' point to the larger one
            //and 's' to the smaller one
            int diff = compare_help(arr, inUse, b.arr, b.inUse);
            const BigInt *l = this, *s = &b;
            if (diff == 0)
                result.reset();
            else {
                if (diff < 0) {
                    l = s; s = this;
                    result.isNeg = !result.isNeg;
                }
                int i = doSubtract(l->arr, l->inUse, s->arr, s->inUse, result.arr);
                result.inUse = (i == 0)? 1 : i;
                std::fill_n(result.arr+result.inUse, result.allocated-result.inUse, 0);
            }
            if (move) this->move(std::forward<BigInt&&>(result));
            return result;
        }
        BigInt multiply (const BigInt& b, bool move = false) {
            BigInt result(inUse+b.inUse, 0, (b.isNeg? !isNeg : isNeg));
            if (inUse + b.inUse + 1 < 19)
                result.move((long long)*this * (long long)b);
            else {
                std::fill_n(result.arr, result.allocated, 0);
                int x = 0, y = 0, carry;
                for (int i = 0; i < inUse; ++i) {
                    carry = 0;
                    y = 0;
                    for (int j = 0; j < b.inUse; ++j) {
                        int sum = arr[i]*b.arr[j] + result.arr[x+y] + carry;
                        carry = sum/10;
                        result.arr[x+y] = sum % 10;
                        ++y;
                    }
                    if (carry > 0)
                        result.arr[x+y] += carry;
                    ++x;
                }
                for (x = result.allocated-1; x > 0 && result.arr[x] == 0; --x);
                result.inUse = x+1;
            }
            
            if (move) this->move(std::forward<BigInt&&>(result));
            return result;
        }
        BigInt multiply(long& n, bool move = false) {
            BigInt result(inUse+4, 0, (n < 0? !isNeg : isNeg));
            long x = (n < 0)? -n : n;
            long long carry = 0;
            int i = 0;
            for(; i < inUse; ++i) {
                carry = arr[i] * x + carry;
                result.arr[i] = carry % 10;
                carry /= 10;
            }
            while (carry) {
                if (i == result.allocated) {
                    result.inUse = i;
                    result.growArray(4);
                } 
                result.arr[i++] = carry%10;
                carry /= 10;
            }
            result.inUse = i;
            std::fill_n(result.arr+i, result.allocated-i, 0);
            if (move) this->move(std::forward<BigInt&&>(result));
            return result;
        }
        BigInt divide(long& divisor, bool move = false) {
            if (divisor == 0)
                throw divisionByZero("Error In BigInt: divisionByZero check\n");
            BigInt result;
            if (divisor == 1)
                result = *this;
            else if (inUse > 1 && inUse < 19)
                result.move((long long)*this / divisor);
            else {
                bool neg = divisor < 0? !isNeg : isNeg;
                if (divisor < 0)
                    divisor = -divisor;
                if (inUse == 1) {
                    int q = arr[0] / divisor;
                    if (q > 0) result.isNeg = neg;
                    result.arr[0] = q;
                }
                else {
                    s_type a[inUse];
                    int i = inUse-1, j = 0;
                    long long temp = arr[i];
                    while (temp < divisor)
                        temp = temp * 10 + (arr[--i]);

                    while (i >= 0) {
                        a[j++] = (temp / divisor);
                        temp = (temp % divisor) * 10 + arr[--i];
                    }
                    if (j == 1) {
                        if (a[0] > 0) result.isNeg = neg;
                        result.arr[0] = a[0];
                    }
                    else {
                        delete [] result.arr;
                        result.allocated = j; result.inUse = 0;
                        result.arr = new s_type[j];
                        for (--j; j >= 0; --j)
                            result.arr[result.inUse++] = a[j];
                        result.isNeg = neg;
                    }
                }
            }
            if (move) this->move(std::forward<BigInt&&>(result));
            return result;
        }
        void divide_modulo_helper(const BigInt& b, s_type* quotient, s_type* temp, int& j, int &t_pos, int &m) {

            /* Strategy: Get a number E with as many digits from N as their are in D
            * (get one more digit if E is smaller than the divisor). Use repeated subtraction
            * to divide E by D and append the quotient to result. Let E be the remainder left
            * from division. Append as many elements in D to it. Repeat the division.
            */ 
            int pos = inUse;

            //loop till number of elements left is fewer than number of elements
            // we need to divide with.
            while (pos > 0) {

                //if we have b.inUse length of remainders, get one more digit since the remainder
                //is guaranteed to be less than b.
                if (pos < t_pos - 1) {
                    quotient[j++] = 0;
                    break;
                }
                if (t_pos == 1)
                    temp[--t_pos] = arr[--pos];
                //else get an extra t_pos-1 digits.
                else {
                    while (t_pos > 1)
                        temp[--t_pos] = arr[--pos];
                }

                int diff = compare_help(temp+t_pos, m-t_pos, b.arr, b.inUse);
                if (diff == 0) {
                    quotient[j++] = 1;
                    t_pos = m;
                }    
                else {
                    if (diff < 0) {
                        quotient[j++] = 0;
                        if (pos == 0) break;
                        //get an extra digit since temp is less than b.
                        temp[--t_pos] = arr[--pos];
                    }
                        
                    s_type* t = temp+t_pos; //used to offset the array for when t_pos == 1 || 0
                    int q = 0, i = m-t_pos;
                        
                    //Perform division by repeated subtraction
                    while (i > b.inUse || (i == b.inUse && t[i-1] > b.arr[i-1])) {
                        i = doSubtract(t, i, b.arr, b.inUse, t);
                        ++q;
                    }
                    if (i == b.inUse) {
                        int n = compare_help(t, i, b.arr, i);
                        if (n > 0) {
                            i = doSubtract(t, i, b.arr, b.inUse, t);
                            ++q;
                        }
                        else if (n == 0) {
                            i = 0; ++q;
                        }
                    }
                    quotient[j++] = q;
                    //update t_pos and shift the remainder to the end of the array if there's any
                    if (i == 0) t_pos = m;
                    else if (i == m-t_pos) t_pos = 1;
                    else {
                        t_pos = m;
                        for (int n = i-1; n >= 0; --n)
                            temp[--t_pos] = t[n];
                    }
                }
            }
        }
        
        BigInt divide(const BigInt& b, bool move = false) {
            if (b.inUse == 1 && b.arr[0] == 0)
                throw divisionByZero("Error In BigInt: divisionByZero check\n");
            BigInt result;
            result.isNeg = b.isNeg? !isNeg : isNeg;
            
            if (this->inUse < 19 && b.inUse < 19)
                result.move((long long)*this / (long long)b);
            
            else if (b.inUse <= inUse) {
                s_type quotient[inUse-b.inUse+1], temp[b.inUse+1];
                int j = 0, m = b.inUse+1;
                int t_pos = m;
                divide_modulo_helper(b, quotient, temp, j, t_pos, m);
                
                if (j == 1) result.arr[0] = quotient[0];
                else {
                    int start = 0;
                    if (quotient[0] == 0) ++start;
                    result.allocated = j - start;
                    result.inUse = result.allocated;
                    delete [] result.arr;
                    result.arr = new s_type [result.allocated];
                    for (int i = j-1, n = 0; i >= start; --i)
                        result.arr[n++] = quotient[i];
                }
            }
            if (move) this->move(std::forward<BigInt&&>(result));
            return result;
        }
        long long modulo1(long long modulus) {
            if (modulus == 0)
                throw divisionByZero("Error In BigInt: divisionByZero check\n");
            long long res = 0;
            if (inUse == 1) {
                res = arr[0] % modulus;
                if (res && isNeg) res = -res;
            }
            else if (modulus == 1) return 0;
            else if (modulus == 2) return (isNeg? -arr[0] : arr[0]) % 2;
            else if (inUse < 19)
                res = (long long)*this % modulus;
            //if modulus is too big (that is; bigger than (LLONG_MAX - 9)/10)
            //convert BigInt and take the modulus
            else if (modulus > 922337203685477579)
                res = (long long)modulo3(BigInt(modulus));
            else {
                for (int i = inUse-1; i >= 0; --i)
                    res = (res*10 + arr[i]) % modulus;
                if (res && isNeg) res = -res;
            }
            return res;
        }
        BigInt& modulo2(long long modulus) {
            if (modulus == 0)
                throw divisionByZero("Error In BigInt: divisionByZero check\n");
            if (inUse == 1) {
                arr[0] %= modulus;
                if (!arr[0]) isNeg = false;
            }
            else if (modulus == 1) this->reset();
            else if (modulus == 2) {
                s_type i = arr[0] % 2;
                if (i == 0)
                    this->reset();
                else
                    this->reset(1, isNeg);
            }
            else if (inUse < 19)
                this->move((long long)*this % modulus);
            else if (modulus > 922337203685477579)
                modulo3(BigInt(modulus), true);
            else {
                long long res = 0;
                for (int i = inUse-1; i >= 0; --i)
                    res = (res*10 + arr[i]) % modulus;
                this->move(BigInt(res));
            }
            return *this;
        }
        BigInt modulo3(const BigInt& b, bool move = false) {
            BigInt result;
            bool done = false;
            if (b.inUse == 1) {
                if (b.arr[0] == 0)
                    throw divisionByZero("Error In BigInt: divisionByZero check\n");
                else if (b.arr[0] == 1)
                    done = true;
                else if (b.arr[0] == 2) {
                    done = true;
                    result.arr[0] = arr[0] % 2;
                    if (result.arr[0]) result.isNeg = isNeg;
                }   
            }
            if (!done) {
                if (this->inUse < 19 && b.inUse < 19)
                    result.move((long long)*this % (long long)b);
                else if (b.inUse <= inUse) {
                    s_type quotient[inUse-b.inUse+1], temp[b.inUse+1];
                    int j = 0, m = b.inUse+1;
                    int t_pos = m;
                    divide_modulo_helper(b, quotient, temp, j, t_pos, m);

                    if (t_pos != m) {
                        delete [] result.arr;
                        result.allocated = m - t_pos;
                        result.arr = new s_type[allocated];
                        result.inUse = 0;
                        for (; t_pos < m; ++t_pos)
                            result.arr[result.inUse++] = temp[t_pos];
                        if (!result)
                            result.isNeg = isNeg;
                    }
                }
            }
            if (move) this->move(std::forward<BigInt&&>(result));
            return result;
        }
        //Used in left and right shift operators for multiplying or dividing by 2^63
        BigInt& get_shift63() {
            static BigInt shift63("9223372036854775808"); //same value as 2^63
            return shift63;
        }
        void shift_helper_fill() {
            if (!pow2[0]) {
                pow2[0] = 2;
                for (int i = 1; i < 62; ++i)
                    pow2[i] = 2 * pow2[i-1];
            }
        }
        template<typename Collection>
        void bitsHelper(BigInt& b, Collection& c) {
            bool x = b % 2;
            if (b.inUse > 1 || b.arr[0] > 1) {
                b /= 2;
                bitsHelper(b, c);
            }
            c.push_back(x);
        }
        void bitStringHelper(BigInt& b, std::string& s, int count = 0) {
            bool x = b % 2;
            if (b.inUse > 1 || b.arr[0] > 1) {
                b /= 2;
                bitStringHelper(b, s);
            }
            s.push_back('0'+x);
        }
        std::string to_words_helper(s_type* arr, int end, bool& isNeg) {
            std::string s;
            if (isNeg) {
                s += "minus ";
                isNeg = false;
            }
            static std::string units[] = {"Zero", "One", "Two", "Three", "Four",
            "Five", "Six", "Seven", "Eight", "Nine"};

            static std::string teens[] = {"Ten", "Eleven", "Twelve", "Thirteen",
            "Fourteen", "Fifteen", "Sixteen", "Seventeen", "Eighteen", "Nineteen"};

            static std::string tens_multiples[] = {"Twenty", "Thirty", "Forty",
            "Fifty", "Sixty", "Seventy", "Eighty", "Ninety"};

            static std::string th_powers[] = {"Thousand", "Million", "Billion",
            "Trillion", "Quadrillion", "Quintillion", "Sextillion", "Septillion",
            "Octillion", "Nonillion", "Decillion", "Undecillion", "Duodecillion",
            "Tredecillion", "Quattuordecillion", "Quindecillion", "Sexdecillion",
            "Septendecillion", "Octodecillion", "Novemdecillion", "Vigintillion"};

            static int powers[] = {4, 7, 10, 13, 16, 19, 22, 25, 28, 31, 34, 37,
                                40, 43, 46, 49, 52, 55, 58, 61, 64};
    
            if (end == 1)
                s += units[arr[0]];
            else if (end == 2 && arr[1] == 1)
                s += teens[arr[0]];
            else if (end < 3) {
                s += tens_multiples[arr[1] - 2];
                if (arr[0])
                    s += "-" + units[arr[0]];
            }
            else if (end < 4) {
                s += units[arr[2]] + " Hundred";
                --end;
                while (end > 0 && arr[end-1] == 0) --end;
                if (end > 0)
                    s += " and " + to_words_helper(arr, end, isNeg);
            }
            else {
                int x = lower_bound(powers, end, 21);  //21 is length of powers
                s += to_words_helper(arr+powers[x]-1, end-powers[x]+1, isNeg) + " " + th_powers[x];
                end = powers[x]-1;
                while (end > 0 && arr[end-1] == 0) --end;
                if (end > 0) {
                    s += (end < 3)? " and " : ", ";
                    s += to_words_helper(arr, end, isNeg);
                }
            }
            return s;
        }

        //Private Constructor
        BigInt (int size, int x, bool isNeg) {
            allocated = size;
            this->isNeg = isNeg;
            inUse = x;
            arr = new s_type[allocated];
        }
        public:
        BigInt() {
            arr = NULL; reset();
        }
        BigInt(unsigned short n, int count) {
            if (n < 1 || n > 9) {
                arr = NULL; reset();
            }
            else {
                allocated = count;
                arr = new s_type[allocated];
                for (inUse = 0; inUse < count; ++inUse)
                    arr[inUse] = n;
                isNeg = false;
            }
        }
        template <typename T,
            typename = typename std::enable_if<std::is_integral<T>::value>::type>
        BigInt(T n) {
            if (n < 0) {
                isNeg = true;  n = -n;
            }
            else
                isNeg = false;
            allocated = numLength(n);
            arr = new s_type[allocated];
            inUse = 0;
            arr[inUse++] = n % 10;
            while (n >= 10) {
                n /= 10;
                arr[inUse++] = n % 10;
            }
        }
        BigInt(long double n) {
            if (n < 0) {
                isNeg = true;  n = -n;
            }
            else
                isNeg = false;
            std::string s = std::to_string(n);
            int index = s.find_last_of('.');
            allocated = index;
            arr = new s_type[allocated];
            for (inUse = 0, --index; inUse < allocated; ++inUse, --index)
                arr[inUse] = s[index] - '0';
        }
        BigInt(const char* s): BigInt() {
            if (s != NULL) {
                if (s[0] == '-') {
                    isNeg = true; ++s;
                }
                int i = 0;
                for (; s[i] >= '0' && s[i] <= '9'; ++i);
                if (i == 1)
                    arr[0] = s[0] - '0';
                else if (i > 1) {
                    delete [] arr;
                    allocated = i; inUse = 0;
                    arr = new s_type[allocated];
                    for (--i; i >= 0; --i)
                        arr[inUse++] = s[i] - '0';
                } 
            }
        }
        BigInt(const BigInt& b) {
            allocated = b.inUse;
            inUse = b.inUse;
            isNeg = b.isNeg;
            arr = new s_type[allocated];
            for (int i = 0; i < inUse; ++i)
                arr[i] = b.arr[i];
        }
        #if __cplusplus >= 201103L
        BigInt(BigInt&& b) {
            arr = NULL;
            this->move(std::forward<BigInt&&>(b));
        }
        BigInt& operator=(BigInt&& b) {
            if (this != &b)
                this->move(std::forward<BigInt&&>(b));
            return *this;
        }
        #endif
        ~BigInt() {
            delete [] arr;
        }
        template <typename T,
            typename = typename std::enable_if<std::is_integral<T>::value ||
            std::is_floating_point<T>::value>::type>
        explicit operator T() const {
            T result = 0;
            for (int i = inUse-1; i >= 0; --i)
                result = (result*10) + arr[i];
            if (isNeg) result *= -1;
            return result;
        }
        template<bool> explicit operator bool() const {
            return (inUse > 1 || arr[0] > 0);
        }
        explicit operator std::string() const {
            std::string result;
            int j = 0;
            if (isNeg) {
                result.resize(inUse + 1);
                result[j++] = '-';
            }
            else
                result.resize(inUse);
            for (int i = inUse-1; i >= 0; i--)
                result[j++] = '0' + arr[i];
            return result;
        }
        BigInt& operator<<= (unsigned int n) {
            shift_helper_fill();
            while (n >= 63) {
                *this *= get_shift63();
                n -= 63;
            }
            if (n > 0)
                *this *= pow2[n-1];
            return *this;
        }
        BigInt& operator>>= (unsigned int n) {
            shift_helper_fill();
            while (n >= 63) {
                *this /= get_shift63();
                n -= 63;
            }
            if (n > 0)
                *this /= pow2[n-1];
            return *this;
        }
        BigInt operator>> (unsigned int n) {
            shift_helper_fill();
            BigInt result = *this;
            while (n >= 63) {
                result /= get_shift63();
                n -= 63;
            }
            if (n > 0)
                result /= pow2[n-1];
            return result;
        }
        BigInt operator<< (unsigned int n) {
            shift_helper_fill();
            BigInt result = *this;
            while (n >= 63) {
                result *= get_shift63();
                n -= 63;
            }
            if (n > 0)
                result *= pow2[n-1];
            return result;
        }
        BigInt& operator+=(const BigInt& b) {
            if (isNeg != b.isNeg)
                subtract(b, true);
            else
                add(b, true);
            return *this;
        }
        BigInt operator+(const BigInt& b) {
            if (isNeg != b.isNeg)
                return subtract(b);
            else
                return add(b);
        }
        BigInt& operator++() {
            if (isNeg) {
                decrease();
                if (inUse == 1 && arr[0] == 0)
                    isNeg = false;
            }
            else
                increase();
            return *this;
        }
        BigInt operator++(int) {
            BigInt temp = *this; ++*this; return temp;
        }
        BigInt& operator-=(const BigInt& b) {
            if (isNeg != b.isNeg)
                add(b, true);
            else
                subtract(b, true);
            return *this;
        }
        BigInt operator-(const BigInt& b) {
            if (isNeg != b.isNeg)
                return add(b);
            else
                return subtract(b);
        }
        BigInt& operator--() {
            if (!isNeg) {
                if (inUse == 1 && arr[0] == 0) {
                    arr[0] = 1;
                    isNeg = true;
                }
                else
                    decrease();
            }
            else
                increase();
            return *this;
        }
        BigInt operator*(const BigInt& b) {
            return multiply(b);
        }
        BigInt& operator*=(const BigInt& b) {
            multiply(b, true);
            return *this;
        }
        BigInt operator*(long b) {
            return multiply(b);
        }
        BigInt& operator*=(long b) {
            multiply(b, true);
            return *this;
        }
        BigInt operator/(long b) {
            return divide(b);
        }
        BigInt& operator/=(long b) {
            divide(b, true);
            return *this;
        }
        BigInt operator/(const BigInt& b) {
            return divide(b);
        }
        BigInt& operator/=(const BigInt& b) {
            divide(b, true);
            return *this;
        }
        BigInt operator%(const BigInt& b) {
            return modulo3(b);
        }
        BigInt& operator%=(const BigInt& b) {
            modulo3(b, true);
            return *this;
        }
        long long operator%(long long b) {
            return modulo1(b);
        }
        BigInt& operator%=(long long b) {
            return modulo2(b);
        }
        template <typename T,
            typename = typename std::enable_if<std::is_integral<T>::value ||
                std::is_floating_point<T>::value>::type>
        friend T& operator+=(T& t, const BigInt& b) {
            t += (T)b;
            return t;
        }
        template <typename T,
            typename = typename std::enable_if<std::is_integral<T>::value>::type>
        friend BigInt operator+(T t, const BigInt& b) {
            return BigInt(t) + b;
        }
        friend long double operator+(long double d, const BigInt& b) {
            return d + (long double) b;
        }
        template <typename T,
            typename = typename std::enable_if<std::is_integral<T>::value ||
                std::is_floating_point<T>::value>::type>
        friend T& operator-=(T& t, const BigInt& b) {
            t -= (T)b;
            return t;
        }
        template <typename T,
            typename = typename std::enable_if<std::is_integral<T>::value>::type>
        friend BigInt operator-(T t, const BigInt& b) {
            return BigInt(t) - b;
        }
        friend long double operator-(long double d, const BigInt& b) {
            return d - (long double) b;
        }
        template <typename T,
            typename = typename std::enable_if<std::is_integral<T>::value ||
                std::is_floating_point<T>::value>::type>
        friend T& operator*=(T& t, const BigInt& b) {
            t *= (T)b;
            return t;
        }
        template <typename T,
            typename = typename std::enable_if<std::is_integral<T>::value>::type>
        friend BigInt operator*(T t, const BigInt& b) {
            return BigInt(t) * b;
        }
        friend long double operator*(long double d, const BigInt& b) {
            return d + (long double) b;
        }
        template <typename T,
            typename = typename std::enable_if<std::is_integral<T>::value ||
                std::is_floating_point<T>::value>::type>
        friend T& operator/=(T& t, const BigInt& b) {
            t += (T)b;
            return t;
        }
        template <typename T,
            typename = typename std::enable_if<std::is_integral<T>::value>::type>
        friend T operator/(T t, const BigInt& b) {
            if (b.inUse < 19)
                return t / (T)b;
            else if (t > 999999999999999999)
                return (T) (BigInt(t) / b);
            return 0;
        }
        friend long double operator/(long double d, const BigInt& b) {
            return d / (long double) b;
        }
        template <typename T,
            typename = typename std::enable_if<std::is_integral<T>::value>::type>
        friend T& operator%=(T& t, const BigInt& b) {
            if (b.inUse < 19)
                t %= (T)b;
            else if (t > 999999999999999999) {   //if t has 19 digits
                t = (T) (BigInt(t) % b);
            }
            return t;
        }
        template <typename T,
            typename = typename std::enable_if<std::is_integral<T>::value>::type>
        friend T operator%(T t, const BigInt& b) {
            if (b.inUse < 19)
                return t % (T)b;
            else if (t > 999999999999999999) {
                return (T) (BigInt(t) % b);
            }
            return t;
        }
        BigInt& operator=(const BigInt& b) {
            if (this != &b) {
                delete [] arr;
                inUse = b.inUse;
                allocated = b.inUse;
                arr = new s_type[allocated];
                for (int i = 0; i < inUse; ++i)
                    arr[i] = b.arr[i];
            }
            return *this;
        }
        BigInt operator-() {
            BigInt result = *this;
            if (result)
                result.isNeg = !isNeg;
            return result;
        }
        BigInt& negate() {
            if (*this)
                isNeg = !isNeg;
            return *this;
        }
        s_type operator[] (unsigned int i) {
            if (i >= inUse)
                throw indexOutOfRange("In BigInt::range_check. Index is out of range.");
            return arr[inUse-i-1];
        }
        void set (unsigned int i, s_type val) {
            if (i >= inUse || val > 9 || (i == 0 && val == 0))
                return;
            arr[inUse-i-1] = val;
        }
        template<typename T, typename A,
            template <typename, typename> class Collection, typename = typename std::enable_if<std::is_integral<T>::value>::type>
        void to_bits(Collection<T, A>& c) {
            BigInt temp = *this;
            bitsHelper(temp, c);
        }
        std::string to_bit_string() {
            std::string res;
            res.reserve(inUse*2);
            BigInt temp = *this;
            bitStringHelper(temp, res);
            return res;
        }
        BigInt& left_shift (unsigned int n) {
            if (inUse > 1 || arr[0] > 0) {
                allocated = inUse+n;
                s_type* temp = new s_type[allocated];
                int j = allocated-1;
                for (int i = inUse-1; i >= 0; --i, --j)
                    temp[j] = arr[i];
                for (; j >= 0; --j)
                    temp[j] = 0;
                inUse = allocated;
                delete [] arr;
                arr = temp;
            }
            return *this;
        }
        BigInt& right_shift (unsigned int n) {
            if (inUse > 1 || arr[0] > 0) {
                if (n >= inUse) {
                    delete [] arr;
                    allocated = 1; inUse = 1;
                    arr = new s_type[allocated];
                    arr[0] = 0;
                }
                else {
                    for (int i = n; i < inUse; ++i)
                        arr[i-n] = arr[i];
                    inUse -= n;
                }
            }
            return *this;
        }
        int compare(const BigInt& b) const {
            if (!isNeg && b.isNeg)
                return 1;
            if (isNeg && !b.isNeg)
                return -1;
            int c = compare_help(arr, inUse, b.arr, b.inUse);
            return (isNeg && b.isNeg)? -c : c;
        }
        size_t length() const {return inUse;}
        size_t capacity() const {return allocated;}

        void shrink_to_fit() {
            allocated = inUse;
            s_type* temp = new s_type[allocated];
            for (int i = 0; i < inUse; ++i)
                temp[i] = arr[i];
            delete [] arr;
            arr = temp;
        }

        std::string to_words() {
            if (inUse > 64)
                return "undefined";
            else {
                bool neg = isNeg;
                return to_words_helper(arr, inUse, neg);
            }  
        }

        friend void swap(BigInt& a, BigInt& b) {
            s_type* t_arr = b.arr; b.arr = a.arr; a.arr = t_arr;
            size_t t_inUse = b.inUse; b.inUse = a.inUse; a.inUse = t_inUse;
            size_t cap = b.allocated; b.allocated = a.allocated; a.allocated = cap;
            bool neg = b.isNeg; b.isNeg = a.isNeg; a.isNeg = neg;
        }
        friend BigInt abs(const BigInt& b) {
            BigInt result = b;
            result.isNeg = false;
            return result;
        }
        friend BigInt pow(BigInt x, unsigned int y) {
            BigInt res(1,1);

            while (y > 0) { 
                if (y % 2 == 1)
                    res *= x;
    
                y = y >> 1;
                x *= x;
            } 
            return res;
        }
        friend std::string to_string(const BigInt& b) {return (std::string)b;}
        friend std::ostream& operator<<(std::ostream& os, const BigInt& b);
        friend std::istream& operator>>(std::istream& os, BigInt& b);
    };

    /*------------------relational operators------------*/
    bool operator<(const BigInt& lhs, const BigInt& rhs) {
        return lhs.compare(rhs) < 0;
    }
    bool operator<=(const BigInt& lhs, const BigInt& rhs) {
        return lhs.compare(rhs) <= 0;
    }
    bool operator>(const BigInt& lhs, const BigInt& rhs) {
        return lhs.compare(rhs) > 0;
    }
    bool operator>=(const BigInt& lhs, const BigInt& rhs) {
        return lhs.compare(rhs) >= 0;
    }
    bool operator==(const BigInt& lhs, const BigInt& rhs) {
        return lhs.compare(rhs) == 0;
    }
    bool operator!=(const BigInt& lhs, const BigInt& rhs) {
        return lhs.compare(rhs) != 0;
    }
    /*----------------------------------------------------*/
    
    std::ostream& operator<<(std::ostream& os, const BigInt& b) {
        if (b.isNeg)
            os << '-';
        for (int i = b.inUse-1; i >= 0; i--)
            os << b.arr[i];
        return os;
    }
    std::istream& operator>>(std::istream& is, BigInt& b) {
        std::string s;
        is >> s;
        b.move(BigInt(s.c_str()));
        return is;
    }
    
    BigInt factorial(unsigned int n) {
        BigInt result(1,1);
        for (; n > 1; --n)
            result *= n;
        return result;
    }       
}
#endif
