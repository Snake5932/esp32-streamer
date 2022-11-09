package cv

import (
	"fmt"
	"github.com/muesli/clusters"
	"github.com/muesli/kmeans"
	"gocv.io/x/gocv"
	"image"
	"log"
	"math"
)

func getDeg(x, y float64) float64 {
	deg := math.Atan2(y, x)
	if deg < 0 {
		deg = 2*math.Pi + deg
	}
	return deg
}

func Analyze(imgData []byte) (float64, error) {
	imgGray, err := gocv.IMDecode(imgData, 0)
	if err != nil {
		return 0, fmt.Errorf("can't decode image: %v", err)
	}

	imgInverse := gocv.NewMat()
	gocv.BitwiseNot(imgGray, &imgInverse)
	imgThresh := gocv.NewMat()
	gocv.Threshold(imgGray, &imgThresh, 150, 255, gocv.ThresholdBinary)
	imgErode := gocv.NewMat()
	gocv.Erode(imgThresh, &imgErode, gocv.NewMat())
	//gocv.Erode(imgErode, &imgErode, gocv.NewMat())
	edges := gocv.NewMat()
	gocv.Canny(imgErode, &edges, 100, 200)
	circles := gocv.NewMat()
	gocv.HoughCirclesWithParams(imgInverse, &circles, gocv.HoughGradient, 1.5, 20, 50, 200, 0, 0)
	center := image.Point{}
	var maxRad float32 = 0.0
	for i := 0; i < circles.Rows(); i++ {
		rad := circles.GetFloatAt(i, 2)
		if rad > maxRad {
			maxRad = rad
			center.X = int(circles.GetFloatAt(i, 0))
			center.Y = int(circles.GetFloatAt(i, 1))
		}
	}
	contours := gocv.FindContours(edges, gocv.RetrievalTree, gocv.ChainApproxNone)
	hands := gocv.NewPointsVector()
	for i := 0; i < contours.Size(); i++ {
		cntr := contours.At(i)
		rect := gocv.BoundingRect(cntr)
		if center.In(rect) {
			hands.Append(cntr)
		}
	}
	minArea := math.MaxFloat64
	handContour := gocv.PointVector{}
	for i := 0; i < hands.Size(); i++ {
		hull := gocv.NewMat()
		gocv.ConvexHull(hands.At(i), &hull, true, true)
		points := make([]image.Point, 0)
		for j := 0; j < hull.Size()[0]; j++ {
			points = append(points, image.Point{
				X: int(hull.GetVeciAt(j, 0)[0]),
				Y: int(hull.GetVeciAt(j, 0)[1]),
			})
		}
		ps := gocv.NewPointVectorFromPoints(points)
		area := gocv.ContourArea(ps)
		if minArea > area {
			minArea = area
			handContour = ps
		}
	}

	var d clusters.Observations
	for i := 0; i < handContour.Size(); i++ {
		d = append(d, clusters.Coordinates{
			float64(handContour.At(i).X),
			float64(-handContour.At(i).Y),
		})
	}
	km := kmeans.New()
	cls, err := km.Partition(d, 2)
	if err != nil {
		log.Fatal(err)
	}
	minDist := math.MaxFloat64
	vecX := 0.0
	vecY := 0.0
	for _, c := range cls {
		dist := math.Sqrt(math.Pow(float64(center.X)-c.Center[0], 2) + math.Pow(float64(-center.Y)-c.Center[1], 2))
		if dist < minDist {
			vecX = float64(center.X) - c.Center[0]
			vecY = float64(-center.Y) - c.Center[1]
			minDist = dist
		}
	}
	return getDeg(vecX, vecY), nil
}
