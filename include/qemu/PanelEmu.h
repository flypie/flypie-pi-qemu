/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   PanelEmu.h
 * Author: John Bradley
 *
 * Created on 22 April 2017, 22:26
 */

#ifndef PANELEMU_H
#define PANELEMU_H

#ifdef __cplusplus
extern "C" {
#endif

    

#define DRIVER_NAME "RDC-GPIO: "
#define PANEL_NAME "GPIO panel: "
#define PANEL_PORT 0xb1ff       //45567



#define PANEL_PINS 64    
    
typedef struct panel_connection 
{
	int socket;    /* socket we'll connect to the panel with */
	fd_set fds;    /* list of descriptors (only the above socket */
	char last[PANEL_PINS/8]; /* we don't want to send updates to the panel
	                       unless something changed */
} panel_connection_t;

int panel_open(panel_connection_t* h);

int panel_read(panel_connection_t* h, uint64_t *Data);
void panel_write(panel_connection_t* h, uint64_t pin, char val);
void panel_send_read_command(panel_connection_t* h);



#ifdef __cplusplus
}
#endif

#endif /* PANELEMU_H */

