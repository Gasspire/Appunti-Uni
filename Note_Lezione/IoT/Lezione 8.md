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
