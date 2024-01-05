// transport h264 video
// ffmpeg -i test.mp4 -codec copy -bsf: h264_mp4toannexb -f h264 test.h264
// gcc main.c rtp.c rtp.h -o main
// ./main test.h264
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "rtp.h"

#define H264_FILE_NAME "test.h264"
#define ServerIP "192.168.50.236"
#define SERVER_PORT 8554
#define SERVER_RTP_PORT 55532
#define SERVER_RTCP_PORT 55533
#define BUF_MAX_SIZE (1024 * 1024)

static int createTcpSocket()
{
    int sockfd;
    int on = 1;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        return -1;

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(on));

    return sockfd;
}

static int createUdpSocket()
{
    int sockfd;
    int on = 1;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        return -1;

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(on));

    return sockfd;
}

static int bindSocketAddr(int sockfd, const char *ip, int port)
{
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0)
        return -1;

    return 0;
}

static int acceptClient(int sockfd, char *ip, int *port)
{
    int clientfd;
    socklen_t len = 0;
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    len = sizeof(addr);

    clientfd = accept(sockfd, (struct sockaddr *)&addr, &len);
    if (clientfd < 0)
        return -1;

    strcpy(ip, inet_ntoa(addr.sin_addr));
    *port = ntohs(addr.sin_port);

    return clientfd;
}
// 判断前三个元素是否分别为 0 0 1
// 用于标识帧开始或编码参数等信息
static inline int startCode3(char *buf)
{
    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1)
        return 1;
    else
        return 0;
}

// 判断前四个字节是否分别为 0 0 0 1
// 用于表示帧开始或编码参数等信息
static inline int startCode4(char *buf)
{
    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 1)
        return 1;
    else
        return 0;
}

static char *findNextStartCode(char *buf, int len)
{
    int i;

    if (len < 3)
        return NULL;

    for (i = 0; i < len - 3; ++i)
    {
        if (startCode3(buf) || startCode4(buf))
            return buf;

        ++buf;
    }

    if (startCode3(buf))
        return buf;

    return NULL;
}

// 从H.264文件中读取一个帧的数据
static int getFrameFromH264File(FILE *fp, char *frame, int size)
{
    int rSize, frameSize;
    char *nextStartCode;

    if (fp < 0)
        return -1;

    rSize = fread(frame, 1, size, fp);

    // 检查是否为特定的起始码开始
    if (!startCode3(frame) && !startCode4(frame))
        return -1;

    nextStartCode = findNextStartCode(frame + 3, rSize - 3);
    if (!nextStartCode)
    {
        // lseek(fd, 0, SEEK_SET);
        // frameSize = rSize;
        return -1;
    }
    else
    {
        frameSize = (nextStartCode - frame);
        fseek(fp, frameSize - rSize, SEEK_CUR);
    }

    return frameSize;
}

// 将 H.264 视频帧通过RTP协议发送到指定的IP地址和端口
static int rtpSendH264Frame(int serverRtpSockfd, const char *ip, int16_t port,
                            struct RtpPacket *rtpPacket, char *frame, uint32_t frameSize)
{

    uint8_t naluType; // nalu第一个字节
    int sendBytes = 0;
    int ret;

    // 获取帧数据的第一个字节
    naluType = frame[0];

    printf("frameSize=%d \n", frameSize);

    // 如果帧数据的大小小于RTP最大包大小
    if (frameSize <= RTP_MAX_PKT_SIZE) // nalu长度小于最大包长：单一NALU单元模式
    {

        //*   0 1 2 3 4 5 6 7 8 9
        //*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //*  |F|NRI|  Type   | a single NAL unit ... |
        //*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

        // 先将整个数据复制到rtpPacket的负载payload中
        // 然后调用rtpSendPacketOverUdp通过UDP发送RTP包
        memcpy(rtpPacket->payload, frame, frameSize);
        ret = rtpSendPacketOverUdp(serverRtpSockfd, ip, port, rtpPacket, frameSize);
        if (ret < 0)
            return -1;

        // 发送成功后更新RTP序列号，增加发送的字节数，并检查帧数据的类型是否为SPS或PPS
        rtpPacket->rtpHeader.seq++;
        sendBytes += ret;
        if ((naluType & 0x1F) == 7 || (naluType & 0x1F) == 8) // 如果是SPS、PPS就不需要加时间戳
            goto out;
    }
    else // nalu长度小于最大包场：分片模式
    {

        //*  0                   1                   2
        //*  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
        //* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //* | FU indicator  |   FU header   |   FU payload   ...  |
        //* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

        //*     FU Indicator
        //*    0 1 2 3 4 5 6 7
        //*   +-+-+-+-+-+-+-+-+
        //*   |F|NRI|  Type   |
        //*   +---------------+

        //*      FU Header
        //*    0 1 2 3 4 5 6 7
        //*   +-+-+-+-+-+-+-+-+
        //*   |S|E|R|  Type   |
        //*   +---------------+

        int pktNum = frameSize / RTP_MAX_PKT_SIZE;        // 有几个完整的包
        int remainPktSize = frameSize % RTP_MAX_PKT_SIZE; // 剩余不完整包的大小
        int i, pos = 1;

        // 发送完整的包
        for (i = 0; i < pktNum; i++)
        {
            // 为了设置分片包的FU Indicator字段
            rtpPacket->payload[0] = (naluType & 0x60) | 28;
            // 为了设置分包的UF Header字段
            rtpPacket->payload[1] = naluType & 0x1F;

            if (i == 0)                                     // 第一包数据
                rtpPacket->payload[1] |= 0x80;              // start
            else if (remainPktSize == 0 && i == pktNum - 1) // 最后一包数据
                rtpPacket->payload[1] |= 0x40;              // end

            memcpy(rtpPacket->payload + 2, frame + pos, RTP_MAX_PKT_SIZE);
            ret = rtpSendPacketOverUdp(serverRtpSockfd, ip, port, rtpPacket, RTP_MAX_PKT_SIZE + 2);
            if (ret < 0)
                return -1;

            rtpPacket->rtpHeader.seq++;
            sendBytes += ret;
            pos += RTP_MAX_PKT_SIZE;
        }

        // 发送剩余的数据
        if (remainPktSize > 0)
        {
            rtpPacket->payload[0] = (naluType & 0x60) | 28;
            rtpPacket->payload[1] = naluType & 0x1F;
            rtpPacket->payload[1] |= 0x40; // end

            memcpy(rtpPacket->payload + 2, frame + pos, remainPktSize + 2);
            ret = rtpSendPacketOverUdp(serverRtpSockfd, ip, port, rtpPacket, remainPktSize + 2);
            if (ret < 0)
                return -1;

            rtpPacket->rtpHeader.seq++;
            sendBytes += ret;
        }
    }

    rtpPacket->rtpHeader.timestamp += 90000 / 25;
out:

    return sendBytes;
}

static int handleCmd_OPTIONS(char *result, int cseq)
{
    sprintf(result, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %d\r\n"
                    "Public: OPTIONS, DESCRIBE, SETUP, PLAY\r\n"
                    "\r\n",
            cseq);

    return 0;
}

static int handleCmd_DESCRIBE(char *result, int cseq, char *url)
{
    char sdp[500];
    char localIp[100];

    sscanf(url, "rtsp://%[^:]:", localIp);

    sprintf(sdp, "v=0\r\n"
                 "o=- 9%ld 1 IN IP4 %s\r\n"
                 "t=0 0\r\n"
                 "a=control:*\r\n"
                 "m=video 0 RTP/AVP 96\r\n"
                 "a=rtpmap:96 H264/90000\r\n"
                 "a=control:track0\r\n",
            time(NULL), localIp);

    sprintf(result, "RTSP/1.0 200 OK\r\nCSeq: %d\r\n"
                    "Content-Base: %s\r\n"
                    "Content-type: application/sdp\r\n"
                    "Content-length: %zu\r\n\r\n"
                    "%s",
            cseq,
            url,
            strlen(sdp),
            sdp);

    return 0;
}

static int handleCmd_SETUP(char *result, int cseq, int clientRtpPort)
{
    sprintf(result, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %d\r\n"
                    "Transport: RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d\r\n"
                    "Session: 66334873\r\n"
                    "\r\n",
            cseq,
            clientRtpPort,
            clientRtpPort + 1,
            SERVER_RTP_PORT,
            SERVER_RTCP_PORT);

    return 0;
}

static int handleCmd_PLAY(char *result, int cseq)
{
    sprintf(result, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %d\r\n"
                    "Range: npt=0.000-\r\n"
                    "Session: 66334873; timeout=10\r\n\r\n",
            cseq);

    return 0;
}

//
static void doClient(int clientSockfd, const char *clientIP, int clientPort)
{
    // 存储解析出的请求方法的字符串数组
    char method[40];
    // 存储解析出的URL的字符串数组
    char url[100];
    // 存储解析出的协议版本号的字符串数组
    char version[40];
    // 存储解析出的请求序列号的整数
    int CSeq;

    int serverRtpSockfd = -1, serverRtcpSockfd = -1;
    int clientRtpPort, clientRtcpPort;
    // 用于接收客户端请求消息的缓冲区
    char *rBuf = (char *)malloc(BUF_MAX_SIZE);
    // 用于存储服务器相应消息的缓冲区
    char *sBuf = (char *)malloc(BUF_MAX_SIZE);

    while (1)
    {
        int recvLen;
        // 从客户端接收请求消息，并存储在rBuf缓冲区中
        recvLen = recv(clientSockfd, rBuf, BUF_MAX_SIZE, 0);
        if (recvLen <= 0)
        {
            break;
        }

        rBuf[recvLen] = '\0';
        printf("%s rBuf = %s \n", __FUNCTION__, rBuf);

        const char *sep = "\n";
        char *line = strtok(rBuf, sep);
        // 根据解析出的方法调用相应的处理函数,并传递相应的参数
        while (line)
        {
            if (strstr(line, "OPTIONS") ||
                strstr(line, "DESCRIBE") ||
                strstr(line, "SETUP") ||
                strstr(line, "PLAY"))
            {

                if (sscanf(line, "%s %s %s\r\n", method, url, version) != 3)
                {
                    // error
                }
            }
            else if (strstr(line, "CSeq"))
            {
                if (sscanf(line, "CSeq: %d\r\n", &CSeq) != 1)
                {
                    // error
                }
            }
            else if (!strncmp(line, "Transport:", strlen("Transport:")))
            {
                // Transport: RTP/AVP/UDP;unicast;client_port=13358-13359
                // Transport: RTP/AVP;unicast;client_port=13358-13359

                if (sscanf(line, "Transport: RTP/AVP/UDP;unicast;client_port=%d-%d\r\n",
                           &clientRtpPort, &clientRtcpPort) != 2)
                {
                    // error
                    printf("parse Transport error \n");
                }
            }
            line = strtok(NULL, sep);
        }

        if (!strcmp(method, "OPTIONS"))
        {
            if (handleCmd_OPTIONS(sBuf, CSeq))
            {
                printf("failed to handle options\n");
                break;
            }
        }
        else if (!strcmp(method, "DESCRIBE"))
        {
            if (handleCmd_DESCRIBE(sBuf, CSeq, url))
            {
                printf("failed to handle describe\n");
                break;
            }
        }
        else if (!strcmp(method, "SETUP"))
        {
            if (handleCmd_SETUP(sBuf, CSeq, clientRtpPort))
            {
                printf("failed to handle setup\n");
                break;
            }

            serverRtpSockfd = createUdpSocket();
            serverRtcpSockfd = createUdpSocket();

            if (serverRtpSockfd < 0 || serverRtcpSockfd < 0)
            {
                printf("failed to create udp socket\n");
                break;
            }

            if (bindSocketAddr(serverRtpSockfd, ServerIP, SERVER_RTP_PORT) < 0 ||
                bindSocketAddr(serverRtcpSockfd, ServerIP, SERVER_RTCP_PORT) < 0)
            {
                printf("failed to bind addr\n");
                break;
            }
        }
        else if (!strcmp(method, "PLAY"))
        {
            if (handleCmd_PLAY(sBuf, CSeq))
            {
                printf("failed to handle play\n");
                break;
            }
        }
        else
        {
            printf("未定义的method = %s \n", method);
            break;
        }
        printf("sBuf = %s \n", sBuf);
        printf("%s sBuf = %s \n", __FUNCTION__, sBuf);

        // 将处理函数返回的相应消息存储在sBuf缓冲区中
        send(clientSockfd, sBuf, strlen(sBuf), 0);

        // 开始播放，发送RTP包
        if (!strcmp(method, "PLAY"))
        {
            // 存储帧数据的大小和起始码的类型
            int frameSize, startCode;
            // 用于存储数据帧
            char *frame = (char *)malloc(500000);
            // 用于存储RTP数据包
            struct RtpPacket *rtpPacket = (struct RtpPacket *)malloc(500000);
            // 用于打开 H.264视频文件
            FILE *fp = fopen(H264_FILE_NAME, "rb");
            if (!fp)
            {
                printf("读取 %s 失败\n", H264_FILE_NAME);
                break;
            }
            // 初始化rtpPacket中的RTP头部信息，包括版本号，负载类型
            rtpHeaderInit(rtpPacket, 0, 0, 0, RTP_VESION, RTP_PAYLOAD_TYPE_H264, 0,
                          0, 0, 0x88923423);

            printf("start play\n");
            printf("client ip:%s\n", clientIP);
            printf("client port:%d\n", clientRtpPort);

            // 从H.264文件中读取帧数据到fram数组中，如果读取失败或已经读取完所有的帧数据就跳出循环
            while (1)
            {
                frameSize = getFrameFromH264File(fp, frame, 500000);
                if (frameSize < 0)
                {
                    printf("读取%s结束,frameSize=%d \n", H264_FILE_NAME, frameSize);
                    break;
                }

                // 检查起始码类型
                if (startCode3(frame))
                    startCode = 3;
                else
                    startCode = 4;
                // 根据起始码类型调整帧数据的大小，将起始码部分去除
                frameSize -= startCode;
                // 使用RTP协议将帧数据发送到客户端的RTP端口
                rtpSendH264Frame(serverRtpSockfd, clientIP, clientRtpPort,
                                 rtpPacket, frame + startCode, frameSize);

                // 控制帧的发送速率
                //  Sleep(40);
                usleep(40000); // 1000/25 * 1000
            }
            // 释放动态分配的内存，包括frame数组和rtpPacket结构体
            free(frame);
            free(rtpPacket);

            break;
        }
        // method字符串数组用于存储解析出的请求方法
        // url字符串数组用于存储解析出的URL
        // 这里将其之前存储的内容清空，以便下一次请求的处理
        memset(method, 0, sizeof(method) / sizeof(char));
        memset(url, 0, sizeof(url) / sizeof(char));
        // 重置序列号，以便下一次请求的处理
        CSeq = 0;
    }

    // 关闭与客户端的套接字连接，释放与客户端通信的资源
    close(clientSockfd);
    if (serverRtpSockfd)
    {
        close(serverRtpSockfd);
    }
    if (serverRtcpSockfd > 0)
    {
        close(serverRtcpSockfd);
    }

    free(rBuf);
    free(sBuf);
}

int main(int argc, char *argv[])
{

    int rtspServerSockfd;

    rtspServerSockfd = createTcpSocket();
    if (rtspServerSockfd < 0)
    {
        printf("failed to create tcp socket\n");
        return -1;
    }

    if (bindSocketAddr(rtspServerSockfd, ServerIP, SERVER_PORT) < 0)
    {
        printf("failed to bind addr\n");
        return -1;
    }

    if (listen(rtspServerSockfd, 10) < 0)
    {
        printf("failed to listen\n");
        return -1;
    }

    printf("%s rtsp://%s:%d\n", __FILE__, ServerIP, SERVER_PORT);

    while (1)
    {
        int clientSockfd;
        char clientIp[40];
        int clientPort;

        clientSockfd = acceptClient(rtspServerSockfd, clientIp, &clientPort);
        if (clientSockfd < 0)
        {
            printf("failed to accept client\n");
            return -1;
        }

        printf("accept client;client ip:%s,client port:%d\n", clientIp, clientPort);

        doClient(clientSockfd, clientIp, clientPort);
    }

    // 关闭套接字连接
    close(rtspServerSockfd);

    return 0;
}
