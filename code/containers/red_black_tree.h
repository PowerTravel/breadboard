#pragma once

#define RED_BLACK_TREE_UNIT_TESTS 1


// Header only structure.
#if RED_BLACK_TREE_UNIT_TESTS
#define RedBlackTreeAssert(Expression) if(!(Expression)){ *(int *)0 = 0;}
#endif

enum class reorder_case
{
  None,
  LeftLeft,
  LeftRight,
  RightLeft,
  RightRight
};

#define CHILD_IS_ROOT 0
#define CHILD_IS_LEFT 1
#define CHILD_IS_RIGHT 2

enum class node_color
{
  RED,
  BLACK,
};

struct red_black_tree_node_data
{
  void* Data;
  red_black_tree_node_data* Next;
};

struct red_black_tree_node
{
  size_t Key;
  node_color Color;
  red_black_tree_node_data* Data;
  red_black_tree_node* Left;
  red_black_tree_node* Right;
  red_black_tree_node* Parent; 
};

struct red_black_tree
{
  int NodeCount;
  red_black_tree_node* Root;
};

// Interface Function:
red_black_tree NewRedBlackTree();
red_black_tree_node_data NewRedBlackTreeNodeData(void* Data);
red_black_tree_node NewRedBlackTreeNode(size_t Key, red_black_tree_node_data* Data);

// Don't call InOrderTraverse or PostOrderTraverse in NodeFunction. They modify the tree structure while traversing
void InOrderTraverse(red_black_tree* Tree, void* CustomData, void (*NodeFunction)  (red_black_tree_node const * Node, void* CustomData) );
void PostOrderTraverse(red_black_tree* Tree, void* CustomData, void (*NodeFunction)  (red_black_tree_node const * Node, void* CustomData) );

// From root, leading to left-most leaf first, followed by branch leading to the second left leaf and so on untill all leafs are visited
size_t PreOrderGetStackMemorySize(red_black_tree* Tree);
// PreOrderTraverse can be called within node function
void PreOrderTraverse(red_black_tree* Tree, void* StackMemory, void* CustomData, void (*NodeFunction) (red_black_tree_node const * Node, void* CustomData));

// Returns true if a new node was inserted. False if not. It has however added the
bool RedBlackTreeInsert(red_black_tree* Tree, red_black_tree_node* NewNode);
red_black_tree_node* RedBlackTreeDelete(red_black_tree* Tree, size_t Key);
red_black_tree_node* RedBlackTreeFind(red_black_tree* Tree, size_t Key);

// Since red_black_tree_node_data is a list, this function removes a red_black_tree_node_data from that list without touching any memory
void RedBlackTreeRemoveSingleDataFromDataList(red_black_tree* Tree, red_black_tree_node_data** NodeDataPtr, void* Data);
inline u32 RedBlackTreeNodeCount(red_black_tree* Tree);

// Note: Not unit tested
void RedBlackTreeRemoveSingleDataFromDataList(red_black_tree* Tree, red_black_tree_node_data** NodeDataPtr, void* Data)
{
  if(!*NodeDataPtr)
  {
    return;
  }

  while(*NodeDataPtr)
  {
    red_black_tree_node_data* DataToDelete = *NodeDataPtr;
    if(DataToDelete->Data == Data)
    {
      *NodeDataPtr = DataToDelete->Next;
      break;
    }
    NodeDataPtr = &(*NodeDataPtr)->Next;
  }
}

u32 RedBlackTreeNodeCount(red_black_tree* Tree)
{
  u32 Result = Tree->NodeCount;
  return Result;
}

red_black_tree NewRedBlackTree()
{
  red_black_tree Result = {};
  return Result;
}

// Arguments
//    Data: Pointer to data to be stored
// Returns
//    A initialized red_black_tree_node_data struct
red_black_tree_node_data NewRedBlackTreeNodeData(void* Data)
{
  red_black_tree_node_data Result = {};
  Result.Data = Data;
  return Result;
}

// Arguments
//    Key: A int representation of Data
//    Data: A initialized red_black_tree_node_data created with NewRedBlackTreeNodeData(void* Data);
// Returns
//    A initialized red_black_tree_node_data struct
red_black_tree_node NewRedBlackTreeNode(size_t Key, red_black_tree_node_data* Data)
{
  red_black_tree_node Result = {};
  Result.Key = Key;
  Result.Data = Data;
  return Result;
}

struct red_black_tree_get_family_result
{
  reorder_case Case;
  red_black_tree_node* GrandParent; // If case is NOT node_color::None GrandParent is guaranteed to exist.
  red_black_tree_node* Parent;      // If case is NOT node_color::None Parent is guaranteed to exist.
  red_black_tree_node* Uncle;       // Not guaranteed to exist
  red_black_tree_node* Sibling;     // Not guaranteed to exist
};

reorder_case GetNoderOrder(red_black_tree* Tree, red_black_tree_node* Node)
{
  RedBlackTreeAssert(Node->Parent && Node->Parent->Parent);
  reorder_case Result = {};
  if(Node->Parent->Left == Node)
  {
    if(Node->Parent->Parent->Left == Node->Parent)
    {
      Result = reorder_case::LeftLeft;
    }else if(Node->Parent->Parent->Right == Node->Parent)
    {
      Result = reorder_case::RightLeft;
    }
  }else{
    RedBlackTreeAssert(Node->Parent->Right == Node);
    if(Node->Parent->Parent->Left == Node->Parent)
    {
      Result = reorder_case::LeftRight;
    }else if(Node->Parent->Parent->Right == Node->Parent)
    {
      Result = reorder_case::RightRight;
    }
  }
  return Result;
}

inline red_black_tree_get_family_result GetFamily(red_black_tree* Tree, red_black_tree_node* Node)
{
  red_black_tree_get_family_result Result = {};

  if(!Node || !Node->Parent)
  {
    return Result;
  }
  
  // Get Family and case
  Result.Parent = Node->Parent;
  if(Result.Parent->Color == node_color::RED)
  {
    // Child is the left node
    if(Result.Parent->Left == Node)
    {  
      Result.Sibling = Result.Parent->Right;
      Result.GrandParent = Result.Parent->Parent;
      // Parent is left node, Child is the left node
      if(Result.GrandParent->Left == Result.Parent)
      {
        Result.Uncle = Result.GrandParent->Right;
        Result.Case = reorder_case::LeftLeft;
      }
      // Parent is right node, Child is left node
      else
      {
        Result.Uncle = Result.GrandParent->Left;
        Result.Case = reorder_case::RightLeft;
      }
    }
    // Child is the right node
    else
    {
      Result.Sibling = Result.Parent->Left;

      Result.GrandParent = Result.Parent->Parent;

      // Parent is Left Node, Child is the right node
      if(Result.GrandParent->Left == Result.Parent)
      {
        Result.Uncle = Result.GrandParent->Right;
        Result.Case = reorder_case::LeftRight;
      }
      // Parent is Right Node, Child is the right node
      else
      {
        Result.Uncle = Result.GrandParent->Left;
        Result.Case = reorder_case::RightRight;
      }
    }
  }

  return Result;
}

void RightRotation(red_black_tree* Tree, red_black_tree_node* Node)
{
  // Only right rotate if Node and Left Child exists
  RedBlackTreeAssert(Node && Node->Left);

  red_black_tree_node* LeftChild = Node->Left;
  if(!Node->Parent)
  {
    Tree->Root = LeftChild;
    LeftChild->Parent = 0;
  }else{
    if(Node->Parent->Left == Node)
    {
      Node->Parent->Left = LeftChild;
      LeftChild->Parent = Node->Parent;
    }else{
      Node->Parent->Right = LeftChild;
      LeftChild->Parent = Node->Parent;
    }
  }

  Node->Left = LeftChild->Right;
  if(Node->Left)
  {
    Node->Left->Parent = Node;
  }

  LeftChild->Right = Node;
  LeftChild->Right->Parent = LeftChild;
}

void LeftRotation(red_black_tree* Tree, red_black_tree_node* Node)
{
  // Only left rotate if Node and Right Child exists
  RedBlackTreeAssert(Node && Node->Right);

  red_black_tree_node* RightChild = Node->Right;

  if(!Node->Parent)
  {
    Tree->Root = RightChild;
    RightChild->Parent = 0;
  }else{
    if(Node->Parent->Left == Node)
    {
      Node->Parent->Left = RightChild;
      RightChild->Parent = Node->Parent;
    }else{
      Node->Parent->Right = RightChild;
      RightChild->Parent = Node->Parent;
    }
  }

  Node->Right = RightChild->Left;
  if(Node->Right)
  {
    Node->Right->Parent = Node;
  }

  RightChild->Left = Node;
  RightChild->Left->Parent = RightChild;
}

void ColorSwap(red_black_tree_node* NodeA, red_black_tree_node* NodeB)
{
  // Swap Color of parent and grand parent
  RedBlackTreeAssert(NodeA && NodeB);
  node_color TmpColor = NodeA->Color;
  NodeA->Color = NodeB->Color;
  NodeB->Color = TmpColor;  
}

bool BinarySearchTreeInsert(red_black_tree* Tree, red_black_tree_node* NodeToInsert)
{
  red_black_tree_node* NodeInTree = Tree->Root;
  if(!Tree->Root)
  { 
    Tree->Root = NodeToInsert;
    Tree->NodeCount++;
    return true;
  }

  while(true)
  {
    if(NodeInTree->Key > NodeToInsert->Key)
    {
      // NodeInTree is larger than than NodeToInsert: Navigate Left
      if(NodeInTree->Left)
      {
        // Move down in tree
        NodeInTree = NodeInTree->Left;
      }else{
        // Add NodeToInsert as a leaf and return
        NodeInTree->Left = NodeToInsert;
        NodeToInsert->Parent = NodeInTree;
        Tree->NodeCount++;
        return true;
      }
    }else if(NodeInTree->Key < NodeToInsert->Key){
      // NodeInTree is smaller than NodeToInsert: Navigate Right
      if(NodeInTree->Right)
      {
        // Move down in tree
        NodeInTree = NodeInTree->Right;
      }else{
        // Add NodeToInsert as a leaf and return
        NodeInTree->Right = NodeToInsert;
        NodeToInsert->Parent = NodeInTree;
        Tree->NodeCount++;
        return true;
      }
    }else{
      // NodeInTree has samke key as NodeToInsert: Add to data collection and return
      red_black_tree_node_data* Data = NodeInTree->Data;
      NodeToInsert->Data->Next = NodeInTree->Data;
      NodeInTree->Data = NodeToInsert->Data;
      return false;
    }
  }

  // should never reach here
  return false;
}

void RecolorTreeAfterInsert(red_black_tree* Tree, red_black_tree_node* Node)
{
  Node->Color = node_color::RED;
  while(Node)
  {
    red_black_tree_get_family_result Family = GetFamily(Tree, Node);
    if(Family.Uncle && Family.Uncle->Color == node_color::RED)
    {
      Family.Parent->Color = node_color::BLACK;
      Family.Uncle->Color  = node_color::BLACK;
      Family.GrandParent->Color = node_color::RED;
      Node = Node->Parent ? Node->Parent->Parent : 0;
    }else{
      switch(Family.Case)
      {
        case reorder_case::LeftLeft:
        {
          RightRotation(Tree, Family.GrandParent);
          ColorSwap(Family.Parent, Family.GrandParent);
        }break;
        case reorder_case::LeftRight:
        {
          LeftRotation(Tree, Family.Parent);
          RightRotation(Tree, Family.GrandParent); 
          ColorSwap(Node, Family.GrandParent);
        }break;
        case reorder_case::RightLeft:
        {
          RightRotation(Tree, Family.Parent);
          LeftRotation(Tree, Family.GrandParent);
          ColorSwap(Node, Family.GrandParent);
        }break;
        case reorder_case::RightRight:
        {
          LeftRotation(Tree, Family.GrandParent);
          ColorSwap(Family.Parent, Family.GrandParent);
        }break;
      }
      Node = 0;
    }
  }

  // Tree root is always black
  Tree->Root->Color = node_color::BLACK;
}

// Arguments
//    Tree: A initialized Tree created with NewRedBlackTree();
//    NewNode: A initialized red_black_tree_node created with NewRedBlackTreeNode(int Key, red_black_tree_node_data* Data);
// Returns
//    true if a NewNode node was added to the tree (the data was not already in the tree)
//    false if a NewNode node was NOT added to the tree (the data was already in the tree and added to that node instead)
//       Note: NewNode has been cleared and can thus be discarded
bool RedBlackTreeInsert(red_black_tree* Tree, red_black_tree_node* NewNode)
{
  bool NodeInserted = BinarySearchTreeInsert(Tree, NewNode);
  if(NodeInserted)
  {
    RecolorTreeAfterInsert(Tree, NewNode);
  }

  return NodeInserted;
}

struct node_delete_case
{
  int ChildCount;
  red_black_tree_node* NodeOne;
  red_black_tree_node* NodeTwo;
};

node_delete_case GetNodeDeleteCase(red_black_tree_node* Node)
{
  node_delete_case Result = {};
  if(!Node->Left && !Node->Right)
  {
    Result.ChildCount = 0;
  }else if(Node->Left && !Node->Right)
  {
    Result.ChildCount = 1;
    Result.NodeOne = Node->Left;
  }else if(!Node->Left &&  Node->Right)
  {
    Result.ChildCount = 1;
    Result.NodeOne = Node->Right;
  }else{
    Result.ChildCount = 2;
    Result.NodeOne = Node->Left;
    Result.NodeTwo = Node->Right;
  }
  return Result;
}

void ConnectRight(red_black_tree_node* Parent, red_black_tree_node* Child)
{
  // Parent must exist
  RedBlackTreeAssert(Parent);
  // Parent Cannot have a right child
  RedBlackTreeAssert(!Parent->Right);
  Parent->Right = Child;
  if(Parent->Right)
  {
    Child->Parent = Parent;
  }
}

// It's ok if Child is null
void ConnectLeft(red_black_tree_node* Parent, red_black_tree_node* Child)
{
  // Parent must exist
  RedBlackTreeAssert(Parent);
  // Parent Cannot have a right child
  RedBlackTreeAssert(!Parent->Left);
  Parent->Left = Child;
  if(Parent->Left)
  {
    Child->Parent = Parent;
  }
}

red_black_tree_node* RemoveNodeWithOnlyLeftChildFromTree(red_black_tree_node* Node)
{
  // Left inner node has no right child
  RedBlackTreeAssert(!Node->Right);
  red_black_tree_node* ReplacementNode = 0;
  red_black_tree_node* Parent = Node->Parent;
  if(Parent)
  {
    if(Parent->Right == Node)
    {
      // Dissconnect Node and it's subtree from the parent
      Node->Parent->Right = 0;
      Node->Parent = 0;
      ConnectRight(Parent, Node->Left);
      ReplacementNode = Node->Left;
    }
    else
    {
      RedBlackTreeAssert(Parent->Left == Node);
      Node->Parent->Left = 0;
      Node->Parent = 0;
      ConnectLeft(Parent, Node->Left);
      ReplacementNode = Node->Left;
    }
  }
  Node->Left = 0;
  return ReplacementNode;
}

struct binary_search_tree_delete_result
{
  // The BinarySearchTreeDelete operation navigates to the node with the key to be removed,
  // Then it finds the replacement node and copies the values (Key and Data, not color), into the node which
  // the user removed, the replacement node is the one which is actually deleted.
  // We store the replacement node of the actually deleted node.
  red_black_tree_node* ReplacementNode;
  red_black_tree_node* ReplacementNodeParent;
  red_black_tree_node* DeletedNode;

  // Data of the node which held the key passed to BinarySearchTreeDelete)
  red_black_tree_node_data* ReturnData;
};

binary_search_tree_delete_result BinarySearchTreeDelete(red_black_tree* Tree, size_t Key)
{
  binary_search_tree_delete_result Result = {};
  red_black_tree_node* Node = Tree->Root;
  while(Node)
  {
    if(Key > Node->Key)
    {
      Node = Node->Right;
    }
    else if(Key < Node->Key)
    {
      Node = Node->Left;
    }
    else
    {
      break;
    }
  } 

  if(Node)
  {
    Tree->NodeCount--;
    node_delete_case DeleteCase = GetNodeDeleteCase(Node);
    Result.ReturnData = Node->Data;
    switch(DeleteCase.ChildCount)
    {
      case 0:
      {
        Result.ReplacementNode = NULL; // Replacement Node is NIL Leaf
        Result.DeletedNode = Node;
        Result.ReplacementNodeParent = Node->Parent;
        if(!Result.ReplacementNodeParent)
        {
          Tree->Root = 0;
        }else{
          if(Result.ReplacementNodeParent->Left == Result.DeletedNode)
          {
            Result.ReplacementNodeParent->Left = 0;
          }else{
            Assert(Result.ReplacementNodeParent->Right == Node);
            Result.ReplacementNodeParent->Right = 0;
          }
        }

        Node->Parent = 0;
        Node->Left = 0;
        Node->Right = 0;
      }break;
      case 1:
      {
        Result.ReplacementNode = Node->Left ? Node->Left : Node->Right;
        Result.DeletedNode = Node;
        Result.ReplacementNodeParent = Node->Parent;

        if(!Result.ReplacementNodeParent)
        {
          Tree->Root = Result.ReplacementNode;
          Result.ReplacementNode->Parent = 0;
        }else{
          if(Result.ReplacementNodeParent->Left == Node)
          {
            Result.ReplacementNode->Parent = Result.ReplacementNodeParent;
            Result.ReplacementNodeParent->Left = Result.ReplacementNode;
          }else{
            Assert(Result.ReplacementNodeParent->Right == Node);
            Result.ReplacementNode->Parent = Result.ReplacementNodeParent;
            Result.ReplacementNodeParent->Right = Result.ReplacementNode;
          }
        }

        Node->Parent = 0;
        Node->Left = 0;
        Node->Right = 0;
      }break;
      case 2:
      {
        red_black_tree_node* NodeToDelete = Node->Left;

        while(NodeToDelete->Right != NULL)
        {
          NodeToDelete = NodeToDelete->Right;
        }

        RedBlackTreeAssert(NodeToDelete);
        RedBlackTreeAssert(NodeToDelete->Parent);
        RedBlackTreeAssert(!NodeToDelete->Right);

        Result.DeletedNode = NodeToDelete;
        Result.ReplacementNodeParent = NodeToDelete->Parent;
        Node->Key = NodeToDelete->Key;
        Node->Data = NodeToDelete->Data;

        Result.ReplacementNode = NodeToDelete->Left;

        // Deleted Node is left child
        if(NodeToDelete->Parent->Left == NodeToDelete)
        {
          // Deleted Node was not a leaf
          if(Result.ReplacementNode)
          {
            Result.ReplacementNode->Parent = Result.ReplacementNodeParent;
            Result.ReplacementNodeParent->Left = Result.ReplacementNode;
          }else{
            Result.ReplacementNodeParent->Left = 0;
          }
        }
        // Deleted Node is right child
        else
        {
          Assert(NodeToDelete->Parent->Right == NodeToDelete);
          // Deleted Node was not a leaf
          if(Result.ReplacementNode)
          {
            Result.ReplacementNode->Parent = Result.ReplacementNodeParent;
            Result.ReplacementNodeParent->Right = Result.ReplacementNode;
          }else{
            Result.ReplacementNodeParent->Right = 0;
          }
        }

        NodeToDelete->Parent = 0;
        NodeToDelete->Left = 0;
        NodeToDelete->Right = 0;
      }break;
    }
  }
  return Result;
}

static node_color GetColor(red_black_tree_node* Node)
{
  if(!Node)
  {
    return node_color::BLACK;
  }else{
    return Node->Color;
  }
}


// Color criteria:
// 0: A node must be either red or black (double black exists but violates tree structure)
// 1: Root is always black
// 2: Nil Leafs are always black
// 3: There are no successive Red nodes
// 4: Black node count is the same for all paths from root to leaf

// If a black node is deleted and it's replacement-child is black (nil-leafs are black). The node becomes double black.

// A double black node must give one black to another node.
// It can only give its black color to a red sibling or a black sibling with one or more red children, otherwise it gives it's
// black color to its black parent which becomes double black.

// From https://www.javatpoint.com/red-black-tree
// Case 0: Nothing to be done
// Case 1: Node to be deleted is red ->
//            Simply delete it in BST fashion. Removing a Red node does not change color criteria
// Case 2: Root node is double black ->
//            Make it single black
// Case 3: Double black sibling is black and both it's children are black ->
//            Pass the black color to the parent.
//            - If parent receiving black color is red it becomes black
//            - If parent receiving black color is black it becomes double black
//            Double blacks sibling becomes red.
//            Double black node becomes single black
//            Reapply cases while a node is double black
// Case 4: Double Black sibling is red ->
//            Swap color of double black parent and sibling
//            Rotate parent in direction of double black
//            Reapply cases
// Case 5: Double black sibling is black, far siblings child is black, near sibling child is red ->
//            Swap color of double blacks sibling and the sibling child which is red (near)
//            Rotate the sibling in the opposite direction of double black
//            Apply case 6
// Case 6: Double blacks sibling is black, near sibling child is black, far sibling child is red ->
//            Swap the color of the parent and its sibling node
//            Rotate the parent towards double black
//            Remove double black
//            Change Red color to black (that is, the far sibling)
// Case 7: Double blacks sibling is black, both sibling children are red (Not from link, Did they forget this case? May be incomplete)
//            Set far sibling child to black.
//            Rotate double black parent towards double black node


int GetDeleteCase(red_black_tree_node* DoubleBlackNode, red_black_tree_node* DoubleBlackParent)
{
  int Result = 0;
  if(!DoubleBlackParent)
  {
    // Case 2: DoubleBlackNode is Root
    Result = 2;
  }
  else
  {
    red_black_tree_node* Sibling = DoubleBlackParent->Left == DoubleBlackNode ? DoubleBlackParent->Right : DoubleBlackParent->Left;
    if(GetColor(Sibling) == node_color::RED)
    {
      //  Case 4: Double Black sibling is red
      Result = 4;
    }
    else
    {
      if(Sibling && (GetColor(Sibling->Left) == node_color::BLACK) && (GetColor(Sibling->Right) == node_color::RED))
      {
        if(Sibling->Parent->Right == Sibling)
        {
          // Case 6: Double blacks sibling is black, near sibling child is black, far sibling child is red
          Result = 6;
        }else{
          // Case 5: Double black sibling is black, far siblings child is black, near sibling child is red
          Result = 5;
        }
      }
      else if(Sibling && (GetColor(Sibling->Left) == node_color::RED) && (GetColor(Sibling->Right) == node_color::BLACK))
      {
        if(Sibling->Parent->Right == Sibling)
        {
          // Case 5: Double black sibling is black, far siblings child is black, near sibling child is red
          Result = 5;
        }else{
          // Case 6: Double blacks sibling is black, near sibling child is black, far sibling child is red
          Result = 6;
        }
      }
      else if(Sibling && (GetColor(Sibling->Left) == node_color::RED) && (GetColor(Sibling->Right) == node_color::RED))
      {
        // Case 6: Double blacks sibling is black, both sibling children are red (Far sibling child is guaranteed to be red)
        Result = 6;
      }
      else
      {
        //Case 3: Double black sibling is black and both it's children are black
        Result = 3;
      }
    }
  }
  return Result;
}

void RecolorTreeAfterDelete(red_black_tree* Tree, binary_search_tree_delete_result BSTDelete)
{
  // Case 1: Deleted node or its replacement is RED. No reordering needs to be done.
  if(GetColor(BSTDelete.DeletedNode) == node_color::RED || GetColor(BSTDelete.ReplacementNode) == node_color::RED)
  {
    // If DeletedNode is RED it cannot be the same color as ReplacementNode since they were in a parent-child relationship. (No consecutive red nodes)
    RedBlackTreeAssert(GetColor(BSTDelete.DeletedNode) != GetColor(BSTDelete.ReplacementNode));
    
    // If the ReplacementNode node's color was red
    if(GetColor(BSTDelete.ReplacementNode) == node_color::RED)
    {
      // Simply color the it black.
      BSTDelete.ReplacementNode->Color = node_color::BLACK;  
    }
    return;
  }
  
  red_black_tree_node* DoubleBlackNode = BSTDelete.ReplacementNode;
  red_black_tree_node* DoubleBlackParent = BSTDelete.ReplacementNodeParent;
  bool ShouldStop = false;
  while(!ShouldStop)
  {
    int DeleteCase = GetDeleteCase(DoubleBlackNode, DoubleBlackParent);
    switch(DeleteCase)
    {
      case 2:
      {
        // Root is double black (Do nothing, ie, it is already black)
        ShouldStop = true;
      }break;
      case 3:
      {
        // Case 3: Double black sibling is black and both it's children are black ->
        //            Pass the black color to the parent.
        //            - If parent receiving black color is red it becomes black
        //            - If parent receiving black color is black it becomes double black
        //            Double blacks sibling becomes red.
        //            Double black node becomes single black
        RedBlackTreeAssert(DoubleBlackParent);
        red_black_tree_node* Sibling = DoubleBlackParent->Left == DoubleBlackNode ? DoubleBlackParent->Right : DoubleBlackParent->Left;
        RedBlackTreeAssert(GetColor(Sibling) == node_color::BLACK);
        Sibling->Color = node_color::RED;
        if(GetColor(DoubleBlackParent) == node_color::BLACK)
        {
          DoubleBlackNode = DoubleBlackParent;
          DoubleBlackParent = DoubleBlackNode->Parent;
        }else{
          DoubleBlackParent->Color = node_color::BLACK;
          ShouldStop = true;
        }
      }break;
      case 4:
      {
        // Case 4: Double Black sibling is red ->
        //            Swap color of double black parent and sibling
        //            Rotate parent in direction of double black
        //            Reapply cases
        RedBlackTreeAssert(DoubleBlackParent);

        if(DoubleBlackParent->Left == DoubleBlackNode)
        {
          RedBlackTreeAssert(DoubleBlackParent->Right);
          ColorSwap(DoubleBlackParent, DoubleBlackParent->Right);
          LeftRotation(Tree, DoubleBlackParent);
          // DoubleBlackParent and DoubleBlackNode relationship holds after rotation towards double black
        }else{
          RedBlackTreeAssert(DoubleBlackParent->Right == DoubleBlackNode); 
          ColorSwap(DoubleBlackParent, DoubleBlackParent->Left);
          RightRotation(Tree, DoubleBlackParent);
          // DoubleBlackParent and DoubleBlackNode relationship holds after rotation towards double black
        }
      }break;
      case 5:
      {
        // Case 5: Double black sibling is black, far siblings child is black, near sibling child is red ->
        //            Swap color of double blacks sibling and the sibling child which is red (near)
        //            Rotate the sibling in the opposite direction of double black
        //            Apply case 6
        
        if(DoubleBlackParent->Left == DoubleBlackNode)
        {
          red_black_tree_node* Sibling = DoubleBlackParent->Right;
          RedBlackTreeAssert(Sibling);
          RedBlackTreeAssert(GetColor(Sibling)         == node_color::BLACK);
          RedBlackTreeAssert(GetColor(Sibling->Left)   == node_color::RED);
          RedBlackTreeAssert(GetColor(Sibling->Right)  == node_color::BLACK);
          ColorSwap(Sibling->Left, Sibling);
          RightRotation(Tree, Sibling);
        }else{
          red_black_tree_node* Sibling = DoubleBlackParent->Left;
          RedBlackTreeAssert(Sibling);
          RedBlackTreeAssert(GetColor(Sibling)        == node_color::BLACK);
          RedBlackTreeAssert(GetColor(Sibling->Right) == node_color::RED);
          RedBlackTreeAssert(GetColor(Sibling->Left)  == node_color::BLACK);
          ColorSwap(Sibling->Right, Sibling);
          LeftRotation(Tree, Sibling);
        }
      } // FallTrhough
      case 6:
      {
        // Case 6: Double blacks sibling is black, near sibling child is black, far sibling child is red ->
        //            Swap the color of the parent and its sibling node
        //            Rotate the parent towards double black
        //            Remove double black
        //            Change Red color to black (that is, the far child)
        
        if(DoubleBlackParent->Left == DoubleBlackNode)
        {
          red_black_tree_node* Sibling = DoubleBlackParent->Right;
          red_black_tree_node* RedChild = Sibling->Right;
          RedBlackTreeAssert(Sibling);
          RedBlackTreeAssert(GetColor(Sibling)    == node_color::BLACK);
          RedBlackTreeAssert(GetColor(RedChild)   == node_color::RED);

          ColorSwap(Sibling, DoubleBlackParent);
          LeftRotation(Tree, DoubleBlackParent);
          RedChild->Color = node_color::BLACK;
        }else{
          red_black_tree_node* Sibling = DoubleBlackParent->Left;
          red_black_tree_node* RedChild = Sibling->Left;
          RedBlackTreeAssert(Sibling);
          RedBlackTreeAssert(GetColor(Sibling)    == node_color::BLACK);
          RedBlackTreeAssert(GetColor(RedChild)   == node_color::RED);
          ColorSwap(Sibling, DoubleBlackParent);
          RightRotation(Tree, DoubleBlackParent);
          RedChild->Color = node_color::BLACK;
        }

        // DoubleBlackParent and DoubleBlackNode relationship does not hold after rotation of DoubleBlackParent
        
        DoubleBlackParent = 0;
        DoubleBlackNode = 0;
        ShouldStop = true;
      }break;
    }
  }
}

red_black_tree_node* RedBlackTreeDelete(red_black_tree* Tree, size_t Key)
{
  binary_search_tree_delete_result BSTDelete = BinarySearchTreeDelete(Tree, Key);
  RecolorTreeAfterDelete(Tree, BSTDelete);
  if(BSTDelete.DeletedNode)
  {
    BSTDelete.DeletedNode->Data = BSTDelete.ReturnData;
    BSTDelete.DeletedNode->Key = Key;
    BSTDelete.DeletedNode->Left = 0;
    BSTDelete.DeletedNode->Right = 0;
    BSTDelete.DeletedNode->Parent = 0;
  }
  return BSTDelete.DeletedNode;
}

red_black_tree_node* RedBlackTreeFind(red_black_tree* Tree, size_t Key)
{
  red_black_tree_node* Node = Tree->Root;
  while(Node)
  {
    if(Key > Node->Key)
    {
      Node = Node->Right;
    }else if (Key < Node->Key)
    {
      Node = Node->Left;
    }else{
      break;
    }
  }
  return Node;
}


// Tree:
//       1 
//      / \    
//     /   \
//    /     \
//   2       3
//  / \     / \
// 4   5   6   7

// PreOrder : 1, 2, 4, 5, 3, 6, 7
// PostOrder: 4, 5, 2, 6, 7, 3, 1
// InOrder  : 4, 2, 5, 1, 6, 3, 7

// From root, leading to left-most leaf first, followed by branch leading to the second left leaf and so on untill all leafs are visited
size_t PreOrderGetStackMemorySize(red_black_tree* Tree)
{
  size_t Result = Tree->NodeCount * sizeof(red_black_tree_node*);
  return Result;
}

int Push(int StackSize, red_black_tree_node** NodeStack, red_black_tree_node* Node)
{
  if(!Node)
  {
    return StackSize;
  }
  red_black_tree_node** NodeSpace = NodeStack + StackSize;
  *NodeSpace = Node;
  return StackSize+1;
}

int Pop(int StackSize, red_black_tree_node** NodeStack, red_black_tree_node** NodeResult)
{
  red_black_tree_node** NodePos = NodeStack+StackSize;
  RedBlackTreeAssert(StackSize > 0);
  RedBlackTreeAssert(NodeStack[StackSize-1]);
  *NodeResult = NodeStack[StackSize-1];
  NodeStack[StackSize-1] = NULL;
  return StackSize-1;
}

// Stack Memory must be large enough to hold at least Tree->NodeCount number of nodes
void PreOrderTraverse(red_black_tree* Tree, void* StackMemory, void* CustomData, void (*NodeFunction) (red_black_tree_node const * Node, void* CustomData))
{
  red_black_tree_node** NodeStack = (red_black_tree_node**) StackMemory;
  int StackCount = Push(0, NodeStack, Tree->Root);

  while(StackCount)
  {
    red_black_tree_node* Current = 0;
    StackCount = Pop(StackCount, NodeStack, &Current);
    RedBlackTreeAssert(Current);

    NodeFunction(Current, CustomData);

    StackCount = Push(StackCount, NodeStack, Current->Right);
    StackCount = Push(StackCount, NodeStack, Current->Left);

  }
}

// From Left most leaf : L R N
// https://www.geeksforgeeks.org/post-order-traversal-of-binary-tree-in-on-using-o1-space/
void PostOrderTraverse(red_black_tree* Tree, void* CustomData, void (*NodeFunction)  (red_black_tree_node const * Node, void* CustomData) )
{
  if(!Tree->Root)
  {
    return;
  }

  red_black_tree_node DummyNode = {};
  red_black_tree_node* Current = &DummyNode;
  red_black_tree_node* Pre = NULL;
  red_black_tree_node* Prev = NULL;
  red_black_tree_node* Succ = NULL;
  red_black_tree_node* Temp = NULL;

  Current->Left = Tree->Root;
  while(Current)
  {

    if(Current->Left == NULL)
    {
      Current = Current->Right;
    }
    else
    {
      Pre = Current->Left;

      while(Pre->Right && 
            Pre->Right != Current)
      {
        Pre = Pre->Right;
      }

      if(Pre->Right == NULL)
      {
        Pre->Right = Current;
        Current = Current->Left;
      }
      else
      {
        Pre->Right = NULL;
        Succ = Current;
        Current = Current->Left;
        Prev = NULL;

        while(Current != NULL)
        {
          Temp = Current->Right;
          Current->Right = Prev;
          Prev = Current;
          Current = Temp;
        }

        while(Prev != NULL)
        {
          NodeFunction(Prev, CustomData);

          Temp = Prev->Right;
          Prev->Right = Current;
          Current = Prev;
          Prev = Temp;
        }

        Current = Succ;
        Current = Current->Right;
      }
    }
  }
}

// From Left most leaf : L N R
// https://www.geeksforgeeks.org/inorder-tree-traversal-without-recursion-and-without-stack/
void InOrderTraverse(red_black_tree* Tree, void* CustomData, void (*NodeFunction)  (red_black_tree_node const * Node, void* CustomData) )
{
  if(!Tree->Root)
  {
    return;
  }

  red_black_tree_node* Current = Tree->Root;

  while(Current)
  {
    if(!Current->Left)
    {
      NodeFunction(Current, CustomData);
      Current = Current->Right;
    }else{
      red_black_tree_node* Pre = Current->Left;
      while(Pre->Right != NULL && Pre->Right != Current)
      {
        Pre = Pre->Right;
      }

      if(!Pre->Right)
      {
        Pre->Right = Current;
        Current = Current->Left;
      }else{
        Pre->Right = NULL;
        NodeFunction(Current, CustomData);
        Current = Current->Right;
      }
    }
  }
}