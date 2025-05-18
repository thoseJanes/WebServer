#include <sys/prctl.h>


int main(int argc, char* argv[]){
    prctl(PR_SET_NAME, "123456789abcdefghijklmnopqrst");



    return 0;
}