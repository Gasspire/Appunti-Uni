#include <cstdint>
#include <iot_board.h>
#include <sys/types.h>
#include <Wire.h>

// Identificativo univoco del vostro gruppo
#define ID_GRUPPO 0x05

// Numero totale di board nel gruppo (1 Root + 3 Station)
#define NUM_BOARD 4

#define MAC_0 "34:85:18:7B:1F:68" // Antonio - ROOT (Gruppo 0)
#define MAC_1 "34:85:18:7A:B7:00" // Diletta       (Gruppo 1)
#define MAC_2 "34:85:18:7A:FA:14" // Marco         (Gruppo 0)
#define MAC_3 "34:85:18:7B:0A:0C" // Dave          (Gruppo 1)

enum STATI { SYNC, ROSSO, VERDE, GIALLO };

uint8_t turno = 0;
uint8_t gruppo = 0;
uint8_t myMac[6];
int numNodi = 0;
bool SONO_ROOT = false;
STATI stato = SYNC;

// Funzione centralizzata per aggiornare il display OLED
// Viene chiamata SOLO ai cambi di stato effettivi per evitare blocchi I2C
void aggiornaDisplay() {
  display->clearDisplay();
  display->setCursor(0, 0);
  display->printf("G%d - %s\n", gruppo, SONO_ROOT ? "ROOT" : "NODO");
  display->printf("Nodi: %d/%d\n", numNodi, NUM_BOARD);
  display->printf("Stato: %s\n", (stato == SYNC ? "SYNC" : (stato == VERDE ? "VERDE" : "ROSSO")));
  display->printf("Turno: %d\n", turno);
  display->display();
}

// Aggiorna lo stato in base al turno; restituisce true se lo stato è cambiato
bool applicaTurno(uint8_t t) {
  turno = t;
  STATI vecchioStato = stato;
  if (gruppo == turno) {
    stato = VERDE;
  } else {
    stato = ROSSO;
  }
  return (stato != vecchioStato);
}

// Invia un comando a tutti i nodi:
// 1. Broadcast (raggiunge i nodi direttamente connessi in 1 hop)
// 2. Unicast P2P a ciascun nodo nella routing table (raggiunge i nodi a più hop di distanza)
void inviaATutti(const String &cmd) {
  espmesh->broadcast(cmd.c_str(), cmd.length());

  uint8_t nodes[10][6];
  int count = espmesh->getNodes(nodes, 10);
  for (int i = 0; i < count; i++) {
    if (memcmp(nodes[i], myMac, 6) != 0) {
      espmesh->sendTo(nodes[i], cmd.c_str(), cmd.length());
    }
  }
}

void onMesh(const EspMeshEvent &e) {
  switch (e.type) {
  case EspMeshEventType::Connected:
    Serial.printf("[" MACSTR "] Connesso alla rete mesh!\n", MAC2STR(e.from));
    stato = SYNC;
    aggiornaDisplay();
    break;

  case EspMeshEventType::Disconnected:
    if (!SONO_ROOT) {
      stato = SYNC;
      Serial.printf("[" MACSTR "] Disconnesso dalla rete mesh!\n", MAC2STR(e.from));
      aggiornaDisplay();
    }
    break;

  case EspMeshEventType::ChildJoined:
    Serial.printf("[ROOT] Nodo connesso: " MACSTR "\n", MAC2STR(e.from));
    break;

  case EspMeshEventType::ChildLeft:
    Serial.printf("[ROOT] Nodo caduto/scollegato: " MACSTR "\n", MAC2STR(e.from));
    if (SONO_ROOT) {
      stato = SYNC;
      inviaATutti("CMD:SYNC");
      aggiornaDisplay();
      Serial.println("[ROOT] Inviato allarme immediato CMD:SYNC!");
    }
    break;

  case EspMeshEventType::Message:
    // Ignoro il messaggio se è l'eco del mio stesso messaggio
    if (memcmp(e.from, myMac, 6) == 0)
      return;

    // Se sono una Station, applico il comando
    if (!SONO_ROOT) {
      bool cambiato = false;
      if (strstr((char *)e.data, "CMD:SYNC") != nullptr) {
        if (stato != SYNC) {
          stato = SYNC;
          cambiato = true;
        }
      } else if (strstr((char *)e.data, "CMD:TURNO:0") != nullptr) {
        cambiato = applicaTurno(0);
      } else if (strstr((char *)e.data, "CMD:TURNO:1") != nullptr) {
        cambiato = applicaTurno(1);
      }

      // CRUCIALE PER LE PRESTAZIONI:
      // Aggiorniamo il display SOLO se lo stato è cambiato veramente!
      // In questo modo i messaggi di heartbeat non bloccano il bus I2C né accumulano delay nella coda.
      if (cambiato) {
        aggiornaDisplay();
        Serial.printf("[RX da " MACSTR "]: %s -> Nuovo Stato: %s\n",
                      MAC2STR(e.from), (char *)e.data,
                      (stato == SYNC ? "SYNC" : (stato == VERDE ? "VERDE" : "ROSSO")));
      }
    }
    break;

  default:
    break;
  }
}

void setup() {
  IoTBoard::init_serial();
  IoTBoard::init_leds();
  IoTBoard::init_display();

  // Velocizza il bus I2C a 400kHz per ridurre i tempi di rendering dello schermo OLED
  Wire.setClock(400000);

  display->clearDisplay();
  display->setCursor(0, 0);

  // Legge il MAC address hardware
  esp_read_mac(myMac, ESP_MAC_WIFI_STA);

  // Determina ruolo e gruppo automaticamente dal MAC
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           myMac[0], myMac[1], myMac[2], myMac[3], myMac[4], myMac[5]);

  if (strcmp(macStr, MAC_0) == 0) {
    SONO_ROOT = true;
    turno = 0;
  }

  if (strcmp(macStr, MAC_0) == 0 || strcmp(macStr, MAC_2) == 0) {
    gruppo = 0;
  } else if (strcmp(macStr, MAC_1) == 0 || strcmp(macStr, MAC_3) == 0) {
    gruppo = 1;
  } else {
    Serial.println("Errore - MAC non riconosciuto!");
  }

  EspMeshConfig config;
  config.router_ssid = "";       // Standalone senza AP esterno
  config.on_event = onMesh;      // Callback eventi mesh
  config.channel = 11;           // Stesso canale radio per tutte le board
  config.mesh_id[5] = ID_GRUPPO; // Stesso ID gruppo per tutte le board
  config.role = SONO_ROOT ? EspMeshRole::Root : EspMeshRole::Node;

  if (IoTBoard::init_espmesh(config)) {
    // Timeout AP a 1 secondo su tutte le board per rilevamento immediato nodi caduti
    esp_mesh_set_ap_assoc_expire(1);

    // Timeout di invio ridotto a 50ms (invece dei 5000ms di default) per non bloccare il loop
    // se un nodo non risponde immediatamente
    esp_mesh_send_block_time(50);

    display->println("Mesh avviata!");
    display->printf("MAC: %s\n", macStr);
    display->printf("Ruolo: %s (G%d)\n", SONO_ROOT ? "ROOT" : "NODO", gruppo);
  } else {
    display->println("Errore avvio Mesh");
  }
  display->display();
}

void loop() {
  espmesh->update();

  if (SONO_ROOT) {
    // 1. Controllo periodico della topologia (ogni 500ms sul Root)
    if (IoTBoard::runEvery(500, 2)) {
      uint8_t nodes[10][6];
      numNodi = espmesh->getNodes(nodes, 10);

      // CASO A: Rete completa (4/4) ed eravamo in SYNC -> Avvio semafori
      if (numNodi == NUM_BOARD && stato == SYNC) {
        Serial.printf("[ROOT] Rete completa (%d/%d)! Avvio semafori al turno %d\n",
                      numNodi, NUM_BOARD, turno);
        applicaTurno(turno);
        String cmd = (turno == 0) ? "CMD:TURNO:0" : "CMD:TURNO:1";
        inviaATutti(cmd);
        aggiornaDisplay();
      }
      // CASO B: Manca qualche board ed eravamo attivi -> Torno subito a SYNC
      else if (numNodi < NUM_BOARD && stato != SYNC) {
        Serial.printf("[ROOT] Mancano nodi (%d/%d)! Torno a STATO = SYNC\n",
                      numNodi, NUM_BOARD);
        stato = SYNC;
        inviaATutti("CMD:SYNC");
        aggiornaDisplay();
      }
    }

    // 2. Heartbeat ogni 2000ms:
    // Mantiene allineate le board senza intasare la rete o le code FreeRTOS
    if (IoTBoard::runEvery(2000, 3)) {
      if (numNodi < NUM_BOARD) {
        inviaATutti("CMD:SYNC");
      } else if (stato != SYNC) {
        String cmd = (turno == 0) ? "CMD:TURNO:0" : "CMD:TURNO:1";
        inviaATutti(cmd);
      }
    }

    // 3. Alternanza del turno semaforico ogni 5 secondi (solo se la rete è al completo)
    if (IoTBoard::runEvery(5000, 0)) {
      if (numNodi == NUM_BOARD && stato != SYNC) {
        turno = (turno == 0) ? 1 : 0;
        applicaTurno(turno);
        String cmd = (turno == 0) ? "CMD:TURNO:0" : "CMD:TURNO:1";
        inviaATutti(cmd);
        aggiornaDisplay();
        Serial.printf("[ROOT] Cambio turno -> %d (Gruppo 0: %s, Gruppo 1: %s)\n",
                      turno, (turno == 0 ? "VERDE" : "ROSSO"), (turno == 1 ? "VERDE" : "ROSSO"));
      }
    }
  }

  // 4. Gestione centralizzata dei LED in base allo stato
  switch (stato) {
  case SYNC:
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);
    // Lampeggio giallo ogni 500ms
    if (IoTBoard::runEvery(500, 1)) {
      digitalWrite(LED_YELLOW, !digitalRead(LED_YELLOW));
    }
    break;

  case VERDE:
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, HIGH);
    break;

  case GIALLO:
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_YELLOW, HIGH);
    break;

  case ROSSO:
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_RED, HIGH);
    break;
  }
}