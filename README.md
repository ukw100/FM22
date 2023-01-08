# DCC-Zentrale FM22
## _DCC-Zentrale - auch für Mittelleiter-Systeme_

This project is still in the process of development. A translation into English will be done as soon as the documentation is complete.

Dieses Projekt ist noch in der Entstehung. Eine Übersetzung ins Englische wird erfolgen, sobald die Dokumentation vollständig ist.

![DCC-FM22 Loksteuerung](https://raw.githubusercontent.com/ukw100/FM22/main/images/lok1.png "Loksteuerung")

## Funktionen

- RailCom®-fähige DCC-Zentrale
- System: Raspberry Pi4 als Zentrale, STM32 als DCC-Controller
- Bedienung über Webbrowser: Direkt am RasPi, Smartphone, Tablet oder auch PC
- Multiuser-System: Bedienung durch mehrere Personen gleichzeitig möglich
- Automatisierte Abläufe über Lok-Makros und durch S88/RailCom ausgelöste Aktionen
- Feststellung des Aufenthaltsortes aller Loks über RailCom
- Software-Lösungen für Pendelzugsteuerung, Bremsstrecken, Blocksteuerungen usw.
- Komfortable Konfiguration von Fahrstraßen, wie z.B. für Schattenbahnhöfe, Bahnhöfe und zugbezogene Fahrstrecken
- Einfache Konfiguration von komplexen Ablaufsteuerungen über S88-Module und lokale RailCom-Detektoren
- Konfiguration von DCC-Decodern über Programmiergleis
- Konfiguration von DCC-Decodern über Hauptgleis (POM)
- Sehr einfache und Komfortable Konfiguration von Funktionmapping/Funktionsausgängen für diverse Decoder-Familien

FM22 ist eine RailCom-fähige DCC-Zentrale..

## Inhaltsverzeichnis

* [Aufbau der DCC-Zentrale](#aufbau-der-dcc-zentrale)
* [Notwendige Hardware](#notwendige-hardware)
* [Steuerung](#steuerung)
  * [Lokliste](#lokliste)
  * [Zusatzdecoder](#zusatzdecoder)
  * [Weichen](#weichen)
  * [Fahrstraßen](#fahrstraßen)
  * [Weichentest](#weichentest)
  * [S88-Module](#s88-module)
  * [RailCom®-Module](#railcom-module)
* [Programmierung über Programmiergleis](#programmierung-über-programmiergleis)
  * [PGM Decoder-Info](#pgm-decoder-info)
  * [PGM Decoder-Adresse](#pgm-decoder-adresse)
  * [PGM Decoder-CV](#pgm-decoder-cv)
* [Programmierung über Hauptgleis (POM)](#programmierung-über-hauptgleis-pom)
  * [POM Decoder-Info](#pom-decoder-info)
  * [POM Decoder-Adresse](#pom-decoder-adresse)
  * [POM Decoder-CV](#pom-decoder-cv)
  * [POM Funktionsmapping](#pom-funktionsmapping)
  * [POM Funktionsausgänge](#pom-funktionsausgänge)
* [Zentrale](#zentrale)
  * [Einstellungen](#einstellungen)
  * [Konfiguration Netzwerk](#konfiguration-netzwerk)
  * [Firmware-Updates](#firmware-updates)
* [Anhang](#anhang)

## Aufbau der DCC-Zentrale


## Notwendige Hardware

## Steuerung

Das Menü "Steuerung" bietet folgende Punkte:

![DCC-FM22 Menu Steuerung](https://raw.githubusercontent.com/ukw100/FM22/main/images/menu1.png "Menu Steuerung")

* [Lokliste](#lokliste)
* [Zusatzdecoder](#zusatzdecoder)
* [Weichen](#weichen)
* [Fahrstraßen](#fahrstraßen)
* [Weichentest](#weichentest)
* [S88-Module](#s88-module)
* [RailCom®-Module](#railcom-module)

### Lokliste

Die Lokliste ist wohl die wichtigste Anzeige während des Betriebs. Hier wird angezeigt:

 * ID der Loks
 * Online-Status (grün=online, sonst offline)
 * Bezeichnung der Lok
 * Aktueller Aufenthaltsort der Lok
 * Buttons "MF" und "MH" zum Ausführen von Makros (hier "Fahrt" und "Halt"
 * Adresse der Lok
 * RailCom Channel 2 Übertragungsqualität
 * Ein Button zum Bearbeiten der Eigenschaften - nur sichtbar im Bearbeitungsmodus

Ein Beispiel:

![DCC-FM22 Lokliste](https://raw.githubusercontent.com/ukw100/FM22/main/images/lokliste.png "Lokliste")

Im obigen Beispiel stehen die Loks mit den Nummern (IDs) 0 bis 8 auf der Anlage - gekenzeichnet durch den Online-Status. Die Loks mit den IDs
9 - 12 sind zur Zeit offline. Sie wurden zur Veranschaulichung vom Gleis genommen. Sobald eine dieser Loks auf das Gleis gestellt
wird, ändert sich auch instantan der Online-Status. Dieser wechselt dann auf die Farbe "grün".

Durch die Buttons "MF" und "MH" können automatisiert Abläufe (Macros) gestartet werden, zum Beispiel für "MF" (Macro "Fahrt"):

* Einschalten der Beleuchtung
* Nach 2 Sekunden Einschalten des Betriebsgeräusches
* Nach 4 Sekunden Bahnhofsdurchsage
* Nach 10 Sekunden Schaffnerpfiff
* Nach 15 Sekunden Fahrtaufnahme Erhöhung der Geschwindigkeit über eine Rampendefinition.

Diese war nur ein Beispiel. Insgesamt können bis zu 8 Macros pro Lok definiert werden. Weitere Informationen siehe [Lok-Macros](#lok-macros).

Die Spalte "Adresse" gibt die Lokadresse aus. Die Spalte "RC2" zeigt die Übertragungsqualität von RailCom-Rückmeldungen während des Betriebs.
Solange keine Störungen auf der Anlage sind, sollten hier Werte zwischen 98% und 100% ausgegeben werden. Bei 100% wurde jeder DCC-Befehl korrekt
beantwortet und auch vollständig von der Zentrale "verstanden".

Der Button "Bearbeiten" erscheint nur im Bearbeitungsmodus. Hier können dann die Grundeinstellungen der Lok - wie zum Beispiel die Adresse -
geändert werden.

Durch Klick auf die Lokbezeichnung gelangt man in die Loksteuerung der gewünschten Lok.

### Loksteuerung

Auf dieser Seite kann die ausgewählte Lok gesteuert werden. Dies ist über Smartphone, Tablett, direkt am Pi oder auch mit einem PC möglich.
So können direkt mehrere Personen verschiedene Loks gleichzeitig steuern.

Hier sieht man:

* Lokadresse
* Aufenthaltsort der Lok
* Aktuelle Geschwindigkeit der Lok
* Richtung
* Bezeichnungen und Zustände der Funktionstasten

![DCC-FM22 Loksteuerung](https://raw.githubusercontent.com/ukw100/FM22/main/images/lok1.png "Loksteuerung")

Im obigen Beispiel wird als Lokadresse 1013, als Aufenthaltsort "Einfahrt Schattenbahhnhof", als Geschwindigkeit "60" und als Richtung "vorwärts"
ausgegeben. Außerdem ist gerade die Funktion F0 "Spitzensignal / Schlußlicht rot" aktiv.

Alle Ausgaben können sich sofort ändern, wenn eine andere Person oder eine aktivierte Ereignissteuerung in den Ablauf eingreift. Mittels
Schieberegler kann die Geschwindigkeit und mit den Pfeilen darunter kann die Fahrtrichtung geändert werden. Ein Klick auf das schwarze Quadrat
stellt die Geschwindigkeit auf 0, ein Klick auf den schwarzen Kreis daneben führt einen Notstopp aus, d.h. die Lok bleibt dann sofort stehen - ohne
Berücksichtigung irgendwelcher eingesteller Bremsverzögerungen.

Insgesamt können 8 Lok-Macros definiert und ausgelöst werden, siehe auch [Lok-Macros](#lok-macros).

### Neue Lok

In der Lokliste kann durch Klick auf den Button "Neue Lok" unterhalb der Liste eine neue Lok angelegt werden. Dieser Button erscheint nur
im Bearbeitungsmodus. Es erscheinen dann direkt unterhalb des Buttons Eingabefelder, mittels derer man die neue Lok anmelden kann:

![DCC-FM22 Neue Lok](https://raw.githubusercontent.com/ukw100/FM22/main/images/neue-lok.png "Neue Lok")

Im obigen Beispiel wurde als Name "BR111 S-Bahn" gewählt, die Adresse auf 1042 gesetzt und die Fahrstufen mit "128" angegeben.
Nach Klick auf Speichern erscheint dann die neue Lok in der Lokliste.

### Lok bearbeiten

Klickt man in der Lokliste auf den Button "Bearbeiten", kann man die Grundeinstellungen wie Name und Adresse ändern:

![DCC-FM22 Lok bearbeiten](https://raw.githubusercontent.com/ukw100/FM22/main/images/lok-bearbeiten.png "Lok bearbeiten")

Durch Einstellung der ID kann man auch den gewänschten Lokeintrag in der Liste von oben nach unten und umgekehrt "wandern" lassen,
die Lokliste also neu sortieren.

### Lok-Macros

![DCC-FM22 Lok Macro MF 1](https://raw.githubusercontent.com/ukw100/FM22/main/images/macro1.png "Lok Macro MF 1")

![DCC-FM22 Lok Macro MF 2](https://raw.githubusercontent.com/ukw100/FM22/main/images/macro2.png "Lok Macro MF 2")

![DCC-FM22 Lok Macro MH](https://raw.githubusercontent.com/ukw100/FM22/main/images/macro3.png "Lok Macro MH")

### Zusatzdecoder

![DCC-FM22 Zusatzdecoder](https://raw.githubusercontent.com/ukw100/FM22/main/images/zusatzdecoder.png "Zusatzdecoder")

![DCC-FM22 Zusatzdecoder neu](https://raw.githubusercontent.com/ukw100/FM22/main/images/addon-neu.png "Zusatzdecoder neu")

![DCC-FM22 Zusatzdecoder bearbeiten](https://raw.githubusercontent.com/ukw100/FM22/main/images/addon-bearbeiten.png "Zusatzdecoder bearbeiten")

![DCC-FM22 Zusatzdecoder Lok](https://raw.githubusercontent.com/ukw100/FM22/main/images/lok-addon.png "Zusatzdecoder Lok")

### Weichen

![DCC-FM22 Weichen](https://raw.githubusercontent.com/ukw100/FM22/main/images/weichen.png "Weichen")

### Fahrstraßen

![DCC-FM22 Fahrstraßen](https://raw.githubusercontent.com/ukw100/FM22/main/images/fahrstrassen.png "Fahrstraßen")

![DCC-FM22 Neue Fahrstraße](https://raw.githubusercontent.com/ukw100/FM22/main/images/neue-fahrstrasse.png "Neue Fahrstraße")

![DCC-FM22 Fahrstraße bearbeiten 1](https://raw.githubusercontent.com/ukw100/FM22/main/images/fahrstrasse-bearbeiten-1.png "Fahrstraße bearbeiten 1")

![DCC-FM22 Fahrstraße bearbeiten 2](https://raw.githubusercontent.com/ukw100/FM22/main/images/fahrstrasse-bearbeiten-2.png "Fahrstraße bearbeiten 2")

### Weichentest

![DCC-FM22 Weichentest](https://raw.githubusercontent.com/ukw100/FM22/main/images/weichentest.png "Weichentest")

### S88-Module

![DCC-FM22 S88-Module](https://raw.githubusercontent.com/ukw100/FM22/main/images/s88.png "S88-Module")

![DCC-FM22 S88-Module](https://raw.githubusercontent.com/ukw100/FM22/main/images/s88-edit.png "S88-Module")

![DCC-FM22 S88-Module](https://raw.githubusercontent.com/ukw100/FM22/main/images/aktionen-s88.png "S88-Module")

### RailCom®-Module

![DCC-FM22 RailCom-Module](https://raw.githubusercontent.com/ukw100/FM22/main/images/rcl.png "RailCom-Module")

![DCC-FM22 RailCom-Module](https://raw.githubusercontent.com/ukw100/FM22/main/images/rcl-bearbeiten.png "RailCom-Module")

![DCC-FM22 RailCom-Module](https://raw.githubusercontent.com/ukw100/FM22/main/images/aktionen-rcl.png "RailCom-Module")

## Programmierung über Programmiergleis

![DCC-FM22 Menu Programmierung](https://raw.githubusercontent.com/ukw100/FM22/main/images/menu2.png "Menu Programmierung")

### PGM Decoder-Info

![DCC-FM22 Decoder-Info](https://raw.githubusercontent.com/ukw100/FM22/main/images/pgm-info-3.png " Decoder-Info")

### PGM Decoder-Adresse

![DCC-FM22 Decoder-Info](https://raw.githubusercontent.com/ukw100/FM22/main/images/pgm-addr-2.png " Decoder-Info")

### PGM Decoder-CV

![DCC-FM22 Decoder-CV](https://raw.githubusercontent.com/ukw100/FM22/main/images/pgm-cv-2.png " Decoder-CV")

## Programmierung über Hauptgleis (POM)

### POM Decoder-Info

![DCC-FM22 Decoder-Info](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-info-2.png "Decoder-Info")

### POM Decoder-Adresse

![DCC-FM22 Decoder-Adresse](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-addr.png "Decoder-Adresse")

### POM Decoder-CV

![DCC-FM22 Decoder-CV](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-cv.png "Decoder-CV")

### POM Funktionsmapping

![DCC-FM22 Funktionsmapping](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-mapping-1.png "Funktionsmapping")

![DCC-FM22 Funktionsmapping Lenz](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-mapping-lenz.png "Funktionsmapping Lenz")

![DCC-FM22 Funktionsmapping ESU](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-mapping-esu.png "Funktionsmapping ESU")

### POM Funktionsausgänge

![DCC-FM22 Funktionsausgänge Adresse](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-ausgaenge-1.png "Funktionsausgänge Adresse")

![DCC-FM22 Funktionsausgänge Hersteller](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-ausgaenge-2.png "Funktionsausgänge Hersteller")

![DCC-FM22 Funktionsausgänge einlesen](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-ausgaenge-3.png "Funktionsausgänge einlesen")

![DCC-FM22 Funktionsausgänge ESU](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-ausgaenge-4.png "Funktionsausgänge ESU")

![DCC-FM22 Funktionsausgänge Modi](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-ausgaenge-5.png "Funktionsausgänge Modi")

## Zentrale

![DCC-FM22 Menu Zentrale](https://raw.githubusercontent.com/ukw100/FM22/main/images/menu3.png "Menu Zentrale")

### Einstellungen


### Konfiguration Netzwerk

### Firmware-Updates

## Anhang

_RailCom®_

"RailCom" ist eine auf den Namen von Lenz Elektronik für die Klasse 9 "Elektronische 
Steuerungen" unter der Nummer 301 16 303 eingetragene Deutsche Marke sowie ein für die 
Klassen 21, 23, 26, 36 und 38 "Electronic Controls for Model Railways" in U.S.A. unter 
Reg. Nr. 2,746,080 eingetragene Trademark.
