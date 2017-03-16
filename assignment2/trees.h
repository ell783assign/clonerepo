#ifndef ASSIGNMENT_2_TREES_H_
#define ASSIGNMENT_2_TREES_H_
typedef int32_t(COMPARE)(void *, void*);

typedef struct bst
{
	struct bst_node *root;
	COMPARE *compare;
	uint32_t key_offset;

	struct bst_node *first;
}BST;

typedef struct bst_node
{
	struct bst_node *parent;
	struct bst_node *left;
	struct bst_node *right;

	void *content;
	/* for red-black tree, we add color */
	enum coloring {RED, BLACK} color;
}BST_NODE;

#define BST_TREE_INIT(TREE, FUNC, OFFSET)		\
	(TREE).root = NULL;							\
	(TREE).compare = FUNC;						\
	(TREE).key_offset = OFFSET;

#define BST_INIT_NODE(NODE, SELF) 				\
	(NODE).content = SELF;						\
	(NODE).parent = NULL;						\
	(NODE).left = NULL;							\
	(NODE).right = NULL;						\

void * bst_insert(BST_NODE *, BST *);
void * bst_find(void *, BST *);
void * bst_next(BST_NODE *, BST *);
void * bst_first(BST);
void bst_delete(BST_NODE *, BST *);
void bst_rotate_left(BST_NODE *, BST *);
void bst_rotate_right(BST_NODE *, BST *);
void red_black_insert(BST_NODE *, BST *);
void red_black_delete(BST_NODE *, BST *);

int32_t compare_int(void *, void *); 

#endif