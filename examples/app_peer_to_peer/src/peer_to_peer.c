/**
 * ,---------,       ____  _ __
 * |  ,-^-,  |      / __ )(_) /_______________ _____  ___
 * | (  O  ) |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * | / ,--´  |    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *    +------`   /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * Crazyflie control firmware
 *
 * Copyright (C) 2019 Bitcraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * peer_to_peer.c - App layer application of simple demonstartion peer to peer
 *  communication. Two crazyflies need this program in order to send and receive.
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

#define MESSAGE "hello world"
#define MESSAGE_LENGHT 11

// own includes
#include "estimator_kalman.h"
#include "stabilizer_types.h"

point_t my_data; 
struct payload {
  float x;
  float y;
  float z;
} payload;

void p2pcallbackHandler(P2PPacket *p)
{
  // Parse the data from the other crazyflie and print it
  uint8_t other_id = p->data[0];
  static struct payload msg;
  memcpy(&msg,&p->data[1],sizeof(msg));
  //static char msg[MESSAGE_LENGHT + 1];
  //memcpy(&msg, &p->data[1], sizeof(char)*MESSAGE_LENGHT);
  //msg[MESSAGE_LENGHT] = 0;
  uint8_t rssi = p->rssi;

  DEBUG_PRINT("[RSSI: -%d dBm] Message from CF nr. %d, %f , %f, %f  \n", rssi, other_id, (double)msg.x ,(double) msg.y , (double) msg.z);
}

void appMain()
{
  DEBUG_PRINT("Waiting for activation ...\n");
DEBUG_PRINT("X: %.3f, Y: %.3f, Z: %.3f\n", (double) my_data.x,(double) my_data.y, (double) my_data.z);
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
    //char *str="Hello World";
    

    

    // Register the callback function so that the CF can receive packets as well.
    p2pRegisterCB(p2pcallbackHandler);

  while(1) {
    // Send a message every 2 seconds
    //   Note: if they are sending at the exact same time, there will be message collisions, 
    //    however since they are sending every 2 seconds, and they are not started up at the same
    //    time and their internal clocks are different, there is not really something to worry about
    vTaskDelay(M2T(2000));
estimatorKalmanGetEstimatedPos(&my_data);
payload.x = my_data.x;
payload.y = my_data.y;
payload.z = my_data.z;
memcpy(&p_reply.data[1], &payload, sizeof(payload));
// Set the size, which is the amount of bytes the payload with ID and the string 
    p_reply.size=sizeof(payload)+1; 
    radiolinkSendP2PPacketBroadcast(&p_reply);
    DEBUG_PRINT("X: %f,Y: %f , Z: %f\n", (double) payload.x,(double) payload.y,(double) payload.z);
  }
}

