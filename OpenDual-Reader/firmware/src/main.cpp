#include <Arduino.h>
#include <Preferences.h>
#include <driver/uart.h>
#include <driver/rmt.h>
#include <esp_timer.h>
#include <bootloader_random.h>
extern "C" {
#include <osdp.h>
}
#include "pins.h"
#include "config.h"
#include "credential.h"
#include "whitelist.h"
#include "pn532_async.h"

static Pn532Async hf;
static BoundedQueue<Credential,8> credentials;
static Credential previous[3];
static osdp_t *pd=nullptr;
static Preferences prefs;
static uint8_t scbk[16];
static esp_timer_handle_t relayTimer;
static bool tamper=false, previousTamper=false;
static uint32_t tamperChanged=0, denied=0, badFrames=0;
static uint32_t maxLoopUs=0;
static osdp_event events[8]{};
static bool eventBusy[8]{};
static osdp_cmd_led_params permanent{}, temporary{};
static uint32_t ledStart=0, tempUntil=0, buzzerStart=0, buzzerUntil=0;
static uint16_t buzzOn=0,buzzOff=0;
static rmt_item32_t d0Wave[80]{},d1Wave[80]{};
static uint32_t wiegandUntil=0;

static bool reached(uint32_t now,uint32_t then){return int32_t(now-then)>=0;}
static void setColor(uint8_t c){
  static const uint8_t colors[]={0,1,2,3,4,5,6,7};
  uint8_t mask=c<sizeof(colors)?colors[c]:0;
  digitalWrite(Pins::LED_R,mask&1);digitalWrite(Pins::LED_G,(mask>>1)&1);digitalWrite(Pins::LED_B,(mask>>2)&1);
}
static void uiTick(uint32_t now){
  auto &p=(!reached(now,tempUntil))?temporary:permanent;
  uint32_t period=100U*(uint32_t(p.on_count)+p.off_count);
  bool on=period==0 || (now-ledStart)%period<100U*p.on_count;
  setColor(on?p.on_color:p.off_color);
  bool sounding=!reached(now,buzzerUntil) && (buzzOn+buzzOff)>0 &&
                (now-buzzerStart)%(buzzOn+buzzOff)<buzzOn;
  ledcWrite(0,sounding?512:0); // 4kHz passive piezo carrier, envelope scheduled here.
}
static void relayOff(void*){digitalWrite(Pins::DEMO_RELAY,LOW);}
static bool keyValid(const uint8_t *k){
  uint8_t any=0,all=0xFF;bool def=true;
  for(int i=0;i<16;i++){any|=k[i];all&=k[i];def &= k[i]==uint8_t(0x30+i);}
  return any && all!=0xFF && !def;
}
static bool secure(){uint8_t mask[16]{};if(pd)osdp_get_sc_status_mask(pd,mask);return pd && (mask[0]&1);}
static int recvOsdp(void*,uint8_t *buf,int n){return uart_read_bytes(UART_NUM_1,buf,n,0);}
static int sendOsdp(void*,uint8_t *buf,int n){
  // Driver TX ring is 1024 bytes; LibOSDP packets are <=256. No partial enqueue.
  size_t free=0; if(uart_get_tx_buffer_free_size(UART_NUM_1,&free)!=ESP_OK || free<size_t(n))return 0;
  return uart_write_bytes(UART_NUM_1,reinterpret_cast<const char*>(buf),n);
}
static void flushOsdp(void*){uart_flush_input(UART_NUM_1);}
static void eventDone(void*,const osdp_event *ev,osdp_completion_status){
  for(size_t i=0;i<8;i++)if(ev==&events[i])eventBusy[i]=false;
}
static osdp_event *allocEvent(){for(size_t i=0;i<8;i++)if(!eventBusy[i]){eventBusy[i]=true;events[i]={};return &events[i];}return nullptr;}
static bool submit(osdp_event *ev){
  if(osdp_pd_submit_event(pd,ev)==0)return true;
  eventDone(nullptr,ev,OSDP_COMPLETION_FAILED);return false;
}
static void statusEvent(){
  if(!secure())return;auto *e=allocEvent();if(!e)return;
  e->type=OSDP_EVENT_STATUS;e->status.type=OSDP_STATUS_REPORT_LOCAL;
  e->status.nr_entries=2;e->status.report[0]=tamper?1:0;submit(e);
}
static int command(void*,osdp_cmd *cmd){
  if(!secure())return -1;
  switch(cmd->id){
    case OSDP_CMD_LED:
      if(cmd->led.reader || cmd->led.led_number || cmd->led.temporary.control_code>2 ||
         cmd->led.permanent.control_code>1 || cmd->led.temporary.on_color>7 ||
         cmd->led.temporary.off_color>7 || cmd->led.permanent.on_color>7 || cmd->led.permanent.off_color>7)return -1;
      if(cmd->led.permanent.control_code)permanent=cmd->led.permanent;
      if(cmd->led.temporary.control_code==1)tempUntil=millis();
      if(cmd->led.temporary.control_code==2){temporary=cmd->led.temporary;tempUntil=millis()+100U*temporary.timer_count;}
      ledStart=millis();return 0;
    case OSDP_CMD_BUZZER:
      if(cmd->buzzer.reader || cmd->buzzer.control_code>2)return -1;
      if(cmd->buzzer.control_code<2){buzzerUntil=millis();return 0;}
      // Reject unbounded sound; a finite pattern must complete within30seconds.
      {uint32_t duration=100U*(cmd->buzzer.on_count+cmd->buzzer.off_count)*cmd->buzzer.rep_count;
       if(!cmd->buzzer.rep_count || !duration || duration>30000)return -1;
       buzzOn=100U*cmd->buzzer.on_count;buzzOff=100U*cmd->buzzer.off_count;
       buzzerStart=millis();buzzerUntil=buzzerStart+duration;return 0;}
    case OSDP_CMD_KEYSET:
      if(cmd->keyset.type!=1 || cmd->keyset.length!=16 || !keyValid(cmd->keyset.data))return -1;
      if(!prefs.begin("opendual",false))return -1;
      {size_t n=prefs.putBytes("scbk",cmd->keyset.data,16);prefs.end();if(n!=16)return -1;}
      memcpy(scbk,cmd->keyset.data,16);return 0;
    case OSDP_CMD_STATUS:
      if(cmd->status.type!=OSDP_STATUS_REPORT_LOCAL)return -1;
      cmd->status.nr_entries=2;memset(cmd->status.report,0,sizeof(cmd->status.report));
      cmd->status.report[0]=tamper?1:0;return 0;
    // No relay capability in panel modes. Address/baud set by wired provisioning.
    default:return -1;
  }
}
static bool startOsdp(){
  if(!prefs.begin("opendual",true))return false;
  uint8_t address=prefs.getUChar("addr",0xFF);
  size_t n=prefs.getBytes("scbk",scbk,16);prefs.end();
  if(address>126 || n!=16 || !keyValid(scbk))return false;
  uart_config_t uc{};uc.baud_rate=OSDP_BAUD;uc.data_bits=UART_DATA_8_BITS;
  uc.parity=UART_PARITY_DISABLE;uc.stop_bits=UART_STOP_BITS_1;uc.flow_ctrl=UART_HW_FLOWCTRL_DISABLE;
  uc.source_clk=UART_SCLK_APB;
  if(uart_param_config(UART_NUM_1,&uc)!=ESP_OK ||
     uart_set_pin(UART_NUM_1,Pins::RS485_TX,Pins::RS485_RX,Pins::RS485_DIR,UART_PIN_NO_CHANGE)!=ESP_OK ||
     uart_driver_install(UART_NUM_1,1024,1024,0,nullptr,0)!=ESP_OK ||
     uart_set_mode(UART_NUM_1,UART_MODE_RS485_HALF_DUPLEX)!=ESP_OK)return false;
  static const osdp_pd_cap caps[]={
    {OSDP_PD_CAP_READER_LED_CONTROL,1,1},{OSDP_PD_CAP_READER_AUDIBLE_OUTPUT,1,1},
    {OSDP_PD_CAP_READERS,0,2},{OSDP_PD_CAP_COMMUNICATION_SECURITY,1,1},
    {OSDP_PD_CAP_CHECK_CHARACTER_SUPPORT,1,0},{0,0,0}};
  osdp_pd_info_t info{};info.name="OpenDual";info.address=address;info.baud_rate=OSDP_BAUD;
  info.flags=OSDP_FLAG_ENFORCE_SECURE;info.scbk=scbk;info.cap=caps;
  info.id.version=1;info.id.model=1;info.id.vendor_code=0; // no invented IEEE OUI
  info.id.serial_number=uint32_t(ESP.getEfuseMac());info.id.firmware_version=0x000100;
  info.channel.recv=recvOsdp;info.channel.send=sendOsdp;info.channel.flush=flushOsdp;
  pd=osdp_pd_setup(&info);if(!pd)return false;
  osdp_pd_set_command_callback(pd,command,nullptr);
  osdp_pd_set_event_completion_callback(pd,eventDone,nullptr);return true;
}
static void acquired(Credential c){
  size_t i=size_t(c.kind)-1;if(i>=3)return;
  if(sameCredential(c,previous[i]) && c.seen-previous[i].seen<DUPLICATE_MS)return;
  previous[i]=c;credentials.push(c);
}
static void lfTick(uint32_t now){
  static uint8_t frame[40];static size_t count=0;static bool active=false,overflow=false;
  static uint8_t ending=0;static uint32_t last=0;
  if(active && now-last>100){active=false;++badFrames;}
  uint8_t bytes[32];int available=uart_read_bytes(UART_NUM_2,bytes,sizeof(bytes),0);
  for(int budget=0;budget<available;budget++){
    uint8_t b=bytes[budget];last=now;
    if(b==2){active=true;overflow=false;ending=0;count=0;continue;}
    if(!active)continue;
    if(b==3){Credential c;active=false;
      if(!overflow && ending==2 && parseLf(frame,count,c)){c.seen=now;acquired(c);}else ++badFrames;
      continue;
    }
    if(b==13 && ending==0){ending=1;continue;}
    if(b==10 && ending==1){ending=2;continue;}
    if(ending || count>=sizeof(frame)){overflow=true;continue;}
    frame[count++]=b;
  }
}
static bool initWiegand(){
  for(int i=0;i<2;i++){
    rmt_config_t c{};c.rmt_mode=RMT_MODE_TX;c.channel=rmt_channel_t(i);
    c.gpio_num=gpio_num_t(i?Pins::WIEGAND_D1:Pins::WIEGAND_D0);c.clk_div=80;c.mem_block_num=1;
    c.tx_config.idle_output_en=true;c.tx_config.idle_level=RMT_IDLE_LEVEL_LOW;
    if(rmt_config(&c)!=ESP_OK || rmt_driver_install(c.channel,0,0)!=ESP_OK)return false;
  }return true;
}
static bool wiegand(const Credential &c,uint32_t now){
  if(!reached(now,wiegandUntil))return false;
  if((c.kind==CredentialKind::HF_UID && !ALLOW_HF_RAW_WIEGAND) ||
     (c.kind==CredentialKind::LF_EM && !ALLOW_EM_RAW_WIEGAND) || c.kind==CredentialKind::LF_HID_RAW){++denied;return true;}
  unsigned bits=c.length*8;if(bits>80){++denied;return true;}
  for(unsigned i=0;i<bits;i++){
    bool one=c.data[i/8]&(0x80>>(i%8));
    d0Wave[i].duration0=d1Wave[i].duration0=50;
    d0Wave[i].duration1=d1Wave[i].duration1=1950;
    d0Wave[i].level0=!one;d1Wave[i].level0=one;d0Wave[i].level1=d1Wave[i].level1=0;
  }
  if(rmt_write_items(RMT_CHANNEL_0,d0Wave,bits,false)!=ESP_OK ||
     rmt_write_items(RMT_CHANNEL_1,d1Wave,bits,false)!=ESP_OK){++denied;return true;}
  wiegandUntil=now+bits*2+30;return true;
}
static void process(uint32_t now){
  auto *c=credentials.front();if(!c)return;
  if(now-c->seen>CREDENTIAL_TTL_MS){credentials.pop();return;}
  if(MODE==OperatingMode::OSDP_PD){
    if(!secure()){credentials.pop();return;}
    auto *e=allocEvent();if(!e)return;e->type=OSDP_EVENT_CARDREAD;
    e->cardread.reader_no=c->kind==CredentialKind::HF_UID?0:1;
    e->cardread.format=OSDP_CARD_FMT_RAW_UNSPECIFIED;e->cardread.length=c->length*8;
    memcpy(e->cardread.data,c->data,c->length);if(!submit(e))return;
  }else if(MODE==OperatingMode::WIEGAND_READER){if(!wiegand(*c,now))return;
  }else{
    bool allowed=false;for(size_t i=0;i<DEMO_WHITELIST_COUNT;i++)allowed |= sameCredential(*c,DEMO_WHITELIST[i]);
    if(allowed && !tamper && !esp_timer_is_active(relayTimer)){
      // Never extend an active grant by repeating a credential.
      digitalWrite(Pins::DEMO_RELAY,HIGH);
      if(esp_timer_start_once(relayTimer,RELAY_MS*1000ULL)!=ESP_OK)relayOff(nullptr);
    }else ++denied;
  }credentials.pop();
}

#ifdef ODR_PROVISIONING_IMAGE
static void provisionTick(){
  static char line[80];static size_t pos=0;static bool overflow=false;
  for(int i=0;i<16 && Serial.available();i++){
    char c=Serial.read();if(c=='\r')continue;
    if(c!='\n'){if(pos<sizeof(line)-1)line[pos++]=c;else overflow=true;continue;}
    line[pos]=0;unsigned addr=999;char hex[34]{};uint8_t key[16]{};
    bool ok=!overflow && sscanf(line,"KEY %u %33s",&addr,hex)==2 && addr<127 && strlen(hex)==32;
    for(int j=0;j<16 && ok;j++){int a=hexDigit(hex[j*2]),b=hexDigit(hex[j*2+1]);if(a<0||b<0)ok=false;else key[j]=(a<<4)|b;}
    ok &= keyValid(key);
    if(ok && prefs.begin("opendual",false)){
      ok=prefs.putBytes("scbk",key,16)==16 && prefs.putUChar("addr",addr)==1;
      uint8_t verify[16]{};ok &= prefs.getBytes("scbk",verify,16)==16 && !memcmp(key,verify,16);
      prefs.end();
    }else ok=false;
    memset(key,0,sizeof(key));memset(line,0,sizeof(line));memset(hex,0,sizeof(hex));pos=0;overflow=false;
    Serial.println(ok?"PROVISIONED: flash normal image preserving NVS":"REJECTED");
  }
}
#endif
void setup(){
  const int safe[]={Pins::RS485_DIR,Pins::WIEGAND_D0,Pins::WIEGAND_D1,Pins::LED_R,Pins::LED_G,Pins::LED_B,Pins::BUZZER,Pins::DEMO_RELAY};
  for(int p:safe){digitalWrite(p,LOW);pinMode(p,OUTPUT);}
  pinMode(Pins::TAMPER,INPUT);tamper=previousTamper=digitalRead(Pins::TAMPER)==HIGH;
  Serial.begin(115200);bootloader_random_enable(); // ADC entropy; do not enable Wi-Fi/BT/ADC concurrently.
  esp_timer_create_args_t timer{};timer.callback=relayOff;timer.name="relay_off";
  if(esp_timer_create(&timer,&relayTimer)!=ESP_OK)abort();
#ifdef ODR_PROVISIONING_IMAGE
  Serial.println("WIRED PROVISIONING IMAGE. All reader outputs disabled. KEY <0..126> <32 hex>");
#else
  bool ready=true;
  if(MODE==OperatingMode::OSDP_PD)ready=startOsdp();
  if(MODE==OperatingMode::WIEGAND_READER)ready=initWiegand();
  permanent.on_color=ready?OSDP_LED_COLOR_BLUE:OSDP_LED_COLOR_RED;
  // Use IDF driver directly: HardwareSerial(-1 TX) can select default GPIO17,
  // which would conflict with Wiegand. UART2 TX is never routed to a pad.
  uart_config_t lc{};lc.baud_rate=9600;lc.data_bits=UART_DATA_8_BITS;
  lc.parity=UART_PARITY_DISABLE;lc.stop_bits=UART_STOP_BITS_1;
  lc.flow_ctrl=UART_HW_FLOWCTRL_DISABLE;lc.source_clk=UART_SCLK_APB;
  ESP_ERROR_CHECK(uart_param_config(UART_NUM_2,&lc));
  ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2,UART_PIN_NO_CHANGE,Pins::LF_RX,UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE));
  ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2,256,0,0,nullptr,0));
  ledcSetup(0,4000,10);ledcAttachPin(Pins::BUZZER,0);ledcWrite(0,0);
  hf.begin(millis());
  Serial.println(ready?"OpenDual engineering firmware ready":"FAIL CLOSED: missing provisioning or interface failure");
#endif
}
void loop(){
#ifdef ODR_PROVISIONING_IMAGE
  provisionTick();delay(1);
#else
  uint32_t start=micros(),now=millis();
  bool raw=digitalRead(Pins::TAMPER)==HIGH;
  if(raw!=previousTamper){previousTamper=raw;tamperChanged=now;}
  if(raw!=tamper && now-tamperChanged>=30){tamper=raw;if(tamper)relayOff(nullptr);statusEvent();}
  if(pd){osdp_pd_refresh(pd);if(!secure())osdp_pd_flush_events(pd);}
  lfTick(now);Credential c;if(hf.tick(now,c))acquired(c);
  process(now);uiTick(now);
  uint32_t elapsed=micros()-start;if(elapsed>maxLoopUs)maxLoopUs=elapsed;
  delay(1);
#endif
}

