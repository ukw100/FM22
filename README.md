# DCC-Zentrale FM22
## _DCC-Zentrale - auch für Mittelleiter-Systeme_

Dieses Projekt ist noch in der Entstehung. Eine Übersetzung ins Englische wird erfolgen, sobald die Dokumentation vollständig ist.

![DCC-FM22 Lokliste](https://raw.githubusercontent.com/FM22/main/images/lok1.png "Lokliste")

## Funktionen

- RailCom®-fähige DCC-Zentrale
- System: Raspberry Pi4 als Zentrale, STM32 als DCC-Controller
- Bedienung über Webbrowser: Direkt am RasPi, Smartphone, Tablet oder auch PC
- Mehrbenutzer-System: Bedienung von mehreren Personen gleichzeitig möglich
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

- Aufbau der DCC-Zentrale
- Notwendige Hardware
- Steuerung
-- Lokliste
-- Zusatzdecoder
-- Weichen
-- Fahrstraßen
-- Weichentest
-- S88-Module
-- RailCom®-Module
- Programmierung über Programmiergleis
-- Decoder-Info
-- Decoder-Adresse
-- Decoder-CV
- Programmierung über Hauptgleis (POM)
-- Decoder-Info
-- Decoder-Adresse
-- Decoder-CV
-- Funktionsmapping
-- Funktionsausgänge
- Zentrale
-- Einstellungen
-- Konfiguration Netzwerk
-- Firmware-Updates

## Aufbau der DCC-Zentrale


## Notwendige Hardware

## Steuerung

![DCC-FM22 Menu Steuerung](https://raw.githubusercontent.com/FM22/main/images/menu1.png "Menu Steuerung")


### Lokliste

![DCC-FM22 Lokliste](https://raw.githubusercontent.com/FM22/main/images/lokliste.png "Lokliste")

### Lok

![DCC-FM22 Loksteuerung](https://raw.githubusercontent.com/FM22/main/images/lok1.png "Loksteuerung")

![DCC-FM22 Neue Lok](https://raw.githubusercontent.com/FM22/main/images/neue-lok.png "Neue Lok")

![DCC-FM22 Lok bearbeiten](https://raw.githubusercontent.com/FM22/main/images/lok-bearbeiten.png "Lok bearbeiten")

![DCC-FM22 Lok Macro MF 1](https://raw.githubusercontent.com/FM22/main/images/macro1.png "Lok Macro MF 1")

![DCC-FM22 Lok Macro MF 2](https://raw.githubusercontent.com/FM22/main/images/macro2.png "Lok Macro MF 2")

![DCC-FM22 Lok Macro MH](https://raw.githubusercontent.com/FM22/main/images/macro3.png "Lok Macro MH")

### Zusatzdecoder

![DCC-FM22 Zusatzdecoder](https://raw.githubusercontent.com/FM22/main/images/zusatzdecoder.png "Zusatzdecoder")

![DCC-FM22 Zusatzdecoder neu](https://raw.githubusercontent.com/FM22/main/images/addon-neu.png "Zusatzdecoder neu")

![DCC-FM22 Zusatzdecoder bearbeiten](https://raw.githubusercontent.com/FM22/main/images/addon-bearbeiten.png "Zusatzdecoder bearbeiten")

![DCC-FM22 Zusatzdecoder Lok](https://raw.githubusercontent.com/FM22/main/images/lok-addon.png "Zusatzdecoder Lok")

### Weichen

![DCC-FM22 Weichen](https://raw.githubusercontent.com/FM22/main/images/weichen.png "Weichen")

### Fahrstraßen

![DCC-FM22 Fahrstraßen](https://raw.githubusercontent.com/FM22/main/images/fahrstrassen.png "Fahrstraßen")

![DCC-FM22 Neue Fahrstraße](https://raw.githubusercontent.com/FM22/main/images/neue-fahrstrasse.png "Neue Fahrstraße")

![DCC-FM22 Fahrstraße bearbeiten 1](https://raw.githubusercontent.com/FM22/main/images/fahrstrasse-bearbeiten-1.png "Fahrstraße bearbeiten 1")

![DCC-FM22 Fahrstraße bearbeiten 2](https://raw.githubusercontent.com/FM22/main/images/fahrstrasse-bearbeiten-2.png "Fahrstraße bearbeiten 2")

### Weichentest

![DCC-FM22 Weichentest](https://raw.githubusercontent.com/FM22/main/images/weichentest.png "Weichentest")

### S88-Module

![DCC-FM22 S88-Module](https://raw.githubusercontent.com/FM22/main/images/s88.png "S88-Module")

![DCC-FM22 S88-Module](https://raw.githubusercontent.com/FM22/main/images/s88-edit.png "S88-Module")

![DCC-FM22 S88-Module](https://raw.githubusercontent.com/FM22/main/images/aktionen-s88.png "S88-Module")

### RailCom®-Module

![DCC-FM22 RailCom-Module](https://raw.githubusercontent.com/FM22/main/images/rcl.png "RailCom-Module")

![DCC-FM22 RailCom-Module](https://raw.githubusercontent.com/FM22/main/images/rcl-bearbeiten.png "RailCom-Module")

![DCC-FM22 RailCom-Module](https://raw.githubusercontent.com/FM22/main/images/aktionen-rcl.png "RailCom-Module")

## Programmierung über Programmiergleis

![DCC-FM22 Menu Programmierung](https://raw.githubusercontent.com/FM22/main/images/menu2.png "Menu Programmierung")

### Decoder-Info

![DCC-FM22 Decoder-Info](https://raw.githubusercontent.com/FM22/main/images/pgm-info-3.png " Decoder-Info")

### Decoder-Adresse

![DCC-FM22 Decoder-Info](https://raw.githubusercontent.com/FM22/main/images/pgm-addr-2.png " Decoder-Info")

### Decoder-CV

![DCC-FM22 Decoder-CV](https://raw.githubusercontent.com/FM22/main/images/pgm-cv-2.png " Decoder-CV")

## Programmierung über Hauptgleis (POM)

### Decoder-Info

![DCC-FM22 Decoder-Info](https://raw.githubusercontent.com/FM22/main/images/pom-info-2.png "Decoder-Info")

### Decoder-Adresse

![DCC-FM22 Decoder-Adresse](https://raw.githubusercontent.com/FM22/main/images/pom-addr.png "Decoder-Adresse")

### Decoder-CV

![DCC-FM22 Decoder-CV](https://raw.githubusercontent.com/FM22/main/images/pom-cv.png "Decoder-CV")

### Funktionsmapping

![DCC-FM22 Funktionsmapping](https://raw.githubusercontent.com/FM22/main/images/pom-mapping-1.png "Funktionsmapping")

![DCC-FM22 Funktionsmapping Lenz](https://raw.githubusercontent.com/FM22/main/images/pom-mapping-lenz.png "Funktionsmapping Lenz")

![DCC-FM22 Funktionsmapping ESU](https://raw.githubusercontent.com/FM22/main/images/pom-mapping-esu.png "Funktionsmapping ESU")

### Funktionsausgänge

![DCC-FM22 Funktionsausgänge Adresse](https://raw.githubusercontent.com/FM22/main/images/pom-ausgaenge-1.png "Funktionsausgänge Adresse")

![DCC-FM22 Funktionsausgänge Hersteller](https://raw.githubusercontent.com/FM22/main/images/pom-ausgaenge-2.png "Funktionsausgänge Hersteller")

![DCC-FM22 Funktionsausgänge einlesen](https://raw.githubusercontent.com/FM22/main/images/pom-ausgaenge-3.png "Funktionsausgänge einlesen")

![DCC-FM22 Funktionsausgänge ESU](https://raw.githubusercontent.com/FM22/main/images/pom-ausgaenge-4.png "Funktionsausgänge ESU")

![DCC-FM22 Funktionsausgänge Modi](https://raw.githubusercontent.com/FM22/main/images/pom-ausgaenge-5.png "Funktionsausgänge Modi")

## Zentrale

![DCC-FM22 Menu Zentrale](https://raw.githubusercontent.com/FM22/main/images/menu3.png "Menu Zentrale")

### Einstellungen


### Konfiguration Netzwerk

### Firmware-Updates

## Anhang

_RailCom®_

"RailCom" ist eine auf den Namen von Lenz Elektronik für die Klasse 9 "Elektronische 
Steuerungen" unter der Nummer 301 16 303 eingetragene Deutsche Marke sowie ein für die 
Klassen 21, 23, 26, 36 und 38 "Electronic Controls for Model Railways" in U.S.A. unter 
Reg. Nr. 2,746,080 eingetragene Trademark. Das Europäische Patent 1 380 326 B1 wurde 
aufgehoben. Damit ist RailCom unter Beachtung der Warenzeichen frei verwendbar.
