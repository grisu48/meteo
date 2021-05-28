package main

import (
	"fmt"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

type MqttReceive func(msg mqtt.Message)

type MQTT struct {
	client mqtt.Client
}

func ConnectMQTT(remote string, port int) (MQTT, error) {
	var ret MQTT
	opts := mqtt.NewClientOptions()
	remote = fmt.Sprintf("tcp://%s:%d", remote, port)
	opts.AddBroker(remote)
	opts.AutoReconnect = true

	ret.client = mqtt.NewClient(opts)
	token := ret.client.Connect()
	for !token.WaitTimeout(5 * time.Second) {
	}
	return ret, token.Error()
}

func (mq *MQTT) Subscribe(topic string, callback MqttReceive) error {
	token := mq.client.Subscribe(topic, 0, func(client mqtt.Client, msg mqtt.Message) {
		callback(msg)
	})
	token.Wait()
	return token.Error()
}
