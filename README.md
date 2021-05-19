# Programmable-pets-feeder
Programmable pets feeder consiste in una ciotola per cani cui l'oario di apertura è programmabile tramite bluetooth(app), da presentare come progetto del corso di [Sistemi embedded](https://gitlab.di.unimi.it/sistemiembedded/2019-2020). 

## Introduzione
L'idea alla base del progetto deriva da un problema quotidiano di un mio conoscente: avendo un cane e lavorando fino a tarda serata, il padrone si trova costretto a dover dare da mangiare in tarda serata anche al suo animale domestico. Per risolvere questa grana, ho pensato a una ciotola che possa essere riempita di croccantini a qualsiasi orario, chiudersi e riaprirsi automaticamente solo ad un orario programmato in precedenza.

_Nota: l'idea iniziale era quella di aggiungere più ciotole per la possibilità di avere molteplici pasti disponibili ma comportava un contenitore ingombrante e poco utile dati gli altri bisogni di un cane durante la giornata che non possono essere assolit senza l'ausilio del padrone._

_Nota 2: Senza alcun motivo ai fini della risoluzione del problema iniziale sono stati aggiunti un sensore di temperatura e un sensore di luminosità al circuito_
## Componentistica
* Arduino uno
* Motore passo passo (28byj-48 5V DC)
* Driver motor (ULN2003)
* Display lcd (LCD1602)
* Sensore bluetooth (HC-05)
* Fotoresistenza
* Sensore di temperatura
* 3 Resistenze 10k ohm
* 1 resistenza 220 ohm
* 2 resistenze 3k ohm

## Descrizione
Il sistema è composto da un'applicazione android e dallo sketch che gira su microcontrollore Arduino uno.
All'esterno della scatola si trova:
* Un braccio in legno fissato all'albero di un motore passo passo che ha lo scopo di far girare un coperchio circolare in modo da coprire e scoprire la ciotola del cane(bilanciato con dei piombini da pesca)
* Un display led con lo scopo di mostrare lo stato del sistema
* Un bottone di accensione/spegnimento display led
* Un bottone per azionare il coperchio(sia apertura nel momento in cui si vogliono mettere i croccantini, che chiusura)
* Un fine corsa (composto da materiale conduttore) che funziona come interruttore permettendo al sistema di avere un segnale di feedback nel momento in cui il coperchio è chiuso completamente.

### Immagine Scatola
[![Scatola](https://github.com/Demacri/Programmable-pets-feeder/blob/main/img/scatola_davanti.jpg?raw_true)]
### Screenshot App (in funzione)
[![Screenshot App](https://github.com/Demacri/Programmable-pets-feeder/blob/main/img/screenshot_app.jpg?raw=true)]

_Altre immagini si trovano nella directory /img_

## Funzionamento
L'alimentazione al sistema è fornita tramite cavo usb(che esce dalla scatola), appositamente tagliato e collegato direttamente ai 5V di Arduino(**saltando così le protezioni interne di quest'ultimo, bisogna assicurarsi che la tensione sia corretta sui 5V**). Appena il circuito viene alimentato il sistema è funzionante, il display rimane acceso per 15secondi mostrando la scritta "Pross. Apertura \ Not Set", dopodiché si spegnerà. Il bottone più a destra della scatola permette di accendere e spegnere il display. Per riempire la ciotola basterà premere il bottone più a sinistra che aprirà il coperchio, mostrerà l'informazione di apertura sul display e permetterà l'aggiunta dei croccantini nella ciotola. Una volta riempita la ciotola, ripremendo il bottone, inizierà la chiusura del coperchio che terminerà nel momento in cui la piastra metallica laterale del coperchio toccando con la piastra di finecorsa sulla scatola chiuderà un semplicissimo circuito "informando" l'arduino dell'avvenuta chiusura.

_Nota: nonostante la precisione dei motori passo passo ho voluto aggiungere questo sistema di feedback per i casi in cui il braccio del coperchio viene spostato manualmente per errore._

Prima di uscire di casa, aprendo l'applicazione sul proprio smartphone vengono mostrate alcune informazioni dai sensori di temperatura e luminosità e l'orario in cui il coperchio dovrà essere aperto(Inizialmente questo valore è nullo; "non impostato"), compilando la casella di testo che viene mostrata con un orario e premendo il bottone "salva" viene inviata l'informazione al sensore bluetooth collegato all'arduino il quale salva l'orario e aggiorna l'informazione presente sul display: "Pross. apertura \ hh:mm". Premendo il tasto "Ricarica" si può richiedere un aggiornamento delle informazioni al sistema(arduino)

_Nota: non avendo un modo per tenere salvato in maniera permanente l'orario con Arduino, all'avvio dell'app viene inviato, in maniera trasparente all'utente, l'orario corrente dello smartphone all'arduino che lo memorizza e lo utilizza come punto di riferimento per calcolare le successive richieste di data/ora corrente._

Ogni minuto viene controllato se l'ora corrente è uguale all'orario impostato e in quest'ultimo caso viene aperto il coperchio e il nostro adorato cane affamato potrà mangiare i suoi deliziosi croccantini!