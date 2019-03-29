/*
 * FILE: rdt_receiver.cc
 * DESCRIPTION: Reliable data transfer receiver.
 * NOTE: This implementation assumes there is no packet loss, corruption, or 
 *       reordering.  You will need to enhance it to deal with all these 
 *       situations.  In this implementation, the packet format is laid out as 
 *       the following:
 *       
 *       |<-    2 byte  ->|<-   4 byte ->|
 *       |<-    checksum->|<-   seq num->|
 *
 *       The first byte of each packet indicates the size of the payload
 *       (excluding this single-byte header)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdt_struct.h"
#include "rdt_receiver.h"

#define SENDHEADERSIZE 8
#define HEADERSIZE 6
#define BUFFERSIZE 50000
#define MAXSIZE 1000
#define WINDOWSIZE 20

static packet *tmp_buffer;
int expected = 0;
int recentmessage = 0;
int lastone = 0;

short getchecksum(char *pkt)
{
    short checksum = ((short *)(pkt + 1))[0];
    return checksum;
}

short calChecksum(char *pkt)
{
    unsigned int sum = 0;
    sum += pkt[0];
    for(int i = 3; i < RDT_PKTSIZE; ++i)
    {
        sum += pkt[i];
    }
    while (sum >> 16)
        sum = (sum >> 16) + (sum & 0xFFFF);
    return ~sum;
}

short ackchecksum(char *pkt)
{
    unsigned int sum = 0;
    for(int i = 2; i < RDT_PKTSIZE; ++i)
    {
        sum += pkt[i];
    }
    while (sum >> 16)
        sum = (sum >> 16) + (sum & 0xFFFF);
    return ~sum;
}

bool verifysend(packet *pkt)
{
    short checksum1 = getchecksum(pkt->data);
    short checksum2 = calChecksum(pkt->data);
    return checksum1 == checksum2;
}

void addSeqnumber(char *pkt, int seqnum)
{
    ((int *)(pkt + 2))[0] = seqnum;
}

void addChecksum(char *pkt)
{
    ((short *)pkt)[0] = ackchecksum(pkt);
}

bool isFirst(char *pkt)
{
    return (pkt[0] == 1) || (pkt[0] == 3);
}

bool isEnd(char *pkt)
{
    return (pkt[0] == 2) || (pkt[0] == 3);
}

int seqnum(char *pkt)
{
    int seq = ((int *)(pkt + 3))[0];
    return seq;
}

static int getsize(char *pkt)
{
    int size = pkt[7];
    return size;
}

/* receiver initialization, called once at the very beginning */
void Receiver_Init()
{
    tmp_buffer = (packet *)malloc(BUFFERSIZE * sizeof(packet));
    fprintf(stdout, "At %.2fs: receiver initializing ...\n", GetSimulationTime());
}

/* receiver finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to use this opportunity to release some 
   memory you allocated in Receiver_init(). */
void Receiver_Final()
{
    fprintf(stdout, "At %.2fs: receiver finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a packet is passed from the lower layer at the 
   receiver */
void Receiver_FromLowerLayer(struct packet *pkt)
{
    if (!verifysend(pkt) || seqnum(pkt->data) > expected + WINDOWSIZE)
    {
        //printf("receiver: checksum error\n");
        return;
    }
    int seq = seqnum(pkt->data);
    if (seq > lastone)
    {
        lastone = seq;
    }
    //printf("receiver: a send come, seqnum %d\n", seq);
    if (seq < expected)
    {
        packet pt;
        addSeqnumber(pt.data, expected - 1);
        addChecksum(pt.data);
        Receiver_ToLowerLayer(&pt);
        return;
    }
    else if (seq != expected)
    {
        //printf("receiver: add %d to buffer, expected: %d\n", seq, expected);
        tmp_buffer[seq % BUFFERSIZE] = *pkt;
        return;
    }
    ////printf("receiver: receive an expected packet :%d\n", expected);
    tmp_buffer[seq % BUFFERSIZE] = *pkt;
    //printf("receiver: put the expected into buffer: %d, seqnum: %d\n", seq, seqnum(pkt->data));
    int i = expected;
    while (i <= lastone)
    {
        packet pkt = tmp_buffer[i % BUFFERSIZE];
        //printf("receiver: expected: %d, pkt num: %d\n", expected, seqnum(pkt.data));
        if (seqnum(pkt.data) != expected)
        {
            //printf("receiver: missing packet %d\n", seqnum(pkt.data));
            break;
        }
        ++expected;
        if (isFirst(pkt.data))
        {
            //printf("receiver: %d is a first packet\n", seqnum(pkt.data));
            recentmessage = seqnum(pkt.data);
        }
        if (isEnd(pkt.data))
        {
            //printf("receiver: %d is the last packet\n", seq);
            struct message *msg = (struct message *)malloc(sizeof(struct message));
            msg->data = (char *)malloc(MAXSIZE);
            msg->size = 0;

            int j = recentmessage;
            while (j <= i)
            {
                pkt = tmp_buffer[j % BUFFERSIZE];
                //printf("receiver: enter message loop, first seq %d, now seq %d\n", recentmessage, j);
                getsize(pkt.data);
                //printf("package size: %d\n", getsize(pkt.data));
                memcpy(msg->data + msg->size, pkt.data + SENDHEADERSIZE, getsize(pkt.data));
                //printf("...\n");
                msg->size += getsize(tmp_buffer[j % BUFFERSIZE].data);
                //printf(",,,\n");
                j++;
            }
            //printf("receiver: send message to upper layer\n");
            Receiver_ToUpperLayer(msg);
            if (msg->data != NULL)
                free(msg->data);
            if (msg != NULL)
                free(msg);
        }
        i++;
    }

    //printf("receiver: send back ack %d\n", expected - 1);
    packet pt;
    addSeqnumber(pt.data, expected - 1);
    addChecksum(pt.data);
    Receiver_ToLowerLayer(&pt);
}
