/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Allan Stockdill-Mander/Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/

#if defined(USES_MQTT) || (USES_MQTT_CLIENT)

#include "user_api.h"
#include "MQTTClient.h"

                                                // local function prototpypes
int  cycle (Client *c, Timer *timer);
int  decodePacket (Client *c, int *value, int timeout);
int  deliverMessage (Client *c, MQTTString *topicName, MQTTMessage *message);
int  getNextPacketId (Client *c);
char isTopicMatched (char *topicFilter, MQTTString *topicName);
int  keepalive (Client *c);
int  readPacket (Client *c, Timer *timer);
int  sendPacket (Client *c, int length, Timer* timer);
int  waitfor (Client *c, int packet_type, Timer *timer);
void NewMessageData (MessageData *md, MQTTString *aTopicName, MQTTMessage *aMessgage);


//**********************************************************************************
// MQTTClient
//
//          Initial call to the MQTT support.
//          Initializes the MQTT Client structure, including
//          setting Max send/rcv buffer sizes and wait timeouts
//**********************************************************************************
void  MQTTClient (Client *c,  Network *network,  unsigned int command_timeout_ms,
                  unsigned char *buf,     size_t buf_size,
                  unsigned char *readbuf, size_t readbuf_size)
{
    int i;

    c->ipstack = network;    // save pointer to our TCP network ctl blovk, that has
                             // the function pointers to TCP Send / Rcv / Disconnect.

    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
        c->messageHandlers[i].topicFilter = 0;

    c->command_timeout_ms = command_timeout_ms;      // Save Max wait parm

    c->buf      = buf;      // user supplied send buffer for TCP I/O of MQTT packets
    c->buf_size = buf_size;
    c->readbuf  = readbuf;  // user supplied rcv  buffer for TCP I/O of MQTT packets
    c->readbuf_size = readbuf_size;
    c->isconnected  = 0;
    c->ping_outstanding = 0;
    c->defaultMessageHandler = NULL;
    InitTimer (&c->ping_timer);
}


//**********************************************************************************
// MQTTConnect
//
//          At the MQTT level, connect to a Broker.
//          Generate a MQTT Connect packet and send it to the Broker,
//          then wait for the MQTT ConnAck reply.
//
//          This assumes that a TCP Connect to the MQTT Broker's server node has
//          already been completed.
//**********************************************************************************

int  MQTTConnect (Client *c, MQTTPacket_connectData *options)
{
    Timer     connect_timer;
    int       rc = FAILURE;
    int       len = 0;
    MQTTPacket_connectData  default_options = MQTTPacket_connectData_initializer;

    InitTimer (&connect_timer);
    countdown_ms (&connect_timer, c->command_timeout_ms);

    if (c->isconnected) // don't send connect packet again if we are already connected
        goto exit;

    if (options == 0)
       options = &default_options;  // set default options if none were supplied

    c->keepAliveInterval = options->keepAliveInterval;
    countdown (&c->ping_timer, c->keepAliveInterval);

       //--------------------------------------------------------------------
       // Generate a MQTT "Connect" packet, and send it to the remote Broker
       //--------------------------------------------------------------------
    len = MQTTSerialize_connect (c->buf, c->buf_size, options);
    if (len <= 0)
        goto exit;                              // supplied buffer is too small
    rc = sendPacket (c, len, &connect_timer);   // send the connect packet
    if (rc != SUCCESS)
        goto exit;                              // there was a problem

       //--------------------------------------------------------------------
       // Wait for and read in the MQTT "ConnAck" reply packet.
       //--------------------------------------------------------------------
        // this will be a blocking call, wait for the connack
    if (waitfor(c, CONNACK, &connect_timer) == CONNACK)
      {
        unsigned char connack_rc     = 255;
        char          sessionPresent = 0;
        if (MQTTDeserialize_connack((unsigned char*) &sessionPresent, &connack_rc,
                                    c->readbuf, c->readbuf_size) == 1)
            rc = connack_rc;
            else rc = FAILURE;
      }
     else rc = FAILURE;

exit:
    if (rc == SUCCESS)
       c->isconnected = 1;
    return rc;
}


int  MQTTDisconnect (Client *c)
{
    Timer  timer; // we might wait for incomplete incoming publishes to complete
    int    rc = FAILURE;
    int    len;

    len = MQTTSerialize_disconnect (c->buf, c->buf_size);

    InitTimer (&timer);
    countdown_ms (&timer, c->command_timeout_ms);

    if (len > 0)
        rc = sendPacket (c, len, &timer);         // send the disconnect packet

    c->isconnected = 0;
    return rc;
}


int  MQTTPublish (Client *c, const char *topicName, MQTTMessage *message)
{
    int         rc = FAILURE;
    int         len = 0;
    Timer       timer;
    MQTTString  topic = MQTTString_initializer;

    topic.cstring = (char*) topicName;

    InitTimer (&timer);
    countdown_ms (&timer, c->command_timeout_ms);

    if (!c->isconnected)
        goto exit;

    if (message->qos == QOS1 || message->qos == QOS2)
        message->id = getNextPacketId(c);

    len = MQTTSerialize_publish (c->buf, c->buf_size, 0, message->qos,
                                 message->retained, message->id,
                                 topic, (unsigned char*) message->payload,
                                 message->payloadlen);
    if (len <= 0)
        goto exit;
    if ((rc = sendPacket(c, len, &timer)) != SUCCESS) // send the subscribe packet
        goto exit;           // there was a problem

    if (message->qos == QOS1)
      {
        if (waitfor(c, PUBACK, &timer) == PUBACK)
          {
            unsigned short mypacketid;
            unsigned char  dup,  type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbuf_size) != 1)
                rc = FAILURE;
          }
         else rc = FAILURE;       // timed out - no PUBACK received
      }
     else if (message->qos == QOS2)
      {
        if (waitfor(c, PUBCOMP, &timer) == PUBCOMP)
          {
            unsigned short mypacketid;
            unsigned char  dup,  type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbuf_size) != 1)
                rc = FAILURE;
          }
         else rc = FAILURE;       // timed out - no PUBCOMP received
      }

exit:
    return rc;
}


int  MQTTSubscribe (Client *c, const char *topicFilter,  enum QoS qos,
                    messageHandler messageHandler)
{
    int         i;
    int         rc = FAILURE;
    Timer       timer;
    int         len = 0;
    MQTTString  topic = MQTTString_initializer;

    topic.cstring = (char*) topicFilter;

    InitTimer (&timer);
    countdown_ms (&timer, c->command_timeout_ms);   // default is 1 second timeouts

    if ( ! c->isconnected)
        goto exit;

    len = MQTTSerialize_subscribe (c->buf, c->buf_size, 0, getNextPacketId(c), 1,
                                   &topic, (int*) &qos);
    if (len <= 0)
        goto exit;
    if ((rc = sendPacket(c, len, &timer)) != SUCCESS)  // send the subscribe packet
        goto exit;             // there was a problem

    if (waitfor(c, SUBACK, &timer) == SUBACK)          // wait for suback
      {
        int count = 0, grantedQoS = -1;
        unsigned short mypacketid;
        if (MQTTDeserialize_suback(&mypacketid, 1, &count, &grantedQoS, c->readbuf, c->readbuf_size) == 1)
           rc = grantedQoS;       // will be 0, 1, 2 or 0x80
        if (rc != 0x80)
          {
            for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
              {
                if (c->messageHandlers[i].topicFilter == 0)
                  {
                    c->messageHandlers[i].topicFilter = topicFilter;
                    c->messageHandlers[i].fp = messageHandler;
                    rc = 0;    // denote success
                    break;
                  }
              }
          }
      }
     else rc = FAILURE;        // timed out - no SUBACK received

exit:
    return rc;
}


int  MQTTUnsubscribe (Client *c, const char *topicFilter)
{
    int         rc  = FAILURE;
    int         len = 0;
    Timer       timer;
    MQTTString  topic = MQTTString_initializer;

    topic.cstring = (char*) topicFilter;

    InitTimer (&timer);
    countdown_ms (&timer, c->command_timeout_ms);

    if (!c->isconnected)
        goto exit;

    if ((len = MQTTSerialize_unsubscribe(c->buf, c->buf_size, 0, getNextPacketId(c), 1, &topic)) <= 0)
        goto exit;
    if ((rc = sendPacket(c, len, &timer)) != SUCCESS) // send the subscribe packet
        goto exit; // there was a problem

    if (waitfor(c, UNSUBACK, &timer) == UNSUBACK)
      {
        unsigned short mypacketid;  // should be the same as the packetid above
        if (MQTTDeserialize_unsuback(&mypacketid, c->readbuf, c->readbuf_size) == 1)
            rc = 0;
      }
     else rc = FAILURE;      // timed out - no UNSUBACK received

exit:
    return rc;
}


int  MQTTYield (Client *c, int timeout_ms)
{
    int    rc = SUCCESS;
    Timer  timer;

    InitTimer (&timer);
    countdown_ms (&timer, timeout_ms);

    while ( ! expired(&timer))
      {
        if (cycle(c, &timer) == FAILURE)  // see if any MQTT packets were received
           {
              rc = FAILURE;               // no packets were received this pass
              break;
           }
      }

    return rc;
}


void  NewMessageData (MessageData *md, MQTTString *aTopicName, MQTTMessage *aMessgage)
{
    md->topicName = aTopicName;
    md->message   = aMessgage;
}


//*****************************************************************************
//*****************************************************************************
//                            UTILITY  ROUTINES
//*****************************************************************************
//*****************************************************************************

int  cycle (Client *c, Timer *timer)
{
    unsigned short  packet_type;
    int             len;
    int             rc;

        // read the socket, see what work is due
    packet_type = readPacket (c, timer);     // if no packet, returns FAILURE

    len = 0;
    rc  = SUCCESS;

    switch (packet_type)
      {
        case CONNACK:
        case PUBACK:
        case SUBACK:
            break;

        case PUBLISH:
          {
            MQTTString topicName;
            MQTTMessage msg;
            if (MQTTDeserialize_publish ((unsigned char*)&msg.dup, (int*)&msg.qos,
                                        (unsigned char*)&msg.retained, (unsigned short*)&msg.id, &topicName,
                                        (unsigned char**)&msg.payload, (int*)&msg.payloadlen, c->readbuf, c->readbuf_size) != 1)
                goto exit;

            deliverMessage (c, &topicName, &msg);

            if (msg.qos != QOS0)
              {
                if (msg.qos == QOS1)
                   len = MQTTSerialize_ack (c->buf, c->buf_size, PUBACK, 0, msg.id);
                   else if (msg.qos == QOS2)
                           len = MQTTSerialize_ack (c->buf, c->buf_size, PUBREC, 0, msg.id);
                if (len <= 0)
                   rc = FAILURE;
                   else rc = sendPacket (c, len, timer);    // send a PUBACK
                if (rc == FAILURE)
                   goto exit;               // there was a problem
              }
            break;
          }

        case PUBREC:
          {
            unsigned short mypacketid;
            unsigned char dup, type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbuf_size) != 1)
                rc = FAILURE;
            else if ((len = MQTTSerialize_ack(c->buf, c->buf_size, PUBREL, 0, mypacketid)) <= 0)
                rc = FAILURE;
            else if ((rc = sendPacket(c, len, timer)) != SUCCESS) // send the PUBREL packet
                rc = FAILURE; // there was a problem
            if (rc == FAILURE)
                goto exit; // there was a problem
            break;
          }

        case PUBCOMP:
            break;

        case PINGRESP:
            c->ping_outstanding = 0;
            break;
      }                                      //  end  switch()

    keepalive (c);

exit:
    if (rc == SUCCESS)
       rc = packet_type;
    return rc;
}


int  decodePacket (Client *c, int *value, int timeout)
{
    unsigned char  i;
    int            multiplier = 1;
    int            len = 0;
    const int      MAX_NO_OF_REMAINING_LENGTH_BYTES = 4;

    *value = 0;
    do
    {
        int rc = MQTTPACKET_READ_ERROR;

        if (++len > MAX_NO_OF_REMAINING_LENGTH_BYTES)
          {
            rc = MQTTPACKET_READ_ERROR; /* bad data */
            goto exit;
          }
            //--------------------------------------
            // Issue a TCP Recv to read in any data
            //--------------------------------------
        rc = c->ipstack->mqttread (c->ipstack, &i, 1, timeout);
        if (rc != 1)
            goto exit;
        *value += (i & 127) * multiplier;
        multiplier *= 128;
    } while ((i & 128) != 0);

exit:
    return len;
}


int  deliverMessage (Client *c, MQTTString *topicName, MQTTMessage *message)
{
    int i;
    int rc = FAILURE;

        // we have to find the right message handler - indexed by topic
    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
      {
        if (c->messageHandlers[i].topicFilter != 0 && (MQTTPacket_equals(topicName, (char*)c->messageHandlers[i].topicFilter)
          || isTopicMatched((char*)c->messageHandlers[i].topicFilter, topicName)))
          {
            if (c->messageHandlers[i].fp != NULL)
              {
                MessageData md;
                NewMessageData (&md, topicName, message);
                c->messageHandlers[i].fp (&md);
                rc = SUCCESS;
              }
          }
      }

    if (rc == FAILURE && c->defaultMessageHandler != NULL)
      {
        MessageData  md;
        NewMessageData (&md, topicName, message);
        c->defaultMessageHandler (&md);
        rc = SUCCESS;
      }

    return rc;
}


int  getNextPacketId (Client *c)
{
    return c->next_packetid = (c->next_packetid == MAX_PACKET_ID) ? 1 : c->next_packetid + 1;
}


// assume topic filter and name is in correct format
// # can only be at end
// + and # can only be next to separator
char  isTopicMatched (char *topicFilter, MQTTString *topicName)
{
    char* curf = topicFilter;
    char* curn = topicName->lenstring.data;
    char* curn_end = curn + topicName->lenstring.len;

    while (*curf && curn < curn_end)
      {
        if (*curn == '/' && *curf != '/')
            break;
        if (*curf != '+' && *curf != '#' && *curf != *curn)
            break;
        if (*curf == '+')
          {   // skip until we meet the next separator, or end of string
            char* nextpos = curn + 1;
            while (nextpos < curn_end && *nextpos != '/')
                nextpos = ++curn + 1;
          }
         else if (*curf == '#')
                 curn = curn_end - 1;    // skip until end of string
        curf++;
        curn++;
      };

    return (curn == curn_end) && (*curf == '\0');
}


int  keepalive (Client *c)
{
    int rc = FAILURE;

    if (c->keepAliveInterval == 0)
      {
        rc = SUCCESS;
        goto exit;
      }

    if (expired(&c->ping_timer))
      {
        if (!c->ping_outstanding)
          {
            Timer timer;
            InitTimer (&timer);
            countdown_ms (&timer, 1000);
            int len = MQTTSerialize_pingreq (c->buf, c->buf_size);
            if (len > 0 && (rc = sendPacket(c, len, &timer)) == SUCCESS) // send the ping packet
               c->ping_outstanding = 1;
          }
      }

exit:
    return rc;
}


int  readPacket (Client *c, Timer *timer)
{
    int         rc;
    MQTTHeader  header = {0};
    int         len    = 0;
    int         rem_len = 0;

    rc = FAILURE;      // default return code if no MQTT packet rcvd

        /* 1. read the header byte.  This has the packet type in it */
    if (c->ipstack->mqttread(c->ipstack, c->readbuf, 1, left_ms(timer)) != 1)
       goto exit;

       /* 2. read the remaining length.  This is variable in itself */
    len = 1;
    decodePacket (c, &rem_len, left_ms(timer));
    len += MQTTPacket_encode (c->readbuf + 1, rem_len); /* put the original remaining length back into the buffer */

       /* 3. read the rest of the buffer using a callback to supply the rest of the data */
    if (rem_len > 0 && (c->ipstack->mqttread(c->ipstack, c->readbuf + len, rem_len, left_ms(timer)) != rem_len))
       goto exit;

    header.byte = c->readbuf[0];
    rc = header.bits.type;

exit:
    return rc;
}


int  sendPacket (Client *c, int length, Timer* timer)
{
    int  rc   = FAILURE;
    int  sent = 0;

    while (sent < length && !expired(timer))
      {          //-------------------------------------
                 //  issue TCP send of the MQTT packet
                 //-------------------------------------
        rc = c->ipstack->mqttwrite (c->ipstack, &c->buf[sent],
                                    length, left_ms(timer));
        if (rc < 0)     // there was an error writing the data
            break;
        sent += rc;
      }
    if (sent == length)
      {
        countdown (&c->ping_timer, c->keepAliveInterval); // record the fact that we have successfully sent the packet
        rc = SUCCESS;
      }
    else
        rc = FAILURE;
    return rc;
}


// only used in single-threaded mode where one command at a time is in process
int  waitfor (Client *c, int packet_type, Timer *timer)
{
    int rc = FAILURE;

    do
      {
         if (expired(timer))
            break;              // we timed out - bail out
      } while ((rc = cycle(c, timer)) != packet_type);

    return rc;
}

#endif                                     // (USES_MQTT)
