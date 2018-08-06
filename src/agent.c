/*********************************************
 *  agent.c
 *  Sample Agent for Text-Based Adventure Game
 *  COMP3411 Artificial Intelligence
 *  UNSW Session 1, 2017
*/
/*
***********************************************************************************************************************
Data structure used: 
struct Status: record the current position and direction of the agent, and whether it has key, axe, treasure, raft, and
               number of stones. Also 'a' is the top-left point of the map it has seen and 'b' is the bottom-right 
               point of the map it has seen. Also record whether has been to sea.

struct astarNode: this node record the position and direction of the state that agent would be. Also the g: cost, h:
                  heuristic value, f: the sum of g and h. 'steps' record the steps agent needs to get to this state 
                  from current state. And a pointer point to next node.
    astarnode_insert(): check if there is already node in same state and with lower f values, which means more efficient
                        and don't insert. Or delete the original one, and insert new one in the right position based on
                        the f value.

struct tool, door, tree is just used to record the tools, doors and trees that have been seen but not reach/open/chop yet.

We have linked lists to store all tools, doors, trees that have been seen.

Algorithm:
main: 1. Get the current view, as current direction has four situations, so rotate the view to the right direciton, which
        is up-north, right-east, down-south, left-south. And use the view to update the global map. The global map will be
        used to make decision later.
      2. Using the global map to make a dicision what is going to do in the next several actions, cause it may needs a 
         few actions to get to one desired point. And if we only make one action decision at each time, the agent may 
         get stuck at some point.
      3. The dicision usually has more than one actions, so in the next few iteration, just pass the action one by one to
         the server, until the decision finished.

find_a_path: 1. If we are in go to lake state, just go to the destination we have made last time.
             2. If we already got treasure, try to go back to original point if possible.
             3. If we seen some tools before and not collect it yet, if accessable, go to collect it.
             4. If a door is in the neighbor point of the current point, turn to it and open it.
             5. If we have key and saw a door before, go to the neighbor point of the door if possible.
             6. Try to explore the map, basically go to the point we have seen but not been there, also accessable.
             6. If a tree is in the neighbor point of the current point, turn to it and chop it.
             7. If we have axe and saw a tree before, to to the neighbor point of the tree if possible.
             If the code reach here, means agent stuck on the land or the sea.
             8. If we are hang around on the sea:
                a: Go to the land which has tree.
                b: If we have treasure, go to the land of start point.
                c: Go to the land which has door.
             9. If we are hang around on the land:
                a: If we have stones, try to go to a lake and we can get stone on the other land.
                b: Try to use raft as stone to pass the lake.
                c: Go back to sea.
             10. If still not get treasure yet, treasure is on an island, go to the island using stones.

is_accessable: Mark the neighbor point if it is reachable, doing this recursively, so the accessable[][] will spread out
               this is basiclly like BFS to mark all accessable points.

get_path_a_star: Using A* search to find the path from src to dest. The prev-request is these two points should be accessable,
                 so usually I call is_accessable() first.

***********************************************************************************************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "pipe.h"



typedef int bool;
#define true 1
#define false 0

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define MAX_SIZE 80

// This include 2 layer of '.' at border
#define MAP_SIZE 2*MAX_SIZE-1+2*2

// At the middle of the map
#define START_POINT MAX_SIZE-1+2

// Char for different stuffs
// Tools
#define AXE 'a'
#define KEY 'k'
#define STONE 'o'
#define TREASURE '$'

// Different directions
#define NORTH 0
#define EAST 1
#define SOUTH 2
#define WEST 3

//*********************************************************************************************************************

struct point
{
    int x;
    int y;
};

// Status for agent, global
struct Status
{
    struct point pos;
    int direction;
    bool key;
    bool axe;
    bool treasure;
    bool raft;
    int num_stone;
    struct point a;// top-left point have known
    struct point b;// bottom-right point have known
    bool been_to_sea;
}; 

struct astarNode
{
    int x;
    int y;
    int direction;
    int g;
    int h;
    int f;
    char steps[200];
    struct astarNode* next;
};

typedef struct astarNode* AstarNode;

struct astarNodeHead
{
    int num;
    AstarNode list;
};

typedef struct astarNodeHead* AstarNodeHead;

AstarNode astarNode_create(int x, int y, int direction, int g, int h, char act, char* steps)
{
    AstarNode ret = malloc(sizeof(struct astarNode));
    ret->x=x;
    ret->y=y;
    ret->direction=direction;
    ret->g=g;
    ret->h=h;
    ret->f=g+h;
    ret->next=NULL;
    int i;
    for(i=0; i<g; ++i)
    {
        if(i==g-1) ret->steps[i] = act;
        else ret->steps[i] = steps[i];
    }
    return ret;
}

void astarNode_insert(AstarNodeHead queue, AstarNode node)
{
    if(queue->num==0)
    {
        queue->list=node;
        queue->num++;
        return;
    }
    AstarNode curr = queue->list;
    AstarNode prev = queue->list;
    assert(curr!=NULL);

    while(curr != NULL)
    {
        // Find a node in same state
        if(curr->x==node->x && curr->y==node->y && curr->direction==node->direction)
        {
            // Existing one is more efficient, don't insert
            if(curr->f < node->f) 
            {
                free(node);
                return; 
            }
            // Delete existing one
            if(curr==queue->list)
            {
                queue->list = curr->next;
                queue->num--;
                free(curr);
            }
            else
            {
                prev->next = curr->next;
                queue->num--;
                free(curr);
            }
            break;
        }
        prev = curr;
        curr = curr->next;
    }

    curr = queue->list;
    prev = queue->list;

    if(queue->list==NULL)
    {
        queue->list = node;
        queue->num++;
        return;
    }

    while(curr!=NULL)
    {
        if(curr->f > node->f)
        {
            if(prev==curr)
            {
                queue->list = node;
                node->next = curr;
            }
            else
            {
                prev->next = node;
                node->next = curr;
            }
            queue->num++;
            return;
        }
        prev=curr;
        curr=curr->next;
    }
    queue->num++;
    prev->next = node;

    return;
}

struct tool
{
    char type;
    int x;
    int y;
    struct tool* next;
};

typedef struct tool* Tool;

struct door
{
    int x;
    int y;
    struct door* next;
};

typedef struct door* Door;

struct tree
{
    int x;
    int y;
    struct tree* next;
};

typedef struct tree* Tree;

//*********************************************************************************************************************

// Functions declared 

// Initialise the map, which going to be used to memorize
void map_init();

// Initialise the status at the begin of the game
void status_init();

// Print current view
void print_view();

// Update status of map, tool, status based on view
void update_status( char env[5][5] );

// Rotate the matrix environment(view)
void rotate_view(char (*env)[5]);

// Using the environment(view) to update map
void updata_map(char (*env)[5]);

// Add new discover tool to list
void addTool(char a_tool, int x, int y);

// Delete tool that have been reached
void deleteTool(char a_tool, int x, int y);

// Add new discover door to list
void addDoor(int x, int y);

// Delete door that have been opened
void deleteDoor(int x, int y);

// Add new discover tree to list
void addTree(int x, int y);

// Delete tree that have been chopped
void deleteTree(int x, int y);

// Add new discovered tree
void addTreasure(int x, int y);

// Find the next destination and find a path to it
int find_a_path( char* step );

// Get if accessable form src to dest
bool is_accessable(int srcX, int srcY, int destX, int destY, bool transfer);

// Recursive function to mark if a point is accessable from (x,y)
void recursive_mark_access(int x, int y, char type);

// Get if accessable form src to dest, works on different array
bool is_accessable_2(int srcX, int srcY, int destX, int destY, bool transfer);

// Recursive function to mark if a point is accessable from (x,y), works on different array
void recursive_mark_access_2(int x, int y, char type);

// Open the neighbor door
int open_door(char* steps);

// Chop the neighbor tree
int chop_tree(char* steps);

// Get action to send to server
char interpret_action(char* step, int curr_step);


// Help function*******************************************************************************************************

// Print the map that have been seen
void print_map();

// Heuristic function of two points on map
int heuristic(int srcX, int srcY, int destX, int destY);

// Whether availabe to pass lake by stones
bool is_accessable_lake(int srcX, int srcY, int destX, int destY);

// Get a path from src to dest, using A* search
int get_path_a_star(int srcX, int srcY, int direction, int destX, int destY, char* path);

// Print all tool
void print_tool_list(void);

// Print accessable from current point
void print_accessible(void);

// Print accessable from current point
void print_accessible_2(void);

// Global variables****************************************************************************************************

int   pipe_fd;
FILE* in_stream;
FILE* out_stream;

char view[5][5];
char map[MAP_SIZE][MAP_SIZE];
bool accessable[MAP_SIZE][MAP_SIZE];
bool accessable_2[MAP_SIZE][MAP_SIZE];
bool seen[MAP_SIZE][MAP_SIZE];
bool been[MAP_SIZE][MAP_SIZE];
struct Status status;

Tool tool_list;
Door door_list;
Tree tree_list;
struct point* Treasure;

bool go_on_lake;
int stoneX;
int stoneY;

//*********************************************************************************************************************

int main( int argc, char *argv[] )
{
    char action;
    int sd;
    int ch;
    int i,j;

    if ( argc < 3 ) {
        printf("Usage: %s -p port\n", argv[0] );
        exit(1);
    }

    // open socket to Game Engine
    sd = tcpopen("localhost", atoi( argv[2] ));

    pipe_fd    = sd;
    in_stream  = fdopen(sd,"r");
    out_stream = fdopen(sd,"w");

    map_init();
    
    status_init();

    char* steps = (char*)malloc(200); 

    int num_of_steps=0;
    int curr_step=0;

    while(1) 
    {
        // scan 5-by-5 wintow around current location
        for( i=0; i < 5; i++ ) 
        {
            for( j=0; j < 5; j++ ) 
            {
                if( !(( i == 2 )&&( j == 2 ))) 
                {
                    ch = getc( in_stream );
                    if( ch == -1 ) {
                        exit(1);
                    }
                    view[i][j] = ch;
                }
            }
        }

        //print_view(); // COMMENT THIS OUT BEFORE SUBMISSION

        update_status( view );

        if(curr_step>=num_of_steps)
        {
            num_of_steps = find_a_path( steps );
            curr_step = 0;
        }

        if(num_of_steps==0) 
        {
            //print_map();
            while(1);
            break;
        }


        action = interpret_action(steps, curr_step);
        curr_step++;


        putc( action, out_stream );
        fflush( out_stream );
    }

    return 0;
}


void map_init()
{
    int i,j;
    for(i=0; i<MAP_SIZE; ++i)
    {
        for (j=0; j<MAP_SIZE; ++j)
        {
            map[i][j]='.';
            seen[i][j]=false;
            been[i][j]=false;
        }
    }
}

// Initialise the status at the begin of the game
void status_init()
{
    status.pos.x=START_POINT;
    status.pos.y=START_POINT;
    status.direction = SOUTH;
    status.key = false;
    status.axe = false;
    status.treasure = false;
    status.raft = false;
    status.num_stone = 0;
    status.a.x=START_POINT-2;
    status.a.y=START_POINT-2;
    status.b.x=START_POINT+2;
    status.b.y=START_POINT+2;
    status.been_to_sea = false;
    tool_list = NULL;
    door_list = NULL;
    tree_list = NULL;
    Treasure = NULL;
    go_on_lake=false;    
}

// Print current view
void print_view()
{
    int i,j;

    printf("\n+-----+\n");
    for( i=0; i < 5; i++ ) {
        putchar('|');
        for( j=0; j < 5; j++ ) {
            if(( i == 2 )&&( j == 2 )) {
                putchar( '^' );
            }
            else {
                putchar( view[i][j] );
            }
        }
        printf("|\n");
    }
    printf("+-----+\n");
}

void update_status( char env[5][5] )
{
    rotate_view(env);
    updata_map(env);
}

// Rotate the matrix environment(view)
void rotate_view(char (*env)[5])
{
    int i,j;
    if(status.direction==NORTH)
    {
        // Do not need rotate
    }
    else if(status.direction==EAST)
    {
        char temp;
        // i starts from 1 to skip first row
        for (i = 1; i < 5; ++i) {
            for (j = 0; j < i; ++j) {
                temp = env[i][j];
                env[i][j] = env[j][i];
                env[j][i] = temp;
            }
        }
        // Switch in each row
        for (i = 0; i < 5; ++i) {
            temp = env[i][0];
            env[i][0] = env[i][4];
            env[i][4] = temp;
            temp = env[i][1];
            env[i][1] = env[i][3];
            env[i][3] = temp;
        }
    }
    else if(status.direction==SOUTH)
    {
        char temp;
        // Switch in each row
        for (i = 0; i < 5; ++i) {
            temp = env[i][0];
            env[i][0] = env[i][4];
            env[i][4] = temp;
            temp = env[i][1];
            env[i][1] = env[i][3];
            env[i][3] = temp;
        }
        // Switch in each col
        for (i = 0; i < 5; ++i) {
            temp = env[0][i];
            env[0][i] = env[4][i];
            env[4][i] = temp;
            temp = env[1][i];
            env[1][i] = env[3][i];
            env[3][i] = temp;
        }
    }
    else if(status.direction==WEST)
    {
        char temp;
        // i starts from 1 to skip first row
        for (i = 1; i < 5; ++i) {
            for (j = 0; j < i; ++j) {
                temp = env[i][j];
                env[i][j] = env[j][i];
                env[j][i] = temp;
            }
        }
        // Switch in each col
        for (i = 0; i < 5; ++i) {
            temp = env[0][i];
            env[0][i] = env[4][i];
            env[4][i] = temp;
            temp = env[1][i];
            env[1][i] = env[3][i];
            env[3][i] = temp;
        }
    }
    else
    {
        //printf("Error occur in direction\n");
    }
}

// Using the environment(view) to update map
void updata_map(char (*env)[5])
{
    int i,j;
    for(i=0; i<5; ++i)
    {
        for(j=0; j<5; ++j)
        {
            if(i==2&&j==2) 
            {
                deleteTool(map[status.pos.y][status.pos.x], status.pos.x, status.pos.y);
                if(map[status.pos.y][status.pos.x]=='$') status.treasure=true;
                been[status.pos.y][status.pos.x]=true;
                continue;
            }
            
            map[status.pos.y+i-2][status.pos.x+j-2] = env[i][j];
            seen[status.pos.y+i-2][status.pos.x+j-2] = true;

            status.a.x=MIN(status.a.x, status.pos.x+j-2);
            status.a.y=MIN(status.a.y, status.pos.y+i-2);
            status.b.x=MAX(status.b.x, status.pos.x+j-2);
            status.b.y=MAX(status.b.y, status.pos.y+i-2);

            if(    map[status.pos.y+i-2][status.pos.x+j-2] == 'a' 
                || map[status.pos.y+i-2][status.pos.x+j-2] == 'k'
                || map[status.pos.y+i-2][status.pos.x+j-2] == 'o')
            {
                addTool(map[status.pos.y+i-2][status.pos.x+j-2], status.pos.x+j-2, status.pos.y+i-2);
            }

            if(map[status.pos.y+i-2][status.pos.x+j-2] == '-')
            {
                addDoor(status.pos.x+j-2, status.pos.y+i-2);
            }

            if(map[status.pos.y+i-2][status.pos.x+j-2] == 'T')
            {
                addTree(status.pos.x+j-2, status.pos.y+i-2);
            }

            if(map[status.pos.y+i-2][status.pos.x+j-2] == '$')
            {
                addTreasure(status.pos.x+j-2, status.pos.y+i-2);
            }
        }
    }
}

void addTool(char a_tool, int x, int y)
{
    Tool curr = tool_list;
    Tool prev;
    while(curr!=NULL)
    {
        if(curr->type==a_tool && curr->x==x && curr->y==y) return;
        prev=curr;
        curr=curr->next;
    }
    
    Tool new_tool = malloc(sizeof(struct tool));
    new_tool->type = a_tool;
    new_tool->x=x;
    new_tool->y=y;
    new_tool->next = NULL;

    if(tool_list==NULL)
    {
        tool_list=new_tool;
    }
    else
    {
        prev->next = new_tool;
    }
}

void deleteTool(char a_tool, int x, int y)
{
    if(tool_list==NULL) return;

    if(a_tool=='k') status.key=true;
    else if(a_tool=='a') status.axe=true;  
    else if(a_tool=='o') 
    {
        status.num_stone++;
        map[status.pos.y][status.pos.x]=' ';  
    }
    Tool curr = tool_list;
    Tool prev = tool_list;

    while(curr!=NULL)
    {
        if(curr->type==a_tool && curr->x==x && curr->y==y) break;
        prev=curr;
        curr=curr->next;
    }

    if(curr==tool_list)
    {
        tool_list = curr->next;
        free(curr);
        return;
    }

    if(curr==NULL) return;

    prev->next = curr->next;
    free(curr);
}

void addDoor(int x, int y)
{
    Door curr = door_list;
    Door prev;
    while(curr!=NULL)
    {
        if(curr->x==x && curr->y==y) return;
        prev=curr;
        curr=curr->next;
    }
    
    Door new_door = malloc(sizeof(struct door));
    new_door->x=x;
    new_door->y=y;
    new_door->next = NULL;

    if(door_list==NULL)
    {
        door_list=new_door;
    }
    else
    {
        prev->next = new_door;
    }
}

void deleteDoor(int x, int y)
{
    assert(door_list!=NULL);

    Door curr = door_list;
    Door prev = door_list;

    while(curr!=NULL)
    {
        if(curr->x==x && curr->y==y) break;
        prev=curr;
        curr=curr->next;
    }

    if(curr==door_list)
    {
        door_list = curr->next;
        free(curr);
        return;
    }

    assert(curr!=NULL);

    prev->next = curr->next;
    free(curr);
}

void addTree(int x, int y)
{
    Tree curr = tree_list;
    Tree prev;
    while(curr!=NULL)
    {
        if(curr->x==x && curr->y==y) return;
        prev=curr;
        curr=curr->next;
    }
    
    Tree new_tree = malloc(sizeof(struct tree));
    new_tree->x=x;
    new_tree->y=y;
    new_tree->next = NULL;

    if(tree_list==NULL)
    {
        tree_list=new_tree;
    }
    else
    {
        prev->next = new_tree;
    }    
}

void deleteTree(int x, int y)
{
    assert(tree_list!=NULL);

    Tree curr = tree_list;
    Tree prev = tree_list;

    while(curr!=NULL)
    {
        if(curr->x==x && curr->y==y) break;
        prev=curr;
        curr=curr->next;
    }

    if(curr==tree_list)
    {
        tree_list = curr->next;
        free(curr);
        return;
    }

    assert(curr!=NULL);

    prev->next = curr->next;
    free(curr);
}

void addTreasure(int x, int y)
{
    if(Treasure!=NULL) return;
    Treasure = malloc(sizeof(struct point));
    Treasure->x=x;
    Treasure->y=y;
}

int find_a_path( char* steps )
{
    int ret=0, i, j, m, n;

    if(go_on_lake)
    {
        for (i = 0; i < MAP_SIZE; ++i)
        {
            for(j=0; j < MAP_SIZE; ++j)
            {
                accessable[i][j]=true;
            }
        }
        ret = get_path_a_star(status.pos.x, status.pos.y, status.direction, stoneX, stoneY, steps);
        go_on_lake=false;
        return ret;            
    }

    is_accessable(status.pos.x, status.pos.y, 0, 0, false);


    // Try to go back start point
    if(status.treasure)
    {
        if(accessable[START_POINT][START_POINT])
        {
            ret = get_path_a_star(status.pos.x, status.pos.y, status.direction, START_POINT, START_POINT, steps);
            return ret;              
        }
    }


    // Try to collect tools
    if(tool_list!=NULL)
    {
        //printf("delete tool\n");
        Tool curr_tool = tool_list;
        while(curr_tool!=NULL)
        {
            if(accessable[curr_tool->y][curr_tool->x])
            {
                ret = get_path_a_star(status.pos.x, status.pos.y, status.direction, curr_tool->x, curr_tool->y, steps);
                return ret;
            }
            curr_tool = curr_tool->next;
        }
    }


    // Try to open door
    if(status.key && (map[status.pos.y-1][status.pos.x]=='-' || map[status.pos.y][status.pos.x+1]=='-'
            || map[status.pos.y+1][status.pos.x]=='-' || map[status.pos.y][status.pos.x-1]=='-'))
    {
        //printf("open door\n");
        ret = open_door(steps);
        return ret;
    }


    // Try to go to door
    if(status.key && door_list!=NULL)
    {
        //printf("go to door\n");
        Door curr_door = door_list;
        while(curr_door!=NULL)
        {
            if(accessable[curr_door->y-1][curr_door->x])
            {
                ret = get_path_a_star(status.pos.x, status.pos.y, status.direction, curr_door->x, curr_door->y-1, steps);
                return ret;                
            }
            if(accessable[curr_door->y][curr_door->x+1])
            {
                ret = get_path_a_star(status.pos.x, status.pos.y, status.direction, curr_door->x+1, curr_door->y, steps);
                return ret;                
            }
            if(accessable[curr_door->y+1][curr_door->x])
            {
                ret = get_path_a_star(status.pos.x, status.pos.y, status.direction, curr_door->x, curr_door->y+1, steps);
                return ret;                
            }
            if(accessable[curr_door->y][curr_door->x-1])
            {
                ret = get_path_a_star(status.pos.x, status.pos.y, status.direction, curr_door->x-1, curr_door->y, steps);
                return ret;                
            }
            curr_door = curr_door->next;
        }
    }


    //printf("explore\n");
    // Just to find if every points accessable
    int nearX = 0;
    int nearY = 0;
    int distance = 100000;
    for(i=0; i<MAP_SIZE; ++i)
    {
        for(j=0; j<MAP_SIZE; ++j)
        {
            if(accessable[i][j] && seen[i][j] && !been[i][j] && heuristic(status.pos.x, status.pos.y, j, i)<distance)
            {
                nearX = j;
                nearY = i;
                distance = heuristic(status.pos.x, status.pos.y, j, i);
            }
        }
    }

    if(distance!=100000) 
    {
        ret = get_path_a_star(status.pos.x, status.pos.y, status.direction, nearX, nearY, steps);
        return ret;  
    }


    // Try to chop tree
    if(status.axe && (map[status.pos.y-1][status.pos.x]=='T' || map[status.pos.y][status.pos.x+1]=='T'
            || map[status.pos.y+1][status.pos.x]=='T' || map[status.pos.y][status.pos.x-1]=='T'))
    {
        //printf("chop tree\n");
        ret = chop_tree(steps);
        return ret;
    }


    // Try to go to tree
    if(map[status.pos.y][status.pos.x]!='~' && status.axe && tree_list!=NULL)
    {
        //printf("go to tree\n");
        Tree curr_tree = tree_list;
        while(curr_tree!=NULL)
        {
            if(accessable[curr_tree->y-1][curr_tree->x])
            {
                ret = get_path_a_star(status.pos.x, status.pos.y, status.direction, curr_tree->x, curr_tree->y-1, steps);
                return ret;                
            }
            if(accessable[curr_tree->y][curr_tree->x+1])
            {
                ret = get_path_a_star(status.pos.x, status.pos.y, status.direction, curr_tree->x+1, curr_tree->y, steps);
                return ret;                
            }
            if(accessable[curr_tree->y+1][curr_tree->x])
            {
                ret = get_path_a_star(status.pos.x, status.pos.y, status.direction, curr_tree->x, curr_tree->y+1, steps);
                return ret;                
            }
            if(accessable[curr_tree->y][curr_tree->x-1])
            {
                ret = get_path_a_star(status.pos.x, status.pos.y, status.direction, curr_tree->x-1, curr_tree->y, steps);
                return ret;                
            }
            curr_tree = curr_tree->next;
        }
    }


    // If hanging around on the sea
    if(map[status.pos.y][status.pos.x]=='~')
    {
        // Still have tree not choped yet
        if(tree_list!=NULL)
        {
            Tree curr=tree_list;
            while(curr!=NULL)
            {
                is_accessable(status.pos.x, status.pos.y, 0, 0, true);
                is_accessable_2(curr->x, curr->y, 0, 0, false);
                for(i=0; i<MAP_SIZE; ++i)
                {
                    for(j=0; j<MAP_SIZE; ++j)
                    {
                        if(accessable[i][j] && accessable_2[i][j])
                        {
                            is_accessable(status.pos.x, status.pos.y, 0, 0, false);
                            accessable[i][j] = true;
                            ret = get_path_a_star(status.pos.x, status.pos.y, status.direction, j, i, steps);
                            return ret;                         
                        }
                    }
                }
                curr = curr->next;
            }            
        }
        // Already got reasure
        if(status.treasure)
        {
            is_accessable(status.pos.x, status.pos.y, 0, 0, true);
            is_accessable_2(START_POINT, START_POINT, 0, 0, false);
            for(i=0; i<MAP_SIZE; ++i)
            {
                for(j=0; j<MAP_SIZE; ++j)
                {
                    if(accessable[i][j] && accessable_2[i][j])
                    {
                        is_accessable(status.pos.x, status.pos.y, 0, 0, false);
                        accessable[i][j] = true;
                        ret = get_path_a_star(status.pos.x, status.pos.y, status.direction, j, i, steps);
                        return ret;                         
                    }
                }
            }  
        }
        if(status.key && door_list!=NULL)
        {
            Door curr = door_list;
            is_accessable(status.pos.x, status.pos.y, 0, 0, true);
            is_accessable_2(curr->x, curr->y, 0, 0, false);
            for(i=0; i<MAP_SIZE; ++i)
            {
                for(j=0; j<MAP_SIZE; ++j)
                {
                    if(accessable[i][j] && accessable_2[i][j])
                    {
                        is_accessable(status.pos.x, status.pos.y, 0, 0, false);
                        accessable[i][j] = true;
                        ret = get_path_a_star(status.pos.x, status.pos.y, status.direction, j, i, steps);
                        return ret;                         
                    }
                }
            }  
        }
    }
    // else hang around on the land or stepping stone
    else
    {
        is_accessable(status.pos.x, status.pos.y, 0, 0, true);
        // Pass a lake by stepping stones
        if(status.num_stone>0)
        {
            is_accessable(status.pos.x, status.pos.y, 0, 0, false);
            int stone_used=1;
            while(stone_used<=status.num_stone)
            {
                for(i=status.a.y; i<=status.b.y; ++i)
                {
                    for(j=status.a.x; j<=status.b.x; ++j)
                    {
                        //is_accessable(status.pos.x, status.pos.y, 0, 0, false);

                        if(been[i][j] && map[i][j]==' ' && accessable[i][j])
                        {
                            if(map[i-1][j]!='~' && map[i+1][j]!='~' && map[i][j-1]!='~' && map[i][j+1]!='~')
                                continue;
                            if(i-stone_used-1<0 || i+stone_used+1>=MAP_SIZE
                                || j-stone_used-1<0 || j+stone_used+1>=MAP_SIZE)
                            {
                                continue;
                            }
                            for(m=i-stone_used-1; m<=i+stone_used+1; ++m)
                            {
                                for(n=j-stone_used-1; n<=j+stone_used+1; ++n)
                                {
                                    //is_accessable(status.pos.x, status.pos.y, 0, 0, false);
                                    if(heuristic(j, i, n, m)!=stone_used+1)
                                        continue;
                                    if(map[m][n]=='o' && is_accessable_lake(j,i,n,m))
                                    {
                                        //is_accessable(status.pos.x, status.pos.y, 0, 0, false);
                                        ret = get_path_a_star(status.pos.x, status.pos.y, status.direction, j, i, steps);
                                        go_on_lake=true;
                                        stoneX=n;
                                        stoneY=m;
                                        return ret;
                                    }                                                 
                                }
                            }
                        }
                    }
                }                
                stone_used++;
            }
        }
        
        else
        {
            is_accessable(status.pos.x, status.pos.y, 0, 0, true);
            // Considering using raft to pass lake
            if(status.been_to_sea && !status.treasure)
            {
                int lakeX = 0;
                int lakeY = 0;
                int landNeighbor = 0;
                int count=0;
                for(i=0; i<MAP_SIZE; ++i)
                {
                    for(j=0; j<MAP_SIZE; ++j)
                    {
                        if(map[i][j]=='~' && accessable[i][j])
                        {
                            count=0;
                            if(map[i-1][j]==' ') count++;
                            if(map[i+1][j]==' ') count++;
                            if(map[i][j-1]==' ') count++;
                            if(map[i][j+1]==' ') count++;
                            if(count>landNeighbor)
                            {
                                lakeX = j;
                                lakeY = i;
                                landNeighbor=count;
                            }
                        }
                    }
                }
                if(landNeighbor!=0) 
                {
                    is_accessable(status.pos.x, status.pos.y, 0, 0, false);
                    accessable[lakeY][lakeX]=true;
                    ret = get_path_a_star(status.pos.x, status.pos.y, status.direction, lakeX, lakeY, steps);
                    return ret;  
                }                
            }
            if(tree_list!=NULL)
            {
                int seaX = 0;
                int seaY = 0;
                int landNeighbor = 4;
                int count=0;
                for(i=0; i<MAP_SIZE; ++i)
                {
                    for(j=0; j<MAP_SIZE; ++j)
                    {
                        if(map[i][j]=='~')
                        {
                            count=0;
                            if(map[i-1][j]!='~') count++;
                            if(map[i+1][j]!='~') count++;
                            if(map[i][j-1]!='~') count++;
                            if(map[i][j+1]!='~') count++;
                            is_accessable(status.pos.x, status.pos.y, 0, 0, true);
                            is_accessable_2(tree_list->x, tree_list->y, 0, 0, true);
                            if(accessable[j][i] && accessable_2[j][i])
                            {
                                seaX = j;
                                seaY = i;
                                landNeighbor=1;
                                is_accessable(status.pos.x, status.pos.y, 0, 0, false);
                                accessable[seaY][seaX]=true;
                                ret = get_path_a_star(status.pos.x, status.pos.y, status.direction, seaX, seaY, steps);
                                status.been_to_sea = true;
                                return ret;  
                            }
                        }
                    }
                }
                if(landNeighbor!=4) 
                {
                    is_accessable(status.pos.x, status.pos.y, 0, 0, false);
                    accessable[seaY][seaX]=true;
                    ret = get_path_a_star(status.pos.x, status.pos.y, status.direction, seaX, seaY, steps);
                    status.been_to_sea = true;
                    return ret;  
                }                   
            }
            int seaX = 0;
            int seaY = 0;
            int landNeighbor = 4;
            int count=0;
            for(i=0; i<MAP_SIZE; ++i)
            {
                for(j=0; j<MAP_SIZE; ++j)
                {
                    if(map[i][j]=='~' && accessable[i][j])
                    {
                        count=0;
                        if(map[i-1][j]!='~') count++;
                        if(map[i+1][j]!='~') count++;
                        if(map[i][j-1]!='~') count++;
                        if(map[i][j+1]!='~') count++;
                        if(count<landNeighbor)
                        {
                            seaX = j;
                            seaY = i;
                            landNeighbor=count;
                        }
                    }
                }
            }
            if(landNeighbor!=4) 
            {
                is_accessable(status.pos.x, status.pos.y, 0, 0, false);
                accessable[seaY][seaX]=true;
                ret = get_path_a_star(status.pos.x, status.pos.y, status.direction, seaX, seaY, steps);
                status.been_to_sea = true;
                return ret;  
            }            
        }        
    }

    if(map[status.pos.y][status.pos.x]!='~' && status.treasure==false)
    {
        if(status.num_stone>0)
        {
            is_accessable(status.pos.x, status.pos.y, 0, 0, false);
            int stone_used=1;
            while(stone_used<=status.num_stone)
            {
                for(i=status.a.y; i<=status.b.y; ++i)
                {
                    for(j=status.a.x; j<=status.b.x; ++j)
                    {
                        //is_accessable(status.pos.x, status.pos.y, 0, 0, false);

                        if(been[i][j] && (map[i][j]==' '|| map[i][j]=='O')&& accessable[i][j])
                        {
                            if(map[i-1][j]!='~' && map[i+1][j]!='~' && map[i][j-1]!='~' && map[i][j+1]!='~')
                                continue;
                            if(i-stone_used-1<0 || i+stone_used+1>=MAP_SIZE
                                || j-stone_used-1<0 || j+stone_used+1>=MAP_SIZE)
                            {
                                continue;
                            }
                            for(m=i-stone_used-1; m<=i+stone_used+1; ++m)
                            {
                                for(n=j-stone_used-1; n<=j+stone_used+1; ++n)
                                {
                                    //is_accessable(status.pos.x, status.pos.y, 0, 0, false);
                                    if(heuristic(j, i, n, m)!=stone_used+1)
                                        continue;
                                    if(map[m][n]=='$' && is_accessable_lake(j,i,n,m))
                                    {
                                        //is_accessable(status.pos.x, status.pos.y, 0, 0, false);
                                        ret = get_path_a_star(status.pos.x, status.pos.y, status.direction, j, i, steps);
                                        go_on_lake=true;
                                        stoneX=n;
                                        stoneY=m;
                                        return ret;
                                    }                                                 
                                }
                            }
                        }
                    }
                }                
                stone_used++;
            }            
        }
    }
    if(map[status.pos.y][status.pos.x]!='~' && status.num_stone>0)
    {
        is_accessable(status.pos.x, status.pos.y, 0, 0, true);
        int lakeX = 0;
        int lakeY = 0;
        int landNeighbor = 0;
        int count=0;
        for(i=0; i<MAP_SIZE; ++i)
        {
            for(j=0; j<MAP_SIZE; ++j)
            {
                if(map[i][j]=='~' && accessable[i][j])
                {
                    count=0;
                    if(map[i-1][j]==' ') count++;
                    if(map[i+1][j]==' ') count++;
                    if(map[i][j-1]==' ') count++;
                    if(map[i][j+1]==' ') count++;
                    if(count>landNeighbor)
                    {
                        lakeX = j;
                        lakeY = i;
                        landNeighbor=count;
                    }
                }
            }
        }
        if(landNeighbor!=0) 
        {
            is_accessable(status.pos.x, status.pos.y, 0, 0, false);
            accessable[lakeY][lakeX]=true;
            ret = get_path_a_star(status.pos.x, status.pos.y, status.direction, lakeX, lakeY, steps);
            return ret;  
        }
    }

    return 0;  
}

bool is_accessable(int srcX, int srcY, int destX, int destY, bool transfer)
{
    int i,j;
    for (i = 0; i < MAP_SIZE; ++i)
    {
        for(j=0; j < MAP_SIZE; ++j)
        {
            accessable[i][j]=false;
        }
    }

    if(map[srcY][srcX]=='~')
    {
        accessable[srcY][srcX] = true;
        recursive_mark_access(srcX, srcY, '~');
        if(transfer)
        {
            for(i=0; i<MAP_SIZE; ++i)
            {
                for(j=0; j<MAP_SIZE; ++j)
                {
                    if(map[i][j]!='*' && map[i][j]!='~' && map[i][j]!='T'
                        && (   (map[i-1][j]=='~' && accessable[i-1][j]) 
                            || (map[i+1][j]=='~' && accessable[i+1][j]) 
                            || (map[i][j-1]=='~' && accessable[i][j-1]) 
                            || (map[i][j+1]=='~' && accessable[i][j+1]) ) )
                    {
                        accessable[i][j]=true;
                    }
                }
            }
        }  
    }
    else
    {        
        accessable[srcY][srcX] = true;
        recursive_mark_access(srcX, srcY, ' ');
        if(transfer)
        {
            for(i=0; i<MAP_SIZE; ++i)
            {
                for(j=0; j<MAP_SIZE; ++j)
                {
                    if(map[i][j]=='~' 
                        && (   (map[i-1][j]!='~' && accessable[i-1][j]) 
                            || (map[i+1][j]!='~' && accessable[i+1][j]) 
                            || (map[i][j-1]!='~' && accessable[i][j-1]) 
                            || (map[i][j+1]!='~' && accessable[i][j+1]) ) )
                    {
                        accessable[i][j]=true;
                    }
                }
            }
        }  
    }

    return accessable[destY][destX];
}

void recursive_mark_access(int x, int y, char type)
{
    if(type=='~')
    {
        if(seen[y-1][x] && map[y-1][x]=='~' && accessable[y-1][x] == false)
        {
            accessable[y-1][x] = true;
            recursive_mark_access(x, y-1, type);
        }

        if(seen[y+1][x] && map[y+1][x]=='~' && accessable[y+1][x] == false)
        {
            accessable[y+1][x] = true;
            recursive_mark_access(x, y+1, type);
        }

        if(seen[y][x-1] && map[y][x-1]=='~' && accessable[y][x-1] == false)
        {
            accessable[y][x-1] = true;
            recursive_mark_access(x-1, y, type);
        }

        if(seen[y][x+1] && map[y][x+1]=='~' && accessable[y][x+1] == false)
        {
            accessable[y][x+1] = true;
            recursive_mark_access(x+1, y, type);
        }
    }
    else
    {
        if(seen[y-1][x] && map[y-1][x]!='~' && map[y-1][x]!='*' && map[y-1][x]!='T' && map[y-1][x]!='-' && accessable[y-1][x] == false)
        {
            accessable[y-1][x] = true;
            recursive_mark_access(x, y-1, type);
        }

        if(seen[y+1][x] && map[y+1][x]!='~' && map[y+1][x]!='*' && map[y+1][x]!='T' && map[y+1][x]!='-' && accessable[y+1][x] == false)
        {
            accessable[y+1][x] = true;
            recursive_mark_access(x, y+1, type);
        }

        if(seen[y][x-1] && map[y][x-1]!='~' && map[y][x-1]!='*' && map[y][x-1]!='T' && map[y][x-1]!='-' && accessable[y][x-1] == false)
        {
            accessable[y][x-1] = true;
            recursive_mark_access(x-1, y, type);
        }

        if(seen[y][x+1] && map[y][x+1]!='~' && map[y][x+1]!='*' && map[y][x+1]!='T' && map[y][x+1]!='-' && accessable[y][x+1] == false)
        {
            accessable[y][x+1] = true;
            recursive_mark_access(x+1, y, type);
        }
    }
}

bool is_accessable_2(int srcX, int srcY, int destX, int destY, bool transfer)
{
    int i,j;
    for (i = 0; i < MAP_SIZE; ++i)
    {
        for(j=0; j < MAP_SIZE; ++j)
        {
            accessable_2[i][j]=false;
        }
    }

    if(map[srcY][srcX]=='~')
    {
        accessable_2[srcY][srcX] = true;
        recursive_mark_access_2(srcX, srcY, '~');
        if(transfer)
        {
            for(i=0; i<MAP_SIZE; ++i)
            {
                for(j=0; j<MAP_SIZE; ++j)
                {
                    if(map[i][j]!='*' && map[i][j]!='~' && map[i][j]!='T'
                        && (   (map[i-1][j]=='~' && accessable_2[i-1][j]) 
                            || (map[i+1][j]=='~' && accessable_2[i+1][j]) 
                            || (map[i][j-1]=='~' && accessable_2[i][j-1]) 
                            || (map[i][j+1]=='~' && accessable_2[i][j+1]) ) )
                    {
                        accessable_2[i][j]=true;
                    }
                }
            }
        }  
    }
    else
    {        
        accessable_2[srcY][srcX] = true;
        recursive_mark_access_2(srcX, srcY, ' ');
        if(transfer)
        {
            for(i=0; i<MAP_SIZE; ++i)
            {
                for(j=0; j<MAP_SIZE; ++j)
                {
                    if(map[i][j]=='~' 
                        && (   (map[i-1][j]!='~' && accessable_2[i-1][j]) 
                            || (map[i+1][j]!='~' && accessable_2[i+1][j]) 
                            || (map[i][j-1]!='~' && accessable_2[i][j-1]) 
                            || (map[i][j+1]!='~' && accessable_2[i][j+1]) ) )
                    {
                        accessable_2[i][j]=true;
                    }
                }
            }
        }  
    }

    return accessable_2[destY][destX];
}

void recursive_mark_access_2(int x, int y, char type)
{
    if(type=='~')
    {
        if(seen[y-1][x] && map[y-1][x]=='~' && accessable_2[y-1][x] == false)
        {
            accessable_2[y-1][x] = true;
            recursive_mark_access_2(x, y-1, type);
        }

        if(seen[y+1][x] && map[y+1][x]=='~' && accessable_2[y+1][x] == false)
        {
            accessable_2[y+1][x] = true;
            recursive_mark_access_2(x, y+1, type);
        }

        if(seen[y][x-1] && map[y][x-1]=='~' && accessable_2[y][x-1] == false)
        {
            accessable_2[y][x-1] = true;
            recursive_mark_access_2(x-1, y, type);
        }

        if(seen[y][x+1] && map[y][x+1]=='~' && accessable_2[y][x+1] == false)
        {
            accessable_2[y][x+1] = true;
            recursive_mark_access_2(x+1, y, type);
        }
    }
    else
    {
        if(seen[y-1][x] && map[y-1][x]!='~' && map[y-1][x]!='*' && map[y-1][x]!='T' && map[y-1][x]!='-' && accessable_2[y-1][x] == false)
        {
            accessable_2[y-1][x] = true;
            recursive_mark_access_2(x, y-1, type);
        }

        if(seen[y+1][x] && map[y+1][x]!='~' && map[y+1][x]!='*' && map[y+1][x]!='T' && map[y+1][x]!='-' && accessable_2[y+1][x] == false)
        {
            accessable_2[y+1][x] = true;
            recursive_mark_access_2(x, y+1, type);
        }

        if(seen[y][x-1] && map[y][x-1]!='~' && map[y][x-1]!='*' && map[y][x-1]!='T' && map[y][x-1]!='-' && accessable_2[y][x-1] == false)
        {
            accessable_2[y][x-1] = true;
            recursive_mark_access_2(x-1, y, type);
        }

        if(seen[y][x+1] && map[y][x+1]!='~' && map[y][x+1]!='*' && map[y][x+1]!='T' && map[y][x+1]!='-' && accessable_2[y][x+1] == false)
        {
            accessable_2[y][x+1] = true;
            recursive_mark_access_2(x+1, y, type);
        }
    }
}

int open_door(char* steps)
{
    if(status.direction==NORTH)
    {
        if(map[status.pos.y-1][status.pos.x]=='-')
        {
            steps[0] = 'u';
            return 1;
        }
        else if(map[status.pos.y][status.pos.x+1]=='-')
        {
            steps[0] = 'r';
            steps[1] = 'u';
            return 2;
        }
        else if(map[status.pos.y+1][status.pos.x]=='-')
        {
            steps[0] = 'r';
            steps[1] = 'r';
            steps[2] = 'u';
            return 3;
        }
        else if(map[status.pos.y][status.pos.x-1]=='-') 
        {
            steps[0] = 'l';
            steps[1] = 'u';
            return 2;
        }
    }
    else if(status.direction==EAST)
    {
        if(map[status.pos.y-1][status.pos.x]=='-')
        {
            steps[0] = 'l';
            steps[1] = 'u';
            return 2;
        }
        else if(map[status.pos.y][status.pos.x+1]=='-')
        {
            steps[0] = 'u';
            return 1;
        }
        else if(map[status.pos.y+1][status.pos.x]=='-')
        {
            steps[0] = 'r';
            steps[1] = 'u';
            return 2;
        }
        else if(map[status.pos.y][status.pos.x-1]=='-') 
        {
            steps[0] = 'r';
            steps[1] = 'r';
            steps[2] = 'u';
            return 3;           
        }
    }
    else if(status.direction==SOUTH)
    {
        if(map[status.pos.y-1][status.pos.x]=='-')
        {
            steps[0] = 'r';
            steps[1] = 'r';
            steps[2] = 'u';
            return 3;             
        }
        else if(map[status.pos.y][status.pos.x+1]=='-')
        {
            steps[0] = 'l';
            steps[1] = 'u';
            return 2;
        }
        else if(map[status.pos.y+1][status.pos.x]=='-')
        {
            steps[0] = 'u';
            return 1;
        }
        else if(map[status.pos.y][status.pos.x-1]=='-') 
        {
            steps[0] = 'r';
            steps[1] = 'u';
            return 2;            
        }
    }
    else if(status.direction==WEST)
    {
        if(map[status.pos.y-1][status.pos.x]=='-')
        {
            steps[0] = 'r';
            steps[1] = 'u';
            return 2;             
        }
        else if(map[status.pos.y][status.pos.x+1]=='-')
        {
            steps[0] = 'r';
            steps[1] = 'r';
            steps[2] = 'u';
            return 3; 
        }
        else if(map[status.pos.y+1][status.pos.x]=='-')
        {
            steps[0] = 'l';
            steps[1] = 'u';
            return 2;
        }
        else if(map[status.pos.y][status.pos.x-1]=='-') 
        {
            steps[0] = 'u';
            return 1;            
        }
    }
    return 0;
}

int chop_tree(char* steps)
{
    if(status.direction==NORTH)
    {
        if(map[status.pos.y-1][status.pos.x]=='T')
        {
            steps[0] = 'c';
            return 1;
        }
        else if(map[status.pos.y][status.pos.x+1]=='T')
        {
            steps[0] = 'r';
            steps[1] = 'c';
            return 2;
        }
        else if(map[status.pos.y+1][status.pos.x]=='T')
        {
            steps[0] = 'r';
            steps[1] = 'r';
            steps[2] = 'c';
            return 3;
        }
        else if(map[status.pos.y][status.pos.x-1]=='T') 
        {
            steps[0] = 'l';
            steps[1] = 'c';
            return 2;
        }
    }
    else if(status.direction==EAST)
    {
        if(map[status.pos.y-1][status.pos.x]=='T')
        {
            steps[0] = 'l';
            steps[1] = 'c';
            return 2;
        }
        else if(map[status.pos.y][status.pos.x+1]=='T')
        {
            steps[0] = 'c';
            return 1;
        }
        else if(map[status.pos.y+1][status.pos.x]=='T')
        {
            steps[0] = 'r';
            steps[1] = 'c';
            return 2;
        }
        else if(map[status.pos.y][status.pos.x-1]=='T') 
        {
            steps[0] = 'r';
            steps[1] = 'r';
            steps[2] = 'c';
            return 3;           
        }
    }
    else if(status.direction==SOUTH)
    {
        if(map[status.pos.y-1][status.pos.x]=='T')
        {
            steps[0] = 'r';
            steps[1] = 'r';
            steps[2] = 'c';
            return 3;             
        }
        else if(map[status.pos.y][status.pos.x+1]=='T')
        {
            steps[0] = 'l';
            steps[1] = 'c';
            return 2;
        }
        else if(map[status.pos.y+1][status.pos.x]=='T')
        {
            steps[0] = 'c';
            return 1;
        }
        else if(map[status.pos.y][status.pos.x-1]=='T') 
        {
            steps[0] = 'r';
            steps[1] = 'c';
            return 2;            
        }
    }
    else if(status.direction==WEST)
    {
        if(map[status.pos.y-1][status.pos.x]=='T')
        {
            steps[0] = 'r';
            steps[1] = 'c';
            return 2;             
        }
        else if(map[status.pos.y][status.pos.x+1]=='T')
        {
            steps[0] = 'r';
            steps[1] = 'r';
            steps[2] = 'c';
            return 3; 
        }
        else if(map[status.pos.y+1][status.pos.x]=='T')
        {
            steps[0] = 'l';
            steps[1] = 'c';
            return 2;
        }
        else if(map[status.pos.y][status.pos.x-1]=='T') 
        {
            steps[0] = 'c';
            return 1;            
        }
    }
    return 0;    
}

// Get a step from src to dest, using A* search
int get_path_a_star(int srcX, int srcY, int direction, int destX, int destY, char* path)
{
    int i,j,k= 0;

    int analysed[MAP_SIZE][MAP_SIZE][4];
    for(i=0; i<MAP_SIZE; ++i)
    {
        for(j=0; j<MAP_SIZE; ++j)
        {
            for(k=0; k<4; ++k)
            {
                analysed[i][j][k]=0;
            }
        }
    }

    AstarNodeHead queue = malloc(sizeof(struct astarNode));
    queue->num = 0;
    queue->list = NULL;

    AstarNode head = astarNode_create(srcX, srcY, direction, 0, heuristic(srcX,srcY,destX,destY), (char)0, "nothing");

    astarNode_insert(queue, head);

    int ret = 100000;

    AstarNode stored=NULL;

    while(queue->list!=NULL)
    {
        AstarNode temp = queue->list;
        queue->list = temp->next;
        queue->num--;
        analysed[temp->y][temp->x][temp->direction] = 1;


        if(temp->direction==NORTH)
        {
            if(accessable[temp->y-1][temp->x] && !analysed[temp->y-1][temp->x][NORTH])
            {
                AstarNode new = astarNode_create(temp->x, temp->y-1, NORTH, temp->g+1, 
                        heuristic(temp->x, temp->y-1, destX, destY), 'f', temp->steps);
                astarNode_insert(queue, new);
            }
            if(!analysed[temp->y][temp->x][EAST])
            {
                AstarNode new = astarNode_create(temp->x, temp->y, EAST, temp->g+1, 
                        temp->h, 'r', temp->steps);
                astarNode_insert(queue, new);
            }
            if(!analysed[temp->y][temp->x][WEST])
            {
                AstarNode new = astarNode_create(temp->x, temp->y, WEST, temp->g+1, 
                        temp->h, 'l', temp->steps);
                astarNode_insert(queue, new);
            }
        }
        else if(temp->direction==EAST)
        {
            if(accessable[temp->y][temp->x+1] && !analysed[temp->y][temp->x+1][EAST])
            {
                AstarNode new = astarNode_create(temp->x+1, temp->y, EAST, temp->g+1, 
                        heuristic(temp->x+1, temp->y, destX, destY), 'f', temp->steps);
                astarNode_insert(queue, new);
            }
            if(!analysed[temp->y][temp->x][SOUTH])
            {
                AstarNode new = astarNode_create(temp->x, temp->y, SOUTH, temp->g+1, 
                        temp->h, 'r', temp->steps);
                astarNode_insert(queue, new);
            }
            if(!analysed[temp->y][temp->x][NORTH])
            {
                AstarNode new = astarNode_create(temp->x, temp->y, NORTH, temp->g+1, 
                        temp->h, 'l', temp->steps);
                astarNode_insert(queue, new);
            }            
        }
        else if(temp->direction==SOUTH)
        {
            if(accessable[temp->y+1][temp->x] && !analysed[temp->y+1][temp->x][SOUTH])
            {
                AstarNode new = astarNode_create(temp->x, temp->y+1, SOUTH, temp->g+1, 
                        heuristic(temp->x, temp->y+1, destX, destY), 'f', temp->steps);
                astarNode_insert(queue, new);
            }
            if(!analysed[temp->y][temp->x][WEST])
            {
                AstarNode new = astarNode_create(temp->x, temp->y, WEST, temp->g+1, 
                        temp->h, 'r', temp->steps);
                astarNode_insert(queue, new);
            }
            if(!analysed[temp->y][temp->x][EAST])
            {
                AstarNode new = astarNode_create(temp->x, temp->y, EAST, temp->g+1, 
                        temp->h, 'l', temp->steps);
                astarNode_insert(queue, new);
            }             
        }
        else if(temp->direction==WEST)
        {
            if(accessable[temp->y][temp->x-1] && !analysed[temp->y][temp->x-1][WEST])
            {
                AstarNode new = astarNode_create(temp->x-1, temp->y, WEST, temp->g+1, 
                        heuristic(temp->x-1, temp->y, destX, destY), 'f', temp->steps);
                astarNode_insert(queue, new);
            }
            if(!analysed[temp->y][temp->x][NORTH])
            {
                AstarNode new = astarNode_create(temp->x, temp->y, NORTH, temp->g+1, 
                        temp->h, 'r', temp->steps);
                astarNode_insert(queue, new);
            }
            if(!analysed[temp->y][temp->x][SOUTH])
            {
                AstarNode new = astarNode_create(temp->x, temp->y, SOUTH, temp->g+1, 
                        temp->h, 'l', temp->steps);
                astarNode_insert(queue, new);
            }              
        }

        if(temp->f > ret) 
        {
            free(temp);
            break;
        }
        
        if(temp->x == destX && temp->y == destY)
        {
            assert(temp->h==0);
            if(temp->f < ret)
            {
                if(stored!=NULL) free(stored);
                stored = temp;
                ret = temp->f;
                continue;
            }
        }
        free(temp);
    }

    AstarNode curr = queue->list;
    AstarNode prev = queue->list;
    if(curr!=NULL)
    {
        while(curr->next!=NULL)
        {
            prev=curr;
            curr=curr->next;
            free(prev);
        }
        free(curr);
    }
    free(queue);

    for(i=0; i<stored->f; ++i)
    {
        path[i] = stored->steps[i];
    }

    free(stored);
    return ret;
}

// Heuristic function of two points on map
int heuristic(int srcX, int srcY, int destX, int destY)
{
    return abs(srcX-destX)+abs(srcY-destY);
}

bool is_accessable_lake(int srcX, int srcY, int destX, int destY)
{
    bool temp[2*abs(srcY-destY)+1][2*abs(srcX-destX)+1];
    int i, j;
    for (i =0; i < 2*abs(srcY-destY)+1; ++i)
    {
        for(j=0; j < 2*abs(srcX-destX)+1; ++j)
        {
            temp[i][j]=false;
        }
    }    

    bool changing = true;
    temp[abs(srcY-destY)][abs(srcX-destX)] = true;
    while(changing)
    {
        changing=false;
        for (i =0; i < 2*abs(srcY-destY)+1; ++i)
        {
            for(j=0; j < 2*abs(srcX-destX)+1; ++j)
            {
                if((map[i-abs(srcY-destY)+srcY][j-abs(srcX-destX)+srcX]=='~'
                    || map[i-abs(srcY-destY)+srcY][j-abs(srcX-destX)+srcX]=='o'
                    || map[i-abs(srcY-destY)+srcY][j-abs(srcX-destX)+srcX]=='O'
                    || map[i-abs(srcY-destY)+srcY][j-abs(srcX-destX)+srcX]=='$') && temp[i][j]==false)
                {
                    if( (i-1>=0 && temp[i-1][j]==true) 
                        || (i+1<2*abs(srcY-destY)+1 && temp[i+1][j]==true) 
                        || (j-1>0 && temp[i][j-1]==true) 
                        || (j+1<2*abs(srcX-destX)+1 && temp[i][j+1]==true) )
                    {
                        temp[i][j]=true;
                        changing=true;
                    }
                }
            }
        }
    }
    return temp[destY-srcY+abs(srcY-destY)][destX-srcX+abs(srcX-destX)];
}

// Get action to send to server
char interpret_action(char* steps, int curr_step) {

    // REPLACE THIS CODE WITH AI TO CHOOSE ACTION

    char ch= *(steps+curr_step);

    if(ch=='F' || ch=='f')
    {
        if(status.direction==NORTH)
        {
            --status.pos.y;
        }
        if(status.direction==EAST)
        {
            ++status.pos.x;
        }
        if(status.direction==SOUTH)
        {
            ++status.pos.y;
        }
        if(status.direction==WEST)
        {
            --status.pos.x;
        }
        if(map[status.pos.y][status.pos.x]=='~')
        {
            if(status.num_stone>0) 
            {
                status.num_stone--;
                map[status.pos.y][status.pos.x]='O';
            }
            else status.raft=false;
        }
        return ch;
    }
    else if(ch=='L' || ch=='l')
    {
        --status.direction;
        if(status.direction<NORTH)
            status.direction=WEST;

        return ch;
    }
    else if(ch=='R' || ch=='r')
    {
        ++status.direction;
        if(status.direction>WEST)
            status.direction=NORTH; 

        return ch;      
    }
    else if(ch=='C' || ch=='c')
    {
        if(status.direction==NORTH) deleteTree(status.pos.x, status.pos.y-1);
        else if(status.direction==EAST) deleteTree(status.pos.x+1, status.pos.y);
        else if(status.direction==SOUTH) deleteTree(status.pos.x, status.pos.y+1);
        else if(status.direction==WEST) deleteTree(status.pos.x-1, status.pos.y);
        status.raft=true;
        return ch;
    }
    else if(ch=='U' || ch=='u')
    {
        if(status.direction==NORTH) deleteDoor(status.pos.x, status.pos.y-1);
        else if(status.direction==EAST) deleteDoor(status.pos.x+1, status.pos.y);
        else if(status.direction==SOUTH) deleteDoor(status.pos.x, status.pos.y+1);
        else if(status.direction==WEST) deleteDoor(status.pos.x-1, status.pos.y);
        return ch;
    }
    else if(ch=='B' || ch=='b')
    {
        return ch;
    }
    else if(ch=='p')
    {
        return ch;
    }
  
    return 0;
}


// Help function*******************************************************************************************************
// Print the map that have been seen
void print_map()
{
    int i,j;
    for(i=status.a.y; i < status.b.y+1; i++ ) 
    {
        for(j=status.a.x; j < status.b.x+1; j++ ) 
        {
            putchar( map[i][j] );
        }
        printf("\n");
    }
}




void print_tool_list(void)
{
    Tool curr = tool_list;
    while(curr!=NULL)
    {
        printf("Tool %c at map[%d][%d]\n", curr->type, curr->y, curr->x);
        curr=curr->next;
    }
}



void print_accessible(void)
{
    int i,j;
    for(i=status.a.y; i < status.b.y+1; i++ ) 
    {
        for(j=status.a.x; j < status.b.x+1; j++ ) 
        {
            if(accessable[i][j]) putchar('+');
            else putchar('-');
        }
        printf("\n");
    }
}

void print_accessible_2(void)
{
    int i,j;
    for(i=status.a.y; i < status.b.y+1; i++ ) 
    {
        for(j=status.a.x; j < status.b.x+1; j++ ) 
        {
            if(accessable_2[i][j]) putchar('+');
            else putchar('-');
        }
        printf("\n");
    }
}