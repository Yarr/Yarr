import os
import subprocess

project_path = os.getcwd()
ip_folder = "ip-cores"
board_folder = "syn/xpressk7"
file_list = os.listdir(project_path)
board_file_list = os.listdir(project_path + "/" + board_folder)

# TDO create the file if doesn't exist
script_filename = "ip.tcl"
script_file = open(script_filename, 'w+')


for roots,dirs,files in os.walk(ip_folder):
	for ip_file in files:
		if(ip_file.endswith(".xci") == False and ip_file.endswith(".prj") == False):
			file_path = os.path.join(project_path + "/" + roots, ip_file)
			subprocess.call(["rm", file_path])
			

script_file.write(
"######################################################\n" +
"# Generated file to add the Vivado IP into the project\n" +
"######################################################\n" +
"\n\n" + 
"#Run ip.py to generate this file\n\n")


for file in file_list:
	if (file.endswith(".xpr")):
		script_file.write("open_project " + os.path.join(project_path + "/", file) + "\n")
for file in board_file_list:
	if (file.endswith(".xdc")):
		script_file.write( "add_files " + os.path.join(project_path + "/" + board_folder + "/", file)+ "\n")

# DDR3 desactivated
script_file.write( "set_property is_enabled false [get_files  *xpressk7-ddr3.xdc] \n")


ips = []
cmds = [
"add_files {2}\n" + 
"generate_target all [get_files {2}]\n" +
"create_ip_run [get_files -of_objects [get_fileset sources_1] {2}]\n" +
"update_compile_order -fileset sources_1\n" +
"launch_run -jobs 4 {0}_synth_1\n",

"wait_on_run {0}_synth_1\n" +
"export_ip_user_files -of_objects [get_files {0}] -no_script -force\n"
]

for roots,dirs,files in os.walk(ip_folder):
	for ip_file in files:
		if(ip_file.endswith(".xci")):
			ip_name = os.path.splitext(ip_file)[0]
			file_path = os.path.join(project_path + "/" + roots, ip_file)
			ips.append((ip_name,ip_file,file_path))

for cmd in cmds:
	for ip in ips:
		script_file.write(cmd.format(ip[0],ip[1],ip[2]))	
		script_file.write("\n")	
	script_file.write("\n")	


script_file.close()
subprocess.call(["vivado", "-mode", "batch","-source", script_filename])
