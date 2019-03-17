# Distributed system lab 1
#### 515015910005 丁丁 dingd2015@sjtu.edu.cn

## packet组织
packet主要分为两类：
- 由sender发送的带message内容的packet
- 由receiver发送的ack

sender发送的packet结构组织如下：
| 1 byte | 2 byte | 4 byte | 1 byte | rest |
| ------ | ------ | ------ | ------ | ------ |
| position | checksum | sequence number | payload size | payload |
其中， sequence number唯一标记了这个packet的顺序，用于处理乱序接收的情况；checksum计算了这个包的hash值，用于处理在传输过程中出现error的情况。payload size指定了payload的大小。

由于1个message的大小可能会超过一个packet所能容纳的大小，而被分为多个packet发送。为了receiver能够成功拼接出原来完整的message，在packet的结构中加入了position位，当position为1时，该包为某个message的开头。当position为2时，该包为某个message的结尾，当position为3时，该包既是某个message的开头，也是这个message的结尾。

receiver发送的packet结构组织如下：
| 2 byte | 4 byte |
| ------ | ------ |
| checksum | sequence number |
其中，sequence number标记了当前receiver收到了序列号<=sequence number的包。如，receiver发送了sequence number为5的ack，则表明receiver已经成功接收到了所有sequence number<=5的packet。checksum同样为了处理传输过程中可能出现的error。

## 协议
### 协议总览
#### 主要策略
主要采用GO-BACK-N策略，根据测试结果，window size设为20，timeout时间设为0.3，测试结果最佳。

#### 优化
两个buffer：发送端的send buffer用于存放由于upper level传输消息过快，来未来得及发送出去的包；接收端的receive buffer用于存放由于网络乱序，不连续的超前到达的包。

#### 协议流程
sender:

发送流程:
- 从upper layer接收message，对message做包装处理。
- 如果这个包处于可发送状态（可以放入windows中），则直接发送；若这个包当前处于不可发送状态（windows已满），则暂时将其放入send buffer中，等待被发送。

接收流程:
- 从lower layer接收ack，对packet进行解析，读取sequence number。
- 根据接收的sequence number调整当前的window，从buffer中读取当前可被取出发送的packet并发出。

超时：
- 当出现超时时，sender将当前window里的所有packet都进行重发。

receiver:

接收流程：
- 从lower layer接收packet，对packet进行解析，读取sequence number。
- 如果接受的sequence number不是当前expected的packet（与之前收到的包不连续），则将其放入receive buffer中。如果收到的包是当前expected的packet，则开始扫描receive buffer。程序维护当前receiver收到的第一个还未解析完整的message sequence number。扫描过程从此sequence number开始，在buffer中从前向后扫描，通过while循环取出所有buffer内已经填充完成的所有message，并发送到上层。
- 将更新过后的expected - 1发送回给sender，表明expected之前的所有packet已经完全接收完毕。

## checksum

checksum算法借鉴16-bit因特网算法。
```
sender:
unsigned int sum = 0;
sum += pkt[0];
for(int i = 3; i < RDT_PKTSIZE; ++i)
{
    sum += pkt[i];
}
while (sum >> 16)
    sum = (sum >> 16) + (sum & 0xFFFF);
return ~sum;
```

```
receiver:
unsigned int sum = 0;
for(int i = 2; i < RDT_PKTSIZE; ++i)
{
    sum += pkt[i];
}
while (sum >> 16)
    sum = (sum >> 16) + (sum & 0xFFFF);
return ~sum;
```

