make exec_sql

cd ../src/execution
pass="Pass Success!";

chmod 755 task1_test.sh
result=$(./task1_test.sh | tail -n 1)
if [ "$result" == "$pass" ];then
    echo -e "\033[34mlab03 task1 passed\033[0m"
else
    echo -e "\033[34mlab03 task1 failed\033[0m"
fi

chmod 755 task2_test.sh
result=$(./task2_test.sh | tail -n 1)
if [ "$result" == "$pass" ];then
    echo -e "\033[34mlab03 task2 passed\033[0m"
else 
    echo -e "\033[34mlab03 task2 failed\033[0m"
fi

chmod 755 task3_test.sh
result=$(./task3_test.sh | tail -n 1)
if [ "$result" == "$pass" ];then
    echo -e "\033[34mlab03 task3 passed\033[0m"
else 
    echo -e "\033[34mlab03 task3 failed\033[0m"
fi

chmod 755 taskall_test.sh
result=$(./taskall_test.sh | tail -n 1)
if [ "$result" == "$pass" ];then
    echo -e "\033[34mlab03 taskall passed\033[0m"
else 
    echo -e "\033[34mlab03 taskall failed\033[0m"
fi
cd -

