#include <include.h>

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
	(TREE).offset = OFFSET;

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

void * bst_insert(BST_NODE *node, BST *tree)
{
	BST_NODE *inserted = NULL;
	BST_NODE *penultimate = NULL;
	int32_t comparison_result;
	void *key = node->content + tree->key_offset;

	inserted = bst_find(key, tree);
	if(inserted!= NULL)
	{
		/* Cannot insert, there already exists an entry */
		if(node->content == inserted->content)
		{
			inserted = NULL;
			goto EXIT_LABEL;
		}
	}
	/* else, try to insert */
	inserted = tree->root;
	while(inserted!=NULL)
	{
		comparison_result = tree->compare(key, inserted->content + tree->key_offset);
		penultimate = inserted;
		if(comparison_result <0)
		{
			/* Move left */
			inserted = inserted->left;
		}
		/* So that same key guys get FCFS traversal. */
		else if(comparison_result>=0)
		{
			/* Move right */
			inserted = inserted->left;	
		}
		else
		{
			fprintf(stderr, "Key clashes!\n");
			inserted = NULL;
			goto EXIT_LABEL;
		}
	}
	/* By now, we've found the place to insert after */
	if(comparison_result<0)
	{
		penultimate->left = node;
	}
	else
	{
		penultimate->right = node;
	}
	node->parent = penultimate;

	inserted = node;
	/* Just for speed, cache the leftmost entry in the tree itself */
	if(tree->first!=NULL && tree->compare(key, tree->first->content + tree->key_offset) < 0)
	{
		tree->first = inserted;
	}
EXIT_LABEL:
	return(inserted);	
}

void * bst_find(void *key, BST *tree)
{
	BST_NODE *node = NULL;
	int32_t comparison_result;
	node = tree->root;

	while(node!=NULL)
	{
		comparison_result = tree->compare(key, node->content + tree->key_offset);
		if(comparison_result==0)
		{
			break;
		}
		/* else traverse tree further */
		node = bst_next(node, tree);
	}
	return(node);
}

void * bst_next(BST_NODE *node, BST *tree)
{
	BST_NODE *next = NULL;
	/* Next is always the leftmost child of the right child if it exists, or parent */
	if(node->right != NULL)
	{
		/* Get to the leftmost child of right child */
		next = node->right;
		while(next->left!=NULL)
		{
			next = next->left;
		}
		/* Wherever we stopped, is the next node */	
	}
	else
	{
		/* Was leaf node. Pass next available right node of parent */
		if(node->parent!=NULL && node != node->parent->right)/* not root */
		{
			next = node->parent;
		}
		else
		{
			next = node->parent->parent;
		}
	}

	return(next);
}

void bst_delete(BST_NODE *node, BST *tree)
{
	BST_NODE *right_child = NULL;

	if(tree->first == node)
	{
		/* Update cached variable */
		tree->first = node->parent;
	}
	if(node->right==NULL)
	{
		/* Just make its left child its replacement */
		if(node->left!=NULL)
		{
			/* Check if node deleted is root */
			if(node == tree->root)
			{
				tree->root = node->left;
			}
			else if(node == node->parent->left)
			{
				node->parent->left = node->left;
			}
			else
			{
				node->parent->right = node->left;	
			}
		}
	}
	else if(node->left==NULL)
	{
		/* Just make its right child its replacement */
		if(node->right!=NULL)
		{
			if(node == tree->root)
			{
				tree->root = node->right;
			}
			else if(node == node->parent->left)
			{
				node->parent->left = node->right;
			}
			else
			{
				node->parent->right = node->right;	
			}
		}	
	}
	else if(node->left !=NULL && node->right != NULL)
	{
		right_child = node->right;
		/* Replace node by its left child */
		if(node == tree->root)
		{
			tree->root = node->left;
		}
		else if(node == node->parent->left)
		{
			node->parent->left = node->left;
		}
		else
		{
			node->parent->right = node->left;	
		}
		/* 
		   Then, since the right side of parent was strictly greater than the greatest node on left
		   insert the right_child at the right of the rightmost node of left-child.
		 */
		node = node->right;
		while(node->right!=NULL)
		{
			node = node->right;
		} 
		node->right = right_child;
	}

	return;
}

void bst_rotate_right(BST_NODE *node, BST *tree)
{
	/* Rotate right about node */
	BST_NODE *left_child = NULL;

	left_child = node->left; /* Replaces node in tree */
	if(left_child->right!=NULL)
	{
		node->left = left_child->right;
	}
	left_child->parent = node->parent;
	if(left_child->parent == NULL)
	{
		tree->root = left_child;
	}
	else
	{
		if(node == node->parent->left)
		{
			node->parent->left = left_child;	
		}
		else
		{
			node->parent->right = left_child;		
		}
	}
	left_child->right = node;
	node->parent = left_child;

	return;
}

void red_black_insert(BST_NODE *node, BST *tree)
{
	/* First insert the node in tree as usual. Then restore red-black property */
	BST_NODE *inserted = NULL;
	BST_NODE *uncle = NULL;
	inserted = bst_insert(node, tree);
	if(inserted==NULL)
	{
		fprintf(stderr, "Error inserting into tree.\n");
		exit(0);
	}
	/* Always give node red color */
	node->color = RED;

	/* If red-black properties are destroyed, restore them */
	while((node != tree->root) && (node->parent->color == RED))
	{
		/* Node and its parent can't be of color RED */
		if((node->parent->parent!= NULL) && (node->parent == node->parent->parent->left))
		{
			uncle = node->parent->parent->right;
			/* Uncle is on the right */
			if(uncle->color == RED)
			{
				/* Change colors of parent, uncle to black and grandparent to red*/
				node->parent->color = BLACK;
				uncle->color = BLACK;
				node->parent->parent->color = RED;
				node = node->parent->parent;
			}
			else
			{
				/* Uncle is black, but parent is RED */
				if(node == node->parent->right)
				{
					/* Node on parent's right */
					node = node->parent;
					bst_rotate_left(node, tree);
				}

				/* Left left case */
				/* Node is red */
				node->parent->color = BLACK;
				node->parent->parent->color = RED;
				bst_rotate_right(node->parent->parent, tree);				
				
			}
		}
		else if(node->parent->parent!= NULL)
		{
			/* node's parent on right of parent's parent*/
			uncle = node->parent->parent->left;	
			if(uncle->color == RED)
			{
				/* Change colors of parent, uncle to black and grandparent to red*/
				node->parent->color = BLACK;
				uncle->color = BLACK;
				node->parent->parent->color = RED;
			}
			else
			{
				/* Uncle is black, but parent is RED */
				/*Right left case */
				if(node == node->parent->left)
				{
					/* Node on parent's right */
					node = node->parent;
					bst_rotate_right(node, tree);
				}
				/* Node is red */
				node->parent->color = BLACK;
				node->parent->parent->color = RED;
				bst_rotate_left(node->parent->parent, tree);
			}
		}
	}
	/* Color the root black */
	tree->root->color = BLACK;
	return;
}

static void deletion_recurse(BST_NODE *node, BST *tree)
{
	if(node->right==NULL && node->left != NULL)
	{
		/* Replace contents of node by its left child's content */
		node->content = node->left->content;	
		deletion_recurse(node->left, tree);
	}
	else if(node->right!=NULL)
	{
		/* Make right child its replacement and recursively delete it */
		node->content = node->right->content;
		deletion_recurse(node->right, tree);
	}
	else
	{
		/* Both left and right are NULL. */
		if(node->parent == NULL)
		{
			/* I am groot.*/
			tree->root = NULL;
		}
		else
		{
			if(node == node->parent->left)
			{
				node->parent->left = NULL;
			}
			else
			{
				node->parent->right = NULL;	
			}
			free(node);
		}
	}
	return;
}

void *bst_first(BST tree)
{
	return (tree.first);
}

void bst_rotate_left(BST_NODE *node, BST *tree)
{
	/* This code ignores the possibility that someone will try to rotate left around a node that is rightmost */
	BST_NODE *sibling = NULL;

	BST_NODE *right_child=NULL;

	right_child = node->right;
	if(right_child->left != NULL)
	{
		right_child->left->parent = node;	
	}
	right_child->parent = node->parent; /* Could be root too */
	if(right_child->parent == NULL)
	{
		tree->root = right_child;
	}
	else /* Not root */
	{
		if(node == node->parent->left)
		{
			/* Non-root and left child of parent */
			node->parent->left = right_child;
		}
		else
		{	
			/* Non-root and right child of parent */
			node->parent->right = right_child;
		}
		if(node->color == BLACK && node->parent->color == BLACK)
		{
			/* Look into the sibling */

		}
	}
	right_child->left = node;
	node->parent = right_child;
}

void red_black_delete(BST_NODE *node, BST *tree)
{
 	BST_NODE *backup = (BST_NODE *)malloc(sizeof(BST_NODE));
	if(!backup)
	{
		fprintf(stderr, "Could not alloc memory.\n");
		exit(0);
	}
	memcpy(node, backup, sizeof(BST_NODE));

	deletion_recurse(node, tree);

}

int32_t compare_int(void *aa, void *bb)
{
	int32_t key1 = (int32_t) *aa;
	int32_t key2 = (int32_t) *bb;

	if(key1<key2)
	{
		return -1;
	}
	else if(key1>key2)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}