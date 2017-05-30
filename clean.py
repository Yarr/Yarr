import os
import subprocess

project_path = os.getcwd()
ip_folder = "ip-cores"
ip_list = os.walk(ip_folder)
board_folder = "syn/xpressk7"
file_list = os.listdir(project_path)
board_file_list = os.listdir(project_path + "/" + board_folder)



for dirs,paths,files in ip_list:
	for ip_file in files:
		if(ip_file.endswith(".xci") == False and ip_file.endswith(".prj") == False):
			file_path = os.path.join(project_path + "/" + dirs, ip_file)
			subprocess.call(["rm", file_path])
			#print "deleted: " + file_path
			

for file in file_list:
	if (file.endswith((".xpr",".tcl",".log",".jou")) ) or (file.split('_',1)[0] == 'vivado'):
		file_path= os.path.join(project_path + "/", file)
		subprocess.call(["rm", file_path])
		#print "deleted: " + file_path
	if file.endswith((".cache",".hw",".ip_user_files",".runs")):
		file_path= os.path.join(project_path + "/", file)
		subprocess.call(["rm", "-r",file_path])
		#print "deleted: " + file_path

