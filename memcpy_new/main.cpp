#include <iostream>
#include <stdio.h>
#include <cstdio>
#include <cstring>

void *copyByteByByte(void *dest, const void *src, size_t count) {
    char *cdest = (char *) dest;
    const char *csrc = (char *) src;
    for (size_t i = 0; i < count; i++)
        *cdest++ = *csrc++;

    return dest;
}

void *copyWiki(void *dst, const void *src, size_t count) {
    for (int i = 0; i < count; i++)
        ((unsigned char *) dst)[i] = ((unsigned char *) src)[i];

    return dst;
}

void *copyLongByLong(void *dst, const void *src, size_t count) {
    //Данная версия копирует 4 или 8 байт (размер типа long равен 32 битам
    // на 32-битной платформе и 64 на 64-битной) за цикл, но не проверяет выровненность данных.

    auto *wdst = (unsigned long *) dst;  // текущая позиция в буфере назначения
    auto *wsrc = (unsigned long *) src;  // текущая позиция в источнике
    auto *cdst = (unsigned char *) dst;
    auto *csrc = (unsigned char *) src;

    for (size_t i = 0, m = count / sizeof(long); i < m; i++)  // копируем основную часть блоками по размеру long
        *(wdst++) = *(wsrc++);

    for (size_t i = 0, m = count % sizeof(long); i < m; i++)   // остаток копируем побайтно
        *(cdst++) = *(csrc++);

    return dst;
}

//копирование по 8 байт; rax - 64-битный регистр
// Директива .intel_syntax меняет  синтаксис AT&T на  синтаксис Intel
//noprefix - смена синтаксиса для операндов инструкций (при записи регистров можно не юзать %)
//Оба операнда должны иметь одинаковую размерность — байт, слово или двойное слово
//размер слова в x86x64 - 64 бита
//"r" - регистр общего назначения
void *copy8(void *dest, const void *src, size_t count) {
    char *cdest = (char *) dest;
    const char *csrc = (char *) src;
    size_t i = 0;
    for (; i < count; i += 8) {
        __asm__ (R"(
        .intel_syntax noprefix
        mov rax, [%0]
        mov [%1], rax
        .att_syntax prefix
        )"
        :
        :"r"(csrc + i), "r"(cdest + i) // input
        :"%rax"
        );
    }
    for (; i < count; ++i) {
        *(cdest + i) = *(csrc + i);
    }
    return dest;
}

//невыровненный =
//При сохранении какого-то объекта в памяти может случиться, что некое поле,
// состоящее из нескольких байтов, пересечёт «естественную границу» слов в памяти.
void *copy16_1(void *dest, const void *src, size_t count) {
    //Move Unaligned Double Quadword
    //Double Quadword - 128 - 16 байт; Quadword -  - 64 - 8 байт; Word  - 8 - 2 байта;
    // When the source or destination operand is a memory operand, the operand may
    // be unaligned on a 16-byte (Double Quadword) boundary without causing a general-protection exception (#GP)
    // to be generated.
    char *cdest = (char *) dest;
    const char *csrc = (char *) src;
    size_t i = 0;

    for (; i < count; i += 16) {
        __asm__ (R"(
        .intel_syntax noprefix
        movdqu xmm1, [%0]
        movdqu [%1], xmm1
        .att_syntax prefix
        )"
        :
        :"r"( csrc + i), "r"( cdest + i)
        :"%xmm1"
        );
    }
    for (; i < count; ++i) {
        *(cdest + i) = *(csrc + i);
    }
    return dest;
}

//выровненный
//movdqa специально для выровненных
void *copy16_2(void *dest, const void *src, size_t count) {
    char *cdest = (char *) dest;
    const char *csrc = (char *) src;
    size_t i = 0;

    bool end = false;
    //можно начинать копировать movdqa толко когда источник+i кратен 16,
    // до тех пор копируем побайтово
    //если слово заканчивается раньше, то и ладненько, значит, все скопировали
    //приемник не выравниваем


    while ((((unsigned long int) (csrc + i) & 15) != 0) && (i < count)) {
        *(cdest + i) = *(csrc + i);
        i++;

        if (i == count - 1) end = true; //скопировали всё
    }

    if (!end) {
        for (; i + 16 < count; i += 16) {

            __asm__ (R"(
        .intel_syntax noprefix
        movdqa xmm1, [%0]
        movdqu [%1], xmm1
        .att_syntax prefix
        )"
            :
            :"r"(csrc + i), "r"(cdest + i)
            :"%xmm1"
            );
        }

        for (; i < count; ++i) {
            *(cdest + i) = *(csrc + i);
        }

    }
    
    return dest;
}


int main() {

    bool testOk = true;
    for (int c = 0; c < 100; c++) {

        size_t N = 100;
        char src[N];
        char *dst = new char[N];

        srand(time(NULL) + c);
        for (int i = 0; i < N; i++)
            src[i] = rand() % 2 == 0 ? ' ' : 'a';


        for (int shiftS = 0; shiftS < N; shiftS++) {
            for (int shiftD = 0; shiftD < N; shiftD++) {

                copy16_2(dst + shiftD, src + shiftS, N);

                size_t k = std::min(N - shiftS, N - shiftD);

                for (int i = 0; i < k; i++)
                    if (src[shiftS + i] != dst[shiftD + i]) {
                        testOk = false;
                        break;
                    }
            }
        }
    }
    (testOk) ? printf("yes\n") : printf("no\n");

    return 0;
}