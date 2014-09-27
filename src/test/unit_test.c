#include <stdio.h>
#include <CUnit/CUnit.h>
#include <CUnit/Console.h>

//-----------------------------------------------------------------------------
// $B%W%m%H%?%$%W@k8@(B
//-----------------------------------------------------------------------------
void sort(int array[], int num);
void test_sort_001(void);
void test_sort_002(void);
void test_sort_003(void);
void test_sort_004(void);
void test_sort_005(void);

void test_main() {
  printf("test\n");
  CU_pSuite sort_suite;

  CU_initialize_registry();
  sort_suite = CU_add_suite("Sort", NULL, NULL);
  CU_add_test(sort_suite, "test_001", test_sort_001);
  CU_add_test(sort_suite, "test_002", test_sort_002);
  CU_add_test(sort_suite, "test_003", test_sort_003);
  CU_add_test(sort_suite, "test_004", test_sort_004);
  CU_add_test(sort_suite, "test_005", test_sort_005);
  CU_basic_run_tests();
  CU_cleanup_registry();

  return(0);
}

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
