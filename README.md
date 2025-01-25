# btstack_a2dp_player
目前还是个半成品

在`gpio_config.h`中配置引脚,目前仅支持ES9038Q2M

存在的问题:
1. 音频流在i2s启动后可能不同步.已经在dma0_transfer_done检测欠载.或者可以尝试直接写入i2s,但是这样可能会引发过载.

2. i2s运行一段时间后会直接无声,现象为`0x2000c020`处的寄存器会一直收到中断请求,但是`0x2000ab88`处的数据没有任何改变.i2s_do引脚的LED灯也没有变化,执行`i2s clear` `i2s reset` `i2s start`后依然无声.
