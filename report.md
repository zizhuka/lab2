# ICS 2026 Fall BombLab 实验报告

学号：25800180144  姓名：刘丰玮

## 一、实验目的

1. 深入理解 x86-64 汇编语言：寄存器、栈帧、函数调用约定、指令集。
2. 掌握静态逆向工具 objdump 与动态调试工具 gdb 的使用方法。
3. 理解高级语言结构（switch 跳转表、循环、链表、递归）在汇编层面的实现。
4. 学会从汇编代码中恢复计算逻辑，并据此构造合法输入（"拆弹"）。
5. 综合运用位运算、递推、二分查找等知识解决实际问题。

## 二、实验环境

| 项目 | 内容 |
| --- | --- |
| 操作系统 | Linux x86-64 |
| 编译器/构建 | gcc / make |
| 分析工具 | objdump、gdb |
| 测试框架 | public_test.py（Python 3） |
| 个人 bomb package | 2b5a6d1dc802102f1e1a5d01 |
| 实验仓库 | https://github.com/zizhuka/lab2 |

## 三、实验思路与方法

整体流程：**反汇编 → 识别输入格式 → 恢复计算关系 → 编写求解函数 → 多 seed 验证**。

1. 对目标函数 `objdump -d --no-show-raw-insn --disassemble=phase_N ./bomb` 反汇编，先看入口校验（`valid_u32_fields`、`sscanf` 格式串）确定口令的个数与格式。
2. 追踪 challenge 的每一位分解（`>>16`、`>>32`、`& 0x3F` 等），把汇编逐步还原为 C 表达式。
3. 对 switch 结构用 `objdump -s` 读取跳转表内容、用 gdb 的 `x/i` 确认每个索引对应的分支，避免映射错位。
4. 对递归/循环结构在 gdb 中设置断点，观察寄存器与栈的变化，确认循环次数与回溯方式。
5. 将恢复的公式实现为 `solve_phase_N`，用公开测试与额外 seed（seed 43）验证；全部通过后提交。

## 四、逐关分析与验证

### Phase 1（1 个数）

| 项目 | 内容 |
| --- | --- |
| 输入格式 | 1 个 uint32 |
| 关键汇编 | `p1_step0: eax=rdi+rdi*4; esi+=0x55b8; eax^=esi`；`p1_step1: edi-=esi; eax=(rdi+rdi*4)+0xf9a6`；`p1_step2: eax=rsi+0xd953; eax^=edi` |
| 推理过程 | challenge 被拆成 lo（低32位）、hi（高32位）、mid（16~47位）三部分，分别送入三个子函数组合计算 |
| 计算关系 | b=(lo*5)^(hi+0x55b8)；a=(hi-mid)*5+0xf9a6；口令=(a+0xd953)^b（全部 32 位回绕） |
| 验证 | seed 42、seed 43 均通过；Public tests 10/10 |

### Phase 2（6 个数）

| 项目 | 内容 |
| --- | --- |
| 输入格式 | 6 个 uint32 |
| 关键汇编 | 入口直接计算 v[0]，随后进入循环按索引递推 v[1..5] |
| 推理过程 | 每个后继项只依赖前一项与 challenge 中的一段位字段，是标准的递推关系 |
| 计算关系 | v0=3*((challenge>>35)&0xFF)；v[i]=5*(v[i-1]+i)-((challenge>>(15+2*(i-1)))&0x3F)，i=1..5（移位 15,17,19,21,23） |
| 验证 | seed 42、seed 43 均通过；Public tests 10/10 |

### Phase 3（2 个数）

| 项目 | 内容 |
| --- | --- |
| 输入格式 | 2 个 uint32 |
| 关键汇编 | `p3_dispatch` 用 `jmp *0x405820(,%rax,8)` 跳转表分发，v0=challenge&7 作为索引 |
| 推理过程 | 先用 objdump -s 读出跳转表 8 个目标地址，再用 gdb `x/i` 逐一确认索引→分支的映射，避免 switch 映射错位 |
| 计算关系 | x=(challenge>>3)^(challenge>>32)；索引 0~7 对应：0→(4x)^0xc0df，1→5x+0xf797，2→5x+0x25896，3→(x^0xa654)+(x>>2)，4→5x+0x36ed，5→(x^0xd1a0)+(x>>1)，6→(4x)^0xe128，7→5x+0x3af2 |
| 验证 | 初版映射错 8/10；修正后 seed 42、seed 43 通过，Public tests 10/10 |

### Phase 4（1 个数）

| 项目 | 内容 |
| --- | --- |
| 输入格式 | 1 个 uint32 |
| 关键汇编 | 在 0..127 上二分查找 target，用数组保存路径中间值与方向，退出后回溯合并 |
| 推理过程 | 二分过程被显式记录：向左（target<mid）记 tag=0x1a，向右记 tag=0x1f；找到 target 后从栈底回溯逐层合并 |
| 计算关系 | target=challenge&0x7f；salt=(challenge>>7)^(challenge>>32)；命中时 result=(mid+0x1a)^salt；回溯 result=(result*5)^(mid+salt) 再 ^tag |
| 验证 | seed 42、seed 43 均通过；Public tests 10/10 |

### Phase 5（2 个数）

| 项目 | 内容 |
| --- | --- |
| 输入格式 | 2 个 uint32 |
| 关键汇编 | 12 节点链表（node_value/node_tag/next_node 三张静态表）+ 8 个根节点，按 challenge 选起点和步数遍历 |
| 推理过程 | 起点 roots[challenge&7]，步数 (challenge>>3)&3+1；每步累加节点值再乘 5 异或 tag，同时保存最后一步的 tag |
| 计算关系 | x=(challenge>>6)^(challenge>>32)；acc 初值 x^0x57b；每步 acc=((acc+node_value)*5)^node_tag；输出 acc 与最后一步 tag |
| 验证 | seed 42、seed 43 均通过；Public tests 10/10 |

### Phase 6（1 个数）

| 项目 | 内容 |
| --- | --- |
| 输入格式 | 1 个 uint32 |
| 关键汇编 | 连续两次 8 分支 switch 变换（同一组 8 个变换函数），中间插入常量异或，结尾加常量 |
| 推理过程 | 第一轮按 challenge&7 选变换，异或 0xcf99；第二轮按 (challenge>>3)&7 再选变换，加 0x92f9 |
| 计算关系 | x=(challenge>>6)^(challenge>>32)；value=变换1(x)；value^=0xcf99；value=变换2(value)；value+=0x92f9。8 个变换：f0=(v+0x1f14)^(v>>2)；f1=(v>>2)|(v<<30)；f2=((v<<7)|(v>>25))^0xa4a2；f3=(v^0x844f)+(v<<7)；f4=f0；f5=((v>>5)^v)+0x3dd4；f6=f0；f7=(v*8+0x1af9)^(v*5) |
| 验证 | seed 42、seed 43 均通过；Public tests 10/10 |

### Honor（secret phase，1 行口令）

| 项目 | 内容 |
| --- | --- |
| 口令格式 | 32 个任意字符 + "OPEN" + " n0 n1 n2"（总长 36~163 字节） |
| 关键汇编 | secret_gate 检查第 33~36 字节为 "OPEN" 后调用 secret_phase；secret_phase 用 sscanf 解析 3 个整数并与 challenge/context 校验 |
| 推理过程 | 先在 gdb 中于 secret_gate 断点对照寄存器，确认 secret_gate 的第 2、3 参数依次为 challenge、context；再按 secret_mix 的 ROL/add/xor 序列恢复公式 |
| 计算关系 | c=context&3；A=(challenge>>32)^challenge；B=(context>>32)^context；m=ROL32(A^B,5)+0x8410^0xfbf3；n0=c；n1=m^((c*0x8410)^((context>>16)&0xFFFFFFFF))；n2=n1^(B+c+0xfbf3+(A>>5)) |
| 验证 | Secret public tests 10/10；seed 43 通过 |

## 五、遇到的问题与解决

| 问题 | 现象 | 解决方法 |
| --- | --- | --- |
| Phase 3 跳转表映射错误 | Public tests 8/10（seed 2026、2^64-1 失败） | 用 `objdump -s` 读跳转表字节 + gdb `x/i 0x402278` 逐项确认索引→分支映射，修正 case 顺序 |
| Honor 全部 wrong_answer | Secret public tests 0/10 | 在 solve_secret 中打印 challenge/context，并在 gdb 中于 secret_gate 断点查看 $rsi/$rdx，发现第 2、3 参数为 challenge/context（与最初假设相反），将公式中 challenge/context 互换后修正 |
| 终端无法连接 GitHub | git push 报 443 连接失败 | 改用浏览器网页上传文件，并通过 Releases 创建 final 标签 |

## 六、实验结论

1. 本实验全部完成：六关 + Honor 全部通过，`make test` 为 Public tests 60/60、Secret public tests 10/10，GitHub Actions 运行通过；额外使用 seed 43 完整通关验证。
2. 通过本次实验，掌握了从汇编恢复高层语义的方法：识别位分解、跳转表、递推与回溯结构；体会到 objdump 静态分析与 gdb 动态调试结合的重要性。
3. 认识到"逐位核对"的价值——第 3 关的跳转表映射与 Honor 的参数顺序问题，都只有通过工具实测才能定位。

## 七、参考资料与 AI 工具使用说明

- 实验说明：https://ics-26fall-fdu.github.io/labs/lab2-bomb-lab/
- 使用工具：objdump（静态反汇编）、gdb（动态调试）、make/public_test.py（验证）。
- AI 工具：使用豆包协助汇编解读、公式整理与错误排查；所有题解代码与报告内容均经本人理解确认后提交。

## 八、运行结果

| 测试项 | 命令 | 结果 |
| --- | --- | --- |
| 公开测试 | `make test` | Public tests 60/60、Secret public tests 10/10，全部通过 |
| 额外 seed 验证 | `./defuser --seed 43 \| ./bomb --seed 43` | Phase 1~6 与 Secret 全部 defused，输出 Congratulations |
| 在线验证 | GitHub Actions（BombLab public tests） | 运行通过（绿色 ✓） |
