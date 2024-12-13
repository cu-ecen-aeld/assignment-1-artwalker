#include "unity.h"
#include <stdbool.h>
#include <stdlib.h>
#include "../../examples/autotest-validate/autotest-validate.h"
#include "../../assignment-autotest/test/assignment1/username-from-conf-file.h"

/**
* This function should:
*   1) Call the my_username() function in autotest-validate.c to get your hard coded username.
*   2) Obtain the value returned from function malloc_username_from_conf_file() in username-from-conf-file.h within
*       the assignment autotest submodule at assignment-autotest/test/assignment1/
*   3) Use unity assertion TEST_ASSERT_EQUAL_STRING_MESSAGE to verify the two strings are equal.  See
*       the [unity assertion reference](https://github.com/ThrowTheSwitch/Unity/blob/master/docs/UnityAssertionsReference.md)
*/
void test_validate_my_username()
{
    /**
     * TODO: Replace the line below with your code here as described above to verify your /conf/username.txt 
     * config file and my_username() functions are setup properly
     */
    //TEST_ASSERT_TRUE_MESSAGE(false,"AESD students, please fix me!");
    // 获取硬编码的用户名
    const char *hard_coded_username = my_username();

    // 从配置文件中读取用户名
    char *username_from_file = malloc_username_from_conf_file();

    // 比较两个字符串是否相同，并打印一个消息如果它们不匹配
    TEST_ASSERT_EQUAL_STRING_MESSAGE(hard_coded_username, username_from_file, 
        "The username from conf file does not match the hard-coded username.");

    // 记得释放动态分配的内存
    free(username_from_file); 
}
