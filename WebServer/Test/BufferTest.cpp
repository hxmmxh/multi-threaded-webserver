#include "Buffer.h"

#include <iostream>
#include <type_traits> //is_same

using namespace std;
using namespace hxmmxh;
static int test_count = 0;
static int test_pass = 0;

#define CHECKEQUAL(expect, actual)                                                            \
    do                                                                                        \
    {                                                                                         \
        test_count++;                                                                         \
        if ((expect) == (actual))                                                             \
            test_pass++;                                                                      \
        else                                                                                  \
        {                                                                                     \
            std::cout << "line: " << __LINE__ << "error:   " << #expect << " != " << #actual; \
            std::cout << "    " #expect << " = " << expect << std::endl;                      \
        }                                                                                     \
    } while (0)

void testBufferAppendRetrieve()
{
    Buffer buf;
    CHECKEQUAL(buf.readableBytes(), 0);
    CHECKEQUAL(buf.writableBytes(), Buffer::kInitialSize);
    CHECKEQUAL(buf.prependableBytes(), Buffer::kCheapPrepend);

    const string str(200, 'x');
    buf.append(str);
    CHECKEQUAL(buf.readableBytes(), str.size());
    CHECKEQUAL(buf.writableBytes(), Buffer::kInitialSize - str.size());
    CHECKEQUAL(buf.prependableBytes(), Buffer::kCheapPrepend);

    const string str2 = buf.retrieveAsString(50);
    CHECKEQUAL(str2.size(), 50);
    CHECKEQUAL(buf.readableBytes(), str.size() - str2.size());
    CHECKEQUAL(buf.writableBytes(), Buffer::kInitialSize - str.size());
    CHECKEQUAL(buf.prependableBytes(), Buffer::kCheapPrepend + str2.size());
    CHECKEQUAL(str2, string(50, 'x'));

    buf.append(str);
    CHECKEQUAL(buf.readableBytes(), 2 * str.size() - str2.size());
    CHECKEQUAL(buf.writableBytes(), Buffer::kInitialSize - 2 * str.size());
    CHECKEQUAL(buf.prependableBytes(), Buffer::kCheapPrepend + str2.size());

    const string str3 = buf.retrieveAllAsString();
    CHECKEQUAL(str3.size(), 350); //200+200-50
    CHECKEQUAL(buf.readableBytes(), 0);
    CHECKEQUAL(buf.writableBytes(), Buffer::kInitialSize);
    CHECKEQUAL(buf.prependableBytes(), Buffer::kCheapPrepend);
    CHECKEQUAL(str3, string(350, 'x'));
}

void testBufferInsideGrow()
{
    Buffer buf;
    buf.append(string(800, 'y'));
    CHECKEQUAL(buf.readableBytes(), 800);
    CHECKEQUAL(buf.writableBytes(), Buffer::kInitialSize - 800);

    buf.retrieve(500);
    CHECKEQUAL(buf.readableBytes(), 300);
    CHECKEQUAL(buf.writableBytes(), Buffer::kInitialSize - 800);
    CHECKEQUAL(buf.prependableBytes(), Buffer::kCheapPrepend + 500);

    //这个操作会把readable区往前移，挤占prepend区域
    buf.append(string(300, 'z'));
    CHECKEQUAL(buf.readableBytes(), 600);
    CHECKEQUAL(buf.writableBytes(), Buffer::kInitialSize - 600);
    CHECKEQUAL(buf.prependableBytes(), Buffer::kCheapPrepend);
}

void testBufferShrink()
{
    Buffer buf;
    buf.append(string(2000, 'y'));
    CHECKEQUAL(buf.readableBytes(), 2000);
    CHECKEQUAL(buf.writableBytes(), 0);
    CHECKEQUAL(buf.prependableBytes(), Buffer::kCheapPrepend);
    CHECKEQUAL(buf.internalCapacity(), 2064);

    buf.retrieve(1500);
    CHECKEQUAL(buf.readableBytes(), 500);
    CHECKEQUAL(buf.writableBytes(), 0);
    CHECKEQUAL(buf.prependableBytes(), Buffer::kCheapPrepend + 1500);

    //收缩后的writableBytes为kInitialSize - readableBytes()和reserve之间的较大者
    buf.shrink(5); //1024-500=524
    CHECKEQUAL(buf.readableBytes(), 500);
    CHECKEQUAL(buf.writableBytes(), Buffer::kInitialSize - 500);
    CHECKEQUAL(buf.retrieveAllAsString(), string(500, 'y'));
    CHECKEQUAL(buf.prependableBytes(), Buffer::kCheapPrepend);

    buf.append(string(2000, 'y'));
    buf.retrieve(1500);
    buf.shrink(525);
    CHECKEQUAL(buf.writableBytes(), 525);
}

void testBufferPrepend()
{
    Buffer buf;
    buf.append(string(200, 'y'));
    CHECKEQUAL(buf.readableBytes(), 200);
    CHECKEQUAL(buf.writableBytes(), Buffer::kInitialSize - 200);
    CHECKEQUAL(buf.prependableBytes(), Buffer::kCheapPrepend);

    int x = 0;
    buf.prepend(&x, sizeof(x));
    CHECKEQUAL(buf.readableBytes(), 204);
    CHECKEQUAL(buf.writableBytes(), Buffer::kInitialSize - 200);
    CHECKEQUAL(buf.prependableBytes(), Buffer::kCheapPrepend - 4);
}

void testBufferReadInt()
{
    Buffer buf;
    buf.append("HTTP");

    CHECKEQUAL(buf.readableBytes(), 4);
    CHECKEQUAL(buf.peekInt8(), 'H');
    int top16 = buf.peekInt16();
    CHECKEQUAL(top16, 'H' * 256 + 'T');
    CHECKEQUAL(buf.peekInt32(), top16 * 65536 + 'T' * 256 + 'P');

    CHECKEQUAL(buf.readInt8(), 'H');
    CHECKEQUAL(buf.readInt16(), 'T' * 256 + 'T');
    CHECKEQUAL(buf.readInt8(), 'P');
    CHECKEQUAL(buf.readableBytes(), 0);
    CHECKEQUAL(buf.writableBytes(), Buffer::kInitialSize);

    buf.appendInt8(-1);
    buf.appendInt16(-2);
    buf.appendInt32(-3);
    CHECKEQUAL(buf.readableBytes(), 7); //1+2+4
    CHECKEQUAL(buf.readInt8(), -1);
    CHECKEQUAL(buf.readInt16(), -2);
    CHECKEQUAL(buf.readInt32(), -3);
}

int main()
{
    testBufferAppendRetrieve();
    testBufferInsideGrow();
    testBufferShrink();
    testBufferPrepend();
    testBufferReadInt();
    std::cout << test_pass << "/" << test_count << " passed\n";
}