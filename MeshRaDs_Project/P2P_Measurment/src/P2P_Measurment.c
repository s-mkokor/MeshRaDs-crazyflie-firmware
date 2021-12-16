/**
 *      __  ___          __    ____        ____  _____
 *     /  |/  /__  _____/ /_  / __ \____ _/ __ \/ ___/
 *    / /|_/ / _ \/ ___/ __ \/ /_/ / __ `/ / / /\__ \ 
 *   / /  / /  __(__  ) / / / _, _/ /_/ / /_/ /___/ / 
 *  /_/  /_/\___/____/_/ /_/_/ |_|\__,_/_____//____/  
 *  
 *
 *  Copyright (C) 2021 MeshRaDs
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA  02110-1301, USA.
 *
 * P2P_Measurment.c - App layer application to make Perr to Peer RSSI measurments
 * 
 */


#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>


#include "app.h"

#include "FreeRTOS.h"
#include "task.h"

#include "radiolink.h"
#include "configblock.h"

#define DEBUG_MODULE "P2P"
#include "debug.h"

#include "estimator_kalman.h"
#include "stabilizer_types.h"

#define MAX_MESSAGE_LENGTH 23

point_t position;

void p2pcallbackHandler(P2PPacket *p)
{
  // Parse the data from the other crazyflie and print it
  uint8_t other_id = p->data[0];
  static char msg[MAX_MESSAGE_LENGTH + 1];
  memcpy(&msg, &p->data[1], sizeof(char)*MAX_MESSAGE_LENGTH);
  msg[MAX_MESSAGE_LENGTH] = 0;
  uint8_t rssi = p->rssi;

  uint64_t address = configblockGetRadioAddress();
  uint8_t my_id =(uint8_t)((address) & 0x00000000ff);

  DEBUG_PRINT("%d, %d, %d, %s \n", my_id, rssi, other_id, msg);
}

void appMain()
{
  DEBUG_PRINT("Waiting for activation ...\n");

    // Initialize the p2p packet 
    static P2PPacket p_reply;
    p_reply.port=0x00;
    
    // Get the current address of the crazyflie and obtain
    //   the last two digits and send it as the first byte
    //   of the payload
    uint64_t address = configblockGetRadioAddress();
    uint8_t my_id =(uint8_t)((address) & 0x00000000ff);
    p_reply.data[0]=my_id;

    //Put a string in the payload
    estimatorKalmanGetEstimatedPos(&position);    //Get X, Y, Z Position
    char x_pos[7];
    char y_pos[7];
    char z_pos[7];
   
    snprintf(x_pos, sizeof(x_pos), "%.3f", position.x);    
    snprintf(y_pos, sizeof(y_pos), "%.3f", position.y);
    snprintf(z_pos, sizeof(z_pos), "%.3f", position.z);

    char str[MAX_MESSAGE_LENGTH] = "";                            //Building message string str
    char help_str[4] = ", ";
    strcat(str, x_pos);
    strcat(str, help_str);
    strcat(str, y_pos);
    strcat(str, help_str);
    strcat(str, z_pos);

    int message_length = strlen(str);

    memcpy(&p_reply.data[1], &str, sizeof(char)*message_length);

    // Set the size, which is the amount of bytes the payload with ID and the string 
    p_reply.size=sizeof(char)*message_length+1;

    // Register the callback function so that the CF can receive packets as well.
    p2pRegisterCB(p2pcallbackHandler);

  while(1) {
    // Send a message every 2 seconds
    //   Note: if they are sending at the exact same time, there will be message collisions, 
    //    however since they are sending every 2 seconds, and they are not started up at the same
    //    time and their internal clocks are different, there is not really something to worry about

    vTaskDelay(M2T(2000));
    radiolinkSendP2PPacketBroadcast(&p_reply);
  }
}

