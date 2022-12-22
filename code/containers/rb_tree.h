#pragma once
#include "containers/red_black_tree.h"
#include "containers/chunk_list.h"
#include "memory.h"
#include "types.h"

struct rb_tree
{
  red_black_tree Tree;
  chunk_list Nodes;
  chunk_list Data;
  memory_arena* Arena;
};

// Interface Function:
rb_tree NewRBTree(memory_arena* Arena, u32 ApproxNodeCount, u32 ApproxDataCount)
{
  rb_tree Tree = {};
  Tree.Nodes = NewChunkList(Arena, sizeof(red_black_tree_node), ApproxNodeCount);
  Tree.Data = NewChunkList(Arena, sizeof(red_black_tree_node_data), ApproxDataCount);
  Tree.Arena = Arena;
  return Tree;
}

void DeleteRBTree(rb_tree* Tree)
{
  Clear(&Tree->Nodes);
  Clear(&Tree->Data);
  *Tree = {};
}

void Insert(rb_tree* Tree, u32 Key, void* Data)
{
  red_black_tree_node* NewNode = (red_black_tree_node*) GetNewBlock(Tree->Arena, &Tree->Nodes);
  red_black_tree_node_data* NewData = (red_black_tree_node_data*) GetNewBlock(Tree->Arena, &Tree->Data);
  *NewData = NewRedBlackTreeNodeData(Data);
  *NewNode = NewRedBlackTreeNode(Key, NewData);
  
  b32 NodeInserted = RedBlackTreeInsert(&Tree->Tree, NewNode);
  if(!NodeInserted)
  {
    // Free the node memory if the node was already in the tree
    // Data was added however
    FreeBlock(&Tree->Nodes, (bptr)NewNode);
  }
}

void Delete(rb_tree* Tree, int Key)
{
  red_black_tree_node* DeletedNode = RedBlackTreeDelete(&Tree->Tree, Key);
  if(DeletedNode)
  {
    red_black_tree_node_data* NodeData = DeletedNode->Data;
    while(NodeData)
    {
      red_black_tree_node_data* NextData = NodeData->Next;
      FreeBlock(&Tree->Data, (bptr)NodeData);
      NodeData = NextData;
    }
    FreeBlock(&Tree->Nodes, (bptr)DeletedNode);
  }
}

// IndexCount returns the total number of data held by a node
// DataIndex  specifies which data in the list we want to get
// Returns pointer to the data contained in the node
void* Find(rb_tree* Tree, int Key, u32 DataIndex = 0, u32* DataCount = 0)
{
  red_black_tree_node* Node = RedBlackTreeFind(&Tree->Tree, Key);
  void* Result = 0;
  if(Node)
  {
    u32 Index = 0;

    red_black_tree_node_data* Data = Node->Data;
    while(Data)
    {
      if(Index == DataIndex)
      {
        Result = Data->Data;
        if(!DataCount)
        {
          break;
        }
      }
      Index++;
      Data = Data->Next;
    }
    if(DataCount)
    {
      *DataCount = Index;
    }
  }
  return Result;
}

u32 NodeCount(rb_tree* Tree)
{
  u32 Result = Tree->Tree.NodeCount;
  return Result;
}