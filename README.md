# SAE – Hôpital de campagne : partie sécurité

Contrôle d'accès par badge pour un hôpital de campagne, avec supervision en temps réel.

## Architecture

ESP32 + lecteur PN532 → MQTT (Mosquitto) → Node-RED → InfluxDB

- Le lecteur de badge (ESP32 Huzzah32 + PN532) lit l'identifiant du badge.
- Node-RED décide si l'accès est autorisé, puis commande la porte.
- InfluxDB enregistre les accès et les mesures des capteurs simulés pour la supervision.

## Topics MQTT

- `iot/badge/access` : badge lu par le lecteur
- `securite/alertes` : alertes de sécurité (badge inconnu, brute force, température élevée)
- `securite/porte/commande` : ordre d'ouverture ou de refus envoyé au lecteur

## Détection d'anomalies

- 5 refus en 60 secondes : brute force (critique)
- Badge inconnu : critique
- Accès non autorisé dans une salle : avertissement
- Température supérieure à 28 °C : critique

## Contenu du dépôt

- `esp32/` : code Arduino du lecteur de badge
- `node-red/` : docker-compose.yml, configuration Mosquitto et flux Node-RED (flows.json)
- `docs/` : captures d'écran (Node-RED, InfluxDB)

## Lancer le projet

Dans le dossier node-red, lancer : docker compose up -d

Importer ensuite flows.json dans Node-RED (menu, Importer).

Avant de téléverser le code ESP32, renseigner le nom du WiFi, son mot de passe et l'adresse IP du broker MQTT.

## Vidéo de démonstration

https://youtu.be/m2cwiAwQjhk 

## Auteure

Aminata Yabre – BUT R&T, IUT Nord Franche-Comté
