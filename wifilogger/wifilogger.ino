#include <SPI.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <Wire.h>
#include <BMA222.h>

#include "local_defs.h"
#include "url_decode.h"

static char setup_page[] = "<html><head>CC3200 logger for embedded.com blog</head>\n"
"<body><H1>Welcome to cc3200 web logger</H1>\n"
"<H2>Please enter network credentials:</H2>\n"
"<form action=\"s.cgi\" method=\"get\">SSID:<br>\n"
"<input type=\"text\" name=\"ssid\"> <br>\n"
"Password:<br>\n"
"<input type=\"password\" name=\"pwd\"> <br><br>\n"
"<input type=\"hidden\" name=\"e\" value=\"e\">\n"
"<input type=\"submit\" value=\"OK\">\n"
"</form></body></html>\n";

unsigned char file_name[] = "creds.bin";
credentials_t credentials;
char temp_pass[PASS_LEN] = {0};
char temp_ssid[SSID_LEN] = {0};
token_state_t state = SEEN_NOTHING;
char token_len = 0;
WiFiServer server(80);

BMA222 accelerometer;

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("Network Name: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

// simplest form of checksum - rotating byte
char cals_cs(char* pData, int len)
{
  char cs = 0;
  for(int i = 0; i < len ; i++) cs+=pData[i];
  return cs; 
}

void setup()
{
  Serial.begin(115200);
  WiFiClass::init();
  long cred_file;
  if(sl_FsOpen(file_name,FS_MODE_OPEN_CREATE(98,_FS_FILE_OPEN_FLAG_COMMIT|_FS_FILE_PUBLIC_WRITE) , NULL,&cred_file))
  {
    credentials.is_provisioned = 0;
    Serial.println("can't find credentials, assuming network unprovisioned");
  }
  else //go ahead and read stored info
  {
    long size_read = sl_FsRead(cred_file, 0, (unsigned char*)&credentials, sizeof(credentials_t));
    if(size_read != sizeof(credentials_t) ||credentials.cs != cals_cs((char*)&credentials, size_read-1))
    {
       Serial.println("can't load credentials, assuming network unprovisioned");
       credentials.is_provisioned = 0;
    }
    sl_FsClose(cred_file, 0,0,0);
  }  
  
  // start web server and ap if there are no wifi credentials
  if(!credentials.is_provisioned)
  {
    Serial.println("Not provisioned");
    pinMode(RED_LED, OUTPUT);
    unsigned char  val = SL_SEC_TYPE_OPEN;
    sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SECURITY_TYPE, 1, (unsigned char *)&val);    
    WiFi.beginNetwork("CC3200Logger");
    Serial.println("Starting server");
    while (WiFi.localIP() == INADDR_NONE) 
    {
      // print dots while we wait for an ip addresss
      Serial.print(".");
      delay(300);
    }
    server.begin();
  }
  else
  {
      Serial.println("Provisioned with following:");
      Serial.print("SSID: ");
      Serial.println(credentials.ssid);
      Serial.print("PWD: ");
      Serial.println(credentials.pass);
      WiFi.begin(credentials.ssid, credentials.pass);
      while ( WiFi.status() != WL_CONNECTED) 
      {
        // print dots while we wait to connect
        Serial.print(".");
         delay(300);
      }
      Serial.println("\nConnected to the network");
      Serial.println("Waiting for an ip address");

      while (WiFi.localIP() == INADDR_NONE) 
      {
        // print dots while we wait for an ip addresss
        Serial.print(".");
        delay(300);
      }

      // connected, print out the status  
      printWifiStatus();
      pinMode(GREEN_LED, OUTPUT);
  }
    
  
}



token_state_t run_statemachine_provision(char c)
{
  switch(state) 
  {
    case SEEN_NOTHING:
      if(c == REF_TOKEN[token_len]) token_len++;
      else 
      {
        token_len = 0;
        state = SEEN_NOTHING;
        Serial.write(c);
        break;
      }
      if(token_len == strlen(REF_TOKEN)) 
      {
        state = SEEN_REF;
        Serial.println("->SEEN_REF");
        token_len = 0;
      }
    break;
    case SEEN_REF:
      if(c == SSID_TOKEN[token_len]) token_len++;
      else 
      {
        token_len = 0;
        Serial.println("->SEEN_NOTHING");
        state = SEEN_NOTHING;
        break;
      }

      if(token_len == strlen(SSID_TOKEN))
      {
        state = READ_SSID;
        Serial.println("->READ_SSID");
        token_len = 0;
      }
    break;
    case READ_SSID:
      if((c == '&') || (token_len > sizeof(temp_ssid)))
      {
        temp_ssid[token_len] = '\0';
        state = SEEN_SSID;
        Serial.println("->SEEN_SSID");
        token_len = 0;
        break;
      }
      temp_ssid[token_len] = c;
      token_len++;
    break;
    case SEEN_SSID:
      if(c == PWD_TOKEN[token_len]) token_len++;
      else 
      {
        token_len = 0;
        Serial.println("->SEEN_NOTHING");
        state = SEEN_NOTHING;
        break;
      }

      if(token_len == strlen(PWD_TOKEN))
      {
        state = READ_PWD;
        Serial.println("->READ_PWD");
        token_len = 0;
      }
    break;
    case SEEN_PWD:
   
        
        Serial.println("Provisioning to wifi...");
     
    break;
    case READ_PWD:
      if((c == '\n' || c == '\r' || c == '&') || (token_len > sizeof(temp_pass)))
      {
        temp_pass[token_len] = '\0';
        if(decode(temp_ssid, credentials.ssid) > 0 &&      
            decode(temp_pass, credentials.pass) > 0)
        {        
          state = SEEN_PWD;
          Serial.println("->SEEN_PWD");
        }
        else
        {
          state = SEEN_NOTHING;
        }        
        token_len = 0;
        break;
      }
      temp_pass[token_len] = c;
      token_len++;
    break;
    default:
      token_len = 0;
      Serial.write(c);
      state = SEEN_NOTHING;
    break;
  };
  return state;
}

void save_credentials()
{
}

void loop()
{
  // provisioning functionality
  if(!credentials.is_provisioned)
  {
    WiFiClient client = server.available();
    if (client) {
      Serial.println("new client");
      // an http request ends with a blank line
      boolean currentLineIsBlank = true;
      while (client.connected()) 
      {
        if (client.available()) 
        {
          char c = client.read();
          if(SEEN_PWD == run_statemachine_provision(c))
          {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close"); 
            client.println();
            client.println("<!DOCTYPE HTML>");
            client.println("<HTML><HEAD>connecting</HEAD><BODY><H1>Connecting...</H1></BODY></HTML>");
            // give the web browser time to receive the data
            delay(100);
            // close the connection:
            client.stop();
                     
            WiFi.disconnect();
            sl_WlanSetMode(ROLE_STA);
            sl_Stop(0xff);
            Serial.print("Connecting to: ");
            Serial.println(credentials.ssid);
            Serial.print("Pass: ");
            Serial.println(credentials.pass);
            sl_Start(NULL,NULL,NULL);
            WiFi.begin(credentials.ssid, credentials.pass);
            int stat = 0;
            while (( stat =  WiFi.status() )!= WL_CONNECTED) 
            {
              // print dots while we wait to connect
              Serial.print(".");
               delay(300);
            }
            Serial.println("\nConnected to the network");
            Serial.println("Waiting for an ip address");
      
            while (WiFi.localIP() == INADDR_NONE) 
            {
              // print dots while we wait for an ip addresss
              Serial.print(".");
              delay(300);
            }
      
            // connected, print out the status  
            printWifiStatus();
            credentials.is_provisioned = 1;
            save_credentials();
            accelerometer.begin();
            pinMode(GREEN_LED, OUTPUT);
          }
          // if you've gotten to the end of the line (received a newline
          // character) and the line is blank, the http request has ended,
          // so you can send a reply
          if (c == '\n' && currentLineIsBlank) {
            // send a standard http response header
            Serial.println("Sending provisioning page:");
            Serial.println(setup_page);
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close"); 
            client.println();
            client.println("<!DOCTYPE HTML>");
            client.print(setup_page);
            // give the web browser time to receive the data
            delay(100);
            // close the connection:
            client.stop();
            Serial.println("client disonnected");           
            break;
          }
          if (c == '\n' || c == '\r') 
          {
            // you're starting a new line
            currentLineIsBlank = true;
          }
          else
          {
            // you've gotten a character on the current line
            currentLineIsBlank = false;
          }
        }
      }
     
    }
  }
  // data logging functionality
  else
  {
    float x = BIT2G * accelerometer.readXData();
    float y = BIT2G * accelerometer.readYData();
    float z = BIT2G * accelerometer.readZData();
    Serial.print("<");
    Serial.print(x);
    Serial.print(", ");
    Serial.print(y);
    Serial.print(", ");
    Serial.print(z);    
    Serial.println(">");
    delay(1000);
  }
  
  
}
