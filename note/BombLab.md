# Bomb Lab 

本实验我们了解了C语言提供的抽象层下面的东西，通过阅读编译器生成的汇编代码，了解了编译器和它的优化能力，以及机器、数据类型和指令集。

Bomb Lab有6个炸弹（其实是7个）。

下载代码后，使用反编译器执行 objdump -d bomb > bomb.txt获取bomb程序反汇编的全部内容。

查看`main`函数，发现调用了 phase_1, phase_2, phase_3, phase_4, phase_5, phase_6和 secret_phase。

用gdb 启动 bomb，并设置相应断点（比如查看phase_1时,要在phase_1,phase_2处设置断点）

## phase_1

这是第一个函数：

```
0000000000400ee0 <phase_1>:
  400ee0:   48 83 ec 08             sub    $0x8,%rsp                        // 开辟的8b的栈帧             
  400ee4:   be 00 24 40 00          mov    $0x402400,%esi                   // %esi = $0x40240 
  400ee9:   e8 4a 04 00 00          callq  401338 <strings_not_equal>       // 调用 strings_not_equal(%rdi,%rsi)
  400eee:   85 c0                   test   %eax,%eax                        // 检查strings_not_equal的函数的返回值
  400ef0:   74 05                   je     400ef7 <phase_1+0x17>            // 为0就跳转
  400ef2:   e8 43 05 00 00          callq  40143a <explode_bomb>            // 引爆炸弹 
  400ef7:   48 83 c4 08             add    $0x8,%rsp                        // 回收栈帧 
  400efb:   c3                      retq                                    // 函数返回 
```

发现`400ef7`行有 `explode_bomb`函数，就知道了触发了这个函数就会引爆炸弹，实验失败，所以我们的任务就是跳过这些`explode_bomb`函数。同时，在使用gdb调试时，同样要给 `explode_bomb`函数打断点，防止引发炸弹。

阅读程序，发现是，比较 以%rdi为地址的字符串和 以%rsi为地址的字符串(指定字符串)，如果不相等，就会引爆炸弹。（根据X86-64的参数传递规则，我们知道%rsi寄存器保存的是函数调用的第二个参数，%rdi寄存器保存的是函数调用的第一个参数，然后调用`strings_not_equal`函数。）

使用gdb查看 %rsi为地址的字符串(0x402400):  `x/s 0x402400`，得到字符串 `Border relations with Canada have never been better.`

%rdi中则是我们输入的字符串，所以我们要输入 `Border relations with Canada have never been better.`  第一个炸弹就解开了。

## phase_2

第二个函数:

```
0000000000400efc <phase_2>:
  400efc:   55                      push   %rbp                        // 保存 %rbp
  400efd:   53                      push   %rbx                        // 保存 %rbx 
  400efe:   48 83 ec 28             sub    $0x28,%rsp                  // 分配 40b 栈帧
  400f02:   48 89 e6                mov    %rsp,%rsi                   // %rsi = %rsp 
  400f05:   e8 52 05 00 00          callq  40145c <read_six_numbers>   // 调用read_six_numbers，读6个数，%rsp为第一个数字的地址
-------------------------------------------------------------------                 // 分隔  
  400f0a:   83 3c 24 01             cmpl   $0x1,(%rsp)                 // 比较读入的第一个数与1，发现第一个数必须是1
  400f0e:   74 20                   je     400f30 <phase_2+0x34>       // 跳转到到400f30 
  400f10:   e8 25 05 00 00          callq  40143a <explode_bomb>
  400f15:   eb 19                   jmp    400f30 <phase_2+0x34>
  
  400f17:   8b 43 fc                mov    -0x4(%rbx),%eax             // %eax = %rbx - 4
  400f1a:   01 c0                   add    %eax,%eax                   // %eax = %eax + %eax 
  400f1c:   39 03                   cmp    %eax,(%rbx)                 // 比较 %eax 与 (%rbx) 中的值，从这里知道前后两个数有一个二倍的关系
  400f1e:   74 05                   je     400f25 <phase_2+0x29>       // 相等就跳转到 400f25 
  400f20:   e8 15 05 00 00          callq  40143a <explode_bomb>       // 不相等就爆炸
  400f25:   48 83 c3 04             add    $0x4,%rbx                   // %rbx = %rbx + 4 指向下一个数 
  400f29:   48 39 eb                cmp    %rbp,%rbx                   // %rbx 比较 %rbp，即是否已经到达最后一个数 
  400f2c:   75 e9                   jne    400f17 <phase_2+0x1b>       // 未到最后一个数就回到400f17，发现这是一个循环 
  400f2e:   eb 0c                   jmp    400f3c <phase_2+0x40>       // 跳出循环，跳转到 400f3c

  400f30:   48 8d 5c 24 04          lea    0x4(%rsp),%rbx              // %rbx = %rsp + 4，%rbx指向读入的下一个数，此时%rbx是第二个数
  400f35:   48 8d 6c 24 18          lea    0x18(%rsp),%rbp             // %rbp = %rsp +24, %rbp指向读入的最后一个数
  400f3a:   eb db                   jmp    400f17 <phase_2+0x1b>       // 跳到 400f17 
-------------------------------------------------------------------    // 分隔线 
  400f3c:   48 83 c4 28             add    $0x28,%rsp                  // 回收栈帧  
  400f40:   5b                      pop    %rbx                        // 弹出 %rbx
  400f41:   5d                      pop    %rbp                        // 弹出 %rbp 
  400f42:   c3                      retq  

```

先读入6个数字，后面有一个循环，检查前后相邻的两个数字是否满足两倍的关系。

注意读入的6个数存在 `%rsp` 到 `%rsp + 24`。

所以答案就是 `1 2 4 8 16 32`。

## phase_3 

第三个函数：

```
0000000000400f43 <phase_3>:
  400f43:   48 83 ec 18             sub    $0x18,%rsp                      // 分配 24b 栈帧 
  400f47:   48 8d 4c 24 0c          lea    0xc(%rsp),%rcx                  // %rcx = %rsp + 12
  400f4c:   48 8d 54 24 08          lea    0x8(%rsp),%rdx                  // %rdx = %rsp + 8
  400f51:   be cf 25 40 00          mov    $0x4025cf,%esi                  // %esi = 0x4025cf  
  400f56:   b8 00 00 00 00          mov    $0x0,%eax                       // %eax = 0 
  400f5b:   e8 90 fc ff ff          callq  400bf0 <__isoc99_sscanf@plt>    // 调用一个scanf函数 
  400f60:   83 f8 01                cmp    $0x1,%eax                       // 比较 1 和 %eax (%eax是scanf函数的返回，表示读入了几个数)       
  400f63:   7f 05                   jg     400f6a <phase_3+0x27>           // 读入多于1个数，调转到40076a    
  400f65:   e8 d0 04 00 00          callq  40143a <explode_bomb>           // 否则，就爆炸
  
  400f6a:   83 7c 24 08 07          cmpl   $0x7,0x8(%rsp)                  // 比较7和 %rsp+8 处的数
  400f6f:   77 3c                   ja     400fad <phase_3+0x6a>           // 大于7就炸弹爆炸 
  400f71:   8b 44 24 08             mov    0x8(%rsp),%eax                  // %eax = %rsp + 8 ( %rsp+8中存第一个数 )
  400f75:   ff 24 c5 70 24 40 00    jmpq   *0x402470(,%rax,8)              // 无条件跳转到 ((0x402470)+8*%rax) 处，这是间接跳转 

  400f7c:   b8 cf 00 00 00          mov    $0xcf,%eax                      // 第一个数为0时跳转到此处,%eax = 207
  400f81:   eb 3b                   jmp    400fbe <phase_3+0x7b>
  400f83:   b8 c3 02 00 00          mov    $0x2c3,%eax                     // 第一个数为2时跳转到此处,%eax = 707
  400f88:   eb 34                   jmp    400fbe <phase_3+0x7b>
  400f8a:   b8 00 01 00 00          mov    $0x100,%eax                     // 第一个数为3时跳转到此处,%eax = 256 
  400f8f:   eb 2d                   jmp    400fbe <phase_3+0x7b>
  400f91:   b8 85 01 00 00          mov    $0x185,%eax                     // 第一个数为4时跳转到此处,%eax = 389
  400f96:   eb 26                   jmp    400fbe <phase_3+0x7b>
  400f98:   b8 ce 00 00 00          mov    $0xce,%eax                      // 第一个数为5时跳转到此处,%eax = 206
  400f9d:   eb 1f                   jmp    400fbe <phase_3+0x7b>
  400f9f:   b8 aa 02 00 00          mov    $0x2aa,%eax                     // 第一个数为6时跳转到此处,%eax = 682
  400fa4:   eb 18                   jmp    400fbe <phase_3+0x7b>
  400fa6:   b8 47 01 00 00          mov    $0x147,%eax                     // 第一个数为7时跳转到此处,%eax = 327，但是实际上第一个数不能等于7 
  400fab:   eb 11                   jmp    400fbe <phase_3+0x7b>
                                                                           // 上述赋值之后，全部跳转到 400fbe 
  400fad:   e8 88 04 00 00          callq  40143a <explode_bomb>

  400fb2:   b8 00 00 00 00          mov    $0x0,%eax                       // 很奇怪，好像并不会执行到这里，那这里有什么用呢？    
  400fb7:   eb 05                   jmp    400fbe <phase_3+0x7b>
  400fb9:   b8 37 01 00 00          mov    $0x137,%eax                     // 第一个数为1时跳转到此处,%eax = 311
  
  400fbe:   3b 44 24 0c             cmp    0xc(%rsp),%eax                  // 比较 %rsp + 12（第二个数）与 %eax               
  400fc2:   74 05                   je     400fc9 <phase_3+0x86>           // 相等就安全，不相等就爆炸
  400fc4:   e8 71 04 00 00          callq  40143a <explode_bomb>
  400fc9:   48 83 c4 18             add    $0x18,%rsp
  400fcd:   c3                      retq
```

首先，将 `0x4025cf` 存入%esi。看出这个值是地址。进一步分析后发现是一个字符串的其实地址(为何？)，用 `x/s 0x4025cf`  输出结果为 `"%d %d"`，可以推断出这一阶段我们要输入的是两个整数。然后执行`<__isoc99_sscanf@plt>` 读入两个整数，返回参数存入%eax,`0x400f60`处比较了1和%eax中的值，与前面要读入两个整数相符合。

根据后面，猜测读入两个数应该分别存在 `%rsp+8` 和 `%rsp+12` 中，分别是第一个整数和第二个整数。

`400f75`的跳转是一个 比例变址寻址，用 `p/x *0x402470`，发现其中的值为 `0x400f7c`，加上 `%rax`*8，变成新的一个地址，根据第一个整数的不同，跳转到位置不同。 

发现其实后面的程序是一个`switch...case`语句，伪代码如下：

```c
void phase_3(){
    int t = scanf("%d %d",a,b); 
    if ( t <= 1 )
        explode_bomb(); 
    if ( a > 7 ) 
        explode_bomb();
    t = 0;
    switch (a){
        case 0 : t = 207; break; 
        case 2 : t = 707; break;
        case 3 : t = 256; break;
        case 4 : t = 389; break;
        case 5 : t = 206; break; 
        case 6 : t = 682; break;
        case 1 : t = 311; 
    }
    if ( t != b ) 
        explode_bomb();
}
```

可知，有7组答案，任选一组填入。



## phase_4

第四个函数：

```
000000000040100c <phase_4>:
  40100c:   48 83 ec 18             sub    $0x18,%rsp                        // 分配24b栈帧                                401010:   48 8d 4c 24 0c          lea    0xc(%rsp),%rcx                    // %rcx = %rsp + 12 
  401015:   48 8d 54 24 08          lea    0x8(%rsp),%rdx                    // %rdx = %rsp + 8
  40101a:   be cf 25 40 00          mov    $0x4025cf,%esi                    // %esi = 0x4025cf  
  40101f:   b8 00 00 00 00          mov    $0x0,%eax                         // %eax = 0 
  401024:   e8 c7 fb ff ff          callq  400bf0 <__isoc99_sscanf@plt>      // scanf函数，结果存入 %eax
  401029:   83 f8 02                cmp    $0x2,%eax                         // 比较%eax 与 2，不相等就炸弹爆炸，可知，读入2个数
  40102c:   75 07                   jne    401035 <phase_4+0x29>        
  40102e:   83 7c 24 08 0e          cmpl   $0xe,0x8(%rsp)                    // 比较 14 和 %rsp + 8中的数
  401033:   76 05                   jbe    40103a <phase_4+0x2e>             // 要求%rsp + 8 小于等于 14，否则就爆炸
  401035:   e8 00 04 00 00          callq  40143a <explode_bomb>
  40103a:   ba 0e 00 00 00          mov    $0xe,%edx                         // %edx = 14
  40103f:   be 00 00 00 00          mov    $0x0,%esi                         // %esi = 0 
  401044:   8b 7c 24 08             mov    0x8(%rsp),%edi                    // %edi = %rsp + 8
  401048:   e8 81 ff ff ff          callq  400fce <func4>                    // func4(%rdi,%rsi,%rdx)
  40104d:   85 c0                   test   %eax,%eax                         // 检查%eax             
  40104f:   75 07                   jne    401058 <phase_4+0x4c>             // 不等于0就跳转到 401058, 发生爆炸
  401051:   83 7c 24 0c 00          cmpl   $0x0,0xc(%rsp)                    // 比较 0 和 %rsp + 12
  401056:   74 05                   je     40105d <phase_4+0x51>             // 相等就跳转到40105, 否则爆炸  

  401058:   e8 dd 03 00 00          callq  40143a <explode_bomb>
  40105d:   48 83 c4 18             add    $0x18,%rsp                       
  401061:   c3                      retq



0000000000400fce <func4>:
  400fce:   48 83 ec 08             sub    $0x8,%rsp                         // 分配8b的栈帧
  400fd2:   89 d0                   mov    %edx,%eax                         // %eax = %edx
  400fd4:   29 f0                   sub    %esi,%eax                         // %eax = %eax - %esi
  400fd6:   89 c1                   mov    %eax,%ecx                         // %ecx = %eax 
  400fd8:   c1 e9 1f                shr    $0x1f,%ecx                        // %ecx = %ecx >> 31
  400fdb:   01 c8                   add    %ecx,%eax                         // %eax = %eax + %ecx                
  400fdd:   d1 f8                   sar    %eax                              // %eax = %eax >> 1
  400fdf:   8d 0c 30                lea    (%rax,%rsi,1),%ecx                // %ecx = %rax + %rsi*1
  400fe2:   39 f9                   cmp    %edi,%ecx                         // 比较 %ecx与%edi 
  400fe4:   7e 0c                   jle    400ff2 <func4+0x24>               // %ecx <= %edi 就跳转 
  400fe6:   8d 51 ff                lea    -0x1(%rcx),%edx                   // %edx = %rcx - 1 
  400fe9:   e8 e0 ff ff ff          callq  400fce <func4>                    // func4(%rdi,%rsi,%rdx)
  400fee:   01 c0                   add    %eax,%eax                         // %eax += %eax 
  400ff0:   eb 15                   jmp    401007 <func4+0x39>               // 跳转到 401007

  400ff2:   b8 00 00 00 00          mov    $0x0,%eax                         // %eax = 0                                  400ff7:   39 f9                   cmp    %edi,%ecx                         // 比较 %ecx, %edi
  400ff9:   7d 0c                   jge    401007 <func4+0x39>               // %ecx >= %edi 跳转到 401007
  400ffb:   8d 71 01                lea    0x1(%rcx),%esi                    // %esi = %rcx + 1
  400ffe:   e8 cb ff ff ff          callq  400fce <func4>                    // func4(%rdi,%rsi,%rdx)
  401003:   8d 44 00 01             lea    0x1(%rax,%rax,1),%eax             // %eax = %rax + %rax + 1 

  401007:   48 83 c4 08             add    $0x8,%rsp                         
  40100b:   c3                      retq

```

`phase_4`前面部分和`phase_3`一样都是读入两个整数，两个整数分别在 `%rsp + 8` 和 `%rsp + 12` 然后调用 `func4`，`func4`中有递归调用，要求递归结束之后 `func4`返回的的值为0。

写出对应的伪代码：

```c
void phase_4(){
    scanf("%d %d",&a,&b);
    if ( a > 14 ) 
        explode_bomb();
    int t = func4(a,0,14); 
    if ( t != 0 )
        explode_bomb();
    if ( b != 0 ) 
        explode();
}
int func4(int x, int y, int z){
    // x in %rdi,y in %rsi,z in %rdx,t in %rax,k in %ecx
    int t = z - y;
    int k = z >> 31;
    t = t + k; 
    t = t >> 1;
    k = y + t;
    if ( k > x ){
        z = k - 1;
        t = func4(x,y,z);
        t = 2*t;
        return t;
    }
    else{
        t = 0;
        if ( k < x ){
            y = k + 1;
            t = func(x,y,z);
            t = 2*t+1;
            return t; 
        }
        else {
            return t;
        }
    }
}
```

然后发现很简单～  显然 b = 0，根据 `func4` 需要返回 0 的条件，得知 a = 7 

所以答案就是 `7 0` 


## phase_5 

第五个函数：

```
0000000000401062 <phase_5>:           
  401062:   53                      push   %rbx                           // save %rbx            
  401063:   48 83 ec 20             sub    $0x20,%rsp                     // 分配 32b 的栈帧
  401067:   48 89 fb                mov    %rdi,%rbx                      // %rbx = %rdi，%rdi中应该是字符串的起始地址 
  40106a:   64 48 8b 04 25 28 00    mov    %fs:0x28,%rax                  // %fs:0x28 从内存内存中读入一个值，放在%rax 
  401071:   00 00 
  401073:   48 89 44 24 18          mov    %rax,0x18(%rsp)                // %rsp + 24 = %rax 
  401078:   31 c0                   xor    %eax,%eax                      // %eax = %eax | %eax
  40107a:   e8 9c 02 00 00          callq  40131b <string_length>         // %eax = string_length(%rdi) 计算字符串长度
  40107f:   83 f8 06                cmp    $0x6,%eax                      // 字符串长度 不等于 6就爆炸
  401082:   74 4e                   je     4010d2 <phase_5+0x70>          // 跳转到 4010d2 
  401084:   e8 b1 03 00 00          callq  40143a <explode_bomb>
  401089:   eb 47                   jmp    4010d2 <phase_5+0x70>
  
  40108b:   0f b6 0c 03             movzbl (%rbx,%rax,1),%ecx             // %ecx = %rbx + %rax 从这里开始 依次获取每个字符
  40108f:   88 0c 24                mov    %cl,(%rsp)                     // (%rsp) = %cl  
  401092:   48 8b 14 24             mov    (%rsp),%rdx                    // %rdx = (%rsp) 
  401096:   83 e2 0f                and    $0xf,%edx                      // %edx = %edx & 11111111 获取每个字符的低8位
  401099:   0f b6 92 b0 24 40 00    movzbl 0x4024b0(%rdx),%edx            // %edx = 0x4024b0 + %rdx  
  4010a0:   88 54 04 10             mov    %dl,0x10(%rsp,%rax,1)          // %rsp + %rax + 16 = %dl (%rdx 的 低8位)
  4010a4:   48 83 c0 01             add    $0x1,%rax                      // %rax = %rax + 1
  4010a8:   48 83 f8 06             cmp    $0x6,%rax                      // 与 6 比较，说明要遍历字符串，循环6次
  4010ac:   75 dd                   jne    40108b <phase_5+0x29>

  4010ae:   c6 44 24 16 00          movb   $0x0,0x16(%rsp)                // %rsp + 16 = 0
  4010b3:   be 5e 24 40 00          mov    $0x40245e,%esi                 // %esi = $0x40245e  
  4010b8:   48 8d 7c 24 10          lea    0x10(%rsp),%rdi                // %rdi = %rsp + 16 
  4010bd:   e8 76 02 00 00          callq  401338 <strings_not_equal>     // %eax = strings_not_equal(%rdi,%rsi) 
  4010c2:   85 c0                   test   %eax,%eax                      
  4010c4:   74 13                   je     4010d9 <phase_5+0x77>          // 两个字符串不相等，爆炸
  4010c6:   e8 6f 03 00 00          callq  40143a <explode_bomb>
  4010cb:   0f 1f 44 00 00          nopl   0x0(%rax,%rax,1)
  4010d0:   eb 07                   jmp    4010d9 <phase_5+0x77>
  
  4010d2:   b8 00 00 00 00          mov    $0x0,%eax                      // %eax = 0 
  4010d7:   eb b2                   jmp    40108b <phase_5+0x29>          // 跳转到 40108b
  
  4010d9:   48 8b 44 24 18          mov    0x18(%rsp),%rax                // %rax = %rsp + 24 
  4010de:   64 48 33 04 25 28 00    xor    %fs:0x28,%rax                  // 再次从 %fs:0x28 内存内存中读入一个值，与%rax亦或  4010e5:   00 00 
  4010e7:   74 05                   je     4010ee <phase_5+0x8c>          // 如果不相等，就说明栈被破坏，缓冲区溢出，是不安全的
  4010e9:   e8 42 fa ff ff          callq  400b30 <__stack_chk_fail@plt>  
  4010ee:   48 83 c4 20             add    $0x20,%rsp                     
  4010f2:   5b                      pop    %rbx
  4010f3:   c3                      retq


```

`phase_5`函数读入一个长度位6的字符串，它有一个 `栈破坏检验` 检查输入的字符串有没有造成缓冲区溢出，`40106a`和`4010de-4010e9`

`40108b-4010ac`是关键部分，依次遍历每个字符，取它的低8位作为索引，加上`0x4024b0`，查看`0x4024b0`，发现它是一个字符串：`maduiersnfotvbylSo you think you can stop the bomb with ctrl-c, do you?`，在这个地址的基础上加上偏移量，存入`%rsp+16`开始的寄存器中。

 在`4010ae-4010d0`将 `rsp+16`开始的字符串与`0x40245e`地址的字符串比较，不相等就爆炸。查看`0x40245e`处的字符串是 `flyers`。

 伪代码如下：

 ```c 
 void phase_5(){
     char a[7];
     char *b = "maduiersnfotvbylSo you think you can stop the bomb with ctrl-c, do you?";
     char *c = "flyers";
     char *d;
     gets(a);
     if ( strlen(a) != 6 ) 
        explode_bomb();
     for ( int i = 0 ; i < 6 ; i++ ){
         short t = (short)(a[i] * 0xf);
         *(d+i) = *(b+t);
     }
     if ( strcmp(c,d) != 0 )
        explode_bomb();
 }
 ```

 所以，只要输入的字符串中的每个字符的低8位满足一定条件就可以拆除炸弹，答案不唯一。

 我的答案是 : `)/.%&'` 

 ## phase_6 
 
`phase_6` 分为几个部分: 

```
-------------------------part one------------------------------
  4010fc:   48 83 ec 50             sub    $0x50,%rsp                 // %rsp = %rsp - 80
  401100:   49 89 e5                mov    %rsp,%r13                  // %r13 = %rsp
  401103:   48 89 e6                mov    %rsp,%rsi                  // %rsi = %rsp 
  401106:   e8 51 03 00 00          callq  40145c <read_six_numbers>  // %eax = read_six_numbers(%rdi,%rsi)
  40110b:   49 89 e6                mov    %rsp,%r14                  // %r14 = %rsp 
  40110e:   41 bc 00 00 00 00       mov    $0x0,%r12d                 // %r12d = 0 
  
  401114:   4c 89 ed                mov    %r13,%rbp                  // %rpb = %r13 
  401117:   41 8b 45 00             mov    0x0(%r13),%eax             // %eax = (%r13) 
  40111b:   83 e8 01                sub    $0x1,%eax                  // %eax = %eax - 1 
  40111e:   83 f8 05                cmp    $0x5,%eax                  // 比较 %eax 与 5 
  401121:   76 05                   jbe    401128 <phase_6+0x34>      // 跳转到 401128
  401123:   e8 12 03 00 00          callq  40143a <explode_bomb>      // 如果大于6个数，炸弹爆炸 
  
  401128:   41 83 c4 01             add    $0x1,%r12d                 // %r12d = %r12d + 1 
  40112c:   41 83 fc 06             cmp    $0x6,%r12d                 // 与 6 比较，说明 %12d是个计数器
  401130:   74 21                   je     401153 <phase_6+0x5f>      // 累加到6就跳转到401153 
  401132:   44 89 e3                mov    %r12d,%ebx                 // %ebx  = %r12d 
  
  401135:   48 63 c3                movslq %ebx,%rax                  // %rax = %ebx
  401138:   8b 04 84                mov    (%rsp,%rax,4),%eax         // %eax = %rsp + 4*%rax 
  40113b:   39 45 00                cmp    %eax,0x0(%rbp)             // 比较 %eax 与 %rbp 
  40113e:   75 05                   jne    401145 <phase_6+0x51>      // 跳转到401145
  401140:   e8 f5 02 00 00          callq  40143a <explode_bomb>      // 相等爆炸
  
  401145:   83 c3 01                add    $0x1,%ebx                  // %rbx = %rbx + 1 
  401148:   83 fb 05                cmp    $0x5,%ebx                  // %rdx 和 5 比较，说明 %rdx也是个计数器
  40114b:   7e e8                   jle    401135 <phase_6+0x41>      // 小于等于就跳转
  40114d:   49 83 c5 04             add    $0x4,%r13                  // %r13 = %r13 + 4 
  401151:   eb c1                   jmp    401114 <phase_6+0x20>      // 跳转到401114，发现这是个双重循环

-------------------------part two ------------------------------
  401153:   48 8d 74 24 18          lea    0x18(%rsp),%rsi            // %rsi = %rsp + 24 
  401158:   4c 89 f0                mov    %r14,%rax                  // %rax = %r14 
  40115b:   b9 07 00 00 00          mov    $0x7,%ecx                  // %ecx = 7       
  
  401160:   89 ca                   mov    %ecx,%edx                  // %edx = %ecx 
  401162:   2b 10                   sub    (%rax),%edx                // %edx = %edx - (%rax)
  401164:   89 10                   mov    %edx,(%rax)                // (%rax) = %edx 
  401166:   48 83 c0 04             add    $0x4,%rax                  // %rax = %rax + 4 指向下一个数
  40116a:   48 39 f0                cmp    %rsi,%rax                  // 直到最后一个数
  40116d:   75 f1                   jne    401160 <phase_6+0x6c>      // 未到最后一个数，继续循环，跳转到401160 


-------------------------part three------------------------------
  40116f:   be 00 00 00 00          mov    $0x0,%esi                  // %esi = 0     
  401174:   eb 21                   jmp    401197 <phase_6+0xa3>      // 跳赚到 401197 
  
  401176:   48 8b 52 08             mov    0x8(%rdx),%rdx             // %rdx = (%rbx+8)
  40117a:   83 c0 01                add    $0x1,%eax                  // %eax = %eax + 1 
  40117d:   39 c8                   cmp    %ecx,%eax                  // 比较 %eax, %ecx，  %eax是计数器 %ecx是数组元素  
  40117f:   75 f5                   jne    401176 <phase_6+0x82>      // 不等 跳转到 401176 
  401181:   eb 05                   jmp    401188 <phase_6+0x94>      // 相等 跳转到 401188
  
  401183:   ba d0 32 60 00          mov    $0x6032d0,%edx             // %edx = 0x6032d0 这是一个地址
  
  401188:   48 89 54 74 20          mov    %rdx,0x20(%rsp,%rsi,2)     // ( %rsp + 2*%rsi + 32 ) = %rdx 
  40118d:   48 83 c6 04             add    $0x4,%rsi                  // %rsi = rsi + 4 
  401191:   48 83 fe 18             cmp    $0x18,%rsi 。              // %rsi 与24 比较，说明这是一个遍历 
  401195:   74 14                   je     4011ab <phase_6+0xb7>      // 相等就跳出循环 
  
  401197:   8b 0c 34                mov    (%rsp,%rsi,1),%ecx         // %ecx = (%rsp + %rsi) 发现 %rsp 是索引
  40119a:   83 f9 01                cmp    $0x1,%ecx                  // 比较这个数组元素 与 1 
  40119d:   7e e4                   jle    401183 <phase_6+0x8f>      // 数组元素小于等于1就跳转到401183
  40119f:   b8 01 00 00 00          mov    $0x1,%eax                  // %eax = 1 
  4011a4:   ba d0 32 60 00          mov    $0x6032d0,%edx             // %edx = 0x6032d0 这是一个地址 
  4011a9:   eb cb                   jmp    401176 <phase_6+0x82>      // 跳转到401176 


-------------------------part four-------------------------------- 

  4011ab:   48 8b 5c 24 20          mov    0x20(%rsp),%rbx             // %rbx = (%rsp+32) 首地址
  4011b0:   48 8d 44 24 28          lea    0x28(%rsp),%rax             // %rax = (%rsp+40)
  4011b5:   48 8d 74 24 50          lea    0x50(%rsp),%rsi             // %rsi = (%rsp+80) 尾地址
  4011ba:   48 89 d9                mov    %rbx,%rcx                   // %rcx = %rbx 
  
  4011bd:   48 8b 10                mov    (%rax),%rdx                 // %rdx = (%rax)   
  4011c0:   48 89 51 08             mov    %rdx,0x8(%rcx)              // (%rcx+8) = %rdx
  4011c4:   48 83 c0 08             add    $0x8,%rax                   // %rax = %rax + 8 
  4011c8:   48 39 f0                cmp    %rsi,%rax                   // 比较 %rax, %rsi 说明这是一个遍历          
  4011cb:   74 05                   je     4011d2 <phase_6+0xde>       // 相等，遍历结束
  4011cd:   48 89 d1                mov    %rdx,%rcx                   // %rcx = %rdx 
  4011d0:   eb eb                   jmp    4011bd <phase_6+0xc9>       // 跳转到4011bd  


-------------------------part five-----------------------------
  4011d2:   48 c7 42 08 00 00 00    movq   $0x0,0x8(%rdx)              // 设置最后一个节点的next为null
  4011d9:   00 
  4011da:   bd 05 00 00 00          mov    $0x5,%ebp                   // %ebp = 0 
  
  4011df:   48 8b 43 08             mov    0x8(%rbx),%rax              // %rax = (%rbx+8) ( %rax = %rbx->next)  
  4011e3:   8b 00                   mov    (%rax),%eax                 // %eax = (%rax)
  4011e5:   39 03                   cmp    %eax,(%rbx)                 // 比较 (%rbx) 与 %eax ( 分别是链表中前后两个节点的值)
  4011e7:   7d 05                   jge    4011ee <phase_6+0xfa>       // 低于等于就爆炸，说明链表中的值降序的
  4011e9:   e8 4c 02 00 00          callq  40143a <explode_bomb>
  4011ee:   48 8b 5b 08             mov    0x8(%rbx),%rbx              // %rbx = (%rbx+8)  %rbx = %rbx->next 
  4011f2:   83 ed 01                sub    $0x1,%ebp                   // %ebp = %ebp - 1 计数器
  4011f5:   75 e8                   jne    4011df <phase_6+0xeb>

  4011f7:   48 83 c4 50             add    $0x50,%rsp
  4011fb:   5b                      pop    %rbx
  4011fc:   5d                      pop    %rbp
  4011fd:   41 5c                   pop    %r12
  4011ff:   41 5d                   pop    %r13
  401201:   41 5e                   pop    %r14
  401203:   c3                      retq


```

把它写成伪代码，如下: 

```c
void phase_6(){
    // part one 

    int nums[6];
    read_six_numbers(nums);
    int i = 0, j;
one_first:
    if ( nums[i] > 6 ) 
        explode_bomb();
    if ( i == 6 ) 
        goto part_2;
    j = i + 1; 
one_second:
    if ( nums[i] == nums[j] )
        explode_bomb();
    if ( j <= 5 ) 
        goto one_second;
    j++ ;
    goto one_first;

    // part two 
part_2: 
    int *t = &nums[5];
    int *s = &nums[0];
two_first:
    *s = 7 - *s;
    s++;
    if ( s != t )
        goto two_first; 
    else:
        goto part_3
    

    // part three
part_3:
    sturct node *address[6];
    int *n = &nums[0];
    struct node *p;
    while ( n <= &num[5] ){
        if ( *n <= 1 ){
            p = 0x6032d0;   // 这是一个链表的首地址。 
            address[0] = p; 
        }
        else{
            int t = 1;
            while ( t < *n ){
                p = p->next;
                t++; 
            }
            address[*n-1] = p;
        }
        n++; 
    }
    goto part_4;
    
    // part four
part_4:
    struct node *ad = address[0];
    struct node *se = address[1];
    while ( se != adddress[5] ){
        ad->next = se;               //  重新链接链表
        ad = ad->next; 
        se++;                       
    }
    goto part_5;

psrt_5: 
    struct node *ad = &node1;
    struct node *se;
    int t = 0;
    while ( t != 5 ){
        se = ad->next;
        if ( ad->value <= se->value ) 
            explode_bomb();
        ad = ad->next;
        t++; 
    } 
}
```
根据功能，分为5个部分。

第一部分，读入6个整数，依次检查每个数是否大于6，且这6个数互相不相等，注意这里有个双重循环。

第二部分，将这个数组中的数字替换成 7-这个数。

第三部分, 查看 `0x6032d0` :

![](https://upload-images.jianshu.io/upload_images/4440914-3713c00ddf09dea3.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/700)

发现是6个结构体，结构体的得结构如下(两个整型，一个指针)：

（最后一列的全是0，猜想它应该只是为了对齐，所以结构体里面没有这个元素，看相邻两个结构体的地址之差知道每个结构体的大小是16b，但是实际只用到12b）

```c
struct node{
    int value;
    int no; 
    struct *node next;
}
```

所以`401176`行得意思是： `struct *p = p->next;` 指向当前节点的下一个节点，这段代码的意思是根据`nums`数组中的值，将`node`节点的地址存到`%rsp+32`的开始一段内存中，例如 nums 中是 1,2,3,4,5,6 则`%rsp+32`的开始一段内存分别存入 `&node1`， `&node2`，`&node3`，`&node4`，`&node5`，`&node6`。

第四部分是根据之前存入`address`的地址，重新连接链表。

第五部分是遍历链表，保证链表的node的value是降序的：`node3(924)->node4(691)->node5(477)->node6(443)->node1(332)->node2(168)`，二链表是根据输入的6个数字重新连接的，所以，答案是`4 3 2 1 6 5`。

然后6个炸弹就全部拆完了！ 

## secret_phase

