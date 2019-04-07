#Distributed system lab3
#### 515015910005 丁丁

## 参数设置

### srTCM参数设置
| flow id | cir | cbs | ebs |
| ------ | ------ | ------ | ------ |
| 0 | 2000000000 | 80000 | 80000 |
| 1 | 2000000000 | 40000 | 40000 |
| 2 | 2000000000 | 20000 | 20000 |
| 3 | 2000000000 | 10000 | 10000 |


### WRED参数设置
| color | wq_log2 | min_th | max_th | maxp_inv |
| ------ | ------ | ------ | ------ | ----- |
| GREEN | 9 | 800 | 801 | 10 |
| YELLOW | 9 | 800 | 801 | 10 |
| RED | 9 | 0 | 1 | 10 |

lab要求flow0的bandwidth在1.28Gbps左右，即每秒约发包1.28 * 1024^3 / 8个byte。在测试程序中， 每隔1ms发包，由此每次发包的大小大约为170000byte。

通过设置cbs和ebs来控制发包速率，即设置cbs+ebs为17000左右即可满足要求。在本次设计中采取cbs与ebs各取80000（便于后面计算比例）。

同时，四个流的带宽比需要满足8:4:2:1，通过按比例缩放cbs与ebs即可实现。

CIR的设置首先要满足flow0的bandwidth要求，实际测试设为170000000左右似乎并不够，最后调大均设为2000000000。

为了GREEN与YELLOW包尽量通过，将min_th都设为800。RED包进行丢弃，将min_th设为0。

### APIs
| API | 功能 |
| ------ | ------ |
| rte_meter_srtcm_config() | 根据传入的参数初始化srTCM流 |
| rte_meter_srtcm_color_blind_check() | 为package标记颜色 |
| rte_red_config_init() | 通过给定的值初始化RED算法的参数 |
| rte_red_rt_data_init() | 初始化RED数据 |
| rte_red_mark_queue_empty() | 清空queue |
| rte_red_enqueue() | 决定是向queue内加入一个包还是丢弃一个包 |