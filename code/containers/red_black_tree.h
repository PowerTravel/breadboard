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
  int Key;
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
red_black_tree_node NewRedBlackTreeNode(int Key, red_black_tree_node_data* Data)
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
    }
    Node = Node->Parent ? Node->Parent->Parent : 0;
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
#if 0
void ReplaceNode(red_black_tree* Tree, red_black_tree_node* NodeToBeReplaced, red_black_tree_node* NodeToReplace)
{
  // Make sure the NodeToReplace is not connected to anything
  RedBlackTreeAssert(!NodeToReplace->Parent);
  RedBlackTreeAssert(!NodeToReplace->Left);
  RedBlackTreeAssert(!NodeToReplace->Right);

  if(!NodeToBeReplaced->Parent)
  {
    Tree->Root = NodeToReplace;
  }else{
    if(NodeToBeReplaced->Parent->Left == NodeToBeReplaced)
    {
      NodeToBeReplaced->Parent->Left = NodeToReplace;
    }else 
    {
      RedBlackTreeAssert(NodeToBeReplaced->Parent->Right == NodeToBeReplaced);
      NodeToBeReplaced->Parent->Right = NodeToReplace;
    }
  }
  NodeToReplace->Parent = NodeToBeReplaced->Parent;
  NodeToBeReplaced->Parent = 0;
  
  NodeToReplace->Left = NodeToBeReplaced->Left;
  if(NodeToReplace->Left)
  {
    NodeToReplace->Left->Parent = NodeToReplace;
  }
  NodeToBeReplaced->Left = 0;

  NodeToReplace->Right = NodeToBeReplaced->Right;
  if(NodeToReplace->Right)
  {
    NodeToReplace->Right->Parent = NodeToReplace;
  }
  NodeToBeReplaced->Right = 0;
}
#endif
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

binary_search_tree_delete_result BinarySearchTreeDelete(red_black_tree* Tree, int Key)
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

        red_black_tree_node* NodeParent = Node->Parent;
        if(!Node->Parent)
        {
          Tree->Root = 0;
        }
        else
        {
          if(Node->Parent->Left == Node)
          {
            Node->Parent->Left = 0;
          }
          else
          {
            Node->Parent->Right = 0;
          }  
        }
        Node->Parent = 0;
      }break;
      case 1:
      {
        red_black_tree_node* NodeParent = Node->Parent;

        Result.ReplacementNode = DeleteCase.NodeOne;
        Result.ReplacementNodeParent = DeleteCase.NodeOne->Parent;
        Result.DeletedNode = Node;
        if(!NodeParent)
        {
          Tree->Root = DeleteCase.NodeOne;
          DeleteCase.NodeOne->Parent = 0;
        }
        else
        {
          if(NodeParent->Left == Node)
          {
            DeleteCase.NodeOne->Parent = NodeParent;
            NodeParent->Left = DeleteCase.NodeOne;
          }
          else
          {
            DeleteCase.NodeOne->Parent = NodeParent;
            NodeParent->Right = DeleteCase.NodeOne;
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

        Result.ReturnData = Node->Data;
        Node->Key = NodeToDelete->Key;
        Node->Data = NodeToDelete->Data;
        Result.DeletedNode = NodeToDelete;
        Result.ReplacementNode = RemoveNodeWithOnlyLeftChildFromTree(NodeToDelete);
        Result.ReplacementNodeParent = Result.ReplacementNode->Parent;
      }break;
    }
  }
  return Result;
}

enum class delete_case_a
{
  DELETED_OR_REPLACEMENT_CHILD_IS_RED,
  NODE_AND_REPLACEMENT_CHILD_IS_BLACK,
  REPLACEMENT_CHILD_IS_ROOT,
};

static node_color GetColor(red_black_tree_node* Node)
{
  if(!Node)
  {
    return node_color::BLACK;
  }else{
    return Node->Color;
  }
}

delete_case_a GetDeleteCaseA(binary_search_tree_delete_result BSTDelete)
{
  delete_case_a Result = {};
  // Both DeletedNode and ReplacementNode cannot be red at the same time
  Assert(!((GetColor(BSTDelete.DeletedNode) == node_color::RED) && (GetColor(BSTDelete.ReplacementNode) == node_color::RED)));

  if(BSTDelete.ReplacementNode && !BSTDelete.ReplacementNodeParent)
  {
    Result = delete_case_a::REPLACEMENT_CHILD_IS_ROOT;
  }else if(GetColor(BSTDelete.DeletedNode) == node_color::RED || GetColor(BSTDelete.ReplacementNode) == node_color::RED)
  {
    Result = delete_case_a::DELETED_OR_REPLACEMENT_CHILD_IS_RED;
  }
  else
  {
    Result = delete_case_a::NODE_AND_REPLACEMENT_CHILD_IS_BLACK;
  }
  return Result;
}

enum class delete_case_b_case
{
  BLACK_SIBLING_ONE_OR_BOTH_CHILD_IS_RED,
  BLACK_SIBLING_BLACK_CHILDREN,
  SIBLING_IS_RED,
};

struct delete_case_b
{
  delete_case_b_case Case;
  red_black_tree_node* SiblingRedChild;
};

delete_case_b GetDeleteCaseB(binary_search_tree_delete_result BSTDelete)
{
  delete_case_b Result = {};

  // Don't know if this is always true, here to check
  RedBlackTreeAssert(BSTDelete.ReplacementNodeParent);

  bool SiblingIsRight = BSTDelete.ReplacementNodeParent->Left == BSTDelete.ReplacementNode;
  red_black_tree_node* Sibling = SiblingIsRight ? BSTDelete.ReplacementNodeParent->Right : BSTDelete.ReplacementNodeParent->Left;

  // Don't know if this is always true, here to check
  RedBlackTreeAssert(Sibling);

  if(GetColor(Sibling) == node_color::BLACK) 
  {
    if((GetColor(Sibling->Left) == node_color::RED && GetColor(Sibling->Right) == node_color::RED))
    {
      Result.Case = delete_case_b_case::BLACK_SIBLING_ONE_OR_BOTH_CHILD_IS_RED;
      Result.SiblingRedChild = SiblingIsRight ? Sibling->Right : Sibling->Left; 
    }else if((GetColor(Sibling->Left) == node_color::RED || GetColor(Sibling->Right) == node_color::RED))
    {
      Result.Case = delete_case_b_case::BLACK_SIBLING_ONE_OR_BOTH_CHILD_IS_RED;
      Result.SiblingRedChild = GetColor(Sibling->Left) == node_color::RED ? Sibling->Left : Sibling->Right;
    }
    else
    {
      Result.Case = delete_case_b_case::BLACK_SIBLING_BLACK_CHILDREN;
    }
  }else{
    Result.Case = delete_case_b_case::SIBLING_IS_RED;
  }
  return Result;
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

// Case 1: Node to be deleted is red ->
//            Simply delete it in BST fashion. Removing a Red node does not change color criteria
// Case 2: Root node is double black ->
//            Make it single black
// Case 3: Double black sibling is black and both it's children are black ->
//            Pass the black color to the parent.
//            - If parent receiving black color is red it becomes black
//            - If parent receiving black color is black it becomes double black
//            Double black sibling becomes red.
//            Double black node becomes single black
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

void RecolorTreeAfterDelete(red_black_tree* Tree, binary_search_tree_delete_result BSTDelete)
{
  delete_case_a DeleteCaseA = GetDeleteCaseA(BSTDelete);

  switch(DeleteCaseA)
  {
    case delete_case_a::DELETED_OR_REPLACEMENT_CHILD_IS_RED:
    {
      // Note: Both cannot be black

      // Simply Rrecolor the child to black
      BSTDelete.ReplacementNode->Color = node_color::BLACK;
    }break;
    case delete_case_a::NODE_AND_REPLACEMENT_CHILD_IS_BLACK:
    {
      // Both nodes are black which violates red black tree properties
      // Note: Null nodes are considered black

      // Replacement node is now double black and must be made single black.

      // While Replacement node is double black and not root:
      delete_case_b DeleteCaseB = GetDeleteCaseB(BSTDelete);
      switch(DeleteCaseB.Case)
      {
        case delete_case_b_case::BLACK_SIBLING_ONE_OR_BOTH_CHILD_IS_RED:
        {
          reorder_case Case = GetNoderOrder(Tree,DeleteCaseB.SiblingRedChild);
          switch(Case)
          {
            case reorder_case::LeftLeft:
            {
              
              RightRotation(Tree, DeleteCaseB.SiblingRedChild->Parent->Parent);
              //RedBlackTreeAssert(DeleteCaseB.SiblingRedChild->Parent->Color == node_color::BLACK);

              #if 1
              // Do we do color swap here as well instead to propagate the red color to the parent?
              // And if the parent is root it gets set to black below?
              DeleteCaseB.SiblingRedChild->Color = node_color::BLACK;
              #else
              if(DeleteCaseB.SiblingRedChild->Parent->Left)
              {
                DeleteCaseB.SiblingRedChild->Parent->Left->Color = node_color::BLACK;
              }
              if(DeleteCaseB.SiblingRedChild->Parent->Right)
              {
                DeleteCaseB.SiblingRedChild->Parent->Right->Color = node_color::BLACK;
              }
              DeleteCaseB.SiblingRedChild->Parent->Color = node_color::RED;
              #endif
            }break;
            case reorder_case::LeftRight:
            {
              //DeleteCaseB.SiblingRedChild->Color = node_color::BLACK;
              //DeleteCaseB.SiblingRedChild->Parent->Color = node_color::RED;
              LeftRotation(Tree, DeleteCaseB.SiblingRedChild->Parent);

              RightRotation(Tree, DeleteCaseB.SiblingRedChild->Parent);
              DeleteCaseB.SiblingRedChild->Left->Color = node_color::BLACK;
            }break;
            case reorder_case::RightLeft:
            {
              //DeleteCaseB.SiblingRedChild->Color = node_color::BLACK;
              //DeleteCaseB.SiblingRedChild->Parent->Color = node_color::RED;

              RightRotation(Tree, DeleteCaseB.SiblingRedChild->Parent);
              ColorSwap(DeleteCaseB.SiblingRedChild, DeleteCaseB.SiblingRedChild->Right);

              LeftRotation(Tree, DeleteCaseB.SiblingRedChild->Parent);

              // Do we do color swap here as well instead to propagate the red color to the parent?
              // And if the parent is root it gets set to black below?
              DeleteCaseB.SiblingRedChild->Right->Color = node_color::BLACK;
            }break;
            case reorder_case::RightRight:
            {
              DeleteCaseB.SiblingRedChild->Color = node_color::BLACK;
              //DeleteCaseB.SiblingRedChild->Parent->Color = node_color::RED;
              LeftRotation(Tree, DeleteCaseB.SiblingRedChild->Parent->Parent);
            }break;
            default:
            {
              RedBlackTreeAssert(0);
            }break;
          }

        }break;
        case delete_case_b_case::BLACK_SIBLING_BLACK_CHILDREN:
        {
          int a = 10;
        }break;
        case delete_case_b_case::SIBLING_IS_RED:
        {
          int a = 10;
          #if 0
          switch(DeleteCaseD)
          {
            case RED_SIBLING_IS_RIGHT:
            {

            }break;
            case RED_SIBLING_IS_LEFT:
            {

            }break;
          }
          #endif
        }break;
      }
    }break;
    case delete_case_a::REPLACEMENT_CHILD_IS_ROOT:
    {
      int a =0;
    }break;
  }

  if(Tree->Root)
  {
    Tree->Root->Color = node_color::BLACK;
  }
}

bool RedBlackTreeDelete(red_black_tree* Tree, int Key)
{
  binary_search_tree_delete_result BSTDelete = BinarySearchTreeDelete(Tree, Key);
  RecolorTreeAfterDelete(Tree, BSTDelete);
  return true;
}

struct node_state
{
  red_black_tree_node* Node;
  node_color Color;
  int Key;
  int ValueCount;
  int Values[32];
};

node_state NodeState(red_black_tree_node* Node, int Key, node_color Color, int ValueCount, int* Values)
{
  node_state Result = {};
  RedBlackTreeAssert(ValueCount < 32);
  Result.Node = Node;
  Result.Key = Key,
  Result.Color = Color;
  Result.ValueCount = ValueCount;
  for (int Index = 0; Index < ValueCount; ++Index)
  {
    Result.Values[Index] = Values[Index];
  }
  return Result;
}

static b32 IsEven(int num)
{
  b32 Result = (num % 2) == 0;
  return Result;
}

void AssertNode(red_black_tree_node* Node, int Key, node_color Color, red_black_tree_node* Parent, red_black_tree_node* LeftChild, red_black_tree_node* RightChild)
{
  RedBlackTreeAssert(Node->Color == Color);
  RedBlackTreeAssert(Node->Key == Key);
  RedBlackTreeAssert(Node->Parent == Parent);
  RedBlackTreeAssert(Node->Right == RightChild);
  RedBlackTreeAssert(Node->Left == LeftChild);
}
void AssertNodeData(red_black_tree_node_data* Data, int ValueCount, int* Value)
{
  int ValueIndex = 0;
  while(Data)
  {
    // DataValues comes in reverse order of insertion
    int DataInNode = *(int*)Data->Data;
    int DataGroundTruth = Value[ValueIndex];
    RedBlackTreeAssert(DataInNode == DataGroundTruth);
    ValueIndex++;
    Data = Data->Next;
  }
  RedBlackTreeAssert(ValueIndex == ValueCount);
}

void AssertTree(red_black_tree* Tree, int NodeCount, int StateCount, node_state* State)
{
  RedBlackTreeAssert(Tree->NodeCount == NodeCount);
  for (int NodeIndex = 0; NodeIndex < StateCount; ++NodeIndex)
  {
    red_black_tree_node* Node = State[NodeIndex].Node;
    if(Node)
    {
      int ValueCount = State[NodeIndex].ValueCount;
      int* Values = State[NodeIndex].Values;
      AssertNodeData(Node->Data, ValueCount, Values);

      int Key = State[NodeIndex].Key;
      node_color Color = State[NodeIndex].Color;

      red_black_tree_node* Parent = 0;
      int ParentIndex = 0;
      if(NodeIndex)
      {
        if(IsEven(NodeIndex))
        {
          ParentIndex = (NodeIndex - 2)/2;
        }else{
          ParentIndex = (NodeIndex - 1)/2;
        }
        Parent = State[ParentIndex].Node;
      }

      red_black_tree_node* LeftChild  = 0;
      int LeftChildIndex = NodeIndex * 2 + 1;
      if( LeftChildIndex < StateCount )
      {
        LeftChild = State[LeftChildIndex].Node;
      }
      
      red_black_tree_node* RightChild  = 0;
      int RightChildIndex = NodeIndex * 2 + 2;
      if( RightChildIndex < StateCount )
      {
        RightChild = State[RightChildIndex].Node;
      }

      AssertNode(Node, Key, Color, Parent, LeftChild, RightChild);
    }
  }
}

void TestInsertion()
{
  red_black_tree Tree = NewRedBlackTree(); 
  //             1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16
  int Data[] = { 3,21,32,15,12,13,14, 1, 5,37,25,40,38,28,23,22}; 
  red_black_tree_node_data Datum[32] = {};
  red_black_tree_node Nodes[32] = {};
  for (int Index = 0; Index < ArrayCount(Data); ++Index)
  {
    Datum[Index] = NewRedBlackTreeNodeData((void*)&Data[Index]);
    Nodes[Index] = NewRedBlackTreeNode(Data[Index], &Datum[Index]);
  }

  // Testing Adding root
  node_state State_0[] = 
  {
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 3
  };
  RedBlackTreeInsert(&Tree, &Nodes[0]);
  AssertTree(&Tree, 1, ArrayCount(State_0), State_0);
  // Testing Right insertion unde root
  node_state State_1[] = 
  {
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 3
    {},
    NodeState(&Nodes[1], Data[1], node_color::RED, 1, &Data[1]), // 21
  };
  RedBlackTreeInsert(&Tree, &Nodes[1]);
  AssertTree(&Tree, 2, ArrayCount(State_1), State_1);
  
  // Testing RR Case
  node_state State_2[] = 
  {
    NodeState(&Nodes[1], Data[1], node_color::BLACK, 1, &Data[1]), // 21 
    NodeState(&Nodes[0], Data[0], node_color::RED,   1, &Data[0]), //  3
    NodeState(&Nodes[2], Data[2], node_color::RED,   1, &Data[2]), // 32
  };
  RedBlackTreeInsert(&Tree, &Nodes[2]);
  AssertTree(&Tree, 3, ArrayCount(State_2), State_2);

  // Testing Right Left insertion with no rotation
  node_state State_3[] = 
  {
    NodeState(&Nodes[1], Data[1], node_color::BLACK, 1, &Data[1]), // 21
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), //  3
    NodeState(&Nodes[2], Data[2], node_color::BLACK, 1, &Data[2]), // 32
    {},
    NodeState(&Nodes[3], Data[3], node_color::RED,   1, &Data[3]), // 15
  };
  RedBlackTreeInsert(&Tree, &Nodes[3]);
  AssertTree(&Tree, 4, ArrayCount(State_3), State_3);


  // Testing RL Case
  node_state State_4[] = 
  {
    NodeState(&Nodes[1], Data[1], node_color::BLACK, 1, &Data[1]), // 21
    NodeState(&Nodes[4], Data[4], node_color::BLACK, 1, &Data[4]), // 12
    NodeState(&Nodes[2], Data[2], node_color::BLACK, 1, &Data[2]), // 32
    NodeState(&Nodes[0], Data[0], node_color::RED,   1, &Data[0]), //  3
    NodeState(&Nodes[3], Data[3], node_color::RED,   1, &Data[3]), // 15
  };
  RedBlackTreeInsert(&Tree, &Nodes[4]);
  AssertTree(&Tree, 5, ArrayCount(State_4), State_4);

  node_state State_5[] = 
  {
    NodeState(&Nodes[1], Data[1], node_color::BLACK, 1, &Data[1]), // 21
    NodeState(&Nodes[4], Data[4], node_color::RED,   1, &Data[4]), // 12
    NodeState(&Nodes[2], Data[2], node_color::BLACK, 1, &Data[2]), // 32
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 3
    NodeState(&Nodes[3], Data[3], node_color::BLACK, 1, &Data[3]), // 15
    {}, {},
    {}, {},
    NodeState(&Nodes[5], Data[5], node_color::RED,   1, &Data[5]), // 13
  };
  RedBlackTreeInsert(&Tree, &Nodes[5]); // No Rotation, Recoloring
  AssertTree(&Tree, 6, ArrayCount(State_5), State_5);

  node_state State_6[] = 
  {
    NodeState(&Nodes[1], Data[1], node_color::BLACK, 1, &Data[1]), // 21
    NodeState(&Nodes[4], Data[4], node_color::RED,   1, &Data[4]), // 12
    NodeState(&Nodes[2], Data[2], node_color::BLACK, 1, &Data[2]), // 32
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 3
    NodeState(&Nodes[6], Data[6], node_color::BLACK, 1, &Data[6]), // 14
    {}, {},
    {}, {},
    NodeState(&Nodes[5], Data[5], node_color::RED,   1, &Data[5]), // 13
    NodeState(&Nodes[3], Data[3], node_color::RED, 1, &Data[3]),   // 15
  };
  RedBlackTreeInsert(&Tree, &Nodes[6]); // No Rotation, Recoloring
  AssertTree(&Tree, 7, ArrayCount(State_6), State_6);

  node_state State_7[] = 
  {
    NodeState(&Nodes[ 1], Data[ 1], node_color::BLACK, 1, &Data[1]),  // 21
    NodeState(&Nodes[ 4], Data[ 4], node_color::RED,   1, &Data[4]),  // 12
    NodeState(&Nodes[ 2], Data[ 2], node_color::BLACK, 1, &Data[2]),  // 32
    NodeState(&Nodes[ 0], Data[ 0], node_color::BLACK, 1, &Data[0]),  // 3
    NodeState(&Nodes[ 6], Data[ 6], node_color::BLACK, 1, &Data[6]),  // 14
    {},{},
    NodeState(&Nodes[ 7], Data[ 7], node_color::RED,   1, &Data[ 7]), // 1
    {},
    NodeState(&Nodes[ 5], Data[ 5], node_color::RED,   1, &Data[ 5]), // 13
    NodeState(&Nodes[ 3], Data[ 3], node_color::RED,   1, &Data[ 3]), // 15
  };
  RedBlackTreeInsert(&Tree, &Nodes[7]);  // 1
  AssertTree(&Tree, 8, ArrayCount(State_7), State_7);


  node_state State_8[] = 
  {
    NodeState(&Nodes[ 1], Data[ 1], node_color::BLACK, 1, &Data[1]),  // 21
    NodeState(&Nodes[ 4], Data[ 4], node_color::RED,   1, &Data[4]),  // 12
    NodeState(&Nodes[ 2], Data[ 2], node_color::BLACK, 1, &Data[2]),  // 32
    NodeState(&Nodes[ 0], Data[ 0], node_color::BLACK, 1, &Data[0]),  // 3
    NodeState(&Nodes[ 6], Data[ 6], node_color::BLACK, 1, &Data[6]),  // 14
    {},{},
    NodeState(&Nodes[ 7], Data[ 7], node_color::RED,   1, &Data[ 7]), // 1
    NodeState(&Nodes[ 8], Data[ 8], node_color::RED,   1, &Data[ 8]), // 5
    NodeState(&Nodes[ 5], Data[ 5], node_color::RED,   1, &Data[ 5]), // 13
    NodeState(&Nodes[ 3], Data[ 3], node_color::RED,   1, &Data[ 3]), // 15
  };
  RedBlackTreeInsert(&Tree, &Nodes[8]);  // 5
  AssertTree(&Tree, 9, ArrayCount(State_8), State_8);
  
  node_state State_9[] = 
  {
    NodeState(&Nodes[ 1], Data[ 1], node_color::BLACK, 1, &Data[1]),  // 21
    NodeState(&Nodes[ 4], Data[ 4], node_color::RED,   1, &Data[4]),  // 12
    NodeState(&Nodes[ 2], Data[ 2], node_color::BLACK, 1, &Data[2]),  // 32
    NodeState(&Nodes[ 0], Data[ 0], node_color::BLACK, 1, &Data[0]),  // 3
    NodeState(&Nodes[ 6], Data[ 6], node_color::BLACK, 1, &Data[6]),  // 14
    {},
    NodeState(&Nodes[ 9], Data[ 9], node_color::RED,   1, &Data[ 9]), // 37
    NodeState(&Nodes[ 7], Data[ 7], node_color::RED,   1, &Data[ 7]), // 1
    NodeState(&Nodes[ 8], Data[ 8], node_color::RED,   1, &Data[ 8]), // 5
    NodeState(&Nodes[ 5], Data[ 5], node_color::RED,   1, &Data[ 5]), // 13
    NodeState(&Nodes[ 3], Data[ 3], node_color::RED,   1, &Data[ 3]), // 15
  };
  RedBlackTreeInsert(&Tree, &Nodes[9]);  // 37
  AssertTree(&Tree, 10, ArrayCount(State_9), State_9);

  node_state State_10[] = 
  {
    NodeState(&Nodes[ 1], Data[ 1], node_color::BLACK, 1, &Data[1]),  // 21
    NodeState(&Nodes[ 4], Data[ 4], node_color::RED,   1, &Data[4]),  // 12
    NodeState(&Nodes[ 2], Data[ 2], node_color::BLACK, 1, &Data[2]),  // 32
    NodeState(&Nodes[ 0], Data[ 0], node_color::BLACK, 1, &Data[0]),  // 3
    NodeState(&Nodes[ 6], Data[ 6], node_color::BLACK, 1, &Data[6]),  // 14
    NodeState(&Nodes[10], Data[10], node_color::RED,   1, &Data[10]), // 25
    NodeState(&Nodes[ 9], Data[ 9], node_color::RED,   1, &Data[ 9]), // 37
    NodeState(&Nodes[ 7], Data[ 7], node_color::RED,   1, &Data[ 7]), // 1
    NodeState(&Nodes[ 8], Data[ 8], node_color::RED,   1, &Data[ 8]), // 5
    NodeState(&Nodes[ 5], Data[ 5], node_color::RED,   1, &Data[ 5]), // 13
    NodeState(&Nodes[ 3], Data[ 3], node_color::RED,   1, &Data[ 3]), // 15
  };
  RedBlackTreeInsert(&Tree, &Nodes[10]); // 25
  AssertTree(&Tree, 11, ArrayCount(State_10), State_10);
  
  node_state State_11[] = 
  {
    NodeState(&Nodes[ 1], Data[ 1], node_color::BLACK, 1, &Data[1]),  // 21
    NodeState(&Nodes[ 4], Data[ 4], node_color::RED,   1, &Data[4]),  // 12
    NodeState(&Nodes[ 2], Data[ 2], node_color::RED,   1, &Data[2]),  // 32
    NodeState(&Nodes[ 0], Data[ 0], node_color::BLACK, 1, &Data[0]),  // 3
    NodeState(&Nodes[ 6], Data[ 6], node_color::BLACK, 1, &Data[6]),  // 14
    NodeState(&Nodes[10], Data[10], node_color::BLACK, 1, &Data[10]), // 25
    NodeState(&Nodes[ 9], Data[ 9], node_color::BLACK, 1, &Data[ 9]), // 37
    NodeState(&Nodes[ 7], Data[ 7], node_color::RED,   1, &Data[ 7]), // 1
    NodeState(&Nodes[ 8], Data[ 8], node_color::RED,   1, &Data[ 8]), // 5
    NodeState(&Nodes[ 5], Data[ 5], node_color::RED,   1, &Data[ 5]), // 13
    NodeState(&Nodes[ 3], Data[ 3], node_color::RED,   1, &Data[ 3]), // 15
    {},{},{},
    NodeState(&Nodes[11], Data[11], node_color::RED,   1, &Data[11]), // 40

  };
  RedBlackTreeInsert(&Tree, &Nodes[11]); // 40
  AssertTree(&Tree, 12, ArrayCount(State_11), State_11);
  
  node_state State_12[] = 
  {
    NodeState(&Nodes[ 1], Data[ 1], node_color::BLACK, 1, &Data[1]),  // 21
    NodeState(&Nodes[ 4], Data[ 4], node_color::RED,   1, &Data[4]),  // 12
    NodeState(&Nodes[ 2], Data[ 2], node_color::RED,   1, &Data[2]),  // 32
    NodeState(&Nodes[ 0], Data[ 0], node_color::BLACK, 1, &Data[0]),  // 3
    NodeState(&Nodes[ 6], Data[ 6], node_color::BLACK, 1, &Data[6]),  // 14
    NodeState(&Nodes[10], Data[10], node_color::BLACK, 1, &Data[10]), // 25
    NodeState(&Nodes[12], Data[12], node_color::BLACK, 1, &Data[12]), // 38
    NodeState(&Nodes[ 7], Data[ 7], node_color::RED,   1, &Data[ 7]), // 1
    NodeState(&Nodes[ 8], Data[ 8], node_color::RED,   1, &Data[ 8]), // 5
    NodeState(&Nodes[ 5], Data[ 5], node_color::RED,   1, &Data[ 5]), // 13
    NodeState(&Nodes[ 3], Data[ 3], node_color::RED,   1, &Data[ 3]), // 15
    {},{},
    NodeState(&Nodes[ 9], Data[ 9], node_color::RED,   1, &Data[ 9]), // 37
    NodeState(&Nodes[11], Data[11], node_color::RED,   1, &Data[11]), // 40
  };
  RedBlackTreeInsert(&Tree, &Nodes[12]); // 38
  AssertTree(&Tree, 13, ArrayCount(State_12), State_12);
  
  node_state State_13[] = 
  {
    NodeState(&Nodes[ 1], Data[ 1], node_color::BLACK, 1, &Data[1]),  // 21
    NodeState(&Nodes[ 4], Data[ 4], node_color::RED,   1, &Data[4]),  // 12
    NodeState(&Nodes[ 2], Data[ 2], node_color::RED,   1, &Data[2]),  // 32
    NodeState(&Nodes[ 0], Data[ 0], node_color::BLACK, 1, &Data[0]),  // 3
    NodeState(&Nodes[ 6], Data[ 6], node_color::BLACK, 1, &Data[6]),  // 14
    NodeState(&Nodes[10], Data[10], node_color::BLACK, 1, &Data[10]), // 25
    NodeState(&Nodes[12], Data[12], node_color::BLACK, 1, &Data[12]), // 38
    NodeState(&Nodes[ 7], Data[ 7], node_color::RED,   1, &Data[ 7]), // 1
    NodeState(&Nodes[ 8], Data[ 8], node_color::RED,   1, &Data[ 8]), // 5
    NodeState(&Nodes[ 5], Data[ 5], node_color::RED,   1, &Data[ 5]), // 13
    NodeState(&Nodes[ 3], Data[ 3], node_color::RED,   1, &Data[ 3]), // 15
    {},
    NodeState(&Nodes[13], Data[13], node_color::RED,   1, &Data[13]), // 28
    NodeState(&Nodes[ 9], Data[ 9], node_color::RED,   1, &Data[ 9]), // 37
    NodeState(&Nodes[11], Data[11], node_color::RED,   1, &Data[11]), // 40
  };
  RedBlackTreeInsert(&Tree, &Nodes[13]); // 28
  AssertTree(&Tree, 14, ArrayCount(State_13), State_13);
  
  node_state State_14[] = 
  {
    NodeState(&Nodes[ 1], Data[ 1], node_color::BLACK, 1, &Data[1]),  // 21
    NodeState(&Nodes[ 4], Data[ 4], node_color::RED,   1, &Data[4]),  // 12
    NodeState(&Nodes[ 2], Data[ 2], node_color::RED,   1, &Data[2]),  // 32
    NodeState(&Nodes[ 0], Data[ 0], node_color::BLACK, 1, &Data[0]),  // 3
    NodeState(&Nodes[ 6], Data[ 6], node_color::BLACK, 1, &Data[6]),  // 14
    NodeState(&Nodes[10], Data[10], node_color::BLACK, 1, &Data[10]), // 25
    NodeState(&Nodes[12], Data[12], node_color::BLACK, 1, &Data[12]), // 38
    NodeState(&Nodes[ 7], Data[ 7], node_color::RED,   1, &Data[ 7]), // 1
    NodeState(&Nodes[ 8], Data[ 8], node_color::RED,   1, &Data[ 8]), // 5
    NodeState(&Nodes[ 5], Data[ 5], node_color::RED,   1, &Data[ 5]), // 13
    NodeState(&Nodes[ 3], Data[ 3], node_color::RED,   1, &Data[ 3]), // 15
    NodeState(&Nodes[14], Data[14], node_color::RED,   1, &Data[14]), // 23
    NodeState(&Nodes[13], Data[13], node_color::RED,   1, &Data[13]), // 28
    NodeState(&Nodes[ 9], Data[ 9], node_color::RED,   1, &Data[ 9]), // 37
    NodeState(&Nodes[11], Data[11], node_color::RED,   1, &Data[11]), // 40
  };
  RedBlackTreeInsert(&Tree, &Nodes[14]); // 23
  AssertTree(&Tree, 15, ArrayCount(State_14), State_14);


  node_state State_15[] = 
  {
    NodeState(&Nodes[ 1], Data[ 1], node_color::BLACK, 1, &Data[1]),  // 21
    NodeState(&Nodes[ 4], Data[ 4], node_color::BLACK, 1, &Data[4]),  // 12
    NodeState(&Nodes[ 2], Data[ 2], node_color::BLACK, 1, &Data[2]),  // 32
    NodeState(&Nodes[ 0], Data[ 0], node_color::BLACK, 1, &Data[0]),  // 3
    NodeState(&Nodes[ 6], Data[ 6], node_color::BLACK, 1, &Data[6]),  // 14
    NodeState(&Nodes[10], Data[10], node_color::RED,   1, &Data[10]), // 25
    NodeState(&Nodes[12], Data[12], node_color::BLACK, 1, &Data[12]), // 38
    NodeState(&Nodes[ 7], Data[ 7], node_color::RED,   1, &Data[ 7]), // 1
    NodeState(&Nodes[ 8], Data[ 8], node_color::RED,   1, &Data[ 8]), // 5
    NodeState(&Nodes[ 5], Data[ 5], node_color::RED,   1, &Data[ 5]), // 13
    NodeState(&Nodes[ 3], Data[ 3], node_color::RED,   1, &Data[ 3]), // 15
    NodeState(&Nodes[14], Data[14], node_color::BLACK, 1, &Data[14]), // 23
    NodeState(&Nodes[13], Data[13], node_color::BLACK, 1, &Data[13]), // 28
    NodeState(&Nodes[ 9], Data[ 9], node_color::RED,   1, &Data[ 9]), // 37
    NodeState(&Nodes[11], Data[11], node_color::RED,   1, &Data[11]), // 40
    {},{},{},{},{},{},{},{},
    NodeState(&Nodes[15], Data[15], node_color::RED,   1, &Data[15]), // 22
  };
  RedBlackTreeInsert(&Tree, &Nodes[15]); // 24
  AssertTree(&Tree, 16, ArrayCount(State_15), State_15);

}


void TestDeletionSimpleCase1()
{  
  red_black_tree Tree = NewRedBlackTree(); 
  //             0   1   2   3
  int Data[] = {30, 20, 40, 10}; 
  red_black_tree_node_data Datum[32] = {};
  red_black_tree_node Nodes[32] = {};
  for (int Index = 0; Index < ArrayCount(Data); ++Index)
  {
    Datum[Index] = NewRedBlackTreeNodeData((void*)&Data[Index]);
    Nodes[Index] = NewRedBlackTreeNode(Data[Index], &Datum[Index]);
  }

  // Testing Adding root
  node_state State_0[] = 
  {
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 30
    NodeState(&Nodes[1], Data[1], node_color::BLACK, 1, &Data[1]), // 20
    NodeState(&Nodes[2], Data[2], node_color::BLACK, 1, &Data[2]), // 40
    NodeState(&Nodes[3], Data[3], node_color::RED,   1, &Data[3]), // 10
  };

  RedBlackTreeInsert(&Tree, &Nodes[0]);
  RedBlackTreeInsert(&Tree, &Nodes[1]);
  RedBlackTreeInsert(&Tree, &Nodes[2]);
  RedBlackTreeInsert(&Tree, &Nodes[3]);

  AssertTree(&Tree, 4, ArrayCount(State_0), State_0);

  RedBlackTreeDelete(&Tree, 20);  // Case with 0 nodes

  // Testing Adding root
  node_state State_1[] = 
  {
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 30
    NodeState(&Nodes[3], Data[3], node_color::BLACK, 1, &Data[3]), // 10
    NodeState(&Nodes[2], Data[2], node_color::BLACK, 1, &Data[2]), // 40
  };
  AssertTree(&Tree, 3, ArrayCount(State_1), State_1);
}

void TestDeletionSimpleCase2()
{  
  red_black_tree Tree = NewRedBlackTree(); 
  //             0   1   2   3
  int Data[] = {30, 20, 40, 25}; 
  red_black_tree_node_data Datum[32] = {};
  red_black_tree_node Nodes[32] = {};
  for (int Index = 0; Index < ArrayCount(Data); ++Index)
  {
    Datum[Index] = NewRedBlackTreeNodeData((void*)&Data[Index]);
    Nodes[Index] = NewRedBlackTreeNode(Data[Index], &Datum[Index]);
  }

  // Testing Adding root
  node_state State_0[] = 
  {
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 30
    NodeState(&Nodes[1], Data[1], node_color::BLACK, 1, &Data[1]), // 20
    NodeState(&Nodes[2], Data[2], node_color::BLACK, 1, &Data[2]), // 40
    {},
    NodeState(&Nodes[3], Data[3], node_color::RED,   1, &Data[3]), // 25
  };

  RedBlackTreeInsert(&Tree, &Nodes[0]);
  RedBlackTreeInsert(&Tree, &Nodes[1]);
  RedBlackTreeInsert(&Tree, &Nodes[2]);
  RedBlackTreeInsert(&Tree, &Nodes[3]);

  AssertTree(&Tree, 4, ArrayCount(State_0), State_0);

  RedBlackTreeDelete(&Tree, 20);  // Case with 0 nodes

  // Testing Adding root
  node_state State_1[] = 
  {
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 30
    NodeState(&Nodes[3], Data[3], node_color::BLACK, 1, &Data[3]), // 25
    NodeState(&Nodes[2], Data[2], node_color::BLACK, 1, &Data[2]), // 40
  };
  AssertTree(&Tree, 3, ArrayCount(State_1), State_1);
}

void TestDeletionSimpleCase3()
{  
  red_black_tree Tree = NewRedBlackTree(); 
  //             0   1   2   3
  int Data[] = {30, 20, 40, 35}; 
  red_black_tree_node_data Datum[32] = {};
  red_black_tree_node Nodes[32] = {};
  for (int Index = 0; Index < ArrayCount(Data); ++Index)
  {
    Datum[Index] = NewRedBlackTreeNodeData((void*)&Data[Index]);
    Nodes[Index] = NewRedBlackTreeNode(Data[Index], &Datum[Index]);
  }

  // Testing Adding root
  node_state State_0[] = 
  {
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 30
    NodeState(&Nodes[1], Data[1], node_color::BLACK, 1, &Data[1]), // 20
    NodeState(&Nodes[2], Data[2], node_color::BLACK, 1, &Data[2]), // 40
    {},{},
    NodeState(&Nodes[3], Data[3], node_color::RED,   1, &Data[3]), // 35
  };

  RedBlackTreeInsert(&Tree, &Nodes[0]);
  RedBlackTreeInsert(&Tree, &Nodes[1]);
  RedBlackTreeInsert(&Tree, &Nodes[2]);
  RedBlackTreeInsert(&Tree, &Nodes[3]);

  AssertTree(&Tree, 4, ArrayCount(State_0), State_0);

  RedBlackTreeDelete(&Tree, 40);  // Case with 0 nodes

  // Testing Adding root
  node_state State_1[] = 
  {
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 30
    NodeState(&Nodes[1], Data[1], node_color::BLACK, 1, &Data[1]), // 20
    NodeState(&Nodes[3], Data[3], node_color::BLACK, 1, &Data[3]), // 35
  };
  AssertTree(&Tree, 3, ArrayCount(State_1), State_1);
}

void TestDeletionSimpleCase4()
{  
  red_black_tree Tree = NewRedBlackTree(); 
  //             0   1   2   3
  int Data[] = {30, 20, 40, 45}; 
  red_black_tree_node_data Datum[32] = {};
  red_black_tree_node Nodes[32] = {};
  for (int Index = 0; Index < ArrayCount(Data); ++Index)
  {
    Datum[Index] = NewRedBlackTreeNodeData((void*)&Data[Index]);
    Nodes[Index] = NewRedBlackTreeNode(Data[Index], &Datum[Index]);
  }

  // Testing Adding root
  node_state State_0[] = 
  {
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 30
    NodeState(&Nodes[1], Data[1], node_color::BLACK, 1, &Data[1]), // 20
    NodeState(&Nodes[2], Data[2], node_color::BLACK, 1, &Data[2]), // 40
    {},{},{},
    NodeState(&Nodes[3], Data[3], node_color::RED,   1, &Data[3]), // 45
  };

  RedBlackTreeInsert(&Tree, &Nodes[0]);
  RedBlackTreeInsert(&Tree, &Nodes[1]);
  RedBlackTreeInsert(&Tree, &Nodes[2]);
  RedBlackTreeInsert(&Tree, &Nodes[3]);

  AssertTree(&Tree, 4, ArrayCount(State_0), State_0);

  RedBlackTreeDelete(&Tree, 40);  // Case with 0 nodes

  // Testing Adding root
  node_state State_1[] = 
  {
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 30
    NodeState(&Nodes[1], Data[1], node_color::BLACK, 1, &Data[1]), // 20
    NodeState(&Nodes[3], Data[3], node_color::BLACK, 1, &Data[3]), // 45
  };
  AssertTree(&Tree, 3, ArrayCount(State_1), State_1);
}


void TestDeletionSiblingIsBlackOneRedChild_RR()
{  
  red_black_tree Tree = NewRedBlackTree(); 
  //             0   1   2   3
  int Data[] = {30, 20, 40, 50}; 
  red_black_tree_node_data Datum[32] = {};
  red_black_tree_node Nodes[32] = {};
  for (int Index = 0; Index < ArrayCount(Data); ++Index)
  {
    Datum[Index] = NewRedBlackTreeNodeData((void*)&Data[Index]);
    Nodes[Index] = NewRedBlackTreeNode(Data[Index], &Datum[Index]);
  }

  // Testing Adding root
  node_state State_0[] = 
  {
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 30
    NodeState(&Nodes[1], Data[1], node_color::BLACK, 1, &Data[1]), // 20
    NodeState(&Nodes[2], Data[2], node_color::BLACK, 1, &Data[2]), // 40
    {},{},{},
    NodeState(&Nodes[3], Data[3], node_color::RED,   1, &Data[3]), // 50
  };

  RedBlackTreeInsert(&Tree, &Nodes[0]);
  RedBlackTreeInsert(&Tree, &Nodes[1]);
  RedBlackTreeInsert(&Tree, &Nodes[2]);
  RedBlackTreeInsert(&Tree, &Nodes[3]);

  AssertTree(&Tree, 4, ArrayCount(State_0), State_0);

  RedBlackTreeDelete(&Tree, 20);  // Case with 0 nodes

  // Testing Adding root
  node_state State_1[] = 
  {
    NodeState(&Nodes[2], Data[2], node_color::BLACK, 1, &Data[2]), // 40
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 30
    NodeState(&Nodes[3], Data[3], node_color::BLACK, 1, &Data[3]), // 50
  };
  AssertTree(&Tree, 3, ArrayCount(State_1), State_1);
}


void TestDeletionSiblingIsBlackOneRedChild_RL()
{  
  red_black_tree Tree = NewRedBlackTree(); 
  //             0   1   2   3
  int Data[] = {30, 20, 40, 35}; 
  red_black_tree_node_data Datum[32] = {};
  red_black_tree_node Nodes[32] = {};
  for (int Index = 0; Index < ArrayCount(Data); ++Index)
  {
    Datum[Index] = NewRedBlackTreeNodeData((void*)&Data[Index]);
    Nodes[Index] = NewRedBlackTreeNode(Data[Index], &Datum[Index]);
  }

  // Testing Adding root
  node_state State_0[] = 
  {
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 30
    NodeState(&Nodes[1], Data[1], node_color::BLACK, 1, &Data[1]), // 20
    NodeState(&Nodes[2], Data[2], node_color::BLACK, 1, &Data[2]), // 40
    {},{},
    NodeState(&Nodes[3], Data[3], node_color::RED,   1, &Data[3]), // 35
  };

  RedBlackTreeInsert(&Tree, &Nodes[0]);
  RedBlackTreeInsert(&Tree, &Nodes[1]);
  RedBlackTreeInsert(&Tree, &Nodes[2]);
  RedBlackTreeInsert(&Tree, &Nodes[3]);

  AssertTree(&Tree, 4, ArrayCount(State_0), State_0);

  RedBlackTreeDelete(&Tree, 20);  // Case with 0 nodes

  // Testing Adding root
  node_state State_1[] = 
  {
    NodeState(&Nodes[3], Data[3], node_color::BLACK, 1, &Data[3]), // 35
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 30
    NodeState(&Nodes[2], Data[2], node_color::BLACK, 1, &Data[2]), // 40
  };
  AssertTree(&Tree, 3, ArrayCount(State_1), State_1);
}



void TestDeletionSiblingIsBlackOneRedChild_LR()
{  
  red_black_tree Tree = NewRedBlackTree(); 
  //             0   1   2   3
  int Data[] = {30, 20, 40, 25}; 
  red_black_tree_node_data Datum[32] = {};
  red_black_tree_node Nodes[32] = {};
  for (int Index = 0; Index < ArrayCount(Data); ++Index)
  {
    Datum[Index] = NewRedBlackTreeNodeData((void*)&Data[Index]);
    Nodes[Index] = NewRedBlackTreeNode(Data[Index], &Datum[Index]);
  }

  // Testing Adding root
  node_state State_0[] = 
  {
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 30
    NodeState(&Nodes[1], Data[1], node_color::BLACK, 1, &Data[1]), // 20
    NodeState(&Nodes[2], Data[2], node_color::BLACK, 1, &Data[2]), // 40
    {},
    NodeState(&Nodes[3], Data[3], node_color::RED,   1, &Data[3]), // 25
  };

  RedBlackTreeInsert(&Tree, &Nodes[0]);
  RedBlackTreeInsert(&Tree, &Nodes[1]);
  RedBlackTreeInsert(&Tree, &Nodes[2]);
  RedBlackTreeInsert(&Tree, &Nodes[3]);

  AssertTree(&Tree, 4, ArrayCount(State_0), State_0);

  RedBlackTreeDelete(&Tree, 40);  // Case with 0 nodes

  // Testing Adding root
  node_state State_1[] = 
  {
    NodeState(&Nodes[3], Data[3], node_color::BLACK, 1, &Data[3]), // 25
    NodeState(&Nodes[1], Data[1], node_color::BLACK, 1, &Data[1]), // 20
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 30
  };
  AssertTree(&Tree, 3, ArrayCount(State_1), State_1);
}


void TestDeletionSiblingIsBlackOneRedChild_LL()
{  
  red_black_tree Tree = NewRedBlackTree(); 
  //             0   1   2   3
  int Data[] = {30, 20, 40, 15}; 
  red_black_tree_node_data Datum[32] = {};
  red_black_tree_node Nodes[32] = {};
  for (int Index = 0; Index < ArrayCount(Data); ++Index)
  {
    Datum[Index] = NewRedBlackTreeNodeData((void*)&Data[Index]);
    Nodes[Index] = NewRedBlackTreeNode(Data[Index], &Datum[Index]);
  }

  // Testing Adding root
  node_state State_0[] = 
  {
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 30
    NodeState(&Nodes[1], Data[1], node_color::BLACK, 1, &Data[1]), // 20
    NodeState(&Nodes[2], Data[2], node_color::BLACK, 1, &Data[2]), // 40
    NodeState(&Nodes[3], Data[3], node_color::RED,   1, &Data[3]), // 15
  };

  RedBlackTreeInsert(&Tree, &Nodes[0]);
  RedBlackTreeInsert(&Tree, &Nodes[1]);
  RedBlackTreeInsert(&Tree, &Nodes[2]);
  RedBlackTreeInsert(&Tree, &Nodes[3]);

  AssertTree(&Tree, 4, ArrayCount(State_0), State_0);

  RedBlackTreeDelete(&Tree, 40);  // Case with 0 nodes

  // Testing Adding root
  node_state State_1[] = 
  {
    NodeState(&Nodes[1], Data[1], node_color::BLACK, 1, &Data[1]), // 20
    NodeState(&Nodes[3], Data[3], node_color::BLACK, 1, &Data[3]), // 15
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 30
  };
  AssertTree(&Tree, 3, ArrayCount(State_1), State_1);
}


void TestDeletionSiblingIsBlackTwoRedChildren_RR()
{  
  red_black_tree Tree = NewRedBlackTree(); 
  //             0   1   2   3   4
  int Data[] = {30, 20, 40, 45, 35}; 
  red_black_tree_node_data Datum[32] = {};
  red_black_tree_node Nodes[32] = {};
  for (int Index = 0; Index < ArrayCount(Data); ++Index)
  {
    Datum[Index] = NewRedBlackTreeNodeData((void*)&Data[Index]);
    Nodes[Index] = NewRedBlackTreeNode(Data[Index], &Datum[Index]);
  }

  // Testing Adding root
  node_state State_0[] = 
  {
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 30
    NodeState(&Nodes[1], Data[1], node_color::BLACK, 1, &Data[1]), // 20
    NodeState(&Nodes[2], Data[2], node_color::BLACK, 1, &Data[2]), // 40
    {},{},
    NodeState(&Nodes[4], Data[4], node_color::RED,   1, &Data[4]), // 35
    NodeState(&Nodes[3], Data[3], node_color::RED,   1, &Data[3]), // 45
  };

  RedBlackTreeInsert(&Tree, &Nodes[0]);
  RedBlackTreeInsert(&Tree, &Nodes[1]);
  RedBlackTreeInsert(&Tree, &Nodes[2]);
  RedBlackTreeInsert(&Tree, &Nodes[3]);
  RedBlackTreeInsert(&Tree, &Nodes[4]);

  AssertTree(&Tree, 5, ArrayCount(State_0), State_0);

  RedBlackTreeDelete(&Tree, 20);  // Case with 0 nodes

  // Testing Adding root
  node_state State_1[] = 
  {
    NodeState(&Nodes[2], Data[2], node_color::BLACK, 1, &Data[2]), // 40
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 30
    NodeState(&Nodes[3], Data[3], node_color::BLACK, 1, &Data[3]), // 45
    {},
    NodeState(&Nodes[4], Data[4], node_color::RED,   1, &Data[4]), // 35
  };
  AssertTree(&Tree, 4, ArrayCount(State_1), State_1);
}


void RedBlackTreeUnitTest()
{
  TestInsertion();
  
  TestDeletionSimpleCase1();
  TestDeletionSimpleCase2();
  TestDeletionSimpleCase3();
  TestDeletionSimpleCase4();

  TestDeletionSiblingIsBlackOneRedChild_LL();
  TestDeletionSiblingIsBlackOneRedChild_RR();
  TestDeletionSiblingIsBlackOneRedChild_RL();
  TestDeletionSiblingIsBlackOneRedChild_LR();

  TestDeletionSiblingIsBlackTwoRedChildren_RR();

  //RedBlackTreeDelete(&Tree, 21);  // Case with 2 nodes
  //RedBlackTreeDelete(&Tree, 15);  // Case with 2 nodes but replacement tree has a sub tree
  //RedBlackTreeDelete(&Tree, 3);   // Case with 0 nodes
  //RedBlackTreeDelete(&Tree, 12);  // Case with 1 nodes

}