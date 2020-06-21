#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stddef.h>

int t1_score;
int t2_score;
bool caught_snitch;

//----------------------------

typedef enum{
	Playing = 0, OutOfGame = 1, BludgerIncoming = 2
}PlayerStatus;

typedef enum{
	Chaser = 0, Beater = 1, Keeper = 2, Seeker = 3
}PlayerRole;

struct Player
{
	pthread_t ThreadID;
	PlayerStatus Status;
	PlayerRole Role;
	char *Team;
	char *Name;
	int Abilities;
};

char* Team1;
char* Team2;
struct Player *players[14];

struct Quaffle
{
	pthread_t ThreadID;
	struct Player LastPlayerToTouch;
	bool EnrouteToGoal;
	pthread_t GoalID;	
};



// balls
struct Quaffle quaffle;
pthread_t bludger1;
pthread_t bludger2;
pthread_t golden_snitch;

// team 1
struct Player t1_chaser1;
struct Player t1_chaser2;
struct Player t1_chaser3;
struct Player t1_beater1;
struct Player t1_beater2;
struct Player t1_keeper;
struct Player t1_seeker;
struct Player *team1[7];
int t1_count;

// team 2
struct Player t2_chaser1;
struct Player t2_chaser2;
struct Player t2_chaser3;
struct Player t2_beater1;
struct Player t2_beater2;
struct Player t2_keeper;
struct Player t2_seeker;
struct Player *team2[7];
int t2_count;

// team goal posts
pthread_t t1_goalpost;
pthread_t t2_goalpost;



struct Player *GetPlayer(pthread_t threadID){
	for (size_t i = 0; i < 14; i++)
	{
		if(players[i]->ThreadID == threadID)
		return players[i];
	}

	return NULL;
}

int RandInRange(int lower, int upper){
	return (rand() % (upper - lower + 1)) + lower; 
}

void DelayAndSignal(int signal, int seconds, pthread_t threadID){
	sleep(seconds);
	pthread_kill(threadID, signal);
}

char *get_bludger(pthread_t bludger)
{
	if (bludger == bludger1)
		return "Bludger 1";
	return "Bludger 2";
}

int FactorAbilities(int timeToWait, int abilities){
	switch (abilities)
	{
		case 5:
			return timeToWait * 6 / 10;
		case 4:
			return timeToWait * 7 / 10;
		case 3:
			return timeToWait * 8 / 10;
		case 2:
			return timeToWait * 9 / 10;
		case 1:
		default:
			return timeToWait;
	}
}

void print_scores()
{
	printf("  Team %s: %i  |  Team %s: %i  \n", Team1, t1_score, Team2, t2_score);
}

void end_game()
{
	printf("\nGame has ended!! Final score is:\n");
	print_scores();
	printf("\n\n");
	exit(0);
}

void catch_signal(int sig_caught)
{
	pthread_t threadId = pthread_self();
	struct Player *playerPtr = GetPlayer(threadId);
		
    switch (sig_caught)
    {
		case SIGWINCH:     // process SIGINT 
			if (playerPtr != NULL){
				if(playerPtr->Status == Playing)
				{
					//Mark for danger of getting out of game
					playerPtr->Status = BludgerIncoming;

					DelayAndSignal(SIGURG, 2, threadId);
				}
			}
			else if(threadId == t1_goalpost || threadId == t2_goalpost){
				quaffle.EnrouteToGoal = true;
				quaffle.GoalID = threadId;

				DelayAndSignal(SIGURG, 2, threadId);
			}
			break;
		case SIGURG:     // process SIGURG 
			if (playerPtr != NULL){
				if(playerPtr->Status == BludgerIncoming)
				{
					//get out of game
					playerPtr->Status = OutOfGame;

					if(strcmp(playerPtr->Team, Team1) == 0)
					{
						t1_count--;
						if (t1_count == 0)
						{
							printf("Team 1 has all been knocked off!\n");
							end_game();
						}
						else
							printf("Team 1 only has %i members left\n", t1_count);
					}
					else{
						t2_count--;
						if (t2_count == 0)
						{
							printf("Team 2 has all been knocked off!\n");
							end_game();
						}
						else
							printf("Team 2 only has %i members left\n", t2_count);
					}

					printf("%s of Team %s has been knocked off their broom by a Bludger!\n", playerPtr->Name, playerPtr->Team);
        			int ret1  = 100;
					pthread_exit(&ret1);
				}

			}
			else if(threadId == t1_goalpost){
				if(quaffle.EnrouteToGoal && quaffle.GoalID == threadId)
				{
					struct Player player = quaffle.LastPlayerToTouch;
					
					printf("%s scored a goal for Team: %s\n", player.Name, player.Team);
					
					t2_score += 10;				
					print_scores();

					quaffle.EnrouteToGoal = false;
				}
			}
			else if(threadId == t2_goalpost){
				if(quaffle.EnrouteToGoal && quaffle.GoalID == threadId)
				{
					struct Player player = quaffle.LastPlayerToTouch;
					
					printf("%s scored a goal for Team: %s\n", player.Name, player.Team);
					
					t1_score += 10;			
					print_scores();

					quaffle.EnrouteToGoal = false;
				}
			}
			break;
		case SIGUSR1:
			if(playerPtr != NULL){
				if(playerPtr->Status == BludgerIncoming){
					playerPtr->Status = Playing;
					printf("%s of Team %s has been saved from an incoming Bludger!\n", playerPtr->Name, playerPtr->Team);
				}
			}
			else if(quaffle.GoalID == threadId){
				quaffle.EnrouteToGoal = false;
			}
			break;
		case SIGUSR2:	// signal for chasers to throw quaffle to opposing goal
			if (playerPtr != NULL && playerPtr->Role == Chaser)
			{
				quaffle.LastPlayerToTouch = *playerPtr;

				if(strcmp(playerPtr->Team, Team1) == 0)
					pthread_kill(t2_goalpost, SIGWINCH);
				else
					pthread_kill(t1_goalpost, SIGWINCH);
			}
			break;
		default:        // should normally not happen
			printf ("Unexpected signal %d\n", sig_caught);
			break;
    }
}

void *chaser_player(void *arg)
{
	int n;
	int signo;
	struct Player player = *GetPlayer(pthread_self());

	for (;;)
	{		
		n = FactorAbilities(RandInRange(5,15), player.Abilities);

		do{
			n = sleep(n);
		}while(n != 0);

		if(player.Status == OutOfGame)
			break;
	}
}

void *beater_player(void *arg)
{
	int n, p;
	struct Player player = *GetPlayer(pthread_self());
	bool inTeam1 = strcmp(player.Team, Team1) == 0;

	for (;;)
	{
		n = FactorAbilities(RandInRange(15, 25), player.Abilities);

		do{
			n = sleep(n);
		}while(n != 0);

		if(player.Status == OutOfGame)
			break;

		struct Player chosen = *players[0];
		do{
			p = RandInRange(0,6);		// randomly select a player from 0 - 6

			if(inTeam1)
				chosen = *team1[p];
			else
				chosen = *team2[p];
		}while(chosen.Status == OutOfGame);


		printf("%s is protecting %s\n",player.Name, chosen.Name);
		pthread_kill(chosen.ThreadID, SIGUSR1);
	}
}

void *keeper_player(void *arg)
{
	int n;
	struct Player player = *GetPlayer(pthread_self());
	for (;;)
	{
		n = FactorAbilities(RandInRange(60,130), player.Abilities);

		do{
			n = sleep(n);
		}while(n != 0);

		if(player.Status == OutOfGame)
			break;

		if(strcmp(player.Team, Team1) == 0)
		{
			if(quaffle.GoalID == t1_goalpost && quaffle.EnrouteToGoal)
			{
				quaffle.LastPlayerToTouch = player;
				pthread_kill(t1_goalpost, SIGUSR1);

				printf("%s of Team %s blocked a shot attempt from %s.\n", player.Name, player.Team, quaffle.LastPlayerToTouch.Name);
			}
			else{
				pthread_kill(t1_goalpost, SIGUSR1);
				printf("%s checks Team %s's goal\n", player.Name, player.Team);
			}
		}
		else
		{
			if(quaffle.GoalID == t2_goalpost && quaffle.EnrouteToGoal)
			{
				quaffle.LastPlayerToTouch = player;
				pthread_kill(t2_goalpost, SIGUSR1);

				printf("%s of Team %s blocked a shot attempt from %s.\n", player.Name, player.Team, quaffle.LastPlayerToTouch.Name);
			}
			else{
				pthread_kill(t2_goalpost, SIGUSR1);
				printf("%s checks Team %s's goal\n", player.Name, player.Team);
			}
		}
	}
}

void *seeker_player(void *arg)
{
	int n;
	struct Player player = *GetPlayer(pthread_self());
	for(;;)
	{
		n = FactorAbilities(RandInRange(50, 150), player.Abilities);

		do{
			n = sleep(n);
		}while(n != 0);

		if(player.Status == OutOfGame)
			break;

		printf("%s of Team %s is seeking Golden Snitch!\n", player.Name, player.Team);
		if (caught_snitch)
		{
			if(strcmp(player.Team, Team1) == 0)
				t1_score += 150;
			else
				t2_score += 150;
			printf("%s has caught the Golden Snitch!\n", player.Name);
			end_game();
		}
		printf("%s couldn't catch the Golden Snitch...\n", player.Name);
	}
}

void *goal(void *arg)
{
	pthread_t ID = pthread_self();

	int signo;
	for(;;)
	{
		sleep(5);
	}
}

void *quaffle_ball(void *arg)
{
	int n;
	int p;
	for(;;)
	{
		// get random number b/w 7 & 18 to sleep
		sleep(RandInRange(4,10));

		printf("Quaffle ball has been moved!\n");
		
		struct Player chosen = *players[0];
		do{
			p = RandInRange(0,5);		// randomly select a player from 0 - 5

			if(p<3)
				chosen = *team1[p];
			else
				chosen = *team2[p - 3];
		}while(chosen.Status == OutOfGame);

		printf("%s has the Quaffle from Team %s!\n", chosen.Name, chosen.Team);
		pthread_kill(chosen.ThreadID, SIGUSR2);
	}
}

void *bludger_ball(void *arg)
{
	int n;
	int p;
	for(;;)
	{
		sleep(RandInRange(16,23));

		printf("%s has started moving!\n", get_bludger(pthread_self()));

		struct Player chosen = *players[0];
		do{
			p = RandInRange(0,13);		// randomly select a player from 0 - 13

			if (p < 7)				// randomly selected player is in team 1
				chosen = *team1[p];
			else					// randomly selected player is in team 2
				chosen = *team2[p-7];
		}while(chosen.Status != Playing);

		printf("%s is targeting %s of Team %s!\n", get_bludger(pthread_self()), chosen.Name, chosen.Team);
		pthread_kill(chosen.ThreadID, SIGWINCH);
	}
}

void *golden_snitch_ball(void *arg)
{
	int n;
	for(;;)
	{
		// get random number b/w 10 & 100 to sleep
		n = RandInRange(10,100);
		sleep(n);

		caught_snitch = !caught_snitch;
		printf("Golden snitch has been toggled: %d\n", caught_snitch);
	}
}

// creates and initializes all the pthreads (players, balls, and goals)
void init_pethreads()
{
	// create team 1
	printf("TEAM %s *************************************************\n", Team1);
	if (0 != pthread_create(&t1_chaser1.ThreadID, NULL, chaser_player, NULL))
		printf("Failed to create thread\n");
	else
		printf("%s Chaser %s steps onto the field ready to play: %li\n", Team1, t1_chaser1.Name, t1_chaser1.ThreadID);

	if (0 != pthread_create(&t1_chaser2.ThreadID, NULL, chaser_player, NULL))
		printf("Failed to create thread\n");
	else
		printf("%s Chaser %s steps onto the field ready to play: %li\n", Team1, t1_chaser2.Name, t1_chaser2.ThreadID);

	if (0 != pthread_create(&t1_chaser3.ThreadID, NULL, chaser_player, NULL))
		printf("Failed to create thread\n");
	else
		printf("%s Chaser %s steps onto the field ready to play: %li\n", Team1, t1_chaser3.Name, t1_chaser3.ThreadID);

	if (0 != pthread_create(&t1_beater1.ThreadID, NULL, beater_player, NULL))
		printf("Failed to create thread\n");
	else
		printf("%s Beater %s steps onto the field ready to play: %li\n", Team1, t1_beater1.Name, t1_beater1.ThreadID);

	if (0 != pthread_create(&t1_beater2.ThreadID, NULL, beater_player, NULL))
		printf("Failed to create thread\n");
	else
		printf("%s Beater %s steps onto the field ready to play: %li\n", Team1, t1_beater2.Name, t1_beater2.ThreadID);

	if (0 != pthread_create(&t1_keeper.ThreadID, NULL, keeper_player, NULL))
		printf("Failed to create thread\n");
	else
		printf("%s Keeper %s steps onto the field ready to play: %li\n", Team1, t1_keeper.Name, t1_keeper.ThreadID);

	if (0 != pthread_create(&t1_seeker.ThreadID, NULL, seeker_player, NULL))
		printf("Failed to create thread\n");
	else
		printf("%s Seeker %s steps onto the field ready to play: %li\n", Team1, t1_seeker.Name, t1_seeker.ThreadID);
	printf("********************************************************\n\n");	

	// create team 2
	printf("TEAM %s *************************************************\n", Team2);
	if (0 != pthread_create(&t2_chaser1.ThreadID, NULL, chaser_player, NULL))
		printf("Failed to create thread\n");
	else
		printf("%s Chaser %s steps onto the field ready to play: %li\n", Team2, t2_chaser1.Name, t2_chaser1.ThreadID);

	if (0 != pthread_create(&t2_chaser2.ThreadID, NULL, chaser_player, NULL))
		printf("Failed to create thread\n");
	else
		printf("%s Chaser %s steps onto the field ready to play: %li\n", Team2, t2_chaser2.Name, t2_chaser2.ThreadID);

	if (0 != pthread_create(&t2_chaser3.ThreadID, NULL, chaser_player, NULL))
		printf("Failed to create thread\n");
	else
		printf("%s Chaser %s steps onto the field ready to play: %li\n", Team2, t2_chaser3.Name, t2_chaser3.ThreadID);

	if (0 != pthread_create(&t2_beater1.ThreadID, NULL, beater_player, NULL))
		printf("Failed to create thread\n");
	else
		printf("%s Beater %s steps onto the field ready to play: %li\n", Team2, t2_beater1.Name, t2_beater1.ThreadID);

	if (0 != pthread_create(&t2_beater2.ThreadID, NULL, beater_player, NULL))
		printf("Failed to create thread\n");
	else
		printf("%s Beater %s steps onto the field ready to play: %li\n", Team2, t2_beater2.Name, t2_beater2.ThreadID);

	if (0 != pthread_create(&t2_keeper.ThreadID, NULL, keeper_player, NULL))
		printf("Failed to create thread\n");
	else
		printf("%s Keeper %s steps onto the field ready to play: %li\n", Team2, t2_keeper.Name, t2_keeper.ThreadID);

	if (0 != pthread_create(&t2_seeker.ThreadID, NULL, seeker_player, NULL))
		printf("Failed to create thread\n");
	else
		printf("%s Seeker %s steps onto the field ready to play: %li\n", Team2, t2_seeker.Name, t2_seeker.ThreadID);
	printf("********************************************************\n\n");	

	// create goals
	printf("GOALS **************************************************\n");
	if (0 != pthread_create(&t1_goalpost, NULL, goal, NULL))
		printf("Failed to create thread\n");
	else
		printf("Created %s goal post: %li\n", Team1, t1_goalpost);

	if (0 != pthread_create(&t2_goalpost, NULL, goal, NULL))
		printf("Failed to create thread\n");
	else
		printf("Created %s goal post: %li\n", Team2, t2_goalpost);
	printf("********************************************************\n\n");	

	// create balls
	printf("BALL ***************************************************\n");
	if (0 != pthread_create(&quaffle.ThreadID, NULL, quaffle_ball, NULL))
		printf("Failed to create thread\n");
	else
		printf("Created quaffle ball:   %li\n", quaffle.ThreadID);

	if (0 != pthread_create(&bludger1, NULL, bludger_ball, NULL))
		printf("Failed to create thread\n");
	else
		printf("Created bludger ball 1: %li\n", bludger1);

	if (0 != pthread_create(&bludger2, NULL, bludger_ball, NULL))
		printf("Failed to create thread\n");
	else
		printf("Created bludger ball 2: %li\n", bludger2);

	if (0 != pthread_create(&golden_snitch, NULL, golden_snitch_ball, NULL))
		printf("Failed to create thread\n");
	else
		printf("Created golden snitch:  %li\n", golden_snitch);
	printf("********************************************************\n\n");	
}

void setup_teams()
{
	//*** Each Team gets to spread 21 talent points accross their players ***
	//Higher talent points will make you a better player by making your wait time shorter
	Team1 = "Gryffindor";
	t1_chaser1.Name = "Katie Bell";
	t1_chaser1.Role = Chaser;
	t1_chaser1.Status = Playing;
	t1_chaser1.Team = Team1;
	t1_chaser1.Abilities = 2;
	
	t1_chaser2.Name = "Angelina Johnson";
	t1_chaser2.Role = Chaser;
	t1_chaser2.Status = Playing;
	t1_chaser2.Team = Team1;
	t1_chaser2.Abilities = 4;
	
	t1_chaser3.Name = "Alicia Spinnet";
	t1_chaser3.Role = Chaser;
	t1_chaser3.Status = Playing;
	t1_chaser3.Team = Team1;
	t1_chaser3.Abilities = 2;
	
	t1_beater1.Name = "Fred Weasley";
	t1_beater1.Role = Beater;
	t1_beater1.Status = Playing;
	t1_beater1.Team = Team1;
	t1_beater1.Abilities = 3;
	
	t1_beater2.Name = "George Weasley";
	t1_beater2.Role = Beater;
	t1_beater2.Status = Playing;
	t1_beater2.Team = Team1;
	t1_beater2.Abilities = 3;
	
	t1_keeper.Name = "Ron Weasley";
	t1_keeper.Role = Keeper;
	t1_keeper.Status = Playing;
	t1_keeper.Team = Team1;
	t1_keeper.Abilities = 2;
	
	t1_seeker.Name = "Harry Potter";
	t1_seeker.Role = Seeker;
	t1_seeker.Status = Playing;
	t1_seeker.Team = Team1;
	t1_seeker.Abilities = 5;


	Team2 = "Slytherin";
	t2_chaser1.Name = "Graham Montague";
	t2_chaser1.Role = Chaser;
	t2_chaser1.Status = Playing;
	t2_chaser1.Team = Team2;
	t2_chaser1.Abilities = 3;
	
	t2_chaser2.Name = "Adrian Pucey";
	t2_chaser2.Role = Chaser;
	t2_chaser2.Status = Playing;
	t2_chaser2.Team = Team2;
	t2_chaser2.Abilities = 2;
	
	t2_chaser3.Name = "Cassius Warrington";
	t2_chaser3.Role = Chaser;
	t2_chaser3.Status = Playing;
	t2_chaser3.Team = Team2;
	t2_chaser3.Abilities = 2;
	
	t2_beater1.Name = "Vincent Crabbe";
	t2_beater1.Role = Beater;
	t2_beater1.Status = Playing;
	t2_beater1.Team = Team2;
	t2_beater1.Abilities = 4;
	
	t2_beater2.Name = "Gregory Goyle";
	t2_beater2.Role = Beater;
	t2_beater2.Status = Playing;
	t2_beater2.Team = Team2;
	t2_beater2.Abilities = 3;
	
	t2_keeper.Name = "Miles Bletchley";
	t2_keeper.Role = Keeper;
	t2_keeper.Status = Playing;
	t2_keeper.Team = Team2;
	t2_keeper.Abilities = 3;
	
	t2_seeker.Name = "Draco Malfoy";
	t2_seeker.Role = Seeker;
	t2_seeker.Status = Playing;
	t2_seeker.Team = Team2;
	t2_seeker.Abilities = 4;


	team1[0] = &t1_chaser1;
	team1[1] = &t1_chaser2;
	team1[2] = &t1_chaser3;
	team1[3] = &t1_beater1;
	team1[4] = &t1_beater2;
	team1[5] = &t1_keeper;
	team1[6] = &t1_seeker;

	team2[0] = &t2_chaser1;
	team2[1] = &t2_chaser2;
	team2[2] = &t2_chaser3;
	team2[3] = &t2_beater1;
	team2[4] = &t2_beater2;
	team2[5] = &t2_keeper;
	team2[6] = &t2_seeker;
	
	players[0] = &t1_chaser1;
	players[1] = &t1_chaser2;
	players[2] = &t1_chaser3;
	players[3] = &t1_beater1;
	players[4] = &t1_beater2;
	players[5] = &t1_keeper;
	players[6] = &t1_seeker;

	players[7] = &t2_chaser1;
	players[8] = &t2_chaser2;
	players[9] = &t2_chaser3;
	players[10] = &t2_beater1;
	players[11] = &t2_beater2;
	players[12] = &t2_keeper;
	players[13] = &t2_seeker;
}

int main(void)
{
	srand(time(0)); 
		
	printf("\nMain thread id is: %li \n\n", pthread_self());

	// initialize values
	t1_score = t2_score = 0;
	t1_count = t2_count = 7;
	caught_snitch = false;

	struct sigaction signalAction;
	memset(&signalAction, 0, sizeof(signalAction));

    signalAction.sa_handler = catch_signal;
    signalAction.sa_flags = SA_NODEFER;

	if (sigaction(SIGWINCH, &signalAction, 0) == -1)//Had trouble getting SIGINT to work so used SIGWINCH
        perror("Failed to block SIGWINCH signal\n");
	if (sigaction(SIGUSR2, &signalAction, 0) == -1)
        perror("Failed to block SIGUSR2 signal\n");
	if (sigaction(SIGUSR1, &signalAction, 0) == -1)
        perror("Failed to block SIGUSR1 signal\n");
	if (sigaction(SIGURG, &signalAction, 0) == -1)//This is a helper signal
        perror("Failed to block SIGURG signal\n");

	setup_teams();
	init_pethreads();

	printf("-- Game Start! --\n");

	for(;;);
}