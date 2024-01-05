#include <stdint.h>

#define RTP_VESION 2

#define RTP_PAYLOAD_TYPE_H264 96
#define RTP_PAYLOAD_TYPE_AAC 97

#define RTP_HEADER_SIZE 12
#define RTP_MAX_PKT_SIZE 1400


struct RtpHeader
{
    /* byte 0 */
    uint8_t csrcLen : 4;   // CSRC计数器，占4位，指示CSRC 标识符的个数。
    uint8_t extension : 1; // 占1位，如果X=1，则在RTP报头后跟有一个扩展报头。
    uint8_t padding : 1;   // 填充标志，占1位，如果P=1，则在该报文的尾部填充一个或多个额外的八位组，它们不是有效载荷的一部分。
    uint8_t version : 2;   // RTP协议的版本号，占2位，当前协议版本号为2。

    /* byte 1 */
    uint8_t payloadType : 7; // 有效载荷类型，占7位，用于说明RTP报文中有效载荷的类型，如GSM音频、JPEM图像等。
    uint8_t marker : 1;      // 标记，占1位，不同的有效载荷有不同的含义，对于视频，标记一帧的结束；对于音频，标记会话的开始。

    /* bytes 2,3 */
    uint16_t seq; // 占16位，用于标识发送者所发送的RTP报文的序列号，每发送一个报文，序列号增1。接收者通过序列号来检测报文丢失情况，重新排序报文，恢复数据。

    /* bytes 4-7 */
    uint32_t timestamp; // 占32位，时戳反映了该RTP报文的第一个八位组的采样时刻。接收者使用时戳来计算延迟和延迟抖动，并进行同步控制。

    /* bytes 8-11 */
    uint32_t ssrc; // 占32位，用于标识同步信源。该标识符是随机选择的，参加同一视频会议的两个同步信源不能有相同的SSRC。

    /*标准的RTP Header 还可能存在 0-15个特约信源(CSRC)标识符

    每个CSRC标识符占32位，可以有0～15个。每个CSRC标识了包含在该RTP报文有效载荷中的所有特约信源

    */
};

struct RtpPacket
{
    struct RtpHeader rtpHeader;
    uint8_t payload[0];
};

// 初始化struct RtpPacket中的struct RtpHeader字段，可以设置各个字段的值
void rtpHeaderInit(struct RtpPacket *rtpPacket, uint8_t csrcLen, uint8_t extension,
                   uint8_t padding, uint8_t version, uint8_t payloadType, uint8_t marker,
                   uint16_t seq, uint32_t timestamp, uint32_t ssrc);

// 通过TCP将RTP报文发送给客户端，它接受一个客户端套接字描述符、指向RTP报文的指针以及数据大小，并将报文发送给客户端
int rtpSendPacketOverTcp(int clientSockfd, struct RtpPacket *rtpPacket, uint32_t dataSize);
// 通过UDP将RTP报文发送给指定的IP地址和端口号，它接受一个服务器的RTP套接字描述符，目标IP地址、目标端口号、
// 指向RTP报文的指针以及数据大小，并将报文发送给指定的地址和端口
int rtpSendPacketOverUdp(int serverRtpSockfd, const char *ip, int16_t port, struct RtpPacket *rtpPacket, uint32_t dataSize);
