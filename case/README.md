# Gehäuseplan

OpenSCAD öffnen und `part` auf `base` oder `lid` stellen. Mit F6 rendern und
das jeweilige Teil als STL exportieren. Der Deckel liegt dabei mit der
sichtbaren Seite auf dem Druckbett. PETG und M3-Schrauben sind der Ausgangspunkt.
Die Schraubdome sind für Vorbohren bzw. Gewindeschneiden modelliert.
Die Platinen im Boden sitzen auf Klebehaltern. Für CONFIRM und BACK sind
separate Stößel vorgesehen, deren Länge vom gemessenen Tastenabstand abhängt.

Vor dem Druck am realen Aufbau messen:

- Größe und Lochabstand der OLED-Platine sowie Lage der sichtbaren Fläche,
  der Encoderachse und beider Tasten zu den Montagelöchern.
- Höhe der Encoderbuchse und Hub der Taster. Abstand zwischen Glas, Platine und
  Deckel mit Abstandshaltern einstellen. Das Glas darf keine Last aufnehmen.
- Position von USB-Buchse und SD-Schacht mit montierten Haltern. USB-Stecker und
  SD-Karte außen greifen können.
- Maße des konkreten ISP-Steckers und Platz für Flachbandkabel und Zugentlastung.
- Bauhöhe aller Module mit Steckern, Kabeln und Mindestabstand zu den Deckelbolzen.

Zuerst nur die Frontplatte aus Pappe oder als dünnen Probedruck testen. Die
Ausschnitte im OpenSCAD-Modell sind Startwerte aus dem Konzept.
