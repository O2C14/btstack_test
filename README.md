# btstack_a2dp_player
目前还是个半成品

在`gpio_config.h`中配置引脚, 目前仅支持ES9038Q2M

存在的问题:
---
1. 目前使用的缓冲区大小最小为100ms, 但这样也有可能失去同步.

如何配置DMA:
---
`pcm_open`中的`base_buffer_size`决定了DMA中断的频率, 也决定了检查是否同步的频率

`pcm_open`中的`buffer_num`和`base_buffer_size`相乘的结果就是缓冲区的总大小, 单位是字节

`set_start_loc`这个函数决定了过载缓冲区和欠载缓冲区的大小, 参数为0代表欠载缓冲区大小为0, 即不能接受欠载, 也就是说一旦发生欠载就会直接重置DMA. 过载缓冲区与欠载缓冲区之和就是最终缓冲区的大小

配置过程:
---
1. 获取解码器每次解码的输出长度, 必须是定值(单位是samples), 也就是`a2dp_decoder.h`中的`nframes_per_buffer`
2. 计算输出长度对应的时长(单位是ms), 乘以一个整数使其大于10ms(检查间隔), 即`init_decoder`中的`base_size`
3. 计算最终缓冲区的大小, 注意不要大于`pcm_buffer`
4. 计算`set_start_loc`的参数, 推荐大小为`nframes_per_buffer`的整数倍, 并使过载缓冲区和欠载缓冲区的大小一样.

I2S移植:
---
在`es9038q2m.h`已经介绍了.

可能会遇到的问题:
---
I2S没有输出, GPIO_PIN_12 拉高(在Ai-M61-32S-kit上表现为亮红灯). 这代表I2S已经崩溃, 并且无法通过复位I2S和DMA恢复, 应该检查MCU的供电是否稳定.

目前已经支持的编码:
---
LHDCV5(未公开), LDAC, AAC, SBC