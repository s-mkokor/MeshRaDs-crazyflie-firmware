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
 *                    Peer to Peer messages that contain the current position and timestamp of the drone
 * Note: if they are sending at the exact same time, there will be message collisions, 
 *       however since they are sending every 2 seconds, and they are not started up at the same
 *       time and their internal clocks are different, there is not really something to worry about
 */


#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "app.h"

#include "FreeRTOS.h"
#include "task.h"

#include "radiolink.h"
#include "configblock.h"

#define DEBUG_MODULE "P2P"
#include "debug.h"

#include "estimator_kalman.h"
#include "stabilizer_types.h"

point_t payload;

void p2pcallbackHandler(P2PPacket *p)
{
  // Parse the data from the other crazyflie and print it
  uint8_t other_id = p->data[0];
  static point_t msg;
  memcpy(&msg,&p->data[1],sizeof(msg));
  uint8_t rssi = p->rssi;

  uint64_t address = configblockGetRadioAddress();
  uint8_t my_id =(uint8_t)((address) & 0x00000000ff);

  DEBUG_PRINT("%d, %d, %lu, %f, %f, %f, %d\n", my_id, rssi, msg.timestamp, (double)msg.x, (double)msg.y, (double)msg.z, other_id);
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

    // Register the callback function so that the CF can receive packets as well.
    p2pRegisterCB(p2pcallbackHandler);

  while(1) {
    // Send a message every 0.25 seconds
    vTaskDelay(M2T(250));

    // Generating  the Payload
    estimatorKalmanGetEstimatedPos(&payload);
    payload.timestamp = (uint32_t)(usecTimestamp()/1000);

    // Filling the message package
    memcpy(&p_reply.data[1], &payload, sizeof(payload));
    p_reply.size=sizeof(payload)+1; 

    // Sending the message
    radiolinkSendP2PPacketBroadcast(&p_reply);
  }
}

