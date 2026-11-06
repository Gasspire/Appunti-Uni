## Circuiti Slave

Autoreload Register
Caputre reload register 

Questi sono circuiti slave ^

![[Pasted image 20261106112053.png]]

## PWM output
![[Pasted image 20261106112135.png]]
PSC e ARR sono da inizializzare e poi è da programmare il cirucito slave in termini di PWM (?)

Il registro CCR serve a qualcosa

Esiste la funzione TIM_CONFIG_PWM() 
![[Pasted image 20261106112319.png]]

Una volta che si accende il timer su TIMX_CHY spunta fuori il segnale 
Alternate function:
ogni pin ha la possibilità di comportarsi come GPIO ma quando lo impostiamo per alternate function, ogni pin può avere fino a 16 comportamenti diversi (ciò implica che il pin è collegato a periferiche diverse)
Table 9 alternate function (STM32)
![[Pasted image 20261106124417.png]]
## Input Capture

//vedi codici
