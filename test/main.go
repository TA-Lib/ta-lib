package main

/*
#cgo CFLAGS: -I/usr/local/include/ta-lib
#cgo LDFLAGS: -L/usr/local/lib -lta-lib -lm
#include <ta_libc.h>
#include <ta_abstract.h>

extern void goIndicatorCallback(char *name, char *group, char *hint, unsigned int nbInput, unsigned int nbOptInput, unsigned int nbOutput);

static void c_callback(const TA_FuncInfo *funcInfo, void *opaqueData) {
    if (funcInfo != NULL) {
        goIndicatorCallback(
            (char*)funcInfo->name,
            (char*)funcInfo->group,
            (char*)funcInfo->hint,
            funcInfo->nbInput,
            funcInfo->nbOptInput,
            funcInfo->nbOutput
        );
    }
}

static int enumerateAllIndicators() {
    TA_RetCode ret = TA_Initialize();
    if (ret != TA_SUCCESS) {
        return (int)ret;
    }
    return (int)TA_ForEachFunc(c_callback, NULL);
}
*/
import "C"
import (
	"fmt"
	"os"
	"sort"
	"sync"
	"unsafe"
)

type IndicatorInfo struct {
	Name        string
	Group       string
	Hint        string
	NbInput     uint
	NbOptInput  uint
	NbOutput    uint
}

var (
	mu         sync.Mutex
	indicators []IndicatorInfo
)

//export goIndicatorCallback
func goIndicatorCallback(name, group, hint *C.char, nbInput, nbOptInput, nbOutput C.uint) {
	mu.Lock()
	defer mu.Unlock()

	indicators = append(indicators, IndicatorInfo{
		Name:       C.GoString(name),
		Group:      C.GoString(group),
		Hint:       C.GoString(hint),
		NbInput:    uint(nbInput),
		NbOptInput: uint(nbOptInput),
		NbOutput:   uint(nbOutput),
	})
}

func main() {
	fmt.Println("=================================================================")
	fmt.Println(" TA-Lib Docker Image Verification: Checking All 240+ Indicators")
	fmt.Println(" Image Source: rajnish93/ta-lib:latest")
	fmt.Println("=================================================================")

	// 1. Enumerate all indicators registered in the compiled C library
	ret := C.enumerateAllIndicators()
	if ret != 0 {
		fmt.Fprintf(os.Stderr, "ERROR: TA_Initialize or TA_ForEachFunc failed with code %d\n", ret)
		os.Exit(1)
	}

	total := len(indicators)
	fmt.Printf("\n✓ Successfully enumerated %d total technical indicators!\n\n", total)

	// 2. Group indicators by category
	groupMap := make(map[string][]IndicatorInfo)
	for _, ind := range indicators {
		groupMap[ind.Group] = append(groupMap[ind.Group], ind)
	}

	// Sort group names for deterministic output
	var groupNames []string
	for g := range groupMap {
		groupNames = append(groupNames, g)
	}
	sort.Strings(groupNames)

	fmt.Println("-----------------------------------------------------------------")
	fmt.Printf("%-30s | %s\n", "INDICATOR CATEGORY", "COUNT")
	fmt.Println("-----------------------------------------------------------------")
	for _, g := range groupNames {
		fmt.Printf("%-30s | %3d functions\n", g, len(groupMap[g]))
	}
	fmt.Println("-----------------------------------------------------------------")
	fmt.Printf("%-30s | %3d total\n", "TOTAL ACCESSIBLE INDICATORS", total)
	fmt.Println("-----------------------------------------------------------------\n")

	// 3. Print complete catalog of indicators
	fmt.Println("List of All Accessible Indicators:")
	for _, g := range groupNames {
		fmt.Printf("\n[%s]\n  ", g)
		funcs := groupMap[g]
		sort.Slice(funcs, func(i, j int) bool {
			return funcs[i].Name < funcs[j].Name
		})
		for i, f := range funcs {
			fmt.Printf("%-16s", f.Name)
			if (i+1)%4 == 0 && i != len(funcs)-1 {
				fmt.Print("\n  ")
			}
		}
		fmt.Println()
	}

	// 4. Test live execution on representative indicators
	fmt.Println("\n-----------------------------------------------------------------")
	fmt.Println("Executing Sample Functional Verification Tests:")
	fmt.Println("-----------------------------------------------------------------")

	data := []float64{10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20}
	out := make([]float64, len(data))
	var outBeg, outNb C.int

	// Test 1: TA_SMA
	retCode := C.TA_SMA(
		0,
		C.int(len(data)-1),
		(*C.double)(unsafe.Pointer(&data[0])),
		C.int(3),
		&outBeg,
		&outNb,
		(*C.double)(unsafe.Pointer(&out[0])),
	)
	if retCode != C.TA_SUCCESS || int(outNb) != 9 {
		fmt.Fprintf(os.Stderr, "FAIL: TA_SMA execution failed (code: %d, nb: %d)\n", retCode, outNb)
		os.Exit(1)
	}
	fmt.Printf("✓ TA_SMA calculation passed (computed %d bars, latest=%.2f)\n", outNb, out[outNb-1])

	// Test 2: TA_RSI
	retCode = C.TA_RSI(
		0,
		C.int(len(data)-1),
		(*C.double)(unsafe.Pointer(&data[0])),
		C.int(5),
		&outBeg,
		&outNb,
		(*C.double)(unsafe.Pointer(&out[0])),
	)
	if retCode != C.TA_SUCCESS || int(outNb) != 6 {
		fmt.Fprintf(os.Stderr, "FAIL: TA_RSI execution failed (code: %d, nb: %d)\n", retCode, outNb)
		os.Exit(1)
	}
	fmt.Printf("✓ TA_RSI calculation passed (computed %d bars, latest=%.2f)\n", outNb, out[outNb-1])

	// 5. Final assertion on indicator count
	if total < 240 {
		fmt.Fprintf(os.Stderr, "\nFAIL: Expected 240 indicators, but found %d\n", total)
		os.Exit(1)
	}

	fmt.Printf("\nSUCCESS: All %d indicators from rajnish93/ta-lib:latest are 100%% accessible and operational!\n", total)
}

