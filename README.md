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

BOM:

|Pos|Anzahl|Name|Wert|Reichelt Bestellnr.|Bemerkung|
|---|------|----|----|-------------------|---------|
|1|5|C1, C3, C4, C5, C7|100nF|X7R-2,5 100N KOM||
|2|1|C2|220µF/35V|RAD FR 220/50 (RM 5,08)|alternativ RAD FR 220/35|
|3|1|C6|100µF/16V|RAD FR 100/16||
|4|1|D1|1N4007|1N 4007||
|5|2|D2, D3|SB320|SB 320||
|7|1|IC2|MAX485CPA|MAX 485 CPA||
|8|1|IC3|78L05|µA 78L05||
|9|1|IC4|6N137|6N 137||
|10|1|IC5|LM393N|LM 393 DIP||
|11|1|IC6|PC817|LTV 817||
|12|2|K1, K2|RJ45_8POLIG|MEBP 8-8S||
|13|1|K3|Stiftleiste_1x06_W_2,54|SL 1X40W 2,54||
|14|1|K4|Stiftleiste_1x06_G_2,54|SL 1X40G 2,54||
|15|1|K6|KLEMME2POL|AKL 101-02||
|16|1|K7|KLEMME4POL|AKL 101-04||
|17|3|R1, R3, R11|22,1K|METALL 22,1K||
|18|1|R2|4,87K|METALL 4,87K||
|19|1|R4|4,64K|METALL 4,64K||
|20|1|R5|1,8 2W|2W METALL 1,8||
|21|2|R6, R7|390|METALL 390||
|22|1|R8|560|METALL 560||
|23|2|R9, R16|10K|METALL 10,0K||
|24|1|R10|4,75K|METALL 4,75K||
|25|1|R12|120|METALL 120||
|26|3|R13, R17, R18|1K|METALL 1,00K||
|27|1|R14|33 2W|2W METALL 33||
|28|1|R15|5,1K|METALL 5,10K||
|29|1|RPI Header|40pol|RPI HEADER 40||
|30|1|RPI Abstandsbolzen Set|20mm|RPI MOUNTINGKIT2||
|31|4|IC-Sockel 8-polig||GS 8P|Einen Sockel für PC817 halbieren|

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

* Einschalten der Stirnbeleuchtung
* Nach 5 Sekunden Einschalten der Innenbeleuchtung
* Nach 7 Sekunden Einschalten des Betriebsgeräusches
* Nach 10 Sekunden Bahnhofsdurchsage
* Nach 15 Sekunden Schaffnerpfiff
* Nach 18 Sekunden Fahrtaufnahme durch Erhöhung der Geschwindigkeit über eine Rampendefinition.
* Nach 24 Sekunden Lokpfiff

Diese war nur ein Beispiel. Insgesamt können bis zu 8 Macros pro Lok definiert werden. Weitere Informationen siehe [Lok-Macros](#lok-macros).

Die Spalte "Adresse" gibt die Lokadresse aus. Die Spalte "RC2" zeigt die Übertragungsqualität von RailCom-Rückmeldungen während des Betriebs.
Solange keine Störungen auf der Anlage sind, sollten hier Werte zwischen 98% und 100% ausgegeben werden. Bei 100% wurde jeder DCC-Befehl korrekt
beantwortet und auch vollständig von der Zentrale "verstanden".

Der Button "Bearbeiten" erscheint nur im Bearbeitungsmodus. Hier können dann die Grundeinstellungen der Lok - wie zum Beispiel die Adresse -
geändert werden.

Durch Klick auf die Lokbezeichnung gelangt man in die Loksteuerung der gewünschten Lok.

### Loksteuerung

Auf dieser Seite kann die ausgewählte Lok gesteuert werden. Dies ist über Smartphone, Tablett, direkt am Pi oder auch mit einem PC möglich.
So können mehrere Personen verschiedene Loks gleichzeitig steuern.

Hier sieht man:

* Lokadresse mit Online-Status
* Aufenthaltsort der Lok
* Aktuelle Geschwindigkeit der Lok
* Richtung
* Fahrtziel
* Bezeichnungen und Zustände der Funktionstasten

![DCC-FM22 Loksteuerung](https://raw.githubusercontent.com/ukw100/FM22/main/images/lok1.png "Loksteuerung")

Im obigen Beispiel wird als Lokadresse 1012 ausgegeben. Die Adresse ist grün hinterlegt, um zu signalisieren, dass die Lok "online" ist.
Der Aufenthaltsort der Lok ist gerade "Einfahrt Schattenbahhnhof", die Geschwindigkeit ist "0" und die Richtung ist "vorwärts".
Außerdem ist gerade die Funktion F0 "Spitzensignal / Schlusslicht rot" aktiv.

Alle Ausgaben können sich sofort ändern, wenn eine andere Person oder eine aktivierte Ereignissteuerung in den Ablauf eingreift. Mittels
Schieberegler kann die Geschwindigkeit, mit den Pfeilen darunter kann die Fahrtrichtung geändert werden. Ein Klick auf das schwarze Quadrat
stellt die Geschwindigkeit auf 0, ein Klick auf den schwarzen Kreis daneben führt einen Notstopp aus, d.h. die Lok bleibt dann sofort stehen - ohne
Berücksichtigung irgendwelcher eingesteller Bremsverzögerungen.

Insgesamt können 8 Lok-Macros (MF, MH, M3 - M8) definiert und ausgelöst werden, siehe auch [Lok-Macros](#lok-macros).

### Neue Lok

In der Lokliste kann durch Klick auf den Button "Neue Lok" unterhalb der Liste eine neue Lok angelegt werden. Dieser Button erscheint nur
im Bearbeitungsmodus. Es erscheinen dann direkt unterhalb des Buttons Eingabefelder, mittels derer man die neue Lok anmelden kann:

![DCC-FM22 Neue Lok](https://raw.githubusercontent.com/ukw100/FM22/main/images/neue-lok.png "Neue Lok")

Im obigen Beispiel wurde als Name "BR 628 Triebwagenzug" gewählt, die Adresse auf 1019 gesetzt und die Fahrstufen mit "128" angegeben.
Wird die Lok als "Aktiv" eingestellt, wird sie auch durch Lokbefehle gesteuert. Ist die Lok hier deaktiviert, werden keine Lokbefehle
an die Lok gesandt. Nach Klick auf Speichern erscheint dann die neue Lok in der Lokliste.

### Lok bearbeiten

Klickt man in der Lokliste auf den Button "Bearbeiten", kann man die Grundeinstellungen wie Name und Adresse ändern:

![DCC-FM22 Lok bearbeiten](https://raw.githubusercontent.com/ukw100/FM22/main/images/lok-bearbeiten.png "Lok bearbeiten")

Durch Einstellung der ID kann man auch den gewünschten Lokeintrag in der Liste von oben nach unten und umgekehrt "wandern" lassen,
die Lokliste also neu sortieren. Sonst gelten dieselben Einstellungen wie bei einer neuen Lok.

### Lok-Macros

Durch Lok-Macros können bestimmte Aktionen automatisiert ablaufen. Insgesamt sind bis zu 16 Aktionen pro Lok-Macro möglich. Im unteren
Beispiel sind folgende Aktionen eingestellt:

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

Zusatzdecoder sind weitere Decoder im Zug. Das kann zum Beispiel ein Lichtdecoder im Steuerwagen des Zugs sein, hier beispielsweise
für einen IC-Steuerwagen:

![DCC-FM22 Zusatzdecoder](https://raw.githubusercontent.com/ukw100/FM22/main/images/zusatzdecoder.png "Zusatzdecoder")

Man konfiguriert einen neuen Zusatzdecoder über den entsprechenden Button. Anschließend wählt man aus bzw. stellt man ein:

* Name, z.B. Inennbeleuchtung ICE2
* Adresse, hier 2001
* Verknüpfung mit Lok im Zug

Durch die Verknüpfung des Zusatzdecoders mit der dazugehörenden Lok erscheinen die möglichen Zusatzfunktionen des Decoders bei
der Bedienung der Lok. Ein Beispiel dazu folgt weiter unten.

![DCC-FM22 Zusatzdecoder neu](https://raw.githubusercontent.com/ukw100/FM22/main/images/addon-neu.png "Zusatzdecoder neu")

Durch Klick auf "Bearbeiten lassen sich die Einstellungen nachträglich ändern. Außerdem kann man durch Angabe einer abweichenden ID
die Reihenfolge der Zusatzdecoder in der Anzeige ändern.

![DCC-FM22 Zusatzdecoder bearbeiten](https://raw.githubusercontent.com/ukw100/FM22/main/images/addon-bearbeiten.png "Zusatzdecoder bearbeiten")

Hier ein Beispiel: Der Zusatzdecoder der im IC Steuerwagen wurde hier mit dem dre Lok "BR 103 Intercity" verknüpft. Nun erscheinen
bei der entsprechenden Lok die Zusatzfunktionen wie

* F0 Spitzensignal / Schlusslicht rot
* F1 Fernlicht
* F2 Innenbeleuchtung
* F3 Führerstandsbeleuchtung
* F4 Stromführende Kupplung

unterhalb der Bedienungselemente für die Lok. Richtungswechsel der Lok haben auch direkte Auswirkungen auf die eingestellte Richtung
für den Zusatzdecoder, z.B. den Wechsel der Stirnbeleuchtung von Rot auf Weiß und umgekehrt.

Hier kann man die Funktion F0 des Zusatzdecoders mit F0 der Lok verknüpfen. Das heißt: Schalte ich nun F0 der Lok ein, wird auch
automatisch die Funktion F0 des Zusatzdecoders entsprechend der Richtung der Lok aktiviert. Damit ist nun die Stirnbeleuchtung an beiden
Enden des Zuges entprechend zueinander passend eingestellt.

![DCC-FM22 Zusatzdecoder Lok](https://raw.githubusercontent.com/ukw100/FM22/main/images/lok-addon.png "Zusatzdecoder Lok")

### Weichen

Unter dem Menüpunkt "Weichen" können die Weichen der Anlage konfiguriert werden. Diese bindet man dann später am besten in eine oder
mehrere Fahrstraßen ein. Dann werden bei Aktivierung einer Fahrstraße direkt Gruppen von Weichen zeitlich versetzt geschaltet, um eine
Fahrstraße zu bilden.

![DCC-FM22 Weichen](https://raw.githubusercontent.com/ukw100/FM22/main/images/weichen.png "Weichen")

In dieser Ansicht kann man die Weichen auch manuell schalten, falls dies ausnahmsweise notwendig sein sollte. Dies geschieht automatisch
beim Klick auf die entsprechende Schaltfläche wie "Gerade", "Abzweig" und "Abzweig2", wobei letztere nur für 3-Wege-Weichen angezeigt wird.

Konfiguriert man eine neue oder bereits vorhandene Weiche, kann man folgende Einstellungen vornehmen:

* Name
* Adresse
* Haken für 3-Wege-Weiche

### Fahrstraßen

Fahrstraßen fassen bestimmte Gleise, die duch Weichen angesteuert werden, zu Gruppen zusammen.

Hier ein Beispiel:

![DCC-FM22 Fahrstraßen](https://raw.githubusercontent.com/ukw100/FM22/main/images/fahrstrassen.png "Fahrstraßen")

Diese Fahrstraße namens "Schattenbahnhof" besteht aus insgesamt 9 Gleisen. Jedes dieser Gleise kann einen Zug ("verknüpfte Lok")
aufnehmen. Die Gleise kann man - bei verschiedenen Längen sinnvoll - fest mit den Loks verknüpfen oder auch frei wählbar lassen.

In dieser Ansicht werden dargestellt:

* Gleisname
* Verknüpfte Lok
* Aktivierung des Gleises durch RailCom-Detektor oder S88-Kontaktgleis
* Auf dem Gleise erkannte Lok

Welches Gleis der Fahrstraße gerade aktiv ist, wird durch die grüne Markierung in der Spalte ID angezeigt. In unserem Beispiel wurde
das Gleis 1 ausgewählt, um die verknüpfte Lok einfahren zu lassen. Diese wurde auch erkannt, wie man weiter rechts sieht. Man kann
ein bestimmtes Gleis auch selbst durch Klick auf die Schaltfläche "Setzen" selber aktivieren. Dann werden alle für dieses Gleis zuständigen
Weichen automatisch so umgeschaltet, dass der Weg zum Gleis 1 im Schattenbahnhof aktiviert ist.

Durch Klick auf "Neue Fahrstraße" wird eine neue Fahrstraße erzeugt. Dieser muss man zunächst einen Namen geben:

![DCC-FM22 Neue Fahrstraße](https://raw.githubusercontent.com/ukw100/FM22/main/images/neue-fahrstrasse.png "Neue Fahrstraße")

Die Fahrstraßen lassen sich durch "Bearbeiten" auch nachträglich umbenennen oder durch Wahl einer neuen ID auch umsortieren:

![DCC-FM22 Fahrstraße bearbeiten 1](https://raw.githubusercontent.com/ukw100/FM22/main/images/fahrstrasse-bearbeiten-1.png "Fahrstraße bearbeiten 1")

Nachdem man eine Fahrstraße angelegt hat, kann man nun im nächsten Schritt die Fahrstraße konfigurieren. Dabei definiert man die dazugehörenden
Gleise. Im untenstehenden Beispiel wird gezeigt, wie das Gleis 9 der Fahrstraße eingerichtet wird.

Also wird zunächste der Name festgelegt: "Gleis 9". Anschließende kann man - falls gewünscht - eine Lok mit dem Gleis verknüpfen. Damit
bekommt die Lok ein "Heimatgleis", welches sie immer anfahren kann, wenn sie abgestellt werden soll:

![DCC-FM22 Fahrstraße bearbeiten 2](https://raw.githubusercontent.com/ukw100/FM22/main/images/fahrstrasse-bearbeiten-2.png "Fahrstraße bearbeiten 2")

Als letztes müssen noch die Weichen so eingestellt werden, dass die Lok auch ihr Heimatgleis erreichen kann. Im Beispiel ist dafür hier
notwendig:

* Weiche 5 auf Abzweig
* Weiche 3 auf Gerade
* Weiche 7 auf Abzweig
* Weiche 4 auf Gerade

Wird die Lok nun im Laufe Ihrer Fahrt irgendwann in den Schattenbahnhof geleitet, werden sich anschließend die Weichen automatisch auf
das Heimatgleis einstellen, die Lok ihre Fahrt verlangsamen und am Ende sicher auf dem Heimatgleis zum Stehen kommen. Wie hier das
Zusammenspiel von RC-Detektoren, S88-Kontakten und Fahrstraßen funktioniert, wird weiter unten detailliert erklärt.

### Signale

Signale sind nicht für die Steuerung notwendig. Sie dienen nur der optischen Anzeige. Die Steuerung selbst geschieht ausnahmslos über
die FM22-Zentrale.

Unter dem Menüpunkt "Signale" werden die Signale definiert. Hier werden nur Signale unterstützt, die lediglich 2 Zustände kennen. Unter
dem Menüpunkt "LEDs" können bis zu 8 LEDs zu einem Signal zusammengefasst werden. Dazu kommen wir weiter unten.

Im Beispiel unten wurden 4 Signale konfiguriert:

![DCC-FM22 Signale](https://raw.githubusercontent.com/ukw100/FM22/main/images/signale.png "Signale")

Neben dem Namen und der Adresse wird hier auch der aktuelle Anzeigestatus der Signale angezeigt. im oberen Beispiel befinden sich zwei Signale
im Zustand "Fahrt" und zwei im Zustand "Halt". Das wird auch durch die Farben Grün und Rot entsprechend angezeigt. Durch Klick auf
die entsprechenden Schaltflächen kann man den Zustand der Signale manuell wechseln. Sinnvoller ist jedoch so ein Lichtsignalwechsel
über eine Aktion eines RC-Detektors oder S88-Kontaktgleises.

Neben der Neuanlage eines Signals kann man diese auch noch nachträglich bearbeiten. Hier lässt sich neben dem Namen und der Adresse auch noch
die Anzeigereihenfolge über die ID bewerkstelligen:

![DCC-FM22 Signal bearbeiten](https://raw.githubusercontent.com/ukw100/FM22/main/images/signal-bearbeiten.png "Signal bearbeiten")

### LEDs

Unter dem Menüpunkt "LEDs" können Lichtsignale gesteuert werden, die mehr als 2 Zustände anzeigen können. Das sind entweder komplexere
Lichtsignale oder es können auch Beleuchtungen von Gebäuden sein, zum Beispiel durch Verwendung eine FM22-RC-Detektors mit WS2812-LED-Ausgang.

In der Regel werden für komplexe Lichtsignale sogenannte "erweiterte Zubehördecoder" verwendet. Hier werden dann zum Decoder nicht
nur zwei Zustände wie "Fahrt" und "Halt" übertragen, sondern ein ganzer Byte-Wert, welches insgesamt 256 Zustände annehmen kann bzw. bis zu 8 LEDs
unabhängig voneinander steuert.

Im untenstehenden Beispiel wurden zwei LED-Gruppen von je 8 LEDs angelegt, nämlich ein "Brückenstellwerk" und noch die Beleuchtung
des Bahnhofs "Kümmelhausen":

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

Heutzutage ist die Programmierung über das Programmiergleis nur noch für eine einzige Funktion gedacht, nämlich das Übertragen bzw. Einstellen
einer neuen Decoder-Adresse. Alle anderen Programmierungen lassen sich mit modernen Decodern auf dem Hauptgleis erledigen. Damit muss
die Lok also gar nicht mehr von der Anlage genommen werden. Die heutigen Decoder erlauben auch sämtliche Programmierfunktionen über
das Hauptgleis - mit einer Ausnahme: Meist kann die Adresse ausschließlich über das Programmiergleis eingestellt werden.

Der Grund dafür ist einfach: Auch nach Ändern der Decoderadresse verliert man niemals die Kontrolle über den Decoder, denn hier
können sämtliche Konfigurationsvariablen - auch die Adresse - ohne Kenntnis der Adresse durchgeführt werden.

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

![DCC-FM22 Funktionsmapping](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-mapping-1.png "Funktionsmapping")

![DCC-FM22 Funktionsmapping Lenz](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-mapping-lenz.png "Funktionsmapping Lenz")

![DCC-FM22 Funktionsmapping ESU](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-mapping-esu.png "Funktionsmapping ESU")

### POM Funktionsmapping

![DCC-FM22 Funktionsmapping](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-mapping-1.png "Funktionsmapping")

![DCC-FM22 Funktionsmapping Lenz](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-mapping-lenz.png "Funktionsmapping Lenz")

![DCC-FM22 Funktionsmapping ESU](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-mapping-esu.png "Funktionsmapping ESU")


### POM Funktionsausgänge

![DCC-FM22 Funktionsausgänge Adresse](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-ausgaenge-1.png "Funktionsausgänge Adresse")

![DCC-FM22 Funktionsausgänge Hersteller](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-ausgaenge-2.png "Funktionsausgänge Hersteller")

![DCC-FM22 Funktionsausgänge einlesen](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-ausgaenge-3.png "Funktionsausgänge einlesen")

![DCC-FM22 Funktionsausgänge ESU](https://raw.githubusercontent.com/ukw100/FM22/main/images/pom-ausgaenge-4.png "Funktionsausgänge ESU")

<img align="right" src="https://github.com/ukw100/FM22/blob/main/images/pom-ausgaenge-5.png>

Einige der umfangreichen Modi sind rechts aufgelistet. Für jeden der möglichen Ausgänge können sie mit einem Klick aus
der Liste ausgewählt werden.

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
