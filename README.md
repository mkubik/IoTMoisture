# IoTMoisture
Simple IoT device with an ESP8266 (ESP-12F) and a capacitive soil moisture sensor

## Introduction

This was my Saturday afternoon/evening project. After I thought that my wife is watering plants too often, I thought it would be a good idea to make watering a data driven decision rather than simply "feel" that it needs watering.

So I looked around and found a pretty neat device that had all I required:
- an onboard WIFI
- small in size
- powerable with batteries
- an analog input

Pretty quickly you stumble across pretty cheap devices (approx 2€ at time of writing, when ordered directly through AliExpress) named ESP8266

The choice of moisture sensors is also more than reasonable, there's only 2 types, one that has some rust problem over time and one that is capacitive. This adds 0.5€ more to the bill per device.

The whole solution is based on a kubernetes cluster. You should actually be able to pick any cloud provider and get a k8s cluster from there, however, I decided to get a local copy of [K3s](https://k3s.io).
The ESP8266 connects to a WiFi network and communicates to an [MQTT server](htps://mosquitto.org). The data is sent to an [InfluxDB](https;//wwww.influxdata.com) using their[Telegraf plugin](https://github.com/influxdata/telegraf/tree/master/plugins/inputs/mqtt_consumer). Once the data is in the InfluxDB, you can display the KPI's using [Grafana](https://grafana.com/) which I chose to install via helm.

### Overall architecture

![Architecture](./docs/images/architecture.svg)


## Prerequisites

Useful links, downloads and install:
- Arduino IDE: https://www.arduino.cc/en/main/software
- ESP8266 Firmware generator:  https://nodemcu-build.com/
- ESP8266 device support for Arduino IDE: http://arduino.esp8266.com/stable/package_esp8266com_index.json

## Hardware

### ESP8266 

There is a bunch of ESP8266 devices on AliExpress and Amazon. The ones that I got are these:

<img src="./docs/images/IMG_20190804_103353.jpg" alt="drawing" style="width:250px;"/>
<img src="./docs/images/IMG_20190804_103409.jpg" alt="drawing" style="width:250px;"/>

You'll find 2 types with different UART chips, namely CH340 and CP2102. For my purposes, I came to the conclusion that it wouldn't really matter but as the CP2102 is newer, I picked that one, but the CH340 variants should work exactly the same.

### Moisture Sensor

There's no real number or abreviation od any identifiable ID for these. But as there's only 2 types of sensors, one type that's mainly back coated and the other that has holes in it and is mainly whitish or transparent. Those also have  a separated electronics board, while the capacitive ones mostly have their electronics on the sensor board itself.

<img src="./docs/images/IMG_20190804_103430.jpg" alt="drawing" style="width:200px;"/>
<img src="./docs/images/IMG_20190804_103439.jpg" alt="drawing" style="width:200px;"/>

### Wires

You'll need one additional male-female patch cable for wiring that I had to bend to make if fit:

<img src="./docs/images/IMG_20190804_103450.jpg" alt="drawing" style="width:200px;"/>

Connecting the wires is dead-simple. The 3 connectors on the sensor are VCC (3.3 V input), GND (Ground, obviously) and AOUT (the humidity reading as a voltage)
Looking at the board, there's at least 3 pin pairs that offer 3.3V and GND. I picked the ones that made it easy to connect the patch cable.

<img src="./docs/images/IMG_20190804_103555.jpg" alt="drawing" style="width:200px;"/>
<img src="./docs/images/IMG_20190804_103619.jpg" alt="drawing" style="width:200px;"/>
<img src="./docs/images/IMG_20190804_103644.jpg" alt="drawing" style="width:200px;"/>

In order to save power and as moisture will not drop in milliseconds, it's sufficient to get readings every now and then. My impression is that 15min is way more than really required. You can play with it so it works best for you. 

I also use the *deepSleep* mode of the ESP8266, so it only consumes relevant power ever 15 min for a few seconds. This should make the battery last quite some time.

Note: deepSleep mode requires an additional wire from pin D0 to RST. This is required as the timer will reset the device. Note also, that for programming, you want to disconnect the wire.

## Power

I intend to power the devices with a 9V battery using a standard connector that needs to be soldered (or otherwise connected) to *Vin* and *GND*

<img src="./docs/images/IMG_20200102_205639.jpg" alt="9V connector" style="width:200px;"/>


## References

There's a few things that you may want to look at in more depth:

- Investigate "deep sleep": https://randomnerdtutorials.com/esp8266-deep-sleep-with-arduino-ide/

## Deployment

Besides the ESP8266 code which is deployed to the device with the [Arduino IDE](https://www.arduino.cc/en/main/software), there's a decent amount of steps that I didn't quite yet automate but rather just listed in [k3sinstall](./k3s/k3sinstall). Feel free to create a PR and help automating. As a general note, I used `mqtt.example.com` throughout this project. 

**You *MUST* change it to a domain you own, otherwise it'll fail.**

I'm describing the files that need modification prior to deployment:

### stagingissuer.yaml

Make sure you change the email address to match the one you want to use. Note, Letsencrypt requires the email to have the same domain as the host you're requesting the certificate for.

### prodissuer.yaml

See above, same here. Note that you should deploy both issuers and switch them in ingress.yaml.

### grafana-secrets.yaml

Change the entries using the output of the following command: `echo -n '<youradmin> | base64`. Do the same for the password.

### grafana-values.yaml

Changes are related to `persistence` and `admin` secrets. These are already done, so this is just fyi.

### ingress.yaml

Make sure you change the hostname to your hostname to match your hostname. For testing, switch to the issuer `cert-manager.io/cluster-issuer` to _letsencrypt-staging_. Once you get a cert, switch to _letsencrypt_prod_ and re-apply.

### mosquitto.yaml

I secured mosquitto with a password only to avoid fiddling with certificates on the ESP8266, so find the ConfigMap named `mosquitto-passwd` and create one or more entries with the outlined pattern using the [description provided by mosquitto](https://mosquitto.org/man/mosquitto_passwd-1.html).

### influx.yaml

Similar to _mosquitto.yaml_, find the `influxdb-secret` SecretMap and change the values in `<>` with clear text, this time (_stringData_ !).

### telegraf.yaml

Telegraf now connects MQTT (as subscriber) to the InfluxDB so that we can display data in Grafana. Find the `inputs.mqtt_consumer` section and verify it matches your setup. Now look for the `telegraf-secrets` SecretMap and change the values in `<>` with clear text.

### Your DSL/WAN router

I had to expose ports 443 and 1883 in my router to make them externally accessible. If you don't want/need that, you can probably also skip the letsencrypt stuff. For me, I decided to hook up the ESP8266's to an external (different) network, so I had to go down that path. 
Note that for non-80/443 ports, Kubernetes will map ports to the 30000 range, so you need to map port 3xxxx (what you get from NodePort when running `kubectl get svc`) to 1883 in your router. Actually, as this port is configurable everywhere, it can be any port, just change it consistently in the files.

## Monitoring

As outlined above, I decided to use _Grafana_ for monitoring. If you followed my description you should be able to access your dashboard by pointing your browser to https://mqtt.example.com and get redirected to a login screen. 
I added the [properties](./k3s/grafana-dashboard.json) for my dashboard. Just configure the datasource connection to your InfluxDB with `http://influxdb:8086` and username/password as configured in `influx.yaml` and you should be able to see something like this:

<img src="./docs/images/grafana.png" alt="grafana" style="width:800px;"/>

## General Note

I found [this guide](https://kubernetes.io/docs/tasks/debug-application-cluster/debug-service/) extremely helpful to figure out why services are not accessible as expected.

## Happy Plants

<img src="./docs/images/IMG_20190804_124235.jpg" alt="pic" style="width:200px;"/>



## TODOs

- Put everything in a waterproof box
- Check power consumption (deep sleep)


