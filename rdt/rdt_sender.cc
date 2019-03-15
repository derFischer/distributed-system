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
#define MAXPAYLOADSIZE (RDT_PKTSIZE - HEADERSIZE）
#define WINDOWSIZE 3
#define BUFFERSIZE 1000

packet* tmp_buffer;
packet* windows;
int sequence_number = 0;
int windowStart = 0;
int bufferNum = 0;
int windowNum = 0;

//helper
short checksum(char *message)
{
    return 0;
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

bool canSend(packet pkt)
{
    int seq = ((int *)(pkt.data + 3))[0];
    return (seq < windowStart + WINDOWSIZE);
}

void addToBuffer(packet pkt)
{
    tmp_buffer[bufferNum++] = pkt;
}

/* sender initialization, called once at the very beginning */
void Sender_Init()
{
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

/* event handler, called when a message is passed from the upper layer at the 
   sender */
void Sender_FromUpperLayer(struct message *msg)
{

    /* split the message if it is too big */

    /* reuse the same packet data structure */
    packet pkt;

    /* the cursor always points to the first unsent byte in the message */
    int cursor = 0;
    bool first = true;
    bool end = false;

    while (msg->size - cursor > MAXPAYLOADSIZE)
    {
        /* fill in the packet */
        memcpy(pkt.data + HEADERSIZE, msg->data + cursor, MAXPAYLOADSIZE);
        package(pkt.data, MAXPAYLOADSIZE, first, end);
        /* send it out through the lower layer */
        if (canSend(pkt))
        {
            Sender_ToLowerLayer(&pkt);
        }
        else
        {
            addToBuffer(pkt);
        }
        /* move the cursor */
        cursor += maxpayload_size;
        first = false;
    }

    /* send out the last packet */
    end = true;
    if (msg->size > cursor)
    {
        /* fill in the packet */
        int size = msg->size - cursor;
        memcpy(pkt.data + HEADERSIZE, msg->data + cursor, size);
        package(pkt.data, size, first, end);
        /* send it out through the lower layer */
        if (canSend(pkt))
        {
            Sender_ToLowerLayer(&pkt);
        }
        else
        {
            addToBuffer(pkt);
        }
    }
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
}
