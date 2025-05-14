Checkpoint 1 Writeup
====================

My name: [your name here]

My SUNet ID: [your sunetid here]

I collaborated with: [list sunetids here]

I would like to thank/reward these classmates for their help: [list sunetids here]

This lab took me about [n] hours to do. I [did/did not] attend the lab session.

I was surprised by or edified to learn that: [describe]

Report from the hands-on component of the lab checkpoint: [include
information from 2.1(4), and report on your experience in 2.2]

Describe Reassembler structure and design. [Describe data structures and
approach taken. Describe alternative designs considered or tested.
Describe benefits and weaknesses of your design compared with
alternatives -- perhaps in terms of simplicity/complexity, risk of
bugs, asymptotic performance, empirical performance, required
implementation time and difficulty, and other factors. Include any
measurements if applicable.]
使用map来存储数据，key为first_index，value为对应的string
使用uint64_t来存储缓存的数据量
使用optional<uint64_t>来存储末尾数据的index，用以控制是否关闭，首次收到结束标记时设置，当数据无法全部缓存，认为非尾包
结束标记只能设置一次
空数据包也能携带有效结束位置
数据不完整时自动清除结束标记
try_close()实现关闭检测与数据插入操作解耦

Implementation Challenges:
[]

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I'm not sure about: [describe]
