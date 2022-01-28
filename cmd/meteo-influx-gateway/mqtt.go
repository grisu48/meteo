package main

import (
	"fmt"
	"os"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

// MqttReceive is the message receive callback
type MqttReceive func(msg mqtt.Message)

// MQTT client structure
type MQTT struct {
	client   mqtt.Client
	address  string
	port     int
	ClientID string
	Verbose  bool
	Callback MqttReceive
}

// Connect connects the MQTT instance to the given server
func (mq *MQTT) Connect(address string, port int) error {
	mq.address = address
	mq.port = port
	opts := mqtt.NewClientOptions()
	opts.AddBroker(fmt.Sprintf("tcp://%s:%d", address, port))
	opts.KeepAlive = 30
	opts.AutoReconnect = true
	opts.ClientID = mq.ClientID
	opts.ResumeSubs = true
	opts.OnConnect = func(client mqtt.Client) {
		if mq.Verbose {
			fmt.Printf("mqtt connected: %s:%d\n", mq.address, mq.port)
		}
		if err := mq.subscribe("meteo/#", mq.Callback); err != nil {
			if mq.Verbose {
				fmt.Fprintf(os.Stderr, "Error subscribing to mqtt topic: %s\n", err)
			}
		}
	}
	opts.OnReconnecting = func(client mqtt.Client, opts *mqtt.ClientOptions) {
		if mq.Verbose {
			fmt.Println("mqtt reconnecting")
		}
	}

	opts.OnConnectionLost = func(client mqtt.Client, err error) {
		if mq.Verbose {
			fmt.Fprintf(os.Stderr, "mqtt connection lost: %v\n", err)
		}
	}

	mq.client = mqtt.NewClient(opts)
	token := mq.client.Connect()
	for !token.WaitTimeout(30 * time.Second) {
	}
	return token.Error()
}

// Subscribe to a given topic with the given callback function
func (mq *MQTT) subscribe(topic string, callback MqttReceive) error {
	token := mq.client.Subscribe(topic, 0, func(client mqtt.Client, msg mqtt.Message) {
		callback(msg)
	})
	token.Wait()
	return token.Error()
}
