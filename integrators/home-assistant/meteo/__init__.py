"""
meteo MQTT component

Listens for meteo messages on MQTT and updates the state accordingly.

Offets a service 'meteo'

Also offers a service 'set_state' that will publish a message on the topic that
will be passed via MQTT to our message received listener. Call the service with
example payload {"new_state": "some new state"}.

Configuration:

To use the meteo component you will need to add the following to your
configuration.yaml file.

meteo:
  topic: "meteo"
"""
from homeassistant.components import mqtt

import voluptuous as vol
import json
import logging

_LOGGER = logging.getLogger(__name__)

# The domain of your component. Should be equal to the name of your component.
DOMAIN = "meteo"

CONF_TOPIC = 'topic'
DEFAULT_TOPIC = 'meteo/#'

## Schema to validate the configured MQTT topic
#CONFIG_SCHEMA = vol.Schema({
#    vol.Optional(CONF_TOPIC, default=DEFAULT_TOPIC): mqtt.valid_subscribe_topic
#})


def friendly_name(name) :
    if name == "t" : return "Temperature"
    elif name == "hum" : return "Humidity"
    elif name == "p" : return "Pressure"
    else : return name

def setup(hass, config):
    topic = config[DOMAIN][CONF_TOPIC]
    hass.states.set("meteo.present", "yes")

    # Listen to a message on MQTT.
    def received(topic, payload, qos):
        # Parse json
        try :
            j = json.loads(payload)
            station_id = int(j['id'])
            station_name = j['name']
            hass.states.set("meteo.%d"%station_id, station_name)
            base = "sensor.meteo_%d" % station_id
            hass.states.set(base + "_name", station_name)
            attr = {}
            for x in j :
                if x in ["id","name"] : continue
                try :
                    attr = {}
                    attr['friendly_name'] = "%s %s" % (station_name, friendly_name(x))
                    hass.states.set(base + "_" + x, float(j[x]),attr)
                except ValueError :
                    continue
        except Exception as e:
            _LOGGER.info("meteo invalid json received: %s (%s)" % (payload, e))
            # Ignore for now
            return

    hass.components.mqtt.subscribe(topic, received)

    # Return boolean to indicate that initialization was successfully.
    return True
