// Переименовать файл secrets_RENAME_ME.h в secrets.h
// Заменить значения секретов на свои
#include "secrets.h"

#include <FastBot.h>
#include <OneWire.h>
#include <EEPROM.h>
#include <string>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 2
#define CORRECTION 3

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
FastBot bot(BOT_TOKEN);

float t;
bool set_limit = false;
float limit = 17;
String user = "380330296";  // айдишник пользователя, который сможет пользоваться ботом (можно заменить на массив и немного переписать 71 строчку)
bool send_msg = true;
String buttons = "🌡Температура\n🔔Порог оповещения";

void setup() {
  delay(5000);
  Serial.begin(115200);
  connectWiFi();

  bot.setChatID(user);
  bot.setPeriod(5000);
  DS18B20.begin();
  EEPROM.begin(100);
  bot.attach(newMsg);

  float lim = 0;
  EEPROM.get(0, lim);
  if (lim != NULL) {
    limit = lim;
  }

  sayConnected("На связи после перезагрузки устройства");
}

String infoString() {
  return ("Температура: " + String(t) + "°C.\n" + "Узнать командой /get_temp или через меню\n\n" + "Порог оповещения: " + String(limit) + "°C\n" + "Установить командой /set_a_threshold или через меню");
}

void sayConnected(String info) {
  getTemp();
  String s = info + "\n\n" + infoString();
  bool no_tg = false;
  while (bot.showMenuText(s, buttons) == 3) {
    no_tg = true;
    Serial.print("#");
    delay(5000);
  };
  if (no_tg == true) {
    delay(2000);
    getTemp();
    bot.sendMessage("❗️Внимание! Был недоступен Телеграм или отключался Интернет. Температура сейчас " + String(t) + "°C");
  }
}

void getTemp() {
  DS18B20.requestTemperatures();
  t = DS18B20.getTempCByIndex(0) - CORRECTION;
}

// обработчик сообщений
void newMsg(FB_msg& msg) {
  // выводим ID чата, имя юзера и текст сообщения
  String id = msg.chatID;
  Serial.print(msg.chatID);  // ID чата
  Serial.print(", ");
  Serial.print(msg.username);  // логин
  Serial.print(", ");
  Serial.println(msg.text);  // текст
  String send_msg1 = "";

  if (msg.text == "/get_temp" or msg.text == "🌡Температура") {
    getTemp();
    send_msg1 = infoString();
    send_msg = true;
  } else if ((msg.text == "/set_a_threshold" or msg.text == "🔔Порог оповещения") and (id == user)) {
    if (set_limit == false) {
      send_msg1 = "Сейчас порог оповещения: " + String(limit) + "°C.\n\n" + "Чтобы изменить, введите новый порог (напр: 25.5)\n" + "Выход из режима без изменения командой /set_a_threshold или нажатием на пункт меню:\n" + "🔔Порог оповещения";
      set_limit = true;
    } else {
      send_msg1 = "Выход из редактирования порога оповещения";
      set_limit = false;
    }
  } else if (set_limit == true and (id == user)) {
    float lim1 = strtof(msg.text.c_str(), NULL);
    if (lim1 != 0) {
      limit = lim1;
      EEPROM.put(0, lim1);
      EEPROM.commit();
      send_msg1 = "Установлен порог оповещения: " + msg.text + "°C", msg.chatID;
      send_msg = true;
      set_limit = false;
    }
  }
  bot.showMenuText(send_msg1, buttons, msg.chatID);
}

void loop() {
  float check_lim;
  if ((WiFi.status() != WL_CONNECTED)) {
    Serial.println("Disconnected from WiFi SSID " + String(WIFI_SSID));
    WiFi.disconnect();
    connectWiFi();
    sayConnected("На связи после отключения WiFi");
  }
  EEPROM.get(0, check_lim);
  getTemp();
  if (t < check_lim and send_msg == true) {
    bool no_tg = false;
    // TODO ЗДЕСЬ И ВЕЗДЕ ИЗМЕНЯТЬ ТЕКСТ, если ТГ недоступен
    while (bot.sendMessage("❗️Внимание! Температура ниже " + String(limit) + "°C.\nПриезжайте домой!") == 3) {
      no_tg = true;
      Serial.print("!");
    }
    if (no_tg == true) {
      getTemp();
      bot.sendMessage("❗️Внимание! Был недоступен Телеграм или отключался Интернет. Температура сейчас " + String(t) + "°C");
    }
    send_msg = false;
  }
  bool no_tg = false;
  while (bot.tick() == 3) {
    no_tg = true;
    Serial.print("+");
  }
  if (no_tg == true) {
    getTemp();
    bot.sendMessage("❗️Внимание! Был недоступен Телеграм или отключался Интернет. Температура сейчас " + String(t) + "°C");
  }
}

void connectWiFi() {
  Serial.println("\nConnecting to WiFi SSID " + String(WIFI_SSID));
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long prevMillis = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    if (millis() - prevMillis > 60000) {
      Serial.println("\nRestarting ESP");
      ESP.restart();
    }
  }
  Serial.println("\nConnected to WiFi SSID " + String(WIFI_SSID));
}
