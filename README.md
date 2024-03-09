# Tcp_Server_Ethernet
This is a tcp server build by esp32s2 and connected via ethernet and its function is echo what you enter.  
This program integrates the official ethernet and tcp server basic code.  
## Usage
This program is designed to use DM9051 module to communicate with RJ-45 interface. The default pin relationship is shown below.  
<kbd>
SPI SCLK -> GPIO 14  
SPI MOSI -> GPIO 13  
SPI MISO -> GPIO 12  
SPI CS0  -> GPIO 15  
INT      -> GPIO 4  
</kbd>  
The maximal value of SPI clock speed is 20MHz.  
If you want to change to other module, you can reference to official code.
