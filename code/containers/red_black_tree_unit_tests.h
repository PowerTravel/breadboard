#include "containers/red_black_tree.h"


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

  for (int Index = 0; Index < ArrayCount(Data); ++Index)
  {
    RedBlackTreeInsert(&Tree, &Nodes[Index]);
  }

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

  for (int Index = 0; Index < ArrayCount(Data); ++Index)
  {
    RedBlackTreeInsert(&Tree, &Nodes[Index]);
  }

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

  for (int Index = 0; Index < ArrayCount(Data); ++Index)
  {
    RedBlackTreeInsert(&Tree, &Nodes[Index]);
  }

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

  for (int Index = 0; Index < ArrayCount(Data); ++Index)
  {
    RedBlackTreeInsert(&Tree, &Nodes[Index]);
  }

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

  for (int Index = 0; Index < ArrayCount(Data); ++Index)
  {
    RedBlackTreeInsert(&Tree, &Nodes[Index]);
  }

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

  for (int Index = 0; Index < ArrayCount(Data); ++Index)
  {
    RedBlackTreeInsert(&Tree, &Nodes[Index]);
  }

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

  for (int Index = 0; Index < ArrayCount(Data); ++Index)
  {
    RedBlackTreeInsert(&Tree, &Nodes[Index]);
  }

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

  for (int Index = 0; Index < ArrayCount(Data); ++Index)
  {
    RedBlackTreeInsert(&Tree, &Nodes[Index]);
  }

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

  for (int Index = 0; Index < ArrayCount(Data); ++Index)
  {
    RedBlackTreeInsert(&Tree, &Nodes[Index]);
  }

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


void TestDeletionRemoveRootWithZeroChildren()
{  
  red_black_tree Tree = NewRedBlackTree(); 
  //             0
  int Data[] = {30}; 
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
  };

  RedBlackTreeInsert(&Tree, &Nodes[0]);

  AssertTree(&Tree, 1, ArrayCount(State_0), State_0);

  RedBlackTreeDelete(&Tree, 30);  // Case with 1 nodes

  // Testing Adding root
  AssertTree(&Tree, 0, 0, {});
}


void TestDeletionRemoveRootWithOneChild()
{  
  red_black_tree Tree = NewRedBlackTree(); 
  //             0,  1
  int Data[] = {30, 20}; 
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
    NodeState(&Nodes[1], Data[1], node_color::RED,   1, &Data[1]), // 20
  };

  for (int Index = 0; Index < ArrayCount(Data); ++Index)
  {
    RedBlackTreeInsert(&Tree, &Nodes[Index]);
  }

  AssertTree(&Tree, 2, ArrayCount(State_0), State_0);

  RedBlackTreeDelete(&Tree, 30);  // Case with 2 nodes
  node_state State_1[] = 
  {
    NodeState(&Nodes[1], Data[1], node_color::BLACK, 1, &Data[1]), // 20
  };
  // Testing Adding root
  AssertTree(&Tree, 1, ArrayCount(State_1), State_1);
}

void TestDeletionRemoveRootWithTwoChildren()
{  
  red_black_tree Tree = NewRedBlackTree(); 
  //             0,  1   2
  int Data[] = {30, 20, 40}; 
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
    NodeState(&Nodes[1], Data[1], node_color::RED,   1, &Data[1]), // 20
    NodeState(&Nodes[2], Data[2], node_color::RED,   1, &Data[2]), // 40
  };

  for (int Index = 0; Index < ArrayCount(Data); ++Index)
  {
    RedBlackTreeInsert(&Tree, &Nodes[Index]);
  }

  AssertTree(&Tree, 3, ArrayCount(State_0), State_0);

  RedBlackTreeDelete(&Tree, 30);
  node_state State_1[] = 
  {
    // TODO: This is a hack, when deleting a node with two children, we store the data and key
    //        of a child node into a parent node. Which means Node[0] now holds the data of Data[1]
    //        True tree assert should not take the Nodes[0] vector, but traverse the tree.
    //        Fix that later once tree traversal function is done.
    NodeState(&Nodes[0], Data[1], node_color::BLACK, 1, &Data[1]), // Node 30, Data 20  
    {},
    NodeState(&Nodes[2], Data[2], node_color::RED,   1, &Data[2]), // 40
  };
  // Testing Adding root
  AssertTree(&Tree, 2, ArrayCount(State_1), State_1);
}


// Case 3: Double black sibling is black and both it's children are black
void TestDeletionCaseThree()
{  
  red_black_tree Tree = NewRedBlackTree(); 
  //             0,  1   2
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
    NodeState(&Nodes[3], Data[3], node_color::RED,   1, &Data[3]), // 40
  };

  for (int Index = 0; Index < ArrayCount(Data); ++Index)
  {
    RedBlackTreeInsert(&Tree, &Nodes[Index]);
  }

  AssertTree(&Tree, 4, ArrayCount(State_0), State_0);

  RedBlackTreeDelete(&Tree, 25);
  node_state State_1[] = 
  {
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 30
    NodeState(&Nodes[1], Data[1], node_color::BLACK, 1, &Data[1]), // 20
    NodeState(&Nodes[2], Data[2], node_color::BLACK, 1, &Data[2]), // 40
  };

  AssertTree(&Tree, 3, ArrayCount(State_1), State_1);

  RedBlackTreeDelete(&Tree, 20);
  node_state State_3[] = 
  {
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 30  
    {},
    NodeState(&Nodes[2], Data[2], node_color::RED,   1, &Data[2]), // 40
  };
  // Testing Adding root
  AssertTree(&Tree, 2, ArrayCount(State_3), State_3);
}


// Case 4: Double black sibling is black and both it's children are black
void TestDeletionCaseFour()
{  
  red_black_tree Tree = NewRedBlackTree(); 
  //             0,  1   2   3   4   5   6   7
  int Data[] = {30, 20, 40, 45, 35, 25, 15, 17};
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
    NodeState(&Nodes[1], Data[1], node_color::RED,   1, &Data[1]), // 20
    NodeState(&Nodes[2], Data[2], node_color::BLACK, 1, &Data[2]), // 40
    NodeState(&Nodes[6], Data[6], node_color::BLACK, 1, &Data[6]), // 15
    NodeState(&Nodes[5], Data[5], node_color::BLACK, 1, &Data[5]), // 25
    NodeState(&Nodes[4], Data[4], node_color::RED,   1, &Data[4]), // 35
    NodeState(&Nodes[3], Data[3], node_color::RED,   1, &Data[3]), // 45
    {},
    NodeState(&Nodes[7], Data[7], node_color::RED,   1, &Data[7]), // 47
  };

  for (int Index = 0; Index < ArrayCount(Data); ++Index)
  {
    RedBlackTreeInsert(&Tree, &Nodes[Index]);
  }

  AssertTree(&Tree, 8, ArrayCount(State_0), State_0);

  RedBlackTreeDelete(&Tree, 40);
  node_state State_1[] = 
  {
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 30
    NodeState(&Nodes[1], Data[1], node_color::RED,   1, &Data[1]), // 20
    NodeState(&Nodes[2], Data[4], node_color::BLACK, 1, &Data[4]), // Node 40, Data 35 Note: This is a hack, see TODO above
    NodeState(&Nodes[6], Data[6], node_color::BLACK, 1, &Data[6]), // 15
    NodeState(&Nodes[5], Data[5], node_color::BLACK, 1, &Data[5]), // 25
    {},
    NodeState(&Nodes[3], Data[3], node_color::RED,   1, &Data[3]), // 45
    {},
    NodeState(&Nodes[7], Data[7], node_color::RED,   1, &Data[7]), // 47
  };
  AssertTree(&Tree, 7, ArrayCount(State_1), State_1);

  RedBlackTreeDelete(&Tree, 45);
  node_state State_2[] = 
  {
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 30
    NodeState(&Nodes[1], Data[1], node_color::RED,   1, &Data[1]), // 20
    NodeState(&Nodes[2], Data[4], node_color::BLACK, 1, &Data[4]), // Node 40, Data 35 Note: This is a hack, see TODO above
    NodeState(&Nodes[6], Data[6], node_color::BLACK, 1, &Data[6]), // 15
    NodeState(&Nodes[5], Data[5], node_color::BLACK, 1, &Data[5]), // 25
    {},
    {},
    {},
    NodeState(&Nodes[7], Data[7], node_color::RED,   1, &Data[7]), // 47
  };
  AssertTree(&Tree, 6, ArrayCount(State_2), State_2);

  RedBlackTreeDelete(&Tree, 35);
  node_state State_3[] = 
  {
    NodeState(&Nodes[1], Data[1], node_color::BLACK, 1, &Data[1]), // 20
    NodeState(&Nodes[6], Data[6], node_color::BLACK, 1, &Data[6]), // 15
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 30
    {},
    NodeState(&Nodes[7], Data[7], node_color::RED,   1, &Data[7]), // 17
    NodeState(&Nodes[5], Data[5], node_color::RED,   1, &Data[5]), // 25
  };

  AssertTree(&Tree, 5, ArrayCount(State_3), State_3);
}

void TestDeletionCaseFiveSix()
{  
  red_black_tree Tree = NewRedBlackTree(); 
  //             0,  1   2   3
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

  for (int Index = 0; Index < ArrayCount(Data); ++Index)
  {
    RedBlackTreeInsert(&Tree, &Nodes[Index]);
  }

  AssertTree(&Tree, 4, ArrayCount(State_0), State_0);

  RedBlackTreeDelete(&Tree, 40);
  node_state State_1[] = 
  {
    NodeState(&Nodes[3], Data[3], node_color::BLACK, 1, &Data[3]), // 25
    NodeState(&Nodes[1], Data[1], node_color::BLACK, 1, &Data[1]), // 20
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 30
  };

  AssertTree(&Tree, 3, ArrayCount(State_1), State_1);
}


void TestDeletionCaseSixTwoRedChildren()
{  
  red_black_tree Tree = NewRedBlackTree(); 
  //             0,  1   2   3   4
  int Data[] = {30, 25, 35, 15, 27};
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
    NodeState(&Nodes[1], Data[1], node_color::BLACK, 1, &Data[1]), // 25
    NodeState(&Nodes[2], Data[2], node_color::BLACK, 1, &Data[2]), // 35
    NodeState(&Nodes[3], Data[3], node_color::RED,   1, &Data[3]), // 15
    NodeState(&Nodes[4], Data[4], node_color::RED,   1, &Data[4]), // 27
  };

  for (int Index = 0; Index < ArrayCount(Data); ++Index)
  {
    RedBlackTreeInsert(&Tree, &Nodes[Index]);
  }

  AssertTree(&Tree, 5, ArrayCount(State_0), State_0);

  RedBlackTreeDelete(&Tree, 35);
  node_state State_1[] = 
  {
    NodeState(&Nodes[1], Data[1], node_color::BLACK, 1, &Data[1]), // 25
    NodeState(&Nodes[3], Data[3], node_color::BLACK, 1, &Data[3]), // 15
    NodeState(&Nodes[0], Data[0], node_color::BLACK, 1, &Data[0]), // 30
    {},{},
    NodeState(&Nodes[4], Data[4], node_color::RED,   1, &Data[4]), // 27
  };

  AssertTree(&Tree, 4, ArrayCount(State_1), State_1);
}

void BigTest()
{
  int Data[] = {72,66,91,57,58,80,24,59, 3,78,
                49,46,14,23, 8,34,63,97,60,65,
                51, 6,36,56,55,32,85,42,95,96,
                 9,76,37,17,43,82,93,16,11,53,
                88,22,40,21,54,27,26,25,33,61, 
                 2,67,31,47,87,18,39,79,00,86,
                10,74,38,83, 5,44,69,73,64,70,
                45,89,35,19,92,62,15,28,81,50,
                12,75,98,41,90, 7,94,52,84,99,
                48,13,20,29,71,68, 4,77, 1,30};
  red_black_tree_node_data Datum[128] = {};
  red_black_tree_node Nodes[128] = {};
  for (int Index = 0; Index < ArrayCount(Data); ++Index)
  {
    Datum[Index] = NewRedBlackTreeNodeData((void*)&Data[Index]);
    Nodes[Index] = NewRedBlackTreeNode(Data[Index], &Datum[Index]);
  }
}

void PrintFunction(red_black_tree_node const * Node, void* CustomData)
{
  Platform.DEBUGPrint("%d %s, ", Node->Key, Node->Color == node_color::RED ? "R" : "B");
}

struct node_assert_data_element
{
  int Key;
  node_color Color;
  int DataCount;
  int* Data;
};

node_assert_data_element CreateAssertElement(int Key, node_color Color, int DataCount, int* Data)
{
  node_assert_data_element Result = {};
  Result.Key = Key;
  Result.Color = Color;
  Result.DataCount = DataCount;
  Result.Data =  Data;
  return Result;
}

struct node_assert_data
{
  int Index;
  int ElementCount;
  node_assert_data_element* Elements;
};

void TraverseAssertFunction(red_black_tree_node const * Node, void* CustomData)
{
  node_assert_data* GroundTruth = (node_assert_data*) CustomData;

  RedBlackTreeAssert(GroundTruth->Index < GroundTruth->ElementCount);
  node_assert_data_element* NodeTruth = GroundTruth->Elements + GroundTruth->Index;
  GroundTruth->Index++;
  RedBlackTreeAssert(NodeTruth->Key == Node->Key);
  RedBlackTreeAssert(NodeTruth->Color == Node->Color);
  red_black_tree_node_data const * NodeData = Node->Data;

  int GroundTruthIndex = 0;
  while(NodeData)
  {
    int NodeDataValue = *((int*) NodeData->Data);
    int GroundTruthValue = NodeTruth->Data[GroundTruthIndex];
    RedBlackTreeAssert(GroundTruthIndex < NodeTruth->DataCount);
    GroundTruthIndex++;
    RedBlackTreeAssert(GroundTruthValue == NodeDataValue);
    NodeData = NodeData->Next;
  }
  RedBlackTreeAssert(GroundTruthIndex == NodeTruth->DataCount);
}

void TestTraverse()
{
  
/*
 *              --==Final Tree structure==--
 *
 *              _____________21B___________    
 *             /                           \
 *       ___12B___                       ___32B___   
 *      /         \                     /         \
 *    3B           14B               25R           38B
 *   /  \         /   \             /   \         /   \
 *  1R   5R    13R     15R       23B     28B   37R     40R
 *                              /
 *                           22R
 */

  int InsertionKeys[] =      { 3, 21, 32, 15, 12, 13, 14,  1,  5, 37, 25, 40, 38, 28, 23, 22};
  
  red_black_tree_node_data Datum[16] = {};
  red_black_tree_node Nodes[16] = {};
  for (int Index = 0; Index < ArrayCount(InsertionKeys); ++Index)
  {
    Datum[Index] = NewRedBlackTreeNodeData( (void*)  &InsertionKeys[Index]);
    Nodes[Index] = NewRedBlackTreeNode(InsertionKeys[Index], &Datum[Index]);
  }

  // Testing Adding root
  red_black_tree Tree = NewRedBlackTree(); 
  for (int Index = 0; Index < ArrayCount(InsertionKeys); ++Index)
  {
    RedBlackTreeInsert(&Tree, &Nodes[Index]);  
  }

  // --==Post Order Traversal (L N R) ==--
  // 1 R, 5 R, 3 B, 13 R, 15 R, 14 B, 12 B, 22 R, 23 B, 28 B, 25 R, 37 R, 40 R, 38 B, 32 B, 21 B, 
  int PostOrderData[] =  { 1,  5,  3, 13, 15, 14, 12, 22, 23, 28, 25, 37, 40, 38, 32, 21};
  int PostOrderColor[] = { 0,  0,  1,  0,  0,  1,  1,  0,  1,  1,  0,  0,  0,  1,  1,  1};
  node_assert_data_element PostOrderElements[16] = {};
  
  // --==Pre Order Traversal (L R N) ==--
  // 21 B, 12 B, 3 B, 1 R, 5 R, 14 B, 13 R, 15 R, 32 B, 25 R, 23 B, 22 R, 28 B, 38 B, 37 R, 40 R, 
  int PreOrderData[] =  { 21, 12,  3,  1, 5, 14, 13, 15, 32, 25, 23, 22, 28, 38, 37, 40};
  int PreOrderColor[] = {  1,  1,  1,  0, 0,  1,  0,  0,  1,  0,  1,  0,  1,  1,  0,  0};
  node_assert_data_element PreOrderElements[16] = {};

  // --==In Order Traversal L R N==--
  // 1 R, 3 B, 5 R, 12 B, 13 R, 14 B, 15 R, 21 B, 22 R, 23 B, 25 R, 28 B, 32 B, 37 R, 38 B, 40 R, 
  int InOrderData[] =  { 1,  3,  5, 12, 13, 14, 15, 21, 22, 23, 25, 28, 32, 37, 38, 40};
  int InOrderColor[] = { 0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  1,  0,  1,  0};
  node_assert_data_element InOrderElements[16] = {};

  for (int Index = 0; Index < ArrayCount(PostOrderData); ++Index)
  {
    PostOrderElements[Index] = CreateAssertElement(PostOrderData[Index], PostOrderColor[Index] == 0 ? node_color::RED : node_color::BLACK, 1, &PostOrderData[Index]);
    PreOrderElements[Index]  = CreateAssertElement(PreOrderData[Index],  PreOrderColor[Index]  == 0 ? node_color::RED : node_color::BLACK, 1, &PreOrderData[Index]);
    InOrderElements[Index]   = CreateAssertElement(InOrderData[Index],   InOrderColor[Index]   == 0 ? node_color::RED : node_color::BLACK, 1, &InOrderData[Index]);
  }

  node_assert_data PostOrderGroundTruth = {};
  PostOrderGroundTruth.Index = 0;
  PostOrderGroundTruth.ElementCount = 16;
  PostOrderGroundTruth.Elements = PostOrderElements;
  PostOrderTraverse(&Tree, (void*) &PostOrderGroundTruth, TraverseAssertFunction);

  node_assert_data PreOrderGroundTruth = {};
  PreOrderGroundTruth.Index = 0;
  PreOrderGroundTruth.ElementCount = 16;
  PreOrderGroundTruth.Elements = PreOrderElements;
  temporary_memory TempMem = BeginTemporaryMemory(GlobalGameState->TransientArena);
  midx MemorySize = InOrderGetStackMemorySize(&Tree);
  void* TraverseStack = PushSize(GlobalGameState->TransientArena, MemorySize);
  PreOrderTraverse(&Tree, TraverseStack, (void*) &PreOrderGroundTruth, TraverseAssertFunction);
  EndTemporaryMemory(TempMem);

  node_assert_data InOrderGroundTruth = {};
  InOrderGroundTruth.Index = 0;
  InOrderGroundTruth.ElementCount = 16;
  InOrderGroundTruth.Elements = InOrderElements;
  InOrderTraverse(&Tree, (void*) &InOrderGroundTruth, TraverseAssertFunction);
}

void RedBlackTreeUnitTest()
{
  TestInsertion();
  
  TestDeletionRemoveRootWithZeroChildren();
  TestDeletionRemoveRootWithOneChild();
  TestDeletionRemoveRootWithTwoChildren();
  TestDeletionCaseThree();
  TestDeletionCaseFour();
  TestDeletionCaseFiveSix();
  TestDeletionCaseSixTwoRedChildren();

  TestDeletionSimpleCase1();
  TestDeletionSimpleCase2();
  TestDeletionSimpleCase3();
  TestDeletionSimpleCase4();

  TestDeletionSiblingIsBlackOneRedChild_LL();
  TestDeletionSiblingIsBlackOneRedChild_RR();
  TestDeletionSiblingIsBlackOneRedChild_RL();
  TestDeletionSiblingIsBlackOneRedChild_LR();

  TestDeletionSiblingIsBlackTwoRedChildren_RR();

  //BigTest();
  TestTraverse();
}

