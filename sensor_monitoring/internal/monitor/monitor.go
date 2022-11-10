package monitor

import (
	"fmt"
	mqtt "github.com/eclipse/paho.mqtt.golang"
	"log"
	"math"
	"os"
	"os/signal"
	"sensor_monitoring/internal/config"
	"sensor_monitoring/internal/cv"
	"sensor_monitoring/internal/models"
	"strconv"
	"strings"
	"syscall"
)

type Monitor struct {
	client     mqtt.Client
	cameras    []models.Camera
	cameraList map[string]int
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
	opts.SetDefaultPublishHandler(defaultHandler)
	monitor.client = mqtt.NewClient(opts)
	monitor.cameras = conf.Cameras
	monitor.cameraList = make(map[string]int)
	for i, cam := range monitor.cameras {
		monitor.cameraList[cam.Name] = i
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
	token := monitor.client.Publish("monitor/state", 2, true, "online")
	token.Wait()
	topics := make(map[string]byte)
	for camera := range monitor.cameraList {
		topics["cameras/"+camera+"/dump"] = 2
	}
	if token = monitor.client.SubscribeMultiple(topics, monitor.getCVHandler()); token.Wait() && token.Error() != nil {
		log.Println(fmt.Errorf("can't subscribe: %v", token.Error()))
		return
	}
	if token = monitor.client.Subscribe("cameras/+/state", 2, monitor.getOnlineHandler()); token.Wait() && token.Error() != nil {
		log.Println(fmt.Errorf("can't subscribe: %v", token.Error()))
		return
	}
	<-shutdownCh
	monitor.client.Disconnect(60)
}

func (monitor *Monitor) getCVHandler() mqtt.MessageHandler {
	return func(client mqtt.Client, msg mqtt.Message) {
		//log.Println(msg.Payload())
		camName := strings.Split(msg.Topic(), "/")[1]
		go monitor.analyze(camName, msg.Payload())
	}
}

func (monitor *Monitor) getOnlineHandler() mqtt.MessageHandler {
	return func(client mqtt.Client, msg mqtt.Message) {
		camName := strings.Split(msg.Topic(), "/")[1]
		//log.Println(msg.Topic())
		//log.Println(string(msg.Payload()))
		if _, ok := monitor.cameraList[camName]; ok {
			go func() {
				token := monitor.client.Publish("monitor/"+camName+"/state", 2, true, string(msg.Payload()))
				token.Wait()
				token = monitor.client.Publish("cameras/"+camName+"/cmd", 2, false, "dump")
				token.Wait()
			}()
		}
	}
}

var defaultHandler mqtt.MessageHandler = func(client mqtt.Client, msg mqtt.Message) {
	fmt.Printf("unexpected topic: %s\n", msg.Topic())
	fmt.Printf("unexpected message: %s\n", msg.Payload())
}

func (monitor *Monitor) analyze(camName string, data []byte) {
	deg, err := cv.Analyze(data)
	if err != nil {
		log.Println(fmt.Errorf("can't analyze image: %v", err))
		return
	}
	i, ok := monitor.cameraList[camName]
	if !ok {
		log.Println(fmt.Sprintf("unknown camera: %s", camName))
		return
	}
	camData := monitor.cameras[i]
	value, isThresh := computeVal(camData, deg)
	//log.Println(value)
	//log.Println(isThresh)
	strVal := strconv.FormatFloat(value, 'f', -1, 64)
	if isThresh {
		token := monitor.client.Publish("monitor/"+camName+"/threshold", 2, false, strVal)
		token.Wait()
	}
	token := monitor.client.Publish("monitor/"+camName+"/data", 0, false, strVal)
	token.Wait()
	token = monitor.client.Publish("cameras/"+camName+"/cmd", 2, false, "dump")
	token.Wait()
}

func computeVal(camera models.Camera, deg float64) (float64, bool) {
	thresh := false
	var val float64
	valDif := camera.MaxVal - camera.MinVal
	radDif := 0.0
	valRadDif := 0.0
	if camera.Dir >= 0 {
		if camera.ThreshMin > camera.ThreshMax {
			if deg > camera.ThreshMin || deg < camera.ThreshMax {
				thresh = true
			}
		} else {
			if deg > camera.ThreshMin && deg < camera.ThreshMax {
				thresh = true
			}
		}
		if camera.MinRad > camera.MaxRad {
			radDif = 2*math.Pi - camera.MinRad + camera.MaxRad
		} else {
			radDif = camera.MaxRad - camera.MinRad
		}
		if camera.MinRad > deg {
			valRadDif = 2*math.Pi - camera.MinRad + deg
		} else {
			valRadDif = deg - camera.MinRad
		}
	} else {
		if camera.ThreshMin < camera.ThreshMax {
			if deg < camera.ThreshMin || deg > camera.ThreshMax {
				thresh = true
			}
		} else {
			if deg < camera.ThreshMin && deg > camera.ThreshMax {
				thresh = true
			}
		}
		if camera.MinRad < camera.MaxRad {
			radDif = 2*math.Pi - camera.MaxRad + camera.MinRad
		} else {
			radDif = camera.MinRad - camera.MaxRad
		}
		if camera.MinRad < deg {
			valRadDif = 2*math.Pi - deg + camera.MinRad
		} else {
			valRadDif = camera.MinRad - deg
		}
	}
	val = valRadDif*valDif/radDif + camera.MinVal
	return val, thresh
}
