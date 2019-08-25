# Big-Integer
A signed class of integers to store integers of unlimited length.

----------------------------Constructors-----------------------------
1) BigInt(); -> Default Constructor : Initializes value to zero

2) BigInt(T n, int length); -> Construct from an integer type T. The length variable specifies the length of the number.

3) BigInt(T n); -> This constructor determines the length of the number first before constructing it.

4) BigInt(long double n); -> Constructs from a floating point number.

5) BigInt(const char* s); -> Construct from a character sequence of numbers.

6) BigInt(const BigInt& b); -> Copy constructor

7) BigInt(BigInt&& b); -> Move Constructor

---------------------------Assignment Operators------------------------
1) BigInt& operator= (const BigInt& b); -> Copy assignment operator. b is unaffected.

2) BigInt& operator= (BigInt&& b); -> Move assignment operator. this becomes b, b becomes zero.

------------------------------Number Operators and Functions-----------------------------
Note: If a primitive type is passed as parameter to a function below without a primitive type overload, a BigInt is constructed from the value and passed to the function;

1) BigInt& operator+= (const BigInt& b);

2) BigInt operator+ (const BigInt& b);

3) BigInt& operator-= (const BigInt& b);

4) BigInt operator- (const BigInt& b);

5) BigInt& operator*= (const BigInt& b);

6) BigInt operator* (const BigInt& b);

7) BigInt& operator*= (long  multiplier);

8) BigInt operator* (long  multiplier);

The division operators throws a divisionByZero exception if the divisor is less than the dividend.

9) BigInt& operator/= (const BigInt& b);

10) BigInt operator/ (const BigInt& b);

11) BigInt& operator/= (long divisor);

12) BigInt operator/ (long divisor);

13) BigInt operator%(const BigInt& b);

14) BigInt& operator%=(const BigInt& b);

15) long long operator%(long long b);

16) BigInt& operator%=(long long b);

17) BigInt operator<< (unsigned int n); -> Bit wise left-shift by n. This is tantamount to multiplying this by 2^n.

18) BigInt operator>> (unsigned int n); -> Bit wise right-shift by n. This is tantamount to dividing this by 2^n.

19) BigInt& operator<<= (unsigned int n);

20) BigInt& operator>>= (unsigned int n);

21) BigInt& left_shift (unsigned int); -> Decimal left-shift by n. This is tantamount to multiplying this by 10^n.

22) BigInt& right_shift (unsigned int n); Decimal right-shift by n. This is tantamount to dividing this by 10^n.

---------------------------Non-member Math operators----------------------------
For the functions below, T must be an integer or floating point type.

1) template<typename T> T& operator+= (T& t, const BigInt& b);

2) template<typename T> T operator+ (T t, const BigInt& b);

3) template<typename T> T& operator*= (T& t, const BigInt& b);

4) template<typename T> T operator* (T t, const BigInt& b);

5) template<typename T> T& operator/= (T& t, const BigInt& b);

6) template<typename T> T operator/ (T t, const BigInt& b);

3) template<typename T> T& operator-= (T& t, const BigInt& b);

4) template<typename T> T operator- (T t, const BigInt& b);
  
----------------------------Conversion Operators-----------------------
1) template<typename T> operator T() const;  -> T must be an integer or floating point type.
  
2) template<bool> operator bool() const; -> template specialization. Return true if number > 0.
  
------------------------------Comparison functions-----------------------
1) int compare(const BigInt& b); -> return a number n. n is greater than 0 if this > b, equal to 0 if this == b, less than 0 if this < b.

Non-member relational operators:

2) bool operator< (const BigInt& lhs, const BigInt& rhs);

3) bool operator<= (const BigInt& lhs, const BigInt& rhs);

4) bool operator> (const BigInt& lhs, const BigInt& rhs);

5) bool operator>= (const BigInt& lhs, const BigInt& rhs);

6) bool operator== (const BigInt& lhs, const BigInt& rhs);

7) bool operator!= (const BigInt& lhs, const BigInt& rhs);

-------------------------Other member functions--------------------------
1) size_t length(); -> Returns the number of digits in the number.

2) size_t capacity(); -> Return the amount of storage allocated to store the number.

3) void shrink_to_fit();

-------------------------------Other Non-member Functions---------------------
1) swap(BigInt& a, BigInt& b);

2) std::string to_string(const BigInt& b);

3) std::ostream& operator<<(std::ostream& os, const BigInt& b);

4) std::istream& operator>>(std::istream& os, BigInt& b);

5) BigInt factorial(unsigned int);
