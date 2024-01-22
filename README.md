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
- Sehr einfache und komfortable Konfiguration von Funktionmapping/Funktionsausgängen für diverse Decoder-Familien

FM22 ist eine RailCom-fähige DCC-Zentrale, die Open-Source-Software verwendet. Die zur Steuerung notwendige Hardware ist dokumentiert, so dass jeder Interessierte die
FM22-Zentrale nachbauen kann. Bei den benötigten Bauteilen handelt es sich um Standardkomponenten, die auf dem Elektronikmarkt leicht zu beschaffen sind. Teilweise wird auf
Module gesetzt, so dass das schwierige Verlöten von SMD-Bauteilen wie STM32-Mikrocontrollern nicht notwendig ist. Die Funktionen der Zentrale werden ständig erweitert.
Durch den möglichen Selbstbau der Zentrale hält sich der finanzielle Aufwand in einem überschaubaren Rahmen.

Die Benutzeroberfläche der FM22-Zentrale wird im Browser dargestellt. Dabei sind alle Webseiten nicht statisch, sondern interaktiv. Das bedeutet: Sobald sich etwas ändert, wie
zum Beispiel die Geschwindigkeit der Lok, wird dies automatisch auch in der Bedienoberfläche angezeigt. Alle Bedienelemente ändern sich also automatisch, man sieht immer den aktuellen
aktuellen Zustand. Der besondere Reiz liegt in der Mehrbenutzerfähigkeit. Die auf der Anlage befindlichen Loks, Signale, Lichtelemente und vieles mehr können von mehreren Personen
über mehrere Geräte bedient werden. Dies funktioniert nicht nur über den Raspberry Pi Desktop, sondern über jedes im LAN/WLAN befindliche Gerät wie PC, Laptop, Tablet
oder auch Smartphone.

Der Betrieb erfolgt "halbautomatisch", d.h. bei bestimmten Ereignissen können vordefinierte Aktionen automatisch ausgelöst werden. Dennoch behält der Benutzer die Kontrolle
in der Hand und kontrolliert bzw. steuert die Abläufe selbst.

Durch den konsequenten Einsatz von RailCom ist die Zentrale jederzeit darüber informiert, wo sich welcher Zug befindet. Die Abbildung aller Ortungsdaten auf einem Gleisplan ist
Gleisplan ist in Entwicklung.

## Inhaltsverzeichnis

* [Aufbau der DCC-Zentrale](#aufbau-der-dcc-zentrale)
* [Notwendige Hardware](#notwendige-hardware)
* [Steuerung](#steuerung)
  * [Lokliste](#lokliste)
  * [Zusatzdecoder](#zusatzdecoder)
  * [Weichen](#weichen)
  * [Signale](#signale)
  * [LEDs](#leds)
  * [Fahrstraßen](#fahrstraßen)
  * [S88-Module](#s88-module)
  * [RailCom®-Module](#railcom-module)
  * [Decodertest](#decodertest)
* [Programmierung](#programmierung)
* [Programmierung über Programmiergleis](#programmierung-über-programmiergleis)
  * [PGM Decoder-Info](#pgm-decoder-info)
  * [PGM Decoder-Adresse](#pgm-decoder-adresse)
  * [PGM Decoder-CV](#pgm-decoder-cv)
* [Programmierung über Hauptgleis (POM)](#programmierung-über-hauptgleis-pom)
  * [POM Decoder-Info](#pom-decoder-info)
  * [POM Decoder-Adresse](#pom-decoder-adresse)
  * [POM Decoder-CV](#pom-decoder-cv)
  * [POM Motorparameter](#pom-decoder-motorparameter)
  * [POM Funktionsmapping](#pom-funktionsmapping)
  * [POM Funktionsausgänge](#pom-funktionsausgänge)
* [Zentrale](#zentrale)
  * [Einstellungen](#einstellungen)
  * [Konfiguration Netzwerk](#konfiguration-netzwerk)
  * [Firmware-Updates](#firmware-updates)
* [Anhang](#anhang)

## Aufbau der DCC-Zentrale


## Notwendige Hardware

* Raspberry Pi 4
* 18V/36W Steckernetzteil o.ä.
* STM32F401CC BlackPill Board
* H-Brücke MiniIBT 5A 12V-48V
* Platine "FM22-Central"
* Bauteile, siehe BOM.

BOM (nicht ganz aktuell, wird mit Veröffentlichung des Schaltplans aktualisiert):

|Pos|Anzahl|Name                   |Wert                     |Reichelt Bestellnr.     |Bemerkung                        |
|---|------|-----------------------|-------------------------|------------------------|---------------------------------|
|1  |5     |C1, C3, C4, C5, C7     |100nF                    |X7R-2,5 100N KOM        |                                 |
|2  |1     |C2                     |220µF/35V                |RAD FR 220/50 (RM 5,08) |alternativ RAD FR 220/35         |
|3  |1     |C6                     |100µF/16V                |RAD FR 100/16           |                                 |
|4  |1     |D1                     |1N4007                   |1N 4007                 |                                 |
|5  |2     |D2, D3                 |SB320                    |SB 320                  |                                 |
|7  |1     |IC2                    |MAX485CPA                |MAX 485 CPA             |                                 |
|8  |1     |IC3                    |78L05                    |µA 78L05                |                                 |
|9  |1     |IC4                    |6N137                    |6N 137                  |                                 |
|10 |1     |IC5                    |LM393N                   |LM 393 DIP              |                                 |
|11 |1     |IC6                    |PC817                    |LTV 817                 |                                 |
|12 |2     |K1, K2                 |RJ45_8POLIG              |MEBP 8-8S               |                                 |
|13 |1     |K3                     |Stiftleiste_1x06_W_2,54  |SL 1X40W 2,54           |                                 |
|14 |1     |K4                     |Stiftleiste_1x06_G_2,54  |SL 1X40G 2,54           |                                 |
|15 |1     |K6                     |KLEMME2POL               |AKL 101-02              |                                 |
|16 |1     |K7                     |KLEMME4POL               |AKL 101-04              |                                 |
|17 |3     |R1, R3, R11            |22,1K                    |METALL 22,1K            |                                 |
|18 |1     |R2                     |4,87K                    |METALL 4,87K            |                                 |
|19 |1     |R4                     |4,64K                    |METALL 4,64K            |                                 |
|20 |1     |R5                     |1,8 2W                   |2W METALL 1,8           |                                 |
|21 |2     |R6, R7                 |390                      |METALL 390              |                                 |
|22 |1     |R8                     |560                      |METALL 560              |                                 |
|23 |2     |R9, R16                |10K                      |METALL 10,0K            |                                 |
|24 |1     |R10                    |4,75K                    |METALL 4,75K            |                                 |
|25 |1     |R12                    |120                      |METALL 120              |                                 |
|26 |3     |R13, R17, R18          |1K                       |METALL 1,00K            |                                 |
|27 |1     |R14                    |33 2W                    |2W METALL 33            |                                 |
|28 |1     |R15                    |5,1K                     |METALL 5,10K            |                                 |
|29 |1     |RPI Header             |40pol                    |RPI HEADER 40           |                                 |
|30 |1     |RPI Abstandsbolzen Set |20mm                     |RPI MOUNTINGKIT2        |                                 |
|31 |4     |IC-Sockel 8-polig      |                         |GS 8P                   |Einen Sockel für PC817 halbieren |

Die Zentrale kann mit bis zu 32 RailCom-Detektormodulen erweitert werden. Diese Module können selbst bis zu 8 Streckenabschnitte überwachen und der Zentrale
melden, in welchem Abschnitt sich welche Lok gerade befindet. Insgesamt können so bis zu 256 Abschnitte auf der Anlage überwacht werden.

Die Beschreibung des RC-Detektor-Moduls folgt später an dieser Stelle.


## Steuerung

Das Menü "Steuerung" bietet folgende Punkte:

<img align="right" src="https://github.com/ukw100/FM22/blob/main/images/menu1.png">

* [Lokliste](#lokliste)
* [Zusatzdecoder](#zusatzdecoder)
* [Weichen](#weichen)
* [Signale](#signale)
* [LEDs](#leds)
* [Fahrstraßen](#fahrstraßen)
* [S88](#s88-module)
* [RC-Detektoren](#railcom-module)
* [Decodertest](#decodertest)

### Lokliste

Die Lokliste ist wohl die wichtigste Anzeige während des Betriebs. Hier wird angezeigt:

 * ID der Loks
 * Online-Status (grün=online, sonst offline)
 * Bezeichnung der Lok
 * Aktueller Aufenthaltsort der Lok
 * Buttons "MF" und "MH" zum Ausführen von Makros (hier "Fahrt" und "Halt")
 * Adresse der Lok
 * RailCom Channel 2 Übertragungsqualität
 * Ein Button zum Bearbeiten der Eigenschaften - nur sichtbar im Bearbeitungsmodus

Ein Beispiel:

![DCC-FM22 Lokliste](https://raw.githubusercontent.com/ukw100/FM22/main/images/lokliste.png "Lokliste")

Im obigen Beispiel stehen die Loks mit den Nummern (IDs) 0 bis 8 auf der Anlage - gekenzeichnet durch den Online-Status. Die Loks mit den IDs
9 - 12 sind zur Zeit offline. Sie wurden zur Veranschaulichung vom Gleis genommen. Sobald eine dieser Loks auf das Gleis gestellt
wird, ändert sich auch instantan der Online-Status. Dieser wechselt dann auf die Farbe "grün".

Durch die Buttons "MF" und "MH" können automatisiert Abläufe (Macros) gestartet werden, zum Beispiel für "MF" (Macro "Fahrt"):

* Einschalten der Stirnbeleuchtung
* Nach 5 Sekunden Einschalten der Innenbeleuchtung
* Nach 7 Sekunden Einschalten des Betriebsgeräusches
* Nach 10 Sekunden Bahnhofsdurchsage
* Nach 15 Sekunden Schaffnerpfiff
* Nach 18 Sekunden Fahrtaufnahme durch Erhöhung der Geschwindigkeit über eine Rampendefinition.
* Nach 24 Sekunden Lokpfiff

Dies ist nur ein Beispiel. Insgesamt können bis zu 8 Makros mit jeweils 16 Aktionen pro Lok definiert werden. Für weitere Informationen siehe [Lok-Macros](#lok-macros).

In der Spalte "Adresse" wird die Lokadresse angezeigt. Die Spalte "RC2" zeigt die Übertragungsqualität der RailCom-Rückmeldungen im Betrieb.
Solange keine Störungen auf der Anlage vorliegen, sollten hier Werte zwischen 98% und 100% angezeigt werden. Bei einem Wert von 100% wurde jeder DCC-Befehl vom Decoder korrekt
vom Decoder richtig beantwortet und auch von der Zentrale vollständig "verstanden". Nach den Spalten ID, Name und Adresse kann sortiert werden. Die Sortierung erfolgt
durch Anklicken der entsprechenden Spalte. Die aktive Spaltenbezeichnung wird dann grün dargestellt.

Der Button "Bearbeiten" erscheint nur im Bearbeitungsmodus. Hier können dann die Grundeinstellungen der Lok - wie zum Beispiel die Adresse - geändert werden.

Durch Anklicken des Loknamens gelangt man in die Loksteuerung der gewünschten Lok.

### Loksteuerung

Auf dieser Seite kann die ausgewählte Lok gesteuert werden. Dies ist per Smartphone, Tablet, direkt am Pi oder auch mit einem PC möglich.
So können mehrere Personen gleichzeitig verschiedene Loks steuern.

Hier sieht man:

* Lokadresse mit Online-Status
* Aufenthaltsort der Lok
* Aktuelle Geschwindigkeit der Lok
* Richtung
* Fahrtziel
* Bezeichnungen und Zustände der Funktionstasten

![DCC-FM22 Loksteuerung](https://raw.githubusercontent.com/ukw100/FM22/main/images/lok1.png "Loksteuerung")

Im obigen Beispiel wird die Lokadresse 1012 angezeigt. Die Adresse ist grün hinterlegt, um anzuzeigen, dass die Lok "online" ist.
Der Aufenthaltsort der Lok ist gerade "Einfahrt Schattenbahnhof", die Geschwindigkeit ist "0" und die Fahrtrichtung ist "vorwärts".
Außerdem ist gerade die Funktion F0 "Spitzensignal / Schlusslicht rot" aktiv.

Alle Ausgaben können sich sofort ändern, wenn eine andere Person oder eine aktivierte Ereignissteuerung in den Ablauf eingreift. Mit
dem Schieberegler kann die Geschwindigkeit, mit den Pfeilen darunter kann die Fahrtrichtung geändert werden. Ein Klick auf das schwarze Quadrat
setzt die Geschwindigkeit auf 0, ein Klick auf den schwarzen Kreis daneben löst einen Nothalt aus, d.h. die Lok kommt sofort zum Stehen - ohne
Rücksicht auf eventuell eingestellte Bremsverzögerungen.

Insgesamt können 8 Lok-Macros (MF, MH, M3 - M8) definiert und ausgelöst werden, siehe auch [Lok-Macros](#lok-macros).

### Neue Lok

In der Lokliste kann durch Anklicken des Buttons "Neue Lok" unterhalb der Liste eine neue Lok angelegt werden. Dieser Button erscheint nur
im Bearbeitungsmodus. Direkt unter dem Button erscheinen dann Eingabefelder, über welche die neue Lok eingetragen werden kann:

![DCC-FM22 Neue Lok](https://raw.githubusercontent.com/ukw100/FM22/main/images/neue-lok.png "Neue Lok")

Im obigen Beispiel wurde der Name "BR 628 Triebwagenzug" gewählt, die Adresse auf 1019 gesetzt und die Fahrstufen auf "128".
Ist die Lok auf "Aktiv" gestellt, wird sie auch mit Lokbefehlen gesteuert. Ist die Lok hier deaktiviert, werden keine Lokbefehle
an die Lok gesendet. Nach einem Klick auf Speichern erscheint die neue Lok in der Lokliste.

### Lok bearbeiten

Klickt man in der Lokliste auf den Button "Bearbeiten", kann man die Grundeinstellungen wie Name und Adresse ändern:

![DCC-FM22 Lok bearbeiten](https://raw.githubusercontent.com/ukw100/FM22/main/images/lok-bearbeiten.png "Lok bearbeiten")

Durch Setzen der ID kann der gewünschte Lokeintrag auch in der Liste von oben nach unten und umgekehrt "wandern",
also die Lokliste neu sortieren. Ansonsten gelten die gleichen Einstellungen wie für eine neue Lok.

### Lok-Funktionen

Die Lokfunktionen müssen für jeden Lokdecoder einmal eingestellt werden. Dies ist relativ einfach und schnell zu bewerkstelligen, da die meisten Lokfunktionen
vordefiniert und müssen nur noch den Funktionstasten F0 bis F31 zugeordnet werden.

Dies geschieht durch Klick auf die Schaltfläche "FX" im unteren Bereich. Dann öffnet sich unterhalb der Schaltfläche folgender Dialog:

![DCC-FM22 Lok-Funktionen](https://raw.githubusercontent.com/ukw100/FM22/main/images/lokfunktionen.png "Lok-Funktionen")

Hier sind nun folgende Einstellungen nötig:

* Funktionstastennummer F0 - F31
* Name der Funktion - auswählbar aus Liste
* Puls: Ja/nein
* Sound: Ja/nein

Der Funktionsname kann aus einer Liste von ca. 250 Bezeichnungen ausgewählt werden. Mit der Einstellung "Puls" kann gewählt werden, ob ein Druck auf die Funktionstaste
nur einen kurzen Impuls oder ein Dauersignal auslösen soll. Der kurze Impuls ist sinnvoll bei Geräuschen, die nicht dauerhaft abgespielt werden sollen, wie z.B. ein Lokpfiff.

Die Einstellung "Sound" dient lediglich der Information, ob die gewählte Funktion einen Sound abspielt oder nicht.

Sind alle Einstellungen vorgenommen worden, könnte die Bedienoberfläche für eine Lok mit umfangreichen Lokfunktionen nun folgendermaßen aussehen:

![DCC-FM22 Loksteuerung im Detail](https://raw.githubusercontent.com/ukw100/FM22/main/images/lok2.png "Loksteuerung im Detail")

### Lok-Macros

Mit Lok-Makros können bestimmte Aktionen automatisiert ausgeführt werden. Insgesamt sind bis zu 16 Aktionen pro Lok-Makro möglich. Im unteren
Beispiel sind folgende Aktionen definiert:

* Schalte die Funktion F0 sofort ein
* Schalte die Funktion F2 sofort ein
* Fahre die Geschwindigkeit nach 2 Sekunden auf 60 in einer Rampe mit Dauer von 8 Sekunden

![DCC-FM22 Lok Macro MF 1](https://raw.githubusercontent.com/ukw100/FM22/main/images/macro1.png "Lok Macro MF 1")

Folgende Aktionen sind möglich:

* Setze Geschwindigkeit
* Setze Mindestgeschwindigkeit
* Setze Höchstgeschwindigkeit
* Setze Richtung
* Schalte alle Funktionen aus
* Schalte Funktion aus
* Schalte Funktion ein

![DCC-FM22 Lok Macro MF 2](https://raw.githubusercontent.com/ukw100/FM22/main/images/macro2.png "Lok Macro MF 2")

Hier ein weiteres Beispiel, um eine Lok im Schattenbahnhof auf Ihrem zugeordneten Gleis abzustellen:

* Setze Geschwindigkeit sofort auf 0
* Schalte nach 2 Sekunden F2 (Betriebsgeräusch) aus
* Schalte nach 15,5 Sekunden alle Funktionen aus.

![DCC-FM22 Lok Macro MH](https://raw.githubusercontent.com/ukw100/FM22/main/images/macro3.png "Lok Macro MH")

### Zusatzdecoder

Zusatzdecoder sind zusätzliche Decoder im Zug. Das kann z.B. ein Lichtdecoder im Steuerwagen des Zuges sein, hier z.B.
für einen IC-Steuerwagen:

![DCC-FM22 Zusatzdecoder](https://raw.githubusercontent.com/ukw100/FM22/main/images/zusatzdecoder.png "Zusatzdecoder")

Ein neuer Zusatzdecoder wird über die entsprechende Schaltfläche konfiguriert. Anschließend wird ausgewählt bzw. eingestellt:

* Name, z.B. Inennbeleuchtung ICE2
* Adresse, hier 2001
* Verknüpfung mit Lok im Zug

Durch die Verknüpfung des Zusatzdecoders mit der zugehörigen Lok werden die möglichen Zusatzfunktionen des Decoders bei der Bedienung der Lok sichtbar.
Betrieb der Lok. Ein Beispiel dazu folgt weiter unten.

![DCC-FM22 Zusatzdecoder neu](https://raw.githubusercontent.com/ukw100/FM22/main/images/addon-neu.png "Zusatzdecoder neu")

Durch Klicken auf "Bearbeiten" können die Einstellungen nachträglich geändert werden. Außerdem kann durch Eingabe einer anderen ID
die Reihenfolge der Zusatzdecoder in der Anzeige geändert werden.

![DCC-FM22 Zusatzdecoder bearbeiten](https://raw.githubusercontent.com/ukw100/FM22/main/images/addon-bearbeiten.png "Zusatzdecoder bearbeiten")

Hier ein Beispiel: Der Zusatzdecoder des IC-Steuerwagens wurde hier mit der Lok "BR 103 Intercity" verknüpft. Nun erscheinen
der entsprechenden Lok die Zusatzfunktionen wie

* F0 Spitzensignal / Schlusslicht rot
* F1 Fernlicht
* F2 Innenbeleuchtung
* F3 Führerstandsbeleuchtung
* F4 Stromführende Kupplung

unter den Bedienelementen der Lokomotive. Fahrtrichtungsänderungen der Lok wirken sich auch direkt auf die eingestellte Fahrtrichtung des Zusatzdecoders aus,
z. B. das Umschalten der Stirnbeleuchtung von rot auf weiß und umgekehrt.

Hier kann die Funktion F0 des Zusatzdecoders mit F0 der Lok verknüpft werden. Das heißt, wenn ich jetzt F0 der Lok einschalte, wird automatisch auch
automatisch die Funktion F0 des Zusatzdecoders entsprechend der Fahrtrichtung der Lok aktiviert. Damit ist nun an beiden Enden des Zuges
Enden des Zuges entsprechend eingestellt.

![DCC-FM22 Zusatzdecoder Lok](https://raw.githubusercontent.com/ukw100/FM22/main/images/lok-addon.png "Zusatzdecoder Lok")

### Weichen

Unter dem Menüpunkt "Weichen" können die Weichen der Anlage konfiguriert werden. Diese werden später in eine oder mehrere
Fahrstraßen eingebunden. Beim Aktivieren einer Fahrstraße werden dann direkt Gruppen von Weichen zeitversetzt geschaltet, um eine Fahrstraße
für die Lok zu bilden.

![DCC-FM22 Weichen](https://raw.githubusercontent.com/ukw100/FM22/main/images/weichen.png "Weichen")

In dieser Ansicht können die Weichen auch manuell geschaltet werden, falls dies ausnahmsweise erforderlich sein sollte. Dies geschieht automatisch
durch Anklicken der entsprechenden Schaltflächen wie "Gerade", "Abzweig" und "Abzweig2", wobei letztere nur bei 3-Wege-Weichen angezeigt wird.

Bei der Konfiguration einer neuen oder bereits vorhandenen Weiche können folgende Einstellungen vorgenommen werden:

* Name
* Adresse
* Haken für 3-Wege-Weiche

### Fahrstraßen

Fahrstraßen fassen bestimmte Gleise, die durch Weichen bedient werden, zu Gruppen zusammen.

Hier ein Beispiel:

![DCC-FM22 Fahrstraßen](https://raw.githubusercontent.com/ukw100/FM22/main/images/fahrstrassen.png "Fahrstraßen")

Diese "Schattenbahnhof" genannte Fahrstraße besteht aus insgesamt 9 Gleisen. Jedes dieser Gleise kann einen Zug ("Lokomotive") aufnehmen.
Die Gleise können - sinnvoll bei unterschiedlichen Längen - fest mit den Lokomotiven verbunden oder frei wählbar sein.

In dieser Ansicht werden dargestellt:

* Gleisname
* Verknüpfte Lok
* Aktivierung des Gleises durch RailCom-Detektor oder S88-Kontaktgleis
* Auf dem Gleise erkannte Lok

Welches Gleis der Fahrstraße gerade aktiv ist, wird durch die grüne Markierung in der Spalte ID angezeigt. In unserem Beispiel wurde
Gleis 1 für die Einfahrt der zugeordneten Lok gewählt. Diese wurde auch erkannt, wie rechts zu sehen ist. Du kannst ein bestimmtes Gleis
auch selbst aktivieren, indem Du auf die Schaltfläche "Setzen" klickst. Dann werden alle zu diesem Gleis gehörenden Weichen automatisch
so umgestellt, dass das Gleis 1 im Schattenbahnhof aktiviert ist.

Durch Klicken auf "Neue Fahrstraße" wird eine neue Fahrstraße angelegt. Dieser muss zunächst ein Name gegeben werden:

![DCC-FM22 Neue Fahrstraße](https://raw.githubusercontent.com/ukw100/FM22/main/images/neue-fahrstrasse.png "Neue Fahrstraße")

Eine Umbenennung der Fahrstraßen ist auch nachträglich über den Menüpunkt "Bearbeiten" möglich, ebenso eine Neusortierung der Fahrstraßen durch die Wahl einer neuen ID:

![DCC-FM22 Fahrstraße bearbeiten 1](https://raw.githubusercontent.com/ukw100/FM22/main/images/fahrstrasse-bearbeiten-1.png "Fahrstraße bearbeiten 1")

Nachdem ein Fahrweg angelegt wurde, kann im nächsten Schritt der Fahrweg konfiguriert werden. Dabei werden die zugehörigen
Spuren. Das folgende Beispiel zeigt, wie das Gleis 9 der Fahrstraße konfiguriert wird.

Zuerst wird der Name festgelegt: "Gleis 9". Anschließend kann - falls gewünscht - eine Lokomotive mit dem Gleis verknüpft werden. Damit
Damit hat die Lok ein "Heimatgleis", das sie immer anfahren kann, wenn sie abgestellt werden soll:

![DCC-FM22 Fahrstraße bearbeiten 2](https://raw.githubusercontent.com/ukw100/FM22/main/images/fahrstrasse-bearbeiten-2.png "Fahrstraße bearbeiten 2")

Zuletzt müssen die Weichen so gestellt werden, dass die Lokomotive auch ihr Heimatgleis erreichen kann. Im Beispiel sind dazu erforderlich:

* Weiche 5 auf Abzweig
* Weiche 3 auf Gerade
* Weiche 7 auf Abzweig
* Weiche 4 auf Gerade

Wenn die Lok irgendwann während der Fahrt in den Schattenbahnhof einfährt, stellen sich die Weichen automatisch auf das Heimatgleis.
Die Lok verlangsamt ihre Fahrt und kommt schließlich sicher auf dem Heimatgleis zum Stehen. Wie hier das Zusammenspiel von RC-Detektoren,
S88-Kontakten und Fahrstraßen funktioniert, wird später weiter unten im Detail erklärt.

### Signale

Die Signale sind für die Steuerung nicht erforderlich. Sie dienen lediglich der optischen Anzeige. Die Steuerung selbst erfolgt ausnahmslos
die Zentrale FM22.

Unter dem Menüpunkt "Signale" werden die Signale definiert. Hier werden nur Signale unterstützt, die lediglich 2 Zustände kennen.
Unter dem Menüpunkt "LEDs" können bis zu 8 LEDs zu einem Signal zusammengefasst werden. Dazu später mehr.

Im Beispiel unten wurden 4 Signale konfiguriert:

![DCC-FM22 Signale](https://raw.githubusercontent.com/ukw100/FM22/main/images/signale.png "Signale")

Neben dem Namen und der Adresse wird auch der aktuelle Anzeigestatus der Signale angezeigt. im obigen Beispiel befinden sich zwei Signale
im Zustand "Fahrt" und zwei im Zustand "Halt". Dies wird durch die Farben grün und rot entsprechend angezeigt. Durch Anklicken der
entsprechenden Schaltflächen kann der Zustand der Signale manuell geändert werden. Sinnvoller ist es jedoch, einen solchen Signalwechsel
über eine Aktion eines RC-Detektors oder S88-Kontaktgleises ausführen zu lassen.

Neben der Neuanlage eines Signals ist auch eine nachträgliche Bearbeitung möglich. Hierbei kann neben dem Namen und der Adresse auch die
die Signalfolge über die ID geändert werden:

![DCC-FM22 Signal bearbeiten](https://raw.githubusercontent.com/ukw100/FM22/main/images/signal-bearbeiten.png "Signal bearbeiten")

### LEDs

Unter dem Menüpunkt "LEDs" können Lichtsignale angesteuert werden, die mehr als 2 Zustände anzeigen können. Dies sind entweder komplexere
Lichtsignale oder es können auch Gebäudebeleuchtungen sein, z.B. bei Verwendung eines FM22-RC Detektors mit WS2812-LED-Ausgang.

Für komplexe Lichtsignale werden in der Regel sogenannte "erweiterte Zubehördecoder" verwendet. Hier werden zum Decoder nicht nur zwei Zustände
wie "Fahrt" und "Halt" übertragen, sondern ein ganzer Bytewert, der insgesamt 256 Zustände annehmen kann bzw. bis zu 8 LEDs
unabhängig voneinander ansteuern kann.

Im folgenden Beispiel wurden zwei LED-Gruppen mit je 8 LEDs erzeugt, nämlich ein "Brückenstellwerk" und die Beleuchtung des Bahnhofs "Kümmelhausen":

![DCC-FM22 LEDs](https://raw.githubusercontent.com/ukw100/FM22/main/images/leds.png "LEDs")

Hier können jeweils 8 LEDs (0-7) unabhängig voneinander manuell ein- und ausgeschaltet werden. Der aktuelle Zustand wird durch einen
grünen Hintergrund dargestellt: Die entsprechenden LEDs sind damit eingeschaltet. Die Anzeige ändert sich automatisch in Echtzeit, wenn
irgendeines der LED-Signale einen anderen Zustand annimmt.

Selbstverständlich lassen sich die Lichtzustände automatisiert als RailCom-Detektor- oder S88-Kontaktgleis-Aktionen steuern. Dazu werden
weiter unten entsprechende Beispiel-Anwendungen aufgeführt.

Bei der Bearbeitung der LED-Konfiguration können der Name und die Adresse der LED-Gruppe neu eingestellt werden. Ebenso kann man über
die Angabe einer neuen ID die Anzeigereihenfolge ändern:

![DCC-FM22 LEDs bearbeiten](https://raw.githubusercontent.com/ukw100/FM22/main/images/leds-bearbeiten.png "LEDs bearbeiten")

### S88-Module

![DCC-FM22 S88-Module](https://raw.githubusercontent.com/ukw100/FM22/main/images/s88.png "S88-Module")

![DCC-FM22 S88-Module](https://raw.githubusercontent.com/ukw100/FM22/main/images/s88-edit.png "S88-Module")

Dabei sind folgende Aktionen wählbar:

<img align="right" src="https://github.com/ukw100/FM22/blob/main/images/aktionen-s88.png">

* Setze Geschwindigkeit: Einstellung der Geschwindigkeit.
* Setze Mindestgeschwindigkeit: Ist die momentane Geschwindigkeit niedriger, wird sie auf den konfigurierten Wert erhöht.
* Setze Höchstgeschwindigkeit: Ist die momentane Geschwindigkeit höher, wird sie auf den konfigurierten Wert erniedrigt.
* Setze Richtung: Richtung wird auf vorwärts oder rückwärts gesetzt.
* Schalte alle Funktionen aus: Es werden alle Lokfunktionen abgeschaltet.
* Schalte Funktion aus: Es wird die ausgewählte Lokfunktion abgeschaltet.
* Schalte Funktion ein: Es wird die ausgewählte Lokfunktion eingeschaltet.
* Führe Lok-Macro aus: Es wird das audgwählte Macro ausgeführt.
* Schalte alle Funktionen aus von Zusatzdecoder: hier werden alle Funktionen des mit der Lok verknüpften Zusatzdecoders abgeschaltet.
* Schalte Funktion aus von Zusatzdecoder: Es wird die ausgewählte Funktion des Zusatzdcoders abgeschaltet.
* Schalte Funktion ein von Zusatzdecoder: Es wird die ausgewählte Funktion des Zusatzdcoders eingeschaltet.
* Schalte Gleis von Fahrstraße: Es wird das Gleis einer ausgewählten Fahrstraße geschaltet.
* Schalte freies Gleis von Fahrstraße: Es wird das freie Gleis einer ausgewählten Fahrstraße geschaltet.
* Setze Ziel: Das Ziel der Lok wird auf die gewählte Fahrstraße gesetzt.
* Schalte LED: Es wird eine LED-Gruppe eines Signals oder Beleuchtungsmoduls geschaltet.
* Schalte Weiche: Es wird eine Weiche geschaltet.
* Schalte Signal: Es wird ein Signal geschaltet.

### RailCom®-Module

![DCC-FM22 RailCom-Module](https://raw.githubusercontent.com/ukw100/FM22/main/images/rcl.png "RailCom-Module")

![DCC-FM22 RailCom-Module](https://raw.githubusercontent.com/ukw100/FM22/main/images/rcl-bearbeiten.png "RailCom-Module")

Dabei sind folgende Aktionen wählbar:

<img align="right" src="https://github.com/ukw100/FM22/blob/main/images/aktionen-rcl.png">

* Setze Geschwindigkeit: Einstellung der Geschwindigkeit.
* Setze Mindestgeschwindigkeit: Ist die momentane Geschwindigkeit niedriger, wird sie auf den konfigurierten Wert erhöht.
* Setze Höchstgeschwindigkeit: Ist die momentane Geschwindigkeit höher, wird sie auf den konfigurierten Wert erniedrigt.
* Setze Richtung: Richtung wird auf vorwärts oder rückwärts gesetzt-
* Schalte alle Funktionen aus: Es werden alle Lokfunktionen abgeschaltet.
* Schalte Funktion aus: Es wird die ausgewählte Lokfunktion abgeschaltet.
* Schalte Funktion ein: Es wird die ausgewählte Lokfunktion eingeschaltet.
* Führe Lok-Macro aus: Es wird das audgwählte Macro ausgeführt.
* Schalte alle Funktionen aus von Zusatzdecoder: hier werden alle Funktionen des mit der Lok verknüpften Zusatzdecoders abgeschaltet.
* Schalte Funktion aus von Zusatzdecoder: Es wird die ausgewählte Funktion des Zusatzdcoders abgeschaltet.
* Schalte Funktion ein von Zusatzdecoder: Es wird die ausgewählte Funktion des Zusatzdcoders eingeschaltet.
* Schalte Gleis von Fahrstraße: Es wird das Gleis einer ausgewählten Fahrstraße geschaltet.
* Schalte freies Gleis von Fahrstraße: Es wird das freie Gleis einer ausgewählten Fahrstraße geschaltet.
* Warte auf freien S88-Kontakt, Halt: Es wird gewartet, bis der ausgewählte S88-Kontakt frei ist.
* Setze Ziel: Das Ziel der Lok wird auf die gewählte Fahrstraße gesetzt.
* Schalte LED: Es wird eine LED-Gruppe eines Signals oder Beleuchtungsmoduls geschaltet.
* Schalte Weiche: Es wird eine Weiche geschaltet.
* Schalte Signal: Es wird ein Signal geschaltet.

### Decodertest

Unter dem Menüpunkt "Decodertest" können Zubehördecoder einfach und schnell getestet werden. Hier gibt man einfach eine Adresse
ein, wählt die gewünschte Funktion und betätigt die Schaltfläche "Schalten":

![DCC-FM22 Decodertest](https://raw.githubusercontent.com/ukw100/FM22/main/images/decodertest.png "Decodertest")

Ausgewählt werden kann hier unter

* Weichen-Decoder für Weichen
* Signal-Decoder für einfache Signale wie Fahrt/Halt
* LED-Decoder für komplexe Signale oder Beleuchtungen


## Programmierung

Das Menü "Programmierung" bietet folgende Punkte:

<img align="right" src="https://github.com/ukw100/FM22/blob/main/images/menu2.png">

* [Programmierung über Programmiergleis](#programmierung-über-programmiergleis)
  * [PGM Decoder-Info](#pgm-decoder-info)
  * [PGM Decoder-Adresse](#pgm-decoder-adresse)
  * [PGM Decoder-CV](#pgm-decoder-cv)
* [Programmierung über Hauptgleis (POM)](#programmierung-über-hauptgleis-pom)
  * [POM Decoder-Info](#pom-decoder-info)
  * [POM Decoder-Adresse](#pom-decoder-adresse)
  * [POM Decoder-CV](#pom-decoder-cv)
  * [POM Decoder-Motorparameter](#pom-decoder-motorparameter)
  * [POM Funktionsmapping](#pom-funktionsmapping)
  * [POM Funktionsausgänge](#pom-funktionsausgänge)

## Programmierung über Programmiergleis

Am Programmiergleis darf lediglich ein Decoder angeschlossen werden, denn hier werden die Programmierbefehle an den Decoder ohne
die Übertragung einer Decoder-Adresse verwendet. Diese Methode hat Vorteile, aber auch Nachteile:

Vorteile:

* Programmierung ohne Wissen einer Decoderdadresse

Nachteile:

* Es darf nur ein Decoder über das Programmiergleis angeschlossen sein
* Programmierung ist sehr langsam
* Am Decoder muss ein Verbraucher (Lokmotor, LED-Kette oder sonstiges) angeschlossen sein.

Heutzutage ist die Programmierung über das Programmiergleis nur noch für eine einzige Funktion gedacht, nämlich das Übertragen bzw. Einstellen
einer neuen Decoder-Adresse. Alle anderen Programmierungen lassen sich mit modernen Decodern auf dem Hauptgleis erledigen. Damit muss
die Lok also gar nicht mehr von der Anlage genommen werden. Die heutigen Decoder erlauben auch sämtliche Programmierfunktionen über
das Hauptgleis - mit einer Ausnahme: Meist kann die Adresse ausschließlich über das Programmiergleis eingestellt werden.

Der Grund dafür ist einfach: Auch nach Ändern der Decoderadresse verliert man niemals die Kontrolle über den Decoder, denn hier
können sämtliche Konfigurationsvariablen - auch die Adresse - ohne Kenntnis der Adresse selbst durchgeführt werden.

Die Ermittlung eines CV-Wertes über PGM geschieht durch eine Strommessung. Dies ist nur möglich, wenn der Decoder auch die Möglichkeit hat, den
entsprechenden Strom zu erzeugen. Manche Lokdecoder verwenden dafür den Lokmotor, der dann während der Programmierung kleine Sprünge macht, andere
erwarten einen speziellen Verbraucher wie zum Beispiel eine LED-Kette, die an einem Funktionsausgang angeschlossen werden muss. Im Zweifel muss man
hier das Decoder-Handbuch zu Rate ziehen. Da bei RailCom-fähigen Decodern fast immer die Programmierung über POM vorzuziehen ist, kann her nur geraten
werden, bis auf die Programmierung der Decoder-Adresse auch POM zu verwenden.

Über die Menüeinträge unterhalb Programmierung -> Programmiergleis können die Programmierungen über das Programmiergleis vorgenommen werden, über die
Menüeinträge unterhalb Programmierung -> Hauptgleis können die Programmierungen über das Hauptgleis durch direkte Angabe der Adresse vorgenommen werden.

### PGM Decoder-Info

Hier erhält man nützliche Decoder-Informationen wie Hersteller, Version, Modell, Adresse und die wichtigsten Konfigurationsparameter.

Ganz wichtig: Da hier ohne die Angabe jedweder Adresse gearbeitet wird, darf nur eine Lok auf dem Programmiergleis stehen! Das Hauptgleis darf nicht
gleichzeitig angeschlossen sein!

Nach Betätigen der Schaltfläche "Decoder-Info lesen" erhält man die folgende Ausgabe:

![DCC-FM22 Decoder-Info](https://raw.githubusercontent.com/ukw100/FM22/main/images/pgm-info-3.png " Decoder-Info")

Dabei werden zunächst die Inhalte der wichtigsten Konfigurationsvariablen (CVs) angezeigt. Da sich die Bedeutung der CV #29 aus einzelnen Bits zusammensetzt, werden diese hier
noch einmal speziell erläutert.

Hinweis: Die Decoder-Infos über POM (Hauptgleisprogrammierung) werden nicht nur schneller ermittelt, sondern werden auch wesentlich ausführlicher dargestellt,
siehe [POM Decoder-Info](#pom-decoder-info).

### PGM Decoder-Adresse

Hier kann die Adresse des Decoders gewechselt werden:

![DCC-FM22 Decoder-Info](https://raw.githubusercontent.com/ukw100/FM22/main/images/pgm-addr-2.png " Decoder-Info")

Dabei werden Umstellungen von kurzer auf lange Adresse - und umgekehrt - automatisch vorgenommen. Beachte dazu auch die ausgegebenen Empfehlungen zu den Adressbereichen.

Hinweis: Die FM22-Zentrale bietet über [POM Decoder-Adresse](#pom-decoder-adresse) auch die Möglichkeit, die Adresse über POM zu ändern. Nur verweigern die meisten Decoder
aus Betriebssicherheitsgründen die Änderung der Adresse über POM. Diese kann dann nur über die Programmiergleis-Programmierung  (PGM) geändert werden.

### PGM Decoder-CV

Unter diesem Menüpunkt können einzelne CV-Werte geändert werden. Zunächst sollte man den Inhalt einer Konfigurationsvariablen auslesen:

![DCC-FM22 Decoder-CV](https://raw.githubusercontent.com/ukw100/FM22/main/images/pgm-cv-2.png " Decoder-CV")

Anschließend kann man den Wert ändern. Die Eingabe des neuen Wertes kann dezimal oder binär erfolgen. Durch Betätigung der Schaltfläche "CV Speichern" werden die neuen Daten in
der Konfigurationsvariablen gespeichert.

Hinweis: Die CV-Inhalte können auch über POM (Hauptgleisprogrammierung) ermittelt und geändert werden, siehe [POM Decoder-Info](#pom-decoder-info). Die Programmierung über POM
ist dabei im allgemeinen schneller.

## Programmierung über Hauptgleis (POM)

Neben der Decoder-Programmierung über PGM ermöglichen fast alle modernen DCC-Decoder auch die Programmierung direkt auf dem Hauptgleis (POM).

Dieses hat wesentliche Vorteile:

* Die Programmierung kann auf dem Hauptgleis vorgenommen werden. Ein spezielles vom Rest der Anlage isoliertes Programmiergleis ist nicht notwendig.
* Die Programmierung und die Rückmeldung erfolgt wesentlich schneller.
* Die Rückmeldung des Dedoders erfolgt nicht über Stromspitzen, sondern über RailCom.
* Es muss kein spezieller Verbraucher am Decoder angeschlossen werden.

Hinweis: Es gibt auch POM-fähige Decoder, die zwar über POM programmiert werden können, aber nicht Railcom-fähig sind. In diesem Fall bekommt man keine Rückmeldung vom Decoder,
ob die Programmierung erfolgreich war oder nicht. Im Zweifel muss man, wenn man die programmierten Werte wieder auslesen will, dann die Programmierung über das Programmiergleis
vornehmen (PGM).

### POM Decoder-Info

Hier erhält man nützliche Decoder-Informationen wie Hersteller, Version, Modell, Adresse und die wichtigsten Konfigurationsparameter.

Nach Betätigen der Schaltfläche "Decoder-Info lesen" erhält man die folgende Ausgabe:

![DCC-FM22 Decoder-Info](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-info-2.png "Decoder-Info")

Dabei werden zunächst die Inhalte der wichtigsten Konfigurationsvariablen (CVs) angezeigt. Da sich die Bedeutung der CV #29 aus einzelnen Bits zusammensetzt, werden diese hier
noch einmal speziell erläutert. Darunter werden dann die Inhalte der Konfigurationsvariablen #28 und #29 im Detail aufgeführt, da sich hier die Bedeutungen der Werte aus einzelnen
Bits zusammensetzen. Hier hat man die Möglichkeit diese Einzelwerte direkt zu ändern und zu speichern.

Zum Schluss werden noch erweiterte Decoder-Infos ausgegeben, die herstellerspezifisch sind und je nach Decoder unterschiedlich ausfallen. Das können unter anderem sein:

* Hersteller-ID
* Decodertyp
* Produktnummer
* Seriennummer
* Produktionsdatum und -zeit
* RailCom-Version
* Firmware-Version

### POM Decoder-Adresse

Hier kann die Adresse des Decoders gewechselt werden:

![DCC-FM22 Decoder-Adresse](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-addr.png "Decoder-Adresse")

Dabei werden Umstellungen von kurzer auf lange Adresse - und umgekehrt - automatisch vorgenommen. Beachte dazu auch die ausgegebenen Empfehlungen zu den Adressbereichen.

Hinweis: Die FM22-Zentrale bietet zwar über POM die Möglichkeit, die Adresse zu ändern. Nur verweigern die meisten Decoder aus Betriebssicherheitsgründen die Änderung der
Adresse über POM. Diese kann dann nur über die Programmiergleis-Programmierung [PGM Decoder-Adresse](#pgm-decoder-adresse) geändert werden.

### POM Decoder-CV

Unter diesem Menüpunkt können einzelne CV-Werte geändert werden. Zunächst sollte man den Inhalt einer Konfigurationsvariablen auslesen:

![DCC-FM22 Decoder-CV](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-cv.png "Decoder-CV")

Anschließend kann man den Wert ändern. Die Eingabe des neuen Wertes kann dezimal oder binär erfolgen. Durch Betätigung der Schaltfläche "CV Speichern" werden die neuen Daten in
der Konfigurationsvariablen gespeichert.

### POM Decoder-Motorparameter

Unter dem Menüpunkt "Motorparameter" lassen sich die für die Ansteuerung eines Lokmotors zuständigen Einstellungen gezielt anpassen. Hier
wird schwerpunktmäßig auf die Einstellungen der ESU-Lokdecoder eingegangen, da diese den größtmöglichen Rahmen von Einstellungsmöglichkeiten
bilden.

Nach der Auswahl des Menüpunktes "Motorparameter" muss zunächst der Hersteller ermittelt werden. Dafür ist die Angabe der Lokadresse notwendig:

![DCC-FM22 Motorparameter](https://raw.githubusercontent.com/ukw100/FM22/main/images/motorparameter.png "Motorparameter")

Nachdem die Adresse eingegeben wurde, wird der Hersteller automatisch ermittelt und angezeigt. Im untenstehenden Beispiel ist der Hersteller
"Electronic Solutions Ulm GmbH" oder auch kurz "ESU":

![DCC-FM22 Motorparameter Hersteller](https://raw.githubusercontent.com/ukw100/FM22/main/images/motorparameter-hersteller.png "Motorparameter Decoderhersteller")

Mit der Betätigung der Schaltfläche "Motorparameter lesen" werden nun alle für die Motorkonfiguration zuständigen Konfigurationsvariablen ("CVs")
eingelesen und kompakt angezeigt:

![DCC-FM22 Motorparameter bearbeiten](https://raw.githubusercontent.com/ukw100/FM22/main/images/motorparameter-bearbeiten.png "Motorparameter bearbeiten")

ESU selbst bietet für gängige Motor-/Lokhersteller bereits vorkonfigurierte Einstellungen an. Dabei werden alle vorkonfigurierte CV-Werte, die mit
den tatsächlich ausgelesenen Werten übereinstimmen, in grün angezeigt. Stimmen sämtliche gelesene CV-Variablen mit einer Beispielkonfiguration
ausnahmslos überein, wird der Motortyp zusätzlich grün hinterlegt. Im obigen Beispiel entspricht die ausgelesene Konfiguration der von der ESU
präferierten Einstellung für einen Märklin® 5-Sterne-Hochleistungsmotor.

Möchte man die Konfiguration ändern, kann man durch Betätigen einer der Schaltflächen "Speichern" eine vorkonfigurierte Einstellung übernehmen oder
in der letzten Zeile die CV-Werte individuell anpassen.

### POM Funktionsmapping

Über den Menüpunkt "Funktionsmapping" kann für Decoder einiger Hersteller das Funktionsmapping ausgelesen und geändert werden. Leider arbeiten hier die Hersteller
nicht einheitlich. Viele favorisieren hier ihre eigenen Methoden, wie sie das Funktionsmapping in CV-Konfigurationsvariablen speichern. Im folgenden wird hier das
Funktionsmapping von Lenz (einfach) und auch ESU (komplex, aber vielfältig) besprochen.

Zunächst gibt man nach dem Auswahl des Menüpunkts die Lokadresse ein. Die Zentrale ermittelt dann im ersten Schritt den Hersteller:

![DCC-FM22 Funktionsmapping](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-mapping-1.png "Funktionsmapping")

Im Falle eines Lenz-Decoders wird zunächst gefragt, ob der Decoder 4 oder 8 Ausgänge unterstützt. Je nach Angabe erscheint dann eine entsprechende
Tabelle, mit welcher Funktionstaste F0 - F28 welcher Ausgang angesteuert wird:

![DCC-FM22 Funktionsmapping Lenz](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-mapping-lenz.png "Funktionsmapping Lenz")

Die grünen Kästchen zeigen die Verknüpfung der Funktionstaste zum jeweiligen Ausgang an. Möchte man diese Konfiguration ändern, klickt man einfach auf die
entsprechende Tabellenzelle, um eine Verknüpfung herzustellen oder wieder zu löschen. Am Schluss klickt man auf die Schaltfläche "Änderungen senden".
Die geänderte Konfiguration wird dann im Decoder geändert.

Das Funktionsmapping von ESU ist hier wesentlich umfangreicher angelegt. Hier erkauft man sich die erweiterte Funktionalität aber auch mit einer erheblich höheren Komplexität.
Während hier die Programmierung über einzelne CVs zu einem aussichtslosen Unterfangen werden kann, reichen bei der FM22-Zentrale lediglich ein paar Mausklicks, um das
ESU-Funktionsmapping anzupassen und zu ändern.

Dabei muss man zunächst das Prinzip verstehen:

Während sonstige Hersteller eine direkte Verknüpfung einer Funktionstaste zu einem Ausgang vornehmen (siehe obiges Beispiel "Lenz"), gibt es hier einfach nur sogenannte
"Konfigurationszeilen". Diese bestehen aus bis zu 72 Zeilen. Standardmäßig sind lediglich 16 Zeilen mit einer Konfiguration belegt, deshalb man kann hier
auswählen, ob man die ersten 16, 32 oder 72 Zeilen einlesen möchte. Der Grund für diese Abfrage ist lediglich die für die Ermittlung der Werte erforderliche Zeitdauer. Das
Einlesen von 16 Zeilen dauert ca. 3-4 Sekunden, das Einlesen von 72 Zeilen dauert dann entsprechend länger - auch wenn diese Zeilen eventuell nicht genutzt werden. Dabei
werden ca. 100 Konfigurationsvariablen pro Sekunde gelesen.

Das Prinzip ist nun, dass man Bedingungen mit Ausgängen oder einer Logik verknüpft. Das kann in einer beliebigen Konfigurationszeile erfolgen, die nicht unbedingt an
eine bestimmte Funktionstaste verknüpft sein muss. Im Gegenteil: Ein Ausgang kann auch mit mehreren Funktionstasten verknüpft werden, die dann alle gleichzeitig
gedrückt werden müssen!

Das Standard-Funktionsmapping von ESU sieht folgendermaßen aus:

![DCC-FM22 Funktionsmapping ESU](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-mapping-esu.png "Funktionsmapping ESU")

Rote Kästchen bedeuten hier, dass die Bedingung (Fahrt, Richtung oder Funktionstaste) ausgeschaltet ist, grüne Kästchen bedeuten eine eingeschaltete Bedingung.

Dann erhalten die Konfigurationszeilen folgende Bedeutungen:

* Zeile 1: Wenn Vorwärtsrichtung an und F0 an, dann Licht vorn an.
* Zeile 2: Wenn Vorwärtsrichtung aus und F0 an, dann Licht hinten an.
* Zeile 3: Wenn Vorwärtsrichtung an und F1 an, dann Ausgang AUX1 an.
* Zeile 4: Wenn F2 an, dann Ausgang AUX2 an.
* Zeile 5: Wenn F3 an, dann Logik "Rangiergang" an.
* Zeile 6: Wenn F4 an, dann Logik "ABV aus" an.
* Zeile 7: Wenn F5 an, dann AUX3 an.
* Zeile 8: Wenn F6 an, dann AUX4 an.
* usw.

Nun kann man durch Klick auf eine Tabellenzelle die Bedeutung einer Verknüpfung ändern.

Der Klick auf eine Zelle in den Spalten "Bedingungen" löst folgendes aus:

* Erster Klick: Bedingung wird grün, d.h. der oder die Ausgänge werden gesetzt, wenn die Bedingung zutrifft.
* Zweiter Klick: Bedingung wird rot d.h. der oder die Ausgänge werden gesetzt, wenn die Bedingung NICHT zutrifft.
* Dritter Klick: Bedingung wird farblos, d.h. die Bedingung wird hier ignoriert.

Der Klick auf eine Zelle in den Spalten "Ausgang" oder "Logik" löst folgendes aus:

* Erster Klick: Zelle wird grün, d.h. der entsprechende Ausgang/Logik wird für die gegebenen Bedingungen gesetzt.
* Zweiter Klick: Zelle wird farblos, d.h. die Verknüpfung zur Bedingung wird gelöscht.

Hat man alle gewünschten Konfigurationen vorgenommen, dann kann man diese mit dem Klick auf die Schaltfläche "Änderungen speichern" vornehmen.

### POM Funktionsausgänge

Manche Hersteller ermöglichen es, die Funktionsausgänge des Decoders in ihrer Eigenschaft zu ändern. Das reicht vom langsamen Hochdimmen beim Einschalten über Blinken bis zu
umfangreichen Simulationen wie "Feuerbüchse" oder "Pantographensteuerung".

Um solche Funktionsausgänge zu konfigurieren, ist zunächst die Eingabe der Decoderadresse nötig.

![DCC-FM22 Funktionsausgänge Adresse](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-ausgaenge-1.png "Funktionsausgänge Adresse")

Anhand der Adresse wird dann im nächsten Schritt der Decoderhersteller ermittelt:

![DCC-FM22 Funktionsausgänge Hersteller](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-ausgaenge-2.png "Funktionsausgänge Hersteller")

Anschließend lässt sich dann die Konfiguration der Funktionsausgänge auslesen:

![DCC-FM22 Funktionsausgänge einlesen](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-ausgaenge-3.png "Funktionsausgänge einlesen")

Diese werden dann in einer Tabelle angezeigt. Hier ein Beispiel eines ESU-Decoders:

![DCC-FM22 Funktionsausgänge ESU](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-ausgaenge-4.png "Funktionsausgänge ESU")

<img align="right" src="https://github.com/ukw100/FM22/blob/main/images/pom-ausgaenge-5.png">

Man kann nun den sogenannten "Mode" des Ausgangs über eine Dropdown-Liste anpassen. Ebenso kann man eine Einschalt- und Ausschaltverzögerung konfigurieren.

Die automatische Abschaltung sorgt dafür, dass der Ausgang nach einer wählbaren Dauer automatisch deaktiviert wird. Das ist unter anderem für die Steuerung
von Kupplungen notwendig.

Jeden Ausgang kann man noch mit einer Pulsweitenmodulation (PWM) belegen, die zur Helligkeitssteuerung für LEDs genutzt werden kann. Dabei bedeutet der Wert
"0" "LED dunkel", der Wert "31" "maximale Helligkeit", alles andere dazwischen sind entsprechende Zwischenwerte.

Einige der möglichen Modes sind rechts aufgelistet. Diese können für jeden der möglichen Ausgänge mit einem Klick aus der Liste ausgewählt werden. Die vollständige
Liste von Modes und deren Beedeutung entnimmst Du am besten dem Decoder-Handbuch.

## Zentrale

Das Menü "Steuerung" bietet folgende Punkte:

<img align="right" src="https://github.com/ukw100/FM22/blob/main/images/menu3.png">

* [Einstellungen](#einstellungen)
* [Netzwerk](#netzwerk)
* [Upload Hex](#upload-hex)
* [Flash STM32](#flash-stm32)
* Info

### Einstellungen

Todo

### Konfiguration Netzwerk

Todo

### Firmware-Updates

Todo

## Anhang

_RailCom®_

"RailCom" ist eine auf den Namen von Lenz Elektronik für die Klasse 9 "Elektronische 
Steuerungen" unter der Nummer 301 16 303 eingetragene Deutsche Marke sowie ein für die 
Klassen 21, 23, 26, 36 und 38 "Electronic Controls for Model Railways" in U.S.A. unter 
Reg. Nr. 2,746,080 eingetragene Trademark.
