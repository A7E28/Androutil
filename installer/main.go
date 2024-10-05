package main

import (
	"fmt"
	"log"
	"os"

	"github.com/A7E28/AdbInstaller/modules"
)

func main() {
	const (
		url  = "https://dl.google.com/android/repository/platform-tools-latest-windows.zip"
		name = "platform-tools.zip"
	)

	if modules.CheckADB() {
		fmt.Println("ADB is already installed.")
		modules.WaitForInput()
		os.Exit(0)
	}

	if err := modules.Download(url, name); err != nil {
		log.Fatalf("Error while downloading: %v", err)
		return
	}
	defer os.Remove(name)

	if err := modules.Unzip(name); err != nil {
		log.Fatalf("Error while extracting: %v", err)
		return
	}

	if err := modules.Handlenv(); err != nil {
		log.Fatalf("Error while setting env: %v", err)
		return
	}

	modules.WaitForInput()

}
