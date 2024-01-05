
#include <stdint.h>

#define RTP_VESION 2

#define RTP_PAYLOAD_TYPE_H264 96
#define RTP_PAYLOAD_TYPE_AAC 97

#define RTP_HEADER_SIZE 12
#define RTP_MAX_PKT_SIZE 1400

/*
 *    0                   1                   2                   3
 *    7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |                           timestamp                           |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |           synchronization source (SSRC) identifier            |
 *   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *   |            contributing source (CSRC) identifiers             |
 *   :                             ....                              :
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */

//定义RTP包头部的结构。它包含了一个12字节的头部，用来描述RTP数据包的一些信息
//如版本号、负载类型、序列号、时间戳等
struct RtpHeader
{
    /* byte 0 */
    uint8_t csrcLen : 4;
    uint8_t extension : 1;
    uint8_t padding : 1;
    uint8_t version : 2;

    /* byte 1 */
    uint8_t payloadType : 7;
    uint8_t marker : 1;

    /* bytes 2,3 */
    uint16_t seq;

    /* bytes 4-7 */
    uint32_t timestamp;

    /* bytes 8-11 */
    uint32_t ssrc;
};

//定义了一个完整的RTP数据包的结构
//包含了RtpHeader结构和一个长度不确定的负载部分
struct RtpPacket
{
    struct RtpHeader rtpHeader;
    uint8_t payload[0];
};

//初始化RTP数据包的头部
void rtpHeaderInit(struct RtpPacket *rtpPacket, uint8_t csrcLen, uint8_t extension,
                   uint8_t padding, uint8_t version, uint8_t payloadType, uint8_t marker,
                   uint16_t seq, uint32_t timestamp, uint32_t ssrc);
//通过TCP协议发送RTP数据包
//接受一个客户端的套接字描述符，一个RTP数据包的指针以及数据包的大小作为参数
int rtpSendPacketOverTcp(int clientSockfd, struct RtpPacket *rtpPacket, uint32_t dataSize);

//通过UDP协议额发送RTP数据包
//接受一个服务器端的RTP套接字描述符、目标IP地址、目标端口号、RTP数据包的指针以及数据包的大小作为参数
int rtpSendPacketOverUdp(int serverRtpSockfd, const char *ip, int16_t port, struct RtpPacket *rtpPacket, uint32_t dataSize);
