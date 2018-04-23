/*
 * CS:APP Data Lab
 *
 * <Please put your name and userid here>
 *
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:

  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code
  must conform to the following style:

  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>

  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.


  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function.
     The max operator count is checked by dlc. Note that '=' is not
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 *
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce
 *      the correct answers.
 */


#endif
/*
 * bitAnd - x&y using only ~ and |
 *   Example: bitAnd(6, 5) = 4
 *   Legal ops: ~ |
 *   Max ops: 8
 *   Rating: 1
 */
int bitAnd(int x, int y) {
  return ~((~x) | (~y) ) ;
}
/*
 * getByte - Extract byte n from word x
 *   Bytes numbered from 0 (LSB) to 3 (MSB)
 *   Examples: getByte(0x12345678,1) = 0x56
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 2
 */
int getByte(int x, int n) {
    int res = ( x >> ( n << 3 ) ) & 0xff ;
    //int res = ( x & ( 0xff << ( n << 3 ) ) ) >> ( n << 3 );
    // 算数右移，所以第二种不行
    return res ;

}
/*
 * logicalShift - shift x to the right by n, using a logical shift
 *   Can assume that 0 <= n <= 31
 *   Examples: logicalShift(0x87654321,4) = 0x08765432
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3
 */
int logicalShift(int x, int n) {
    int a = x & 0x80000000 ;         // 符号位
    int b = x & 0x7fffffff ;         // 数据位
    int res = ( b >> n ) | ( (a >> n) & (1 << ( 32 + ~n )) ) ;
    return res ;
}
/*
 * bitCount - returns count of number of 1's in word
 *   Examples: bitCount(5) = 2, bitCount(7) = 3
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 40
 *   Rating: 4
 */
int bitCount(int x) {
    int tmp = 0x01010101 ;
    int res = x & tmp ;
    res += ((x >> 1) & tmp) ;
    res += ((x >> 2) & tmp) ;
    res += ((x >> 3) & tmp) ;
    res += ((x >> 4) & tmp) ;
    res += ((x >> 5) & tmp) ;
    res += ((x >> 6) & tmp) ;
    res += ((x >> 7) & tmp) ;
    int bits = (res & 0xff) + ((res >> 8) & 0xff) + ((res >> 16) & 0xff) + ((res >> 24) & 0xff) ;
    // 注意要加括号，位运算的优先级是比➕高的!!!
    return bits ;
}
/*
 * bang - Compute !x without using !
 *   Examples: bang(3) = 0, bang(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4
 */
int bang(int x) {
    /* 法一：
     * 把所有位都移到最后一位*/
    x |= ( x >> 16 ) ;
    x |= ( x >> 8 ) ;
    x |= ( x >> 4 ) ;
    x |= ( x >> 2 ) ;
    x |= ( x >> 1 ) ;
    return ( x & 0x1 ) ^ 0x1 ;
    /*
     * 法二：
     * 利用 0和 -0的符号相同，其他的数字和自己的相反数符号位不同
    return ((~(~x+1)^x) >> 31) & 0x1 ;
    */
}
/*
 * tmin - return minimum two's complement integer
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
    // 最小的补码。。
    return 0x80000000 ;
}
/*
 * fitsBits - return 1 if x can be represented as an
 *  n-bit, two's complement integer.
 *   1 <= n <= 32
 *   Examples: fitsBits(5,3) = 0, fitsBits(-4,3) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
int fitsBits(int x, int n) {
    /*
     * 正数的前面全是0，负数的前面全是1
     * 思路：检查前32-n位是否相等
     * 利用算数右移的性质
     */
    int shift = ~n + 33 ;
    return !(x ^ (x << shift >> shift)) ;
}
/*
 * divpwr2 - Compute x/(2^n), for 0 <= n <= 30
 *  Round toward zero
 *   Examples: divpwr2(15,1) = 7, divpwr2(-33,4) = -2
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
int divpwr2(int x, int n) {
    /*
     * 加一的条件：
     * 1. x是负数
     * 2. 被右移的n位里面有1（就是有被省去）
     */
    int res = x >> n ;
    int tmp = ~((~0)<<n) & x ;
    res += ((( x >> 31) & 0x1) & (!(!tmp))) ;
    return res;
}
/*
 * negate - return -x
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
  return ~x + 1 ;
}
/*
 * isPositive - return 1 if x > 0, return 0 otherwise
 *   Example: isPositive(-1) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 8
 *   Rating: 3
 */
int isPositive(int x) {
    /*
     * 检查符号位位0，并且不等于0
     */
    return !((x >> 31)& 0x1) & !(!x) ;
}
/*
 * isLessOrEqual - if x <= y  then return 1, else return 0
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
    /*
     * 返回1的条件
     * 1. x < 0 且 y > 0
     * 2. x, y 符号相同时，z = x - y，其中z 满足 z < 0 或 z == 0
     * */
    int fx = ( x >> 31 ) & 0x1 ;
    int fy = ( y >> 31 ) & 0x1 ;
    int z = x + ~y + 1 ;
    return (( fx ^ fy ) & fx ) | ( (!(fx^fy)) & ((( z >> 31 ) & 0x1)|(!z)) ) ;
}
/*
 * ilog2 - return floor(log base 2 of x), where x > 0
 *   Example: ilog2(16) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 90
 *   Rating: 4
 */
int ilog2(int x) {
    int count = 0 ;
    count = count + ( !!(x>>(count+16)) << 4 ) ;
    count = count + ( !!(x>>(count+ 8)) << 3 ) ;
    count = count + ( !!(x>>(count+ 4)) << 2 ) ;
    count = count + ( !!(x>>(count+ 2)) << 1 ) ;
    count = count + ( !!(x>>(count+ 1)) << 0 ) ;
    return count ;
}
/*
 * float_neg - Return bit-level equivalent of expression -f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representations of
 *   single-precision floating point values.
 *   When argument is NaN, return argument.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 10
 *   Rating: 2
 */
unsigned float_neg(unsigned uf) {
    if ( ( uf & 0x7fffffff ) > 0x7f800000 ) // NaN
        return uf ;
    return uf ^ 0x80000000 ;
}
/*
 * float_i2f - Return bit-level equivalent of expression (float) x
 *   Result is returned as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point values.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_i2f(int x) {
    if ( x == 0 ) return 0 ;
    if ( x == 0x80000000) return 3472883712u ;
    int sign = !!(x&0x80000000) ;               // 符号位
    if ( sign ) x = ~x + 1 ;                    // 取绝对值
    int first = 30 ;
    while ( !(x >> first) )                    //  找出第一位1的位置
        first-- ;
    int exp = first + 127 ;
    x <<= ( 31 - first ) ;                     // 消除第一位的1 前面的数，包括符号位
    int fac = 0x7fffff & ( x >> 8 ) ;          // 取后23位做为 fac, x的后8被舍去(int型不可能是非格式化，所有虽然是舍去9位，但是第一位是1，所以x是后移8位)
    x = x & 0xff ;                             // 取x的后8位，判断是不是应该进位
    if ( x > 128 || ( x == 128 && (fac & 0x1 )))
        fac++ ;
    if ( fac >> 23 ) {
        fac &= 0x7fffff ;                      // 超出23位，只取23位，阶码加一
        exp++ ;
    }
    return (sign << 31) | ( exp << 23 ) | fac ;
}
/*
 * float_twice - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_twice(unsigned uf) {
    if ( (uf & 0x7f800000) == 0 ) {
        uf = (( uf & 0x007fffff )<< 1) | (0x80000000 & uf) ;  // 非格式化的数
    }
    else if ( ( uf & 0x7f800000 ) != 0x7f800000 ) {           // 格式化的数，且不是特殊值
        uf += 0x00800000 ;
    }
    return uf ;
}
