/* Router guidelines
 *  - each call to an interface write function should send a valid LocoNet message and a reqID parameter
 *  - the reqID value should be a unique number from 0 to 16383 (lower 14 bits of reqID
 *  - the reqID msb bits are used as routing table and are set by the router for these gateway routing cases
 *  rtc0: originally received on LocoNet Interface, routed to application and MQTT Broker. reqID of such message is 0 or reply reqID
 *  rtc1: originally received on MQTT interface ("From" is not us, echo flag cleared), routed to LocoNet, and echo of it to application and as echoTopic to MQTT
 *  rtc2: when echo flag is set when receiving from LocoNet: outgoing message from application, routed to LocoNet, and echo of it to application and MQTT
 *        when echo flag is cleared when receiving from LocoNet: reply to earlier outgoing message from application, routed to application and MQTT
 */
#define rtc0 0x0000
#define rtc1 0x4000
#define rtc2 0x8000
#define rtc3 0xC000

#include <IoTT_Gateway.h>

cbFct appCallback = NULL;
LocoNetESPSerial * serialPort = NULL;
MQTTESP32 * mqttPort = NULL;
cmdSourceType workMode = GW;


ln_mqttGateway::ln_mqttGateway()
{
}

ln_mqttGateway::ln_mqttGateway(LocoNetESPSerial * newLNPort, MQTTESP32 * newMQTTPort, cbFct newCB)
{
	setSerialPort(newLNPort);
	setMQTTPort(newMQTTPort);
	setAppCallback(newCB);
}

ln_mqttGateway::~ln_mqttGateway()
{
}

void ln_mqttGateway::setSerialPort(LocoNetESPSerial * newPort)
{
	serialPort = newPort;
	serialPort->setLNCallback(&onLocoNetMessage);
	
}

void ln_mqttGateway::setMQTTPort(MQTTESP32 * newPort)
{
	mqttPort = newPort;
	mqttPort->setMQTTCallback(&onMQTTMessage);
}

void ln_mqttGateway::setAppCallback(cbFct newCB)
{
	appCallback = newCB;
}

void ln_mqttGateway::setCommMode(cmdSourceType newMode)
{
	workMode = newMode;
}

void ln_mqttGateway::onLocoNetMessage(lnReceiveBuffer * newData) //this is the callback function for the LocoNet library
{
  switch (workMode)
  {
    case LN: //everything that shows up here gets routed to the application
      newData->reqID &= ~rtc3; //clear routing flags
      if (appCallback != NULL)
		appCallback(newData);
      break;
    case GW:
      switch ((newData->reqID & rtc3) >> 14)
      {
        case 0: //new message from LocoNet
		  if (appCallback != NULL)
			appCallback(newData);
          mqttPort->lnWriteMsg(*newData);        
          break;    
        case 1: //originally received from MQTT, than transmitted on LocoNet, now going to the application  
          newData->reqID &= (~rtc3); //clear routing flags
          if ((newData->errorFlags & msgEcho) == 0) //this is a reply message to a request originating from MQTT
            mqttPort->lnWriteMsg(*newData); // then send to MQTT
          newData->errorFlags &= ~msgEcho; //clear echo flag as this was an original incoming message
		  if (appCallback != NULL)
			appCallback(newData);
          break;    
        case 2: //sent by the application to Loconet, now the echo goes to MQTT. Also: could be a reply to an earlier message. In this case, send to application as well
          newData->reqID &= (~rtc3); //clear routing flags
		  if (appCallback != NULL)
			appCallback(newData);
          newData->errorFlags &= ~msgEcho; //clear echo flag, 
		  mqttPort->lnWriteMsg(*newData); // then send to MQTT
          break;    
      }
  }
}

void ln_mqttGateway::onMQTTMessage(lnReceiveBuffer * newData) //this is the callback function for the MQTT library
{
  if ((newData->errorFlags & msgEcho) > 0)
  {
    //this is an echo returned from broker. Use only for confirmation that earlier message was successfully sent
  }
  else //this is a new incoming message in the MQTT interface, case rtc1
    switch (workMode)
    {
      case MQTT:
        newData->reqID &= ~rtc3; //clear all router flags
  	    if (appCallback != NULL)
			appCallback(newData);
        break;
      case GW:
        newData->reqID = (newData->reqID & 0x3FFF) | rtc1; //set router case 1
		serialPort->lnWriteMsg(*newData);
        break;  
      default: break; //case LN not possible here
  }
}

uint16_t ln_mqttGateway::lnWriteMsg(lnTransmitMsg txData)
{
	if (txData.reqID == 0)
		txData.reqID = random(0x3FFF);
    txData.reqID |= rtc2; //set use case flags for outgoing message
    if (serialPort != NULL)
		return serialPort->lnWriteMsg(txData);
	else
		return 0;
}

uint16_t ln_mqttGateway::lnWriteMsg(lnReceiveBuffer txData)
{
	if (txData.reqID == 0)
		txData.reqID = random(0x3FFF);
    txData.reqID |= rtc2; //set use case flags for outgoing message
    if (serialPort != NULL)
		return serialPort->lnWriteMsg(txData);
	else
		return 0;
}

void ln_mqttGateway::processLoop()
{
    if (serialPort != NULL)
		serialPort->processLoop();
    if (mqttPort != NULL)
		mqttPort->processLoop();
}
