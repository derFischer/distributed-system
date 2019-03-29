/*
 * FILE: rdt_sender.cc
 * DESCRIPTION: Reliable data transfer sender.
 * NOTE: This implementation assumes there is no packet loss, corruption, or 
 *       reordering.  You will need to enhance it to deal with all these 
 *       situations.  In this implementation, the packet format is laid out as 
 *       the following:
 *       
 *  |<-     1 byte      ->|<-     2 byte      ->|<-         4 byte          ->|<-  1 byte  ->|<-             the rest            ->|
 *  |<-     first packet->|<-     checksum    ->|<-     sequence number     ->| payload size |<-             payload             ->|
 *
 *       The first byte of each packet indicates the size of the payload
 *       (excluding this single-byte header)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdt_struct.h"
#include "rdt_sender.h"

#define HEADERSIZE 8
#define MAXPAYLOADSIZE (RDT_PKTSIZE - HEADERSIZE)
#define WINDOWSIZE 20
#define BUFFERSIZE 50000
#define TIMEOUT 0.3

static packet *tmp_buffer;
packet *windows;
int sequence_number = -2;
int windowStart = 0;
int bufferNum = 0;
int windowNum = 0;
int next_send_seq = 0;

//helper
short checksum(char *pkt)
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

short ackcheck(char *pkt)
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

void addFirstPacket(char *packet, bool first, bool end)
{
    if (first && end)
    {
        packet[0] = 3;
    }
    else if (first)
    {
        packet[0] = 1;
    }
    else if (end)
    {
        packet[0] = 2;
    }
    else
    {
        packet[0] = 0;
    }
}

short recchecksum(char *pkt)
{
    short checksum = ((short *)(pkt))[0];
    return checksum;
}

void addChecksum(char *packet, short checksum)
{
    packet[1] = (checksum)&0xFF;
    packet[2] = (checksum >> 8) & 0xFF;
}

void addSequencenumber(char *packet)
{
    ((int *)(packet + 3))[0] = sequence_number++;
}

void addPayloadsize(char *packet, int payload)
{
    packet[7] = payload & 0xFF;
}

void package(char *packet, int payload, bool first, bool end)
{
    addSequencenumber(packet);
    addPayloadsize(packet, payload);
    addFirstPacket(packet, first, end);
    addChecksum(packet, checksum(packet));
}

int sendseqnum(packet pkt)
{
    int seq = ((int *)(pkt.data + 3))[0];
    return seq;
}

bool canSend(packet pkt)
{
    int seq = ((int *)(pkt.data + 3))[0];
    //printf("sender: now seqnum %d, window start %d\n", seq, windowStart);
    return (seq < windowStart + WINDOWSIZE && seq >= windowStart);
}

int recseqnum(packet *pkt)
{
    int seq = ((int *)(pkt->data + 2))[0];
    return seq;
}

void addToBuffer(packet pkt)
{
    int seq = sendseqnum(pkt);
    int seq2 = sendseqnum(tmp_buffer[seq % BUFFERSIZE]);
    if (seq != seq2)
    {
        ++bufferNum;
        tmp_buffer[sendseqnum(pkt) % BUFFERSIZE] = pkt;
    }
}

bool verifyrec(packet *pkt)
{
    short checksum1 = recchecksum(pkt->data);
    short checksum2 = ackcheck(pkt->data);
    return checksum1 == checksum2;
}

static int getsize(char *pkt)
{
    int size = pkt[7];
    return size;
}

/* sender initialization, called once at the very beginning */
void Sender_Init()
{
    tmp_buffer = (packet *)malloc(BUFFERSIZE * sizeof(packet));
    windows = (packet *)malloc(WINDOWSIZE * sizeof(packet));
    addSequencenumber(windows[0].data);
    addSequencenumber(tmp_buffer[0].data);

    fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some 
   memory you allocated in Sender_init(). */
void Sender_Final()
{
    fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
}

void addTowindow(packet pkt)
{
    int seq = sendseqnum(pkt);
    int seq2 = sendseqnum(windows[seq % WINDOWSIZE]);
    if (seq != seq2)
    {
        ++windowNum;
        ASSERT(windowNum <= WINDOWSIZE);
        windows[seq % WINDOWSIZE] = pkt;
    }
    return;
}

void sendpkt(packet *pkt)
{
    if (canSend(*pkt))
    {
        addTowindow(*pkt);
        if (!Sender_isTimerSet())
        {
            Sender_StartTimer(TIMEOUT);
        }
        Sender_ToLowerLayer(pkt);
        int seq = sendseqnum(*pkt);
        if (seq >= next_send_seq)
        {
            next_send_seq = seq + 1;
        }
        //printf("sender: send %d to lower layer finish\n", sendseqnum(*pkt));
    }
    else
    {
        addToBuffer(*pkt);
        //printf("sender: add %d to buffer\n", sendseqnum(*pkt));
    }
}

/* event handler, called when a message is passed from the upper layer at the 
   sender */
void Sender_FromUpperLayer(struct message *msg)
{
    //printf("sender: a message comes\n");

    /* split the message if it is too big */

    /* reuse the same packet data structure */
    packet pkt;

    /* the cursor always points to the first unsent byte in the message */
    int cursor = 0;
    bool first = true;
    bool end = false;

    while (msg->size - cursor > MAXPAYLOADSIZE)
    {
        //printf("sender: start a round of sending\n");
        /* fill in the packet */
        memcpy(pkt.data + HEADERSIZE, msg->data + cursor, MAXPAYLOADSIZE);
        //printf("sender: package the packet\n");
        package(pkt.data, MAXPAYLOADSIZE, first, end);
        /* send it out through the lower layer */
        //printf("sender: send the package to lower layer, size %d\n", getsize(pkt.data));
        sendpkt(&pkt);
        //printf("sender: finish sending\n");
        /* move the cursor */
        cursor += MAXPAYLOADSIZE;
        first = false;
    }

    /* send out the last packet */
    end = true;
    if (msg->size > cursor)
    {
        /* fill in the packet */
        int size = msg->size - cursor;
        //printf("start a round of sending\n");
        memcpy(pkt.data + HEADERSIZE, msg->data + cursor, size);
        //printf("package the packet\n");
        package(pkt.data, size, first, end);
        /* send it out through the lower layer */
        //printf("sender: send the package to lower layer, size %d\n", getsize(pkt.data));
        sendpkt(&pkt);
        //printf("finish sending\n");
    }
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{
    if (!verifyrec(pkt) || (recseqnum(pkt) > sequence_number))
    {
        //printf("sender: checksum error!\n");
        return;
    }
    else
    {
        int recseq = recseqnum(pkt);
        //printf("sender: an ack come, ack seq num %d\n", recseq);
        int oldstart = windowStart;
        if (recseq == windowStart + windowNum - 1)
        {
            //printf("sender: stop timer\n");
            Sender_StopTimer();
        }
        if (windowStart > recseq)
        {
            return;
        }
        windowStart = recseq + 1;
        windowNum -= windowStart - oldstart;
        packet p;
        int i = 0;
        while (bufferNum > 0 && canSend(p = tmp_buffer[next_send_seq % BUFFERSIZE]))
        {
            //printf("sender: send the package %d in the buffer to lower layer\n", sendseqnum(p));
            sendpkt(&p);
            i++;
            bufferNum--;
        }
    }
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
    //printf("sender: time out\n");
    int i = 0;
    int start = windowStart;
    while (i < windowNum)
    {
        //printf("sender: i:%d, windownum:%d\n", i, windowNum);
        sendpkt(&windows[start % WINDOWSIZE]);
        i++;
        start++;
    }
}
