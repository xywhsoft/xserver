// go_router — Go 1.22 stdlib ServeMux（原生 {id} 通配）基准路由
// 用法：go_router.exe -routes <routes.txt> -port 9092
package main

import (
	"flag"
	"fmt"
	"net/http"
	"os"
	"strings"
)

var respHead = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nContent-Type: text/plain\r\n\r\n"

func main() {
	routes := flag.String("routes", "", "routes file")
	port := flag.Int("port", 9092, "listen port")
	flag.Parse()

	data, err := os.ReadFile(*routes)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
	mux := http.NewServeMux()
	for _, line := range strings.Split(string(data), "\n") {
		line = strings.TrimSpace(line)
		if line == "" {
			continue
		}
		parts := strings.SplitN(line, " ", 2)
		mux.HandleFunc(parts[1], func(w http.ResponseWriter, r *http.Request) {
			w.Header().Set("Content-Type", "text/plain")
			w.Write([]byte("ok"))
		})
	}
	_ = respHead
	fmt.Println("go ready", *port, len(strings.Split(string(data), "\n")))
	http.ListenAndServe(fmt.Sprintf("127.0.0.1:%d", *port), mux)
}
