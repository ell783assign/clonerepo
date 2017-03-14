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
		inserted = NULL;
		goto EXIT_LABEL;
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
		else if(comparison_result>0)
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
			if(node == node->parent->left)
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
			if(node == node->parent->left)
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
		if(node == node->parent->left)
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

void *bst_first(BST tree)
{
	return (tree.first);
}