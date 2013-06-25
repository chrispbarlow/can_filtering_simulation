//###########################################################################
//
// FILE:    Example_2833xSci_Echoback.c
//
// TITLE:   DSP2833x Device SCI Echoback.
//
// ASSUMPTIONS:
//
//    This program requires the DSP2833x header files.
//    As supplied, this project is configured for "boot to SARAM" operation.
//
//    Connect the SCI-A port to a PC via a transciever and cable.
//    The PC application 'hypterterminal' can be used to view the data
//    from the SCI and to send information to the SCI.  Characters recieved
//    by the SCI port are sent back to the host.
//
//    As supplied, this project is configured for "boot to SARAM"
//    operation.  The 2833x Boot Mode table is shown below.
//    For information on configuring the boot mode of an eZdsp,
//    please refer to the documentation included with the eZdsp,
//
//       $Boot_Table:
//
//         GPIO87   GPIO86     GPIO85   GPIO84
//          XA15     XA14       XA13     XA12
//           PU       PU         PU       PU
//        ==========================================
//            1        1          1        1    Jump to Flash
//            1        1          1        0    SCI-A boot
//            1        1          0        1    SPI-A boot
//            1        1          0        0    I2C-A boot
//            1        0          1        1    eCAN-A boot
//            1        0          1        0    McBSP-A boot
//            1        0          0        1    Jump to XINTF x16
//            1        0          0        0    Jump to XINTF x32
//            0        1          1        1    Jump to OTP
//            0        1          1        0    Parallel GPIO I/O boot
//            0        1          0        1    Parallel XINTF boot
//            0        1          0        0    Jump to SARAM	    <- "boot to SARAM"
//            0        0          1        1    Branch to check boot mode
//            0        0          1        0    Boot to flash, bypass ADC cal
//            0        0          0        1    Boot to SARAM, bypass ADC cal
//            0        0          0        0    Boot to SCI-A, bypass ADC cal
//                                              Boot_Table_End$
//
// DESCRIPTION:
//
//
//    This test recieves and echo-backs data through the SCI-A port.
//
//    1) Configure hyperterminal:
//       Use the included hyperterminal configuration file SCI_96.ht.
//       To load this configuration in hyperterminal: file->open
//       and then select the SCI_96.ht file.
//    2) Check the COM port.
//       The configuration file is currently setup for COM1.
//       If this is not correct, disconnect Call->Disconnect
//       Open the File-Properties dialog and select the correct COM port.
//    3) Connect hyperterminal Call->Call
//       and then start the 2833x SCI echoback program execution.
//    4) The program will print out a greeting and then ask you to
//       enter a character which it will echo back to hyperterminal.
//
//    As is, the program configures SCI-A for 9600 baud with
//    SYSCLKOUT = 150MHz and LSPCLK = 37.5 MHz
//    SYSCLKOUT = 100MHz and LSPCLK = 25.0 Mhz
//
//
//    Watch Variables:
//       LoopCount for the number of characters sent
//       ErrorCount
//
//
//###########################################################################
// $TI Release: 2833x/2823x Header Files and Peripheral Examples V133 $
// $Release Date: June 8, 2012 $
//###########################################################################


#include "SCI.h"


// Test 1,SCIA  DLB, 8-bit word, baud rate 0x000F, default, 1 STOP bit, no parity
void scia_init()
{
    // Note: Clocks were turned on to the SCIA peripheral
    // in the InitSysCtrl() function

 	SciaRegs.SCICCR.all =0x0007;   // 1 stop bit,  No loopback
                                   // No parity,8 char bits,
                                   // async mode, idle-line protocol
	SciaRegs.SCICTL1.all =0x0003;  // enable TX, RX, internal SCICLK,
                                   // Disable RX ERR, SLEEP, TXWAKE
	SciaRegs.SCICTL2.all =0x0003;
	SciaRegs.SCICTL2.bit.TXINTENA =0;
	SciaRegs.SCICTL2.bit.RXBKINTENA =0;
	SciaRegs.SCIFFRX.bit.RXFFIENA = 1;	//Use receive FIFO

	#if (CPU_FRQ_150MHZ)
	      SciaRegs.SCIHBAUD    =0x0001;  // 9600 baud @LSPCLK = 37.5MHz.
	      SciaRegs.SCILBAUD    =0x00E7;
	#endif
	#if (CPU_FRQ_100MHZ)
      SciaRegs.SCIHBAUD    =0x0001;  // 9600 baud @LSPCLK = 20MHz.
      SciaRegs.SCILBAUD    =0x0044;
	#endif
	SciaRegs.SCICTL1.all =0x0023;  // Relinquish SCI from Reset
}

/*TODO: make these externally accessible */
// Transmit a character from the SCI
void scia_xmit(int a)
{
    while (SciaRegs.SCIFFTX.bit.TXFFST != 0) {}
    SciaRegs.SCITXBUF=a;

}

void scia_msg(char * msg)
{
    int i;
    i = 0;
    while(msg[i] != '\0')
    {
        scia_xmit(msg[i]);
        i++;
    }
}

// Initalize the SCI FIFO
void scia_fifo_init()
{
    SciaRegs.SCIFFTX.all=0xE040;
    SciaRegs.SCIFFRX.all=0x204f;
    SciaRegs.SCIFFCT.all=0x0;

}






//===========================================================================
// No more.
//===========================================================================

