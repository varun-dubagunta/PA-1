/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Varun Dubagunta
	UIN: 434004362
	Date: 28/9
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <sys/wait.h>
using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = -1;
	double t = -1;
	int e = -1;
	bool make_chan = false;
	int m  = MAX_MESSAGE;
	vector<FIFORequestChannel*> channels;
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'm':
				m = atoi(optarg);
				break;
			case 'c':
				make_chan = true;
				break;
		}
	}
	//give arguments for the server
	//server needs path './server' '-m', '<val for -m args>' , 'NULL'
	//fork
	//in child run execvp using the server args
	string m_size = to_string(m);
	

	char* cmd[] = {(char*) "./server",(char*)"-m", (char*)m_size.c_str(), nullptr};

	pid_t pid_0 = fork();
	if(pid_0 == 0){
		execvp(cmd[0], cmd);
		perror("execvp");
		exit(-1);
	}

	FIFORequestChannel ctrl_chan("control", FIFORequestChannel::CLIENT_SIDE);
	channels.push_back(&ctrl_chan);
	if(make_chan){
		//create new channel name and message, and push to vector
		MESSAGE_TYPE nc = NEWCHANNEL_MSG;
		ctrl_chan.cwrite(&nc, sizeof(MESSAGE_TYPE));
		char name[MAX_MESSAGE];
		ctrl_chan.cread(name, MAX_MESSAGE);

		FIFORequestChannel* nchan = new FIFORequestChannel(name, FIFORequestChannel::CLIENT_SIDE);
		channels.push_back(nchan);

	}
	FIFORequestChannel chan = *(channels.back());
	// example data point request
	//single data point, p,t,e != -1
    char buf[MAX_MESSAGE]; // 256
	if(p!=-1 && e!=-1 && t!=-1){ //make sure commands are passed in
		 datamsg x(p, t, e); //change from hardcoding
	
		memcpy(buf, &x, sizeof(datamsg));
		chan.cwrite(buf, sizeof(datamsg)); // question
		double reply;
		chan.cread(&reply, sizeof(double)); //answer
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;

	}
	else if(p!= -1){
		// sending a non-sense message, you need to change this
	  //Else if p!=-1, request 1000 datapoints
	  //loop over first 1000 lines
	  //request ecg 1 and ecg 2
	  //write lines to x1/recieved.csv
	
		ofstream file_out("received/x1.csv");
		for(int i = 0; i<1000;i++){
			double t = i*0.004;
			datamsg ecg_1_req(p, t, 1);
			memcpy(buf,&ecg_1_req,sizeof(datamsg));
			chan.cwrite(buf, sizeof(datamsg));
			double ecg_1;
			chan.cread(&ecg_1, sizeof(double));

			datamsg ecg_2_req(p, t, 2);
			memcpy(buf,&ecg_2_req,sizeof(datamsg));
			chan.cwrite(buf, sizeof(datamsg));
			double ecg_2;
			chan.cread(&ecg_2, sizeof(double));

			file_out << t << ',' << ecg_1 << ',' << ecg_2 << '\n';

		}
		file_out.close();
	}
	if(!filename.empty()){
		filemsg fm(0, 0);
		string fname = filename;
		
		int len = sizeof(filemsg) + (fname.size() + 1);
		char* buf2 = new char[len];
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), fname.c_str());
		chan.cwrite(buf2, len);  // I want the file length;
		int64_t filesize = 0;
		chan.cread(&filesize, sizeof(int64_t));

		char* buf3 = new char[m];//create new buffer of size m

		//loop over elements in the file in filesize/buffer_size segments
		filemsg*  file_req = (filemsg*) buf2;
		int64_t offset = 0;
		ofstream file_out(("received/"+filename).c_str(), ios::binary);
		while(offset < filesize){
			file_req->offset = offset;
			file_req->length = min((int64_t)m, filesize-offset);

			chan.cwrite(buf2, len);
			chan.cread(buf3, file_req->length);
			file_out.write(buf3, file_req->length);
			offset+=file_req->length;
		}
		delete[] buf2;
		delete[] buf3;
		file_out.close();
	}
   MESSAGE_TYPE quit = QUIT_MSG;
   if(make_chan){
	channels.back()->cwrite(&quit, sizeof(MESSAGE_TYPE));
	delete channels.back();
	channels.pop_back();
   }
   chan = *(channels.back()); 
   chan.cwrite(&quit, sizeof(MESSAGE_TYPE));

   wait(nullptr);
	

	
	
}
