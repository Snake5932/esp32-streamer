package models

type Camera struct {
	Name      string
	MinVal    float64 //по шкале
	MaxVal    float64
	MinRad    float64 // в соответствии со шкалой
	MaxRad    float64
	ThreshMin float64 // в соответствии со шкалой
	ThreshMax float64
	Dir       int // направление шкалы: < 0 по часовой, >= 0 против
}
