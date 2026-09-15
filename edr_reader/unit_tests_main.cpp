#define CATCH_CONFIG_MAIN 
#include <catch2/catch_test_macros.hpp>
#include "ring_buffer.hpp"

TEST_CASE("Items are added", "[buffer]") {
    BufferRingThreadSafe<std::string> buffer;
   for (int i=0;i<5;++i)
    {
        buffer.push("test string");
    }
    REQUIRE(buffer.size()==5);
}

TEST_CASE("Items are added and popped", "[buffer]") {
    BufferRingThreadSafe<std::string> buffer;
   for (int i=0;i<5;++i)
    {
        buffer.push("test string");
    }
   for (int i=0;i<5;++i)
    {
        buffer.pop();    
    } 
    REQUIRE(buffer.size()==0);
}

TEST_CASE("Items are limited to 10 by default", "[buffer]") {
    BufferRingThreadSafe<std::string> buffer;
    for (int i=0;i<11;++i)
    {
        buffer.push("test string");
    }

    REQUIRE(buffer.size()==10);
}





