## Bluetooth

WPAN: protocolli per collegare dispositivi personali.

Nasce nel 99 dall'idea di sostituire i cavi.  UART wireless

Nascita di bluetooth e linea temporale. Bluetooth low energy versione 5 nelle schede.![[Pasted image 20261110172635.png]]

Un solo client collegato a più server, è così che funziona Bluetooth. 

## Funzionamento
Utilizziamo una **piconet** c'è un dispositivo centrale (non un serve ma un client) e poi abbiamo altre periferiche che si collegano (server).

L'unità centrale gestisce l'ordine con cui parlare con i server (cioè con le periferiche). Posso espandere la piconet con un bridge, altrimenti no. è sempre una SOLA unità centrale.

Le cuffie non sono totalmente BLE. (in realtà in bluetooth abbiamo nello stack lo streaming output)![[Pasted image 20261110173727.png]]

Opera sulla stessa banda di Wifi.
![[Pasted image 20261110173908.png]]

L2CAP si occupa della segmentazione e del controllo di flusso.  
RFCOMM simula la seriale mediante bluetooth.
SDP quello per il suono
SPP, HFP e HID sono layer fatti apposta che permettono la comunicazione seriale hid simula la tastiera (ad esempio)

2.4 GHz è la più utilizzata perché è libera e in termini di disturbi e distanza è ggwp. L'antenna poi già c'è.
Attualmente le onde elettromagnetica è quella più sensata per il momento per il wireless

Bluetooth è cifrato
Pairing/Bonding

## BLE
Definiamo dei servizi per le periferiche. 
inizialmente diversa da Bluetooth, poi integrata.
![[Pasted image 20261110175729.png]]

Bluetooth normale si utilizza ancora per le macchine (?) o per trasferimento file o per audio.

![[Pasted image 20261110175843.png]]
Protocollo
![[Pasted image 20261110180031.png]]

GAP: quello che definisce il ruolo, la discovery e le connessioni.
ATT: è un blocco che permette a un dispositivo cosa è un servizio e cosa è un attributo.
SMP: si occupa del pairing, scambio chiavi ecc.
L2CAP permette di comunicare ATT e SMP
GATT è quello che ci permette di definire il servizio e ciò che è caratteristica del servizio.

#### GAP
![[Pasted image 20261110180229.png]]

#### Advertising
![[Pasted image 20261110180452.png]]

esiste una versione estesa che dovrebbe riuscire ad avere un PDU di 1600 Byte.

In ADV data possiamo avere:
![[Pasted image 20261110180600.png]]

GAT
![[Pasted image 20261110180716.png]]

Il gestore è un facade
Posso creare i miei servizi 

Notify è il permesso che ha reso broken BLE

GATT funziona con ATT
![[Pasted image 20261110182003.png]]