#include <iostream>
#include <mutex>
#include <Windows.h>
#include <map>

std::mutex memLock;
//-----------------------------
// 
//-----------------------------
class LinerAllocator {
public:
    LinerAllocator() {
        LPVOID allocMemory = VirtualAlloc(0, 100000000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        this->begin = allocMemory;
     
    }

    void* alloc(size_t size);
    void  free(void* pool,size_t size);

private:
    void* begin;
    std::map<size_t, void*> sizeMemory;// size -> memory
};

void* LinerAllocator::alloc(size_t size)
{
    std::lock_guard<std::mutex> lock(memLock);
    if (!begin) {
        LPVOID allocMemory = VirtualAlloc(0, 100000000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        this->begin = allocMemory;
    }

    for (auto& a : sizeMemory) {
        if (a.first >= size) {
            auto ret = a.second;
            sizeMemory.erase(a.first);
            return ret;
        }
    }

    auto allocBlock = begin;
    begin = (void*)((uintptr_t)begin + size);
    return allocBlock;
}

void LinerAllocator::free(void* pool,size_t size) {
    std::lock_guard<std::mutex> lock(memLock);

    sizeMemory[size] = pool;
    memset(pool, 0, size);
}

LinerAllocator linerAllocator;

class AllocateTest {
public:

    __inline AllocateTest(const int& age,const char* name):m_iAge(age),m_name(name)  {

    }

    //===================================================

    void* operator new(size_t size) {
        return linerAllocator.alloc(size);
    }
    void operator delete(void* freeMem) {
        linerAllocator.free(freeMem, sizeof(AllocateTest));
    }

    //===================================================

    __inline int& get_age() {
        return m_iAge;
    }

    __inline const char* get_name() {
        return m_name;
    }
private:
    int m_iAge;
    const char* m_name;

};
int main() {
    
    AllocateTest* test = new AllocateTest(1337, "zxc gul");

    std::cout << " pointer = " << test << " age " << test->get_age() << " name " << test->get_name() << std::endl;

    delete test;

}