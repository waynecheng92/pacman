#ifndef __AI__
#define __AI__

#include <stdint.h>
#include <unistd.h>
#include "node.h"
#include "priority_queue.h"


void initialize_ai();

move_t get_next_move( state_t init_state, int budget, propagation_t propagation, char* stats, int* totalGenerated, int* totalExpanded, int* maxDepth);
void copy_state(state_t* dst, state_t* src);
node_t* create_init_node( state_t* init_state );
float heuristic( node_t* n );
float get_reward ( node_t* n );
bool applyAction(node_t* n, node_t** new_node, move_t action );
void free_explored(node_t** explored, int order);
void propagateBackScore(node_t* new_node, propagation_t propagation, float best_action_score[]);
void random_best_action(node_t** explored, float best_action_score[], move_t* best_action, propagation_t propagation, int expanded_nodes);
node_t* get_first_move(node_t* new_node);
#endif
