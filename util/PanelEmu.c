/*
 * Emulation for Rasp PI GPIO via Server connected to via Socket
 *
 */
#include "qemu/osdep.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#ifdef __MINGW32__
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif


#include "qemu/PanelEmu.h"

typedef enum
{
    MACHINEDESC = 0,
    PINSTOPANEL = 1,
    READREQ = 2,
    PINCOUNT = 3,
    ENABLEMAP = 4,
    INPUTMAP = 5,
    OUTPUTMAP = 6,
    PINSTOQEMU = 7
} PacketType;

#define MAXPACKET   255

#define PACKETLEN   0  //Includes Packet Length
#define PACKETTYPE  1

typedef struct
{
    unsigned short int Data[MAXPACKET];
} CommandPacket;

/*
 * Hub connections
 *
 */

int panel_open(panel_connection_t* h)
{
    struct sockaddr_in remote;
    int returnval = - 1;

#ifdef __MINGW32__
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(1, 1), &wsadata) == SOCKET_ERROR) {
        printf("Error creating socket.");
    }
    else
#endif
    {
        if ((h->socket = socket(AF_INET, SOCK_STREAM, 0)) != - 1) {
            remote.sin_family = AF_INET;
            remote.sin_port = htons(PANEL_PORT);
            remote.sin_addr.s_addr = inet_addr("127.0.0.1");
            if (connect(h->socket, (struct sockaddr *) &remote, sizeof (remote)) != - 1) {
#ifdef __MINGW32__
                char value = 1;
                setsockopt(h->socket, IPPROTO_TCP, TCP_NODELAY, &value, sizeof ( value));

#endif
                FD_ZERO(&h->fds);

                /* Set our connected socket */
                FD_SET(h->socket, &h->fds);

                printf(PANEL_NAME "Connected OK\n");
                returnval = 0;
            } else {
                perror(PANEL_NAME "connection");
#ifdef __MINGW32__
                closesocket(h->socket);
#else
                close(h->socket);
#endif
                h->socket = - 1;
            }
        }
    }
    return returnval;
}

static void panel_command(panel_connection_t *h, CommandPacket *Pkt)
{
    if (send(h->socket, (char *) Pkt, Pkt->Data[PACKETLEN], 0) == - 1) {
        perror(PANEL_NAME "send");
#ifdef __MINGW32__
        closesocket(h->socket);
#else
        close(h->socket);
#endif
        h->socket = - 1; /* act like we never connected */
    }
}

/* Wait for values to be read back from panel */
bool panel_read(panel_connection_t* h, uint64_t* Data)
{
    fd_set rfds, efds;
    int LengthInBuffer;
    int select_res = 0;

    CommandPacket *PktPtr = (CommandPacket *) malloc(sizeof (CommandPacket));
    CommandPacket *Pkt;
    bool NoError = true;
    bool NewData = false;
    bool NoData = false;
    struct timeval timeout;

    int ReadStart = 0;

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    if (h->socket != - 1) {
        rfds = h->fds;
        efds = h->fds;

        printf(PANEL_NAME "panel_read\n");

        Pkt = PktPtr;
        while (NoError&&! NoData) {
            select_res = select(h->socket + 1, &rfds, NULL, &efds, &timeout);
            if (select_res > 0) {
                if (FD_ISSET(h->socket, &rfds)) {
                    /* receive more data */
                    if ((LengthInBuffer = recv(h->socket, (char *) &Pkt[ReadStart], sizeof (*Pkt) - ReadStart, 0)) > 0) {
                        LengthInBuffer += ReadStart;
                        for (int i = 0; LengthInBuffer > 0; i ++) {
                            if (LengthInBuffer >= Pkt->Data[i + PACKETLEN]) {
                                if (Pkt->Data[i + PACKETTYPE] == PINSTOQEMU) {
                                    *Data = (uint64_t) Pkt->Data[i + 2];
                                    *Data |= ((uint64_t) Pkt->Data[i + 3]) << 16;
                                    *Data |= ((uint64_t) Pkt->Data[i + 4]) << 32;
                                    *Data |= ((uint64_t) Pkt->Data[i + 5]) << 48;

                                    NewData = true;
                                } else {
                                    printf(PANEL_NAME "Invalid data received\n");
                                }
                                LengthInBuffer -= Pkt->Data[PACKETLEN];
                                i += Pkt->Data[PACKETLEN]; //								Pkt=(CommandPacket *)&(Pkt->Data[Pkt->Data[PACKETLEN]]);
                            } else {
                                ReadStart = LengthInBuffer;
                                for (int j = 0; j < LengthInBuffer; j ++) {
                                    Pkt->Data[j] = Pkt->Data[i + j];
                                }
                                printf(PANEL_NAME "Partial Packet Read");
                            }
                        }
                    } else {
                        if (LengthInBuffer < 0) {
                            if (errno != EINTR) {
                                printf(PANEL_NAME "recv");
                                NoError = FALSE;
                            }
                        } else {
                            printf(PANEL_NAME "closed connection\n");
                            NoError = FALSE;
                        }
                    }
                }
            } else if (select_res == 0) {
                NoData = true;
            } else if (errno != EINTR) {
#ifdef __MINGW32__
                closesocket(h->socket);
#else
                close(h->socket);
#endif
                h->socket = - 1; /* act like we never connected */
                perror(PANEL_NAME "select error");
                NoError = FALSE;
            }
        }
    }

    free(PktPtr);

    return NewData;
}

void panel_send_read_command(panel_connection_t* h)
{
    CommandPacket Pkt;

    Pkt.Data[PACKETLEN] = 4;
    Pkt.Data[PACKETTYPE] = READREQ;

    panel_command(h, &Pkt);
}

/* Set a pin to a specified value */
void senddatatopanel(panel_connection_t* h, uint64_t pin, bool val)
{
    CommandPacket Pkt;

    Pkt.Data[PACKETLEN] = (char *) &Pkt.Data[6 + 1]-(char *) &Pkt.Data[0];
    Pkt.Data[PACKETTYPE] = PINSTOPANEL;
    Pkt.Data[2] = (unsigned short int) (pin & 0xFFFF);
    Pkt.Data[3] = (unsigned short int) ((pin >> 16)&0xFFFF);
    Pkt.Data[4] = (unsigned short int) (pin >> 32 & 0xFFFF);
    Pkt.Data[5] = (unsigned short int) ((pin >> 48)&0xFFFF);
    Pkt.Data[6] = val;

    panel_command(h, &Pkt);
}

void sendpincount(panel_connection_t* h, int val)
{
    CommandPacket Pkt;

    Pkt.Data[PACKETLEN] = (char *) &Pkt.Data[2 + 1]-(char *) &Pkt.Data[0];
    Pkt.Data[PACKETTYPE] = PINCOUNT;
    Pkt.Data[2] = val;

    panel_command(h, &Pkt);
}

void sendenabledmap(panel_connection_t* h, uint64_t pin)
{
    CommandPacket Pkt;

    Pkt.Data[PACKETLEN] = (char *) &Pkt.Data[5 + 1]-(char *) &Pkt.Data[0];
    Pkt.Data[PACKETTYPE] = ENABLEMAP;
    Pkt.Data[2] = (unsigned short int) (pin & 0xFFFF);
    Pkt.Data[3] = (unsigned short int) ((pin >> 16)&0xFFFF);
    Pkt.Data[4] = (unsigned short int) (pin >> 32 & 0xFFFF);
    Pkt.Data[5] = (unsigned short int) ((pin >> 48)&0xFFFF);

    panel_command(h, &Pkt);
}

void sendinputmap(panel_connection_t* h, uint64_t pin)
{
    CommandPacket Pkt;

    Pkt.Data[PACKETLEN] = (char *) &Pkt.Data[5 + 1]-(char *) &Pkt.Data[0];
    Pkt.Data[PACKETTYPE] = INPUTMAP;
    Pkt.Data[2] = (unsigned short int) (pin & 0xFFFF);
    Pkt.Data[3] = (unsigned short int) ((pin >> 16)&0xFFFF);
    Pkt.Data[4] = (unsigned short int) (pin >> 32 & 0xFFFF);
    Pkt.Data[5] = (unsigned short int) ((pin >> 48)&0xFFFF);

    panel_command(h, &Pkt);
}

void sendoutputmap(panel_connection_t* h, uint64_t pin)
{
    CommandPacket Pkt;

    Pkt.Data[PACKETLEN] = (char *) &Pkt.Data[5 + 1]-(char *) &Pkt.Data[0];
    Pkt.Data[PACKETTYPE] = OUTPUTMAP;
    Pkt.Data[2] = (unsigned short int) (pin & 0xFFFF);
    Pkt.Data[3] = (unsigned short int) ((pin >> 16)&0xFFFF);
    Pkt.Data[4] = (unsigned short int) (pin >> 32 & 0xFFFF);
    Pkt.Data[5] = (unsigned short int) ((pin >> 48)&0xFFFF);

    panel_command(h, &Pkt);
}

