/*

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#ifndef IoTT_Gateway_h
#define IoTT_Gateway_h

#include <arduino.h>
#include <inttypes.h>
#include <IoTTCommDef.h>
#include <IoTT_LocoNetHBESP32.h>
#include <IoTT_MQTTESP32.h>

enum cmdSourceType : uint8_t {LN=0, MQTT=1, GW=2};

class ln_mqttGateway
{
public:
	ln_mqttGateway();
	ln_mqttGateway(LocoNetESPSerial * newLNPort, MQTTESP32 * newMQTTPort, cbFct newCB);
	~ln_mqttGateway();
	void processLoop();
	uint16_t lnWriteMsg(lnTransmitMsg txData);
	uint16_t lnWriteMsg(lnReceiveBuffer txData);
	void setSerialPort(LocoNetESPSerial * newPort);
	void setMQTTPort(MQTTESP32 * newPort);
	void setAppCallback(cbFct newCB);
	void setCommMode(cmdSourceType newMode);
	
private:
	static void onLocoNetMessage(lnReceiveBuffer * newData); //this is the callback function for the LocoNet library
	static void onMQTTMessage(lnReceiveBuffer * newData); //this is the callback function for the MQTT library

};
   



#endif
