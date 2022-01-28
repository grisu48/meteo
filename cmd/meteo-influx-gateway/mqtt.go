package main

import (
	"fmt"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

// MqttReceive is the message receive callback
type MqttReceive func(msg mqtt.Message)

// MQTT client structure
type MQTT struct {
	client mqtt.Client
}

// ConnectMQTT connects to a given MQTT server
func ConnectMQTT(address string, port int) (MQTT, error) {
	var ret MQTT
	opts := mqtt.NewClientOptions()
	opts.AddBroker(fmt.Sprintf("tcp://%s:%d", address, port))
	opts.KeepAlive = 30
	opts.AutoReconnect = true

	ret.client = mqtt.NewClient(opts)
	token := ret.client.Connect()
	for !token.WaitTimeout(5 * time.Second) {
	}
	return ret, token.Error()
}

// Subscribe to a given topic with the given callback function
func (mq *MQTT) Subscribe(topic string, callback MqttReceive) error {
	token := mq.client.Subscribe(topic, 0, func(client mqtt.Client, msg mqtt.Message) {
		callback(msg)
	})
	token.Wait()
	return token.Error()
}
