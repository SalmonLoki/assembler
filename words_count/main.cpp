#include <iostream>
#include <string>
#include <cstring>
#include <emmintrin.h>

using namespace std;


int check(const char *str, size_t size) {

    if (size == 0)
        return 0;
    int k = 0;
    int i = 0;
    while (str[i] == ' ')
        i++;
    for (; i < size; i++) {
        if (str[i] == ' ') {
            while (str[i] == ' ')
                i++;
            k++;
            i--;
        }
    }
    if (str[i - 1] != ' ') {
        k++;
    }

    return k;
}
//The __m128i data type, for use with the Streaming SIMD Extensions 2 (SSE2) instructions intrinsics, is defined in emmintrin.h
//A variable of type __m128i maps to the XMM[0-7] registers.
//Variables of type _m128i are automatically aligned on 16-byte boundaries.
//_mm_set1_epi8 = Set sixteen integer values
const __m128i SPACE = _mm_set1_epi8(' ');

int amount_with_m128i(char *str, size_t size) {
    int count = 0;
    size_t i = 0;
    bool check1 = false, check2 = false, check16 = false;
    char *check16Bound = nullptr, *check2Bound = nullptr;


    while ((size_t) (str + i) % 16 != 0 && i < size) {
        i++;
    }
    if (i != 0)
        check1 = true;

    count += check(str, i);

    if (i + 1 + 16 < size) {
        check16 = true;
        check16Bound = str + i;

        int k = 0;
        if (*(str + i) != ' ')
            k++;

        for (;  i + 1 + 16 < size; i += 16) {

            __m128i curString = _mm_cmpgt_epi8(_mm_load_si128((__m128i *) (str + i)), SPACE);

            __m128i stringRightShift = _mm_cmpgt_epi8(_mm_loadu_si128((__m128i *) (str + i + 1)),
                                                      SPACE);

            k += __builtin_popcount( _mm_movemask_epi8(_mm_xor_si128(stringRightShift, curString)));
        }

        if (  *(str + i ) != ' ' )
            k++;

        count += (k / 2);
    }

    if (check16) {
        if ((i + 1) < size) {
            check2 = true;
            check2Bound = str + i + 1;
            count += check(str + i + 1, size - (i + 1));
        }
    } else {
        if (i < size) {
            check2 = true;
            check2Bound = str + i;
            count += check(str + i, size - i);
        }
    }

    //проверка на стыках
    if (check1 && check16 && *(check16Bound - 1) != ' ' && *(check16Bound) != ' ')
        count--;
    if (check1 && !check16 && check2 && *(check2Bound - 1) != ' ' && *(check2Bound) != ' ')
        count--;
    if (check16 && check2 && *(check2Bound - 1) != ' ' && *(check2Bound) != ' ')
        count--;

    return count;
}


int main() {

    bool testOk = true;
    for (int c = 0; c < 50; c++) {

        size_t size = 30;
        char a[size];
        char *arr = a;

        srand(time(NULL) + c);
        for( int i = 0; i < size; i++)
            a[i] = rand() % 2 == 0 ? ' ' : 'a';


        for (int j = 0; j < size; j++) {

            if (check(arr + j, size - j)  !=  amount_with_m128i(arr + j, size - j)) {
                for (int i = j; i < size - j; i++)
                    cout << arr[i];
                cout << '\n';
                cout << check(arr + j, size - j)<< " "; cout << '\n';
                cout << amount_with_m128i(arr + j, size - j) << " ";
                cout << (size_t)(arr + j) % 16 << '\n' << '\n';
                testOk = false;
                break;
            }
        }
    }
    size_t size = 30;
    char a[size];
    char *arr = a;
    a[0] = ' ';
    for( int i = 1; i < size; i++)
        a[i] = rand() % 2 == 0 ? ' ' : 'a';
    for (int j = 0; j < size; j++)
        if (check(arr + j, size - j)  !=  amount_with_m128i(arr + j, size - j)) {
            testOk = false;
            break;
        }
    a[size-1] = ' ';
    for (int j = 0; j < size; j++)
        if (check(arr + j, size - j)  !=  amount_with_m128i(arr + j, size - j)) {
            testOk = false;
            break;
        }

        

    (testOk) ?  printf("yes\n") :  printf("no\n");

    return 0;
}