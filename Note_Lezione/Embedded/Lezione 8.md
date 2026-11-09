I circuiti slave possono essere utilizzati per:
- **Generare particolari segnali periodici.**
- **Misurare il periodo o la durate degli impulsi d'ingresso.**

![[Pasted image 20261108180640.png|700]]
Ogni timer ha quattro canali slave ognuno dei quali può essere connesso a un pin GPIO e può essere usato per programmare:
- **Input capture:** Cattura l'istante esatto (il valore del contatore) in cui si verifica una transizione di segnale sul pin.
- **PWM input measurement:** Utilizza due canali per misurare contemporaneamente la frequenza e il _duty cycle_ di un segnale PWM entrante.
- **Output compare:** Cambia lo stato del pin (HIGH/LOW) quando il contatore raggiunge un valore prefissato.
- **PWM output generation:** Genera un'onda quadra continua con frequenza e _duty cycle_ programmabili per pilotare componenti esterni.
Ogni canale ($y = 1, 2, 3, 4$) possiede un proprio registro dedicato chiamato `CCR1`, `CCR2`, `CCR3` o `CCR4`:
- In **Input Capture**, quando arriva un segnale sul pin, l'hardware copia istantaneamente il valore corrente di `CNT` dentro `CCRy`.
- In **Output Compare / PWM**, il software scrive un valore di confronto dentro `CCRy`. L'hardware confronta continuamente `CNT` con `CCRy` e modifica lo stato del pin quando i due valori coincidono.

## PWM output
![[Pasted image 20261108181022.png]]
**Il Periodo (`ARR`):**
- Il contatore `CNT` parte da $0$ e incrementa ad ogni impulso fornito dal prescaler (`PSC`).
- Il comparatore in alto confronta continuamente `CNT` con il valore contenuto in **`ARR`**.
- Quando `CNT` raggiunge `ARR`, viene generato un **Update Event**, il contatore `CNT` si resetta a $0$ e il ciclo si ripete. Questo definisce il **periodo totale** dell'onda PWM.
**Il Duty Cycle (`CCRy`):**
- Il valore contenuto nel registro **`CCRy`** stabilisce il valore di soglia per il cambio di stato del segnale in uscita.
- Finchè `CNT < CCRy`, il segnale sul pin `TIMx_CHy` rimane ad livello logico ALTO (HIGH).
- Quando `CNT` supera `CCRy`, l'uscita passa a livello logico BASSO (LOW) e vi rimane fino a quando `CNT` non si azzera al termine del periodo

Un canale $y$ può essere configurato in uscita per produrre un segnale PWM con periodo (frequenza) e _duty cycle_ specifici.
Il **periodo** dell'onda (la durata totale dell'impulso ALTO + BASSO) è espresso in numero di conteggi e viene impostato nel registro **`ARR`**.
Il **duty cycle** rappresenta la durata della parte positiva del segnale (livello ALTO). Anch'esso viene espresso in numero di conteggi e si imposta nel registro **`CCRy`** del canale corrispondente.

**`void TIM_config_PWM(TIM_TypeDef * tim, int channel);`**
- **Funzione:** Configura uno specifico canale slave del timer per la modalità di generazione PWM.
- **Parametri:**
    - `tim`: Il puntatore al timer da utilizzare (es. `TIM2`, `TIM3`, `TIM4`).
    - `channel`: Il canale da attivare in uscita, con valore compreso da **1 a 4** (corrispondente a `TIMx_CH1` ... `TIMx_CH4`).
- Non c'è un parametro di periodo dentro la funzione `TIM_config_PWM`.
- Il periodo viene stabilito separatamente impostando il valore di `autoreload` (`ARR`) tramite la funzione di timebase vista in precedenza: `TIM_config_timebase(...)`
- l _duty cycle_ (la durata dell'impulso ALTO) si regola assegnando direttamente un valore al registro hardware **`CCRy`** del canale scelto (es. `TIM2->CCR1 = 2500;` per il canale 1)



ogni pin ha la possibilità di comportarsi come GPIO ma quando lo impostiamo per alternate function, ogni pin può avere fino a 16 comportamenti diversi (ciò implica che il pin è collegato a periferiche diverse)
Table 9 alternate function (STM32):
![[Pasted image 20261106124417.png]]
Nel codice che vedremo tra poco, infatti, noteremo come abbiamo attiveremo la AF02 di PB0 per collegare quel pin al TIM3.
![[Pasted image 20261108182916.png]]![[Pasted image 20261108182931.png]]

## Input Capture

![[Pasted image 20261108183114.png]]
Quando un canale viene usato in _input capture_, il pin GPIO corrispondente deve essere configurato come **ingresso** (`input`).
Il circuito hardware rileva il cambio di stato del segnale in ingresso:
- **Rising-Edge (Fronte di salita):** Il segnale passa da LOW ($0\text{V}$) a HIGH ($3.3\text{V}$).
- **Falling-Edge (Fronte di discesa):** Il segnale passa da HIGH ($3.3\text{V}$) a LOW ($0\text{V}$).

Nell'istante esatto in cui si verifica il fronte (di salita o di discesa), l'hardware **fotografa** il valore corrente del registro contatore (`CNT`) e lo copia automaticamente nel registro del canale (`CCRy`, nell'immagine `CCR1`).
Insieme alla copia del valore, viene generata un'interruzione hardware (**capture-event interrupt** / IRQ), notificando subito la CPU.

**Funzioni di libreria**
- **`TIM_config_IC(TIM_TypeDef * tim, int channel, IC_Mode mode)`**
    - **Scopo:** Configura un canale del timer per catturare il valore del contatore in corrispondenza di un evento sul pin d'ingresso.
    - **Parametri:**
        - `tim`: Il timer da usare (es. `TIM2`, `TIM3`).
        - `channel`: Il canale slave associato al pin ($1 \dots 4$).
        - `mode`: Specifica quale transizione di segnale deve attivare la cattura:
            - `IC_RISING_EDGE`: Scatta quando il segnale passa da LOW ad HIGH (fronte di salita).
            - `IC_FALLING_EDGE`: Scatta quando il segnale passa da HIGH a LOW (fronte di discesa).
            - `IC_BOTH_EDGES`: Scatta su qualsiasi variazione di stato del segnale.
- **`TIM_cc_check(TIM_TypeDef * tim, int channel)`**
    - **Scopo:** Verifica (tramite polling) se si è verificato un evento di cattura sul canale specificato.
    - **Ritorna:** `1` (true) se è arrivato un fronte ed è stato salvato il valore di `CNT` nel registro `CCRy`, oppure `0` (false) se non è successo nulla.
- **`TIM_cc_clear(TIM_TypeDef * tim, int channel)`**
    - **Scopo:** Azzera il flag hardware di notifica dell'evento di cattura per il canale specificato.
    - **Perché serve:** Va chiamata sempre dopo aver gestito la lettura dell'evento; in caso contrario il flag rimarrebbe attivo, indicando falsi eventi ai cicli successivi.

Reaction game:
![[Pasted image 20261108185643.png]]![[Pasted image 20261108185655.png]]![[Pasted image 20261108185705.png]]