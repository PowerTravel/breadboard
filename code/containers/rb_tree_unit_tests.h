
#include "containers/rb_tree.h"

struct rb_tree_unit_test_data{
  u32 a;
  b32 b;
  u64 c;
};

void RunRBTreeUnitTests(memory_arena* Arena)
{
  rb_tree Tree = NewRBTree(Arena, 20, 40);

  rb_tree_unit_test_data* UnitTestData = PushArray(Arena,200, rb_tree_unit_test_data);
  for (u32 Index = 0; Index < 200; ++Index)
  {
    b32 Bool = Index % 2;
    rb_tree_unit_test_data* Data = UnitTestData + Index;
    Data->a = Index;
    Data->b = Bool;
    Data->c = Index;
  }

  // Insert
  for (u32 Index = 0; Index < 200; ++Index)
  {
    b32 Key = Index % 100;
    rb_tree_unit_test_data* Data = UnitTestData + Index;
    Insert(&Tree, Key, (void*) Data);
  }

  // Find
  for (u32 Index = 0; Index < 200; ++Index)
  {
    b32 Key = Index;
    u32 DataCount = 0;
    rb_tree_unit_test_data* Data1 = (rb_tree_unit_test_data*) Find(&Tree, Key);
    rb_tree_unit_test_data* Data2 = (rb_tree_unit_test_data*) Find(&Tree, Key, 1);
    rb_tree_unit_test_data* Data3 = (rb_tree_unit_test_data*) Find(&Tree, Key, 2, &DataCount);

    if(Index < 100)
    {
      Assert(DataCount == 2);
      Assert(Data1 == &UnitTestData[Index+100]);
      Assert(Data2 == &UnitTestData[Index]);
      Assert(!Data3);  
    }else{
      Assert(DataCount == 0);
      Assert(!Data1);
      Assert(!Data2);
      Assert(!Data3);  
    }
  }

  // Delete
  for (u32 Index = 0; Index < 100; ++Index)
  {
    b32 Key = Index;
    Delete(&Tree, Key);
    Assert(NodeCount(&Tree) == (99-Index));
  }

  Assert(NodeCount(&Tree) == 0);
  DeleteRBTree(&Tree);

  // Make sure data is untouched
  for (u32 Index = 0; Index < 200; ++Index)
  {
    b32 Bool = Index % 2;
    rb_tree_unit_test_data* Data = UnitTestData + Index;
    Assert(Data->a == Index);
    Assert(Data->b == Bool);
    Assert(Data->c == Index);
  }
}