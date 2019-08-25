#ifndef BIGINT_H
#define BIGINT_H

#include <iostream>

namespace my {
    struct divisionByZero : public std::exception {
        const char* msg;
        divisionByZero(const char* s = "Error! divisionByZero\n") {
            msg = s;
        }
        const char * what () const throw () {
            return msg;
        }
    };
    long long pow2[63] = {0};  //look up table for left & right shifting
    long long lengths[19] = {0}; //look up table to find number lengths

    class BigInt {
        private:
        short int* arr;
        int allocated;
        int inUse;
        bool isNeg;

        void growArray(int n) {
            allocated += (n < 1)? 2: n;
            short int* temp = new short int[allocated];
            for (int i = 0; i < inUse; ++i)
                temp[i] = arr[i];
            delete [] arr;
            arr = temp;
        }
        int numLength(long long n) const {
            if (!lengths[0]) {     //this executes only once
                lengths[0] = 10;
                for (int i = 1; i < 18; ++i)
                    lengths[i] = 10 * lengths[i-1];
                lengths[18] = 9223372036854775807;   //LL_Max
            }
            if (n < 10) return 1;
            //Use binary search to find the length of n
            int i = 0, j = 19, mid;
            while (i < j) {
                mid = (i + j) / 2;

                if (lengths[mid] == n) 
                    return mid+1;
                if (n < lengths[mid]) {
                    if (mid > 0 && n > lengths[mid-1])
                        return mid+1;
                    j = mid;
                }
                else {
                    if (mid < 18 && n < lengths[mid+1])
                        return mid+2;
                    i = mid+1;
                }
            }
            return mid+1;
        }
        //used in all move operations
        void move(BigInt&& b) {
            delete [] arr;
            allocated = b.allocated; b.allocated = 1;
            inUse = b.inUse; b.inUse = 1;
            isNeg = b.isNeg; b.isNeg = false;
            arr = b.arr; b.arr = new short int[1]; b.arr[0] = 0;
        }
        //Resets value to zero
        void reset() {
            delete [] arr;
            allocated = 1; inUse = 1; isNeg = false;
            arr = new short int[allocated];
            arr[0] = 0;
        }
        //compares two arrays of integers in reverse manner starting from the last element
        int compare_help(short int* a, int a_size, short int* b, int b_size) const {
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
        //Used in left and right shift operators
        BigInt& getLL_Max() {
            static BigInt LL_Max("18446744073709551616");
            return LL_Max;
        }
        
        BigInt add(const BigInt& b, bool move = false) {
            BigInt result((inUse > b.inUse? inUse + 1 : b.inUse + 1), isNeg);

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
        int doSubtract(short int* a, int a_size, short int* b, int b_size, short int* res) {
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
            BigInt result((inUse > b.inUse? inUse : b.inUse), isNeg);

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
            BigInt result(inUse+b.inUse, (b.isNeg? !isNeg : isNeg));
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
            BigInt result(inUse+4, (n < 0? !isNeg : isNeg));
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
            bool neg = divisor < 0? !isNeg : isNeg;
            BigInt result;
            if (inUse == 1) {
                int q = arr[0] / divisor;
                if (q > 0) result.isNeg = neg;
                result.arr[0] = q;
            }
            else if (inUse < 19) {
                result.move((long long)*this / divisor);
            }
            else {  
                short int a[inUse];
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
                    result.arr = new short int[j];
                    for (--j; j >= 0; --j)
                        result.arr[result.inUse++] = a[j];
                    result.isNeg = neg;
                }
            }
            if (move) this->move(std::forward<BigInt&&>(result));
            return result;
        }
        void divide_modulo_helper(const BigInt& b, short int* quotient, short int* temp, int& j, int &t_pos, int &m) {

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
                        
                    short int* t = temp+t_pos; //used to offset the array for when t_pos == 1 || 0
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
                short int quotient[inUse-b.inUse+1], temp[b.inUse+1];
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
                    result.arr = new short int [result.allocated];
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
            long long max = 922337203685477580;
            if (inUse == 1) {
                res = arr[0] % modulus;
                if (res > 0 && isNeg) res = -res;
            }
            else if (inUse < 19)
                res = (long long)*this % modulus;
            else if (modulus > max)
                res = (long long)modulo3(BigInt(modulus));
            else {
                for (int i = inUse-1; i >= 0; --i)
                    res = (res*10 + arr[i]) % modulus;
            }
            return res;
        }
        BigInt& modulo2(long long modulus) {
            if (modulus == 0)
                throw divisionByZero("Error In BigInt: divisionByZero check\n");
            long long max = 922337203685477580;
            if (inUse == 1) {
                arr[0] %= modulus;
                if (!arr[0]) isNeg = false;
            }
            else if (inUse < 19)
                this->move((long long)*this % modulus);
            else if (modulus > max)
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
            if (b.inUse == 1 && b.arr[0] == 0)
                throw divisionByZero("Error In BigInt: divisionByZero check\n");
            BigInt result;
            
            if (this->inUse < 19 && b.inUse < 19)
                result.move((long long)*this % (long long)b);
            else if (b.inUse <= inUse) {
                short int quotient[inUse-b.inUse+1], temp[b.inUse+1];
                int j = 0, m = b.inUse+1;
                int t_pos = m;
                divide_modulo_helper(b, quotient, temp, j, t_pos, m);

                if (t_pos != m) {
                    delete [] result.arr;
                    result.allocated = m - t_pos;
                    result.arr = new short int[allocated];
                    result.inUse = 0;
                    for (; t_pos < m; ++t_pos)
                        result.arr[result.inUse++] = temp[t_pos];
                    if (!result)
                        result.isNeg = isNeg;
                }
            }
            if (move) this->move(std::forward<BigInt&&>(result));
            return result;
        }
        BigInt (int size, bool isNeg) {
            allocated = size;
            this->isNeg = isNeg;
            inUse = 0;
            arr = new short int[allocated];
        }
        public:
        BigInt() {
            arr = NULL; reset();
        }
        template <typename T,
            typename = typename std::enable_if<std::is_integral<T>::value>::type>
        BigInt(T n, int length) {
            if (n < 0) {
                isNeg = true;  n = -n;
            }
            else
                isNeg = false;
            allocated = length;
            arr = new short int[allocated];
            inUse = 0;
            arr[inUse++] = n % 10;
            for (; inUse < length && n >= 10; ++inUse) {
                n /= 10;
                arr[inUse] = n % 10;
            }
            while (inUse > 1 && arr[inUse-1] == 0) --inUse;
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
            arr = new short int[allocated];
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
            arr = new short int[allocated];
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
                    arr = new short int[allocated];
                    for (--i; i >= 0; --i)
                        arr[inUse++] = s[i] - '0';
                } 
            }
        }
        BigInt(const BigInt& b) {
            allocated = b.inUse;
            inUse = b.inUse;
            isNeg = b.isNeg;
            arr = new short int[allocated];
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
        BigInt& left_shift (unsigned int n) {
            if (inUse > 1 || arr[0] > 0) {
                allocated = inUse+n;
                short int* temp = new short int[allocated];
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
                    arr = new short int[allocated];
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
        void check_and_fill() {
            if (!pow2[0]) {
                pow2[0] = 2;
                for (int i = 1; i < 63; ++i)
                    pow2[i] = 2 * pow2[i-1];
            }
        }
        BigInt& operator<<= (unsigned int n) {
            check_and_fill();
            while (n >= 64) {
                *this *= getLL_Max();
                n -= 64;
            }
            if (n > 0)
                *this *= pow2[n-1];
            return *this;
        }
        BigInt& operator>>= (unsigned int n) {
            check_and_fill();
            while (n >= 64) {
                *this /= getLL_Max();
                n -= 64;
            }
            if (n > 0)
                *this /= pow2[n-1];
            return *this;
        }
        BigInt operator>> (unsigned int n) {
            check_and_fill();
            BigInt result = *this;
            while (n >= 64) {
                result /= getLL_Max();
                n -= 64;
            }
            if (n > 0)
                result /= pow2[n-1];
            return result;
        }
        BigInt operator<< (unsigned int n) {
            check_and_fill();
            BigInt result = *this;
            while (n >= 64) {
                result *= getLL_Max();
                n -= 64;
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
            typename = typename std::enable_if<std::is_integral<T>::value ||
                std::is_floating_point<T>::value>::type>
        friend T operator+(T t, const BigInt& b) {
            return t + (T)b;
        }
        template <typename T,
            typename = typename std::enable_if<std::is_integral<T>::value ||
                std::is_floating_point<T>::value>::type>
        friend T& operator-=(T& t, const BigInt& b) {
            t -= (T)b;
            return t;
        }
        template <typename T,
            typename = typename std::enable_if<std::is_integral<T>::value ||
                std::is_floating_point<T>::value>::type>
        friend T operator-(T t, const BigInt& b) {
            return t - (T)b;
        }
        template <typename T,
            typename = typename std::enable_if<std::is_integral<T>::value ||
                std::is_floating_point<T>::value>::type>
        friend T& operator*=(T& t, const BigInt& b) {
            t *= (T)b;
            return t;
        }
        template <typename T,
            typename = typename std::enable_if<std::is_integral<T>::value ||
                std::is_floating_point<T>::value>::type>
        friend T operator*(T t, const BigInt& b) {
            return t * (T)b;
        }
        template <typename T,
            typename = typename std::enable_if<std::is_integral<T>::value ||
                std::is_floating_point<T>::value>::type>
        friend T& operator/=(T& t, const BigInt& b) {
            t += (T)b;
            return t;
        }
        template <typename T,
            typename = typename std::enable_if<std::is_integral<T>::value ||
                std::is_floating_point<T>::value>::type>
        friend T operator/(T t, const BigInt& b) {
            return t + (T)b;
        }
        template <typename T,
            typename = typename std::enable_if<std::is_integral<T>::value>::type>
        friend T& operator%=(T& t, const BigInt& b) {
            t %= (T)b;
            return t;
        }
        template <typename T,
            typename = typename std::enable_if<std::is_integral<T>::value>::type>
        friend T operator%(T t, const BigInt& b) {
            return t % (T)b;
        }
        BigInt& operator=(const BigInt& b) {
            if (this != &b) {
                delete [] arr;
                inUse = b.inUse;
                allocated = b.inUse;
                arr = new short int[allocated];
                for (int i = 0; i < inUse; ++i)
                    arr[i] = b.arr[i];
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
            short int* temp = new short int[allocated];
            for (int i = 0; i < inUse; ++i)
                temp[i] = arr[i];
            delete [] arr;
            arr = temp;
        }

        friend void swap(BigInt& a, BigInt& b) {
            short int* t_arr = b.arr; b.arr = a.arr; a.arr = t_arr;
            size_t t_inUse = b.inUse; b.inUse = a.inUse; a.inUse = t_inUse;
            size_t cap = b.allocated; b.allocated = a.allocated; a.allocated = cap;
            bool neg = b.isNeg; b.isNeg = a.isNeg; a.isNeg = neg;
        }

        friend std::string to_string(const BigInt&);
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
    std::string to_string(const BigInt& b) {
        std::string result;
        int j = 0;
        if (b.isNeg) {
            result.resize(b.inUse + 1);
            result[j++] = '-';
        }
        else
            result.resize(b.inUse);
        for (int i = b.inUse-1; i >= 0; i--)
            result[j++] = '0' + b.arr[i];
        return result;
    }
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
