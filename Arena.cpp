#include <iostream>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>
#include <ext/stdio_filebuf.h>
#include <poll.h>
#include <array>
#include <queue>
#include <random>
#include <chrono>
#include <omp.h>
#include <limits>
#include <algorithm>
using namespace std;
using namespace std::chrono;

constexpr double asymetry_limit{0.01};//Positive number, represents a fraction of the board in the Voronoi evaluation
constexpr bool Debug_AI{true},Timeout{true};
constexpr int PIPE_READ{0},PIPE_WRITE{1};
constexpr int H{20},W{30},S{H*W};//Dimensions of the tron board
constexpr int N{2};//Number of players, 1v1
constexpr int int_max{std::numeric_limits<int>::max()};

enum direction{LEFT=0,DOWN=1,RIGHT=2,UP=3};
constexpr array<direction,4> Directions{LEFT,RIGHT,DOWN,UP};

struct vec{
	int x,y;
	inline bool operator==(const vec &a)const noexcept{
		return x==a.x && y==a.y;
	}
	inline void operator+=(const vec &a)noexcept{
		x+=a.x;
		y+=a.y;
	}
	inline int idx()const noexcept{
		return y*W+x;
	}
	inline bool valid()const noexcept{
		return x>=0 && y>=0 && x<W && y<H;
	}
};

constexpr array<vec,4> Directions_Vec{vec{-1,0},vec{0,1},vec{1,0},vec{0,-1}};

ostream& operator<<(ostream &os,const vec &r){
	os << r.x << " " << r.y;
	return os;
}

inline string EmptyPipe(const int f){
	char out[5000];
	fill(out,out+5000,0);
	int ptr{0};
	pollfd outpoll{f,POLLIN};
	while(poll(&outpoll,1,0)){
		ssize_t bytes_read{read(f,out+(ptr++),1)};
		if(bytes_read<1){
			throw(0);
		}
	}
	return string(out);
}

struct AI{
	int id,pid,outPipe,errPipe;
	string name;
	ostream *in;
	__gnu_cxx::stdio_filebuf<char> inBuff;
	vec r,start;
	inline bool has_move(const array<int,S> &grid)const noexcept{
		return (r.y>0 && grid[r.idx()-W]==-1) || (r.y<H-1 && grid[r.idx()+W]==-1) || (r.x<W-1 && grid[r.idx()+1]==-1) || (r.x>0 && grid[r.idx()-1]==-1);
	}
	inline void stop(){
		kill(pid,SIGTERM);
		int status;
		waitpid(pid,&status,0);//It is necessary to read the exit code for the process to stop
		//Should add a step with SIGKILL
	}
	inline bool alive()const{
		return kill(pid,0)!=-1;//Check if process is still running
	}
	inline ~AI(){
		stop();
		close(errPipe);
		close(outPipe);
		delete in;
	}
};

void StartProcess(AI &Bot){
	int StdinPipe[2];
  	int StdoutPipe[2];
  	int StderrPipe[2];
  	if(pipe(StdinPipe)<0){
	    perror("allocating pipe for child input redirect");
  	}
  	if(pipe(StdoutPipe)<0){
	    close(StdinPipe[PIPE_READ]);
	    close(StdinPipe[PIPE_WRITE]);
	    perror("allocating pipe for child output redirect");
  	}
  	if(pipe(StderrPipe)<0){
  		close(StderrPipe[PIPE_READ]);
  		close(StderrPipe[PIPE_WRITE]);
  		perror("allocating pipe for child stderr redirect failed");
  	}
  	int nchild{fork()};
  	if(nchild==0){//Child process
	    if(dup2(StdinPipe[PIPE_READ],STDIN_FILENO)==-1){// redirect stdin
			perror("redirecting stdin");
			return;
	    }
	    if(dup2(StdoutPipe[PIPE_WRITE],STDOUT_FILENO)==-1){// redirect stdout
			perror("redirecting stdout");
			return;
	    }
	    if(dup2(StderrPipe[PIPE_WRITE],STDERR_FILENO)==-1){// redirect stderr
			perror("redirecting stderr");
			return;
	    }
	    close(StdinPipe[PIPE_READ]);
	    close(StdinPipe[PIPE_WRITE]);
	    close(StdoutPipe[PIPE_READ]);
	    close(StdoutPipe[PIPE_WRITE]);
	    close(StderrPipe[PIPE_READ]);
	    close(StderrPipe[PIPE_WRITE]);
	    execl(Bot.name.c_str(),Bot.name.c_str(),(char*)0);//(char*)0 is really important
	    //If you get past the previous line its an error
	    perror("exec of the child process");
  	}
  	else if(nchild>0){//Parent process
  		close(StdinPipe[PIPE_READ]);//Parent does not read from stdin of child
    	close(StdoutPipe[PIPE_WRITE]);//Parent does not write to stdout of child
    	close(StderrPipe[PIPE_WRITE]);//Parent does not write to stderr of child
    	Bot.inBuff=__gnu_cxx::stdio_filebuf<char>(StdinPipe[PIPE_WRITE], std::ios::out);
    	Bot.in=new ostream(&Bot.inBuff);
		Bot.outPipe=StdoutPipe[PIPE_READ];
    	Bot.errPipe=StderrPipe[PIPE_READ];
    	Bot.pid=nchild;
  	}
  	else{//failed to create child
  		close(StdinPipe[PIPE_READ]);
	    close(StdinPipe[PIPE_WRITE]);
	    close(StdoutPipe[PIPE_READ]);
	    close(StdoutPipe[PIPE_WRITE]);
	    perror("Failed to create child process");
  	}
}

inline bool Move_In_Direction(const direction dir,vec &p)noexcept{
	p+=Directions_Vec[dir];
	return p.valid();
}

void Make_Move(array<int,S> &grid,AI &Bot,const string &Move){
	stringstream ss(Move);
	string dir;
	ss >> dir;
	if(dir=="UP" && Bot.r.y>0 && grid[Bot.r.idx()-W]==-1){
		--Bot.r.y;
	}
	else if(dir=="DOWN" && Bot.r.y<H-1 && grid[Bot.r.idx()+W]==-1){
		++Bot.r.y;
	}
	else if(dir=="RIGHT" && Bot.r.x<W-1 && grid[Bot.r.idx()+1]==-1){
		++Bot.r.x;
	}
	else if(dir=="LEFT" && Bot.r.x>0 && grid[Bot.r.idx()-1]==-1){
		--Bot.r.x;
	}
	else{
		throw(0);
	}
	grid[Bot.r.idx()]=Bot.id;
}

bool IsValidMove(const string &Move){
	stringstream ss(Move);
	string dir;
	ss >> dir;
	return dir=="RIGHT" || dir=="LEFT" || dir=="UP" || dir=="DOWN";
}

string GetMove(AI &Bot,const int turn){
	pollfd outpoll{Bot.outPipe,POLLIN|POLLPRI};
	time_point<system_clock> Start_Time{system_clock::now()};
	string Move;
	while( (!Timeout || static_cast<duration<double>>(system_clock::now()-Start_Time).count()<(turn==1?0.15:0.10)) && !IsValidMove(Move)){
		double TimeLeft{(turn==1?0.15:0.10)-static_cast<duration<double>>(system_clock::now()-Start_Time).count()};
		if(poll(&outpoll,1,Timeout?TimeLeft*1000:-1)){
			Move+=EmptyPipe(Bot.outPipe);
		}
	}
	if(!IsValidMove(Move)){
		cerr << "Loss by Timeout of AI " << Bot.id << endl;
		throw(0);
	}
	return Move;
}

inline bool Has_Won(const array<AI,N> &Bot,const int idx){
	for(int i=0;i<N;++i){
		if(i!=idx && Bot[i].alive()){
			return false;
		}
	}
	return true;
}

inline void Play_Move(array<int,S> &grid,const int turn,AI &Bot){
	try{
		Make_Move(grid,Bot,GetMove(Bot,turn));
		string err_str{EmptyPipe(Bot.errPipe)};
		if(Debug_AI){
			ofstream err_out("log.txt",ios::app);
			err_out << err_str << endl;
		}
	}
	catch(const int ex){
		Bot.stop();
	}
}

int Play_Game(const array<string,N> &Bot_Names,const array<vec,N> &Spawns){
	array<AI,N> Bot;
	array<int,S> grid;
	fill(grid.begin(),grid.end(),-1);
	for(int i=0;i<N;++i){
		Bot[i].id=i;
		Bot[i].name=Bot_Names[i];
		Bot[i].start=Spawns[i];
		Bot[i].r=Bot[i].start;
		grid[Bot[i].r.idx()]=i;
		StartProcess(Bot[i]);
	}
	int turn{0};
	while(++turn>0){
		for(int i=0;i<N;++i){
			if(Bot[i].alive()){
				if(Has_Won(Bot,i)){
					return i;
				}
				else if(!Bot[i].has_move(grid)){
					Bot[i].stop();
					continue;
				}
				*Bot[i].in << N << " " << i << endl;
				for(int j=0;j<N;++j){
					*Bot[i].in << Bot[j].start << " " << Bot[j].r << endl;
				}
				Play_Move(grid,turn,Bot[i]);
			}
		}
	}
	throw(0);
}

void Shortest_Paths(const int id,const array<int,S> &grid,const vec &p,array<int,S> &shortest)noexcept{
	array<bool,S> visited;
	fill(visited.begin(),visited.end(),false);
	shortest[p.idx()]=0;
	visited[p.idx()] = true;
	queue<vec> bfs_queue;
    bfs_queue.push(p);
	while(!bfs_queue.empty()){
        vec s=bfs_queue.front();
        bfs_queue.pop();
		for(const direction d:Directions){
			vec candidate=s;
			if(Move_In_Direction(d,candidate)){
				int add{candidate.idx()};
				if(grid[add]==-1){
					if(!visited[add]){
						visited[add]=true;
						shortest[add]=shortest[s.idx()]+1;
						bfs_queue.push(candidate);
					}
				}
			}
		}
    }
}

inline int dist(const int id1,const int id2) noexcept{
	int next{id1},out{0};
	while(next!=id2){
		next=next==N-1?0:next+1;
		++out;
	}
	return out;
}

double Voronoi(const int first_id,const int id,const array<vec,N> &Spawns) noexcept{
	array<int,S> grid;
	fill(grid.begin(),grid.end(),-1);
	for(int i=0;i<N;++i){
		grid[Spawns[i].idx()]=i;
	}
	array<array<int,S>,N> shortest;
	array<double,N> vor;
	fill(vor.begin(),vor.end(),0);
	for(int i=0;i<N;++i){
		Shortest_Paths(i,grid,Spawns[i],shortest[i]);
	}
	for(int i=0;i<S;++i){
		int min{int_max},closest_id{-1};
		for(int j=0;j<N;++j){
			int s{shortest[j][i]};
			if(s<min || (s==min && dist(first_id,j)<dist(first_id,closest_id))){
				min=s;
				closest_id=j;
			}
		}
		if(closest_id!=-1){
			++vor[closest_id];
		}
		else{
			throw(0);
		}
	}
	double total{accumulate(vor.begin(),vor.end(),0.0)};
	return vor[id]/total;
}

bool Fair_Spawns(const array<vec,N> &Spawns){
	for(int i=0;i<N;++i){
		double vor{Voronoi(0,i,Spawns)};
		cerr << vor << endl;
		if(vor<1.0/N-asymetry_limit || vor>1.0/N+asymetry_limit){
			return false;
		}
	}
	return true;
}

bool Valid_Spawns(const array<vec,N> &Spawns){
	for(int i=0;i<N;++i){//Check that no players are spawned on the same cell
		for(int j=i+1;j<N;++j){
			if(Spawns[i]==Spawns[j]){
				return false;
			}
		}
	}
	return Fair_Spawns(Spawns);
}

int Play_Round(const array<string,N> &Bot_Names){
	array<vec,N> Spawns;
	default_random_engine generator(system_clock::now().time_since_epoch().count());
	uniform_int_distribution<int> x_Distribution(0,W-1),y_Distribution(0,H-1);
	do{
		for(vec &r:Spawns){
			r=vec{x_Distribution(generator),y_Distribution(generator)};
		}
	}while(!Valid_Spawns(Spawns));
	array<int,N> winner;
	for(int i=0;i<N;++i){
		winner[i]=Play_Game(Bot_Names,Spawns);
		rotate(Spawns.begin(),Spawns.begin()+1,Spawns.end());
	}
	return winner[0]==winner[1]?winner[0]:-1;
}

int main(int argc,char **argv){
	if(argc<3){
		cerr << "Program takes 2 inputs, the names of the AIs fighting each other" << endl;
		return 0;
	}
	int N_Threads{1};
	if(argc>=4){//Optional N_Threads parameter
		N_Threads=min(2*omp_get_num_procs(),max(1,atoi(argv[3])));
		cerr << "Running " << N_Threads << " arena threads" << endl;
	}
	array<string,N> Bot_Names;
	for(int i=0;i<N;++i){
		Bot_Names[i]=argv[i+1];
	}
	cout << "Testing AI " << Bot_Names[0];
	for(int i=1;i<N;++i){
		cerr << " vs " << Bot_Names[i];
	}
	cerr << endl;
	for(int i=0;i<N;++i){//Check that AI binaries are present
		ifstream Test{Bot_Names[i].c_str()};
		if(!Test){
			cerr << Bot_Names[i] << " couldn't be found" << endl;
			return 0;
		}
		Test.close();
	}
	int N_Rounds{0},win[2]{0,0},draws{0},wins{0};
	#pragma omp parallel num_threads(N_Threads) shared(wins,draws,win,N_Rounds)
	while(true){
		int winner{Play_Round(Bot_Names)};
		if(winner!=-1){
			#pragma omp atomic
			++win[winner];
			#pragma omp atomic
			++wins;
		}
		else{
			#pragma omp atomic
			++draws;
		}
		#pragma omp atomic
		++N_Rounds;
		int N{2*(wins+draws)};
		double p{static_cast<double>(2*win[0]+draws)/N};
		cout << "Rounds: " << N_Rounds << " P0: " << 100*p << "% of wins +- " << 100*sqrt(p*(1-p)/N) << "% and "<< draws << " draws." << endl;
	}
}