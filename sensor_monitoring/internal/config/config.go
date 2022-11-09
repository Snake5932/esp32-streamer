package config

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"sensor_monitoring/internal/models"
)

type Configuration struct {
	MosquittoAddr string
	MosquittoPort int
	MosquittoUser string
	MosquittoPass string
	Cameras       []models.Camera
}

func SetConfig(path string, config interface{}) error {
	configBytes, err := ioutil.ReadFile(path)
	if err != nil {
		return fmt.Errorf("can't read file: %v", err)
	}
	err = json.Unmarshal(configBytes, config)
	if err != nil {
		return fmt.Errorf("can't unmarshal config: %v", err)
	}
	return nil
}
