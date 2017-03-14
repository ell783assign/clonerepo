#include <include.h>

typedef (int32_t)(*COMPARE)(void *, void*);

typedef struct bst
{
	struct bst_node *root;
	COMPARE *compare;
	uint32_t key_offset;
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
void * bst_next(BST_NODE *);

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
	if(node->left != NULL)
	{
		next = node->left;
	}
	else if(node->right != NULL)
	{
		next = node->right;	
	}
	else
	{
		/* Was leaf node. Pass next available right node of parent */
		while(node->parent!=NULL)/* not root */
		{
			if(node->parent.right != NULL && node != node->parent.right)
			{
				next = node->parent.right;
				break;
			}
			else
			{
				node = node->parent;
			}
		}
		if(node->parent==NULL)
		{
			/* No more nodes to traverse */
			next = NULL;
		}

	}

	return(next);
}