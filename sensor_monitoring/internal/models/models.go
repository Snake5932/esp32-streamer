package models

type Camera struct {
	Name      string
	MinVal    int //по шкале
	MaxVal    int
	MinRad    float64 //в соответствии со шкалой
	MaxRad    float64
	ThreshMin float64 // от меньшего к большему
	ThreshMax float64
}
