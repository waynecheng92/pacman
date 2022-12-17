#include <time.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <assert.h>


#include "ai.h"
#include "utils.h"
#include "priority_queue.h"


struct heap h;

float get_reward( node_t* n );

/**
 * Function called by pacman.c
*/
void initialize_ai(){
	heap_init(&h);
}

/**
 * Function to copy a src into a dst state
*/
void copy_state(state_t* dst, state_t* src){
	//Location of Ghosts and Pacman
	memcpy( dst->Loc, src->Loc, 5*2*sizeof(int) );

    //Direction of Ghosts and Pacman
	memcpy( dst->Dir, src->Dir, 5*2*sizeof(int) );

    //Default location in case Pacman/Ghosts die
	memcpy( dst->StartingPoints, src->StartingPoints, 5*2*sizeof(int) );

    //Check for invincibility
    dst->Invincible = src->Invincible;
    
    //Number of pellets left in level
    dst->Food = src->Food;
    
    //Main level array
	memcpy( dst->Level, src->Level, 29*28*sizeof(int) );

    //What level number are we on?
    dst->LevelNumber = src->LevelNumber;
    
    //Keep track of how many points to give for eating ghosts
    dst->GhostsInARow = src->GhostsInARow;

    //How long left for invincibility
    dst->tleft = src->tleft;

    //Initial points
    dst->Points = src->Points;

    //Remiaining Lives
    dst->Lives = src->Lives;   

}

/**
 * Function to create a new node for the current location
*/
node_t* create_init_node( state_t* init_state ){
	node_t * new_n = (node_t *) malloc(sizeof(node_t));
	assert(new_n);
	new_n->parent = NULL;	
	new_n->priority = 0;
	new_n->depth = 0;
	new_n->num_childs = 0;
	copy_state(&(new_n->state), init_state);
	new_n->acc_reward = get_reward( new_n );
	return new_n;
}

/**
 * Function to calculate the heuristic value
*/
float heuristic( node_t* n ){

	float h = 0;
	int i = 0, l = 0, g = 0;

	// Assign i, l, g to the specific values if they match the condition
	if (n->state.Level[n->state.Loc[4][0]][n->state.Loc[4][1]] == 3) {
		i = 10; 
	}
	if (n->parent->state.Lives > n->state.Lives) {
		l = 10;
	}
	if (n->state.Lives < 0) {
		g = 100;
	}

	h = i - l - g;

	return h;
}

/**
 * Function to calculate the estimated reward with the heuristic value
*/
float get_reward ( node_t* n ){

	float reward = 0;
	float discount = pow(0.99,n->depth);

	if (n->parent != NULL) {
		reward = heuristic(n) + n->state.Points - n->parent->state.Points;
	}
   	
	return discount * reward;
}

/**
 * Fill in the details of new node and return whether it is applicable
*/
bool applyAction(node_t* n, node_t** new_node, move_t action ){

	bool changed_dir = false;

	(*new_node)->parent = n;
	copy_state(&((*new_node)->state), &(n->state));
    changed_dir = execute_move_t( &((*new_node)->state), action);
    (*new_node)->move = action;
    (*new_node)->depth = n->depth + 1;
    (*new_node)->priority = -(*new_node)->depth;
    (*new_node)->acc_reward = n->acc_reward + get_reward(*new_node);
    (*new_node)->num_childs = 0;

	return changed_dir;

}


/**
 * Find best action by building all possible paths up to budget
 * and back propagate using either max or avg
 */
move_t get_next_move( state_t init_state, int budget, propagation_t propagation, char* stats, int* totalGenerated, int* totalExpanded, int* maxDepth){
	
	move_t best_action = rand() % 4;
	float best_action_score[4];

	// Initialize each value representing each direction in best action score
	for(int i = 0; i < 4; i++) {
	    best_action_score[i] = INT_MIN;
	}

	unsigned generated_nodes = 0;
	unsigned expanded_nodes = 0;
	unsigned max_depth = 0;
	
	//Add the initial node
	node_t* n = create_init_node( &init_state );
	
	//Use the max heap API provided in priority_queue.h
	heap_push(&h,n);
	
	// Create new node according to the node in "explored" and put it into the priority queue if it's applicable,
	// then pop the node out and put it into the array "explored" based on the priority, and repeat the steps unitl
	// no node is in the priority queue
	node_t** explored = (node_t**)malloc(sizeof(node_t*) * (budget + 1));
	assert(explored);

	while(h.count != 0) {
		explored[expanded_nodes++] = heap_delete(&h);
		if (expanded_nodes < budget + 1) {
			for (int i = 0 ; i < initial_size ; i++) {
				node_t* new_node = (node_t*)malloc(sizeof(node_t));
				assert(new_node);
				if (applyAction(explored[expanded_nodes-1], &new_node, i)) {
					propagateBackScore(new_node, propagation, best_action_score);
					// Delete the node if it can get us lose life
					if (new_node->state.Lives < new_node->parent->state.Lives) {
						free(new_node);
					} else {
						generated_nodes++;
						heap_push(&h, new_node);
						if (new_node->depth > max_depth) {
							max_depth = new_node->depth;
						}
					}
				} else {
					free(new_node);
				}
			}
		} else {
			break;
		}
	}

	random_best_action(explored, best_action_score, &best_action, propagation, expanded_nodes);

	free_explored(explored, expanded_nodes);
	emptyPQ(&h);

	// Update the max and the accumulative data for the entire game
	if (max_depth > *maxDepth) {
		*maxDepth = max_depth;
	}
	*totalGenerated += generated_nodes;
	*totalExpanded += expanded_nodes;

	sprintf(stats, "Max Depth: %d Expanded nodes: %d  Generated nodes: %d\n",max_depth,expanded_nodes,generated_nodes);
	
	if(best_action == left)
		sprintf(stats, "%sSelected action: Left\n",stats);
	if(best_action == right)
		sprintf(stats, "%sSelected action: Right\n",stats);
	if(best_action == up)
		sprintf(stats, "%sSelected action: Up\n",stats);
	if(best_action == down)
		sprintf(stats, "%sSelected action: Down\n",stats);

	sprintf(stats, "%sScore Left %f Right %f Up %f Down %f",stats,best_action_score[left],best_action_score[right],best_action_score[up],best_action_score[down]);

	return best_action;
}

/**
 * To free everything in "explored" including itself
 */
void free_explored(node_t** explored, int num) {

	for(int i = 0 ; i < num ; i++) {
		free(explored[i]);
	}

	free(explored);
}

/**
 * To update best action score and number of child in terms of the new node and the type of propagation
 */
void propagateBackScore(node_t* new_node, propagation_t propagation, float best_action_score[]) {

	node_t* node = get_first_move(new_node);

	if (propagation == max) {
		if (best_action_score[node->move] < new_node->acc_reward) {
			best_action_score[node->move] = new_node->acc_reward;
		}

	} else if (propagation == avg) {
		if (new_node == node) {
			best_action_score[new_node->move] = new_node->acc_reward;
		} else {
			best_action_score[node->move] += new_node->acc_reward;
			node->num_childs++;
		}
	}
}

/**
 * Choose the best action based on the type of propagation and the direction randomly if have same score 
 */
void random_best_action(node_t** explored, float best_action_score[], move_t* best_action, propagation_t propagation, int expanded_nodes) {

	float max_score = INT_MIN;

	if (propagation == max) {
		// Update the max score in terms of the best score action
		for (int i = 0 ; i < initial_size ; i++) {
			if (best_action_score[i] > max_score) {
				max_score = best_action_score[i];
			}
		}
	}
	
	if (propagation == avg) {
		// Update the best score action by dividing the number of child of the corresponding direction
		// and update the max score in terms of the updated best score action
		for (int i = 1 ; i < expanded_nodes ; i++) {
			if (explored[i]->depth == 1) {
				best_action_score[explored[i]->move] /= (explored[i]->num_childs + 1);
				if (best_action_score[explored[i]->move] > max_score) {
					max_score = best_action_score[explored[i]->move];
				}
			} else {
				break;
			}
		}
	}

	// Randomly choose the direction that matches with the max score
	while(best_action_score[*best_action] != max_score) {
		*best_action = rand() % 4;
	}
}

/**
 * Return the node that represents the first move where the new node is generated from
 */
node_t* get_first_move(node_t* new_node) {

	node_t* node = new_node;

	if (new_node->parent->parent != NULL) {
		node = get_first_move(new_node->parent);
	}

	return node;
}
