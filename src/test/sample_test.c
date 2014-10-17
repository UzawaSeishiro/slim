#include "sample_test.h"

//-----------------------------------------------------------------------------
// $B%F%9%HBP>]$N%=!<%H4X?t(B
//-----------------------------------------------------------------------------
void sort(int array[], int num) {
  int i;
  int j;
  int val;

  for(i=0; i<(num-1); i++) {
    for(j=(num-1); j>i; j--) {
      if (array[j-1] > array[j]) {
        val = array[j];
        array[j] = array[j-1];
        array[j-1] = val;
      }
    }
  }
}

//-----------------------------------------------------------------------------
// $B%F%9%H4X?t#1(B
//-----------------------------------------------------------------------------
void test_sort_001(void) {
  int array[] = {3};

  sort(array, 1);
  CU_ASSERT(array[0] == 3);
}

//-----------------------------------------------------------------------------
// $B%F%9%H4X?t#2(B
//-----------------------------------------------------------------------------
void test_sort_002(void) {
  int array[] = {11, 7, 5, 3, 2};

  sort(array, 5);
  CU_ASSERT(array[0] == 2);
  CU_ASSERT(array[1] == 3);
  CU_ASSERT(array[2] == 5);
  CU_ASSERT(array[3] == 7);
  CU_ASSERT(array[4] == 11);
}

//-----------------------------------------------------------------------------
// $B%F%9%H4X?t#3(B
//-----------------------------------------------------------------------------
void test_sort_003(void) {
  int array[] = {7, 11, 3, 2, 5};

  sort(array, 5);
  CU_ASSERT(array[0] == 2);
  CU_ASSERT(array[1] == 3);
  CU_ASSERT(array[2] == 5);
  CU_ASSERT(array[3] == 7);
  CU_ASSERT(array[4] == 11);
}

//-----------------------------------------------------------------------------
// $B%F%9%H4X?t#4(B
//-----------------------------------------------------------------------------
void test_sort_004(void) {
  int array[] = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1};

  sort(array, 10);
  CU_ASSERT(array[0] == 1);
  CU_ASSERT(array[1] == 2);
  CU_ASSERT(array[2] == 3);
  CU_ASSERT(array[3] == 4);
  CU_ASSERT(array[4] == 5);
  CU_ASSERT(array[5] == 6);
  CU_ASSERT(array[6] == 7);
  CU_ASSERT(array[7] == 8);
  CU_ASSERT(array[8] == 9);
  CU_ASSERT(array[9] == 10);
}

//-----------------------------------------------------------------------------
// $B%F%9%H4X?t#5(B
//-----------------------------------------------------------------------------
void test_sort_005(void) {
  int array[] = {2, 9, 3, 6, 10, 5, 8, 4, 1, 7};

  sort(array, 10);
  CU_ASSERT(array[0] == 1);
  CU_ASSERT(array[1] == 2);
  CU_ASSERT(array[2] == 3);
  CU_ASSERT(array[3] == 4);
  CU_ASSERT(array[4] == 5);
  CU_ASSERT(array[5] == 6);
  CU_ASSERT(array[6] == 7);
  CU_ASSERT(array[7] == 8);
  CU_ASSERT(array[8] == 9);
  CU_ASSERT(array[9] == 10);
}

//=============================================================================
// $B%U%!%$%k%(%s%I(B
