
// TextObject implementation:
typedef struct TextObject TextObject;
TextObject *initTextObject(char * dc, char * ic); // dc and ic should be optional, NULL if unset. Allows inflate() and deflate equally.

// BinaryTree & Nodes
typedef struct BinaryTree BinaryTree;
typedef struct Node Node;
int appendNode(Node * node_list, Node new_node); // 0: OK | -1: Err | realloc() the node_list, add new child node.

//
int inflate(TextObject *obj); // Make it big.
int deflate(TextObject *obj); // Make it smol.
