import os
import subprocess



script_path = os.getcwd() + "/" + os.path.splitext(__file__)[0] + ".tcl"
script_file = open(script_path, "w+")	

os.chdir("..")
project_path = os.getcwd()

script_file.write(
"######################################################\n" +
"# Generated file to flash the program RAM\n" +
"######################################################\n" +
"\n\n" + 
"#Run " + __file__+ " to generate this file\n\n")

bit_files = []
bit_file = None

for root, dirs, files in os.walk(project_path):
	for file in files:
		if file.endswith(".bit"):
			bit_file =  os.path.join(root, file)
			bit_files.append(bit_file)
			print "Bitfile found : " + bit_file


if len(bit_files) == 0 :
	print("No bit file found !\n")
elif len(bit_files) == 1:
	print("Bit file found : " + bit_files[0])
	ok = input ("Will you flash the RAM with this file [Y/n] ?")
	if ok[0].lowercase() == 'y':
		bit_file = bit_files[0]
	else:
		bit_file = None
else:
	print("Several bit files found: ")
	i = 0
	for bit_file in bit_files:
		print (str(i) + ": " + bit_file)
		i = i + 1
	try: 
		nb = input("Choose a file by typing a number: ")
		int(nb)
	except:
		print("You didn't enter a valid number")
		bit_file = None
	else:
		if nb >= len(bit_files) or nb < 0 :
			print("You didn't enter a valid number")
			bit_file = None 




if (bit_file != None):
	cmds=("open_hw\n" +
	"connect_hw_server\n" +
	"open_hw_target\n" +
	"current_hw_device [lindex [get_hw_devices] 1]\n" +
	"refresh_hw_device -update_hw_probes false [lindex [get_hw_devices] 1]\n" +
	"set_property PROGRAM.FILE {}{}{} [lindex [get_hw_devices] 1]\n" +
	"program_hw_devices [lindex [get_hw_devices] 1]\n")
	print( cmds.format('{',bit_file,'}') + "\n")
	script_file.write( cmds.format('{',bit_file,'}'))
	script_file.close()
	subprocess.call(["vivado", "-mode", "batch","-source", script_path])
else:
	print "No bit file has been written in the RAM !"
