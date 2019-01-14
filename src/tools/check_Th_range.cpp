// ./bin/check_Th_range path/to/folder with .txt files/ argv[2]=FileName to save to argv[3]= Vth1 value for the current file (that will also be usd in place of Th value of aprox 800e the new .txt file) 

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <array>

using namespace std;

int main(int argc, char **argv) {

ifstream myfile;
string line, line1;
string file = argv[1], file2 = argv [2], Vth1 = argv [3];
string temp_val;

 myfile.open(file);
   if (!myfile.is_open()) {cout<<"Problem opening the file!"<<endl;}
 ofstream myfile1;
	myfile1.open (file2);

double Th;
int row=0;

//begins reding through text file
 while (getline(myfile, line))
   {    
	if (row>=8) //skips the header infromation in the text file (the first 8 lines)
	{      	
	
	int col=0; 
	unsigned i=0, c=0;
	//looks at number of spaces within a line of text
		while (i <= line.length()-2)
		{	
			c = i;
			temp_val = {};
			while (line[c]!=' ') //increments temp_val string array while no space found
			{ 
			temp_val = temp_val + line [c];
			c++;
			}
			i=c+1;
			Th = stod(temp_val); //once space found convert string to decimal and check if within [795..805] Threshold values
			if ((Th >= 795) && (Th <= 805)) 
				{ cout << "found " << Th << " Threshold in [795..805] at row " << row << " col " << col <<endl;
				if (Vth1[0]=='0') myfile1 << Vth1.substr(1,2) << " "; //if first digit in of Vth1 is zero (i.e. 0160) then discard the leading zero
				else myfile1 << Vth1 << " ";
				}
			else myfile1 << "0 ";


			col++;
		}
	row++;
	myfile1 << '\n';
	}
	else 	{ 
		if ((row==1) && (line[0]=='T')) {myfile1 << "Vth1 Map" << '\n';row++;} //sets the Map title (removes ThresholdMap and writes Vth1 Map)
		else { for (unsigned j=0; j<line.length(); j++)
		myfile1 << line[j];
		myfile1 << '\n';
		row++;}
		}
	
    }
   myfile.close();	
myfile1.close();	
 
   return 0;
}
