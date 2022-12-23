package main

import (
	"fmt"
	"io"
    "net"
	"net/http"
	"github.com/gorilla/websocket"
	"log"
	"math/rand"
	"time"
    //"encoding/binary"
)

func getRoot(w http.ResponseWriter, r *http.Request) {
	fmt.Printf("got / request\n")
	io.WriteString(w, "This is my website\n")
}

func getHello(w http.ResponseWriter, r *http.Request) {
	fmt.Printf("got /hello request\n")
	io.WriteString(w, "Hello, HTTP!\n")
}

var upgrader = websocket.Upgrader{}

var websocketConn *websocket.Conn

func socketHandler(w http.ResponseWriter, r *http.Request) {
    var err error

    if websocketConn != nil {
        websocketConn.Close()
    }

	websocketConn, err = upgrader.Upgrade(w, r, nil)

	if err != nil {
		log.Print("upgrade: ", err)
	}

	// defer conn.Close()

    /*
    var n_cells uint32 = 500
    n_cells_arr := make([]byte, 4)
    binary.LittleEndian.PutUint32(n_cells_arr, n_cells)

    err = websocketConn.WriteMessage(websocket.BinaryMessage, n_cells_arr)
    if err != nil {
        log.Println("Error during message writing: ", err)
        return
    }
    */

    return

	for {
		/*
		_, message, err := conn.ReadMessage()
		if err != nil {
			log.Println("Error during message reading: ", err)
			break
		}

		log.Printf("Received msg: ", message)
		*/

		data_arr := make([]byte, 262144)
		rand.Read(data_arr)

		err = websocketConn.WriteMessage(websocket.BinaryMessage, data_arr)
		if err != nil {
			log.Println("Error during message reading: ", err)
			break
		}
		log.Println("Sent message")


		time.Sleep(1000 * time.Millisecond)
	}
}

func handleUdp(conn *net.UDPConn) {
    buffer := make([]byte, 32768)

    for {
        n, _, err := conn.ReadFromUDP(buffer)
        if err != nil {
            fmt.Println("UDP error: ", err)
            return
        }
        fmt.Printf("Read datagram len %d\n", n)

		err = websocketConn.WriteMessage(websocket.BinaryMessage, buffer[:n])
		if err != nil {
			log.Println("Error during message writing: ", err)
			break
		}

    }
}

func Server() {
	fileServer := http.FileServer(http.Dir("./client/"))
    // Connection used for the local processor
    udpConn, err := net.ListenUDP("udp", &net.UDPAddr{
        Port: 18273,
        IP:   net.ParseIP("0.0.0.0"),
    })
    if err != nil {
        panic(err)
    }

    defer udpConn.Close()
    fmt.Printf("Server listening on UDP: %s\n", udpConn.LocalAddr().String())

    go handleUdp(udpConn)


	http.HandleFunc("/hello", func(w http.ResponseWriter, r *http.Request) {
		io.WriteString(w, "Hello there!\n")
	})

	http.HandleFunc("/websocket", socketHandler)

	http.Handle("/", fileServer)

	fmt.Println("Running HTTP server on port 3333")
	log.Fatal(http.ListenAndServe(":3333", nil))
}

func main() {
	Server()
}
