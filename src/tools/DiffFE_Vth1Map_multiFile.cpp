// makes a .txt DiffFE map with Vth1 from multiple Threshold scans (each scan is generated with a specific Vth1 value). Optionally, it can apply the good pixel mask
// ./bin/DiffFe_Vth1Map_multiFile (path/to/folder with .txt files/) (nameOfFile to save to) [mask] 

#include <iostream>
#include <string.h>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <array>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

int goodDiff(int row, int col) {
	int good = 10;
	
	std::array<std::array<unsigned, 8>, 8> mask;
	mask[0] = {{0, 1, 1, 1, 0, 0, 0, 0}};
	mask[1] = {{0, 0, 1, 1, 1, 0, 0, 0}};
	mask[2] = {{0, 1, 0, 1, 1, 0, 0, 0}};
	mask[3] = {{0, 1, 0, 1, 1, 0, 0, 0}};
	mask[4] = {{0, 0, 1, 1, 0, 0, 0, 0}};
	mask[5] = {{0, 0, 1, 1, 0, 0, 0, 0}};
	mask[6] = {{0, 0, 1, 0, 0, 0, 0, 0}};
	mask[7] = {{0, 0, 1, 1, 0, 0, 0, 0}};
	
	
		if (mask[row%8][col%8] == 1) good = 1;
		else good = 0;
	
	
	return good;
}

int main(int argc, char **argv) {

ifstream myfile, myfile2;
string line, line1;
string file1 = argv[2];
string temp_val;
string goodPix;

if (argc > 3) 
  { goodPix = argv[3];}

vector<vector<int> > pixMap1(192);
//Create 2D vector with all zeros
for (unsigned i = 0; i < pixMap1.size(); i++) {
	pixMap1[i] = vector<int>(136);
        for (unsigned j = 0; j < pixMap1[i].size(); j++)  pixMap1[i][j] = 0;
       
    }

if (argc < 2) {
		std::cout << "No directory given!" << std::endl;
		return -1;
	}

	std::string dir, filepath, file_name, chipnum;
	DIR *dp;
	struct dirent *dirp;
	struct stat filestat;

	dp = opendir(argv[1]);	//open directory
	if (dp==NULL) {	//if directory doesn't exist
		std::cout << "Directory not found. " << std::endl;
		return -1;
	}

	dir = argv[1];


int step = 1;
	while ((dirp = readdir(dp))) { //pointer to structure representing directory entry at current position in directory stream, and positions directory stream at the 			next entry. Returns a null pointer at the end of the directory stream.

		file_name = dirp->d_name;
		filepath = dir + "/" + dirp->d_name;
		const char *file_path = filepath.c_str();		

		if (stat(filepath.c_str(), &filestat)) continue; //skip if file is invalid
		if (S_ISDIR(filestat.st_mode)) continue; //skip if file is a directory
		if (strstr(file_path,".pdf")) continue;

				

		
		if ( strstr( file_path, ".txt") != NULL) { //if filename contains string declared in argument.

		std::cout << "Opening file: " << filepath.c_str() << std::endl;
		
		std::string filename = filepath.c_str();
             std::cout << filename.substr(filename.length()-16,16) << std::endl;
		//std::fstream cfgfile(filepath.c_str(), std::ios::in);
						

	// Extract values from txt file
	myfile.open(filepath.c_str()); //.substr(filename.length()-15,16)
	std::cout << "File opened." << std::endl;
	if (!myfile.is_open()) {cout <<"Problem opening the file!"<< std::endl;}
	std::cout << step << std::endl;
	int row=0;
		int step2=0;
		while (getline(myfile, line))
		   {    
			
			if (row>=8)
			{      	
	
			int col=0; 
			unsigned i=0, c=0;
			//looks at number of spaces
				while (i <= line.length()-2)
				{	
					c = i;
					temp_val = {};
					while (line[c]!=' ')
					{ 
					temp_val = temp_val + line [c];
					c++;
					}
					i=c+1;
					//if current value != 0 and the previous one was zero or empty
					if ((stoi(temp_val)!=0) && (pixMap1[row-8][col]==0)) 
                                           {
                                            if ((argc > 3) && (goodPix == "mask")) //applies good pixel mask 
                                              {if (goodDiff(row,col)==1) pixMap1[row-8][col]=stoi(temp_val);
                                          
					       else pixMap1[row-8][col]=0;}
                                            else pixMap1[row-8][col]=stoi(temp_val);}
					col++;
				}
			row++;
			//myfile1 << '\n';
			}
			else 	{ 
			/*	if ((row==1) && (line[0]=='T')) {myfile1 << "Vth1 Map" << '\n';row++;} //sets the Map title (removes "ThresholdMap" and writes "Vth1 Map")
				else { for (unsigned j=0; j<line.length(); j++) myfile1 << line[j];
				myfile1 << '\n';
				row++;} */
				row++;} 
			std::cout << "step 2 " << step2 << " " << line << std::endl;
			step2++;
		    }
   myfile.close();			 

	} 

	step++;
}

//print vector values to screen
 for (unsigned i = 0; i < pixMap1.size(); i++) {
        for (unsigned j = 0; j < pixMap1[i].size(); j++) cout << pixMap1[i][j] << " ";
		cout << '\n';	
       
    }
//Create Vth1Map.txt
ofstream myfile1;
myfile1.open (file1);

for (unsigned i = 0; i < pixMap1.size(); i++) {
        for (unsigned j = 0; j < pixMap1[i].size(); j++) myfile1 << to_string(pixMap1[i][j]) << " ";
		myfile1 << '\n';	
       
    }

 myfile1.close();

	return 0;
} 
