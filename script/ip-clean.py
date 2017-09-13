#################################
# Author: Arnaud Sautaux
# Email: asautaux at lbl.gov
# Project: Yarr
# Description: Cleaning the ip folder
# Comment: 
################################


import os
import subprocess

#project_path = os.getcwd() + "/../"
os.chdir("..")
project_path =  os.getcwd()
ip_folder = "ip-cores/kintex7"
file_list = os.listdir(project_path)

counter = 0

for roots,dirs,files in os.walk(ip_folder):
	for ip_file in files:
		if(ip_file.endswith(".xci") == False and ip_file.endswith(".prj") == False and ip_file.endswith(".gitignore") == False and ip_file.endswith(".py") == False):
			file_path = os.path.join(project_path + "/" + roots, ip_file)
			subprocess.call(["rm", file_path])
			counter = counter + 1

print "{} files deleted".format(counter)
