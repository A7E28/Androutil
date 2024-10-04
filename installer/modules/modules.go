package modules

import (
	"archive/zip"
	_ "embed"
	"fmt"
	"io"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
)

//go:embed setEnv.ps1
var setEnv string

func CheckADB() bool {
	fmt.Println("Checking ADB....")
	output, err := exec.Command("adb", "version").Output()
	if err != nil {
		fmt.Println("ADB not found:", err)
		return false
	}
	fmt.Println("ADB version found:", string(output))
	return true
}

func Download(url, filename string) error {
	fmt.Println("Downloading platform-tools...")
	response, err := http.Get(url)
	if err != nil {
		return err
	}
	defer response.Body.Close()

	if response.StatusCode != http.StatusOK {
		return fmt.Errorf("failed to download: %s", response.Status)
	}

	file, err := os.Create(filename)
	if err != nil {
		return err
	}
	defer file.Close()

	_, err = io.Copy(file, response.Body)
	return err
}

func Unzip(fileName string) error {
	fmt.Println("Extracting", fileName)

	r, err := zip.OpenReader(fileName)
	if err != nil {
		return err
	}
	defer r.Close()

	for _, f := range r.File {
		rc, err := f.Open()
		if err != nil {
			return err
		}
		defer rc.Close()

		fpath := filepath.Join("C:", f.Name)
		if f.FileInfo().IsDir() {
			if err := os.MkdirAll(fpath, 0755); err != nil {
				return err
			}
		} else {
			if err := os.MkdirAll(filepath.Dir(fpath), 0755); err != nil {
				return err
			}
			outFile, err := os.Create(fpath)
			if err != nil {
				return err
			}
			defer outFile.Close()

			if _, err := io.Copy(outFile, rc); err != nil {
				return err
			}
		}
	}
	return nil
}

func Handlenv() error {
	cmd := exec.Command("powershell", "-ExecutionPolicy", "Bypass", "-Command", setEnv)
	output, err := cmd.CombinedOutput()
	if err != nil {
		return fmt.Errorf("Error executing PowerShell script: %v", err)
	}
	fmt.Printf("Output: %s\n", output)
	return nil
}

func WaitForInput() {
	fmt.Println("Press Enter to exit...")
	fmt.Scanln()
}
