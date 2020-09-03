package main

import "time"

// msToTime converts milliseconds since epoch to time.Time
func msToTime(t int64) time.Time {
	return time.Unix(t/int64(1000), (t%int64(1000))*int64(1000000))
}
