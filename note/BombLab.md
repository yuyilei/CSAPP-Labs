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
  400ee0:   48 83 ec 08             sub    $0x8,%rsp       // 开辟的8b的栈帧             
  400ee4:   be 00 24 40 00          mov    $0x402400,%esi  // %esi = $0x40240 
  400ee9:   e8 4a 04 00 00          callq  401338 <strings_not_equal> // 调用 strings_not_equal(%rdi,%rsi)
  400eee:   85 c0                   test   %eax,%eax       // 检查strings_not_equal的函数的返回值
  400ef0:   74 05                   je     400ef7 <phase_1+0x17>  // 为0就跳转
  400ef2:   e8 43 05 00 00          callq  40143a <explode_bomb>  // 引爆炸弹 
  400ef7:   48 83 c4 08             add    $0x8,%rsp              // 回收栈帧 
  400efb:   c3                      retq                          // 函数返回 
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
  40106a:   64 48 8b 04 25 28 00    mov    %fs:0x28,%rax
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
  4010de:   64 48 33 04 25 28 00    xor    %fs:0x28,%rax                  
  4010e5:   00 00 
  4010e7:   74 05                   je     4010ee <phase_5+0x8c>
  4010e9:   e8 42 fa ff ff          callq  400b30 <__stack_chk_fail@plt>
  4010ee:   48 83 c4 20             add    $0x20,%rsp                     
  4010f2:   5b                      pop    %rbx
  4010f3:   c3                      retq


```
