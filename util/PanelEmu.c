/*
 * Emulation for Rasp PI GPIO via Server connected to via Socket
 *
 */

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
    MACHINEDESC=0,
    PINSTATE=1,
    READREQ=2,
    READRESPONSE =3
} PacketType;

#define MAXPACKET   255

#define PACKETLEN   0  //Includes Packet Length
#define PACKETTYPE  1

typedef struct
{
    unsigned short int  Data[MAXPACKET];
} CommandPacket;
// Only deals with the first 'bank'.
typedef struct PCIGPIOState 
{
//    PCIDevice dev;
    panel_connection_t panel;   // connection to GPIO panel
    unsigned char cfg_state[0x100];
} PCIGPIOState;


/*
 * Hub connections
 *
 */

int panel_open(panel_connection_t* h)
{
	struct sockaddr_in remote;

#ifdef __MINGW32__
        WSADATA wsadata;
        if (WSAStartup(MAKEWORD(1,1), &wsadata) == SOCKET_ERROR) 
        {
            printf("Error creating socket.");
            return -1;
        }
#endif
        if ((h->socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
        {
		perror(PANEL_NAME "socket");
		return -1;
	}

	remote.sin_family = AF_INET;
	remote.sin_port = htons(PANEL_PORT);
	remote.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (connect(h->socket, (struct sockaddr *)&remote, sizeof(remote)) == -1) 
        {
		perror(PANEL_NAME "connection");
#ifdef __MINGW32__
                closesocket(h->socket);
#else
		close(h->socket);
#endif
		h->socket = -1;
		return -1;
	}

#ifdef __MINGW32__

        char value = 1;
        setsockopt( h->socket, IPPROTO_TCP, TCP_NODELAY, &value, sizeof( value ) );

#endif
	FD_ZERO(&h->fds);

	/* Set our connected socket */
	FD_SET(h->socket, &h->fds);	
	
	printf(PANEL_NAME "Connected OK\n");
	return 0;
}

static void panel_command(panel_connection_t *h,CommandPacket *Pkt)
{	
        if (send(h->socket, (char *)Pkt, Pkt->Data[PACKETLEN], 0) == -1) 
        {
		perror(PANEL_NAME "send");
#ifdef __MINGW32__
                closesocket(h->socket);
#else		
                close(h->socket);
#endif                
		h->socket = -1;  /* act like we never connected */
	}
}


/* Wait for values to be read back from panel */
static int panel_getpins(panel_connection_t* h, void* Return, int sizeof_return)
{
    fd_set rfds, efds;
    int t;
    int select_res = 0;
    int Status=-1;
    CommandPacket   Pkt;
    BOOL    NoError=TRUE;

    if (h->socket==-1)
    {
        NoError=FALSE;
    }
    else
    {
        // Wait for a response from peripheral, assume it arrives all together
        // (likely since it's only a few bytes long)

        rfds = h->fds;
        efds = h->fds;
    }

    while(NoError)
    {
        select_res = select(h->socket + 1, &rfds, NULL, &efds, NULL);
        if (select_res == -1)
        {
            if (errno == EINTR)
            {
            // try again, syscall
            }
            else
            {
                perror(PANEL_NAME "select");
                NoError=FALSE;
            }
        }
        else if (select_res == 0)
        {
            // timeout, wait a bit longer for data
        }
        else
        {
            // Should be one or more descriptors signalled.
            break;
        }
    }

    if (FD_ISSET(h->socket, &rfds)) 
    {
        /* receive more data */
        if ((t = recv(h->socket, (char *)&Pkt, sizeof(Pkt), 0)) > 0)
        {
            if (Pkt.Data[PACKETTYPE]==READRESPONSE) 
            {
//                  strncpy(status, &str[2],slen-1);
//                  /* ensure termination */
//                  status[slen-1] = '\0';
//                Status=1;
            }
            else
            {
                printf(PANEL_NAME "Invalid data received\n");
            }
        }
        else
        {
            if (t < 0)
            {
                perror("recv");
            }
            else 
            {
                printf(PANEL_NAME "closed connection\n");
            }
            NoError=FALSE;
        }
    }

    if (FD_ISSET(h->socket, &efds)) 
    {
            /* error on this socket */
        printf(PANEL_NAME "closed connection\n");
        NoError=FALSE;
    }

    if(!NoError && h->socket!=-1)
    {
#ifdef __MINGW32__
        closesocket(h->socket);
#else		
        close(h->socket);
#endif  
        h->socket = -1;  /* act like we never connected */           

        Status=0;

    }
    else if(h->socket==-1)
    {
        Status=0;
    }
    else
    {
        Status=1;
    }
    return Status;
}


/* Set a pin to a specified value */
void panel_send_read_command(panel_connection_t* h)
{
        CommandPacket Pkt;
        
        Pkt.Data[PACKETLEN]=4;
        Pkt.Data[PACKETTYPE]=READREQ;
        
	panel_command(h, &Pkt);
}


/* Send a read request */
int panel_read(panel_connection_t* h, void* status, size_t slen)
{
	int len=0;
        CommandPacket Pkt;

        panel_send_read_command(h);

	while (!len)
        {
            len = panel_getpins(h, &Pkt, sizeof(Pkt));
        }
	return len;
}


/* Set a pin to a specified value */
void panel_write(panel_connection_t* h, int pin, char val)
{
        CommandPacket Pkt;
        
        Pkt.Data[PACKETLEN]=(char *)&Pkt.Data[3+1]-(char *)&Pkt.Data[0];
        Pkt.Data[PACKETTYPE]=PINSTATE;
        Pkt.Data[2]=pin;
        Pkt.Data[3]=val;

        
	panel_command(h, &Pkt);
}



//static void panel_close(panel_connection_t* h)
//{
//	close(h->socket);
//}


