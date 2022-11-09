package monitor

import (
	"fmt"
	mqtt "github.com/eclipse/paho.mqtt.golang"
	"log"
	"os"
	"os/signal"
	"sensor_monitoring/internal/config"
	"sensor_monitoring/internal/models"
	"syscall"
)

type Monitor struct {
	client     mqtt.Client
	cameras    []models.Camera
	cameraList map[string]struct{}
}

func Init(configPath string) *Monitor {
	conf := config.Configuration{}
	err := config.SetConfig(configPath, &conf)
	if err != nil {
		log.Fatal(err)
	}
	monitor := &Monitor{}
	mqtt.DEBUG = log.New(os.Stdout, "", 0)
	mqtt.ERROR = log.New(os.Stdout, "", 0)
	opts := mqtt.NewClientOptions()
	opts.AddBroker(fmt.Sprintf("tcp://%s:%d", conf.MosquittoAddr, conf.MosquittoPort))
	opts.SetClientID("monitor")
	opts.SetWill("monitor/state", "offline", 2, true)
	opts.SetUsername(conf.MosquittoUser)
	opts.SetPassword(conf.MosquittoPass)
	monitor.client = mqtt.NewClient(opts)
	monitor.cameras = conf.Cameras
	for _, cam := range monitor.cameras {
		monitor.cameraList[cam.Name] = struct{}{}
	}
	return monitor
}

func (monitor *Monitor) Run() {
	shutdownCh := make(chan struct{}, 1)
	go func() {
		sigint := make(chan os.Signal, 1)
		signal.Notify(sigint, syscall.SIGINT, syscall.SIGTERM, syscall.SIGABRT, syscall.SIGHUP, syscall.SIGKILL, syscall.SIGQUIT)
		<-sigint
		shutdownCh <- struct{}{}
	}()
	if token := monitor.client.Connect(); token.Wait() && token.Error() != nil {
		log.Println(fmt.Errorf("can't connect: %v", token.Error()))
		return
	}
	topics := make(map[string]byte)
	for camera := range monitor.cameraList {
		topics["cameras/"+camera+"/dump"] = 0
	}
	if token := monitor.client.SubscribeMultiple(topics, monitor.getCVHandler()); token.Wait() && token.Error() != nil {
		log.Println(fmt.Errorf("can't subscribe: %v", token.Error()))
		return
	}
	if token := monitor.client.Subscribe("cameras/#/state", 2, monitor.getOnlineHandler()); token.Wait() && token.Error() != nil {
		log.Println(fmt.Errorf("can't subscribe: %v", token.Error()))
		return
	}
	<-shutdownCh
	monitor.client.Disconnect(60)
}

func (monitor *Monitor) getCVHandler() mqtt.MessageHandler {
	return func(client mqtt.Client, msg mqtt.Message) {

	}
}

func (monitor *Monitor) getOnlineHandler() mqtt.MessageHandler {
	return func(client mqtt.Client, msg mqtt.Message) {

	}
}
