# ICS2023 Programming Assignment
refer to https://nju-projectn.github.io/ics-pa-gitbook/ics2023/

打算好好学一下操作系统，找到了jyy的OS课，但该课有一个前置课ICS，PA即其实验课。
（虽然jyy声称要将OS课做成独立课程）。

据说ICS在NJU是低年级课，但不懂汇编和C的话仍相当困难，所以做到哪儿算哪儿。
看Git History应该能反映进度（已经将自动Commit删掉了）。

基于Risc-V 32bit


## PA1
总体还算不难
发现不少写PA1解答的到PA2就没了...

## PA2
2.1 
在框架代码上填充指令，对着手册一条一条来就好。
2.2 
iringbuf和mtrace还好，ftrace实在是困难，暂时放弃了。
可参考的网友解答：
https://blog.csdn.net/qq_57973134/article/details/134999765
https://www.cnblogs.com/nosae/p/17066439.html
https://vgalaxy.work/posts/icspa2/

假设app没问题，先在native上调klib，再在klib基础上调nemu
