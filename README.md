# idprom-repair
SunBlade100 IDPROM randomizer (Linux)

# Sun EEPROM (nvram): Alternative method to restore/change Ethernet address and Host ID

You probably know that when you type banner at the PROM ok prompt, you see the system’s serial number, hostid and MAC address:
```
ok banner 
Sun Ultra 60 UPA/PCI (UltraSPARC-II 296MHz), Keyboard Present 
OpenBoot 3.11, 512 MB memory installed, 
Serial #10573911. Ethernet address 8:0:20:a1:58:57, Host ID: 80a15857 
```
Note that the serial number is the last 3 bytes of the HostID (a15857), converted to decimal.
This content is stored in a NVRAM chip (EEPROM) in the Sun workstation. The
contents of the NVRAM chip can become corrupted for a variety of reasons, most commonly, failure of the embedded battery.
The battery embedded in the NVRAM chip keeps the clock running when the machine is off and also maintains important system configuration information. 
This happens when turn on your Sun workstation and get output which looks something like:
```
Invalid format type in NVRAM
The IDPROM contents are invalid
ERROR: missing or invalid ID prom
Requesting Internet address for 0:0:0:0:0:0
```

Usually, when the NVRAM gets corrupted in this way, this is a symptom that the battery embedded in the
NVRAM chip has run out and you need to replace the chip. 

But you should try reprogramming the NVRAM chip with a hostid and ethernet address. 

This is what we will talk about.

Note: be sure to backup the original hostid and ethernet address values before modification. 
If your Sun EEPROM nvram is invalid, try to find original values (look stikers in your workstation or over nvram) and use this approarch.

This method is applicable to some Sun series, like Sun Blade100, Sun Blade150 (tested in a Sun Blade 100). 
Pay attention to small differences in the screen display and this guide, like the EEPROM path and virtual address that may be different, don't completely copy.


I will use my Blade 100 as an example, the original Host ID: 83188869, Ethernet address:0:3:ba:18:88:69; will be restored. 

 
 1. Enter into OpenBoot prompt
 
 `Stop+A
 ok`
 
 2. Browse the device tree to find the full path to the EEPROM
 
 `ok show-devs` 
 
 3. Change to EEPROM full path:
 
 `ok cd /pci@1f,0/ebus@c/eeprom@1,0`
 
 Note that the EEPROM path to the EEPROM version of the output of different name, please refer to the screen display shall prevail. 
 
 4. List the name and values os the node's properties to find the value of device address
 
 `ok .properties`
 
 5. Use the addres value, in my case fff4a000
 
 `ok fff4a000 >physical`
 
 6. Displays the content of data stack automatically before each ok prompt
 
 `ok showstack`
 
 7. Maps a region of physical device address and return the allocated virtual address
 
 `ok 2000 memmap`
 
 8. Offset
 
 `ok 1fd0 +`
 
 9. Display 30 bytes of memory starting at current addr
 
 `ok 30 dump`
 
 Look at the output of the drawings, starting from fff41fd8 you mean as follows: 
```
 Byte address map
   0 fff41fd8:   Always 01 - Format/version number.
   1 fff41fd9:   1st hostid[0] byte / machine type [ 83 in Sun Blade, 80 in Sun Ultra,etc.]
 2-7 fff41fda~f: Ethernet (MAC) address 
 8-b fff41fe0~3: The date of manufacture, ofter all zeroes, is not a real date
   c fff41fe4:   2nd hostid[1] byte
   d fff41fe5    3rd hostid[2] byte
   e fff41fe6:   4th hostid[3] byte 
   f fff41fe7:   IDPROM checksum - bitwise xor of bytes 0-e (ie. xor of values from fff41fd8 to fff41fe6)
```
To restore the EEPROM values, we will write all these bytes and calculate the checksum. Remember in this example we will use Host ID: 83188869, Ethernet address:0:3:ba:18:88:69.

10. Write version (01) and hostid[0] byte/machine type (83)

```
ok 01 fff41fd8 c!
ok 83 fff41fd9 c!
```

11. Write ethernet address:0:3:ba:18:88:69

```
ok 00 fff41fda c!
ok 03 fff41fdb c!
ok ba fff41fdc c!
ok 18 fff41fdd c!
ok 88 fff41fde c!
ok 69 fff41fdf c!
```

12. Write zeros (if not already) for the production date

```
ok 00 fff41fe0 c!
ok 00 fff41fe1 c!
ok 00 fff41fe2 c!
ok 00 fff41fe3 c!
```

13. Write hostid[1~3] bytes: 188869

```
ok 18 fff41fe4 c!
ok 88 fff41fe5 c!
ok 69 fff41fe6 c!
``` 

14. Calculate XOR-checksum for the values starting from fff41fd8, until fff41fe6

```
ok 01 83 xor
82 ok 00 xor
83 ok 03 xor
81 ok ba xor
3b ok 18 xor
23 ok 88 xor
ab ok 69 xor
c2 ok 18 xor
da ok 88 xor
52 ok 69 xor
3b ok
```

15. Write calculared checksum

```
ok 3b fff41fe7 c!
```

16 Display Banner and reboot

```
ok banner
ok reset-all
```
