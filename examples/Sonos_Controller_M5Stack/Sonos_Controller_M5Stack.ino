#include <SonosUPnP.h>
#include <MicroXPath_P.h>
#include <M5Stack.h>

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

#define SERIAL_DATA_THRESHOLD_MS 500
#define SERIAL_ERROR_TIMEOUT "E: Serial"
#define ETHERNET_ERROR_DHCP "E: DHCP"
#define ETHERNET_ERROR_CONNECT "E: Connect"

void handleSerialRead();
void ethConnectError();
//EthernetClient g_ethClient;
WiFiClient client;
SonosUPnP g_sonos = SonosUPnP(client, ethConnectError);

// Living room
IPAddress g_KitchenIP(192, 168, 1, 206);
const char g_KitchenID[] = "5CAAFD406A906";

const char* ssid     = "wifi";
const char* password = "password";

bool isPlaying = true; //Assume we're playing so always pause first.

char uri[100] = "";
String lastCmd;

#define HTTPPORT 88

WebServer server(HTTPPORT);

void handleRoot();
void handleCmd();
void handleNotFound();
void handleResponse();
void handleGet();
void handleGt();

static const char HTML_HEADER[] PROGMEM = R"(
<html>
<head>
<title>ESP32 Sonos Controller</title>
<style>
.slider{width:250px}
.response{font-family:"Courier New",Courier,"Lucida Sans Typewriter","Lucida Typewriter",monospace;border-radius:10px;background-color:#FFF;height:25%;overflow-y:scroll;padding:10px}
.controls{font-size:36px;color:#000;text-decoration:none;padding:0 10px}
#container{width:98%;height:98%;border-radius:10px;background-color:#999;padding:10px;margin:auto}
#linkholder{margin:auto}
#console_text{width:250px}
</style>
<script>
var v=0,l="";async function sendCmd(c){try{const r=await fetch(`/cmd?cmd=${c}`);if(r.ok){const t=await r.text();addR(t+"<br/>")}}catch(e){addR(`Error: ${e}<br/>`)}}function parseCmd(c){document.getElementById('console_text').value='';sendCmd(c)}async function setV(v){v=parseInt(v);if(Number.isInteger(v)){if(v>99||v<0){addR("Your number must be between 0 and 99<br/>");document.getElementById("volume").value=v}else{await sendCmd(v);v=v;await getV()}}else{addR("You must enter a number<br/>");document.getElementById("volume").value=v}}function addR(r){document.getElementById("response").innerHTML+=r;var e=document.getElementById('response');e.scrollTop=e.scrollHeight}async function getV(){try{const r=await fetch("/get?cmd=gv");if(r.ok){const t=await r.text();addR("v = "+t+"<br/>");v=t;document.getElementById("volume").value=v;document.getElementById("vol").textContent=v;document.getElementById("volume-slider").value=v}}catch(e){addR(`Error: ${e}<br/>`)}}async function getT(){const s=await sendR("gs");addR("SongType = "+s+"<br/>")}async function sendR(c){try{const r=await fetch(`/get?cmd=${c}`);if(r.ok){l=await r.text();return l}}catch(e){addR(`Error: ${e}<br/>`)}return l}async function updateS(){await getT();await getV()}
</script>
</head>
<body>
<div id="container">
<h1>Sonos - M5Stack Web Controller!</h1>
)";

static const char HTML_CONTROLS[] PROGMEM = R"(
<p id="linkholder"><a href="#" onclick="sendCmd('pr');" class="controls">&#9198;</a> 
<a href="#" onclick="sendCmd('pl');" class="controls">&#9654;</a> 
<a href="#" onclick="sendCmd('pa');" class="controls">&#9208;</a> 
<a href="#" onclick="sendCmd('nx');" class="controls">&#9197;</a></p>
)";

static const char HTML_FOOTER[] PROGMEM = R"(
<p>Server Response:<div id="response" class="response"></div></p>
<p><form action="/" method="get" id="console"><input placeholder="Enter a command..." type="text" id='console_text'/></form></p>
<script>var intervalID = window.setInterval(getVolume, 50000);
document.getElementById('console').addEventListener('submit', function(e) { e.preventDefault(); parseCmd(document.getElementById('console_text').value); });
</script>
</div>
<div id="tips"></div>
</body>
</html>
)";

void setup()
{
  Serial.begin(115200);
  M5.begin();
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.printf("Connecting...");
  WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

  Serial.println("connected to WiFi");
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.printf("Wifi Connected!");
  
  server.on("/", handleRoot);
  server.on("/cmd", handleCmd);
  server.on("/get", handleGt);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.print("HTTP server started on ");
  Serial.println(HTTPPORT);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(4);
  M5.Lcd.printf("Playing");
  M5.Lcd.fillRect(145,200,6,30,M5.Lcd.color565(255, 255, 255));
  M5.Lcd.fillRect(165,200,6,30,M5.Lcd.color565(255, 255, 255));
}

void ethConnectError()
{
  Serial.println(ETHERNET_ERROR_CONNECT);
  Serial.println("Wifi died.");
}

void loop()
{
  M5.update();
  server.handleClient();
  if (M5.BtnA.wasReleased()) {
    g_sonos.skip(g_KitchenIP, SONOS_DIRECTION_BACKWARD);
  } else if (M5.BtnB.wasReleased()) {
    if(isPlaying)
    {
      g_sonos.pause(g_KitchenIP);
      isPlaying = false;
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(10, 10);
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setTextSize(4);
      M5.Lcd.printf("Paused");
      M5.Lcd.fillTriangle(185,210,150,190,150,230,M5.Lcd.color565(255, 255, 255));
    }
    else
    {
      g_sonos.play(g_KitchenIP);
      isPlaying = true;
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(10, 10);
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setTextSize(4);
      M5.Lcd.printf("Playing");
      M5.Lcd.fillRect(145,200,6,30,M5.Lcd.color565(255, 255, 255));
      M5.Lcd.fillRect(165,200,6,30,M5.Lcd.color565(255, 255, 255));
    }
  } else if (M5.BtnC.wasReleased()) {
    g_sonos.skip(g_KitchenIP, SONOS_DIRECTION_FORWARD);
  }
}


bool isCommand(const char *command, byte b1, byte b2)
{
  return *command == b1 && *++command == b2;
}

String handleGet(String cmd)
{
    Serial.println("Handling command " + cmd);
    if (cmd == "gv")
    {
      int volume = g_sonos.getVolume(g_KitchenIP);
      return String(volume);
    }
    else if (cmd == "gs")
    {
      String response = "";
      char uri[25] = "";
      TrackInfo track = g_sonos.getTrackInfo(g_KitchenIP, uri, sizeof(uri));
      byte source = g_sonos.getSourceFromURI(track.uri);
      switch (source)
      {
        case SONOS_SOURCE_FILE:
          response += "File: ";
          break;
        case SONOS_SOURCE_HTTP:
          response += "HTTP: ";
          break;
        case SONOS_SOURCE_RADIO:
          response += "Radio: ";
          break;
        case SONOS_SOURCE_LINEIN:
          response += "Line-In: ";
          break;
        case SONOS_SOURCE_MASTER:
          response += "Other Speaker: ";
          break;
        default:
          response += "Unknown";
          break;
      }
      if (source == SONOS_SOURCE_FILE || source == SONOS_SOURCE_HTTP)
      {
        response += ", track = ";
        response += track.number, DEC;
        response += ", pos = ";
        response += track.position, DEC;
        response += " of ";
        response += track.duration, DEC;
      }
      return response;
    }
    else
    {
      return "-1";
    }
}

void handleInput(String cmd, byte b1, byte b2)
{
  // Read 2 bytes from serial buffer
    // Play
    Serial.println("Handling command " + cmd);
    if (cmd == "pl")
    {
      g_sonos.play(g_KitchenIP);
    }
    // Pause
    else if (isCommand("pa", b1, b2))
    {
      g_sonos.pause(g_KitchenIP);
    }
    // Stop
    else if (isCommand("st", b1, b2))
    {
      g_sonos.stop(g_KitchenIP);
    }
    // Previous
    else if (isCommand("pr", b1, b2))
    {
      g_sonos.skip(g_KitchenIP, SONOS_DIRECTION_BACKWARD);
    }
    // Next
    else if (isCommand("nx", b1, b2))
    {
      g_sonos.skip(g_KitchenIP, SONOS_DIRECTION_FORWARD);
    }
    // Play File
    else if (isCommand("fi", b1, b2))
    {
      g_sonos.playFile(g_KitchenIP, "192.168.188.22/Music/ringtone/ring1.mp3");

    }
    // Play HTTP
    else if (isCommand("ht", b1, b2))
    {
      // Playing file from music service WIMP (SID = 20)
      //g_sonos.playHttp(g_KitchenIP, "trackid_37554547.mp4?sid=20&amp;flags=32");

      //g_sonos.playHttp(g_KitchenIP, "http://192.168.188.1:49200/AUDIO/DLNA-1-0/MXT-USB-StorageDevice-01/take_me_to_church.mp3");
      g_sonos.playHttp(g_KitchenIP, "http://192.168.188.28:88/gong.mp3");
    }
    // Play Radio
    else if (isCommand("ra", b1, b2))
    {
      g_sonos.playRadio(g_KitchenIP, "//lyd.nrk.no/nrk_radio_p3_mp3_h.m3u", "NRK P3");

    }
    // Play Line In
    else if (isCommand("li", b1, b2))
    {
      g_sonos.playLineIn(g_KitchenIP, g_KitchenID);
    }
    // Repeat On
    else if (isCommand("re", b1, b2))
    {
      g_sonos.setPlayMode(g_KitchenIP, SONOS_PLAY_MODE_REPEAT);
    }
    // Shuffle On
    else if (isCommand("sh", b1, b2))
    {
      g_sonos.setPlayMode(g_KitchenIP, SONOS_PLAY_MODE_SHUFFLE);
    }
    // Repeat and Shuffle On
    else if (isCommand("rs", b1, b2))
    {
      g_sonos.setPlayMode(g_KitchenIP, SONOS_PLAY_MODE_SHUFFLE_REPEAT);
    }
    // Repeat and Shuffle Off
    else if (isCommand("no", b1, b2))
    {
      g_sonos.setPlayMode(g_KitchenIP, SONOS_PLAY_MODE_NORMAL);
    }
    // Loudness On
    else if (isCommand("lo", b1, b2))
    {
      g_sonos.setLoudness(g_KitchenIP, true);
    }
    // Loudness Off
    else if (isCommand("l_", b1, b2))
    {
      g_sonos.setLoudness(g_KitchenIP, false);
    }
    // Mute On
    else if (isCommand("mu", b1, b2))
    {
      g_sonos.setMute(g_KitchenIP, true);
    }
    // Mute Off
    else if (isCommand("m_", b1, b2))
    {
      g_sonos.setMute(g_KitchenIP, false);
    }
    // Volume/Bass/Treble
    else if (b2 >= '0' && b2 <= '9')
    {
      // Volume 0 to 99
      if (b1 >= '0' && b1 <= '9')
      {
        g_sonos.setVolume(g_KitchenIP, ((b1 - '0') * 10) + (b2 - '0'));
      }
      // Bass 0 to -9
      else if (b1 == 'b')
      {
        g_sonos.setBass(g_KitchenIP, (b2 - '0') * -1);
      }
      // Bass 0 to 9
      else if (b1 == 'B')
      {
        g_sonos.setBass(g_KitchenIP, b2 - '0');
      }
      // Treble 0 to -9
      else if (b1 == 't')
      {
        g_sonos.setTreble(g_KitchenIP, (b2 - '0') * -1);
      }
      // Treble 0 to 9
      else if (b1 == 'T')
      {
        g_sonos.setTreble(g_KitchenIP, b2 - '0');
      }
    }

    else if (isCommand("ti", b1, b2))
    {
      Serial.println("we want the track uri");
      TrackInfo track = g_sonos.getTrackInfo(g_KitchenIP, uri, sizeof(uri));
      Serial.println(uri);
    }

}

/* WebServer Stuff */

void handleRoot() {
    int vol = g_sonos.getVolume(g_KitchenIP);
    Serial.println("VOLUME:");
    Serial.println(g_sonos.getVolume(g_KitchenIP));
    
    String msg = String(FPSTR(HTML_HEADER));
    msg += String(FPSTR(HTML_CONTROLS));
    msg += "<h3>Volume: <span id=\"vol\">"+String(vol)+"</span><input type=\"hidden\" id='volume' value='"+String(vol)+"' onchange=\"setV(this.value)\"/></h3><br/>\n";
    msg += "<input type=\"range\" class=\"slider\"  min=\"0\" max=\"99\" value=\""+String(vol)+"\" name=\"volume-slider\" id=\"volume-slider\" onchange=\"setV(this.value)\" />\n";
    msg += String(FPSTR(HTML_FOOTER));
    
    server.send(200, "text/html", msg);
}

void handleCmd(){
  for (uint8_t i=0; i<server.args(); i++){
    if(server.argName(i) == "cmd") 
    {
      lastCmd = server.arg(i);
      byte b1 =  server.arg(i)[0];
      byte b2 = server.arg(i)[1];
      handleInput(lastCmd,b1,b2);
    }
  }
  handleResponse();
}

void handleGt(){
  String resp;
  for (uint8_t i=0; i<server.args(); i++){
    if(server.argName(i) == "cmd") 
    {
      lastCmd = server.arg(i);
      byte b1 =  server.arg(i)[0];
      byte b2 = server.arg(i)[1];
      resp = handleGet(lastCmd);
    }
  }
  handleGetResponse(resp);
}

void handleNotFound() {

  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);

}

void handleResponse() {
      server.send(200, "text/html", "Worked("+lastCmd+")");
      Serial.println("Got client.");
}

void handleGetResponse(String response) {
      server.send(200, "text/html", response);
      Serial.println("Got client.");
}