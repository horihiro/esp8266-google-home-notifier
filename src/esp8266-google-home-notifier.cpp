#include <esp8266-google-home-notifier.h>

char data[1024];

#ifdef MDNS_H
extern "C" {
    #include "osapi.h"
    #include "ets_sys.h"
    #include "user_interface.h"
}

char txtRecordSeparator[64]="#";

struct TXTRecord {
  char name[128] = "";
  char value[128] = "";
  TXTRecord *_next = NULL;
};

struct GoogleHomeService {
  char serviceName[256] = "";
  char hostName[256] = "";
  char txtRecordsStr[256] = "";
  uint16_t port = 0;
  uint8_t ip[4] = {0};
  TXTRecord *txtRecords = NULL;  
  GoogleHomeService *_next = NULL;
};

#define QUESTION_SERVICE "_googlecast._tcp.local"
GoogleHomeService* m_services = NULL;
#define MAX_MDNS_PACKET_SIZE 512

void answerCallback(const mdns::Answer* answer){
  int i = 0;
  answer->Display();
  GoogleHomeService* service = m_services;
  GoogleHomeService* prev = NULL;
  if (answer->rrtype == MDNS_TYPE_PTR) {
    // rdata_buffer is serviceName
    while (service != NULL) {
      if (strcmp(service->serviceName, answer->rdata_buffer) == 0) {
        // 
        break;
      }
      prev = service;
      service = prev->_next;
    }
    if (service == NULL) {
      GoogleHomeService* tmp = (GoogleHomeService*)os_malloc(sizeof(struct GoogleHomeService));
      os_strcpy(tmp->serviceName, answer->rdata_buffer);
      if (prev == NULL) {
        m_services = tmp;
      } else {
        prev->_next = tmp;
      }
    }
  } else if (answer->rrtype == MDNS_TYPE_TXT) {
    // rdata_buffer is TXT record
    while (service != NULL) {
      if (strcmp(service->serviceName, answer->name_buffer) == 0) {
        break;
      }
      prev = service;
      service = prev->_next;
    }
    if (service != NULL) {

      i = 0;
      char* txt;
      char* pos;
      TXTRecord* currentTxtRecord = service->txtRecords = (TXTRecord*)os_malloc(sizeof(struct TXTRecord));
      TXTRecord* prevTxtRecord;
      do {
        txt = strtok(i == 0 ? (char*)answer->rdata_buffer : NULL, (const char*)txtRecordSeparator);
        if (txt == NULL) {
          os_free(currentTxtRecord);
          prevTxtRecord->_next = NULL;
          break;
        }

        currentTxtRecord->_next = (TXTRecord*)os_malloc(sizeof(struct TXTRecord));

        pos = strchr(txt, '=');
        *pos = '\0';

        strcpy(currentTxtRecord->name, txt);
        strcpy(currentTxtRecord->value, pos + 1);

        prevTxtRecord = currentTxtRecord;
        currentTxtRecord = prevTxtRecord->_next;

        i++;
      } while (true);

    }

  } else if (answer->rrtype == MDNS_TYPE_SRV) {
    // rdata_buffer is hostname, port
    while (service != NULL) {
      if (strcmp(service->serviceName, answer->name_buffer) == 0) {
        break;
      }
      prev = service;
      service = prev->_next;
    }
    if (service != NULL) {
      char* hostCh = strstr(answer->rdata_buffer, ";host=");
      char* portCh = strstr(answer->rdata_buffer, ";port=");
      strcpy(service->hostName, hostCh + 6);
      char portStr[6] = "";
      strncpy(portStr, portCh + 6, hostCh - portCh - 6);
      service->port = atoi(portStr);
    }

  } else if (answer->rrtype == MDNS_TYPE_A) {
    // rdata_buffer is IP address
    unsigned char ip[4];
    char* ipaddressStr = (char*)answer->rdata_buffer;
    char* iprange;
    i = 0;
    do {
      iprange = strtok(i == 0 ? ipaddressStr : NULL, ".");
      if (iprange == NULL) break;
      ip[i] = (uint8_t)atoi(iprange);
      i++;
    } while (i < 4);
    while (service != NULL) {
      if (strcmp(service->hostName, answer->name_buffer) == 0) {
        for (i=0;i<4;i++) {
          service->ip[i] = ip[i];
        }
      }
      prev = service;
      service = prev->_next;
    }
    Serial.println("========================");
    if (prev->serviceName != NULL) {
      Serial.print("  serv:");
      Serial.println(prev->serviceName);
    }
    if (prev->txtRecords != NULL) {
      TXTRecord* txt = prev->txtRecords;
      Serial.println("  txt:");
      int j = 0;
      while (txt != NULL) {
        Serial.print("    ");
        Serial.println(j);
        Serial.print("      n:");
        Serial.println(txt->name);
        Serial.print("      v:");
        Serial.println(txt->value);
        txt = txt->_next;
        j++;
      }
    }
    if (prev->hostName != NULL) {
      Serial.print("  host:");
      Serial.println(prev->hostName);
    }
    if (prev->port > 0) {
      Serial.print("  port:");
      Serial.println(prev->port);
    }
    Serial.print("  iadr:");
    Serial.print(prev->ip[0]);
    Serial.print(".");
    Serial.print(prev->ip[1]);
    Serial.print(".");
    Serial.print(prev->ip[2]);
    Serial.print(".");
    Serial.println(prev->ip[3]);
    Serial.println("========================");
  }

  // GoogleHomeService* svc = m_services;

  // i = 0;
  // while (svc != NULL) {
  //   if (svc->ip[0] != 0 || svc->ip[1] != 0 || svc->ip[2] != 0 || svc->ip[3] != 0) {
  //     Serial.println("========================");
  //     if (svc->serviceName != NULL) {
  //       Serial.print("  serv:");
  //       Serial.println(svc->serviceName);
  //     }
  //     if (svc->txtRecords != NULL) {
  //       TXTRecord* txt = svc->txtRecords;
  //       Serial.println("  txt:");
  //       int j = 0;
  //       while (txt != NULL) {
  //         Serial.print("    ");
  //         Serial.println(j);
  //         Serial.print("      n:");
  //         Serial.println(txt->name);
  //         Serial.print("      v:");
  //         Serial.println(txt->value);
  //         txt = txt->_next;
  //         j++;
  //       }
  //     }
  //     if (svc->hostName != NULL) {
  //       Serial.print("  host:");
  //       Serial.println(svc->hostName);
  //     }
  //     if (svc->port > 0) {
  //       Serial.print("  port:");
  //       Serial.println(svc->port);
  //     }
  //     Serial.print("  iadr:");
  //     Serial.print(svc->ip[0]);
  //     Serial.print(".");
  //     Serial.print(svc->ip[1]);
  //     Serial.print(".");
  //     Serial.print(svc->ip[2]);
  //     Serial.print(".");
  //     Serial.println(svc->ip[3]);
  //     Serial.println("========================");
  //   }
  //   svc = svc->_next;
  //   i++; 
  // }
  // Serial.println();
  // answer->Display();
  // Serial.println();
}

byte buffer[MAX_MDNS_PACKET_SIZE];
mdns::MDns my_mdns(NULL, NULL, answerCallback, buffer, MAX_MDNS_PACKET_SIZE);
#endif

boolean GoogleHomeNotifier::device(const char * name)
{
  return this->device(name, "en");
}

boolean GoogleHomeNotifier::device(const char * name, const char * locale)
{
  int timeout = millis() + 5000;
  int n;
  char hostString[20];
  sprintf(hostString, "ESP_%06X", ESP.getChipId());

#ifdef MDNS_H
  for (n = 1; n < 32; n++) {
    txtRecordSeparator[n] = n;
  }
  txtRecordSeparator[32] = '\'';

  struct mdns::Query query_mqtt;
  strncpy(query_mqtt.qname_buffer, QUESTION_SERVICE, MAX_MDNS_NAME_LEN);
  query_mqtt.qtype = MDNS_TYPE_PTR;
  query_mqtt.qclass = 1;    // "INternet"
  query_mqtt.unicast_response = 0;
  query_mqtt.Display();
  my_mdns.AddQuery(query_mqtt);

  my_mdns.Send();
  while(true) {
    my_mdns.loop();
  }
#endif

#ifdef ESP8266MDNS_H
  int i;
  if (!MDNS.begin(hostString)) {
    this->setLastError("Failed to set up MDNS responder.");
    return false;
  }
  do {
    n = MDNS.queryService("googlecast", "tcp");
    if (millis() > timeout) {
      this->setLastError("mDNS timeout.");
      return false;
    }
    delay(10);
    for(i = 0; i < n; i++) {
      if (strcmp(name, MDNS.txt(i, "fn").c_str()) == 0) {
        break;
      }
    }
  } while (n <= 0 || i >= n);

  this->m_ipaddress = MDNS.IP(i);
  this->m_port = MDNS.port(i);
  sprintf(this->m_name, "%s", name);
  sprintf(this->m_locale, "%s", locale);
#endif
  return true;
}

boolean GoogleHomeNotifier::notify(const char * phrase)
{
  Serial.println(phrase);
  char error[128];
  String speechUrl;
  int n = 0;

  speechUrl = tts.getSpeechUrl(phrase, m_locale);
  n++;
  delay(100);

  Serial.println(n);
  if (speechUrl.indexOf("https://") != 0) {
    this->setLastError("Failed to get TTS url.");
    return false;
  }
  Serial.println(speechUrl);

  delay(100);
  if (!m_client.connect(this->m_ipaddress, this->m_port)) {
    sprintf(error, "Failed to Connect to %d.%d.%d.%d:%d.", this->m_ipaddress[0], this->m_ipaddress[1], this->m_ipaddress[2], this->m_ipaddress[3], this->m_port);
    this->setLastError(error);
    return false;
  }
  
  delay(100);
  if( this->connect() != true) {
    sprintf(error, "Failed to Open-Session. (%s)", this->getLastError());
    this->setLastError(error);
    disconnect();
    return false;
  }
   
  delay(100);
  if( this->play(speechUrl.c_str()) != true) {
    sprintf(error, "Failed to play mp3 file. (%s)", this->getLastError());
    this->setLastError(error);
    disconnect();
    return false;
  }

  disconnect();
  return true;
}

const IPAddress GoogleHomeNotifier::getIPAddress()
{
  return m_ipaddress;
}

const uint16_t GoogleHomeNotifier::getPort()
{
  return m_port;
}

boolean GoogleHomeNotifier::sendMessage(const char* sourceId, const char* destinationId, const char* ns, const char* data)
{
  extensions_api_cast_channel_CastMessage message = extensions_api_cast_channel_CastMessage_init_default;

  message.protocol_version = extensions_api_cast_channel_CastMessage_ProtocolVersion_CASTV2_1_0;
  message.source_id.funcs.encode = &(GoogleHomeNotifier::encode_string);
  message.source_id.arg = (void*)sourceId;
  message.destination_id.funcs.encode = &(GoogleHomeNotifier::encode_string);
  message.destination_id.arg = (void*)destinationId;
  message.namespace_str.funcs.encode = &(GoogleHomeNotifier::encode_string);
  message.namespace_str.arg = (void*)ns;
  message.payload_type = extensions_api_cast_channel_CastMessage_PayloadType_STRING;
  message.payload_utf8.funcs.encode = &(GoogleHomeNotifier::encode_string);
  message.payload_utf8.arg = (void*)data;

  uint8_t* buf = NULL;
  uint32_t bufferSize = 0;
  uint8_t packetSize[4];
  boolean status;

  pb_ostream_t  stream;
  
  do
  {
    if (buf) {
      delete buf;
    }
    bufferSize += 1024;
    buf = new uint8_t[bufferSize];

    stream = pb_ostream_from_buffer(buf, bufferSize);
    status = pb_encode(&stream, extensions_api_cast_channel_CastMessage_fields, &message);
  } while(status == false && bufferSize < 10240);
  if (status == false) {
    char error[128];
    sprintf(error, "Failed to encode. (source_id=%s, destination_id=%s, namespace=%s, data=%s)", sourceId, destinationId, ns, data);
    this->setLastError(error);
    return false;
  }

  bufferSize = stream.bytes_written;
  for(int i=0;i<4;i++) {
    packetSize[3-i] = (bufferSize >> 8*i) & 0x000000FF;
  }
  Serial.print(bufferSize);
  Serial.print(" ");
  Serial.println(data);
  m_client.write(packetSize, 4);
  m_client.write(buf, bufferSize);
  m_client.flush();

  delay(100);
  delete buf;
  return true;
}

boolean GoogleHomeNotifier::connect()
{
  // send 'CONNECT'
  if (this->sendMessage(SOURCE_ID, DESTINATION_ID, CASTV2_NS_CONNECTION, CASTV2_DATA_CONNECT) != true) {
    this->setLastError("'CONNECT' message encoding");
    return false;
  }
  delay(100);

  // send 'PING'
  if (this->sendMessage(SOURCE_ID, DESTINATION_ID, CASTV2_NS_HEARTBEAT, CASTV2_DATA_PING) != true) {
    this->setLastError("'PING' message encoding");
    return false;
  }
  delay(100);

  // send 'LAUNCH'
  sprintf(data, CASTV2_DATA_LAUNCH, APP_ID);
  if (this->sendMessage(SOURCE_ID, DESTINATION_ID, CASTV2_NS_RECEIVER, data) != true) {
    this->setLastError("'LAUNCH' message encoding");
    return false;
  }
  delay(100);

  // waiting for 'PONG' and Transportid
  int timeout = (int)millis() + 5000;
  while (m_client.available() == 0) {
    if (timeout < millis()) {
      this->setLastError("Listening timeout");
      return false;
    }
  }
  timeout = (int)millis() + 5000;
  extensions_api_cast_channel_CastMessage imsg;
  pb_istream_t istream;
  uint8_t pcktSize[4];
  uint8_t buffer[1024];

  uint32_t message_length;
  while(true) {
    delay(500);
    if (millis() > timeout) {
      this->setLastError("Incoming message decoding");
      return false;
    }
    // read message from Google Home
    m_client.read(pcktSize, 4);
    message_length = 0;
    for(int i=0;i<4;i++) {
      message_length |= pcktSize[i] << 8*(3 - i);
    }
    Serial.print(message_length);
    Serial.print(" ");
    m_client.read(buffer, message_length);
    istream = pb_istream_from_buffer(buffer, message_length);

    imsg.source_id.funcs.decode = &(GoogleHomeNotifier::decode_string);
    imsg.source_id.arg = (void*)"sid";
    imsg.destination_id.funcs.decode = &(GoogleHomeNotifier::decode_string);
    imsg.destination_id.arg = (void*)"did";
    imsg.namespace_str.funcs.decode = &(GoogleHomeNotifier::decode_string);
    imsg.namespace_str.arg = (void*)"ns";
    imsg.payload_utf8.funcs.decode = &(GoogleHomeNotifier::decode_string);
    imsg.payload_utf8.arg = (void*)"body";
    /* Fill in the lucky number */

    if (pb_decode(&istream, extensions_api_cast_channel_CastMessage_fields, &imsg) != true){
      this->setLastError("Incoming message decoding");
      return false;
    }
    String json = String((char*)imsg.payload_utf8.arg);
    Serial.println(json);
    int pos = -1;

    // if the incoming message has the transportId, then break;
    if (json.indexOf(String("\"appId\":\"") + APP_ID + "\"") >= 0 &&
        json.indexOf("\"statusText\":\"Ready To Cast\"") >= 0 && 
        (pos = json.indexOf("\"transportId\":")) >= 0
        ) {
      sprintf(this->m_transportid, "%s", json.substring(pos + 15, pos + 51).c_str());
      break;
    }
  }
  sprintf(this->m_clientid, "client-%d", millis());
  return true;
}

boolean GoogleHomeNotifier::play(const char * mp3url)
{
  // send 'CONNECT' again
  sprintf(data, CASTV2_DATA_CONNECT);
  if (this->sendMessage(this->m_clientid, this->m_transportid, CASTV2_NS_CONNECTION, CASTV2_DATA_CONNECT) != true) {
    this->setLastError("'CONNECT' message encoding");
    return false;
  }
  delay(100);

  // send URL of mp3
  sprintf(data, CASTV2_DATA_LOAD, mp3url);
  if (this->sendMessage(this->m_clientid, this->m_transportid, CASTV2_NS_MEDIA, data) != true) {
    this->setLastError("'LOAD' message encoding");
    return false;
  }
  delay(100);
  this->setLastError("");
  return true;
}

void GoogleHomeNotifier::disconnect() {
  m_client.stop();
}

bool GoogleHomeNotifier::encode_string(pb_ostream_t *stream, const pb_field_t *field, void * const *arg)
{
  char *str = (char*) *arg;

  if (!pb_encode_tag_for_field(stream, field))
    return false;

  return pb_encode_string(stream, (uint8_t*)str, strlen(str));
}

bool GoogleHomeNotifier::decode_string(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
  uint8_t buffer[1024] = {0};

  /* We could read block-by-block to avoid the large buffer... */
  if (stream->bytes_left > sizeof(buffer) - 1)
    return false;

  if (!pb_read(stream, buffer, stream->bytes_left))
    return false;

  /* Print the string, in format comparable with protoc --decode.
    * Format comes from the arg defined in main().
    */
  *arg = (void***)buffer;
  return true;
}

const char * GoogleHomeNotifier::getLastError() {
  return m_lastError;
}

void GoogleHomeNotifier::setLastError(const char* lastError) {
  sprintf(m_lastError, "%s", lastError);
}
