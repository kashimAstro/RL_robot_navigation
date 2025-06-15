# Autonomous Navigation Reinforcement Learning Network (Rete di Rinforzo per la Navigazione Autonoma)

[English](README.md) | [Italiano](README.it.md)

## Italiano 

### Panoramica del Progetto 

Questo progetto presenta un sistema di apprendimento per rinforzo (Reinforcement Learning) progettato per la navigazione autonoma in ambienti complessi con ostacoli. Utilizza una piccola rete neurale feed-forward e un agente epsilon-greedy per trovare percorsi ottimali da un punto di partenza a uno di arrivo.

Un punto saliente è la dimostrazione pratica con un piccolo robot basato su ESP8266, programmato con l'ambiente Arduino, che offre un esempio tangibile delle capacità del sistema.

Sviluppato in C++ con un numero minimo di dipendenze, il codice è stato progettato per essere estremamente facile da comprendere e utilizzare, rendendolo ideale per scopi didattici e l'apprendimento pratico.

### Funzionalità

* **Agente di Reinforcement Learning:** Piccola rete feed-forward con politica epsilon-greedy.
* **Ricerca Percorsi:** Determinazione del percorso ottimale in ambienti basati su griglia..
* **Poche Dipendenze:** C++ con librerie esterne minime.
* **Focus Didattico:** Codice pulito e comprensibile, adatto all'apprendimento.
* **Integrazione Hardware:** Esempio di implementazione con un robot ESP8266.

### Demo
* **Inferenza con Simulazione in Bash:**

    ![Video Sample 0](video_sample_0.gif)

* **Inferenza con Comunicazione HTTP con Robot:**
    ![Video Sample 1](video_sample_1.gif)

    ```bash
    ./rl_inference --load-file weights/rl_weights_10x10map_simple.csv --load-grid maps/10x10s.txt --ip-esp 192.168.1.132 --step-lin 450 --step-rot 1100
    ```

    *Nota: Altri esempi sono disponibili nella cartella `weights` insieme alle `mappe` corrispondenti.*

### Dipendenze

* `eigen3` (su Debian/Ubuntu: `sudo apt install libeigen3-dev`)
* `curl` [Opzionale, solo per l'esempio ESP8266] (su Debian/Ubuntu: `sudo apt install libcurl3-gnutls`)

### Come Iniziare

#### Compilazione

Per compilare il progetto, usa i seguenti target Make:

* `make train` - Compila l'applicazione di training.
* `make inference` - Compila l'applicazione di inferenza.
* `make map` - Compila lo strumento per la creazione delle mappe. 

#### Creazione Mappe 

Prima di procedere con il training, devi creare una mappa per il tuo ambiente. Usa un editor di immagini (ad esempio GIMP) per disegnare una piccola mappa.

* **Linee Guida per le Mappe:**
    * **Dimensione Massima:** Non superare i 30x30 pixel.
    * **Sfondo:** Nero.
    * **Ostacoli:** Bianco.
    * **Partenza Robot:** Verde.
    * **Punto di Arrivo:** Rosso.

* **Elaborazione:** Salva la tua immagine (es. 12x20map.png) e poi elaborala con il comando `create_map` per generare una rappresentazione della griglia basata su testo:

    ```bash
    ./create_map maps/12x20map.png > 12x20.txt
    ```

    Questo comando genera un array 2D che rappresenta la mappa con gli ostacoli, inizio, e fine, da passare `--load-grid` parametro.
    **Esempio mappa (2D array):**
    ```
    1 1 0 0 0
    3 1 2 0 0
    0 1 1 0 0 
    0 0 1 1 0
    0 0 0 0 0
    ```

    **Alcuni esempio di mappe:**
    <table>
      <tr>
        <td> <img style="display: inline-block; vertical-align: middle; margin-right: 10px;" width="50px" src="mappe/10x10map.png"/></td>
     <td> <img style="display: inline-block; vertical-align: middle; margin-right: 10px;" width="50px" src="mappe/12x12map.png"/></td>
     <td> <img style="display: inline-block; vertical-align: middle; margin-right: 10px;" width="50px" src="mappe/12x20map.png"/></td>
     <td> <img style="display: inline-block; vertical-align: middle; margin-right: 10px;" width="50px" src="mappe/15x15map_2.png"/></td>
     <td> <img style="display: inline-block; vertical-align: middle; margin-right: 10px;" width="50px" src="mappe/15x20map.png"/></td>
     <td> <img style="display: inline-block; vertical-align: middle; margin-right: 10px;" width="50px" src="mappe/18x15map.png"/></td>
     <td> <img style="display: inline-block; vertical-align: middle; margin-right: 10px;" width="50px" src="mappe/30x30map.png"/></td>
     <td> <img style="display: inline-block; vertical-align: middle; margin-right: 10px;" width="50px" src="mappe/5x5map.png"/></td>
     <td> <img style="display: inline-block; vertical-align: middle; margin-right: 10px;" width="50px" src="mappe/6x30map.png"/></td>
      </tr>
    </table>

#### Training del Modello
Addestra il tuo modello di Reinforcement Learning usando il comando `rl_train`:

```bash
./rl_train --save-file rl_weights_5x5map.csv --load-grid maps/5x5.txt --num-epochs 5000
```

* **--save-file**: Percorso per salvare i pesi addestrati.
* **--load-grid**: Percorso del file della mappa generata.
* **--num-epochs**: Numero di epoche di training.

#### Esecuzione dell'Inferenza
Una volta addestrato, puoi eseguire l'inferenza per vedere il robot navigare:

```bash
./rl_inference --load-file rl_weights_5x5map.csv --load-grid maps/5x5.txt
```

* **--load-file**: Percorso del file dei pesi addestrati.
* **--load-grid**: Percorso del file della mappa usata per l'inferenza.

Esecuzione dell'Inferenza con Robot ESP8266 (Comunicazione HTTP)
Per la dimostrazione con il robot ESP8266 via HTTP:

```bash
./rl_inference --load-file weights/rl_weights_18x15map.csv --load-grid maps/18x15.txt --step-lin 2500 --step-rot 1000 --ip-esp '192.168.1.132'
```

* **--step-lin**: Passi di movimento lineare per il robot.
* **--step-rot**: Passi di movimento rotazionale per il robot.
* **--ip-esp**: Indirizzo IP del tuo robot ESP8266.
