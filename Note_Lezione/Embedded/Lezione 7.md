Un timer è un **circuito hardware** dedicato che fornisce al software la nozione del tempo. È basato su un registro **contatore che viene incrementato o decrementato automaticamente** da una **sorgente di clock** (spesso programmabile). 

Dal punto di vista del software, **il contatore appare come una variabile globale leggibile e modificabile**, in grado di **generare interrupt e di pilotare o misurare segnali elettrici tramite circuiti accessori detti circuiti Slave.** 
- **Il Timer (modulo principale):** Si occupa solo di scandire il tempo incrementando o decrementando il registro contatore.
- **I circuiti accessori / slave:** Sono moduli hardware fisicamente collegati al contatore che utilizzano il suo valore per eseguire operazioni specifiche verso l'esterno

il timer riceve il segnale di clock per incrementare/decrementare il contatore in vari modi:
- **Internal:** Clock di sistema del microcontrollore (es. oscillatore interno o quarzo).
- **External:** Un segnale generato da un circuito esterno applicato a un pin dedicato.
- **Other:** Clock derivato da un altro timer interno (in configurazione master/slave tra timer).

Nel nostro STM32 abbiamo:
- **16-bit:** Può contare fino a $2^{16} - 1 = 65.535$. (TIM3, TIM4)
- **32-bit:** Può contare fino a $2^{32} - 1 = 4.294.967.295$. (TIM2, TIM5)

Un **programmable prescaler** È un circuito divisore di frequenza interno al timer:
- Prende la frequenza del clock in ingresso e la divide per un valore scelto da software (es. $/1, /2, /4, /64, \dots$).
- **Scopo:** Rallentare la velocità di conteggio del timer per misurare tempi più lunghi senza cambiare l'oscillatore principale del sistema.
![[Pasted image 20261108163600.png]]

Gli interrupt del timer possono generare vari eventi:
- **Overflow / Underflow** (il contatore ha raggiunto il massimo o il minimo).
- **Evento di cattura** (Input Capture su uno dei canali).
- **Evento di confronto raggiunto** (Output Compare / PWM)


Un timer ha diversi SFR e delle variabili globali note come **TIMX** che puntano agli SFR.

La struttura hardware di un timer è la seguente:
![[Pasted image 20261108163725.png]]

######  Sorgente di Clock
![[Pasted image 20261108164200.png]]
Per quanto riguarda la sorgente di clock abbiamo diverse opzioni tra cui scegliere (i multiplexer decidono da dove far arrivare il clock):
- **Internal (System Peripheral Clock, default setting):** È il clock di default fornito dall'unità RCC (Reset and Clock Control) del microcontrollore (`CK_INT`). Il timer incrementa a una frequenza derivata dal clock principale di sistema.
- **External (External Pin):** Il clock non viene dal bus interno, ma da un segnale elettrico esterno collegato al pin dedicato `TIMx_ETR` (External Trigger).
- **External in QEI mode (Quadrature-encoder interface):** Il clock viene generato dalla lettura di un encoder rotativo tramite i segnali `TI1FP1` e `TI2FP2` inviati al blocco `Encoder Interface`. Il contatore incrementa o decrementa a seconda della direzione di rotazione del motore/manopola.
- **Gate/Trigger inputs:** Segnali d'ingresso configurabili usati per avviare, fermare o resettare il conteggio in risposta a determinati eventi esterni o interni (es. tramite il multiplexer degli `ITR` - Internal Trigger).

######  Time base

Stabilita la sorgente di clock abbiamo:
![[Pasted image 20261108164349.png]]
Analizziamo i principali registri:
- **`TIMx->PSC` (Prescaler Register):**
    - **Funzione:** Riceve il clock d'ingresso (`CK_PSC`) e lo divide per rallentare il conteggio.
    - **Dettaglio operativo:** Specifica il valore del fattore di divisione meno uno ($\text{Fattore} - 1$). Se ad esempio si scrive $PSC = 7$, la frequenza in ingresso viene divisa per $8$ ($7+1$).
    - Il clock diviso in uscita si chiama **`CK_CNT`** ed è il segnale che fa effettivamente scattare i conteggi del registro contatore.
- **`TIMx->CNT` (Counter Register):**
    - **Funzione:** È il registro che memorizza il valore attuale del conteggio.
    - Ad ogni colpo del clock `CK_CNT`, incrementa o decrementa di $1$ il suo valore interno.
- **`TIMx->ARR` (Auto-Reload Register):**
    - **Funzione:** Stabilisce il **valore limite (periodo)** del conteggio.
    - In modalità _up-counting_ (conteggio crescente), il contatore `CNT` parte da $0$ e incrementa fino a raggiungere il valore impostato in `ARR`. Raggiunto tale limite, `CNT` si resetta automaticamente a $0$ e ricomincia da capo.

Questi, come avevamo già detto, possono generare diverse tipologie di eventi:
- **Update Event ("U" nello schema):**
    - Si verifica esattamente nel momento in cui `CNT` raggiunge il limite `ARR` e viene ricaricato/azzerato (o quando torna al massimo in conteggio decrescente).
- **Update Interrupt ("UI" nello schema):**
    - L'evento di update (`U`) genera un segnale interno che può essere configurato dal software per sollevare un **interrupt (UI)** verso la CPU. Questo consente di eseguire codice periodicamente in corrispondenza del riazzeramento del timer.

Nella nostra libreria dobbiamo utilizzare i timer da *TIM2* a *TIM4* per evitare problemi perché il TIM5 è usato per gestire il display.

Abbiamo diverse funzioni:
- **`TIM_init(TIM_TypeDef * timer);`**
    - **Scopo:** Inizializza l'hardware del timer passato come argomento (es. abilita il segnale di clock periferico tramite RCC e predispone le configurazioni di base).
- **`TIM_config_timebase(TIM_TypeDef * timer, uint32_t prescaler, uint32_t autoreload);`**
    - **Scopo:** Configura i due parametri fondamentali della _time-base unit_ mostrata nello schema in alto:
        - `prescaler`: Imposta il valore nel registro **`PSC`** per dividere la frequenza del clock d'ingresso.
        - `autoreload`: Imposta il valore nel registro **`ARR`**, cioè il limite massimo a cui arriva il contatore `CNT` prima di resettarsi e generare l'evento di update (`U`).
- **`TIM_on(TIM_TypeDef * timer);`**
    - **Scopo:** Avvia il conteggio del timer (imposta il bit di abilita/start nel registro di controllo del timer). Da questo momento il registro `CNT` inizia ad incrementare.
- **`TIM_off(TIM_TypeDef * timer);`**
    - **Scopo:** Ferma il conteggio del timer (disabilita il timer), congelando il valore corrente di `CNT`.
- **`int32_t TIM_get(TIM_TypeDef * timer);`**
    - **Operazione:** Legge il valore attuale del contatore hardware (`TIMx->CNT`).
    - **Uso pratico:** Ritorna il conteggio istantaneo raggiunto dal timer.
- **`void TIM_set(TIM_TypeDef * timer, int32_t value);`**
    - **Operazione:** Scrive un valore specifico direttamente nel registro del contatore (`TIMx->CNT`).
    - **Uso pratico:** Serve per forzare il contatore a partire da un determinato numero anziché da 0.
- **`int TIM_update_check(TIM_TypeDef * timer);`**
    - **Operazione:** Interroga (tramite _polling_) il flag di notifica dell'evento di update (`U` nello schema).
    - **Uso pratico:** Restituisce `1` (o vero) se il contatore ha raggiunto il limite `ARR` ed è avvenuto il reset/reload, altrimenti restituisce `0`.
- **`void TIM_update_clear(TIM_TypeDef * timer);`**
    - **Operazione:** Riconfigura a `0` il flag hardware dell'evento di update (pulisce la notifica).
    - **Uso pratico:** Deve essere chiamata dopo aver rilevato l'evento di update; in caso contrario, la notifica rimarrebbe sempre attiva indicando un falso nuovo evento.


Supponiamo di volere una notifica ogni 500 ms (o un lampeggio di un led):

Innanzitutto:
- $f_{clock} = 84.000.000 = 84 \cdot 10^6$ Hz -> Vuol dire che ogni secondo, il nostro contatore fa $84 \cdot 10^6$ scatti.
- $T_{clock} = \frac{1}{f_{clock}} = 11,9$ ns, cioè facciamo uno scatto ogni 11,9 nano secondi.

Dividiamo allora la frequenza di clock grazie al nostro pre scaler:
- $f_{cnt} = \frac{f_{clock}}{fattore} = \frac{84 \cdot 10^6}{84 \cdot 10^2} = 10^4$ Hz -> Cioè, adesso il nostro contatore farà 10000 scatti al secondo.
- $t_{cnt} = \frac{1}{f_{cnt}} = 10^{-4} = 0,0001$ s -> Cioè facciamo uno scatto ogni $0,1$ ms.

Se vogliamo quindi una notifica ogni 500 ms abbiamo bisogno di $N = \frac{500}{0,1} = 5000$ scatti a cui imposteremo il nostro autoreload.
