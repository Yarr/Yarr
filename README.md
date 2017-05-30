# yarr_fw
This is the first version of the firmware.
## How to generate the bitfile
1. Clean the project
`python clean.py`
2. Generate the Makefile
`hdlmake`
3. IP synthesis
`python ip.py`
4. Generate the bitfile
`make`
5. Flash the RAM program memory
`python flash.py`
