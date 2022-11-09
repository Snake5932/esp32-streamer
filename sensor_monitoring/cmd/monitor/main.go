package main

import (
	"flag"
	"sensor_monitoring/internal/monitor"
)

func main() {
	flag.Parse()
	module := monitor.Init(flag.Arg(0))
	module.Run()
}
