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

void PrintFunction(red_black_tree_node const * Node, void* CustomData)
{
  Platform.DEBUGPrint("%2d %s, ", Node->Key, Node->Color == node_color::RED ? "R" : "B");
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

node_assert_data CreateGroundTruth(int ArrayCount, node_assert_data_element* Elements, int* Color, int* Data)
{
  node_assert_data GroundTruth = {};
  GroundTruth.Index = 0;
  GroundTruth.ElementCount = ArrayCount;
  GroundTruth.Elements = Elements;
  for (int Index = 0; Index < ArrayCount; ++Index)
  {
    Elements[Index] = CreateAssertElement(Data[Index], Color[Index] == 0 ? node_color::RED : node_color::BLACK, 1, &Data[Index]);
  }
  return GroundTruth;
}

void BigTest()
{
  // Big Insertion

  int InsertionKeys[] = {
    72,66,91,57,58,80,24,59, 3,78,
    49,46,14,23, 8,34,63,97,60,65,
    51, 6,36,56,55,32,85,42,95,96,
     9,76,37,17,43,82,93,16,11,53,
    88,22,40,21,54,27,26,25,33,61,
     2,67,31,47,87,18,39,79,00,86,
    10,74,38,83, 5,44,69,73,64,70,
    45,89,35,19,92,62,15,28,81,50,
    12,75,98,41,90, 7,94,52,84,99,
    48,13,20,29,71,68, 4,77, 1,30,
  };

  red_black_tree_node_data Datum[100] = {};
  red_black_tree_node Nodes[100] = {};
  for (int Index = 0; Index < ArrayCount(InsertionKeys); ++Index)
  {
    Datum[Index] = NewRedBlackTreeNodeData( (void*)  &InsertionKeys[Index]);
    Nodes[Index] = NewRedBlackTreeNode(InsertionKeys[Index], &Datum[Index]);
  }

  red_black_tree Tree = NewRedBlackTree(); 
  for (int Index = 0; Index < 10; ++Index)
  {
    RedBlackTreeInsert(&Tree, &Nodes[Index]);  
  }

  // State 1
  int PostOrderColor_1[] = { 0, 0, 1, 0, 1, 0, 0, 0, 1, 1};
  int PostOrderData_1[]  = { 3,57,24,59,66,58,78,91,80,72};
  node_assert_data_element PostOrderElements_1[10] = {};
  node_assert_data PostOrderGroundTruth_1 = CreateGroundTruth(ArrayCount(PostOrderElements_1), PostOrderElements_1, PostOrderColor_1, PostOrderData_1);

  PostOrderTraverse(&Tree, (void*) &PostOrderGroundTruth_1, TraverseAssertFunction);



  for (int Index = 10; Index < 20; ++Index)
  {
    RedBlackTreeInsert(&Tree, &Nodes[Index]);  
  }

  // State 2
  int PostOrderColor_2[] = { 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1};
  int PostOrderData_2[]  = { 8, 3,23,14,34,46,57,49,24,60,59,65,66,63,78,97,91,80,72,58};
  node_assert_data_element PostOrderElements_2[20] = {};
  node_assert_data PostOrderGroundTruth_2 = CreateGroundTruth(ArrayCount(PostOrderElements_2), PostOrderElements_2, PostOrderColor_2, PostOrderData_2);
  PostOrderTraverse(&Tree, (void*) &PostOrderGroundTruth_2, TraverseAssertFunction);  


  for (int Index = 20; Index < 30; ++Index)
  {
    RedBlackTreeInsert(&Tree, &Nodes[Index]);  
  }

  // State 3
  int PostOrderColor_3[] = { 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1};
  int PostOrderData_3[]  = { 3, 8, 6,23,14,32,34,42,46,36,55,51,57,56,49,24,60,59,65,66,63,78,85,95,97,96,91,80,72,58};
  node_assert_data_element PostOrderElements_3[30] = {};
  node_assert_data PostOrderGroundTruth_3 = CreateGroundTruth(ArrayCount(PostOrderElements_3), PostOrderElements_3, PostOrderColor_3, PostOrderData_3);
  PostOrderTraverse(&Tree, (void*) &PostOrderGroundTruth_3, TraverseAssertFunction);  


  for (int Index = 30; Index < 100; ++Index)
  {
    RedBlackTreeInsert(&Tree, &Nodes[Index]);  
  }

  // State Final
  int PostOrderColor_Final[] = {0,  1,  0,  0,  1,  1,  0,  1,  1,  0,  1,  0,  1,  0,  0,  1,  1,  0,  0,  1,  1,  0,  1,  1,  1,  0,  0,  0,  1,  0,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  0,  0,  1,  0,  1,  0,  1,  0,  0,  1,  0,  1,  0,  1,  1,  1,  0,  1,  0,  1,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  0,  1,  0,  1,  1,  1,  1,  0,  1,  0,  1,  0,  0,  1,  0,  1,  1,  0,  1,  0,  0,  0,  1,  1,  1,  0,  1};
  int PostOrderData_Final[]  = {1,  0,  3,  5,  4,  2,  7,  8, 10, 13, 12, 11,  9,  6, 15, 16, 18, 17, 20, 21, 23, 22, 19, 25, 27, 26, 29, 31, 30, 33, 35, 34, 32, 28, 24, 14, 38, 37, 41, 40, 39, 43, 45, 44, 48, 47, 46, 42, 50, 52, 51, 54, 55, 53, 57, 56, 49, 36, 59, 62, 61, 60, 64, 65, 68, 67, 71, 70, 69, 66, 63, 73, 75, 77, 76, 74, 79, 78, 72, 81, 84, 83, 82, 86, 88, 90, 89, 87, 85, 92, 94, 95, 93, 97, 99, 98, 96, 91, 80, 58};
  node_assert_data_element PostOrderElements_Final[100] = {};
  node_assert_data PostOrderGroundTruth_Final = CreateGroundTruth(ArrayCount(PostOrderElements_Final), PostOrderElements_Final, PostOrderColor_Final, PostOrderData_Final);
  PostOrderTraverse(&Tree, (void*) &PostOrderGroundTruth_Final, TraverseAssertFunction);


  // Big Deletion in steps of 10;

  int DeletionSequence_0[]      = {16, 75, 10, 52, 24, 99, 74,  3, 56, 70};
  int DeletionSequence_1[]      = {28, 96, 65, 14, 50, 20, 43, 30, 93, 62};
  int DeletionSequence_2[]      = {35, 26, 34, 11,  8, 17,  0, 44, 59, 94};
  int DeletionSequence_3[]      = {81, 76, 97, 84, 38, 83, 87, 42,  9, 22};
  int DeletionSequence_4[]      = {78, 33,  2, 29, 63, 45, 32, 71, 91, 79};
  int DeletionSequence_5[]      = {18, 49, 60, 85, 23, 66, 69, 39, 72, 36};
  int DeletionSequence_6[]      = { 7, 46, 90, 68, 92, 37,  1, 88, 25, 58};
  int DeletionSequence_7[]      = {61,  4, 73, 51, 31, 86, 95, 80, 67, 41};
  int DeletionSequence_7_5[]    = {40,  6, 27, 47, 13};
  int DeletionSequence_8[]      = {19, 55, 77, 15, 12};
  int DeletionSequence_8_5[]    = {48,  5, 98, 57, 21};
  int DeletionSequence_Final[]  = {89, 53, 54, 82, 64};


  red_black_tree_node* DeletedNodes_0[10] = {};
  for (int Index = 0; Index < ArrayCount(DeletionSequence_0); ++Index)
  {
    DeletedNodes_0[Index] = RedBlackTreeDelete(&Tree, DeletionSequence_0[Index]);
    AssertNodeData(DeletedNodes_0[Index]->Data, 1, &DeletionSequence_0[Index]);
  }



  // DeleteState 3
  int PostOrderColorDelete_0[] = {0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1};
  int PostOrderDataDelete_0[] =  {1, 0, 5, 4, 2, 7, 8,11,13,12, 9, 6,15,18,17,20,22,21,19,25,27,26,29,31,30,33,35,34,32,28,23,14,38,37,41,40,39,43,45,44,48,47,46,42,50,51,54,53,57,55,49,36,59,62,61,60,64,65,68,67,71,69,66,63,73,77,76,79,78,72,81,84,83,82,86,88,90,89,87,85,92,94,95,93,97,98,96,91,80,58};
  node_assert_data_element PostOrderElementsDelete_0[90] = {};
  node_assert_data PostOrderGroundTruthDelete_0 = CreateGroundTruth(ArrayCount(PostOrderElementsDelete_0), PostOrderElementsDelete_0, PostOrderColorDelete_0, PostOrderDataDelete_0);
  PostOrderTraverse(&Tree, (void*) &PostOrderGroundTruthDelete_0, TraverseAssertFunction);  

  red_black_tree_node* DeletedNodes_1[10] = {};
  for (int Index = 0; Index < ArrayCount(DeletionSequence_1); ++Index)
  {
    DeletedNodes_1[Index] = RedBlackTreeDelete(&Tree, DeletionSequence_1[Index]);
    AssertNodeData(DeletedNodes_1[Index]->Data, 1, &DeletionSequence_1[Index]);
  }

  int PostOrderColorDelete_1[] = {0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1};
  int PostOrderDataDelete_1[] =  {1, 0, 5, 4, 2, 7, 8,11,12, 9, 6,15,18,17,22,21,19,25,26,31,29,33,35,34,32,27,23,13,38,37,41,40,39,45,44,48,47,46,42,51,54,53,57,55,49,36,59,61,60,64,68,67,71,69,66,63,73,77,76,79,78,72,81,84,83,82,86,88,90,89,87,85,94,92,97,98,95,91,80,58};
  node_assert_data_element PostOrderElementsDelete_1[80] = {};
  node_assert_data PostOrderGroundTruthDelete_1 = CreateGroundTruth(ArrayCount(PostOrderElementsDelete_1), PostOrderElementsDelete_1, PostOrderColorDelete_1, PostOrderDataDelete_1);
  PostOrderTraverse(&Tree, (void*) &PostOrderGroundTruthDelete_1, TraverseAssertFunction);  

  red_black_tree_node* DeletedNodes_2[10] = {};
  for (int Index = 0; Index < ArrayCount(DeletionSequence_2); ++Index)
  {
    DeletedNodes_2[Index] = RedBlackTreeDelete(&Tree, DeletionSequence_2[Index]);
    AssertNodeData(DeletedNodes_2[Index]->Data, 1, &DeletionSequence_2[Index]);
  }


  int PostOrderColorDelete_2[] = {1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1};
  int PostOrderDataDelete_2[] =  {1, 5, 4, 2, 7,12, 9, 6,18,15,22,21,19,25,31,29,33,32,27,23,13,38,37,41,40,39,45,48,47,46,42,51,54,53,57,55,49,36,61,60,64,63,68,67,71,69,66,73,77,76,79,78,72,81,84,83,82,86,88,90,89,87,85,92,97,98,95,91,80,58};
  node_assert_data_element PostOrderElementsDelete_2[70] = {};
  node_assert_data PostOrderGroundTruthDelete_2 = CreateGroundTruth(ArrayCount(PostOrderElementsDelete_2), PostOrderElementsDelete_2, PostOrderColorDelete_2, PostOrderDataDelete_2);
  PostOrderTraverse(&Tree, (void*) &PostOrderGroundTruthDelete_2, TraverseAssertFunction);  

  red_black_tree_node* DeletedNodes_3[10] = {};
  for (int Index = 0; Index < ArrayCount(DeletionSequence_3); ++Index)
  {
    DeletedNodes_3[Index] = RedBlackTreeDelete(&Tree, DeletionSequence_3[Index]);
    AssertNodeData(DeletedNodes_3[Index]->Data, 1, &DeletionSequence_3[Index]);
  }
   
  int PostOrderColorDelete_3[] = {1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1};
  int PostOrderDataDelete_3[] =  {1, 5, 4, 2,12, 7, 6,18,15,21,19,25,31,29,33,32,27,23,13,37,40,39,45,48,47,46,41,51,54,53,57,55,49,36,61,60,64,63,68,67,71,69,66,77,73,79,78,72,82,88,86,90,89,85,92,98,95,91,80,58};
  node_assert_data_element PostOrderElementsDelete_3[60] = {};
  node_assert_data PostOrderGroundTruthDelete_3 = CreateGroundTruth(ArrayCount(PostOrderElementsDelete_3), PostOrderElementsDelete_3, PostOrderColorDelete_3, PostOrderDataDelete_3);
  PostOrderTraverse(&Tree, (void*) &PostOrderGroundTruthDelete_3, TraverseAssertFunction);  

  red_black_tree_node* DeletedNodes_4[10] = {};
  for (int Index = 0; Index < ArrayCount(DeletionSequence_4); ++Index)
  {
    DeletedNodes_4[Index] = RedBlackTreeDelete(&Tree, DeletionSequence_4[Index]);
    AssertNodeData(DeletedNodes_4[Index]->Data, 1, &DeletionSequence_4[Index]);
  }
  int PostOrderColorDelete_4[] = {1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1};
  int PostOrderDataDelete_4[] =  {1, 5, 4,12, 7, 6,18,15,21,19,25,31,27,23,13,37,40,39,46,48,47,41,51,54,53,57,55,49,36,60,64,61,67,69,68,73,77,72,66,82,86,89,88,85,92,98,95,90,80,58};
  node_assert_data_element PostOrderElementsDelete_4[50] = {};
  node_assert_data PostOrderGroundTruthDelete_4 = CreateGroundTruth(ArrayCount(PostOrderElementsDelete_4), PostOrderElementsDelete_4, PostOrderColorDelete_4, PostOrderDataDelete_4);
  PostOrderTraverse(&Tree, (void*) &PostOrderGroundTruthDelete_4, TraverseAssertFunction);  

  red_black_tree_node* DeletedNodes_5[10] = {};
  for (int Index = 0; Index < ArrayCount(DeletionSequence_5); ++Index)
  {
    DeletedNodes_5[Index] = RedBlackTreeDelete(&Tree, DeletionSequence_5[Index]);
    AssertNodeData(DeletedNodes_5[Index]->Data, 1, &DeletionSequence_5[Index]);
  }

  int PostOrderColorDelete_5[] = {1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1};
  int PostOrderDataDelete_5[] =  {1, 5, 4,12, 7, 6,15,19,25,27,21,13,40,37,46,47,41,51,54,53,57,55,48,31,61,67,64,77,73,68,86,82,89,88,92,98,95,90,80,58};
  node_assert_data_element PostOrderElementsDelete_5[40] = {};
  node_assert_data PostOrderGroundTruthDelete_5 = CreateGroundTruth(ArrayCount(PostOrderElementsDelete_5), PostOrderElementsDelete_5, PostOrderColorDelete_5, PostOrderDataDelete_5);
  PostOrderTraverse(&Tree, (void*) &PostOrderGroundTruthDelete_5, TraverseAssertFunction);  

  red_black_tree_node* DeletedNodes_6[10] = {};
  for (int Index = 0; Index < ArrayCount(DeletionSequence_6); ++Index)
  {
    DeletedNodes_6[Index] = RedBlackTreeDelete(&Tree, DeletionSequence_6[Index]);
    AssertNodeData(DeletedNodes_6[Index]->Data, 1, &DeletionSequence_6[Index]);
  }
  
  int PostOrderColorDelete_6[] = {0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1};
  int PostOrderDataDelete_6[] =  {5, 4,12, 6,15,19,27,21,13,40,47,41,51,54,55,53,48,31,61,64,77,73,67,82,86,98,95,89,80,57};
  node_assert_data_element PostOrderElementsDelete_6[30] = {};
  node_assert_data PostOrderGroundTruthDelete_6 = CreateGroundTruth(ArrayCount(PostOrderElementsDelete_6), PostOrderElementsDelete_6, PostOrderColorDelete_6, PostOrderDataDelete_6);
  PostOrderTraverse(&Tree, (void*) &PostOrderGroundTruthDelete_6, TraverseAssertFunction);  

  red_black_tree_node* DeletedNodes_7[10] = {};
  for (int Index = 0; Index < ArrayCount(DeletionSequence_7); ++Index)
  {
    DeletedNodes_7[Index] = RedBlackTreeDelete(&Tree, DeletionSequence_7[Index]);
    AssertNodeData(DeletedNodes_7[Index]->Data, 1, &DeletionSequence_7[Index]);
  }
  
  int PostOrderColorDelete_7[] = {1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1};
  int PostOrderDataDelete_7[] =  {5,12, 6,15,21,19,13,47,40,53,55,54,48,64,82,98,89,77,57,27};
  node_assert_data_element PostOrderElementsDelete_7[20] = {};
  node_assert_data PostOrderGroundTruthDelete_7 = CreateGroundTruth(ArrayCount(PostOrderElementsDelete_7), PostOrderElementsDelete_7, PostOrderColorDelete_7, PostOrderDataDelete_7);
  PostOrderTraverse(&Tree, (void*) &PostOrderGroundTruthDelete_7, TraverseAssertFunction);  

  red_black_tree_node* DeletedNodes_7_5[5] = {};
  for (int Index = 0; Index < ArrayCount(DeletionSequence_7_5); ++Index)
  {
    DeletedNodes_7_5[Index] = RedBlackTreeDelete(&Tree, DeletionSequence_7_5[Index]);
    AssertNodeData(DeletedNodes_7_5[Index]->Data, 1, &DeletionSequence_7_5[Index]);
  }

  int PostOrderColorDelete_7_5[] = {1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1};
  int PostOrderDataDelete_7_5[] =  {5,15,19,12,53,48,55,54,64,82,98,89,77,57,21};
  node_assert_data_element PostOrderElementsDelete_7_5[15] = {};
  node_assert_data PostOrderGroundTruthDelete_7_5 = CreateGroundTruth(ArrayCount(PostOrderElementsDelete_7_5), PostOrderElementsDelete_7_5, PostOrderColorDelete_7_5, PostOrderDataDelete_7_5);
  PostOrderTraverse(&Tree, (void*) &PostOrderGroundTruthDelete_7_5, TraverseAssertFunction);  

  red_black_tree_node* DeletedNodes_8[5] = {};
  for (int Index = 0; Index < ArrayCount(DeletionSequence_8); ++Index)
  {
    DeletedNodes_8[Index] = RedBlackTreeDelete(&Tree, DeletionSequence_8[Index]);
    AssertNodeData(DeletedNodes_8[Index]->Data, 1, &DeletionSequence_8[Index]);
  }

  int PostOrderColorDelete_8[] = {1, 1, 1, 0, 1, 0, 1, 1, 1, 1};
  int PostOrderDataDelete_8[] =  {5,48,54,53,21,82,64,98,89,57};
  node_assert_data_element PostOrderElementsDelete_8[10] = {};
  node_assert_data PostOrderGroundTruthDelete_8 = CreateGroundTruth(ArrayCount(PostOrderElementsDelete_8), PostOrderElementsDelete_8, PostOrderColorDelete_8, PostOrderDataDelete_8);
  PostOrderTraverse(&Tree, (void*) &PostOrderGroundTruthDelete_8, TraverseAssertFunction);  

  red_black_tree_node* DeletedNodes_8_5[5] = {};
  for (int Index = 0; Index < ArrayCount(DeletionSequence_8_5); ++Index)
  {
    DeletedNodes_8_5[Index] = RedBlackTreeDelete(&Tree, DeletionSequence_8_5[Index]);
    AssertNodeData(DeletedNodes_8_5[Index]->Data, 1, &DeletionSequence_8_5[Index]);
  }

  int PostOrderColorDelete_8_5[] = { 1, 1, 1, 0, 1};
  int PostOrderDataDelete_8_5[] =  {53,64,89,82,54};
  node_assert_data_element PostOrderElementsDelete_8_5[10] = {};
  node_assert_data PostOrderGroundTruthDelete_8_5 = CreateGroundTruth(ArrayCount(PostOrderElementsDelete_8_5), PostOrderElementsDelete_8_5, PostOrderColorDelete_8_5, PostOrderDataDelete_8_5);
  PostOrderTraverse(&Tree, (void*) &PostOrderGroundTruthDelete_8_5, TraverseAssertFunction);  

  int PostOrderColorDelete_Final[5][4] =
  {
    { 1, 0, 1, 1},
    { 1, 1, 1,-1},
    { 0, 1,-1,-1},
    { 1,-1,-1,-1},
    {-1,-1,-1,-1},
   };
  int PostOrderDataDelete_Final[5][4] =  
  {
    {53,64,82,54},
    {54,82,64,-1},
    {82,64,-1,-1},
    {64,-1,-1,-1},
    {-1,-1,-1,-1},
  };

  for (int Index = 0; Index < ArrayCount(DeletionSequence_Final); ++Index)
  {
    red_black_tree_node* DeletedNodes_Final = RedBlackTreeDelete(&Tree, DeletionSequence_Final[Index]);
    AssertNodeData(DeletedNodes_Final->Data, 1, &DeletionSequence_Final[Index]);
    node_assert_data_element PostOrderElementsDelete_Final[4] = {};
    node_assert_data PostOrderGroundTruthDelete_Final = 
      CreateGroundTruth(ArrayCount(DeletionSequence_Final) - Index - 1, PostOrderElementsDelete_Final, PostOrderColorDelete_Final[Index], PostOrderDataDelete_Final[Index]);
    PostOrderTraverse(&Tree, (void*) &PostOrderGroundTruthDelete_Final, TraverseAssertFunction);  
  }

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

  int InsertionKeys[] = { 3, 21, 32, 15, 12, 13, 14,  1,  5, 37, 25, 40, 38, 28, 23, 22};
  
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
  
  // --==Pre Order Traversal (N, L, R) ==--
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
  midx MemorySize = PreOrderGetStackMemorySize(&Tree);
  void* TraverseStack = PushSize(GlobalGameState->TransientArena, MemorySize);
  PreOrderTraverse(&Tree, TraverseStack, (void*) &PreOrderGroundTruth, TraverseAssertFunction);
  EndTemporaryMemory(TempMem);

  node_assert_data InOrderGroundTruth = {};
  InOrderGroundTruth.Index = 0;
  InOrderGroundTruth.ElementCount = 16;
  InOrderGroundTruth.Elements = InOrderElements;
  InOrderTraverse(&Tree, (void*) &InOrderGroundTruth, TraverseAssertFunction);
}


struct data_assert_struct
{
  int UniqueKeyCount;
  int* UniqueKeys;
  int* ValueCount;
  int** Values;
};

void DataAssertFunction(red_black_tree_node const * Node, void* CustomData)
{
  data_assert_struct* DataAssert = (data_assert_struct*) CustomData;

  int KeyIndex = 0;
  while(Node->Key != DataAssert->UniqueKeys[KeyIndex])
  {
    KeyIndex++;
    RedBlackTreeAssert(KeyIndex < DataAssert->UniqueKeyCount);
  }

  int* Values = DataAssert->Values[KeyIndex];
  int ValueCount = DataAssert->ValueCount[KeyIndex];
  AssertNodeData(Node->Data, ValueCount, Values);
}

// Verifies that the node pointer is no longer in the tree, IE, it has been removed
void NodeRemovedAssertFunction(red_black_tree_node const * Node, void* CustomData)
{
  red_black_tree_node* CurrentDeletedNode = (red_black_tree_node*) CustomData;
  RedBlackTreeAssert(CurrentDeletedNode != Node);
}


void TestInsertingSeveralOfSameKey()
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
  //  Data Value          0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
  int InsertionKeys[] = { 3, 21, 32, 15, 12, 13, 14,  1,  5, 37, 25, 40, 38, 28, 23, 22,
  //  Data Value                 16          17          18      19          20      21
                                 32,         13,          5,     25,         28,     22,
  //  Data Value                 22          23                  24
                                 32,         13,                 25,
  //  Data Value                 25                              26
                                 32,                             25,
  //  Data Value                                                 27
                                                                 25};

  int InsertionKeysUnique[] = { 3, 21, 32, 15, 12, 13, 14,  1,  5, 37, 25, 40, 38, 28, 23, 22};

  int InsertionDataGroundTruthValues[16*5] = 
  {
// Key       3                21                32                15
       0,-1,-1,-1,-1,    1,-1,-1,-1,-1,   25,22,16, 2,-1,    3,-1,-1,-1,-1,
// Key      12                13                14                 1
       4,-1,-1,-1,-1,   23,17, 5,-1,-1,    6,-1,-1,-1,-1,    7,-1,-1,-1,-1,
// Key        5               37                25                40
       18, 8,-1,-1,-1,   9,-1,-1,-1,-1,   27,26,24,19,10,   11,-1,-1,-1,-1,
// Key       38               28                23                22
       12,-1,-1,-1,-1,  20,13,-1,-1,-1,   14,-1,-1,-1,-1,   21,15,-1,-1,-1
  };
  int* InsertionDataGroundTruth[16] = {};
  for (int Index = 0; Index < 16; ++Index)
  {
    InsertionDataGroundTruth[Index] = InsertionDataGroundTruthValues + 5*Index;
  }

  int InsertionValueCountGroundTruth[16] = 
  {
// Key 3 21 32 15 12 13 14  1  5 37 25 40 38 28 23 22
       1, 1, 4, 1, 1, 3, 1, 1, 2, 1, 5, 1, 1, 2, 1, 2
  };

  data_assert_struct DataAssert = {};
  DataAssert.UniqueKeyCount = ArrayCount(InsertionKeysUnique);
  DataAssert.UniqueKeys = InsertionKeysUnique;
  DataAssert.ValueCount = InsertionValueCountGroundTruth;
  DataAssert.Values  = InsertionDataGroundTruth;


  int InsertionData[28] = {};
  red_black_tree_node_data Datum[28] = {};
  red_black_tree_node Nodes[28] = {};
  for (int Index = 0; Index < ArrayCount(InsertionKeys); ++Index)
  {
    InsertionData[Index] = Index;
    Datum[Index] = NewRedBlackTreeNodeData( (void*)  &InsertionData[Index]);
    Nodes[Index] = NewRedBlackTreeNode(InsertionKeys[Index], &Datum[Index]);
  }

  red_black_tree Tree = NewRedBlackTree();
  for (int Index = 0; Index < ArrayCount(InsertionKeys); ++Index)
  {
    bool NewNodeAdded = RedBlackTreeInsert(&Tree, &Nodes[Index]);
    if(Index < 16)
    {
      // Node was inserted
      RedBlackTreeAssert(NewNodeAdded);
    }else{
      // Node was not inserted, only data
      RedBlackTreeAssert(!NewNodeAdded);
    } 
  }

  // Test deleting a value that does not exist in the tree
  int NodeCount = Tree.NodeCount;
  red_black_tree_node* NullNode = RedBlackTreeDelete(&Tree, 100);
  RedBlackTreeAssert(!NullNode && Tree.NodeCount == NodeCount);

  // Test finding a node that does not exist
  NullNode = RedBlackTreeFind(&Tree, 8);
  RedBlackTreeAssert(!NullNode);

  // Test finding a value that exists
  red_black_tree_node* NotNUllNode = RedBlackTreeFind(&Tree, 25);
  RedBlackTreeAssert(NotNUllNode);
  AssertNodeData(NotNUllNode->Data, 5, InsertionDataGroundTruth[10]);


  // Delete nodes that exist and verify it has been removed and that the data returned is what we expect
  for (int Index = 0; Index < ArrayCount(InsertionKeysUnique); ++Index)
  {
    red_black_tree_node* DeletedNode = RedBlackTreeDelete(&Tree, InsertionKeysUnique[Index]);
    InOrderTraverse(&Tree, (void*) DeletedNode, NodeRemovedAssertFunction);
    InOrderTraverse(&Tree, (void*) &DataAssert, DataAssertFunction);
  }

  RedBlackTreeAssert(Tree.NodeCount == 0);
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

  TestTraverse();
  TestInsertingSeveralOfSameKey();
  BigTest();
}
